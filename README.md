# BLINK

```text
██████╗  ██╗     ██╗ ███╗   ██╗ ██╗  ██╗
██╔══██╗ ██║     ██║ ████╗  ██║ ██║ ██╔╝
██████╔╝ ██║     ██║ ██╔██╗ ██║ █████╔╝
██╔══██╗ ██║     ██║ ██║╚██╗██║ ██╔═██╗
██████╔╝ ███████╗██║ ██║ ╚████║ ██║  ██╗
╚═════╝  ╚══════╝╚═╝ ╚═╝  ╚═══╝ ╚═╝  ╚═╝
```

**BLINK** is a real-time terminal stealth game written in C.

You play as **Zero**, a small digital survivor moving through a hostile system. Move fast, hide your signal, follow the one, and escape before the system deletes you.

<p align="center">
  <img src="assets/screenshot.jpeg" alt="BLINK gameplay screenshot" width="300">
</p>

---

## The Legend

According to the rumors, **BLINK** began as an obscure 1980s assembly terminal game. Programmers would play it during breaks, competing from their workstations to see who could finish the fastest.

Nobody knows exactly who wrote the original version. Some say it was passed around on copied disks. Others say it lived only inside office machines, rewritten and modified by every programmer who touched it.

This project is a modern recreation of that lost classic, built as a C practice project by a programming student who loves old games, terminals, and strange digital legends.

---

## Gameplay

Zero must cross each room, follow the moving `1`, and reach the door:

```text
[]
```
---

## Controls

```text
W A S D  Move
SPACE    Blink
F        Shoot
B        Back / abandon run
Q        Quit from the main menu
```
---

## Signal

Zero uses **Signal** as a limited resource.

Signal is spent to:

```text
Blink
Shoot
```

Signal can be restored by collecting:

```text
*
```

Managing Signal is part of the strategy. Shooting everything is possible, but not always smart.

---

## Blink

Blink makes Zero temporarily disappear from enemy vision.

Use it to cross dangerous sightlines, escape patrols, or survive when the system enters alert mode.

---

## Profiles and Leaderboard

BLINK uses a local save file:

```text
blink_saves.txt
```

Profiles store:

```text
Name
Wins
Deaths
Best time
```

The leaderboard is stored locally on your machine.

---

## Requirements

You need a C compiler and a terminal with UTF-8 support.

Recommended terminal size:

```text
At least 80 columns wide
At least 40 rows tall
```

The game uses Unicode symbols and ANSI terminal escape codes. A modern terminal is recommended.

---

## Build and Run

Clone the repository:

```bash
git clone https://github.com/luisfim/blink.git
cd blink
```

### Linux

Install GCC if needed:

```bash
sudo apt update
sudo apt install gcc -y
```

Compile:

```bash
gcc -Wall -Wextra -std=c11 blink.c -o blink
```

Run:

```bash
./blink
```

### macOS

Install the Apple command line tools if needed:

```bash
xcode-select --install
```

Compile:

```bash
clang -Wall -Wextra -std=c11 blink.c -o blink
```

Run:

```bash
./blink
```

### Windows

The portable version of BLINK supports native Windows builds through MinGW-w64/MSYS2.

Install MSYS2, open the **MSYS2 MinGW 64-bit** terminal, then install GCC:

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc
```

Clone and compile:

```bash
git clone https://github.com/luisfim/blink.git
cd blink
gcc -Wall -Wextra -std=c11 blink.c -o blink.exe
```

Run:

```bash
./blink.exe
```

### Windows through WSL

You can also run the Linux version on Windows using WSL:

```powershell
wsl --install
```

Then, inside Ubuntu/WSL:

```bash
sudo apt update
sudo apt install gcc git -y
git clone https://github.com/luisfim/blink.git
cd blink
gcc -Wall -Wextra -std=c11 blink.c -o blink
./blink
```

---

## Troubleshooting

### The screen looks broken or scrolls constantly

Make the terminal bigger or zoom out.

```text
BLINK needs enough terminal space for the map and UI.
```

### Symbols look wrong

Use a terminal with UTF-8 support.

Recommended terminals:

```text
Linux: GNOME Terminal, Konsole, xterm, Alacritty
macOS: Terminal.app, iTerm2
Windows: Windows Terminal
```

## Development Notes

This project was built as a C practice project, with focus on:

```text
Real-time terminal input
Cross-platform terminal handling
Structs
Arrays
File saves
Local profiles
Collision detection
Simple AI
Terminal rendering
Game loop timing
ANSI escape codes
```

The code now uses a small platform layer so the same source can compile on Linux, macOS, and Windows.

---

## License

This is a personal learning project.
