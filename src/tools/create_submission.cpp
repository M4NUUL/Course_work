#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>

#include <sodium.h>

#include "../config/ConfigManager.hpp"
#include "../db/Database.hpp"
#include "../crypto/FileCrypto.hpp"
#include "../crypto/KeyProtect.hpp"

static std::string hexEncode(const std::vector<unsigned char>& v) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto c : v) oss << std::setw(2) << static_cast<int>(c);
    return oss.str();
}

static QString toBase64(const std::vector<unsigned char>& v) {
    QByteArray ba(reinterpret_cast<const char*>(v.data()), static_cast<int>(v.size()));
    return QString::fromLatin1(ba.toBase64());
}

static std::string make_uuid_hex() {
    return hexEncode(crypto::genRandomBytes(16));
}

static QString storagePathQ(const std::string &rel) {
    return QString::fromStdString(ConfigManager::instance().storagePath(rel));
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    if (sodium_init() == -1) {
        std::cerr << "Ошибка: sodium_init() failed\n";
        return 1;
    }

    if (argc < 4) {
        std::cerr << "Usage: create_submission <input_file> <student_id> <assignment_id> [original_name]\n";
        return 1;
    }

    const std::string inputFile = argv[1];
    const int studentId = std::atoi(argv[2]);
    const int assignmentId = std::atoi(argv[3]);

    std::string originalName;
    if (argc >= 5) {
        originalName = argv[4];
    } else {
        QFileInfo fi(QString::fromStdString(inputFile));
        originalName = fi.fileName().toStdString();
    }

    if (!ConfigManager::instance().load("config/config.json")) {
        std::cerr << "Ошибка: не удалось загрузить config/config.json\n";
        return 1;
    }

    if (!ConfigManager::instance().ensureStorageLayout()) {
        std::cerr << "Ошибка: не удалось подготовить директории хранилища\n";
        return 1;
    }

    if (!Database::instance().open()) {
        std::cerr << "Ошибка: не удалось открыть соединение с БД\n";
        return 1;
    }

    const std::string uuid = make_uuid_hex();
    const std::string filename = uuid + ".dat";

    const std::string outPath = ConfigManager::instance().storagePath(std::string("files/") + filename);
    const std::string metaPath = ConfigManager::instance().storagePath(std::string("metadata/") + uuid + ".json");

    std::vector<unsigned char> fileKey = crypto::genRandomBytes(32);
    std::vector<unsigned char> fileIv  = crypto::genRandomBytes(16);

    std::string err;
    if (!crypto::aes256_cbc_encrypt(fileKey, fileIv, inputFile, outPath, err)) {
        std::cerr << "Ошибка: не удалось зашифровать файл: " << err << "\n";
        return 1;
    }

    auto master = ConfigManager::instance().masterKey();
    if (master.empty()) {
        std::cerr << "Ошибка: мастер-ключ пустой в config.json\n";
        return 1;
    }

    std::vector<unsigned char> encKey, keyIv, keyTag;
    if (!keyprotect::encryptWithAesGcm(master, fileKey, encKey, keyIv, keyTag, err)) {
        std::cerr << "Ошибка: не удалось защитить ключ файла: " << err << "\n";
        return 1;
    }

    QJsonObject meta;
    meta["owner_id"]      = studentId;
    meta["original_name"] = QString::fromStdString(originalName);
    meta["iv"]            = toBase64(fileIv);
    meta["key_encrypted"] = toBase64(encKey);
    meta["key_iv"]        = toBase64(keyIv);
    meta["key_tag"]       = toBase64(keyTag);

    QFile mf(QString::fromStdString(metaPath));
    if (!mf.open(QIODevice::WriteOnly)) {
        std::cerr << "Ошибка: не удалось открыть metadata для записи: " << metaPath << "\n";
        return 1;
    }
    mf.write(QJsonDocument(meta).toJson(QJsonDocument::Indented));
    mf.close();

    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("SELECT sp_create_submission(?, ?, ?, ?)");
    q.addBindValue(assignmentId);
    q.addBindValue(studentId);
    q.addBindValue(QString::fromStdString(filename));
    q.addBindValue(QString::fromStdString(originalName));

    if (!q.exec()) {
        std::cerr << "Ошибка: insert в submissions провалился: "
                  << q.lastError().text().toStdString() << "\n";
        return 1;
    }

    std::cout << "Submission создан. uuid=" << uuid << " file=" << outPath << "\n";
    return 0;
}
