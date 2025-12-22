#include <QApplication>
#include "db/Database.hpp"
#include "config/ConfigManager.hpp"
#include "gui/LoginWindow.hpp"
#include "gui/MainWindow.hpp"
#include "gui/AdminWindow.hpp"     // добавьте
#include "gui/TeacherWindow.hpp"   // уже мог быть
#include "gui/StudentWindow.hpp"   // если добавлен

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

    // Подключаем сигнал loginSuccess
    QObject::connect(login, &LoginWindow::loginSuccess, [login](int userId, const QString &role) {
        if (role == "admin") {
            AdminWindow *aw = new AdminWindow(userId);
            aw->setAttribute(Qt::WA_DeleteOnClose);
            aw->show();
        } else if (role == "teacher") {
            TeacherWindow *tw = new TeacherWindow(userId);
            tw->setAttribute(Qt::WA_DeleteOnClose);
            tw->show();
        } else { // student
            StudentWindow *sw = new StudentWindow(userId);
            sw->setAttribute(Qt::WA_DeleteOnClose);
            sw->show();
        }
        // Закрываем окно логина (используем ->
        login->close();
    });

    int res = a.exec();
    Database::instance().close();
    return res;
}
