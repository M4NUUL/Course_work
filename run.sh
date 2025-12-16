set -euo pipefail
XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
env -i HOME="$HOME" USER="$USER" PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
DISPLAY="${DISPLAY:-}" XAUTHORITY="${XAUTHORITY:-}" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
./build/CourseWorkApp
