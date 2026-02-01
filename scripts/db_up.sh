#!/usr/bin/env bash
set -euo pipefail

docker compose up -d db

for i in {1..30}; do
  if docker exec edudesk-db pg_isready -U edudesk -d edudesk >/dev/null 2>&1; then
    exit 0
  fi
  sleep 1
done

echo "PostgreSQL не готов" >&2
exit 1