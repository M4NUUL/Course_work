#include "StudentWindow.hpp"
#include "AssignmentDetailDialog.hpp"

#include "../db/Database.hpp"
#include "../config/ConfigManager.hpp"
#include "../crypto/KeyProtect.hpp"
#include "../crypto/FileCrypto.hpp"
#include "../utils/Logger.hpp"

#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>

#include <QSqlQuery>
#include <QSqlError>

#include <QFileDialog>
#include <QMessageBox>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QFileDevice>

StudentWindow::StudentWindow(int studentId, QWidget *parent)
    : QWidget(parent), m_studentId(studentId)
{
    setWindowTitle(QStringLiteral("EduDesk — Студент"));
    resize(900, 700);

    auto *v = new QVBoxLayout(this);

    tblAssignments = new QTableWidget(this);
    tblAssignments->setColumnCount(2);
    tblAssignments->setHorizontalHeaderLabels({QStringLiteral("Задание"), QStringLiteral("Дедлайн")});
    tblAssignments->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblAssignments->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblAssignments->horizontalHeader()->setStretchLastSection(true);
    v->addWidget(new QLabel(QStringLiteral("Доступные задания:"), this));
    v->addWidget(tblAssignments, 1);

    auto *h1 = new QHBoxLayout();
    btnUpload = new QPushButton(QStringLiteral("Загрузить работу"), this);
    h1->addWidget(btnUpload);
    h1->addStretch();
    v->addLayout(h1);

    tblMySubmissions = new QTableWidget(this);
    tblMySubmissions->setColumnCount(4);
    tblMySubmissions->setHorizontalHeaderLabels({
        QStringLiteral("Задание"),
        QStringLiteral("Файл"),
        QStringLiteral("Загружено"),
        QStringLiteral("Оценка / Комментарий")
    });
    tblMySubmissions->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblMySubmissions->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblMySubmissions->horizontalHeader()->setStretchLastSection(true);
    v->addWidget(new QLabel(QStringLiteral("Мои отправления:"), this));
    v->addWidget(tblMySubmissions, 1);

    auto *h2 = new QHBoxLayout();
    auto *btnDownloadMy = new QPushButton(QStringLiteral("Скачать выбранную отправку"), this);
    h2->addWidget(btnDownloadMy);
    h2->addStretch();
    v->addLayout(h2);

    connect(btnUpload, &QPushButton::clicked, this, &StudentWindow::onUpload);
    connect(tblAssignments, &QTableWidget::cellDoubleClicked, this, &StudentWindow::onAssignmentDoubleClicked);
    connect(btnDownloadMy, &QPushButton::clicked, this, &StudentWindow::onDownloadMySubmission);

    loadAssignments();
    loadMySubmissions();
}

void StudentWindow::loadAssignments() {
    tblAssignments->setRowCount(0);

    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT id, title, due_date FROM sp_get_assignments_for_student(?)");
    q.addBindValue(m_studentId);

    if (!q.exec()) {
        qWarning() << "sp_get_assignments_for_student failed:" << q.lastError().text();
        QMessageBox::warning(this, QStringLiteral("Ошибка"),
                             QStringLiteral("Не удалось загрузить задания: ") + q.lastError().text());
        return;
    }

    int r = 0;
    while (q.next()) {
        const int assignmentId = q.value(0).toInt();
        const QString title = q.value(1).toString();
        const QVariant dueVar = q.value(2);

        QString deadlineText;
        if (dueVar.canConvert<QDateTime>()) {
            deadlineText = dueVar.toDateTime().toLocalTime().toString("dd.MM.yyyy HH:mm");
        } else {
            deadlineText = dueVar.toString();
        }

        tblAssignments->insertRow(r);

        auto *titleItem = new QTableWidgetItem(title);
        titleItem->setData(Qt::UserRole, assignmentId);
        tblAssignments->setItem(r, 0, titleItem);

        tblAssignments->setItem(r, 1, new QTableWidgetItem(deadlineText));

        ++r;
    }

    tblAssignments->resizeColumnsToContents();
}

