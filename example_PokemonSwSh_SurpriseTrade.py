#Simple example on how to connect to and send commands to the sys module.
#The example is for Pokemon Sword/Shield, it reads a .ek8 file from a certain file path, injects it into box1slot1
#and starts a surprise trade with the given pokemon. It waits a certain amount of time (hoping the trade has completed)
#before retrieving the new pokemon. Finally it extracts the pokemons .ek8 data from the game and saves it to the hard drive.
#The script assumes the game is set up in a way that the character is not currently in any menus and that the cursor of the
#pokebox is on box1slot1.

#The script isn't exactly robust, there are many ways to make it better (for example one could compare the box1slot1 data in
#RAM with that of the pokemon sent to see if a trade has been found and if not back out of the menu to search for another 10 
#seconds or so instead of waiting a fixed 45 seconds), but it is rather meant as a showcase of the functionalites of the 
#sysmodule anyway.

#Commands:
#make sure to append \r\n to the end of the command string or the switch args parser might not work
#responses end with a \n (only poke has a response atm)

#click A/B/X/Y/LSTICK/RSTICK/L/R/ZL/ZR/PLUS/MINUS/DLEFT/DUP/DDOWN/DRIGHT/HOME/CAPTURE
#press A/B/X/Y/LSTICK/RSTICK/L/R/ZL/ZR/PLUS/MINUS/DLEFT/DUP/DDOWN/DRIGHT/HOME/CAPTURE
#release A/B/X/Y/LSTICK/RSTICK/L/R/ZL/ZR/PLUS/MINUS/DLEFT/DUP/DDOWN/DRIGHT/HOME/CAPTURE

#peek <address in hex, prefaced by 0x> <amount of bytes, dec or hex with 0x>
#poke <address in hex, prefaced by 0x> <data, if in hex prefaced with 0x>

#setStick LEFT/RIGHT <xVal from -0x8000 to 0x7FFF> <yVal from -0x8000 to 0x7FFF


import socket
import time
import binascii


def sendCommand(s, content):
    content += '\r\n' #important for the parser on the switch side
    s.sendall(content.encode())

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("192.168.178.25", 6000))

fileIn = open("C:/temp/toInject.ek8", "rb")
pokemonToInject = fileIn.read(344)
pokemonToInject = str(binascii.hexlify(pokemonToInject), "utf-8")

time.sleep(2)
while True:
    sendCommand(s, f"poke 0x4293D8B0 0x{pokemonToInject}") #read pokemon from file and inject it into box1slot1 for trade


    sendCommand(s, "click Y")
    time.sleep(1)
    sendCommand(s, "click DDOWN")
    time.sleep(0.5)
    sendCommand(s, "click A")
    time.sleep(4)
    sendCommand(s, "click A")
    time.sleep(0.7)
    sendCommand(s, "click A")
    time.sleep(8)

    sendCommand(s, "click A")
    time.sleep(0.7)
    sendCommand(s, "click A")
    time.sleep(0.7)
    sendCommand(s, "click A")
    time.sleep(0.7)

    time.sleep(45) #Time we wait for a trade
    sendCommand(s, "click Y")
    time.sleep(0.7)
    time.sleep(30) #probably needs to be longer for trade evolutions

    sendCommand(s, "peek 0x4293D8B0 344") #get pokemon from box1slot1
    time.sleep(0.5) #give time to answer
    pokemonBytes = s.recv(689)
    pokemonBytes = pokemonBytes[0:-1] #cut off \n at the end
    fileOut = open("C:/temp/lastReceived.ek8", "wb")
    fileOut.write(binascii.unhexlify(pokemonBytes))
    fileOut.close()