#include "AuthManager.hpp"
#include "PasswordUtils.hpp"
#include "../db/Database.hpp"
#include "../config/ConfigManager.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDebug>

// Синглтон менеджера аутентификации
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
        qWarning() << "Password invalid:" << QString::fromStdString(err);
        return false;
    }

    int iters = ConfigManager::instance().pbkdf2Iterations();
    if (iters <= 0) {
        iters = 100000;
    }

    // Создание PBKDF2-хеша
    auto pw = auth::createPasswordHash(password, iters);

    QString loginQ = QString::fromStdString(login);
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);

    q.prepare(R"SQL(
        INSERT INTO users (login, role, password_hash, salt, created_at, active)
        VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP, true)
    )SQL");
    q.addBindValue(loginQ);
    q.addBindValue(QString::fromStdString(role));
    q.addBindValue(QString::fromStdString(auth::toHex(pw.hash)));
    q.addBindValue(QString::fromStdString(auth::toHex(pw.salt)));

    if (!q.exec()) {
        qWarning() << "Register error:" << q.lastError().text();
        return false;
    }
    return true;
}

bool AuthManager::authenticate(const std::string &login,
                               const std::string &password,
                               int &outUserId,
                               std::string &outRole)
{
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);

    q.prepare(R"SQL(
        SELECT id, password_hash, salt, role
        FROM users
        WHERE login = ? AND active = true
    )SQL");
    q.addBindValue(QString::fromStdString(login));

    if (!q.exec()) {
        qWarning() << "Auth query error:" << q.lastError().text();
        return false;
    }

    if (!q.next()) {
        return false;
    }

    int id          = q.value(0).toInt();
    QString hashHex = q.value(1).toString();
    QString saltHex = q.value(2).toString();
    QString roleQ   = q.value(3).toString();

    auto hash = auth::fromHex(hashHex.toStdString());
    auto salt = auth::fromHex(saltHex.toStdString());

    int iters = ConfigManager::instance().pbkdf2Iterations();
    if (iters <= 0) {
        iters = 100000;
    }

    if (auth::verifyPassword(password, salt, hash, iters)) {
        outUserId = id;
        outRole   = roleQ.toStdString();
        return true;
    }
    return false;
}
