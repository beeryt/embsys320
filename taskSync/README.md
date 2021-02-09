# Assignment 3 - uCosPortHW
## [EMBSYS 320 Winter 2021](/../../)
#### Carl B. Smiley

### Description
Implemented task synchronization using the following uCOS/II APIs
#### Semaphores
- `OSSemCreate`
- `OSSemPend`
- `OSSemPost`
#### Queues
- `OSQCreate`
- `OSQPend`
- `OSQPost`
#### Event Flags
- `OSFlagCreate`
- `OSFlagPend`
- `OSFlagPost`
#### Memory Management
- `OSMemCreate`

### Example Output
```
uCOS task synchronization demo: Built Feb  8 2021 18:20:34.

main: Running OSInit()...
main: Creating start up task.
Starting multi-tasking.
StartupTask: Begin
StartupTask: Starting timer tick
HCLK frequency = 16 MHz
Configured ticksPerSec = 1000
StartupTask: Creating application tasks
StartupTask: deleting self
TaskMBTx: starting
TaskMBRxA: starting
TaskMBRxA: actual=A0, expected=A0, received=1 errors=0
TaskMBRxB: starting
TaskQTxA: starting
TaskQTxB: starting
TaskQTxA: sent msg A0
TaskQRx: starting
TaskQTxB: sent msg B0
TaskQRx: received msg A0
TaskRxFlags: starting
TaskQRx: received msg B0
TaskQTxB: sent msg B1
TaskMBRxB: actual=B0, expected=B0, received=1 errors=0
TaskQRx: received msg B1
TaskRxFlags: (TaskMBRxA_msgCount expected=1 actual=1) (TaskMBRxB_msgCount expected=1 actual=1)
TaskQTxA: sent msg A1
TaskQRx: received msg A1
TaskQTxB: sent msg B2
TaskQRx: received msg B2
TaskMBRxA: actual=A1, expected=A1, received=2 errors=0
TaskQTxB: sent msg B3
TaskQRx: received msg B3
TaskQTxB: sent msg B4
TaskQTxA: sent msg A2
TaskQRx: received msg B4
TaskQRx: received msg A2
TaskMBRxB: actual=B1, expected=B1, received=2 errors=0
TaskQTxB: Done! Sent 5 messages
TaskRxFlags: (TaskMBRxA_msgCount expected=2 actual=2) (TaskMBRxB_msgCount expected=2 actual=2)
TaskQTxA: sent msg A3
TaskQRx: received msg A3
TaskMBRxA: actual=A2, expected=A2, received=3 errors=0
TaskQTxA: sent msg A4
TaskMBRxB: actual=B2, expected=B2, received=3 errors=0
TaskQRx: received msg A4
TaskRxFlags: (TaskMBRxA_msgCount expected=3 actual=3) (TaskMBRxB_msgCount expected=3 actual=3)
TaskQRx: Done! Received 10/10 messages.
TaskMBRxA: actual=A3, expected=A3, received=4 errors=0
TaskQTxA: Done! Sent 5 messages
TaskMBRxB: actual=B3, expected=B3, received=4 errors=0
TaskRxFlags: (TaskMBRxA_msgCount expected=4 actual=4) (TaskMBRxB_msgCount expected=4 actual=4)
TaskMBRxA: actual=A4, expected=A4, received=5 errors=0
TaskMBRxA: Done! Received 5 messages, errors=0
TaskMBRxB: actual=B4, expected=B4, received=5 errors=0
TaskRxFlags: (TaskMBRxA_msgCount expected=5 actual=5) (TaskMBRxB_msgCount expected=5 actual=5)
TaskMBRxB: Done! received 5 messages, errors=0
TaskRxFlags: Done! (TaskMBRxA_msgCount expected=5 actual=5) (TaskMBRxB_msgCount expected=5 actual=5)
TaskMBTx: Done! Sent 10 messages
```