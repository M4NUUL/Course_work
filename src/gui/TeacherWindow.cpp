#include "TeacherWindow.hpp"

#include "../db/Database.hpp"
#include "../config/ConfigManager.hpp"
#include "../crypto/KeyProtect.hpp"
#include "../crypto/FileCrypto.hpp"
#include "../utils/Logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTemporaryFile>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QDir>
#include <QByteArray>
#include <QFileInfo>
#include <QFileDialog>
#include <QDateTime>
#include <QListWidget>
#include <QDialog>
#include <QPushButton>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QProcess>
#include <QDebug>
#include <QCryptographicHash>

TeacherWindow::TeacherWindow(int teacherId, QWidget *parent) : QWidget(parent), m_teacherId(teacherId) {
    setWindowTitle("EduDesk — Интерфейс преподавателя");
    resize(1000,600);

    auto v = new QVBoxLayout(this);
    auto htop = new QHBoxLayout();

    // Assignments table
    tblAssignments = new QTableWidget();
    tblAssignments->setColumnCount(2);
    tblAssignments->setHorizontalHeaderLabels({"ID","Задание"});
    tblAssignments->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblAssignments->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblAssignments->horizontalHeader()->setStretchLastSection(true);
    connect(tblAssignments, &QTableWidget::cellClicked, this, &TeacherWindow::onAssignmentSelected);

    // Submissions table
    tblSubmissions = new QTableWidget();
    tblSubmissions->setColumnCount(5);
    tblSubmissions->setHorizontalHeaderLabels({"ID","Студент","Файл","Загружено","Оценка / Комментарий"});
    tblSubmissions->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblSubmissions->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblSubmissions->horizontalHeader()->setStretchLastSection(true);

    htop->addWidget(tblAssignments, 1);
    htop->addWidget(tblSubmissions, 2);

    // Buttons
    btnRefresh = new QPushButton("Обновить");
    btnDownload = new QPushButton("Скачать/Открыть");
    btnGrade = new QPushButton("Оценить / Комментарий");
    btnCreateAssignment = new QPushButton("Создать задание");
    btnDeleteAssignment = new QPushButton("Удалить задание");

    connect(btnRefresh, &QPushButton::clicked, this, &TeacherWindow::loadAssignments);
    connect(btnDownload, &QPushButton::clicked, this, &TeacherWindow::onDownloadSubmission);
    connect(btnGrade, &QPushButton::clicked, this, &TeacherWindow::onGradeSubmission);
    connect(btnCreateAssignment, &QPushButton::clicked, this, &TeacherWindow::onCreateAssignment);
    connect(btnDeleteAssignment, &QPushButton::clicked, this, &TeacherWindow::onDeleteAssignment);

    auto hbot = new QHBoxLayout();
    hbot->addWidget(btnCreateAssignment);
    hbot->addWidget(btnDeleteAssignment);
    hbot->addWidget(btnRefresh);
    hbot->addWidget(btnDownload);
    hbot->addWidget(btnGrade);
    hbot->addStretch();

    v->addLayout(htop);
    v->addLayout(hbot);

    loadAssignments();
}

void TeacherWindow::loadAssignments() {
    tblAssignments->setRowCount(0);
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("SELECT id, title FROM assignments WHERE created_by = ?");
    q.addBindValue(m_teacherId);
    if (!q.exec()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить задания: " + q.lastError().text());
        return;
    }
    int row = 0;
    while (q.next()) {
        tblAssignments->insertRow(row);
        tblAssignments->setItem(row,0, new QTableWidgetItem(QString::number(q.value(0).toInt())));
        tblAssignments->setItem(row,1, new QTableWidgetItem(q.value(1).toString()));
        row++;
    }
    tblAssignments->resizeColumnsToContents();
}

void TeacherWindow::onAssignmentSelected(int row, int) {
    if (row < 0) return;
    auto item = tblAssignments->item(row,0);
    if (!item) return;
    int assignmentId = item->text().toInt();
    currentAssignmentId = assignmentId;
    loadSubmissions(assignmentId);
}

void TeacherWindow::clearSubmissions() {
    tblSubmissions->setRowCount(0);
}

