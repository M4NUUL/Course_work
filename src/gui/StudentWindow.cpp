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

StudentWindow::StudentWindow(int studentId, QWidget *parent)
    : QWidget(parent), m_studentId(studentId)
{
    setWindowTitle("Jumandgi — Студент");
    resize(900, 700);

    auto v = new QVBoxLayout(this);

    // Assignments
    tblAssignments = new QTableWidget(this);
    tblAssignments->setColumnCount(3);
    tblAssignments->setHorizontalHeaderLabels({"ID","Задание","Дедлайн"});
    tblAssignments->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblAssignments->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblAssignments->horizontalHeader()->setStretchLastSection(true);
    v->addWidget(new QLabel("Доступные задания:"));
    v->addWidget(tblAssignments, 1);

    // Buttons for assignments
    auto h1 = new QHBoxLayout();
    btnUpload = new QPushButton("Загрузить работу", this);
    h1->addWidget(btnUpload);
    h1->addStretch();
    v->addLayout(h1);

    // My submissions
    tblMySubmissions = new QTableWidget(this);
    tblMySubmissions->setColumnCount(5);
    tblMySubmissions->setHorizontalHeaderLabels({"ID","Задание","Файл","Загружено","Оценка / Комментарий"});
    tblMySubmissions->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblMySubmissions->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblMySubmissions->horizontalHeader()->setStretchLastSection(true);
    v->addWidget(new QLabel("Мои отправления:"));
    v->addWidget(tblMySubmissions, 1);

    // Download my submission button
    auto h2 = new QHBoxLayout();
    auto btnDownloadMy = new QPushButton("Скачать выбранную отправку", this);
    h2->addWidget(btnDownloadMy);
    h2->addStretch();
    v->addLayout(h2);

    connect(btnUpload, &QPushButton::clicked, this, &StudentWindow::onUpload);
    connect(tblAssignments, &QTableWidget::cellDoubleClicked, this, &StudentWindow::onAssignmentDoubleClicked);
    connect(btnDownloadMy, &QPushButton::clicked, this, &StudentWindow::onDownloadMySubmission);

    // initial load
    loadAssignments();
    loadMySubmissions();
}

void StudentWindow::loadAssignments() {
    tblAssignments->setRowCount(0);
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);

    q.prepare("SELECT id, title, due_date FROM sp_get_assignments_for_student(?)");
    q.addBindValue(m_studentId);
    if (!q.exec()) {
        qWarning() << "sp_get_assignments_for_student failed:" << q.lastError().text();
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить задания: " + q.lastError().text());
        return;
    }

    int r = 0;
    while (q.next()) {
        tblAssignments->insertRow(r);
        tblAssignments->setItem(r,0, new QTableWidgetItem(QString::number(q.value(0).toInt())));
        tblAssignments->setItem(r,1, new QTableWidgetItem(q.value(1).toString()));
        tblAssignments->setItem(r,2, new QTableWidgetItem(q.value(2).toString()));
        r++;
    }
    tblAssignments->resizeColumnsToContents();
}

void StudentWindow::loadMySubmissions() {
    tblMySubmissions->setRowCount(0);
    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT id, assignment_id, original_name, uploaded_at, grade, feedback, file_path FROM sp_get_my_submissions(?)");
    q.addBindValue(m_studentId);
    if (!q.exec()) {
        qWarning() << "sp_get_my_submissions failed:" << q.lastError().text();
        return;
    }
    int r = 0;
    while (q.next()) {
        tblMySubmissions->insertRow(r);
        tblMySubmissions->setItem(r,0, new QTableWidgetItem(QString::number(q.value(0).toInt())));
        tblMySubmissions->setItem(r,1, new QTableWidgetItem(QString::number(q.value(1).toInt())));
        tblMySubmissions->setItem(r,2, new QTableWidgetItem(q.value(2).toString()));
        tblMySubmissions->setItem(r,3, new QTableWidgetItem(q.value(3).toString()));
        QString grade = q.value(4).isNull() ? QString() : q.value(4).toString();
        QString fb = q.value(5).isNull() ? QString() : q.value(5).toString();
        tblMySubmissions->setItem(r,4, new QTableWidgetItem(grade + (fb.isEmpty() ? "" : " / " + fb)));
        tblMySubmissions->item(r,2)->setData(Qt::UserRole, q.value(6).toString());
        r++;
    }
    tblMySubmissions->resizeColumnsToContents();
}

static QString findCreateSubmissionExe() {
    QString appDir = QCoreApplication::applicationDirPath();
    QString exe = QDir(appDir).filePath("create_submission");
    if (QFileInfo(exe).exists() && QFileInfo(exe).isExecutable()) return exe;
    QString alt = QDir(appDir).filePath("../build/create_submission");
    if (QFileInfo(alt).exists() && QFileInfo(alt).isExecutable()) return alt;
    if (QFileInfo("create_submission").exists() && QFileInfo("create_submission").isExecutable()) return QString("create_submission");
    return QString();
}

void StudentWindow::onUpload() {
    auto sel = tblAssignments->selectedItems();
    if (sel.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите задание");
        return;
    }
    int row = tblAssignments->currentRow();
    if (row < 0) { QMessageBox::warning(this, "Ошибка", "Выберите задание"); return; }
    int assignmentId = tblAssignments->item(row,0)->text().toInt();

    QString file = QFileDialog::getOpenFileName(this, "Выберите файл для загрузки");
    if (file.isEmpty()) return;

    QString exe = findCreateSubmissionExe();
    if (exe.isEmpty()) {
        QMessageBox::critical(this, "Ошибка", "Не найден исполняемый файл create_submission рядом с приложением.");
        return;
    }

    QStringList args;
    args << file << QString::number(m_studentId) << QString::number(assignmentId) << QFileInfo(file).fileName();

    int rc = QProcess::execute(exe, args);
    if (rc != 0) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить файл (create_submission вернул ошибку)");
        qWarning() << "create_submission failed, rc=" << rc << "exe=" << exe << "args=" << args;
        return;
    }

    QMessageBox::information(this, "OK", "Файл успешно отправлен");
    Logger::log(m_studentId, "upload_submission", QString("assignment=%1 file=%2").arg(assignmentId).arg(QFileInfo(file).fileName()));
    loadMySubmissions();
}

