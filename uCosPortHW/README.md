# Assignment 3 - uCosPortHW
## [EMBSYS 320 Winter 2021](/../../)
#### Carl B. Smiley

### Description
Implemented the following assembly functions for uCOS/II port:
- `OS_CPU_SR_Save` - saves `PRIMASK` when entering critical sections
- `OS_CPU_SR_Restore` - restores `PRIMASK` on exiting critical sections
- `ContextSwitch` - handles context switch using uCOS/II global vars

### Example Output
```
main: Running OSInit()...
main: Creating start up task.
Starting multi-tasking.
StartupTask: Begin
StartupTask: Starting timer tick
HCLK frequency = 16 MHz
Configured ticksPerSec = 100
StartupTask: Creating the 3 tasks
StartupTask: deleting self
TaskOne: starting
TaskTwo: starting
TaskThree: starting
TaskThree: count = 0
TaskThree: count = 1
TaskThree: count = 2
TaskThree: count = 3
TaskThree: count = 4
TaskThree: count = 5
TaskThree: count = 6
TaskThree: count = 7
TaskThree: count = 8
TaskThree: count = 9
TaskThree: count = 10
TaskThree: count = 11
TaskThree: count = 12
TaskThree: count = 13
TaskThree: count = 14
TaskThree: count = 15
TaskThree: count = 16
TaskThree: count = 17
TaskThree: count = 18
TaskThree: count = 19
TaskOne: Turn LED On
TaskThree: count = 20
TaskThree: count = 21
TaskThree: count = 22
TaskThree: count = 23
TaskThree: count = 24
TaskThree: count = 25
TaskThree: count = 26
TaskThree: count = 27
TaskThree: count = 28
TaskThree: count = 29
TaaskTwo: Turn LED OFF
skThree: count = 30
TaskThree: count = 31
TaskThree: count = 32
```