void TeacherWindow::loadSubmissions(int assignmentId) {
    clearSubmissions();
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    // Select grade and feedback and file_path for download
    q.prepare("SELECT s.id, u.login, s.original_name, s.uploaded_at, s.grade, s.feedback, s.file_path FROM submissions s LEFT JOIN users u ON s.student_id = u.id WHERE s.assignment_id = ?");
    q.addBindValue(assignmentId);
    if (!q.exec()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить отправления: " + q.lastError().text());
        return;
    }
    int row = 0;
    while (q.next()) {
    tblSubmissions->insertRow(row);

    int subId      = q.value(0).toInt();
    QString login  = q.value(1).toString();
    QString orig   = q.value(2).toString();
    QVariant uploadedVar = q.value(3);

    QString uploadedText;
    if (uploadedVar.canConvert<QDateTime>()) {
        QDateTime dt = uploadedVar.toDateTime().toLocalTime();
        uploadedText = dt.toString("dd.MM.yyyy HH:mm");
    } else {
        uploadedText = uploadedVar.toString();
    }

    QString grade    = q.value(4).isNull() ? QString() : q.value(4).toString();
    QString feedback = q.value(5).isNull() ? QString() : q.value(5).toString();
    QString gf = grade;
    if (!feedback.isEmpty()) {
        if (!gf.isEmpty()) gf += " / ";
        gf += feedback;
    }

    tblSubmissions->setItem(row, 0, new QTableWidgetItem(QString::number(subId)));
    tblSubmissions->setItem(row, 1, new QTableWidgetItem(login));
    tblSubmissions->setItem(row, 2, new QTableWidgetItem(orig));
    tblSubmissions->setItem(row, 3, new QTableWidgetItem(uploadedText));
    tblSubmissions->setItem(row, 4, new QTableWidgetItem(gf));

    tblSubmissions->item(row, 2)->setData(Qt::UserRole, q.value(6).toString());
    ++row;
    }

    tblSubmissions->resizeColumnsToContents();
}

void TeacherWindow::onDownloadSubmission() {
    auto sel = tblSubmissions->selectedItems();
    if (sel.isEmpty()) { QMessageBox::warning(this,"Ошибка","Выберите отправление"); return; }
    int row = tblSubmissions->currentRow();
    int subId = tblSubmissions->item(row,0)->text().toInt();

    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("SELECT file_path, original_name FROM submissions WHERE id = ?");
    q.addBindValue(subId);
    if (!q.exec() || !q.next()) {
        QMessageBox::warning(this,"Ошибка","Запись не найдена");
        return;
    }
    QString filePath = q.value(0).toString();
    QString originalName = q.value(1).toString();

    // Read metadata and decrypt key
    QString uuid = QFileInfo(filePath).baseName();
    QString metaPath = QString("storage/metadata/%1.json").arg(uuid);
    QFile mf(metaPath);
    if (!mf.open(QIODevice::ReadOnly)) { QMessageBox::warning(this,"Ошибка","Metadata не найден"); return; }
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

    QString encFilePath = QString("storage/files/%1").arg(filePath);
    if (!QFileInfo::exists(encFilePath)) { QMessageBox::warning(this,"Ошибка","Зашифрованный файл не найден"); return; }

    QString tmpPath = QDir::temp().filePath(QString("jumandgi_sub_%1_%2").arg(subId).arg(originalName));
    tmpPath = tmpPath.replace("/", "_");
    QFile::remove(tmpPath);

    std::string serr;
    if (!crypto::aes256_cbc_decrypt(fileKey, fileIv, encFilePath.toStdString(), tmpPath.toStdString(), serr)) {
        QMessageBox::critical(this, "Ошибка расшифровки файла", QString::fromStdString(serr));
        return;
    }

        // гарантируем абсолютный путь
    QFileInfo tf(tmpPath);
    QString absTmp = tf.absoluteFilePath();
    if (absTmp.isEmpty()) absTmp = tmpPath;

    // права на чтение
    QFile::Permissions perms = QFile::permissions(absTmp);
    perms |= QFileDevice::ReadOwner | QFileDevice::WriteOwner
          | QFileDevice::ReadGroup | QFileDevice::ReadOther;
    QFile::setPermissions(absTmp, perms);

    // 1) пробуем xdg-open
    if (QProcess::startDetached(QStringLiteral("xdg-open"), QStringList() << absTmp)) {
        Logger::log(m_teacherId, "download_submission",
                    QString("submission_id=%1 path=%2 (xdg-open)").arg(subId).arg(absTmp));
        return;
    }

    // 2) пробуем gedit
    if (QProcess::startDetached(QStringLiteral("gedit"), QStringList() << absTmp)) {
        Logger::log(m_teacherId, "download_submission",
                    QString("submission_id=%1 path=%2 (gedit)").arg(subId).arg(absTmp));
        return;
    }

    // 3) пробуем QDesktopServices
    if (QDesktopServices::openUrl(QUrl::fromLocalFile(absTmp))) {
        Logger::log(m_teacherId, "download_submission",
                    QString("submission_id=%1 path=%2 (QDesktopServices)").arg(subId).arg(absTmp));
        return;
    }

    // 4) совсем не повезло
    QMessageBox::warning(this, "Ошибка",
                         "Не удалось автоматически открыть файл. Откройте вручную: " + absTmp);
    qWarning() << "Failed to open file:" << absTmp;
    Logger::log(m_teacherId, "download_submission_fail",
                QString("submission_id=%1 path=%2").arg(subId).arg(absTmp));

    Logger::log(m_teacherId, "download_submission", QString("submission_id=%1 path=%2").arg(subId).arg(tmpPath));
}

