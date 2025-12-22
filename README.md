Содержание README

Описание проекта

Основные роли и функции

Структура проекта (файловая система)

Структура баз данных (основные таблицы)

Хранение файлов и метаданных

Конфигурация (пример config/config.json)

Скрипты и утилиты (инициализация, сборка, создание admin)

Зависимости и подготовка окружения

Безопасность

Как разворачивать (коротко)

Контакты и лицензия

1. Описание проекта

Система предназначена для автоматизации процесса публикации и проверки курсовых работ и практик в учебном процессе.
Реализованы: регистрация/авторизация, разграничение прав по ролям, публикация заданий, отправка/хранение/расшифровка файлов, выставление оценок, ведение журнала действий и экспорт отчётов.

Технологии: C++17/Qt5 (или Qt6), PostgreSQL, OpenSSL, CMake.

2. Основные роли и функции
Роли

Преподаватель

публикует задания (курсовые/практики)

просматривает работы студентов

комментирует и выставляет оценки

ведёт журнал оценок

Студент

просматривает задания по дисциплинам

загружает выполненные работы (файлы шифруются при загрузке)

получает комментарии/оценки, видит историю отправок

Администратор

управляет пользователями (создание/блокировка/назначение ролей)

настраивает дисциплины и группы

просматривает логи, выполняет экспорт/резервирование БД

Основные функции

Авторизация / регистрация (desktop-окно)

валидация сложности пароля; хеширование паролей PBKDF2-HMAC-SHA256

Личный кабинет: задания, мои отправки, журнал оценок

Управление заданиями (создание/правка/удаление)

Загрузка/просмотр/оценивание отправлений

Админ-панель: управление пользователями, дисциплинами, правами

Защита данных: PBKDF2, AES-256-CBC для файлов; ключи файлов шифруются AES-GCM мастерк-ключом

Журнал аудита и логирование (файл + таблица audit_log)

Экспорт/отчёты: CSV/JSON

3. Структура проекта (файловая система)
```
Course_work/
├─ CMakeLists.txt
├─ README.md
├─ config/
│  ├─ config.json.template
│  └─ config.json            # создаётся scripts/init_db.sh
├─ sql/
│  └─ 001_schema.sql         # схема БД для PostgreSQL
├─ scripts/
│  ├─ init_db.sh
│  ├─ build.sh
│  └─ package_release.sh
├─ src/
│  ├─ main.cpp
│  ├─ C++ модули:
│  │  ├─ db/Database.hpp/.cpp
│  │  ├─ config/ConfigManager.hpp/.cpp
│  │  ├─ auth/PasswordUtils.hpp/.cpp
│  │  ├─ auth/AuthManager.hpp/.cpp
│  │  ├─ crypto/FileCrypto.hpp/.cpp
│  │  ├─ crypto/KeyProtect.hpp/.cpp
│  │  ├─ tools/create_admin.cpp
│  │  ├─ utils/Logger.hpp/.cpp
│  │  └─ gui/ (LoginWindow, MainWindow, TeacherWindow и пр.)
├─ storage/
│  ├─ files/                  # зашифрованные файлы: <uuid>.dat
│  └─ metadata/               # метаданные: <uuid>.json (iv, encrypted_key, owner, original_name)
├─ logs/
│  └─ actions.log
└─ data_pg/                   # (опционально) каталог PostgreSQL вместе                        # с docker-compose
```

4. Структура баз данных (основные таблицы)

users
```
id SERIAL PRIMARY KEY,
login TEXT UNIQUE NOT NULL,
full_name TEXT,
email TEXT,
role TEXT NOT NULL, -- student, teacher, admin
password_hash TEXT NOT NULL,
salt TEXT NOT NULL,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
active BOOLEAN DEFAULT TRUE
```

disciplines
```
id SERIAL PRIMARY KEY,
name TEXT NOT NULL,
code TEXT UNIQUE
```

assignments
```
id SERIAL PRIMARY KEY,
title TEXT NOT NULL,
description TEXT,
discipline_id INTEGER REFERENCES disciplines(id),
created_by INTEGER REFERENCES users(id),
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
due_date TIMESTAMP
```

submissions
```
id SERIAL PRIMARY KEY,
assignment_id INTEGER REFERENCES assignments(id),
student_id INTEGER REFERENCES users(id),
file_path TEXT NOT NULL,      -- e.g. uuid.dat
original_name TEXT NOT NULL,
uploaded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
grade TEXT,
feedback TEXT
```

