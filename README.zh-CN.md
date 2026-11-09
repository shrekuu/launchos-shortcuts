# 用 F4 或五指捏合打开 LaunchOS

[English](README.md) · [AI 提示词](PROMPTS.md)

在 Mac 上**直接按 F4**，或者在 MacBook 内置触控板上**用拇指加另外四指向内捏合**，就像点击 Dock 里的 [LaunchOS](https://launchosapp.com/) 图标。两项可以单独安装。

项目使用 LaunchOS 的普通全局快捷键和独立的本地手势助手。已在 Apple 芯片 MacBook、**macOS 27.0.1**、**LaunchOS 2.3.0** 上用实体键盘和内置触控板验证。其他系统版本和外接触控板尚未实测。

先下载仓库，并在仓库目录里运行下面的命令：

```sh
git clone https://github.com/shrekuu/launchos-shortcuts.git
cd launchos-shortcuts
```

## 方案一：F4

1. 进入 LaunchOS → 设置 → 通用 → **快捷键**，录入标准 F4。若键帽上的 F4 默认触发系统搜索，可先用 **Fn + F4** 录入。此操作会替换原来的 LaunchOS 快捷键，先记下原值。若希望登录后立刻可用，请保留 LaunchOS 的登录启动。
2. 若直接按 F4 已能打开 LaunchOS，到这里就可以了。若仍打开 Spotlight 或系统的「应用程序」，运行 `hidutil list` 找到实际键盘。下面只是内置键盘示例。外接键盘请使用现场查到的厂商、产品和必要的位置 ID。选择条件应只匹配一个键盘服务，常见键盘的用法页为 `1`、用法为 `6`。

   ```sh
   MATCH='{"Product":"Apple Internal Keyboard / Trackpad","Transport":"SPI","PrimaryUsagePage":1,"PrimaryUsage":6}'
   /usr/bin/hidutil list --matching "$MATCH"
   ./scripts/install-f4-map.sh "$MATCH"
   /usr/bin/hidutil property --matching "$MATCH" --get UserKeyMapping
   ```

3. 关闭启动器，在 Finder 中直接按 F4 检查。脚本给选中的键盘添加登录映射，把 Spotlight 搜索、显示全部应用和旧款 Apple 启动台按键转成标准 F4。同一键盘上如果另有按键发送这些动作，也会被转换。检测到已有映射时，脚本会停止，避免覆盖它们。

**如果录入框收不到标准 F4：** 完全退出 LaunchOS，读取并记下原值（也可能没有该项），然后设置 F4 并重新打开。此备用方法已在 LaunchOS 2.3.0 验证。

```sh
defaults read app.remixdesign.LaunchOS KeyboardShortcuts_launchApp
defaults write app.remixdesign.LaunchOS KeyboardShortcuts_launchApp -string '{"carbonKeyCode":118,"carbonModifiers":0}'
open -a /Applications/LaunchOS.app
```

外接键盘拔插后可能需要重新应用映射。确认设备仍匹配，且没有新装其他映射工具后，可执行 `launchctl kickstart "gui/$(id -u)/io.github.launchos-shortcuts.f4"`。若映射为空，用 `launchctl print "gui/$(id -u)/io.github.launchos-shortcuts.f4"` 查运行结果。不同 macOS 版本和启动环境可能有不同的权限要求。 [Apple 的键盘映射说明](https://developer.apple.com/library/archive/technotes/tn2450/_index.html)

## 方案二：五指向内捏合

先把 LaunchOS 安装在 `/Applications`，并准备好 Xcode 命令行工具（需要时运行 `xcode-select --install`）。在已登录桌面的终端中运行：

```sh
./scripts/install-gesture.sh
launchctl print "gui/$(id -u)/io.github.launchos-shortcuts.pinch"
```

关闭启动器，把拇指加另外四指放在内置触控板上，再向内收。助手登录时启动，一次捏合只触发一次，调用的是和打开 LaunchOS 应用相同的系统入口。它使用 macOS 的私有 MultitouchSupport 接口，未来系统更新可能影响兼容性。日志只记录启动和打开应用的结果，不保存触点坐标。

### macOS 自带的应用列表也弹出怎么办

先到**系统设置 → 触控板 → 更多手势**关闭「应用程序」（Apps）。实测中，系统有时把五指捏合归到四指手势。另一个现象是「显示桌面」开启时，Apps 开关会回弹。先关闭「显示桌面」，再关闭 Apps，两个开关才都保持关闭。这也会关闭张开手指显示桌面的系统手势。改完后请用实体触控板验证。

如果开关仍无法保持，可参考[英文版里的偏好设置命令](README.md#if-macos-apps-opens-too)。先记录原值和哪些键原本不存在，再决定是否关闭内置触控板的系统四指和五指捏合或张开行为。修改后重新检查两个开关。

如果没有弹出 LaunchOS，先确认 Dock 图标能正常打开应用，然后查助手日志：

```sh
log show --last 5m --style compact \
  --predicate 'process == "launchos-pinch" AND eventMessage BEGINSWITH "LaunchOS gesture:"'
```

- 出现 `application opened` 表示系统接受了打开 LaunchOS 的请求。如果屏幕显示的是系统列表，继续查手势冲突。
- 只有 `built-in trackpad ready` 时，先让五根手指都接触触控板，再向内收。
- 如果休眠唤醒后才失效，可用 `launchctl kickstart -k "gui/$(id -u)/io.github.launchos-shortcuts.pinch"` 重启助手。

## 恢复原设置

- **F4：** 运行 `launchctl bootout "gui/$(id -u)/io.github.launchos-shortcuts.f4"`，然后将对应的 plist 移出 `~/Library/LaunchAgents`。如果之后没有添加其他映射，使用相同的 `MATCH` 执行 `hidutil property --matching "$MATCH" --set '{"UserKeyMapping":[]}'` 清除映射。回到 LaunchOS 设置恢复原快捷键。
- **五指捏合：** 运行 `launchctl bootout "gui/$(id -u)/io.github.launchos-shortcuts.pinch"`，将对应的 plist 移出 `~/Library/LaunchAgents`。若要移除助手或重新安装，再将助手目录 `~/Library/Application Support/LaunchOS Shortcuts` 移走。恢复「应用程序」和「显示桌面」开关。如果用命令改过系统偏好，按原域和原值恢复，原来不存在的键用 `defaults delete` 移除。

项目使用 MIT 许可证，与 LaunchOS 和 Apple 无隶属关系。
