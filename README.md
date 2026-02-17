# Astra Chess Engine
```
╔═══════════════════════════════════════════════════════╗
║     ▄▄      ▄▄▄▄▄     ▄▄▄▄▄▄▄    ▄▄▄▄▄▄        ▄▄     ║
║   ▄█▀▀█▄   ██▀▀▀▀█▄  █▀▀██▀▀▀▀  █▀██▀▀▀█▄    ▄█▀▀█▄   ║
║   ██  ██   ▀██▄  ▄▀     ██        ██▄▄▄█▀    ██  ██   ║
║   ██▀▀██     ▀██▄▄      ██        ██▀▀█▄     ██▀▀██   ║
║ ▄ ██  ██   ▄   ▀██▄     ██      ▄ ██  ██   ▄ ██  ██   ║
║ ▀██▀  ▀█▄█ ▀██████▀     ▀██▄    ▀██▀  ▀██▀ ▀██▀  ▀█▄█ ║
║                                                       ║
╚═══════════════════════════════════════════════════════╝
```
Astra is a low-performance, standalone C++ chess engine with a graphical interface. Rated at approximately 1500 Elo.

# Contents
Astra is distributed as a portable "plug-and-play" package.\
In order to use the optional opening books, keep one or more of them in the same dir as AstraChess.exe.

 - `AstraChess.exe` - The main standalone engine.
 - `book_pro.bin` (3.3M positions)
 - `book.bin`
 - `book_light.bin`
 - `book_flash.bin`
 - `book_flash_light.bin`

# Console Commands
```
╔═══════════════════════╦════════════════════════════════════════════════════╗
║ Command               ║ Description                                        ║
║  set white bot/human  ║ Toggle White between AI and User control.          ║
║  set black bot/human  ║ Toggle Black between AI and User control.          ║
║  set depth [1-7]      ║ Set the AI search depth (Default: 6).              ║
║  set book [filename]  ║ Manually point to a book (set book book_pro.bin)   ║
║  status               ║ View current engine, ELO, and book configuration.  ║
║  help                 ║ Show all available commands.                       ║
║  start                ║ Close configuration and launch the board.          ║
╚═══════════════════════╩════════════════════════════════════════════════════╝
```