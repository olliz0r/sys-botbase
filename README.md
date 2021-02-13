# sys-botbase based on jakibakis sys-netcheat
The purpose of this sys-module is to allow users to remote control their switch via wifi connection as well as to read and write to a games memory. This can be used to create bots for games and other fun automation projects.

## Warning:
Don't even think of blaming me if anything goes wrong with you using this. It's supposed to help you in the development of bot automation, but I am not liable for any damages or bans you might get in the process. Use at your own risk and all that.

## (planned) feature list:
### button input:
- [x] simulate button press
- [x] simulate button hold
- [x] set complete controller state

### touchscreen input:
- [x] simulate touchscreen press
- [x] simulate touchscreen hold
- [x] simulate touchscreen drawing

### Memory reading:
- [x] read x bytes of consecutive memory from RAM based on absolute memory address
- [x] read x bytes of consecutive memory from RAM based on address relative to main nso base
- [x] read x bytes of consecutive memory from RAM based on address relative to heap base

### Memory writing:
- [x] write x bytes of consecutive memory to RAM based on absolute memory address
- [x] write x bytes of consecutive memory to RAM based on address relative to main nso base
- [x] write x bytes of consecutive memory to RAM based on address relative to heap base

### screen capture:
- [x] capture current screen and send it (only as shitty jpeg at this time)

# Easy Installation
Download the .zip file and extract it to your sd card. The zip file contains the necessary folder structure and flag files.
Restart your switch. 

# Manual Installation
Copy the sys-botbase.nsp file to sdmc://atmosphere/contents/430000000000000B and rename it to exefs.nsp.
Create a new folder in sdmc://atmosphere/contents/430000000000000B names "flags".
Create a empty file called boot2.flag inside this folder.
Restart your switch.

The sysmodule opens a socket connection on port 6000. See the python example on how to talk to the sysmodule and what commands are available.


# Credits
big thank you to jakibaki for a great sysmodule base to learn and work with, as well as being helpful on the Reswitched discord!
also thanks to RTNX on discord for bringing to my attention a nasty little bug that would very randomly cause RAM poking to go bad and the switch (sometimes) crashing as a result.
thanks to Anubis for stress testing!