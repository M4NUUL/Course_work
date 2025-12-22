#include "TeacherWindow.hpp"
#include "../db/Database.hpp"
#include "../config/ConfigManager.hpp"
#include "../crypto/KeyProtect.hpp"
#include "../crypto/FileCrypto.hpp"
#include "../utils/Logger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlQuery>
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
#include <QDateTime>
#include <QSqlError>


TeacherWindow::TeacherWindow(int teacherId, QWidget *parent)
    : QWidget(parent), m_teacherId(teacherId)
{
    setWindowTitle("Jumandgi — Интерфейс преподавателя");
    resize(1000,600);
    auto v = new QVBoxLayout(this);
    auto htop = new QHBoxLayout();

    tblAssignments = new QTableWidget();
    tblAssignments->setColumnCount(2);
    tblAssignments->setHorizontalHeaderLabels({"ID","Задание"});
    tblAssignments->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblAssignments->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(tblAssignments, &QTableWidget::cellClicked, this, &TeacherWindow::onAssignmentSelected);

    tblSubmissions = new QTableWidget();
    tblSubmissions->setColumnCount(5);
    tblSubmissions->setHorizontalHeaderLabels({"ID","Студент","Файл","Загружено","Оценка"});
    tblSubmissions->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblSubmissions->setEditTriggers(QAbstractItemView::NoEditTriggers);

    htop->addWidget(tblAssignments, 1);
    htop->addWidget(tblSubmissions, 2);

    btnRefresh = new QPushButton("Обновить");
    btnDownload = new QPushButton("Скачать/Открыть");
    btnGrade = new QPushButton("Оценить / Комментарий");
    btnCreateAssignment = new QPushButton("Создать задание"); // новая кнопка

    connect(btnRefresh, &QPushButton::clicked, this, &TeacherWindow::loadAssignments);
    connect(btnDownload, &QPushButton::clicked, this, &TeacherWindow::onDownloadSubmission);
    connect(btnGrade, &QPushButton::clicked, this, &TeacherWindow::onGradeSubmission);
    connect(btnCreateAssignment, &QPushButton::clicked, this, &TeacherWindow::onCreateAssignment);

    auto hbot = new QHBoxLayout();
    hbot->addWidget(btnCreateAssignment);
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
    if (!q.exec()) { QMessageBox::warning(this, "Ошибка", "Не удалось загрузить задания"); return; }
    int row = 0;
    while (q.next()) {
        tblAssignments->insertRow(row);
        tblAssignments->setItem(row,0, new QTableWidgetItem(QString::number(q.value(0).toInt())));
        tblAssignments->setItem(row,1, new QTableWidgetItem(q.value(1).toString()));
        row++;
    }
    tblAssignments->resizeColumnsToContents();
}

void TeacherWindow::onAssignmentSelected(int row, int /*col*/) {
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
    q.prepare("SELECT s.id, u.login, s.original_name, s.uploaded_at, s.grade FROM submissions s LEFT JOIN users u ON s.student_id = u.id WHERE s.assignment_id = ?");
    q.addBindValue(assignmentId);
    if (!q.exec()) { QMessageBox::warning(this, "Ошибка", "Не удалось загрузить отправления"); return; }
    int row = 0;
    while (q.next()) {
        tblSubmissions->insertRow(row);
        tblSubmissions->setItem(row,0, new QTableWidgetItem(QString::number(q.value(0).toInt())));
        tblSubmissions->setItem(row,1, new QTableWidgetItem(q.value(1).toString()));
        tblSubmissions->setItem(row,2, new QTableWidgetItem(q.value(2).toString()));
        tblSubmissions->setItem(row,3, new QTableWidgetItem(q.value(3).toString()));
        tblSubmissions->setItem(row,4, new QTableWidgetItem(q.value(4).toString()));
        row++;
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
    if (!q.exec() || !q.next()) { QMessageBox::warning(this,"Ошибка","Запись не найдена"); return; }
    QString filePath = q.value(0).toString();
    QString originalName = q.value(1).toString();

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

    std::string serr;
    QString encFilePath = QString("storage/files/%1").arg(filePath);
    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + "/jumandgiXXXXXX_" + originalName);
    if (!tmp.open()) { QMessageBox::warning(this,"Ошибка","Не удалось создать временный файл"); return; }
    QString tmpPath = tmp.fileName();
    tmp.close();

    if (!crypto::aes256_cbc_decrypt(fileKey, fileIv, encFilePath.toStdString(), tmpPath.toStdString(), serr)) {
        QMessageBox::critical(this, "Ошибка расшифровки файла", QString::fromStdString(serr));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(tmpPath));
    Logger::log(m_teacherId, "download_submission", QString("submission_id=%1").arg(subId));
}

void TeacherWindow::onGradeSubmission() {
    auto sel = tblSubmissions->selectedItems();
    if (sel.isEmpty()) { QMessageBox::warning(this,"Ошибка","Выберите отправление"); return; }
    int row = tblSubmissions->currentRow();
    int subId = tblSubmissions->item(row,0)->text().toInt();

    bool ok;
    QString grade = QInputDialog::getText(this, "Оценка", "Введите оценку:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QString feedback = QInputDialog::getMultiLineText(this, "Комментарий", "Комментарий к работе:");

    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("UPDATE submissions SET grade = ?, feedback = ? WHERE id = ?");
    q.addBindValue(grade);
    q.addBindValue(feedback);
    q.addBindValue(subId);
    if (!q.exec()) {
        QMessageBox::warning(this,"Ошибка","Не удалось сохранить оценку");
        return;
    }
    Logger::log(m_teacherId, "grade_submission", QString("submission_id=%1 grade=%2").arg(subId).arg(grade));
    loadSubmissions(currentAssignmentId);
}

void TeacherWindow::onCreateAssignment() {
    bool ok;
    QString title = QInputDialog::getText(this, "Создать задание", "Заголовок:", QLineEdit::Normal, "", &ok);
    if (!ok || title.trimmed().isEmpty()) return;

    QString desc = QInputDialog::getMultiLineText(this, "Описание", "Описание задания:");

    int days = QInputDialog::getInt(this, "Дедлайн", "Через сколько дней дедлайн? (дней)", 7, 0, 3650, 1, &ok);
    if (!ok) return;
    QDateTime due = QDateTime::currentDateTime().addDays(days);

    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("INSERT INTO assignments (title, description, discipline_id, created_by, due_date, created_at) VALUES (?, ?, NULL, ?, ?, NOW())");
    q.addBindValue(title);
    q.addBindValue(desc);
    q.addBindValue(m_teacherId);
    q.addBindValue(due);
    if (!q.exec()) {
        QMessageBox::warning(this,"Ошибка","Не удалось создать задание: " + q.lastError().text());
        return;
    }
    Logger::log(m_teacherId, "create_assignment", QString("title=%1").arg(title));
    loadAssignments();
    QMessageBox::information(this,"OK","Задание создано");
}
