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
- Windows 64bit => x32/x64 games.
- Windows 32bit => x32 games.

# How to use
This video tell you how to use the tool and dump sdk for ue4 game.
- Some changes before watch the video.
  - Now you don't need to do any thing about GNames address since now you just need to click `use` button.

[![Youtube Video](/images/youtube_thumbnail.png)](https://www.youtube.com/watch?v=3CjsrnvKtGs)

# Screen Shots
![ui_1](/images/ui_1.png)
![ui_2](/images/ui_2.png)
![ui_3](/images/ui_3.png)

# Download
[![Last Version](/images/download.gif)](https://github.com/CorrM/Unreal-Finder-Tool/releases/latest)

# Credits
- [@CorrM](https://github.com/CorrM).
- [@WheresMyRide](https://github.com/WheresMyRide).
- @KN4CK3R.
- [@harakirinox](https://www.unknowncheats.me/forum/members/1692305.html).

# Change Log
##### 09-07-2019
- `JsonEngine` is system that's use josn files as container for main ue4 structs.
  - Since UE4 have different versions, some time `ue4 structs` changes.
  - That's make fix specific games problems is easy, since the most of problems because of UE4 Version structs changes.
  - So `JsonEngine` give me the ability to just create an other json file that's have the changed structs to override default structs to support any other UE4 version.
- `SDK generator ReWork` is hard changes for `sdk generator`.
  - That's make the tool now faster than before significantly.
  - Easy to add and improve feature In the future.
- `Tool Debugging`, it's to help fetching why tool crashed and generate file that's help to solve the problem.
- New UI that give me some space to add new `Features`.
- `Address Veiwer` is a hex viwer to dump memory arround to address pulled form the tool.
  - That's give you ability to check if your address is valid to use or not.
- Add some UI labels.
  - Unreal Version: That's fetch which UE that game development with.
  - Win Title: Get window title of target game.
- `GObjects/GName Finder` Improved.
  - Let `GNames address` that's pulled from the finder to be used directly without need to dereference it.
  - `GObjects Finder` now get GObjects chunks address.
- `SDK Generator` Improved.
  - Fix some bugs that's case `some problem` or `missed offsets`.
  - Fix some code form that's slow the generating progress.
- Performance improves.
  - `GObjects/GName Finder` now really faster and more stability.
  - `Instance Logger` now really faster and more stability.
  - `SDK Generator` now really faster and more stability.
- `BUGS`.
  - Fix some `UI` bugs.
  - Fix some `Finder` bugs.

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
