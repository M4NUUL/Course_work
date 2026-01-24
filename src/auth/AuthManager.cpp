#include "AuthManager.hpp"
#include "PasswordUtils.hpp"
#include "../db/Database.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QString>
#include <QDebug>

// Здесь хранится вся логика регистрации и проверки логина/пароля
AuthManager& AuthManager::instance() {
    static AuthManager inst;
    return inst;
}

bool AuthManager::registerUser(const std::string &login,
                               const std::string &role,
                               const std::string &password)
{
    // Проверка правил сложности пароля
    std::string err;
    if (!auth::validatePasswordRules(password, err)) {
        qWarning() << "Пароль не проходит правила:" << QString::fromStdString(err);
        return false;
    }

    // Хеширование пароля (Argon2id через libsodium)
    std::string passwordHash;
    try {
        passwordHash = auth::hashPassword(password);
    } catch (const std::exception &ex) {
        qWarning() << "Ошибка при хешировании пароля:" << ex.what();
        return false;
    }

    // Подготовка INSERT в таблицу users
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);

    q.prepare("INSERT INTO users "
              "(login, role, password_hash, salt, created_at, active) "
              "VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP, true)");

    q.addBindValue(QString::fromStdString(login));
    q.addBindValue(QString::fromStdString(role));
    q.addBindValue(QString::fromStdString(passwordHash));
    // Поле salt остаётся в схеме, но фактически не используется (libsodium прячет соль внутрь строки)
    q.addBindValue(QString(""));

    // Выполнение INSERT
    if (!q.exec()) {
        qWarning() << "Ошибка регистрации пользователя:" << q.lastError().text();
        return false;
    }

    return true;
}

bool AuthManager::authenticate(const std::string &login,
                               const std::string &password,
                               int &outUserId,
                               std::string &outRole)
{
    // Получение записи пользователя по логину
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);

    q.prepare("SELECT id, password_hash, role "
              "FROM users "
              "WHERE login = ? AND active = true");
    q.addBindValue(QString::fromStdString(login));

    if (!q.exec()) {
        qWarning() << "Ошибка запроса авторизации:" << q.lastError().text();
        return false;
    }

    // Пользователь не найден или не активен
    if (!q.next()) {
        return false;
    }

    int userId        = q.value(0).toInt();
    QString hashStr   = q.value(1).toString();
    QString roleStr   = q.value(2).toString();

    // Проверка пароля через libsodium
    bool ok = false;
    try {
        ok = auth::verifyPassword(password, hashStr.toStdString());
    } catch (const std::exception &ex) {
        qWarning() << "Ошибка при проверке пароля:" << ex.what();
        return false;
    }

    if (!ok) {
        // Неверный пароль
        return false;
    }

    // Заполнение выходных параметров при успешной авторизации
    outUserId = userId;
    outRole   = roleStr.toStdString();
    return true;
}
