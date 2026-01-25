#pragma once

#include <vector>
#include <string>

class ConfigManager {
public:
    static ConfigManager& instance();

    bool load(const std::string &path);

    std::vector<unsigned char> masterKey() const;
    int pbkdf2Iterations() const;

    std::string dbHost() const;
    int dbPort() const;
    std::string dbName() const;
    std::string dbUser() const;
    std::string dbPassword() const;

private:
    ConfigManager() = default;

    std::vector<unsigned char> m_master;
    int m_iter = 100000;

    std::string m_dbHost = "localhost";
    int m_dbPort = 5432;
    std::string m_dbName = "jumandgi";
    std::string m_dbUser = "jumandgi_user";
    std::string m_dbPassword = "changeme";
};