void StudentWindow::loadMySubmissions() {
    tblMySubmissions->setRowCount(0);

    QSqlQuery q(Database::instance().get());
    q.prepare(
        "SELECT id, assignment_id, assignment_title, original_name, uploaded_at, grade, feedback, file_path "
        "FROM sp_get_my_submissions(?)"
    );
    q.addBindValue(m_studentId);

    if (!q.exec()) {
        qWarning() << "sp_get_my_submissions failed:" << q.lastError().text();
        return;
    }

    int r = 0;
    while (q.next()) {
        const int subId = q.value(0).toInt();
        const int assignmentId = q.value(1).toInt();
        const QString assignmentTitle = q.value(2).toString();
        const QString origName = q.value(3).toString();
        const QVariant uploadedVar = q.value(4);

        QString uploadedText;
        if (uploadedVar.canConvert<QDateTime>()) {
            uploadedText = uploadedVar.toDateTime().toLocalTime().toString("dd.MM.yyyy HH:mm");
        } else {
            uploadedText = uploadedVar.toString();
        }

        const QString grade = q.value(5).isNull() ? QString() : q.value(5).toString();
        const QString fb = q.value(6).isNull() ? QString() : q.value(6).toString();

        QString gf = grade;
        if (!fb.isEmpty()) {
            if (!gf.isEmpty()) gf += " / ";
            gf += fb;
        }

        const QString filePath = q.value(7).toString();

        tblMySubmissions->insertRow(r);

        auto *asItem = new QTableWidgetItem(assignmentTitle);
        asItem->setData(Qt::UserRole, assignmentId);
        tblMySubmissions->setItem(r, 0, asItem);

        auto *fileItem = new QTableWidgetItem(origName);
        fileItem->setData(Qt::UserRole, subId);
        fileItem->setData(Qt::UserRole + 1, filePath);
        tblMySubmissions->setItem(r, 1, fileItem);

        tblMySubmissions->setItem(r, 2, new QTableWidgetItem(uploadedText));
        tblMySubmissions->setItem(r, 3, new QTableWidgetItem(gf));

        ++r;
    }

    tblMySubmissions->resizeColumnsToContents();
}

static QString findCreateSubmissionExe() {
    const QString appDir = QCoreApplication::applicationDirPath();

    const QString exe = QDir(appDir).filePath("create_submission");
    if (QFileInfo(exe).exists() && QFileInfo(exe).isExecutable()) return exe;

    const QString alt = QDir(appDir).filePath("../build/create_submission");
    if (QFileInfo(alt).exists() && QFileInfo(alt).isExecutable()) return alt;

    if (QFileInfo("create_submission").exists() && QFileInfo("create_submission").isExecutable()) {
        return QStringLiteral("create_submission");
    }

    return QString();
}

void StudentWindow::onUpload() {
    const int row = tblAssignments->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Выберите задание"));
        return;
    }

    auto *titleItem = tblAssignments->item(row, 0);
    if (!titleItem) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Выберите задание"));
        return;
    }

    const int assignmentId = titleItem->data(Qt::UserRole).toInt();
    if (assignmentId <= 0) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Некорректное задание"));
        return;
    }

    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Выберите файл для загрузки"));
    if (file.isEmpty()) return;

    const QString exe = findCreateSubmissionExe();
    if (exe.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Ошибка"),
                              QStringLiteral("Не найден исполняемый файл create_submission рядом с приложением."));
        return;
    }

    QStringList args;
    args << file
         << QString::number(m_studentId)
         << QString::number(assignmentId)
         << QFileInfo(file).fileName();

    const int rc = QProcess::execute(exe, args);
    if (rc != 0) {
        QMessageBox::critical(this, QStringLiteral("Ошибка"),
                              QStringLiteral("Не удалось загрузить файл (create_submission вернул ошибку)"));
        qWarning() << "create_submission failed, rc=" << rc << "exe=" << exe << "args=" << args;
        return;
    }

    QMessageBox::information(this, QStringLiteral("OK"), QStringLiteral("Файл успешно отправлен"));
    Logger::log(m_studentId, "upload_submission",
                QString("assignment=%1 file=%2").arg(assignmentId).arg(QFileInfo(file).fileName()));
    loadMySubmissions();
}

void StudentWindow::onAssignmentDoubleClicked(int row, int) {
    if (row < 0) return;

    auto *titleItem = tblAssignments->item(row, 0);
    if (!titleItem) return;

    const int assignmentId = titleItem->data(Qt::UserRole).toInt();
    if (assignmentId <= 0) return;

    AssignmentDetailDialog dlg(assignmentId, this);
    dlg.exec();
}

