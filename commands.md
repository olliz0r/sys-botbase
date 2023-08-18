# Available Commands

## RAM reading
### Single Read
|Command|Description|Parameters|Usage|
|--|--|--|--|
|peek  |Reads memory at given address relative to heap  |1. address to read from relative to heap in hex<br>2. amount of bytes to read<br>Return: hex string |peek 0x45075880 344   |
|peekAbsolute  |Reads memory at given absolute address  |1. address to read from in hex<br>2. amount of bytes to read<br>Return: hex string |peekAbsolute 0x45075880 344   |
|peekMain  |Reads memory at given address relative to NSOMain |1. address to read from relative to NSOMain in hex<br>2. amount of bytes to read<br>Return: hex string |peekAbsolute 0x45075880 344   |

### Multiple Reads
|Command|Description|Parameters|Usage|
|--|--|--|--|
|peekMulti  |Reads memory at given addresses relative to heap  |1. address to read from relative to heap in hex<br>2. amount of bytes to read<br>...<br>n. address to read from relative to heap in hex<br>n+1. amount of bytes to read <br>Return: hex string |peekMulti 0x45075880 344 0x45097552 344 0x45774450 344  |
|peekAbsoluteMulti  |Reads memory at given absolute addresses  |1. address to read in hex<br>2. amount of bytes to read<br>...<br>n. address to read in hex<br>n+1. amount of bytes to read <br>Return: hex string |peekMulti 0x45075880 344 0x45097552 344 0x45774450 344  |
|peekMainMulti  |Reads memory at given absolute addresses  |1. address to read relative to NSOMain in hex<br>2. amount of bytes to read<br>...<br>n. address to read relative to NSOMain in hex<br>n+1. amount of bytes to read <br>Return: hex string |peekMulti 0x45075880 344 0x45097552 344 0x45774450 344  |

### Pointer Reads
|Command|Description|Parameters|Usage|
|--|--|--|--|
|pointerPeek|Follows a chain of pointers and reads the final value|1. amount of bytes to read<br>2. first jump relative to NSOMain<br>3. offset after following first pointer<br>...<br>n. offset after following previous pointer<br>n+1 Final offset to reach the value to read| pointerPeek 344 0x45097552 0x10 0x20 0x30
|pointerPeekMulti|Follows a chain of pointers and reads the final value, accepts more than one chain separated by *|1. amount of bytes to read<br>2. first jump relative to NSOMain<br>3. offset after following first pointer<br>...<br>n. offset after following previous pointer<br>n+1 Final offset to reach the value to read| pointerPeek 344 0x45097552 0x10 0x20 0x30 * 344 0x62097552 0x10 0x4
|pointer|Follows a chain of pointers and prints the final absolute address|1. first jump relative to NSOMain<br>2. offset after following first pointer<br>...<br>n. offset after following previous pointer (will still jump to this one)| pointer 0x45097552 0x10 0x20
|pointerAll|Follows a chain of pointers and prints the final absolute address, allows adding a final offset without jumping|1. first jump relative to NSOMain<br>2. offset after following first pointer<br>...<br>n. offset after following previous pointer (will **not** jump to this one)| pointerAll 0x45097552 0x10 0x20 0x4
|pointerRelative|Follows a chain of pointers and prints the final address relative to heap, allows adding a final offset without jumping|1. first jump relative to NSOMain<br>2. offset after following first pointer<br>...<br>n. offset after following previous pointer (will **not** jump to this one)| pointerRelative 0x45097552 0x10 0x20 0x4

## RAM Writing
### Single Write
|Command|Description|Parameters|Usage|
|--|--|--|--|
|poke  |Writes bytes to given address relative to heap  |1. address to write to relative to heap in hex<br>2. data in hex to write |poke 0x45075880 0xDEADBEEF   |
|pokeAbsolute  |Writes bytes to given absolute address  |1. absolute address to write to in hex<br>2. data in hex to write |poke 0x45075880 0xDEADBEEF   |
|pokeMain  |Writes bytes to given address relative to NSOMain  |1. address to write to relative to NSOMain in hex<br>2. data in hex to write |poke 0x45075880 0xDEADBEEF   |

### Pointer Write
|Command|Description|Parameters|Usage|
|--|--|--|--|
|pointerPoke  |Writes bytes to address resulting of following a pointer chain  |1. bytes in hex to write <br>2. first jump relative to NSOMain<br>3. offset after following first pointer<br>...<br>n. offset after following previous pointer<br>n+1. offset after following previous pointer (will **not** jump to this one) |pointerPoke 0xDEADBEEF 0x45075880 0x10 0x20 0x30   |

