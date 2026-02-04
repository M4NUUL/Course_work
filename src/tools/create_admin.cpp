#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>

#include <sodium.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include "db/Database.hpp"
#include "config/ConfigManager.hpp"
#include "auth/AuthManager.hpp"

static std::string readPassword(const char *prompt) {
    struct termios oflags, nflags;
    std::string password;

    std::cout << prompt;

    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    tcsetattr(fileno(stdin), TCSANOW, &nflags);

    std::getline(std::cin, password);

    tcsetattr(fileno(stdin), TCSANOW, &oflags);
    std::cout << std::endl;

    return password;
}

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

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    if (sodium_init() == -1) {
        std::cerr << "Ошибка: sodium_init() failed\n";
        return 1;
    }

    const QString cfg = findConfigPath();
    if (cfg.isEmpty()) {
        std::cerr << "Ошибка: не найден config.json\n";
        return 1;
    }

    if (!ConfigManager::instance().load(cfg.toStdString())) {
        std::cerr << "Ошибка: не удалось загрузить конфиг: " << cfg.toStdString() << "\n";
        return 1;
    }

    if (!Database::instance().open()) {
        std::cerr << "Ошибка: не удалось подключиться к PostgreSQL\n";
        return 1;
    }

    std::string login;
    std::cout << "Введите логин администратора (default: admin): ";
    std::getline(std::cin, login);
    if (login.empty()) login = "admin";

    const std::string pwd1 = readPassword("Введите пароль: ");
    const std::string pwd2 = readPassword("Повторите пароль: ");
    if (pwd1 != pwd2) {
        std::cerr << "Ошибка: пароли не совпадают\n";
        return 1;
    }

    if (!AuthManager::instance().registerUser(login, "admin", pwd1)) {
        std::cerr << "Ошибка: не удалось создать пользователя (возможно логин уже существует)\n";
        return 1;
    }

    std::cout << "Администратор создан: " << login << "\n";
    return 0;
}
