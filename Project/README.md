# Project - MP3 Player
## [EMBSYS 320 Winter 2021](/../../)
#### Carl B. Smiley
---

# Description
Final project for EMBSYS 320 implements an MP3 Player using the STM32L4 Discovery Board.

## Youtube Presentation:
[![A video describing the features of the project](thumbnail.gif)](https://youtu.be/5PDZ1TZa48s)

# Features
## UI Icons
The *UI Icons* are loaded from the SD card and stored in a uCOS memory partition.
They are stored as Netbpm .pgm files with 8-bit depth and up to 64x64 pixels.

## Song Details
The *Song Details* are read from the ID3 tags of the .mp3 files stored on the SD card.
The tags that this program examines are the *Title*, *Artist*, and *Album*.
If any of these fields are missing, a default text will be displayed such as *"Unknown Album"*.

## Playback Controls
The *Playback Controls* include the *Previous Song Button*, the *Play/Pause Button* and the *Next Song Button*.
Each button sends a corresponding command to the internal streaming task.

## Song Progress
Song progress is tracked while a song is playing and song duration is precalculated as the file size divided by 192kbps.

# RTOS Tasks
![Diagram of RTOS tasks and interfaces](diagram.png)
## Startup Task
### Priority 4
Initializes global uCOS constructs (queues, mailboxes, memory partitions), loads bitmaps from SD card, and launches other tasks.

## Input Task
### Priority 6
Polls the touch controller and implements state machine to generate onTouch and onRelease events to the `queue<Event>`.

## Display Task
### Priority 7
Displays data read in from the `mbox<Song>` and `mbox<Progress>`. Also reads `queue<Event>` to determine if buttons are pressed and writes to `queue<Command>` if buttons are pressed.

## Stream Task
### Priority 5
Reads the `queue<Command>` to handle play/pause and advance the current song. Updates the `mbox<Song>` whenever current song changes. Updates `song<Progress>` every second while a song is playing.

# Data Structures
## Event
```c++
struct Event {
    enum Type { TOUCH, RELEASE };

    Type type;
    int x,y;
}
```
## Command
```c++
enum class Command { PREVIOUS, PLAYPAUSE, NEXT };
```
## Song
```c++
struct Song {
    struct Info {
       char tag[3];
       char title[30];
       char artist[30];
       char album[30];
       char year[4];
       char comment[30];
       unsigned char genre;
    };

    Info info;
    int duration;
    std::string filename;
};
```
## Progress
```c++
using Progress = int;
```

# Compiling
Use IAR to compile this project. I had to enable many optimizations to get the code size to fit within the limitations of the free IAR license.
