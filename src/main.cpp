#include <QApplication>
#include "db/Database.hpp"
#include "config/ConfigManager.hpp"
#include "gui/LoginWindow.hpp"
#include "gui/MainWindow.hpp"
#include "gui/AdminWindow.hpp"
#include "gui/TeacherWindow.hpp"
#include "gui/StudentWindow.hpp"

#include <QDebug>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    if (!ConfigManager::instance().load("config/config.json")) {
        qWarning() << "Не удалось загрузить configuration file config/config.json";
    }

    if (!Database::instance().open()) {
        qCritical() << "Не удалось открыть базу данных PostgreSQL. Проверьте config/config.json и доступность сервера.";
        return 1;
    }

    // Создаём LoginWindow как указатель (чтобы лямбда могла вызывать login->close())
    LoginWindow *login = new LoginWindow();
    login->show();

    // Подключаем сигнал loginSuccess -> теперь создаём MainWindow, а не отдельные role-windows
    QObject::connect(login, &LoginWindow::loginSuccess, [login](int userId, const QString &role) {
        // Создаём единое главное окно, которое содержит интерфейс в зависимости от роли
        MainWindow *mw = new MainWindow(userId, role);
        mw->setAttribute(Qt::WA_DeleteOnClose);
        mw->show();

        // Закрываем окно логина
        login->close();
    });

    int res = a.exec();
    Database::instance().close();
    return res;
}
