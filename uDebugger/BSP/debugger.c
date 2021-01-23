#include <stdint.h>
#include "print.h"

/*
 *
 * Part of a fault exception handler. Prints the given register values.
 * pc: the value of the program counter when the fault occurred.
 * lr: the value of the link register when the fault occurred.
 *
 */
void FaultPrint(uint32_t pc, uint32_t lr)
{
    // Print an error message specifying the PC and LR values when the fault occurred
    PrintString("Hard fault at PC=0x");
    PrintHex(pc);
    PrintString(" LR=0x");
    PrintHex(lr);
    PrintString("\n");
    // TODO: print R0-R3, R12, PSR
}
