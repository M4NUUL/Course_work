#pragma once

#include <string>

namespace auth {

// Проверка правил сложности пароля (длина, допустимые символы и т.п.)
bool validatePasswordRules(const std::string &password, std::string &errMsg);

// Хеширование пароля (Argon2id через libsodium)
// Выход — строка формата "$argon2id$...$salt$hash", готовая к хранению в БД.
// Соль и параметры вписаны внутрь строки, поэтому не нужны отдельные поля.
std::string hashPassword(const std::string &password);

// Проверка пароля по сохранённому хешу (строке из hashPassword)
// Если пароль корректный — вернёт true.
bool verifyPassword(const std::string &password,
                    const std::string &storedHash);

} // namespace auth
