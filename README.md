# BLINK

**BLINK** is a small terminal stealth game written in C.

You play as a blinking cursor trying to escape a hostile terminal. The player is visible every other turn, and guards can only spot you when you blink into sight.

## Goal

Reach `[]` without being seen.

## Rules

* The player is represented by `_`.
* The exit is represented by `[]`.
* Guards are represented by `^`, `>`, `<`, and `v`.
* A guard’s symbol shows its movement direction and line of sight.
* Guards see in straight lines.
* Walls block vision.
* The player is visible every other turn.
* Press `SPACE` while hidden to stay invisible for one extra move.
* Signal starts at `3`.
* Signal pickups are represented by `*`.

## Controls

| Key     | Action     |
| ------- | ---------- |
| `W`     | Move up    |
| `A`     | Move left  |
| `S`     | Move down  |
| `D`     | Move right |
| `.`     | Wait       |
| `SPACE` | Hold blink |
| `Q`     | Quit       |

## How to compile

```bash
gcc blink.c -o blink
```

## How to run

```bash
./blink
```

## Current version

**BLINK v0.1**

Current features:

* Player movement
* Visible/hidden blink system
* Signal resource
* Hold Blink ability
* Signal pickups
* Guard line of sight
* Moving guard patrols
* Multiple rooms
* Win and game-over conditions

## Status

This is an early prototype made as a learning project in C.