### Freezing of values
|Command|Description|Parameters|Usage|
|--|--|--|--|
|freeze|Freezes a value in RAM (writing every X milliseconds to it to ensure it does not get overwritten by game logic|1. absolute address to freeze<br>2. value to freeze it to in hex|freeze 0x45097552 200|
|unFreeze|Unfreezes a previously frozen value in RAM|1. absolute address to unfreeze|unFreeze 0x45097552|
|freezeCount|Returns number of frozen addresses in RAM|none|freezeCount|
|freezeClear|Unfreezes all values in RAM|none|freezeClear|
|freezePause|Pauses the freezing process, allows for unpause to refreeze|none|freezePause|
|freezeUnpause|Unpauses a previously paused freezing of all values|none|freezeUnpause|


## Controls
### Controller Input
See https://github.com/olliz0r/sys-botbase/blob/master/sys-botbase/source/util.c#L145 for a list of available buttontypes to press.
|Command|Description|Parameters|Usage|
|--|--|--|--|
|press|Presses and holds a button|1. buttonType| press A|
|release|Releases a button from being pressed|1. buttonType| release A|
|click| Holds a button pressed and releases it after a configured period of time<br>default 50ms| 1. buttonType| click A|
|setStick|Sets stick position|1. LEFT/RIGHT<br>2.XVal (-0x8000 is min, 0x7FFF is max)<br>3.YVal| setStick LEFT 0x7FFF 0x0|
|clickSeq|Sends several button inputs and wait commands in sequence|1. single string (no spaces) with comma-separated commands out of the following<br><br>**buttonType** for click<br>**+buttonType** for press<br>**-buttonType** for release<br>**Wnumber** to sleep number ms<br>**%X,Y** move left stick to position X Y<br>**&X,Y** move right stick to position X Y| clickSeq A,W1000,B,W200,DUP,W500,DD,W350,%5000,1500,W2650,%0,0
|clickCancel|Interrupts click sequence|none|clickCancel|
|detachController|Forces the virtual controller to detach, useful in cases where it bugs out|none|detachController|

### Touchscreen Input
|Command|Description|Parameters|Usage|
|--|--|--|--|
|touch|Sequential taps to the touchscreen|1.X in range 0-1280<br>2.Y in range 0-720<br>...<br>n-1. X in range 0-1280<br>n. Y in range 0-720|touch 200 500<br>touch 200 500 200 800|
|touchHold|Single tap to hold<br>Runs in its own thread but will not allow the call again while running|1. X in range 0-1280<br>2. Y in range 0.720<br>3. milliseconds to hold (at least 15)| touchHold 200 500 1000|
|touchDraw|Moves the touch from given position to the next, effectively drawing on the touchscreen<br>Runs in its own thread but will not allow the call again while running|1. X in range 0-1280 starting point<br>2. Y in range 0-720 starting point<br>3. X second point<br>4. Y second point<br>...<br>n-1. X last point<br>n. Y last point|touchDraw 100 200 100 500 200 500 200 200|
|touchCancel|Cancels current touch operation|none|touchCancel|

### Keyboard Input
See https://switchbrew.github.io/libnx/hid_8h.html HidKeyboardKey and HidKeyboardModifier for available keys and modifiers.
|Command|Description|Parameters|Usage|
|--|--|--|--|
|key|Types several keys on the keyboard in sequence|1. HidKeyboardKey1<br>...<br>n. HidKeyboardKeyN|key 11 8 15 15 18|
|keyMod|Types several keys on the keyboard in sequence with modifier keys<br>Do not bitshift the modifiers yourself, sys-botbase will do the shifting| 1. HidKeyboardKey1<br>2.HidKeyboardModifier1<br>...<br>n-1. HidKeyboardKeyN<br>n. HidKeyboardModifierN|keyMod 4 1|
|keyMulti|Presses several keys at the same time|1. HidKeyboardKey1<br>...<br>n. HidKeyboardKeyN|keyMulti 224 226 23|


## Screen Control
|Command|Description|Parameters|Usage|
|--|--|--|--|
|pixelPeek|Returns .jpg file of the current screen|none|pixelPeek|
|screenOff|Turns the screen off|none|screenOff|
|screenOn|Turns the screen on|none|screenOn|

## Utility

|Command|Description|Parameters|Usage|
|--|--|--|--|
|getTitleID|Returns TitleId of application currently running|none|getTitleID|
|getTitleVersion|Returns Version of Title currently running|none|getTitleVersion|
|getSystemLanguage|Returns Language of the Switch OS|none|getSystemLanguage|
|getBuildID|Returns BuildID of the Application running|none|getBuildID|
|getHeapBase|Returns Memory address of the Heap Base|none|getHeapBase|
|getMainNsoBase|Returns Memory address of the NSOMain|none|getMainNsoBase|
|isProgramRunning|Checks if program with given id is running|1. programID to check| isProgramRunning 0x420000000007e51a|
|game|Returns Metadata about the running game|1. one of the following<br>**icon** IconData<br>**version** Game Version<br>**rating** age rating<br>**author** Author of the game<br>**name** Name of the game|game rating|
|getVersion|Returns version of sys-botbased used|none|getVersion|
|charge|Returns charge status of the battery|none|charge|

## Configure
The configure command allows setting of some timing values in sys-botbase:
|Configure parameter|Description|Parameters|Usage|
|--|--|--|--|
|mainLoopSleepTime|Time the main thread sleeps after every single command<br>default 50ms|1. New time in ms to sleep after every command|configure mainLoopSleepTime 10|
|buttonClickSleepTime|How long a button is held down during the "click" call. This blocks the main loop<br> default:50ms<br>Make sure this isn't lower than the fps on the game or a click might not get recognized by the game|1. New time in ms to hold a button down during click|configure buttonClickSleepTime 40|
|echoCommands|Returns every command back for debugging purposes<br>default 0|1 or 0| configure echoCommands 0|
|printDebugResultCodes|Prints some Resultcodes for debugging purposes<br>default 0|1 or 0|configure printDebugResultCodes 0|
|keySleepTime|How long a key is held down during the "key" call. This does not block the main loop<br>default 25|1. New key press sleep time|configure keySleepTime 40|
|fingerDiameter|Controls the diameter of the virtual touch finger<br>default 50|1. new diameter for touch events|configure fingerDiameter 100|
|pollRate|How long a touch event shall be held down<br>default 17<br>polling is linked to screen refresh rate (system UI) or game framerate. Most cases this is 1/60 or 1/30|1. New poll rate|configure pollRate 34|
|freezeRate|How often frozen values shall be rewritten to RAM<br>default 3ms|1. new freezerate in ms|configure freezeRate 10|
|controllerType|controllerType to use for controller input commands<br>default 3|See HidDeviceType on https://switchbrew.github.io/libnx/hid_8h.html|configure controllerType 12|
 










