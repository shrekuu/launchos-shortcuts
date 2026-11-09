#!/bin/bash
set -euo pipefail

repo=$(cd "$(dirname "$0")/.." && pwd)
label=io.github.launchos-shortcuts.pinch
app="/Applications/LaunchOS.app"
support="$HOME/Library/Application Support/LaunchOS Shortcuts"
agent="$HOME/Library/LaunchAgents/$label.plist"
domain="gui/$(id -u)"

if [[ ! -d "$app" ]]; then
    echo "Install LaunchOS at $app first." >&2
    exit 1
fi
if [[ -e "$agent" || -L "$agent" || -e "$support" || -L "$support" ]] ||
    launchctl print "$domain/$label" >/dev/null 2>&1; then
    echo "A gesture installation already exists. See the removal steps in the README." >&2
    exit 1
fi
launchctl print "$domain" >/dev/null

umask 077
mkdir -p "$HOME/Library/LaunchAgents" "$HOME/Library/Application Support"
mkdir "$support"
staged="$support/gesture.plist"
cleanup() {
    if [[ -e "$staged" ]]; then unlink "$staged"; fi
    if [[ -e "$support/launchos-pinch" ]]; then unlink "$support/launchos-pinch"; fi
    rmdir "$support"
}
trap cleanup EXIT

/usr/bin/xcrun clang -O2 -Wall -Wextra -Werror "$repo/src/pinch.c" \
    -framework CoreFoundation -o "$support/launchos-pinch"
/usr/bin/plutil -create xml1 "$staged"
/usr/bin/plutil -insert Label -string "$label" "$staged"
/usr/bin/plutil -insert RunAtLoad -bool true "$staged"
/usr/bin/plutil -insert LimitLoadToSessionType -string Aqua "$staged"
/usr/bin/plutil -insert ProgramArguments -json '[]' "$staged"
/usr/bin/plutil -insert ProgramArguments.0 -string "$support/launchos-pinch" "$staged"
/usr/bin/plutil -lint "$staged"

# A hard link fails if another file already occupies the destination.
ln "$staged" "$agent"
if ! launchctl bootstrap "$domain" "$agent"; then
    unlink "$agent"
    exit 1
fi
unlink "$staged"
trap - EXIT
echo "Installed $label. See the README for system gesture conflicts and removal."
