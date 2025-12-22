// src/tools/create_submission.cpp
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDebug>

#include "../config/ConfigManager.hpp"
#include "../db/Database.hpp"
#include "../crypto/FileCrypto.hpp"
#include "../crypto/KeyProtect.hpp"

/* helper hex */
static std::string hexEncode(const std::vector<unsigned char>& v) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto c : v) oss << std::setw(2) << (int)c;
    return oss.str();
}

/* base64 via Qt */
static QString toBase64(const std::vector<unsigned char>& v) {
    QByteArray ba((const char*)v.data(), (int)v.size());
    return QString::fromLatin1(ba.toBase64());
}

/* generate random hex uuid (16 bytes -> 32 hex chars) */
static std::string make_uuid_hex() {
    auto v = crypto::genRandomBytes(16);
    return hexEncode(v);
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: create_submission <input_file> <student_id> <assignment_id> [original_name]\n";
        return 1;
    }

    std::string inputFile = argv[1];
    int studentId = std::atoi(argv[2]);
    int assignmentId = std::atoi(argv[3]);
    std::string originalName;
    if (argc >= 5) originalName = argv[4];
    else {
        // extract base name
        QFileInfo fi(QString::fromStdString(inputFile));
        originalName = fi.fileName().toStdString();
    }

    // load config
    if (!ConfigManager::instance().load("config/config.json")) {
        std::cerr << "Failed to load config/config.json\n";
        return 1;
    }

    // open db
    if (!Database::instance().open()) {
        std::cerr << "Failed to open DB\n";
        return 1;
    }

    // Ensure storage dirs
    QDir().mkpath("storage/files");
    QDir().mkpath("storage/metadata");

    // generate uuid
    std::string uuid = make_uuid_hex();
    std::string filename = uuid + ".dat";
    std::string outPath = std::string("storage/files/") + filename;

    // generate file key and iv
    std::vector<unsigned char> fileKey = crypto::genRandomBytes(32);
    std::vector<unsigned char> fileIv = crypto::genRandomBytes(16);

    std::string err;
    if (!crypto::aes256_cbc_encrypt(fileKey, fileIv, inputFile, outPath, err)) {
        std::cerr << "Encrypt file failed: " << err << "\n";
        return 1;
    }

    // Protect fileKey with master key (AES-GCM)
    auto master = ConfigManager::instance().masterKey();
    if (master.empty()) {
        std::cerr << "Master key is empty in config\n";
        return 1;
    }

    std::vector<unsigned char> encKey, keyIv, keyTag;
    if (!keyprotect::encryptWithAesGcm(master, fileKey, encKey, keyIv, keyTag, err)) {
        std::cerr << "Key protect failed: " << err << "\n";
        return 1;
    }

    // write metadata JSON
    QJsonObject meta;
    meta["owner_id"] = studentId; // owner = student
    meta["original_name"] = QString::fromStdString(originalName);
    meta["iv"] = toBase64(fileIv);
    meta["key_encrypted"] = toBase64(encKey);
    meta["key_iv"] = toBase64(keyIv);
    meta["key_tag"] = toBase64(keyTag);
    QJsonDocument doc(meta);

    QString metaPath = QString("storage/metadata/%1.json").arg(QString::fromStdString(uuid));
    QFile mf(metaPath);
    if (!mf.open(QIODevice::WriteOnly)) {
        std::cerr << "Cannot open metadata file for writing: " << metaPath.toStdString() << "\n";
        return 1;
    }
    mf.write(doc.toJson(QJsonDocument::Indented));
    mf.close();

    // insert into DB submissions
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("INSERT INTO submissions (assignment_id, student_id, file_path, original_name, uploaded_at) VALUES (?, ?, ?, ?, NOW())");
    q.addBindValue(assignmentId);
    q.addBindValue(studentId);
    q.addBindValue(QString::fromStdString(filename));
    q.addBindValue(QString::fromStdString(originalName));
    if (!q.exec()) {
        std::cerr << "Insert submission failed: " << q.lastError().text().toStdString() << "\n";
        return 1;
    }

    std::cout << "Submission created. uuid=" << uuid << " file=" << outPath << "\n";
    return 0;
}
