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
    setWindowTitle("EduDesk — Администратор");
    resize(900, 600);

    auto v = new QVBoxLayout(this);

    // Таблица пользователей: ID, Login, Full name, Role
    tblUsers = new QTableWidget(this);
    tblUsers->setColumnCount(3);
    tblUsers->setHorizontalHeaderLabels({"Login", "Full name", "Role"});
    tblUsers->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblUsers->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblUsers->horizontalHeader()->setStretchLastSection(true);
    v->addWidget(new QLabel("Пользователи:"));
    v->addWidget(tblUsers, 1);

    // Панель кнопок для работы с пользователями
    auto h = new QHBoxLayout();
    btnCreateUser   = new QPushButton("Создать пользователя", this);
    btnEditUser     = new QPushButton("Редактировать", this);
    btnToggleActive = new QPushButton("Актив/Деактив", this);
    btnDeleteUser   = new QPushButton("Удалить пользователя", this);
    btnRefresh      = new QPushButton("Обновить", this);

    h->addWidget(btnCreateUser);
    h->addWidget(btnEditUser);
    h->addWidget(btnToggleActive);
    h->addWidget(btnDeleteUser);
    h->addStretch();
    h->addWidget(btnRefresh);

    v->addLayout(h);

    // Сигналы
    connect(btnCreateUser,   &QPushButton::clicked, this, &AdminWindow::onCreateUser);
    connect(btnEditUser,     &QPushButton::clicked, this, &AdminWindow::onEditUser);
    connect(btnToggleActive, &QPushButton::clicked, this, &AdminWindow::onToggleActive);
    connect(btnDeleteUser,   &QPushButton::clicked, this, &AdminWindow::onDeleteUser);
    connect(btnRefresh,      &QPushButton::clicked, this, &AdminWindow::onRefresh);

    // Первичная загрузка пользователей
    loadUsers();
}

void AdminWindow::showError(const QString &text) {
    QMessageBox::warning(this, "Ошибка", text);
}

void AdminWindow::loadUsers() {
    tblUsers->setRowCount(0);
    QSqlQuery q(Database::instance().get());

    // Загрузка пользователей без привязки к группам
    if (!q.exec("SELECT u.id, u.login, COALESCE(u.full_name, ''), u.role "
                "FROM users u "
                "ORDER BY u.id")) {
        showError("Не удалось загрузить пользователей: " + q.lastError().text());
        return;
    }

    int r = 0;
while (q.next()) {
    const int uid = q.value(0).toInt();
    const QString login = q.value(1).toString();
    const QString full  = q.value(2).toString();
    const QString role  = q.value(3).toString();

    tblUsers->insertRow(r);

    auto *loginItem = new QTableWidgetItem(login);
    loginItem->setData(Qt::UserRole, uid);   // id НЕ отображаем, храним как metadata
    tblUsers->setItem(r, 0, loginItem);

    tblUsers->setItem(r, 1, new QTableWidgetItem(full));
    tblUsers->setItem(r, 2, new QTableWidgetItem(role));

    ++r;
}
tblUsers->resizeColumnsToContents();
}

