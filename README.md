# Unreal Finder Tool
Useful tool to help you fetch and dump Unreal Engine 4 Games information.

# Features
- **Nice and Easy UI**.
- **Find GNames**.
- **Find GObjects**.
- **Instance Logger**.
- **Sdk Generator**. *Based on @KN4CK3R* (**External**, **More effective**, **Faster** and **Multi-thread**)
- **Kenrnal to read process memory**. [Based on @harakirinox](https://www.unknowncheats.me/forum/anti-cheat-bypass/312791-bypaph-process-hackers-bypass-read-write-process-virtual-memory-kernel-mem.html)

# Compatibility
- Windows 64bit, (32/64bit) games.
- Windows 32bit, (32bit) games Only.
- All UE4 Games, (32/64bit).

# How to use
This video tell you how to use the tool and dump sdk for ue4 game. [Youtube Video](https://www.youtube.com/watch?v=3CjsrnvKtGs)

# Download
Download last version from [Here](https://github.com/CorrM/Unreal-Finder-Tool/releases/latest).

# ScreenShots
![ui_1](/images/ui_1.png)
![ui_2](/images/ui_2.png)
![ui_3](/images/ui_3.png)
![ui_3](/images/cfg_1.png)

# Change Log
##### 04-07-2019
- Add Settings button.
- Start using [Font Awesome](https://fontawesome.com/)

##### 03-07-2019
- Improve `Sdk Generator`.
  - `Sdk Generator` now significantly faster
  - Some organization for `Sdk Generator` code.
- Improve `Class Finder`.
  - Now can search for `class name` of address.
    - Put `instance address` and the tool will give you instance `class name`
  - When you search for `class name` you will get also instance thats `derived` from `class name`.
    - it's useful to find your target fast, you will get the name of `derived` next to instance address.
    - In example you search for `PlayerBase` you will get `PlayerBase`, `PlayerExtarBase` and `GamePlayer`.
  - Now significantly faster.

##### 01-07-2019
- Support `GObjects Chunks`.
  - Some games use `GObject's chunks` as same as `GNames`, so i just support it.
  - You still can set the addres of `first UObject` in GObject list or `first chunk` address.
  - Auto detect it's first `UObject` or first `chunk` address.
  
##### 31-06-2019
- Add `Class Finder`.
  - Search for instance with his `class name`.
  - Useful to find some `hard adrress`, for ex: to make a `sig` to scan.

##### 30-06-2019
- Add new settings.
- Sdk Generator +10% faster.
- Add `Game Name`, `Game Version` to `Sdk Generator`.
- Add `Sdk Type` to `Sdk Generator`.
  - **Internal**: Usually used when your target is inject dll into game process.
    - It's generate functions and funtction params. without `ReadAsMe`/`WriteAsMe` function.
    - You can directly cast/write block of memory as your class/struct.
  - **External**: Usually used when your target is write or read game process memory from your process.
    - Support read as object (class/struct) with `ReadAsMe` function in **every** class/struct.
    - Support write as object (class/struct) with `WriteAsMe` function in **every** class/struct.
    - It's useful to read/write block of memory as class/struct.
    - No functions generaterd for external for now. maybe later i will support call function from external.
    - To support `ReadAsMe`/`WriteAsMe` in your project, you need to edit **settings** file.
    - Good example for `read` function [here](https://github.com/CorrM/Unreal-Finder-Tool/blob/ebc7abfd28b2a5a3df19baffc485770f982d102d/UnrealFinderTool/Memory.h#L24), for write function same as `read` but `WriteProcessMemory`.
- Some Optimization.

##### 28-06-2019
- Add UI.
- Some changes to `SDK Generator` and `Instance Logger`.
- Let GObject address getted form `GObject Finder` be valid to use directly.
  - You can now just set the address you get from `GObjects finder` in the `GObject input field`.

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
