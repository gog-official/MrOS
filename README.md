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

<div style="display: flex; flex-wrap: wrap; gap: 1.5rem; justify-content: center;">

  <div style="flex: 1 1 280px; border: 2px solid #4f46e5; border-radius: 16px; padding: 1.2em; background: #f0f4ff; box-shadow: 0 4px 12px rgba(0,0,0,0.08);">
    <h3 style="margin-top:0; color:#4338ca;">Core & boot</h3>
    <ul style="margin:0; padding-left:1.2em;">
      <li><strong>Custom boot sector</strong>: real mode, loads the kernel via <code>INT 0x13</code></li>
      <li><strong>A20 gate</strong> with 3 fallback methods</li>
      <li><strong>Protected mode switch</strong>: flat 4 GB GDT, <code>CR0.PE</code> bit</li>
      <li><strong>VGA text driver</strong>: 80x25 with hardware cursor, scroll and a nice old statusbar(at 23-24 row)</li>
    </ul>
  </div>

  <div style="flex: 1 1 280px; border: 2px solid #0891b2; border-radius: 16px; padding: 1.2em; background: #ecfeff; box-shadow: 0 4px 12px rgba(0,0,0,0.08);">
    <h3 style="margin-top:0; color:#0e7490;">Kernel Subsystems</h3>
    <ul style="margin:0; padding-left:1.2em;">
      <li><strong>Interruprs</strong>: IDT, PIC remap, <code>outb</code>/<code>inb</code> port I/O</li>
      <li><strong>PS/2 keyboard</strong> + cuter PIT timer (100 Hz / 10 ms tick)</li>
      <li><strong>PC speaker</strong>: square-wave melodies (channel 2)</li>
      <li><strong>ATA disk</strong> driver</li>
    </ul>
  </div>

  <div style="flex: 1 1 280px; border: 2px solid #ca8a04; border-radius: 16px; padding: 1.2em; background: #fef9c3; box-shadow: 0 4px 12px rgba(0,0,0,0.08);">
    <h3 style="margin-top:0; color:#a16207;">Filesystem</h3>
    <ul style="margin:0; padding-left:1.2em;">
      <li><strong>VFS abstraction layer</strong> + FAT12 backend</li>
      <li>Full CRUD, kinda: <code>open</code>, <code>read</code>, <code>write</code>, <code>create</code>, <code>remove</code>, <code>mkdir</code>, <code>readdir</code></li>
    </ul>
  </div>

  <div style="flex: 1 1 280px; border: 2px solid #b91c1c; border-radius: 16px; padding: 1.2em; background: #fee2e2; box-shadow: 0 4px 12px rgba(0,0,0,0.08);">
    <h3 style="margin-top:0; color:#991b1b;">User & Auth</h3>
    <ul style="margin:0; padding-left:1.2em;">
      <li><strong>User accounts</strong> with admin and normal flags, individual home directories</li>
      <li><strong>Login</strong> with 3-attempt lockout (15 s cooldown) (no roast, trust me)</li>
      <li><strong>Hashing ans stuff</strong>: DJB2 double-pass, salted with username + timer + knuth shuffle</li>
    </ul>
    <p style="margin: 0.5em 0 0 0; font-style: italic; background: #fecaca; padding: 0.3em 0.6em; border-radius: 8px; font-weight: bold;">IMPORTANT: first boot wizard creates the initial admin</p>
  </div>

  <div style="flex: 1 1 280px; border: 2px solid #16a34a; border-radius: 16px; padding: 1.2em; background: #dcfce7; box-shadow: 0 4px 12px rgba(0,0,0,0.08);">
    <h3 style="margin-top:0; color:#166534;">Fitness & Health (might be first time you see this in a github repo lol)</h3>
    <ul style="margin:0; padding-left:1.2em;">
      <li><strong>Mandatory Workout</strong>: 40s pushups - 10s rest - 50s squats (can't skip this -_-)</li>
      <li>Custom excercise times, like you can make your own workout (<code>exercise BURPEE 30</code>)</li>
      <li><strong>Nutrition macors</strong>: <code>protein</code>, <code>fat</code>, <code>carbs</code> calculators</li>
      <li>Wootaah reminder (default 1.5 hours water reminder)</li>
      <li>Workout stats & motivational quotes</li>
    </ul>
  </div>

  <div style="flex: 1 1 280px; border: 2px solid #7c3aed; border-radius: 16px; padding: 1.2em; background: #f3e8ff; box-shadow: 0 4px 12px rgba(0,0,0,0.08);">
    <h3 style="margin-top:0; color:#6d28d9;">UI & shell</h3>
    <ul style="margin:0; padding-left:1.2em;">
      <li><strong>Status bar</strong>: shows uptime + timed message on protected rows</li>
      <li><strong>Scrollback buffer</strong>: HEY, <code>Ctrl+UP/Dwn</code> or <code>pg up/down</code></li>
      <li><strong>Interactive shells</strong>: prompt <code>user@gymbro:/cwd&gt;</code> and 25 builtin commands</li>
    </ul>
  </div>

  <div style="flex: 1 1 280px; border: 2px solid #db2777; border-radius: 16px; padding: 1.2em; background: #fce7f3; box-shadow: 0 4px 12px rgba(0,0,0,0.08);">
    <h3 style="margin-top:0; color:#be185d;">Vi-like Editor</h3>
    <ul style="margin:0; padding-left:1.2em;">
      <li>Modal editor: Normal, Insert, Command, Search</li>
      <li><code>hjkl</code>, word motions, <code>0</code>/<code>$</code>, <code>gg</code>/<code>G</code></li>
      <li>Undo, search (<code>/term</code> + <code>n</code>), <code>:w</code> / <code>:q</code>/ <code>:wq</code> / <code>:q!</code></li>
      <li>Direct VGA writes, no terminal emulations lol</li>
    </ul>
  </div>

</div>

## Build & Run

### Prerequisites
- `i686-elf-gcc` (cross-compiler)
- `i686-elf-ld`, `i686-elf-objcopy`
- `nasm`
- `qemu-system-i386`
- `python3`, `make`

### Commands

| Command | Effect |
|:---|:---|
| `make`         |  Build `mros.img` |
| `make run`     |  Useless, runs in text mode |
| `make run-gui` |  Run in QEMU **with PC speaker audio** (PulseAudio) |
| `make clean`   |  Remove all build artefacts |

## Preview

### Video
![video;)](https://cdn.hackclub.com/019de468-4788-7ca3-8986-6a4c7b494650/screenrecording-2026-05-01_22-08-23%20(online-video-cutter.com).mp4)

### Static videos
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
