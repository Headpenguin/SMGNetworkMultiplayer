# Super Mario Galaxy Network Play
This mod allows for Super Mario Galaxy multiplayer over the internet, with multiple Marios

## Installation
Download a release from the releases tab on github, extract the corresponding zip file (either dolphin or wii 
and from the correct version, depending on which platform you will be playing the mod on), and then place
`riivo_USA.xml`, `CustomCode_USA.bin`, and `serverIP.txt` into the riivolution folder. 

### ***or***

compile the patches yourself, create an appropriate riivo_USA.xml file from the produced
files and a loader xml, and then launch the game.

## Running
First, set up a [server](https://github.com/Headpenguin/SMGMultiplayerServer) (or have a friend do this step).
Next, open `serverIP.txt` with a text editor, and enter the IP address and, optionally, port of the 
server you will be connecting to. Enable `Syati` in Riivolution, and then launch the game with the patches!  

## Building
Good luck with this. I have not really put much effort into making a cross-platform Makefile.
Hope you have a good time getting it to work! As a note, config.mk is a good place to start.

### Dependencies
All dependencies of Bussun are required, as well as Bussun itself. Provide the appropriate paths
to config.mk. Also, you should not expect the Makefile to have even the slightest chance of
working on any machine that is not Linux-based.  

**Important:** use the multiplayer branches of my forks of Bussun, Petari, and Kamek for building. 
The upstream repositories may eventually work with this mod, but for now, they do not. My development 
is currently not organized enough to integrate with upstream nicely.

## Reporting issues
Use the issues feature in Github. If you actually want the issue to get solved, be as specific as possible.
Provide screenshots/pictures, and if you get a crash handler, scroll through the whole output and send it.
Of course, any report is better than no report, but the amount of details you provide directly corresponds
to the likelihood of a resolution.    

Feel free to make suggestions here as well.

## Credits
See credits.txt. Tools:
- [Kamek](https://github.com/Treeki/Kamek)
- [Petari](https://github.com/SMGCommunity/Petari)
- [Bussun](https://github.com/SMGCommunity/Bussun)
