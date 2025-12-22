#include "ConfigManager.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QDebug>

static std::vector<unsigned char> hexToBytes(const QString &hex) {
    std::vector<unsigned char> out;
    QString s = hex;
    if (s.size() % 2 != 0) return out;
    for (int i=0;i<s.size(); i+=2) {
        bool ok;
        unsigned int byte = s.mid(i,2).toUInt(&ok,16);
        if (!ok) return std::vector<unsigned char>();
        out.push_back((unsigned char)byte);
    }
    return out;
}

ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

bool ConfigManager::load(const std::string &path) {
    QFile f(QString::fromStdString(path));
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open config" << path.c_str();
        return false;
    }
    auto data = f.readAll();
    f.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return false;
    QJsonObject o = doc.object();
    QString mk = o.value("master_key_hex").toString();
    m_iter = o.value("pbkdf2_iterations").toInt(100000);

    if (o.contains("db") && o.value("db").isObject()) {
        QJsonObject db = o.value("db").toObject();
        m_dbHost = db.value("host").toString("localhost").toStdString();
        m_dbPort = db.value("port").toInt(5432);
        m_dbName = db.value("name").toString("jumandgi").toStdString();
        m_dbUser = db.value("user").toString("jumandgi_user").toStdString();
        m_dbPassword = db.value("password").toString("changeme").toStdString();
    }
    m_master = hexToBytes(mk);
    return m_master.size() == 32;
}

std::vector<unsigned char> ConfigManager::masterKey() const { return m_master; }
int ConfigManager::pbkdf2Iterations() const { return m_iter; }
std::string ConfigManager::dbHost() const { return m_dbHost; }
int ConfigManager::dbPort() const { return m_dbPort; }
std::string ConfigManager::dbName() const { return m_dbName; }
std::string ConfigManager::dbUser() const { return m_dbUser; }
std::string ConfigManager::dbPassword() const { return m_dbPassword; }
