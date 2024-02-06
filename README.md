# Burger Shop 3 Bot

This is a bot for the game Burger Shop 3.  
It's written in C++ and uses the Windows API to read from memory and perform actions in the game.  
Keep in mind that this is a personal project, and it's not meant to be used for cheating or any other malicious purposes.  

## State of the Project
Currently, the bot is able to create all orders that don't require any machines (e.g. drinks, fries, etc.).  
This means that only some Blitz levels can be completed with the bot. Most of the levels require the use of machines, which the bot is not able to interact with yet.  
Keep in mind that the bot is still in development, and some features may not work as expected.  
It only works on Windows, and it has only been tested on Windows 11 (64-bit).

## How to Use
1. Download the latest release from the [releases page](https://github.com/IngoHHacks/BurgerShop3Bot/releases).
2. Extract the files to a folder.
3. Open the game.
4. Run the bot executable (BS3Bot.exe).
5. Make sure the window of the game is in the foreground and not moved or resized from what the bot set it to when started.
6. Start a level in the game.

## Keybinds
- `End`: Stop the bot.
- `Home`: Resume the bot. Hold `Home` to pause the bot until you release the key.
Note that the bot is enabled by default when started.

## Known Issues
- The bot only works on Windows.
- The bot only works on a specific resolution and position of the game window (800x600, top-left corner of the screen).
- The game may crash on rare occasions when starting a level with the bot running. The cause of this is unknown.
- Machines are not supported yet.
- Some food may be messed up. It will automatically discard the food and create a new one if it gets stuck.
- The bot only works on specific versions of the game. This is due to the fact that the bot depends on breakpoints and specific memory offsets, which may change between versions.
