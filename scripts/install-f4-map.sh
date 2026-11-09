#!/bin/bash
set -euo pipefail

label=io.github.launchos-shortcuts.f4
match="${1:-}"
if [[ -z "$match" || "$match" != \{*\} ]]; then
    echo 'Pass a single hidutil keyboard selector as a JSON object. See the README.' >&2
    exit 1
fi
agent="$HOME/Library/LaunchAgents/$label.plist"
domain="gui/$(id -u)"
if [[ -e "$agent" || -L "$agent" ]] ||
    launchctl print "$domain/$label" >/dev/null 2>&1; then
    echo "An F4 installation already exists. See the removal steps in the README." >&2
    exit 1
fi
launchctl print "$domain" >/dev/null
current=$(/usr/bin/hidutil property --matching "$match" --get UserKeyMapping)
# Refuse zero devices, multiple devices, and any existing mappings.
if ! awk 'NR == 2 { ok = $2 == "UserKeyMapping" && $3 == "(null)" } END { exit !(NR == 2 && ok) }' <<< "$current"; then
    echo "Expected exactly one keyboard with no existing UserKeyMapping. No mapping was changed." >&2
    echo "$current" >&2
    exit 1
fi

# Sources: AC Search, AC Desktop Show All Applications, and legacy Apple Launchpad.
mapping='{"UserKeyMapping":[{"HIDKeyboardModifierMappingSrc":51539608097,"HIDKeyboardModifierMappingDst":30064771133},{"HIDKeyboardModifierMappingSrc":51539608226,"HIDKeyboardModifierMappingDst":30064771133},{"HIDKeyboardModifierMappingSrc":280379760050180,"HIDKeyboardModifierMappingDst":30064771133}]}'
umask 077
mkdir -p "$HOME/Library/LaunchAgents"
staged=$(mktemp "$HOME/Library/LaunchAgents/.launchos-f4.XXXXXXXX")
trap 'unlink "$staged"' EXIT
/usr/bin/plutil -create xml1 "$staged"
/usr/bin/plutil -insert Label -string "$label" "$staged"
/usr/bin/plutil -insert RunAtLoad -bool true "$staged"
/usr/bin/plutil -insert LimitLoadToSessionType -string Aqua "$staged"
/usr/bin/plutil -insert ProgramArguments -json '[]' "$staged"
/usr/bin/plutil -insert ProgramArguments.0 -string /usr/bin/hidutil "$staged"
/usr/bin/plutil -insert ProgramArguments.1 -string property "$staged"
/usr/bin/plutil -insert ProgramArguments.2 -string --matching "$staged"
/usr/bin/plutil -insert ProgramArguments.3 -string "$match" "$staged"
/usr/bin/plutil -insert ProgramArguments.4 -string --set "$staged"
/usr/bin/plutil -insert ProgramArguments.5 -string "$mapping" "$staged"
/usr/bin/plutil -lint "$staged"
ln "$staged" "$agent"
if ! launchctl bootstrap "$domain" "$agent"; then
    unlink "$agent"
    exit 1
fi
echo "Loaded $label. Check the mapping with:"
printf '/usr/bin/hidutil property --matching %q --get UserKeyMapping\n' "$match"
