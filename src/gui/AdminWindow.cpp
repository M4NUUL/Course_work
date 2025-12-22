#include "AdminWindow.hpp"
#include "../db/Database.hpp"
#include "../auth/AuthManager.hpp"
#include "../utils/Logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDebug>

AdminWindow::AdminWindow(int adminId, QWidget *parent)
    : QWidget(parent), m_adminId(adminId)
{
    setWindowTitle("Jumandgi — Администратор");
    resize(900,600);

    auto v = new QVBoxLayout(this);

    // Users table: ID, Login, Full name, Role, Group
    tblUsers = new QTableWidget(this);
    tblUsers->setColumnCount(5);
    tblUsers->setHorizontalHeaderLabels({"ID","Login","Full name","Role","Group"});
    tblUsers->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblUsers->horizontalHeader()->setStretchLastSection(true);
    v->addWidget(new QLabel("Пользователи:"));
    v->addWidget(tblUsers, 1);

    // Buttons row (no disciplines / no admin-create-assignment)
    auto h = new QHBoxLayout();
    btnCreateUser = new QPushButton("Создать пользователя", this);
    btnEditUser = new QPushButton("Редактировать", this);
    btnToggleActive = new QPushButton("Актив/Деактив", this);
    btnDeleteUser = new QPushButton("Удалить пользователя", this);
    btnRefresh = new QPushButton("Обновить", this);

    h->addWidget(btnCreateUser);
    h->addWidget(btnEditUser);
    h->addWidget(btnToggleActive);
    h->addWidget(btnDeleteUser);
    h->addStretch();
    h->addWidget(btnRefresh);

    v->addLayout(h);

    // Signals
    connect(btnCreateUser, &QPushButton::clicked, this, &AdminWindow::onCreateUser);
    connect(btnEditUser, &QPushButton::clicked, this, &AdminWindow::onEditUser);
    connect(btnToggleActive, &QPushButton::clicked, this, &AdminWindow::onToggleActive);
    connect(btnDeleteUser, &QPushButton::clicked, this, &AdminWindow::onDeleteUser);
    connect(btnRefresh, &QPushButton::clicked, this, &AdminWindow::onRefresh);

    // Initial load
    loadUsers();
}

void AdminWindow::showError(const QString &text) {
    QMessageBox::warning(this, "Ошибка", text);
}

void AdminWindow::loadUsers() {
    tblUsers->setRowCount(0);
    QSqlQuery q(Database::instance().get());
    // left join groups — ensure table groups exists via migration
    if (!q.exec("SELECT u.id, u.login, u.full_name, u.role, COALESCE(g.name, '') FROM users u LEFT JOIN groups g ON u.group_id = g.id ORDER BY u.id")) {
        showError("Не удалось загрузить пользователей: " + q.lastError().text());
        return;
    }
    int r = 0;
    while (q.next()) {
        tblUsers->insertRow(r);
        tblUsers->setItem(r,0, new QTableWidgetItem(QString::number(q.value(0).toInt())));
        tblUsers->setItem(r,1, new QTableWidgetItem(q.value(1).toString()));
        tblUsers->setItem(r,2, new QTableWidgetItem(q.value(2).toString()));
        tblUsers->setItem(r,3, new QTableWidgetItem(q.value(3).toString()));
        tblUsers->setItem(r,4, new QTableWidgetItem(q.value(4).toString()));
        r++;
    }
    tblUsers->resizeColumnsToContents();
}

void AdminWindow::onCreateUser() {
    bool ok;
    QString login = QInputDialog::getText(this, "Создать пользователя", "Login:", QLineEdit::Normal, "", &ok);
    if (!ok || login.trimmed().isEmpty()) return;

    QString role = QInputDialog::getText(this, "Создать пользователя", "Role (student/teacher/admin):", QLineEdit::Normal, "student", &ok);
    if (!ok || role.trimmed().isEmpty()) return;

    QString full = QInputDialog::getText(this, "Создать пользователя", "Full name:", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QString pwd = QInputDialog::getText(this, "Создать пользователя", "Password:", QLineEdit::Password, "", &ok);
    if (!ok || pwd.isEmpty()) return;

    // Choose group (optional)
    int selectedGroupId = -1;
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
            QString chosen = QInputDialog::getItem(this, "Группа", "Выберите группу (или пусто):", groupNames, 0, true, &ok2);
            if (ok2 && !chosen.isEmpty()) {
                int idx = groupNames.indexOf(chosen);
                if (idx >= 0) selectedGroupId = groupIds[idx];
            }
        }
    }

    // Register via AuthManager
    if (!AuthManager::instance().registerUser(login.toStdString(), role.toStdString(), pwd.toStdString())) {
        showError("Не удалось создать пользователя (возможно логин занят)");
        return;
    }

    // Get created user's id
    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT id FROM users WHERE login = ?");
    q.addBindValue(login);
    if (!q.exec() || !q.next()) {
        showError("Пользователь создан, но не удалось получить id. Проверьте базу.");
        loadUsers();
        return;
    }
    int newUserId = q.value(0).toInt();

    // Update group_id if chosen
    if (selectedGroupId != -1) {
        QSqlQuery uq(Database::instance().get());
        uq.prepare("UPDATE users SET group_id = ? WHERE id = ?");
        uq.addBindValue(selectedGroupId);
        uq.addBindValue(newUserId);
        if (!uq.exec()) {
            showError("Пользователь создан, но не удалось назначить группу: " + uq.lastError().text());
            Logger::log(m_adminId, "create_user_partial", QString("login=%1 id=%2 group_fail=%3").arg(login).arg(newUserId).arg(selectedGroupId));
            loadUsers();
            return;
        }
    }

    Logger::log(m_adminId, "create_user", QString("id=%1 login=%2 role=%3 group=%4").arg(newUserId).arg(login).arg(role).arg(selectedGroupId));
    loadUsers();
    QMessageBox::information(this,"OK","Пользователь создан");
}