void StudentWindow::onDownloadMySubmission() {
    const int row = tblMySubmissions->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Выберите отправление"));
        return;
    }

    auto *fileItem = tblMySubmissions->item(row, 1);
    if (!fileItem) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Выберите отправление"));
        return;
    }

    const QString filePath = fileItem->data(Qt::UserRole + 1).toString();
    const QString originalName = fileItem->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Путь к файлу отсутствует"));
        return;
    }

    const QString encFilePath = QStringLiteral("storage/files/%1").arg(filePath);
    if (!QFileInfo::exists(encFilePath)) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Файл не найден: ") + encFilePath);
        return;
    }

    const QString uuid = QFileInfo(filePath).baseName();
    const QString metaPath = QStringLiteral("storage/metadata/%1.json").arg(uuid);

    QString safeName = QFileInfo(originalName).fileName();
    safeName.replace("/", "_");
    safeName.replace("\\", "_");

    QString tmpPath = QDir::temp().filePath(QStringLiteral("%1_%2").arg(uuid, safeName));
    QFile::remove(tmpPath);

    const QString absTmp = QFileInfo(tmpPath).absoluteFilePath();

    if (QFileInfo(metaPath).exists()) {
        QFile mf(metaPath);
        if (!mf.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось открыть metadata"));
            return;
        }
        const QByteArray metaRaw = mf.readAll();
        mf.close();

        const QJsonDocument jd = QJsonDocument::fromJson(metaRaw);
        if (!jd.isObject()) {
            QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Неверный metadata"));
            return;
        }
        const QJsonObject mo = jd.object();

        const QByteArray encKeyB64 = QByteArray::fromBase64(mo.value("key_encrypted").toString().toUtf8());
        const QByteArray keyIvB64  = QByteArray::fromBase64(mo.value("key_iv").toString().toUtf8());
        const QByteArray keyTagB64 = QByteArray::fromBase64(mo.value("key_tag").toString().toUtf8());
        const QByteArray fileIvB64 = QByteArray::fromBase64(mo.value("iv").toString().toUtf8());

        std::vector<unsigned char> encKey((unsigned char*)encKeyB64.data(), (unsigned char*)encKeyB64.data() + encKeyB64.size());
        std::vector<unsigned char> keyIv((unsigned char*)keyIvB64.data(), (unsigned char*)keyIvB64.data() + keyIvB64.size());
        std::vector<unsigned char> keyTag((unsigned char*)keyTagB64.data(), (unsigned char*)keyTagB64.data() + keyTagB64.size());
        std::vector<unsigned char> fileIv((unsigned char*)fileIvB64.data(), (unsigned char*)fileIvB64.data() + fileIvB64.size());

        const auto master = ConfigManager::instance().masterKey();
        if (master.empty()) {
            QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Мастер-ключ не загружен"));
            return;
        }

        std::string err;
        std::vector<unsigned char> fileKey;
        if (!keyprotect::decryptWithAesGcm(master, encKey, keyIv, keyTag, fileKey, err)) {
            QMessageBox::critical(this, QStringLiteral("Ошибка дешифрования ключа"), QString::fromStdString(err));
            return;
        }

        std::string serr;
        if (!crypto::aes256_cbc_decrypt(fileKey, fileIv, encFilePath.toStdString(), tmpPath.toStdString(), serr)) {
            QMessageBox::critical(this, QStringLiteral("Ошибка расшифровки файла"), QString::fromStdString(serr));
            return;
        }
    } else {
        if (!QFile::copy(encFilePath, tmpPath)) {
            QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось подготовить файл для открытия"));
            return;
        }
    }

    QFile::Permissions perms = QFile::permissions(tmpPath);
    perms |= QFileDevice::ReadOwner | QFileDevice::WriteOwner
          | QFileDevice::ReadGroup | QFileDevice::ReadOther;
    QFile::setPermissions(tmpPath, perms);

    if (QProcess::startDetached(QStringLiteral("xdg-open"), QStringList() << absTmp)) return;
    if (QProcess::startDetached(QStringLiteral("gedit"), QStringList() << absTmp)) return;
    if (QDesktopServices::openUrl(QUrl::fromLocalFile(absTmp))) return;

    QMessageBox::warning(this, QStringLiteral("Ошибка"),
                         QStringLiteral("Не удалось автоматически открыть файл. Откройте вручную: ") + absTmp);
    qWarning() << "Failed to open decrypted file:" << absTmp;
}
