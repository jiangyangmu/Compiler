#pragma once

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

