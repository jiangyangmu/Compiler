#pragma once

#include "type.h"

bool uadd_ok(unsigned int u1, unsigned int u2);
bool usub_ok(unsigned int u1, unsigned int u2);
bool umul_ok(unsigned int u, unsigned int v);
bool udiv_ok(unsigned int u1, unsigned int u2);
bool umod_ok(unsigned int u1, unsigned int u2);

bool tadd_ok(int i1, int i2);
bool tsub_ok(int i1, int i2);
int tmul_ok(int i1, int i2);
int tdiv_ok(int i1, int i2);
int tmod_ok(int i1, int i2);
// warning-list: compare unsigned int with int

// void enum_to_int(EnumType *et, IntType *it);
// void int_to_enum(IntType *it, EnumType *et);

TypeBase *CommonType(TypeBase *left, TypeBase *right);
// void promotion(IntType, FloatType);
// void promotion(FloatType, IntType);
// void promotion(FloatType, FloatType);

// void int_to_int(IntType, IntType);
// void int_to_float(IntType, FloatType);

// void float_to_float(FloatType, FloatType);
// void float_to_int(FloatType, IntType);
