#include "StudentWindow.hpp"
#include "AssignmentDetailDialog.hpp"

#include "../db/Database.hpp"
#include "../utils/Logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
#include <QTableWidget>
#include <QPushButton>
#include <QDir>

StudentWindow::StudentWindow(int studentId, QWidget *parent)
    : QWidget(parent), m_studentId(studentId)
{
    setWindowTitle("Jumandgi — Студент");
    resize(800, 600);

    auto v = new QVBoxLayout(this);

    tblAssignments = new QTableWidget(this);
    tblAssignments->setColumnCount(3);
    tblAssignments->setHorizontalHeaderLabels({"ID","Задание","Дедлайн"});
    tblAssignments->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblAssignments->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblAssignments->horizontalHeader()->setStretchLastSection(true);

    v->addWidget(tblAssignments);

    auto h = new QHBoxLayout();
    btnUpload = new QPushButton("Загрузить работу", this);
    h->addWidget(btnUpload);
    h->addStretch();
    v->addLayout(h);

    connect(btnUpload, &QPushButton::clicked, this, &StudentWindow::onUpload);
    connect(tblAssignments, &QTableWidget::cellDoubleClicked, this, &StudentWindow::onAssignmentDoubleClicked);

    loadAssignments();
}

void StudentWindow::loadAssignments() {
    tblAssignments->setRowCount(0);
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);

    q.prepare(R"SQL(
        SELECT a.id, a.title, a.due_date
        FROM assignments a
        WHERE
          EXISTS (SELECT 1 FROM assignment_students s WHERE s.assignment_id = a.id AND s.student_id = ?)
          OR NOT EXISTS (SELECT 1 FROM assignment_students s WHERE s.assignment_id = a.id)
        ORDER BY a.due_date
    )SQL");
    q.addBindValue(m_studentId);
    if (!q.exec()) {
        qWarning() << "Failed load assignments for student:" << q.lastError().text();
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

static QString findCreateSubmissionExe() {
    // Попробуем рядом с исполняемым приложением
    QString appDir = QCoreApplication::applicationDirPath(); // обычно .../build
    QString exe = QDir(appDir).filePath("create_submission");
    if (QFileInfo::exists(exe) && QFileInfo(exe).isExecutable()) return exe;
    // Попробуем ../build/create_submission (если приложение запущено из project root)
    QString alt = QDir(appDir).filePath("../build/create_submission");
    if (QFileInfo::exists(alt) && QFileInfo(alt).isExecutable()) return alt;
    //Попробуем ./create_submission в текущей директории
    if (QFileInfo::exists("create_submission") && QFileInfo("create_submission").isExecutable()) return QString("create_submission");
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

    // Выполним синхронно
    int rc = QProcess::execute(exe, args);
    if (rc != 0) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить файл (create_submission вернул ошибку)");
        qWarning() << "create_submission failed, rc=" << rc << "exe=" << exe << "args=" << args;
        return;
    }

    QMessageBox::information(this, "OK", "Файл успешно отправлен");
    Logger::log(m_studentId, "upload_submission", QString("assignment=%1 file=%2").arg(assignmentId).arg(QFileInfo(file).fileName()));

    // обновим список отправлений (если такая панель есть) или интерфейс
    loadAssignments();
}

void StudentWindow::onAssignmentDoubleClicked(int row, int /*column*/) {
    if (row < 0) return;
    int assignmentId = tblAssignments->item(row,0)->text().toInt();
    AssignmentDetailDialog dlg(assignmentId, this);
    dlg.exec();
}
