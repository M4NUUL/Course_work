#include "AdminWindow.hpp"
#include "CreateUserDialog.hpp"
#include "../db/Database.hpp"
#include "../auth/AuthManager.hpp"
#include "../utils/Logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>

AdminWindow::AdminWindow(int adminId, QWidget *parent) : QWidget(parent), m_adminId(adminId) {
    setWindowTitle("Jumandgi — Администратор");
    resize(900,600);

    auto v = new QVBoxLayout(this);

    // Users table
    tblUsers = new QTableWidget();
    tblUsers->setColumnCount(6);
    tblUsers->setHorizontalHeaderLabels({"ID","Login","Full name","Email","Role","Active"});
    tblUsers->horizontalHeader()->setStretchLastSection(true);
    tblUsers->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblUsers->setEditTriggers(QAbstractItemView::NoEditTriggers);

    v->addWidget(tblUsers);

    // Buttons
    auto h = new QHBoxLayout();
    btnCreateUser = new QPushButton("Создать пользователя");
    btnEditUser = new QPushButton("Редактировать");
    btnToggleActive = new QPushButton("Включить/Отключить");
    btnCreateDiscipline = new QPushButton("Создать дисциплину");
    btnCreateAssignment = new QPushButton("Создать задание");
    btnRefresh = new QPushButton("Обновить");

    h->addWidget(btnCreateUser);
    h->addWidget(btnEditUser);
    h->addWidget(btnToggleActive);
    h->addWidget(btnCreateDiscipline);
    h->addWidget(btnCreateAssignment);
    h->addStretch();
    h->addWidget(btnRefresh);

    v->addLayout(h);

    connect(btnCreateUser, &QPushButton::clicked, this, &AdminWindow::onCreateUser);
    connect(btnEditUser, &QPushButton::clicked, this, &AdminWindow::onEditUser);
    connect(btnToggleActive, &QPushButton::clicked, this, &AdminWindow::onToggleActive);
    connect(btnCreateDiscipline, &QPushButton::clicked, this, &AdminWindow::onCreateDiscipline);
    connect(btnCreateAssignment, &QPushButton::clicked, this, &AdminWindow::onCreateAssignment);
    connect(btnRefresh, &QPushButton::clicked, this, &AdminWindow::onRefresh);

    loadUsers();
}

void AdminWindow::showError(const QString &text) {
    QMessageBox::critical(this, "Ошибка", text);
}

void AdminWindow::loadUsers() {
    tblUsers->setRowCount(0);
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("SELECT id, login, full_name, email, role, active FROM users ORDER BY id");
    if (!q.exec()) {
        showError("Не удалось загрузить пользователей: " + q.lastError().text());
        return;
    }
    int row = 0;
    while (q.next()) {
        tblUsers->insertRow(row);
        tblUsers->setItem(row,0,new QTableWidgetItem(QString::number(q.value(0).toInt())));
        tblUsers->setItem(row,1,new QTableWidgetItem(q.value(1).toString()));
        tblUsers->setItem(row,2,new QTableWidgetItem(q.value(2).toString()));
        tblUsers->setItem(row,3,new QTableWidgetItem(q.value(3).toString()));
        tblUsers->setItem(row,4,new QTableWidgetItem(q.value(4).toString()));
        tblUsers->setItem(row,5,new QTableWidgetItem(q.value(5).toBool() ? "true" : "false"));
        row++;
    }
    tblUsers->resizeColumnsToContents();
}

void AdminWindow::onCreateUser() {
    CreateUserDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString login = dlg.login().trimmed();
        QString password = dlg.password();
        QString role = dlg.role();
        QString fullName = dlg.fullName();
        QString email = dlg.email();

        // register via AuthManager
        bool ok = AuthManager::instance().registerUser(login.toStdString(), role.toStdString(), password.toStdString());
        if (!ok) {
            showError("Ошибка регистрации пользователя. Возможно логин занят или пароль не соответствует требованиям.");
            return;
        }
        // update full_name & email using DB
        QSqlDatabase db = Database::instance().get();
        QSqlQuery q(db);
        q.prepare("UPDATE users SET full_name = ?, email = ? WHERE login = ?");
        q.addBindValue(fullName);
        q.addBindValue(email);
        q.addBindValue(login);
        if (!q.exec()) {
            showError("Не удалось обновить данные пользователя: " + q.lastError().text());
            return;
        }
        Logger::log(m_adminId, "create_user", QString("login=%1 role=%2").arg(login).arg(role));
        loadUsers();
        QMessageBox::information(this, "OK", "Пользователь создан");
    }
}

void AdminWindow::onToggleActive() {
    auto sel = tblUsers->selectedItems();
    if (sel.isEmpty()) { showError("Выберите пользователя"); return; }
    int row = tblUsers->currentRow();
    int userId = tblUsers->item(row,0)->text().toInt();
    QString active = tblUsers->item(row,5)->text();
    bool activeBool = (active == "true");
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("UPDATE users SET active = ? WHERE id = ?");
    q.addBindValue(!activeBool);
    q.addBindValue(userId);
    if (!q.exec()) { showError("Не удалось изменить активность: "+q.lastError().text()); return; }
    Logger::log(m_adminId, "toggle_user", QString("user_id=%1 active=%2").arg(userId).arg(!activeBool));
    loadUsers();
}

