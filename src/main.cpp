#include <QApplication>
#include <QDebug>

#include <sodium.h>

#include "db/Database.hpp"
#include "config/ConfigManager.hpp"
#include "gui/LoginWindow.hpp"
#include "gui/MainWindow.hpp"
#include "gui/AdminWindow.hpp"
#include "gui/TeacherWindow.hpp"
#include "gui/StudentWindow.hpp"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Инициализация libsodium (криптобиблиотека)
    if (sodium_init() == -1) {
        qCritical() << "Ошибка: sodium_init() failed";
        return 1;
    }

    // Загрузка конфигурационного файла (config/config.json)
    if (!ConfigManager::instance().load("config/config.json")) {
        qCritical() << "Ошибка: не удалось загрузить config/config.json";
        return 1;
    }

    // Подключение к PostgreSQL (параметры берутся из config.json)
    if (!Database::instance().open()) {
        qCritical() << "Ошибка: не удалось открыть базу PostgreSQL. Проверьте параметры подключения";
        return 1;
    }

    // Создание окна авторизации
    LoginWindow *login = new LoginWindow();
    login->show();

    // Привязка сигнала успешного входа -> переключение в главное окно
    QObject::connect(login, &LoginWindow::loginSuccess,
                     [login](int userId, const QString &role) {

        // Создание основного окна (интерфейс зависит от роли пользователя)
        MainWindow *mw = new MainWindow(userId, role);
        mw->setAttribute(Qt::WA_DeleteOnClose);
        mw->show();

        // Закрытие окна логина
        login->close();
    });

    // Запуск цикла обработки событий Qt
    int res = a.exec();

    // Завершение работы: закрытие соединения с БД
    Database::instance().close();
    return res;
}
