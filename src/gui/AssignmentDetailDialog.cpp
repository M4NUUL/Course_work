#include "AssignmentDetailDialog.hpp"

#include "../db/Database.hpp"
#include "../config/ConfigManager.hpp"
#include "../crypto/KeyProtect.hpp"
#include "../crypto/FileCrypto.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QSqlQuery>
#include <QSqlError>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDebug>

AssignmentDetailDialog::AssignmentDetailDialog(int assignmentId, QWidget *parent)
    : QDialog(parent), m_assignmentId(assignmentId)
{
    setWindowTitle(QString("Детали задания #%1").arg(assignmentId));
    resize(700,500);

    auto v = new QVBoxLayout(this);

    // Title and description
    lblTitle = new QLabel(this);
    lblTitle->setWordWrap(true);
    teDescription = new QTextEdit(this);
    teDescription->setReadOnly(true);

    v->addWidget(lblTitle);
    v->addWidget(new QLabel("Описание:"));
    v->addWidget(teDescription, 1);

    // Files table
    tblFiles = new QTableWidget(this);
    tblFiles->setColumnCount(2);
    tblFiles->setHorizontalHeaderLabels({"ID","Файл"});
    tblFiles->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblFiles->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblFiles->horizontalHeader()->setStretchLastSection(true);
    
    QFont tf = tblFiles->font();
    tf.setPointSize(10);
    tblFiles->setFont(tf);
    QFontMetrics fm(tf);
    tblFiles->verticalHeader()->setDefaultSectionSize(fm.height() + 12);
    tblFiles->horizontalHeader()->setFixedHeight(fm.height() + 16);
    tblFiles->setSelectionMode(QAbstractItemView::SingleSelection);
    tblFiles->setAlternatingRowColors(true);

    v->addWidget(new QLabel("Прикреплённые файлы:"));
    v->addWidget(tblFiles, 1);

    // Buttons
    auto h = new QHBoxLayout();
    btnClose = new QPushButton("Закрыть", this);
    h->addStretch();
    h->addWidget(btnClose);
    v->addLayout(h);

    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    connect(tblFiles, &QTableWidget::cellDoubleClicked, this, &AssignmentDetailDialog::onFileDoubleClicked);

    loadDetails();
    loadFiles();
}

void AssignmentDetailDialog::loadDetails() {
    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT title, description, due_date FROM assignments WHERE id = ?");
    q.addBindValue(m_assignmentId);
    if (!q.exec() || !q.next()) {
        lblTitle->setText("Задание не найдено");
        teDescription->setPlainText("");
        return;
    }
    QString title = q.value(0).toString();
    QString desc = q.value(1).toString();
    QVariant dueVar = q.value(2);
    QString dueStr = dueVar.isNull() ? QString() : dueVar.toDateTime().toString();
    lblTitle->setText(QString("%1 (Дедлайн: %2)").arg(title).arg(dueStr));
    teDescription->setPlainText(desc);
}

void AssignmentDetailDialog::loadFiles() {
    tblFiles->setRowCount(0);
    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT id, original_name, file_path FROM assignment_files WHERE assignment_id = ?");
    q.addBindValue(m_assignmentId);
    if (!q.exec()) {
        qWarning() << "Failed load assignment files:" << q.lastError().text();
        return;
    }
    int r = 0;
    while (q.next()) {
        tblFiles->insertRow(r);
        tblFiles->setItem(r,0, new QTableWidgetItem(QString::number(q.value(0).toInt())));
        tblFiles->setItem(r,1, new QTableWidgetItem(q.value(1).toString()));
        // store file_path in UserRole for later retrieval
        tblFiles->item(r,1)->setData(Qt::UserRole, q.value(2).toString());
        r++;
    }
    tblFiles->resizeColumnsToContents();
}

void AssignmentDetailDialog::onFileDoubleClicked(int row, int /*column*/) {
    if (row < 0) return;
    QString fileName = tblFiles->item(row,1)->data(Qt::UserRole).toString();
    QString original = tblFiles->item(row,1)->text();
    if (fileName.isEmpty()) return;
    QString full = QString("storage/assignments/%1").arg(fileName);
    if (!QFileInfo::exists(full)) {
        QMessageBox::warning(this,"Ошибка","Файл не найден: " + full);
        return;
    }

    QString tmp = QDir::temp().filePath(original);
    tmp = tmp.replace("/", "_");
    QFile::remove(tmp);
    if (!QFile::copy(full, tmp)) {
        QMessageBox::warning(this,"Ошибка","Не удалось подготовить файл для открытия");
        return;
    }

    bool opened = QDesktopServices::openUrl(QUrl::fromLocalFile(tmp));
    if (!opened) {
        // fallback to xdg-open on linux
        if (!QProcess::startDetached("xdg-open", QStringList() << tmp)) {
            QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл: " + tmp);
        }
    }
}
