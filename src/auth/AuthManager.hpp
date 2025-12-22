#pragma once
#include <string>

class AuthManager {
public:
    static AuthManager& instance();
    bool registerUser(const std::string &login, const std::string &role, const std::string &password);
    bool authenticate(const std::string &login, const std::string &password, int &outUserId, std::string &outRole);

private:
    AuthManager() = default;
};
