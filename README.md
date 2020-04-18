# sys-botbase
The purpose of this sys-module is to allow users to remote control their Switch via WiFi connection as well as to read and write to a games memory. This can be used to create bots for games and other fun automation projects.

## Disclaimer:
Don't even think of blaming me if anything goes wrong with you using this. It's supposed to help you in the development of bot automation, but I am not liable for any damages or bans you might get in the process. Use at your own risk and all that.

## (Planned) Feature List:
### Button Input:
- [x] simulate button press
- [x] simulate button hold
- [x] set complete controller state

### Touchscreen Input:
- [ ] simulate touchscreen press
- [ ] simulate touchscreen hold
- [ ] simulate touchscreen drawing

### Memory Reading:
- [ ] read x bytes of consecutive memory from RAM based on absolute memory address
- [x] read x bytes of consecutive memory from RAM based on address relative to heap base

### Memory Writing:
- [ ] write x bytes of consecutive memory to RAM based on absolute memory address
- [x] write x bytes of consecutive memory to RAM based on address relative to heap base

### Screen Capture:
- [ ] capture current screen and send it

## Installation
I've only tried this on Atmosphere (0.10.2), so if you are using a different CFW your experience might vary.

1) Copy the sys-botbase.nsp file to sdmc://atmosphere/contents/430000000000000B and rename it to exefs.nsp.
2) Create a new folder in sdmc://atmosphere/contents/430000000000000B names "flags".
3) Create a empty file called boot2.flag inside this folder.
4) Restart your switch.

The sysmodule opens a socket connection on port 6000. See the Python example on how to talk to the sysmodule and what commands are available.

## Credits

* __jakibaki__ for [sys-netcheat](https://github.com/jakibaki/sys-netcheat) from which sysbot-base is based upon and for being incredibly helpful on the ReSwitched Discord.
* __zaksabeast__ for help with ascertaining the language of the system.
* __Behemoth__ for help with screenshot capture for debugging.
* _All those who actively contribute to the sysbot-base repository._