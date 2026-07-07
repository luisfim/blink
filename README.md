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

You play as **Zero**, a small digital survivor moving through a hostile system. Move fast. Hide your signal. Escape before the system deletes you.

---

## The Legend
According to the rummors, **BLINK** began as an 1980s assembly terminal game. Programmers would play it during breaks, competing from their workstations to see who could finish the fastest. It was simple: a blinking character trapped inside a hostile computer system, trying to outrun patrols and reach the next door.

Nobody knows exactly who wrote the original version. Some say it was passed around on copied disks. Others say it lived only inside office machines, rewritten and modified by every programmer who touched it. This project is a modern recreation of that lost classic.

---

## Gameplay

Zero must follow the number crossing each level and reach the door:

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
Enemies shoot.
The room becomes much more dangerous.
```
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

## Build and Run

Compile with GCC:

```bash
git clone https://github.com/luisfim/blink.git

```
Run:

```bash
cd blink
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
---

## License

This is a personal learning project.
