#include "AssignmentDetailDialog.hpp"

#include "../db/Database.hpp"

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
#include <QMessageBox>
#include <QDateTime>
#include <QFileDevice>

AssignmentDetailDialog::AssignmentDetailDialog(int assignmentId, QWidget *parent)
    : QDialog(parent), m_assignmentId(assignmentId)
{
    setWindowTitle(QStringLiteral("Детали задания"));
    resize(700, 500);

    auto *v = new QVBoxLayout(this);

    lblTitle = new QLabel(this);
    lblTitle->setWordWrap(true);

    teDescription = new QTextEdit(this);
    teDescription->setReadOnly(true);

    v->addWidget(lblTitle);
    v->addWidget(new QLabel(QStringLiteral("Описание:"), this));
    v->addWidget(teDescription, 1);

    tblFiles = new QTableWidget(this);
    tblFiles->setColumnCount(1);
    tblFiles->setHorizontalHeaderLabels({QStringLiteral("Файл")});
    tblFiles->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblFiles->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblFiles->horizontalHeader()->setStretchLastSection(true);

    v->addWidget(new QLabel(QStringLiteral("Прикреплённые файлы:"), this));
    v->addWidget(tblFiles, 1);

    auto *h = new QHBoxLayout();
    btnClose = new QPushButton(QStringLiteral("Закрыть"), this);
    h->addStretch();
    h->addWidget(btnClose);
    v->addLayout(h);

    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    connect(tblFiles, &QTableWidget::cellDoubleClicked,
            this, &AssignmentDetailDialog::onFileDoubleClicked);

    loadDetails();
    loadFiles();
}

void AssignmentDetailDialog::loadDetails() {
    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT title, description, due_date FROM assignments WHERE id = ?");
    q.addBindValue(m_assignmentId);

    if (!q.exec() || !q.next()) {
        lblTitle->setText(QStringLiteral("Задание не найдено"));
        teDescription->clear();
        return;
    }

    const QString title = q.value(0).toString();
    const QString desc  = q.value(1).toString();
    const QVariant dueVar = q.value(2);

    QString dueStr;
    if (!dueVar.isNull()) {
        if (dueVar.canConvert<QDateTime>()) {
            dueStr = dueVar.toDateTime().toLocalTime().toString("dd.MM.yyyy HH:mm");
        } else {
            dueStr = dueVar.toString();
        }
    }

    lblTitle->setText(
        dueStr.isEmpty()
            ? title
            : QStringLiteral("%1 (Дедлайн: %2)").arg(title, dueStr)
    );

    teDescription->setPlainText(desc);
}

void AssignmentDetailDialog::loadFiles() {
    tblFiles->setRowCount(0);

    QSqlQuery q(Database::instance().get());
    q.prepare(
        "SELECT id, original_name, file_path "
        "FROM assignment_files "
        "WHERE assignment_id = ?"
    );
    q.addBindValue(m_assignmentId);

    if (!q.exec()) {
        qWarning() << "Failed to load assignment files:" << q.lastError().text();
        return;
    }

    int row = 0;
    while (q.next()) {
        const int fileId = q.value(0).toInt();
        const QString originalName = q.value(1).toString();
        const QString filePath = q.value(2).toString();

        tblFiles->insertRow(row);

        auto *item = new QTableWidgetItem(originalName);
        item->setData(Qt::UserRole, fileId);
        item->setData(Qt::UserRole + 1, filePath);
        tblFiles->setItem(row, 0, item);

        ++row;
    }

    tblFiles->resizeColumnsToContents();
}

void AssignmentDetailDialog::onFileDoubleClicked(int row, int /*column*/) {
    if (row < 0) return;

    auto *item = tblFiles->item(row, 0);
    if (!item) return;

    const QString filePath = item->data(Qt::UserRole + 1).toString();
    const QString originalName = item->text();
    if (filePath.isEmpty()) return;

    const QString fullPath = QStringLiteral("storage/assignments/%1").arg(filePath);
    if (!QFileInfo::exists(fullPath)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Ошибка"),
            QStringLiteral("Файл не найден: ") + fullPath
        );
        return;
    }

    QString tmpPath = QDir::temp().filePath(originalName);
    tmpPath.replace("/", "_");
    QFile::remove(tmpPath);

    if (!QFile::copy(fullPath, tmpPath)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Ошибка"),
            QStringLiteral("Не удалось подготовить файл для открытия")
        );
        return;
    }

    // Явно выставляем права, т.к. некоторые окружения игнорируют права временных файлов
    QFile::Permissions perms = QFile::permissions(tmpPath);
    perms |= QFileDevice::ReadOwner | QFileDevice::WriteOwner
          | QFileDevice::ReadGroup | QFileDevice::ReadOther;
    QFile::setPermissions(tmpPath, perms);

    if (QDesktopServices::openUrl(QUrl::fromLocalFile(tmpPath))) return;
    if (QProcess::startDetached(QStringLiteral("xdg-open"), {tmpPath})) return;

    QMessageBox::warning(
        this,
        QStringLiteral("Ошибка"),
        QStringLiteral("Не удалось открыть файл: ") + tmpPath
    );
}