audit_log
```
id SERIAL PRIMARY KEY,
user_id INTEGER,
action TEXT,
details TEXT,
ts TIMESTAMP DEFAULT CURRENT_TIMESTAMP
```

Полный SQL в: sql/001_schema.sql.

5. Хранение файлов и метаданных

Все загруженные файлы шифруются и хранятся в storage/files/<uuid>.dat.

Для каждого файла создаётся метаданный JSON в storage/metadata/<uuid>.json:
```
{
  "owner_id": 123,
  "original_name": "report.pdf",
  "iv": "<base64>",                // IV для AES-256-CBC
  "key_encrypted": "<base64>",     // AES-GCM зашифрованный file_key
  "key_iv": "<base64>",            // iv для AES-GCM
  "key_tag": "<base64>"           // GCM tag
}
```

Мастер-ключ (master_key_hex) хранится в config/config.json (права 600). Мастер-ключ используется только для защиты file_key (AES-GCM).

6. Конфигурация (пример config/config.json)

config/config.json.template
```
{
  "master_key_hex": "YOUR_MASTER_KEY_HEX_GOES_HERE",
  "pbkdf2_iterations": 100000,
  "db": {
    "host": "localhost",
    "port": 5432,
    "name": "jumandgi",
    "user": "jumandgi_user",
    "password": "changeme"
  }
}
```

config/config.json создаётся скриптом scripts/init_db.sh автоматически (мастер-ключ генерируется).

7. Скрипты и утилиты

scripts/init_db.sh — инициализация: создаёт config/config.json, пытается создать роль/базу в PostgreSQL (через sudo -u postgres) и загружает схему sql/001_schema.sql. Скрипт не использует Python.

scripts/build.sh — сборка проекта (CMake, make).

scripts/package_release.sh — упаковка проекта в tar.gz.

src/tools/create_admin.cpp — CLI-утилита (C++) для создания администратора: корректно хеширует пароль (PBKDF2) и создаёт запись в БД (используется после сборки).
После сборки в build/ запускаете ./create_admin и вводите логин/пароль.

GUI бинарь: build/Jumandgi.

8. Зависимости и подготовка окружения

Минимальные зависимости (Ubuntu/Debian):
```
sudo apt update
sudo apt install -y build-essential cmake libssl-dev libpq-dev postgresql postgresql-contrib \
    qtbase5-dev qttools5-dev qttools5-dev-tools libqt5sql5-psql
```

Пакеты для Qt6 аналогичны, но названия могут отличаться. Убедитесь, что Qt установлен с плагином QPSQL.

9. Безопасность

Пароли: PBKDF2-HMAC-SHA256, соль 16 байт, итерации — 100000 (настраивается в config.json).

Файлы: AES-256-CBC для содержимого, ключ файла (file_key) — AES-256-GCM, зашифрован мастерк-ключом.

Ключи: мастер-ключ хранится в конфиге с правами 600 только для учебной демонстрации. На продакшене следует использовать KMS/HSM.

SQL: все запросы выполняются параметризованно (QSqlQuery::prepare() + addBindValue) — защита от SQL-инъекций.

Пути: проверка путей и фильтрация расширений (белый список).

Аудит: все критичные действия логируются в logs/actions.log и таблицу audit_log.

Регистрация/ввод: валидация всех пользовательских форм.

10. Как разворачивать (коротко)

Скопируйте проект в Course_work/.

Настройте PostgreSQL (можно локально или в контейнере).

Запустите scripts/init_db.sh и следуйте подсказкам (скрипт создаёт config/config.json и загружает схему в БД).

Соберите проект:
```
./scripts/build.sh
```

Создайте администратора (в каталоге build/):
```
./create_admin
```

Запустите приложение:
```
./build/Jumandgi
```

Полные инструкции и дополнительные опции (docker-compose, backup/restore) находятся в разделе docs/ и в scripts/ проекта.

11. Контакты / дальнейшая работа

Автор / контакт: Никита Терещенко

При необходимости могу:

добавить подробную инструкцию развёртывания с docker-compose (Postgres + приложение),

подготовить пакет для Windows,

расширить интерфейс администратора или студента,

подготовить пояснительную записку (ГОСТ).