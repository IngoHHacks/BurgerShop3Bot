# Burger Shop 3 Bot

Burger Shop 3 Bot (or BS3Bot for short, not to be confused with BurgerBot, which is a character in the Burger Shop series) is a bot for the game Burger Shop 3.
It's written in C++ and uses the Windows API to read from memory and perform actions in the game.  
Keep in mind that this is a personal project, and it's not meant to be used for cheating or any other malicious purposes.  
Use at your own risk. I'm not responsible for any damage caused by the bot.  
Remember, if you use the bot, you're not playing the game, and you're not having fun. Using the bot to progress in the game would make the game boring and pointless.  
The bot is not meant to be used for speedrunning or any other competitive purposes, although I doubt you'd be able to use it for that anyway, as the mouse teleporting all over the screen would be a dead giveaway that you're using a bot.

## State of the Project
Currently, the bot is able to create all orders that don't require any machines (e.g. drinks, fries, etc.).  
This means that only some Blitz levels can be completed with the bot. Most of the levels require the use of machines, which the bot is not able to interact with yet.  
Keep in mind that the bot is still in development, and some features may not work as expected.  
It only works on Windows, and it has only been tested on Windows 11 (64-bit).

## How to Use
1. Download the latest release from the [releases page](https://github.com/IngoHHacks/BurgerShop3Bot/releases). Make sure to download the correct version that matches the game version you have.
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
