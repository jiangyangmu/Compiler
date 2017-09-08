#pragma once

#include "type.h"

// constant arithmetic checking
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
// Type *BooleanConversion(const Type *t);
// const Type *IntegerConversion(const Type *from, const Type *to);
// Type *PointerConversion(const Type *from, const Type *to);
// FloatToInteger
// IntegerToFloat
// FloatToFloat

const Type *AssignmentConversion(const Type *from, const Type *to);
// Type * DefaultArgumentPromotion(const Type *param); // for variadic
// function
const Type *UsualArithmeticConversion(const Type *left,
                                    const Type *right);
const Type *IntegerPromotion(const Type *t);
// Type *LvalueConversion(const Type *t, const Environment *env);

const Type *CondResultConversion(const Type *left, const Type *right);

