#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONFIG_DIR="$ROOT/config"
CONFIG_FILE="$CONFIG_DIR/config.json"
SQL_SCHEMA="$ROOT/sql/001_schema.sql"

mkdir -p "$CONFIG_DIR"

echo "=== Инициализация проекта Jumandgi (PostgreSQL) ==="
read -p "Host (default localhost): " DB_HOST
DB_HOST=${DB_HOST:-localhost}
read -p "Port (default 5432): " DB_PORT
DB_PORT=${DB_PORT:-5432}
read -p "Database name (default jumandgi): " DB_NAME
DB_NAME=${DB_NAME:-jumandgi}
read -p "Database user (default jumandgi_user): " DB_USER
DB_USER=${DB_USER:-jumandgi_user}
read -s -p "Database user password (will be stored in config): " DB_PASS
echo
# generate master key hex
MASTER_HEX=$(openssl rand -hex 32)

cat > "$CONFIG_FILE" <<JSON
{
  "master_key_hex": "$MASTER_HEX",
  "pbkdf2_iterations": 100000,
  "db": {
    "host": "$DB_HOST",
    "port": $DB_PORT,
    "name": "$DB_NAME",
    "user": "$DB_USER",
    "password": "$DB_PASS"
  }
}
JSON

chmod 600 "$CONFIG_FILE"
echo "Created config at $CONFIG_FILE (permissions 600)."

echo
echo "Попытка создать роль и базу данных через 'sudo -u postgres'."
if sudo -n true 2>/dev/null; then
    echo "Используем sudo -u postgres для создания роли и базы..."
    sudo -u postgres psql -v ON_ERROR_STOP=1 <<PSQL
DO
\$do\$
BEGIN
   IF NOT EXISTS (SELECT FROM pg_catalog.pg_roles WHERE rolname = '$DB_USER') THEN
      CREATE ROLE $DB_USER WITH LOGIN PASSWORD '$DB_PASS';
   END IF;
END
\$do\$;

-- create DB if not exists
SELECT 'CREATE DATABASE \"$DB_NAME\" OWNER \"$DB_USER\"' WHERE NOT EXISTS (SELECT FROM pg_database WHERE datname = '$DB_NAME')\gexec
PSQL
    echo "Роль и/или база созданы (или уже существуют)."
else
    echo "Не удалось выполнить sudo без пароля. Пожалуйста создайте роль и базу вручную."
    echo "Команды для запуска под пользователем с правами postgres:"
    echo "sudo -u postgres psql -c \"CREATE ROLE $DB_USER WITH LOGIN PASSWORD '$DB_PASS';\""
    echo "sudo -u postgres psql -c \"CREATE DATABASE $DB_NAME OWNER $DB_USER;\""
    echo "После создания нажмите Enter чтобы продолжить..."
    read
fi

# Load schema using psql as created user
echo "Загружаем схему $SQL_SCHEMA в БД $DB_NAME..."
PGPASSWORD="$DB_PASS" psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -f "$SQL_SCHEMA"

echo "Схема загружена."

echo
echo "Инициализация завершена."
echo "Дальше: соберите проект (./scripts/build.sh), затем создайте администратора:"
echo "  cd build && ./create_admin # или ./create_admin в корне проекта"
