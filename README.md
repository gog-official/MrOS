<div align="center">

<pre>



   ▄▄▄▄███▄▄▄▄      ▄████████  ▄██████▄     ▄████████ 
 ▄██▀▀▀███▀▀▀██▄   ███    ███ ███    ███   ███    ███ 
 ███   ███   ███   ███    ███ ███    ███   ███    █▀  
 ███   ███   ███  ▄███▄▄▄▄██▀ ███    ███   ███        
 ███   ███   ███ ▀▀███▀▀▀▀▀   ███    ███ ▀███████████ 
 ███   ███   ███ ▀███████████ ███    ███          ███ 
 ███   ███   ███   ███    ███ ███    ███    ▄█    ███ 
  ▀█   ███   █▀    ███    ███  ▀██████▀   ▄████████▀  
                   ███    ███                         
                   <p><em>"Yeah buddy a fitness os, be ready for a once in a lifetime experience"</em></p>
</pre>

<p>
    <img src="https://img.shields.io/badge/arch-x86%2032--bit-blue?style=flat-square" alt="architecture">
    <img src="https://img.shields.io/badge/lang-C%20%2B%20NASM-orange?style=flat-square" alt="language">
    <img src="https://img.shields.io/badge/boot-custom%20bootloader-grey?style=flat-square" alt="bootloader">
    <img src="https://img.shields.io/badge/fs-FAT12-lightgrey?style=flat-square" alt="filesystem">
    <img src="https://img.shields.io/badge/editor-vi--like-yellow?style=flat-square" alt="editor">
    <img src="https://img.shields.io/badge/FITNESS-MANDATORY-red?style=flat-square" alt="finess">

</p>
</div>

---

## Basic overview

Hey, this os named "MrOS" is a **32-bit x86 freestanding operating system** writen from very scratch in C and NASM. I am a beginner who had little experience reinventing the wheel, so this wasn't that hard for me.
It boots via a custom bootloader, it runs a FAT12 filesystem, it forces you to complete a **mandatory workout** before logging in to the os. Also, it comes with a hand-crafted vi-like editor, auth, nutrition calculators and speaker melodies. I haven't used libc, just raw, real hardware and some real pump ;).

For technical guys:
    - **Memory map**: kernel @ `0x10000`, stack @ `0x200000`, VGA buffer @ `0xB8000`
    - **Disk image**: 4096 sector MBR, max kernel 130KB, FAT12 data area
    - **Shell hostname**: `gymbro` (-_- what else do you expected)

### But why?

So, recently I went through a fitness transformation and realised how important your fitness is to your happiness, I feel much lighter, much happy and much energetic nowadays. 
So, I wanted to set an example by reminding people deep inside tech, how important health is. MrOS is not a special fully built OS, it has bugs(a lot of them) but what it does is helps you realise and motivate you further in your fitness journey.

---

## Buffs

### Core & boot
```
    - **Custom boot sector**: real mode, loads the kernel via `INT 0x13`
    - **A20 gate** with 3 fallback methods
    - **Protected mode switch**: flat 4 GB GDT, `CR0.PE` bit
    - **VGA text driver**: 80x25 with hardware cursor, scroll and a nice old statusbar(at 23-24 row)

### Kernel Subsystems
    - **Interruprs**: IDT, PIC remap, `outb`/`inb` port I/O
    - **PS/2 keyboard** + cuter PIT timer (100 Hz / 10 ms tick)
    - **PC speaker**: square-wave melodies (channel 2)
    - **ATA disk** driver

### Filesystem
    - **VFS abstraction layer** + FAT12 backend
    - Full CRUD, kinda: `open`, `read`, `write`, `create`, `remove`, `mkdir`, `readdir`

### User & Auth
    - **User accounts** with admin and normal flags, individual home directories
    - **Login** with 3-attempt lockout (15 s cooldown) (no roast, trust me)
    - **Hashing ans stuff**: DJB2 double-pass, salted with username + timer + knuth shuffle
    **IMPORTANT**: first boot wizard creates the initial admin

### Fitness & Health(might be first time you see this in a github repo lol)
    - **Mandatory Workout**: 40s pushups - 10s rest - 50s squats(can't skip this -_-)
    - Custom excercise times, like you can make your own workout (`exercise BURPEE 30`)
    - **Nutrition macors**: `protein`, `fat`, `carbs` calculators
    - Wootaah reminder (default 1.5 hours water reminder)
    - Workout stats & motivational quotes

### UI & shell
    - **Status bar**: shows uptime + timed message on protected rows
    - **Scrollback buffer**: HEY, `Ctrl+UP/Dwn` or `pg up/down`
    - **Interactive shells**: prompt `user@gymbro:/cwd>` and 25 builtin commands

### Vi-like Editor
    - Modal editor: Normal, Insert, Command, Search
    - `hjkl`, word motions, `0`/`$`, `gg`/`G`
    - Undo, search (`/term` + `n`), `:w` / `:q`/ `:wq` / `:q!`
    - Direct VGA writes, no terminal emulations lol

## Build & Run

### Prerequisites
- `i686-elf-gcc` (cross-compiler)
- `i686-elf-ld`, `i686-elf-objcopy`
- `nasm`
- `qemu-system-i386`
- `python3`, `make`

### commands

| Command       | Effects
|
|---------------|----------------------------------------------------|
| `make`        | Build `mros.img`
|
| `make run`    | Useless, runs in text mode
|
| `make run-gui`| Run in QEMU **with PC speaker audio** (PulseAudio)
|
| `make clean`  | Remove all build artefacts
|

## Preview

![Kernel](https://cdn.hackclub.com/019de41b-9a2b-7ce0-bb17-0dca656a5a45/screenshot-2026-05-01_20-56-16.png)
![Fitness](https://cdn.hackclub.com/019de41b-c834-7c13-a21b-ab90c68378ee/screenshot-2026-05-01_20-58-04.png)
![login](https://cdn.hackclub.com/019de41b-e033-71bc-a378-34e7aea363da/screenshot-2026-05-01_20-59-36.png)
![shell-help](https://cdn.hackclub.com/019de41c-13cc-75d4-9825-c7a6f9a0b3f1/screenshot-2026-05-01_20-58-58.png)
![shell0help](https://cdn.hackclub.com/019de41b-f848-735a-90bd-4ea41edc2a75/screenshot-2026-05-01_20-59-19.png)
![vi-editor](https://cdn.hackclub.com/019de41c-2fb0-7edb-a273-bd6e9d334abf/screenshot-2026-05-01_20-58-50.png)

## Installation
    Install the appropriate ISO from the release page and boot from a USB to your computer or VM.


Now go drink some water.

---

<div align="center">
    <sub>Built with glutes 💙</sub>
</div>
