#!/usr/bin/env bash
set -euo pipefail

DB_CONTAINER="edudesk-db"
DB_USER="edudesk"
DB_NAME="edudesk"

docker exec -i "$DB_CONTAINER" psql -U "$DB_USER" -d "$DB_NAME" < sql/002_sp.sql
docker exec -i "$DB_CONTAINER" psql -U "$DB_USER" -d "$DB_NAME" < sql/003_fix_sp.sql
