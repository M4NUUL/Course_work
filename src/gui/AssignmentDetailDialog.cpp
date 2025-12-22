#include "AssignmentDetailDialog.hpp"
#include "../db/Database.hpp"

#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QSqlQuery>
#include <QSqlError>

#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QFile>


AssignmentDetailDialog::AssignmentDetailDialog(int assignmentId, QWidget *parent)
    : QDialog(parent), m_assignmentId(assignmentId)
{
    setWindowTitle("Просмотр задания");
    resize(700, 500);

    auto v = new QVBoxLayout(this);
    lblTitle = new QLabel(this);
    lblDue = new QLabel(this);
    txtDescription = new QTextEdit(this);
    txtDescription->setReadOnly(true);

    tblMaterials = new QTableWidget(this);
    tblMaterials->setColumnCount(2);
    tblMaterials->setHorizontalHeaderLabels({"Файл", "Загружен"});
    tblMaterials->horizontalHeader()->setStretchLastSection(true);
    tblMaterials->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblMaterials->setEditTriggers(QAbstractItemView::NoEditTriggers);

    btnDownload = new QPushButton("Скачать/Открыть", this);
    connect(btnDownload, &QPushButton::clicked, this, &AssignmentDetailDialog::onDownloadMaterial);

    v->addWidget(lblTitle);
    v->addWidget(lblDue);
    v->addWidget(txtDescription, 1);
    v->addWidget(new QLabel("Материалы:", this));
    v->addWidget(tblMaterials, 0);
    auto h = new QHBoxLayout();
    h->addStretch();
    h->addWidget(btnDownload);
    v->addLayout(h);

    loadAssignment();
    loadMaterials();
}

void AssignmentDetailDialog::loadAssignment() {
    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT title, description, due_date FROM assignments WHERE id = ?");
    q.addBindValue(m_assignmentId);
    if (!q.exec() || !q.next()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить задание: " + q.lastError().text());
        return;
    }
    lblTitle->setText(QString("<b>%1</b>").arg(q.value(0).toString()));
    lblDue->setText(QString("Дедлайн: %1").arg(q.value(2).toString()));
    txtDescription->setPlainText(q.value(1).toString());
}

void AssignmentDetailDialog::loadMaterials() {
    tblMaterials->setRowCount(0);
    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT id, original_name, uploaded_at, file_path FROM assignment_files WHERE assignment_id = ? ORDER BY uploaded_at");
    q.addBindValue(m_assignmentId);
    if (!q.exec()) {
        return;
    }
    int r = 0;
    while (q.next()) {
        tblMaterials->insertRow(r);
        tblMaterials->setItem(r, 0, new QTableWidgetItem(q.value(1).toString()));
        tblMaterials->setItem(r, 1, new QTableWidgetItem(q.value(2).toString()));
        tblMaterials->item(r,0)->setData(Qt::UserRole, q.value(3).toString());
        tblMaterials->item(r,0)->setData(Qt::UserRole + 1, q.value(0).toInt());
        r++;
    }
    tblMaterials->resizeColumnsToContents();
}

void AssignmentDetailDialog::onDownloadMaterial() {
    auto sel = tblMaterials->selectedItems();
    if (sel.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите материал");
        return;
    }
    int row = tblMaterials->currentRow();
    QTableWidgetItem *nameItem = tblMaterials->item(row,0);
    QString filePath = nameItem->data(Qt::UserRole).toString();
    QString originalName = nameItem->text();

    QString fullPath = QString("storage/materials/%1").arg(filePath);
    if (!QFileInfo::exists(fullPath)) {
        fullPath = QString("storage/files/%1").arg(filePath);
    }
    if (!QFileInfo::exists(fullPath)) {
        QMessageBox::warning(this, "Ошибка", "Файл не найден: " + fullPath);
        return;
    }

    // Если файлы у вас зашифрованы, сюда нужно добавить логику их расшифровки (как в TeacherWindow).
    QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath));
}