void AdminWindow::onToggleActive() {
    auto sel = tblUsers->selectedItems();
    if (sel.isEmpty()) { showError("Выберите пользователя"); return; }
    int row = tblUsers->currentRow();
    int uid = tblUsers->item(row,0)->text().toInt();

    QSqlQuery q(Database::instance().get());
    q.prepare("UPDATE users SET active = NOT active WHERE id = ?");
    q.addBindValue(uid);
    if (!q.exec()) {
        showError("Не удалось обновить статус: " + q.lastError().text());
        return;
    }
    Logger::log(m_adminId, "toggle_active", QString("user_id=%1").arg(uid));
    loadUsers();
}

void AdminWindow::onEditUser() {
    auto sel = tblUsers->selectedItems();
    if (sel.isEmpty()) { showError("Выберите пользователя"); return; }
    int row = tblUsers->currentRow();
    int uid = tblUsers->item(row,0)->text().toInt();

    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT login, full_name, role, group_id FROM users WHERE id = ?");
    q.addBindValue(uid);
    if (!q.exec() || !q.next()) { showError("Пользователь не найден"); return; }
    QString login = q.value(0).toString();
    QString full = q.value(1).toString();
    QString role = q.value(2).toString();
    QVariant curGroup = q.value(3);

    bool ok;
    QString newFull = QInputDialog::getText(this, "Редактировать", "Full name:", QLineEdit::Normal, full, &ok);
    if (!ok) return;
    QString newRole = QInputDialog::getText(this, "Редактировать", "Role:", QLineEdit::Normal, role, &ok);
    if (!ok) return;

    // group selection
    QSqlQuery gq(Database::instance().get());
    gq.exec("SELECT id, name FROM groups ORDER BY name");
    QStringList groupNames;
    QList<int> groupIds;
    int defaultIndex = -1;
    while (gq.next()) {
        int gid = gq.value(0).toInt();
        QString gname = gq.value(1).toString();
        groupIds << gid;
        groupNames << gname;
        if (curGroup.isValid() && curGroup.toInt() == gid) defaultIndex = groupNames.size()-1;
    }

    int selectedGroupId = -1;
    if (!groupNames.isEmpty()) {
        QString chosen = QInputDialog::getItem(this, "Группа", "Выберите группу (или пусто):", groupNames, qMax(0,defaultIndex), true, &ok);
        if (!ok) return;
        if (!chosen.isEmpty()) {
            int idx = groupNames.indexOf(chosen);
            if (idx >= 0) selectedGroupId = groupIds[idx];
        }
    }

    QSqlQuery uq(Database::instance().get());
    uq.prepare("UPDATE users SET full_name = ?, role = ?, group_id = ? WHERE id = ?");
    uq.addBindValue(newFull);
    uq.addBindValue(newRole);
    if (selectedGroupId == -1) uq.addBindValue(QVariant(QVariant::Int));
    else uq.addBindValue(selectedGroupId);
    uq.addBindValue(uid);
    if (!uq.exec()) { showError("Не удалось обновить пользователя: " + uq.lastError().text()); return; }

    Logger::log(m_adminId, "edit_user", QString("user_id=%1").arg(uid));
    loadUsers();
    QMessageBox::information(this,"OK","Пользователь изменён");
}

void AdminWindow::onRefresh() {
    loadUsers();
}

void AdminWindow::onDeleteUser() {
    auto sel = tblUsers->selectedItems();
    if (sel.isEmpty()) { showError("Выберите пользователя"); return; }
    int row = tblUsers->currentRow();
    int userId = tblUsers->item(row,0)->text().toInt();
    QString login = tblUsers->item(row,1)->text();

    if (QMessageBox::question(this, "Удалить пользователя", QString("Удалить пользователя %1 ?").arg(login),
                              QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) return;

    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT sp_delete_user(?, ?)");
    q.addBindValue(userId);
    q.addBindValue(m_adminId);
    if (!q.exec()) {
        showError("Не удалось удалить пользователя: " + q.lastError().text());
        return;
    }
    Logger::log(m_adminId, "delete_user", QString("user_id=%1").arg(userId));
    loadUsers();
    QMessageBox::information(this,"OK","Пользователь удалён");
}
