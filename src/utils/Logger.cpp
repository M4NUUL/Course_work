#include "Logger.hpp"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include "db/Database.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

void Logger::log(int userId, const QString &action, const QString &details) {
    QFile f("logs/actions.log");
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&f);
        out << QDateTime::currentDateTime().toString(Qt::ISODate) << " | user:" << userId << " | " << action << " | " << details << "\n";
        f.close();
    }
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("SELECT sp_log_action(?, ?, ?)");
    q.addBindValue(userId);
    q.addBindValue(action);
    q.addBindValue(details);
    if (!q.exec()) {
        qWarning() << "sp_log_action failed:" << q.lastError().text();
        return;
    }
    q.next();
}
