# sys-netcheat

## Don't use this for online games! It'll ruin the experience for others and will probably get your switch banned in the process!

This is an open-source cheat-engine for the nintendo switch.

It requires a hacked switch (with hekate as the bootloader).

To use this first add `fullsvcperm=1` and `debugmode=1` to your `hekate_ipl.ini`.  
After that move the `sys-netcheat.kip` from the [release](https://github.com/jakibaki/sys-netcheat/releases/) to the `modules`-folder of your sdcard and add the line `kip1=modules/sys-netcheat.kip` to the `hekate_ipl.ini` as well.

Please keep in mind that at this point this will **not** work if layeredfs is enabled!

After installing simply boot your switch, start a game/homebrew and run

```
nc IP_OF_YOUR_SWITCH 5555
```

on your computer. You can find a windows version [here](https://eternallybored.org/misc/netcat/) (untested).

You'll be greeted by this:

```
Welcome to netcheat!
This needs fullsvcperm=1 and debugmode=1 set in your hekate-config!
> help
Commands:
    help                              | Shows this text
    ssearch u8/u16/u32/u64 value      | Starts a search with 'value' as the starting-value
    csearch value                     | Searches the hits of the last search for the new value
    poke address u8/u16/u32/u64 value | Sets the memory at address to value
```

---

As an example I'll show you how to change the number of bananas in botw (the values will be different for you!)

Right now I have `353` Mighty Bananas
```
> ssearch u32 353
Got a hit at c7581d9cc!
Got a hit at c758240a8!
Got a hit at c758279b8!
... (about 500 lines)
Got a hit at 345d1b2a84!
Got a hit at 346d9b9070!
Got a hit at 346dcfb138!
```
This isn't very helpful. I'll eat one banana in order to see which of those hits is the one I want.
```
> csearch 352
Got a hit at 33bb64a888!
Got a hit at 344d109b10!
Got a hit at 344dd0c050!
Got a hit at 3456a577d8!
```
This is a lot better. If there are stil too many hits you should repeat the `csearch` until you find what you want.

Now we'll poke the values to find the one that we actually want and check if any change occured (in zelda closing and reopening the inventory is necessary in order to see the change).

```
> poke 33bb64a888 u32 500
> poke 344d109b10 u32 500
> poke 344dd0c050 u32 500
> poke 3456a577d8 u32 500
```

The last one was the one we wanted!

![screeshot](/screenshot.jpg?raw=true)

We want to make sure that we never suffer from a lack of bananas again! So we'll freeze the value!

```
> afreeze 3456a577d8 u32 500
> lfreeze
0) 3456a577d8 (u32) = 500
```

If we want to unfreeze the value we just need to run

```
> dfreeze 0
```

And the value will be unfrozen.