void StudentWindow::onAssignmentDoubleClicked(int row, int) {
    if (row < 0) return;
    int assignmentId = tblAssignments->item(row,0)->text().toInt();
    AssignmentDetailDialog dlg(assignmentId, this);
    dlg.exec();
}

void StudentWindow::onDownloadMySubmission() {
    auto sel = tblMySubmissions->selectedItems();
    if (sel.isEmpty()) { QMessageBox::warning(this, "Ошибка", "Выберите отправление"); return; }
    int row = tblMySubmissions->currentRow();

    QString filePath = tblMySubmissions->item(row,2)->data(Qt::UserRole).toString();
    QString originalName = tblMySubmissions->item(row,2)->text();
    if (filePath.isEmpty()) { QMessageBox::warning(this,"Ошибка","Путь к файлу отсутствует"); return; }

    QString encFilePath = QString("storage/files/%1").arg(filePath);
    if (!QFileInfo::exists(encFilePath)) { QMessageBox::warning(this,"Ошибка","Файл не найден: " + encFilePath); return; }

    QString uuid = QFileInfo(filePath).baseName();
    QString metaPath = QString("storage/metadata/%1.json").arg(uuid);
    QString tmpPath = QDir::temp().filePath(QString("%1_%2").arg(uuid).arg(originalName));
    tmpPath = tmpPath.replace("/", "_");

    if (QFileInfo(metaPath).exists()) {
        QFile mf(metaPath);
        if (!mf.open(QIODevice::ReadOnly)) { QMessageBox::warning(this,"Ошибка","Не удалось открыть metadata"); return; }
        auto metaRaw = mf.readAll();
        mf.close();
        QJsonDocument jd = QJsonDocument::fromJson(metaRaw);
        if (!jd.isObject()) { QMessageBox::warning(this,"Ошибка","Неверный metadata"); return; }
        QJsonObject mo = jd.object();

        QByteArray encKeyB64 = QByteArray::fromBase64(mo.value("key_encrypted").toString().toUtf8());
        QByteArray keyIvB64  = QByteArray::fromBase64(mo.value("key_iv").toString().toUtf8());
        QByteArray keyTagB64 = QByteArray::fromBase64(mo.value("key_tag").toString().toUtf8());
        QByteArray fileIvB64 = QByteArray::fromBase64(mo.value("iv").toString().toUtf8());

        std::vector<unsigned char> encKey((unsigned char*)encKeyB64.data(), (unsigned char*)encKeyB64.data() + encKeyB64.size());
        std::vector<unsigned char> keyIv((unsigned char*)keyIvB64.data(), (unsigned char*)keyIvB64.data() + keyIvB64.size());
        std::vector<unsigned char> keyTag((unsigned char*)keyTagB64.data(), (unsigned char*)keyTagB64.data() + keyTagB64.size());
        std::vector<unsigned char> fileIv((unsigned char*)fileIvB64.data(), (unsigned char*)fileIvB64.data() + fileIvB64.size());

        auto master = ConfigManager::instance().masterKey();
        if (master.empty()) { QMessageBox::warning(this,"Ошибка","Мастер-ключ не загружен"); return; }

        std::string err;
        std::vector<unsigned char> fileKey;
        if (!keyprotect::decryptWithAesGcm(master, encKey, keyIv, keyTag, fileKey, err)) {
            QMessageBox::critical(this, "Ошибка дешифрования ключа", QString::fromStdString(err));
            return;
        }

        std::string serr;
        if (!crypto::aes256_cbc_decrypt(fileKey, fileIv, encFilePath.toStdString(), tmpPath.toStdString(), serr)) {
            QMessageBox::critical(this, "Ошибка расшифровки файла", QString::fromStdString(serr));
            return;
        }
    } else {
        QFile::remove(tmpPath);
        if (!QFile::copy(encFilePath, tmpPath)) {
            QMessageBox::warning(this,"Ошибка","Не удалось подготовить файл для открытия");
            return;
        }
    }

    // гарантируем абсолютный путь
QFileInfo tf(tmpPath);
QString absTmp = tf.absoluteFilePath();
if (absTmp.isEmpty()) absTmp = tmpPath;

// поставим права на чтение для текущего пользователя (и группы/остальных по желанию)
QFile::Permissions perms = QFile::permissions(absTmp);
perms |= QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther;
QFile::setPermissions(absTmp, perms);

// Попробуем сначала надежно запустить xdg-open (часто работает лучше в окружениях Linux)
bool launched = QProcess::startDetached(QStringLiteral("xdg-open"), QStringList() << absTmp);
if (!launched) {
    // Если xdg-open не сработал, пробуем QDesktopServices
    bool opened = QDesktopServices::openUrl(QUrl::fromLocalFile(absTmp));
    if (!opened) {
        // Оба варианта не сработали — покажем пользователю путь, чтобы он открыл вручную
        QMessageBox::warning(this, "Ошибка", "Не удалось автоматически открыть файл. Откройте вручную: " + absTmp);
        qWarning() << "Failed to open file with xdg-open and QDesktopServices:" << absTmp;
        return;
    }
}

}
