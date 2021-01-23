#include <stdint.h>
#include "print.h"

/*
 * A structure to easily dereference parts of the stack frame.
 * This implementation is brittle. It assumes STM32L475 with no FPU.
 */
struct stack_frame_t {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
};

/*
 *
 * Part of a fault exception handler. Prints the given register values.
 * pc: the value of the program counter when the fault occurred.
 * lr: the value of the link register when the fault occurred.
 *
 */
void FaultPrint(struct stack_frame_t *sf)
{
    // Print an error message specifying the PC and LR values when the fault occurred
    PrintString("Hard fault at PC=0x");
    PrintHex(sf->pc);
    PrintString(" LR=0x");
    PrintHex(sf->lr);
    PrintString("\n");

    // The entire stack frame consists of R0-R3, R12, LR, PC, PSR
    PrintString("  Full Stack Frame:\n");
    PrintString("    R0  0x"); PrintHex(sf->r0);  PrintString("\n");
    PrintString("    R1  0x"); PrintHex(sf->r1);  PrintString("\n");
    PrintString("    R2  0x"); PrintHex(sf->r2);  PrintString("\n");
    PrintString("    R3  0x"); PrintHex(sf->r3);  PrintString("\n");
    PrintString("    R12 0x"); PrintHex(sf->r12); PrintString("\n");
    PrintString("    LR  0x"); PrintHex(sf->lr);  PrintString("\n");
    PrintString("    PC  0x"); PrintHex(sf->pc);  PrintString("\n");
    PrintString("    PSR 0x"); PrintHex(sf->psr); PrintString("\n");
}
