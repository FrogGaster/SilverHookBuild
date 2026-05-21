# SilverHook
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
- Boosting (Now you can pick countries with a menu instead of a weird bypass)

### Debug Options
- Only used for Devs

# I won't be updating this.


# ImGui-DirectX-11-Kiero-Hook
Universal ImGui implementation through DirectX 11 Hook (kiero)
<h1>Setting up the solution</h1>
<ul>
  <li>Download and install <a href="https://www.microsoft.com/en-us/download/details.aspx?id=6812">DirectX SDK</a></li>
  <li>Open the solution on Visual Studio and open the project Properties</li>
  <li>Go to VC++ Directories -> Include Directories. Click on 'Edit' and select the Include folder <br/>located on your DirectX SDK installation path. It is generally this one: <br/>%programfiles(x86)%\Microsoft DirectX SDK (June 2010)\Include\
  <li>Now go to VC++ Directories -> Library Directories. Click on 'Edit' and select the library folder <br/> located on your DirectX SDK installation path. It is generally this one - choose x86 for 32bit and x64 for 64bit: <br/>%programfiles(x86)%\Microsoft DirectX SDK (June 2010)\Lib\</li>
  <li><b>Done!</b></li>
</ul>
<h2>Kiero</h2>
<p>You can find Kiero's official repository <a href="https://github.com/Rebzzel/kiero">here</a>
