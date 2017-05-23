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
// TypeBase *BooleanConversion(const TypeBase *t);
// const TypeBase *IntegerConversion(const TypeBase *from, const TypeBase *to);
// TypeBase *PointerConversion(const TypeBase *from, const TypeBase *to);
// FloatToInteger
// IntegerToFloat
// FloatToFloat

const TypeBase *AssignmentConversion(const TypeBase *from, const TypeBase *to);
// TypeBase * DefaultArgumentPromotion(const TypeBase *param); // for variadic
// function
const TypeBase *UsualArithmeticConversion(const TypeBase *left,
                                    const TypeBase *right);
const TypeBase *IntegerPromotion(const TypeBase *t);
// TypeBase *LvalueConversion(const TypeBase *t, const Environment *env);

const TypeBase *CondResultConversion(const TypeBase *left, const TypeBase *right);

