# Blocking python (3.11) example for Pokemon Sword/Shield

# It reads a .ek8 file from a certain file path, injects it into box1slot1
# Starts a surprise trade with the given pokemon. It waits a certain amount of time (hoping the trade has completed)
# Finally, it extracts the pokemons .ek8 data from the game and saves it to the hard drive.

import socket
import time


# Make sure to append "\r\n" to the end of every command to ensure arg are parsed correctly
def sendCommand(s, content):
    content += '\r\n'
    s.sendall(content.encode())


# sys-botbase port is compiled for 6000
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("192.168.178.25", 6000))


# Read file bytes
with open("C:/temp/toInject.ek8", "rb") as inject:
    pokemon = inject.read().hex()

# Create a never ending loop
while True:

    # Inject pokemon file into box 1 slot 1
    sendCommand(s, f"poke 0x45075880 0x{pokemon}")

    # Automate trade buttons
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

    # Time we wait for a trade
    time.sleep(45)
    sendCommand(s, "click Y")
    time.sleep(0.7)
    time.sleep(30)

    # Read traded pokemon received from box 1 slot 1
    sendCommand(s, "peek 0x45075880 344")

    # Recieve data from socket and remove the trailing "\n"
    traded_pokemon = s.recv(689)[:-1]
    with open("C:/temp/lastReceived.ek8", "wb+") as dump:
        dump.write(bytes.fromhex(traded_pokemon))
