# LaunchOS Shortcuts for macOS

[简体中文](README.zh-CN.md) · [Prompts for AI assistants](PROMPTS.md)

Open [LaunchOS](https://launchosapp.com/) the same way you would by clicking its Dock icon, with either **F4** or a **five-finger inward pinch** on the MacBook's built-in trackpad. You can set up either shortcut on its own.

These are independent helpers. The F4 setup uses LaunchOS's regular global shortcut. The pinch helper opens the installed app through macOS. LaunchOS also offers its own [gesture and F4 features](https://launchosapp.com/features/).

The two shortcuts were tested on an Apple silicon MacBook running **macOS 27.0.1** with **LaunchOS 2.3.0**. Physical F4 and a physical five-finger pinch both opened the LaunchOS grid. Other macOS versions and external trackpads were not tested.

Clone this repository and run the commands from its directory:

```sh
git clone https://github.com/shrekuu/launchos-shortcuts.git
cd launchos-shortcuts
```

## Option A: press F4

1. Open LaunchOS → Settings → General → **Hotkey**. Record **F4** as the regular hotkey, using **Fn + F4** to send a standard F4 if your keyboard normally sends a media action. This replaces your previous LaunchOS hotkey. Note the original value, and leave **Start at Login** enabled if you want the shortcut immediately after logging in.
2. If plain F4 now opens LaunchOS, you are done. If it opens Spotlight or the system Apps view, identify the keyboard service with `hidutil list`. Build a selector for **exactly one keyboard** and confirm it with `hidutil list --matching "$MATCH"`. The example below is for a built-in keyboard with the listed product name and transport. For an external keyboard, use its actual vendor/product and, if needed, location IDs from the local listing. A keyboard service normally has usage page `1` and usage `6`.

   ```sh
   MATCH='{"Product":"Apple Internal Keyboard / Trackpad","Transport":"SPI","PrimaryUsagePage":1,"PrimaryUsage":6}'
   /usr/bin/hidutil list --matching "$MATCH"
   ./scripts/install-f4-map.sh "$MATCH"
   /usr/bin/hidutil property --matching "$MATCH" --get UserKeyMapping
   ```

3. Close the launcher, then press **plain F4** in Finder. The grid should open. The login agent maps Spotlight Search, the standard Show All Applications key, and Apple's legacy Launchpad key to ordinary F4 **on the selected service**. If the keyboard has a separate key sending one of those actions, that key will also send F4. If an existing mapping is present, the installer stops so you can review and merge rules yourself.

**If LaunchOS's hotkey recorder won't capture ordinary F4:** With LaunchOS quit, read and save the previous value (it may be absent), then run the following. Reopen LaunchOS and check that General → Hotkey displays **F4**. This fallback uses a LaunchOS preference verified on version 2.3.0.

```sh
defaults read app.remixdesign.LaunchOS KeyboardShortcuts_launchApp
defaults write app.remixdesign.LaunchOS KeyboardShortcuts_launchApp -string '{"carbonKeyCode":118,"carbonModifiers":0}'
open -a /Applications/LaunchOS.app
```

`hidutil` mappings may disappear when a keyboard disconnects. After reconnecting the same keyboard, you can reapply the installed rule with `launchctl kickstart "gui/$(id -u)/io.github.launchos-shortcuts.f4"` if the service still matches and you have not installed another remapper. If the mapping stays empty, check `launchctl print "gui/$(id -u)/io.github.launchos-shortcuts.f4"`. Permission requirements can vary with macOS and launch context. The tested setup did not require a new permission grant. [Apple's key remapping reference](https://developer.apple.com/library/archive/technotes/tn2450/_index.html)

## Option B: five-finger inward pinch

Install LaunchOS in `/Applications` and have the Xcode Command Line Tools available (`xcode-select --install` if needed). Run from a terminal in your logged-in desktop session:

```sh
./scripts/install-gesture.sh
launchctl print "gui/$(id -u)/io.github.launchos-shortcuts.pinch"
```

With the launcher closed, place your thumb and four fingers on the **built-in trackpad**, then pull them inward. The helper starts at login, triggers once per pinch, and runs the equivalent of `open -a /Applications/LaunchOS.app`. It uses macOS's private MultitouchSupport framework, which can change in future releases. It has no network access code and records only startup and launch outcomes, not touch coordinates.

### If macOS Apps opens too

First open **System Settings → Trackpad → More Gestures** and turn off **Apps**. The tested Mac also treated some five-finger pinches as its system four-finger gesture. Its Apps switch would turn back on while **Show Desktop** was enabled. Turning off **Show Desktop** and then **Apps** made both switches stay off. This also removes the built-in spread-to-show-desktop gesture. Test a physical pinch again before making any further changes.

If the switches will not stick, inspect the settings below. Before changing anything, record the values and note which keys were absent. The following preferences were used on the tested Mac alongside the Settings switches. Only apply them if you are willing to turn off the built-in trackpad's system four- and five-finger pinch/spread behavior.

```sh
for fingers in Four Five; do
  defaults read com.apple.AppleMultitouchTrackpad "Trackpad${fingers}FingerPinchGesture"
done
for fingers in four five; do
  defaults -currentHost read -g "com.apple.trackpad.${fingers}FingerPinchSwipeGesture"
done

# Set both counts only if the system gesture still conflicts.
for fingers in Four Five; do
  defaults write com.apple.AppleMultitouchTrackpad "Trackpad${fingers}FingerPinchGesture" -int 0
done
for fingers in four five; do
  defaults -currentHost write -g "com.apple.trackpad.${fingers}FingerPinchSwipeGesture" -int 0
done
killall Dock
```

Dock briefly disappears and restarts. Check the two switches again. If neither grid appears, try `open -a /Applications/LaunchOS.app`, then check whether the helper recognized the physical pinch:

```sh
log show --last 5m --style compact \
  --predicate 'process == "launchos-pinch" AND eventMessage BEGINSWITH "LaunchOS gesture:"'
```

A recent `application opened` entry means the system accepted the open request. If the wrong grid is still on screen, investigate competing system gestures. If you only see `built-in trackpad ready`, make sure all five fingers are on the surface before pulling inward. If the helper stopped receiving touches after sleep, try `launchctl kickstart -k "gui/$(id -u)/io.github.launchos-shortcuts.pinch"`.

## Undo

- **F4:** Run `launchctl bootout "gui/$(id -u)/io.github.launchos-shortcuts.f4"`, then move its plist out of `~/Library/LaunchAgents`. If you have not added other HID rules since installing, clear the selected keyboard's mapping with `hidutil property --matching "$MATCH" --set '{"UserKeyMapping":[]}'` using the same selector. Restore your previous LaunchOS hotkey in its settings.
- **Pinch:** Run `launchctl bootout "gui/$(id -u)/io.github.launchos-shortcuts.pinch"`, then move its plist out of `~/Library/LaunchAgents`. Move the helper directory, `~/Library/Application Support/LaunchOS Shortcuts`, out of Application Support if you want to remove it or reinstall. Restore your previous **Apps** and **Show Desktop** settings. If you used the manual preference commands, restore the recorded values with `defaults write` or remove keys that were originally absent with `defaults delete`, using the original domain and `-currentHost` where applicable.

## Develop

```sh
mkdir -p build
xcrun clang -DTEST_PINCH -Wall -Wextra -Werror tests/pinch.c -framework CoreFoundation -o build/check-pinch
./build/check-pinch
```

The test uses synthetic frames to cover finger count, inward motion, and repeated triggers. Use a physical keyboard and trackpad to validate the full behavior. The [MultitouchSupport interface reference](https://github.com/calftrail/TrackMagic/blob/master/MultitouchSupport.h) documents the callback layout used by the helper.

MIT licensed. Independent of LaunchOS and Apple.