void AdminWindow::onEditUser() {
    auto sel = tblUsers->selectedItems();
    if (sel.isEmpty()) { showError("Выберите пользователя"); return; }
    int row = tblUsers->currentRow();
    int userId = tblUsers->item(row,0)->text().toInt();

    // Simple edit dialog: edit full_name and email and role
    bool ok;
    QString fullName = QInputDialog::getText(this, "Full name", "Введите полное имя:", QLineEdit::Normal, tblUsers->item(row,2)->text(), &ok);
    if (!ok) return;
    QString email = QInputDialog::getText(this, "Email", "Введите email:", QLineEdit::Normal, tblUsers->item(row,3)->text(), &ok);
    if (!ok) return;
    QString role = QInputDialog::getItem(this, "Role", "Выберите роль:", QStringList({"student","teacher","admin"}), 0, false, &ok);
    if (!ok) return;

    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("UPDATE users SET full_name = ?, email = ?, role = ? WHERE id = ?");
    q.addBindValue(fullName);
    q.addBindValue(email);
    q.addBindValue(role);
    q.addBindValue(userId);
    if (!q.exec()) { showError("Не удалось сохранить изменения: " + q.lastError().text()); return; }
    Logger::log(m_adminId, "edit_user", QString("user_id=%1").arg(userId));
    loadUsers();
}

void AdminWindow::onCreateDiscipline() {
    bool ok;
    QString name = QInputDialog::getText(this, "Создать дисциплину", "Название дисциплины:", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    QString code = QInputDialog::getText(this, "Код дисциплины", "Код (уникальный):", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("INSERT INTO disciplines (name, code) VALUES (?, ?)");
    q.addBindValue(name);
    q.addBindValue(code);
    if (!q.exec()) { showError("Не удалось создать дисциплину: " + q.lastError().text()); return; }
    Logger::log(m_adminId, "create_discipline", QString("name=%1 code=%2").arg(name).arg(code));
    QMessageBox::information(this, "OK", "Дисциплина создана");
}

void AdminWindow::onCreateAssignment() {
    // gather disciplines and teachers
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);

    // disciplines
    QList<QPair<int,QString>> disciplines;
    q.prepare("SELECT id, name FROM disciplines ORDER BY id");
    if (q.exec()) {
        while (q.next()) disciplines.append({ q.value(0).toInt(), q.value(1).toString() });
    }

    // teachers
    QList<QPair<int,QString>> teachers;
    q.prepare("SELECT id, login FROM users WHERE role = 'teacher' ORDER BY id");
    if (q.exec()) {
        while (q.next()) teachers.append({ q.value(0).toInt(), q.value(1).toString() });
    }

    // Dialog for title/desc
    bool ok;
    QString title = QInputDialog::getText(this, "Создать задание", "Заголовок:", QLineEdit::Normal, "", &ok);
    if (!ok || title.trimmed().isEmpty()) return;
    QString desc = QInputDialog::getMultiLineText(this, "Описание", "Описание задания:");

    // choose discipline id (or none)
    QStringList discNames;
    discNames << "<none>";
    for (auto &d : disciplines) discNames << d.second;
    int discIdx = QInputDialog::getItem(this, "Дисциплина", "Выберите дисциплину:", discNames, 0, false, &ok).toInt();
    int discId = 0;
    if (!ok) return;
    if (discIdx > 0) discId = disciplines[discIdx-1].first;

    // choose teacher
    QStringList teacherNames;
    for (auto &t : teachers) teacherNames << t.second;
    if (teacherNames.isEmpty()) {
        showError("Нет зарегистрированных преподавателей. Создайте преподавателя сначала.");
        return;
    }
    QString chosenTeacher = QInputDialog::getItem(this, "Преподаватель", "Выберите преподавателя:", teacherNames, 0, false, &ok);
    if (!ok) return;
    int teacherId = 0;
    for (auto &t : teachers) if (t.second == chosenTeacher) { teacherId = t.first; break; }

    // due date
    QDateTime due = QDateTime::currentDateTime().addDays(7);

    q.prepare("INSERT INTO assignments (title, description, discipline_id, created_by, due_date, created_at) VALUES (?, ?, ?, ?, ?, NOW())");
    q.addBindValue(title);
    q.addBindValue(desc);
    if (discId==0) q.addBindValue(QVariant(QVariant::Int)); else q.addBindValue(discId);
    q.addBindValue(teacherId);
    q.addBindValue(due);
    if (!q.exec()) {
        showError("Не удалось создать задание: " + q.lastError().text());
        return;
    }
    Logger::log(m_adminId, "create_assignment", QString("title=%1 teacher=%2").arg(title).arg(chosenTeacher));
    QMessageBox::information(this, "OK", "Задание создано");
}

void AdminWindow::onRefresh() {
    loadUsers();
}
