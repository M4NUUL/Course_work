#include "PasswordUtils.hpp"

#include <sodium.h>

#include <regex>
#include <string>

// Здесь остаётся логика проверки правил пароля, всё крипто-движение уехало в libsodium
namespace auth {

// Проверка базовых правил сложности пароля
bool validatePasswordRules(const std::string &password, std::string &errMsg) {
    // Минимальная длина
    if (password.size() < 10) {
        errMsg = "Пароль должен быть не меньше 10 символов";
        return false;
    }

    // Запрет пробелов (чисто чтобы не ловить лишние проблемы)
    if (password.find(' ') != std::string::npos) {
        errMsg = "Пароль не должен содержать пробелов";
        return false;
    }

    // Набор регулярных выражений под разные типы символов
    std::regex lower("[a-z]");
    std::regex upper("[A-Z]");
    std::regex digit("\\d");
    std::regex special("[!@#\\$%\\^&\\*\\(\\)\\-_=\\+\\[\\]\\{\\}\\|;:'\",.<>\\/?]");

    if (!std::regex_search(password, lower)) {
        errMsg = "Пароль должен содержать строчную букву";
        return false;
    }
    if (!std::regex_search(password, upper)) {
        errMsg = "Пароль должен содержать заглавную букву";
        return false;
    }
    if (!std::regex_search(password, digit)) {
        errMsg = "Пароль должен содержать цифру";
        return false;
    }
    if (!std::regex_search(password, special)) {
        errMsg = "Пароль должен содержать специальный символ";
        return false;
    }

    return true;
}

// Хеширование пароля через libsodium (Argon2id)
// На выходе — строка, которую можно напрямую класть в поле password_hash
std::string hashPassword(const std::string &password) {
    char out[crypto_pwhash_STRBYTES];

    // Используем "interactive" профиль — его достаточно для пользовательских паролей
    if (crypto_pwhash_str(
            out,
            password.c_str(),
            password.size(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        // Если сюда попали — либо проблемы с памятью, либо что-то серьёзно сломалось
        throw std::runtime_error("crypto_pwhash_str failed");
    }

    return std::string(out);
}

// Проверка пароля по сохранённой строке (из hashPassword)
bool verifyPassword(const std::string &password,
                    const std::string &storedHash) {
    int rc = crypto_pwhash_str_verify(
        storedHash.c_str(),
        password.c_str(),
        password.size()
    );

    // 0 — успешная проверка, всё остальное считай неудача
    return (rc == 0);
}

}
