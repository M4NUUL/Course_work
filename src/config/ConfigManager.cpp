#include "ConfigManager.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QString>

static std::vector<unsigned char> hexToBytes(const QString &hex) {
    std::vector<unsigned char> out;
    QString s = hex.trimmed();
    if (s.size() % 2 != 0) return out;
    out.reserve(s.size() / 2);
    for (int i = 0; i < s.size(); i += 2) {
        bool ok = false;
        unsigned int byte = s.mid(i, 2).toUInt(&ok, 16);
        if (!ok) {
            out.clear();
            return out;
        }
        out.push_back(static_cast<unsigned char>(byte));
    }
    return out;
}

static QString expandHome(const QString &p) {
    if (p.startsWith("~/")) return QDir::homePath() + p.mid(1);
    if (p == "~") return QDir::homePath();
    return p;
}

static QString defaultStorageRoot() {
    return QDir::home().filePath(".local/share/edudesk");
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
    const QByteArray data = f.readAll();
    f.close();

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Config is not a JSON object";
        return false;
    }

    const QJsonObject o = doc.object();

    const QString mk = o.value("master_key_hex").toString();
    if (mk.isEmpty()) {
        qWarning() << "master_key_hex is missing";
        return false;
    }

    if (o.contains("pbkdf2_iterations")) {
        m_iter = o.value("pbkdf2_iterations").toInt(100000);
        if (m_iter <= 0) m_iter = 100000;
    }

    if (o.contains("db") && o.value("db").isObject()) {
        const QJsonObject db = o.value("db").toObject();
        m_dbHost = db.value("host").toString("127.0.0.1").toStdString();
        m_dbPort = db.value("port").toInt(5432);
        m_dbName = db.value("name").toString("edudesk").toStdString();
        m_dbUser = db.value("user").toString("edudesk").toStdString();
        m_dbPassword = db.value("password").toString("edudesk_pass").toStdString();
    }

    QString storage = defaultStorageRoot();
    if (o.contains("storage") && o.value("storage").isObject()) {
        const QJsonObject so = o.value("storage").toObject();
        const QString root = so.value("root").toString();
        if (!root.trimmed().isEmpty()) storage = root.trimmed();
    }
    storage = expandHome(storage);
    m_storageRoot = QDir::cleanPath(storage).toStdString();

    m_master = hexToBytes(mk);
    if (m_master.size() != 32) {
        qWarning() << "master_key_hex has invalid size, expected 32 bytes";
        m_master.clear();
        return false;
    }

    if (!ensureStorageLayout()) {
        qWarning() << "Cannot create storage layout at" << QString::fromStdString(m_storageRoot);
        return false;
    }

    return true;
}

std::vector<unsigned char> ConfigManager::masterKey() const {
    return m_master;
}

int ConfigManager::pbkdf2Iterations() const {
    return m_iter;
}

std::string ConfigManager::dbHost() const {
    return m_dbHost;
}

int ConfigManager::dbPort() const {
    return m_dbPort;
}

std::string ConfigManager::dbName() const {
    return m_dbName;
}

std::string ConfigManager::dbUser() const {
    return m_dbUser;
}

std::string ConfigManager::dbPassword() const {
    return m_dbPassword;
}

std::string ConfigManager::storageRoot() const {
    if (!m_storageRoot.empty()) return m_storageRoot;
    return defaultStorageRoot().toStdString();
}

std::string ConfigManager::storagePath(const std::string &relative) const {
    const QString root = QString::fromStdString(storageRoot());
    const QString rel = QString::fromStdString(relative);
    return QDir(root).filePath(rel).toStdString();
}

bool ConfigManager::ensureStorageLayout() const {
    const QString root = QString::fromStdString(storageRoot());
    QDir d(root);

    if (!d.exists()) {
        if (!QDir().mkpath(root)) return false;
    }

    QDir().mkpath(d.filePath("files"));
    QDir().mkpath(d.filePath("metadata"));
    QDir().mkpath(d.filePath("assignments"));

    return true;
}
