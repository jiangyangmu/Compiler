#pragma once

// C Type => x64 Type => x64 asm
// C Visibility => x64 Visibility => x64 asm
// which segment to reside?
class X64Data
{
    // x64Type
};

// C Function => x64 asm code
class X64Routine
{
    std::string name;
    std::vector<X64Location> parameters;
    std::vector<X64Instruction> instructions;
};

struct X64Instruction
{
    // opcode (int)
    // inst-param (X64Location)
};

// memory, general-registers, sse, x87, imm
struct X64Location
{
};
