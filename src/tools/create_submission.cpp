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
#include <QDebug>

#include <sodium.h>

#include "../config/ConfigManager.hpp"
#include "../db/Database.hpp"
#include "../crypto/FileCrypto.hpp"
#include "../crypto/KeyProtect.hpp"

// Кодек для перевода байтов в hex (для uuid и т.п.)
static std::string hexEncode(const std::vector<unsigned char>& v) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto c : v) {
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

// Обёртка base64 через Qt (для удобства записи в JSON)
static QString toBase64(const std::vector<unsigned char>& v) {
    QByteArray ba(reinterpret_cast<const char*>(v.data()),
                  static_cast<int>(v.size()));
    return QString::fromLatin1(ba.toBase64());
}

// Генерация простого hex-uuid (16 байт -> 32 hex-символа)
static std::string make_uuid_hex() {
    auto v = crypto::genRandomBytes(16);
    return hexEncode(v);
}

int main(int argc, char** argv) {
    // Для Qt-типа (QFile, QDir, QSqlDatabase) достаточно QCoreApplication
    QCoreApplication app(argc, argv);

    // Инициализация libsodium (общий крипто-рантайм)
    if (sodium_init() == -1) {
        std::cerr << "Ошибка: sodium_init() failed" << std::endl;
        return 1;
    }

    // Проверка аргументов командной строки
    if (argc < 4) {
        std::cerr << "Usage: create_submission <input_file> <student_id> <assignment_id> [original_name]\n";
        return 1;
    }

    // Разбор аргументов
    std::string inputFile   = argv[1];
    int         studentId   = std::atoi(argv[2]);
    int         assignmentId = std::atoi(argv[3]);

    // Определение оригинального имени файла
    std::string originalName;
    if (argc >= 5) {
        originalName = argv[4];
    } else {
        QFileInfo fi(QString::fromStdString(inputFile));
        originalName = fi.fileName().toStdString();
    }

    // Загрузка конфигурации (db + мастер-ключ)
    if (!ConfigManager::instance().load("config/config.json")) {
        std::cerr << "Ошибка: не удалось загрузить config/config.json\n";
        return 1;
    }

    // Открытие соединения с БД
    if (!Database::instance().open()) {
        std::cerr << "Ошибка: не удалось открыть соединение с БД\n";
        return 1;
    }

    // Создание директорий под файлы и метаданные (если их ещё нет)
    QDir().mkpath("storage/files");
    QDir().mkpath("storage/metadata");

    // Генерация uuid для файла (идентификатор хранения)
    std::string uuid     = make_uuid_hex();
    std::string filename = uuid + ".dat";
    std::string outPath  = std::string("storage/files/") + filename;

    // Генерация ключа и IV для шифрования файла (AES-256-CBC)
    std::vector<unsigned char> fileKey = crypto::genRandomBytes(32);
    std::vector<unsigned char> fileIv  = crypto::genRandomBytes(16);

    // Шифрование файла на диске
    std::string err;
    if (!crypto::aes256_cbc_encrypt(fileKey, fileIv, inputFile, outPath, err)) {
        std::cerr << "Ошибка: не удалось зашифровать файл: " << err << "\n";
        return 1;
    }

    // Загрузка мастер-ключа (для защиты ключа файла)
    auto master = ConfigManager::instance().masterKey();
    if (master.empty()) {
        std::cerr << "Ошибка: мастер-ключ пустой в config.json\n";
        return 1;
    }

    // Шифрование ключа файла под мастер-ключ (AES-GCM)
    std::vector<unsigned char> encKey, keyIv, keyTag;
    if (!keyprotect::encryptWithAesGcm(master, fileKey, encKey, keyIv, keyTag, err)) {
        std::cerr << "Ошибка: не удалось защитить ключ файла: " << err << "\n";
        return 1;
    }

    // Формирование JSON-метаданных по файлу
    QJsonObject meta;
    meta["owner_id"]      = studentId;  // владелец = студент
    meta["original_name"] = QString::fromStdString(originalName);
    meta["iv"]            = toBase64(fileIv);
    meta["key_encrypted"] = toBase64(encKey);
    meta["key_iv"]        = toBase64(keyIv);
    meta["key_tag"]       = toBase64(keyTag);

    QJsonDocument doc(meta);

    // Запись JSON-метаданных в файл
    QString metaPath = QString("storage/metadata/%1.json")
                           .arg(QString::fromStdString(uuid));
    QFile mf(metaPath);
    if (!mf.open(QIODevice::WriteOnly)) {
        std::cerr << "Ошибка: не удалось открыть файл метаданных для записи: "
                  << metaPath.toStdString() << "\n";
        return 1;
    }
    mf.write(doc.toJson(QJsonDocument::Indented));
    mf.close();

    // Вставка записи в таблицу submissions
    QSqlDatabase db = Database::instance().get();
    QSqlQuery q(db);
    q.prepare("INSERT INTO submissions "
              "(assignment_id, student_id, file_path, original_name, uploaded_at) "
              "VALUES (?, ?, ?, ?, NOW())");
    q.addBindValue(assignmentId);
    q.addBindValue(studentId);
    q.addBindValue(QString::fromStdString(filename));
    q.addBindValue(QString::fromStdString(originalName));

    if (!q.exec()) {
        std::cerr << "Ошибка: insert в submissions провалился: "
                  << q.lastError().text().toStdString() << "\n";
        return 1;
    }

    std::cout << "Submission создан. uuid=" << uuid
              << " file=" << outPath << "\n";
    return 0;
}
