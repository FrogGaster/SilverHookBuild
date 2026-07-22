# ZA_PUTINA
Internal HOI4 Cheat for Windows
Uses <a href="https://github.com/Rebzzel/kiero">KieroHook</a>, <a href="https://github.com/TsudaKageyu/minhook">MinHook</a> and <a href="https://github.com/ocornut/imgui">ImGui</a>

# Features
### Lobby Cheats
- Display Host IP
- Spoof Steam Name
- Ghosting yourself
- Antiban
- Hotjoin Bypass
- Checksum Bypass
- Lobby Rules Hack
- Start Game
- Instant Lobby Kill

### In-game Cheats
- Free Templates
- Cheap Equipment Upgrades (Mechanized, no nsb tanks etc)
- See all combot
- Allowtraits, FOW, debug
- Crasher
- Enable and Disable AI on all or specific Country (must use ID, not tag. Ger = 1, Eng = 2 etc)
- Infinite Pause
- Ghost Pause
- Increase or Decrease speed as player in MP
- Tagswitch

### Misc Cheats
- Country strength menu usable during a running game, with an exact range from 0% to 1,000,000%

### Debug Options
- Only used for Devs

# Building

Requirements:

- Windows 10 or 11
- Visual Studio 2022 or Build Tools 2022 with **Desktop development with C++**
- MSVC v143 and a Windows 10/11 SDK

The legacy DirectX SDK (June 2010) is not required. Direct3D 11 headers and libraries come from the Windows SDK.

Build the 64-bit release DLL from PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Outputs:

- `x64\Release\ZA_PUTINA.dll`
- `x64\Release\ZA_PUTINA_Loader.exe` (contains the DLL and is the only file needed for distribution)

## Release protection

Release builds use link-time optimization, function/data folding, Control Flow Guard,
and compile-time encryption for game signatures and user-facing/diagnostic strings.
The build script also removes generated PDB and ILK files from `x64\Release`, so the
distributed binaries do not contain local source paths or public debug symbols.

This raises the cost of static analysis, but no native client-side binary can be made
impossible to disassemble or reverse engineer. Do not treat obfuscation as a security
boundary or store permanent secrets in the client.

Before loading the DLL after a HOI4 update, verify that all byte signatures still match the installed executable:

```powershell
python .\tools\check_signatures.py
```

You can also pass a custom game path:

```powershell
python .\tools\check_signatures.py "D:\SteamLibrary\steamapps\common\Hearts of Iron IV\hoi4.exe"
```

The checker exits with a non-zero status when a signature is missing. Do not load the DLL into an unsupported game build.

# Running

1. Run `python .\tools\check_signatures.py` and make sure every signature is reported as `[OK]`.
2. Start the 64-bit HOI4 executable using the DirectX 11 renderer and wait for the main menu.
3. Run `x64\Release\ZA_PUTINA_Loader.exe`; the required DLL is embedded in it. Run it as administrator only if Windows denies access to the game process.
4. Press `Insert` to show or hide the menu.

`ZA_PUTINA_Loader` is intentionally limited to `hoi4.exe` and an x64 DLL named `ZA_PUTINA.dll`. The embedded DLL is extracted to a unique temporary directory immediately before loading. Its complete source is in `SilverLoader\main.cpp`; no third-party injector binary is used. Passing an explicit DLL path as the first command-line argument remains available for development builds.

Runtime logs are written to `%APPDATA%\ZA_PUTINA`.

## Telegram launch notifications

The loader can send a Telegram message on every start. It sends only the text
`ZA_PUTINA Loader started. No personal data was collected.`; it does not collect
the Windows user name, computer name, IP address, or game data. Notification
failure never prevents the loader from running.

Create a bot with BotFather, send that bot a message, and set these user-level
environment variables in PowerShell:

```powershell
[Environment]::SetEnvironmentVariable("ZA_PUTINA_TELEGRAM_BOT_TOKEN", "123456:replace-with-token", "User")
[Environment]::SetEnvironmentVariable("ZA_PUTINA_TELEGRAM_CHAT_ID", "replace-with-chat-id", "User")
```

Open a new terminal after setting the variables. If neither variable is set,
notifications are disabled. If only one is set or Telegram cannot be reached,
the loader prints a warning and continues normally.

Do not put the bot token in source code, the repository, or a distributed build.
Environment variables configure notifications only on that Windows account. If
you distribute the loader and need central launch analytics, use a consent-based
HTTPS service that keeps the Telegram token on your server; a secret embedded in
a client executable can always be extracted and abused.

# Kiero

The DirectX 11 hook is based on [kiero](https://github.com/Rebzzel/kiero).
