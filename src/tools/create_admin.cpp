#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>

#include <sodium.h>

#include "db/Database.hpp"
#include "config/ConfigManager.hpp"
#include "auth/AuthManager.hpp"

// Ввод пароля без отображения (unix-стиль)
// (почему-то редко кто делает это нормально)
static std::string readPassword(const char *prompt) {
    struct termios oflags, nflags;
    std::string password;

    std::cout << prompt;

    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    tcsetattr(fileno(stdin), TCSANOW, &nflags);

    std::getline(std::cin, password);

    // Возврат настройки терминала
    tcsetattr(fileno(stdin), TCSANOW, &oflags);
    std::cout << std::endl;

    return password;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Инициализация libsodium (криптографическое ядро)
    if (sodium_init() == -1) {
        std::cerr << "Ошибка: sodium_init() failed" << std::endl;
        return 1;
    }

    // Загрузка конфигурации (db параметры + мастер-ключ)
    if (!ConfigManager::instance().load("config/config.json")) {
        std::cerr << "Ошибка: не удалось загрузить config/config.json" << std::endl;
        return 1;
    }

    // Открытие соединения с PostgreSQL
    if (!Database::instance().open()) {
        std::cerr << "Ошибка: не удалось подключиться к PostgreSQL. Проверьте параметры в config/config.json" << std::endl;
        return 1;
    }

    // Ввод логина администратора
    std::string login;
    std::cout << "Введите логин администратора (default: admin): ";
    std::getline(std::cin, login);
    if (login.empty()) {
        login = "admin";
    }

    // Ввод пароля (дважды)
    std::string pwd1 = readPassword("Введите пароль: ");
    std::string pwd2 = readPassword("Повторите пароль: ");
    if (pwd1 != pwd2) {
        std::cerr << "Ошибка: пароли не совпадают" << std::endl;
        return 1;
    }

    // Создание пользователя с ролью admin
    if (!AuthManager::instance().registerUser(login, "admin", pwd1)) {
        std::cerr << "Ошибка: не удалось создать пользователя (возможно логин уже существует)" << std::endl;
        return 1;
    }

    std::cout << "Администратор создан: " << login << std::endl;
    return 0;
}
