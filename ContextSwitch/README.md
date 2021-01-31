# Assignment 2 - ContextSwitch
## [EMBSYS 320 Winter 2021](/../../)
#### Carl B. Smiley

### Description
Implemented `TaskSwitch` context switching within `switch.s`.
Also implemented `testAndSet` using `LDREX` and `STREX` (also in `switch.s`).

```C
uint32_t gLock = 0;
uint32_t testAndSet(uint32_t *lock);`
```

```C
// acquire lock
while (testAndSet(&gLock) != 0); // spin

// critical section
// ...

// release lock
gLock = 0;
```