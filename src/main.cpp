#include <QApplication>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <sodium.h>

#include "db/Database.hpp"
#include "config/ConfigManager.hpp"
#include "gui/LoginWindow.hpp"
#include "gui/MainWindow.hpp"

static QString findConfigPath() {
    const QString appDir = QCoreApplication::applicationDirPath();

    const QString p1 = QDir(appDir).filePath("config/config.json");
    if (QFileInfo(p1).exists()) return p1;

    const QString p2 = QDir(appDir).filePath("config.json");
    if (QFileInfo(p2).exists()) return p2;

    const QString p3 = QDir::current().filePath("config/config.json");
    if (QFileInfo(p3).exists()) return p3;

    const QString p4 = QDir::current().filePath("config.json");
    if (QFileInfo(p4).exists()) return p4;

    return QString();
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    if (sodium_init() == -1) {
        qCritical() << "Ошибка: sodium_init() failed";
        return 1;
    }

    const QString cfg = findConfigPath();
    if (cfg.isEmpty()) {
        qCritical() << "Ошибка: не найден config.json";
        return 1;
    }

    if (!ConfigManager::instance().load(cfg.toStdString())) {
        qCritical() << "Ошибка: не удалось загрузить конфиг:" << cfg;
        return 1;
    }

    if (!Database::instance().open()) {
        qCritical() << "Ошибка: не удалось открыть базу PostgreSQL. Проверьте параметры подключения";
        return 1;
    }

    LoginWindow *login = new LoginWindow();
    login->show();

    QObject::connect(login, &LoginWindow::loginSuccess, [login](int userId, const QString &role) {
        MainWindow *mw = new MainWindow(userId, role);
        mw->setAttribute(Qt::WA_DeleteOnClose);
        mw->show();
        login->close();
    });

    const int res = a.exec();
    Database::instance().close();
    return res;
}
