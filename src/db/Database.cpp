#include "Database.hpp"
#include "../config/ConfigManager.hpp"
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

Database& Database::instance() {
    static Database inst;
    return inst;
}

bool Database::open() {
    auto &cfg = ConfigManager::instance();
    QString host = QString::fromStdString(cfg.dbHost());
    int port = cfg.dbPort();
    QString dbname = QString::fromStdString(cfg.dbName());
    QString user = QString::fromStdString(cfg.dbUser());
    QString pass = QString::fromStdString(cfg.dbPassword());

    if (QSqlDatabase::contains("JumandgiConnection")) {
        db = QSqlDatabase::database("JumandgiConnection");
    } else {
        db = QSqlDatabase::addDatabase("QPSQL", "JumandgiConnection");
        db.setHostName(host);
        db.setPort(port);
        db.setDatabaseName(dbname);
        db.setUserName(user);
        db.setPassword(pass);
    }
    if (!db.open()) {
        qWarning() << "Failed to open Postgres DB:" << db.lastError().text();
        return false;
    }
    return true;
}

QSqlDatabase Database::get() const {
    return db;
}

void Database::close() {
    if (db.isOpen()) db.close();
}
