# Unreal Finder Tool
Useful tool to help you fetch and dump Unreal Engine 4 Games information.

# Features
- **Nice UI**.
- **Find GNames**.
- **Find GObjects**.
- **Instance Logger**.
- **Sdk Generator**. *Based on @KN4CK3R* (**External**, **light**, **Faster** and **Multi-thread**)
- **Kenrnal to read process memory**. [Based on @harakirinox](https://www.unknowncheats.me/forum/anti-cheat-bypass/312791-bypaph-process-hackers-bypass-read-write-process-virtual-memory-kernel-mem.html)
- **Dynamic JSON Reflector structs**. (**Read struct from JSON files to fit all games structs. *(just edit and run)***)

# Compatibility
- Windows 64bit, (32/64bit) games.
- Windows 32bit, (32bit) games Only.
- All UE4 Games (32/64bit).
- All UE4 Versions.

# Download
Download last version from [Here](https://github.com/CorrM/Unreal-Finder-Tool/releases/latest).

# ScreenShots
![ui_1](/images/ui_1.png)
![ui_2](/images/ui_2.png)
![ui_3](/images/ui_3.png)

# Tested games
- PubgMobile (On TGB Emulator). (x32)
- Snake Pass. (x64)
- What Remains of Edith Finch. (x64)
- Unreal Minecraft (UnrealVoxel). (x64)
- Dauntless. (x64)
- Some Indie games. (64bit / 32bit)

# Change Log
##### 30-06-2019
- Add new settings.
- Add `Game Name`, `Game Version` to `Sdk Generator`.
- Add `Sdk Type` to `Sdk Generator`.
  - **Internal**: Usually used when your target is inject dll into game process.
    - it's generate functions and funtction params. without `ReadAsMe` function.
    - you can directly cast/convert block of memory to your class/struct.
  - **External**: Usually used when your target is wite or read game process memory from your process.
    - For now tool support read as object (class/struct) with `ReadAsMe` function in every class/struct.
    - it's useful to read block of memory as class/struct.
    - no functions generaterd for external for now. maybe later i will support call function from external.
- Some Optimization.

##### 28-06-2019
- Add UI.
- Some changes to `SDK Generator` and `Instance Logger`.
- Let GObject address getted form `GObject Finder` be valid to use directly.

##### 25-06-2019
- Add `SDK Generator`.
- Add Settings file.
- Improve `Instance Logger`.

##### 09-06-2019
- Add `JSON reflctor`.
- Convert `Instance Logger` to `JSON reflector`.

##### 06-06-2019
- Add `Instance Logger`.

##### 02-06-2019
- First version released