void AdminWindow::onCreateUser() {
    bool ok;
    QString login = QInputDialog::getText(this, "Создать пользователя", "Login:",
                                          QLineEdit::Normal, "", &ok);
    if (!ok || login.trimmed().isEmpty()) return;

    QString role = QInputDialog::getText(this, "Создать пользователя",
                                         "Role (student/teacher/admin):",
                                         QLineEdit::Normal, "student", &ok);
    if (!ok || role.trimmed().isEmpty()) return;

    QString full = QInputDialog::getText(this, "Создать пользователя", "Full name:",
                                         QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QString pwd = QInputDialog::getText(this, "Создать пользователя", "Password:",
                                        QLineEdit::Password, "", &ok);
    if (!ok || pwd.isEmpty()) return;

    // Регистрация пользователя (логин/роль/пароль)
    if (!AuthManager::instance().registerUser(login.toStdString(),
                                              role.toStdString(),
                                              pwd.toStdString())) {
        showError("Не удалось создать пользователя (возможно логин занят)");
        return;
    }

    // Получение id созданного пользователя
    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT id FROM users WHERE login = ?");
    q.addBindValue(login);
    if (!q.exec() || !q.next()) {
        showError("Пользователь создан, но не удалось получить id. Проверьте базу.");
        loadUsers();
        return;
    }
    int newUserId = q.value(0).toInt();

    // Обновление поля full_name для созданного пользователя
    QSqlQuery uq(Database::instance().get());
    uq.prepare("UPDATE users SET full_name = ? WHERE id = ?");
    uq.addBindValue(full);
    uq.addBindValue(newUserId);
    if (!uq.exec()) {
        showError("Пользователь создан, но не удалось сохранить имя: " + uq.lastError().text());
        Logger::log(m_adminId, "create_user_partial",
                    QString("login=%1 id=%2 full_name_fail").arg(login).arg(newUserId));
        loadUsers();
        return;
    }

    Logger::log(m_adminId, "create_user",
                QString("id=%1 login=%2 role=%3").arg(newUserId).arg(login).arg(role));
    loadUsers();
    QMessageBox::information(this, "OK", "Пользователь создан");
}

void AdminWindow::onToggleActive() {
    auto sel = tblUsers->selectedItems();
    if (sel.isEmpty()) {
        showError("Выберите пользователя");
        return;
    }
    int uid = selectedUserId();
    if (uid < 0) { showError("Выберите пользователя"); return; }


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
    if (sel.isEmpty()) {
        showError("Выберите пользователя");
        return;
    }
    int uid = selectedUserId();
    if (uid < 0) { showError("Выберите пользователя"); return; }
    QSqlQuery q(Database::instance().get());
    q.prepare("SELECT login, full_name, role FROM users WHERE id = ?");
    q.addBindValue(uid);
    if (!q.exec() || !q.next()) {
        showError("Пользователь не найден");
        return;
    }
    QString login = q.value(0).toString();
    QString full  = q.value(1).toString();
    QString role  = q.value(2).toString();

    bool ok;
    QString newFull = QInputDialog::getText(this, "Редактировать", "Full name:",
                                            QLineEdit::Normal, full, &ok);
    if (!ok) return;

    QString newRole = QInputDialog::getText(this, "Редактировать", "Role:",
                                            QLineEdit::Normal, role, &ok);
    if (!ok) return;

    QSqlQuery uq(Database::instance().get());
    uq.prepare("UPDATE users SET full_name = ?, role = ? WHERE id = ?");
    uq.addBindValue(newFull);
    uq.addBindValue(newRole);
    uq.addBindValue(uid);
    if (!uq.exec()) {
        showError("Не удалось обновить пользователя: " + uq.lastError().text());
        return;
    }

    Logger::log(m_adminId, "edit_user", QString("user_id=%1 login=%2").arg(uid).arg(login));
    loadUsers();
    QMessageBox::information(this, "OK", "Пользователь изменён");
}

void AdminWindow::onRefresh() {
    loadUsers();
}

void AdminWindow::onDeleteUser() {
    auto sel = tblUsers->selectedItems();
    if (sel.isEmpty()) {
        showError("Выберите пользователя");
        return;
    }
    int row = tblUsers->currentRow();
    if (row < 0) { showError("Выберите пользователя"); return; }

    int userId = selectedUserId();
    if (userId < 0) { showError("Выберите пользователя"); return; }

    QString login = tblUsers->item(row, 0)->text(); // login в 0-й колонке


    if (QMessageBox::question(this, "Удалить пользователя",
                              QString("Удалить пользователя %1 ?").arg(login),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

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
    QMessageBox::information(this, "OK", "Пользователь удалён");
}

int AdminWindow::selectedUserId() const {
    int row = tblUsers->currentRow();
    if (row < 0) return -1;
    auto *item = tblUsers->item(row, 0); // login
    if (!item) return -1;
    return item->data(Qt::UserRole).toInt();
}