# Assignment 1 - uDebugger
## [EMBSYS 320 Winter 2021](/../../)
#### Carl B. Smiley

### Description
I modified the `FaultPrint` signature as follows to easily dereference all values from the stack frame.

```C
void FaultPrint(struct stack_frame_t *sf);
```

This had the additional benefit of simplifying `HardFaultIrqHandler` which no longer needs to extract values from the stack frame.

### Example Output
```
Hard fault at PC=0x0800B8A LR=0x0800BAB
  Full Stack Frame:
    R0  0x00000001
    R1  0x00000001
    R2  0x0000000D
    R3  0xE000ED26
    R12 0x00000000
    LR  0x08000BAB
    PC  0x08000B8A
    PSR 0x81000000
```

