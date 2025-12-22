#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>
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

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    if (!ConfigManager::instance().load("config/config.json")) {
        std::cerr << "Failed to load config/config.json\n";
        return 1;
    }
    if (!Database::instance().open()) {
        std::cerr << "Failed to open database. Ensure PostgreSQL is running and config contains correct credentials.\n";
        return 1;
    }

    std::string login;
    std::cout << "Enter admin login (default 'admin'): ";
    std::getline(std::cin, login);
    if (login.empty()) login = "admin";

    std::string pwd1 = readPassword("Enter password: ");
    std::string pwd2 = readPassword("Repeat password: ");
    if (pwd1 != pwd2) {
        std::cerr << "Passwords do not match\n";
        return 1;
    }

    if (!AuthManager::instance().registerUser(login, "admin", pwd1)) {
        std::cerr << "Failed to create admin user. Maybe user already exists.\n";
        return 1;
    }
    std::cout << "Admin user created: " << login << std::endl;
    return 0;
}
