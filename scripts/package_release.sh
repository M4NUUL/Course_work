#!/usr/bin/env bash
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/Course_work_release.tar.gz"
tar --exclude='./build' -czf "$OUT" -C "$ROOT" .
echo "Archive created: $OUT"