void TeacherWindow::onGradeSubmission() {
    auto sel = tblSubmissions->selectedItems();
    if (sel.isEmpty()) { QMessageBox::warning(this,"Ошибка","Выберите отправление"); return; }
    int row = tblSubmissions->currentRow();
    int subId = tblSubmissions->item(row,0)->text().toInt();

    bool ok;
    QString grade = QInputDialog::getText(this, "Оценка", "Введите оценку:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QString feedback = QInputDialog::getMultiLineText(this, "Комментарий", "Комментарий к работе:", QString(), &ok);
    if (!ok) return;

    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("UPDATE submissions SET grade = ?, feedback = ? WHERE id = ?");
    q.addBindValue(grade);
    q.addBindValue(feedback);
    q.addBindValue(subId);
    if (!q.exec()) {
        QMessageBox::warning(this,"Ошибка","Не удалось сохранить оценку: " + q.lastError().text());
        return;
    }
    Logger::log(m_teacherId, "grade_submission", QString("submission_id=%1 grade=%2").arg(subId).arg(grade));
    loadSubmissions(currentAssignmentId);
}

void TeacherWindow::onCreateAssignment() {
    bool ok;
    QString title = QInputDialog::getText(this, "Создать задание", "Заголовок:", QLineEdit::Normal, "", &ok);
    if (!ok || title.trimmed().isEmpty()) return;

    QString desc = QInputDialog::getMultiLineText(this, "Описание", "Описание задания:", QString(), &ok);
    if (!ok) return;

    int days = QInputDialog::getInt(this, "Дедлайн", "Через сколько дней дедлайн? (дней)", 7, 0, 3650, 1, &ok);
    if (!ok) return;
    QDateTime due = QDateTime::currentDateTime().addDays(days);

    // choose group (optional)
    int chosenGroupId = -1;
    QSqlQuery gq(Database::instance().get());
    if (gq.exec("SELECT id, name FROM groups ORDER BY name")) {
        QStringList groupNames;
        QList<int> groupIds;
        while (gq.next()) {
            groupIds << gq.value(0).toInt();
            groupNames << gq.value(1).toString();
        }
        if (!groupNames.isEmpty()) {
            bool ok2;
            QString chosen = QInputDialog::getItem(this, "Группа", "Выберите группу (или пусто для индивидуальных студентов):", groupNames, 0, true, &ok2);
            if (ok2 && !chosen.isEmpty()) {
                int idx = groupNames.indexOf(chosen);
                if (idx >= 0) chosenGroupId = groupIds[idx];
            }
        }
    }

    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("INSERT INTO assignments (title, description, discipline_id, created_by, due_date, created_at) VALUES (?, ?, NULL, ?, ?, NOW()) RETURNING id");
    q.addBindValue(title);
    q.addBindValue(desc);
    q.addBindValue(m_teacherId);
    q.addBindValue(due);
    if (!q.exec() || !q.next()) {
        QMessageBox::warning(this,"Ошибка","Не удалось создать задание: " + q.lastError().text());
        return;
    }
    int assignmentId = q.value(0).toInt();

    if (chosenGroupId != -1) {
        QSqlQuery agq(db);
        agq.prepare("INSERT INTO assignment_groups (assignment_id, group_id) VALUES (?, ?) ON CONFLICT DO NOTHING");
        agq.addBindValue(assignmentId);
        agq.addBindValue(chosenGroupId);
        agq.exec();
    }

    // choose specific students (optional)
    QSqlQuery sq(db);
    sq.prepare("SELECT id, login, full_name FROM users WHERE role = 'student' ORDER BY id");
    if (!sq.exec()) {
        QMessageBox::warning(this,"Ошибка","Не удалось получить список студентов: " + sq.lastError().text());
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Выберите студентов (оставьте пустым для всех/группы)");
    QVBoxLayout *vl = new QVBoxLayout(&dlg);
    QListWidget *lw = new QListWidget(&dlg);
    lw->setSelectionMode(QAbstractItemView::NoSelection);
    vl->addWidget(lw);

    while (sq.next()) {
        int sid = sq.value(0).toInt();
        QString label = sq.value(1).toString();
        QString fn = sq.value(2).toString();
        if (!fn.isEmpty()) label += " (" + fn + ")";
        QListWidgetItem *it = new QListWidgetItem(label, lw);
        it->setData(Qt::UserRole, sid);
        it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
        it->setCheckState(Qt::Unchecked);
    }

    QHBoxLayout *h = new QHBoxLayout();
    QPushButton *btnOk = new QPushButton("OK", &dlg);
    QPushButton *btnCancel = new QPushButton("Отмена", &dlg);
    h->addStretch();
    h->addWidget(btnOk);
    h->addWidget(btnCancel);
    vl->addLayout(h);

    connect(btnOk, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    QList<int> chosen;
    if (dlg.exec() == QDialog::Accepted) {
        for (int i=0;i<lw->count();++i) {
            QListWidgetItem *it = lw->item(i);
            if (it->checkState() == Qt::Checked) chosen.append(it->data(Qt::UserRole).toInt());
        }
    }

    if (!chosen.isEmpty()) {
        QSqlQuery insertq(db);
        insertq.prepare("INSERT INTO assignment_students (assignment_id, student_id) VALUES (?, ?) ON CONFLICT DO NOTHING");
        for (int sid : chosen) {
            insertq.bindValue(0, assignmentId);
            insertq.bindValue(1, sid);
            insertq.exec();
        }
    }

    // attach file
    QString attached = QFileDialog::getOpenFileName(this, "Прикрепить файл к заданию (необязательно)");
    if (!attached.isEmpty()) {
        QDir().mkpath("storage/assignments");
        QString uidSrc = title + QDateTime::currentDateTime().toString() + QString::number(assignmentId);
        QString uuid = QString::fromUtf8(QCryptographicHash::hash(uidSrc.toUtf8(), QCryptographicHash::Md5).toHex());
        QString target = QString("storage/assignments/%1_%2").arg(uuid).arg(QFileInfo(attached).fileName());
        if (QFile::copy(attached, target)) {
            QSqlQuery fq(db);
            fq.prepare("INSERT INTO assignment_files (assignment_id, file_path, original_name, uploaded_by, uploaded_at) VALUES (?, ?, ?, ?, NOW())");
            fq.addBindValue(assignmentId);
            fq.addBindValue(QFileInfo(target).fileName());
            fq.addBindValue(QFileInfo(attached).fileName());
            fq.addBindValue(m_teacherId);
            fq.exec();
        } else {
            QMessageBox::warning(this,"Ошибка","Не удалось сохранить прикреплённый файл");
        }
    }

    Logger::log(m_teacherId, "create_assignment", QString("title=%1 assignment_id=%2 group_id=%3 students=%4").arg(title).arg(assignmentId).arg(chosenGroupId).arg(QString::number(chosen.size())));
    loadAssignments();
    QMessageBox::information(this,"OK","Задание создано");
}

void TeacherWindow::onDeleteAssignment() {
    auto sel = tblAssignments->selectedItems();
    if (sel.isEmpty()) { QMessageBox::warning(this, "Ошибка", "Выберите задание"); return; }
    int row = tblAssignments->currentRow();
    int assignmentId = tblAssignments->item(row,0)->text().toInt();

    if (QMessageBox::question(this, "Удалить задание",
            QString("Вы действительно хотите удалить задание %1 ?").arg(assignmentId),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT sp_delete_assignment(?, ?)");
    q.addBindValue(assignmentId);
    q.addBindValue(m_teacherId);
    if (!q.exec()) {
        QString err = q.lastError().text();
        if (err.contains("forbidden")) {
            QMessageBox::warning(this,"Ошибка","Вы можете удалять только свои задания");
        } else {
            QMessageBox::warning(this,"Ошибка","Не удалось удалить задание: " + err);
        }
        return;
    }

    Logger::log(m_teacherId, "delete_assignment", QString("assignment_id=%1").arg(assignmentId));
    loadAssignments();
    clearSubmissions();
    QMessageBox::information(this, "OK", "Задание удалено");
}
