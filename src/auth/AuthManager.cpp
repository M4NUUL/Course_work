#include "AuthManager.hpp"
#include "PasswordUtils.hpp"
#include "../db/Database.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDebug>

AuthManager& AuthManager::instance() {
    static AuthManager inst;
    return inst;
}

bool AuthManager::registerUser(const std::string &login, const std::string &role, const std::string &password) {
    std::string err;
    if (!auth::validatePasswordRules(password, err)) {
        qWarning() << "Password invalid:" << QString::fromStdString(err);
        return false;
    }
    auto pw = auth::createPasswordHash(password, 100000);
    QString loginQ = QString::fromStdString(login);
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);

    q.prepare("INSERT INTO users (login, role, password_hash, salt, created_at, active) VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP, true)");
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

bool AuthManager::authenticate(const std::string &login, const std::string &password, int &outUserId, std::string &outRole) {
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("SELECT id, password_hash, salt, role FROM users WHERE login = ? AND active = true");
    q.addBindValue(QString::fromStdString(login));
    if (!q.exec()) {
        qWarning() << "Auth query error:" << q.lastError().text();
        return false;
    }
    if (!q.next()) return false;
    int id = q.value(0).toInt();
    QString hashHex = q.value(1).toString();
    QString saltHex = q.value(2).toString();
    QString role = q.value(3).toString();

    auto hash = auth::fromHex(hashHex.toStdString());
    auto salt = auth::fromHex(saltHex.toStdString());

    if (auth::verifyPassword(password, salt, hash, 100000)) {
        outUserId = id;
        outRole = role.toStdString();
        return true;
    }
    return false;
}
