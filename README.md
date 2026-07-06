# BLINK

```text
██████╗ ██╗     ██╗███╗   ██╗██╗  ██╗
██╔══██╗██║     ██║████╗  ██║██║ ██╔╝
██████╔╝██║     ██║██╔██╗ ██║█████╔╝
██╔══██╗██║     ██║██║╚██╗██║██╔═██╗
██████╔╝███████╗██║██║ ╚████║██║  ██╗
╚═════╝ ╚══════╝╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝
```

**BLINK** is a real-time terminal stealth game written in C.

You play as **Zero**, a small digital survivor moving through a hostile system of patrols, broken walls, hidden rooms, signal pickups, secret hats, and terminal doors.

Move fast. Hide your signal. Escape before the system deletes you.

---

## The Legend

Long before modern engines, online leaderboards, and friendly interfaces, there was a terminal game whispered about inside programming offices.

According to the old story, **BLINK** began as an obscure 1980s assembly terminal game. Programmers would secretly play it during breaks, competing from their workstations to see who could finish the fastest. It was simple, unforgiving, and strange: a blinking character trapped inside a hostile computer system, trying to outrun patrols and reach the next door.

Nobody knows exactly who wrote the original version. Some say it was passed around on copied disks. Others say it lived only inside office machines, rewritten and modified by every programmer who touched it.

This project is a modern recreation of that lost classic.

This version was made by a programming student who loves old games and wanted to recreate the feeling of an 80s terminal challenge while training his skills with the **C language**.

---

## Gameplay

Zero must cross each level and reach the door:

```text
[]
```

Both `[` and `]` count as the exit.

The game is played in real time. Enemies move, shoot, chase, and react while you move through the map.

The goal is to finish all levels as fast as possible. The local leaderboard records the best completion times for each profile.

---

## Controls

```text
W A S D  Move
SPACE    Blink
F        Shoot
B        Back / abandon run
Q        Quit from main menu
```

---

## Symbols

```text
o   Zero
ó   Zero moving left
ò   Zero moving right
ø   Zero deleted

#   Wall, drawn as terminal box art
¦   Hidden breakable wall
.   Empty floor
*   Signal pickup
?   Hidden hat pickup
:   Bullet
~   Piercing wave
[]  Door / exit

^ > < v   Standard guards
» «       Advanced guards
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

## Alert Mode

When a guard sees Zero, the room enters **ALERT**.

In alert mode:

```text
Enemies chase Zero.
Enemies shoot when they have line of sight.
The room becomes much more dangerous.
```

---

## Hats

Hats are hidden upgrades. Each level contains a secret wall:

```text
¦
```

Shoot it to reveal a hidden passage. Inside the secret room, collect:

```text
?
```

Each hidden pickup unlocks one hat.

```text
ô   Contact Delete
    Touching guards deletes them instead of killing Zero.

õ   Piercing Wave
    Zero shoots ~ instead of :.
    The wave passes through enemies.

ö   Double Shot
    Zero can have two shots active at the same time.
```

After collecting a hat, you can equip it immediately during the run.

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
Unlocked hats
Selected hat
```

Player names can have up to 6 letters.

There is also one hidden name easter egg:

```text
LAURA -> LAURA♥
```

---

## Build and Run

Compile with GCC:

```bash
gcc -Wall -Wextra -std=c11 blink.c -o blink
```

Run:

```bash
./blink
```

---

## Development Notes

This project was built as a C practice project, with focus on:

```text
Real-time terminal input
Structs
Arrays
File saves
Local profiles
Collision detection
Simple AI
Terminal rendering
Game loop timing
```

The goal was not to make a modern-looking game, but to make something that feels like it could have existed on an old terminal: mysterious, minimal, and competitive.

---

## License

This is a personal learning project.
