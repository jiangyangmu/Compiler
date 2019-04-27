// playground.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

enum E { EC1, EC2 };
void PositiveNegativeZero()
{
    // float f = 1.2.3;
    char c = '0';
    // E e = E::EC1;
    int EC1 = 2;

    double pz = 0.0;
    double nz = 0x8000000000000000; // -0.0 will be changed to 0.0

    long *pz_value = (long *)(&pz);
    long *nz_value = (long *)(&nz);

    if (!pz)
    {
        std::cout << "positive zero == 0" << std::endl;
    }
    if (!nz)
    {
        std::cout << "negative zero == 0" << std::endl;
    }
    if (pz == nz)
    {
        std::cout << "positive zero == negative zero" << std::endl;
    }
    std::cout << "pz = " << *pz_value << ", nz = " << *nz_value << std::endl;
}

class Base
{
public:
    virtual void f() { std::cout << "base::f" << std::endl; }
};
class Derived : public Base
{
protected:
    virtual void f() { std::cout << "derived::f" << std::endl; }
};
void VirtualAndAccessControl()
{
    Derived d;
    Base * b = &d;
    b->f(); // derived::f
}

static int ival = 3;
void bar(int ival)
{
    {
        int ival = 4;
        {
            extern int ival;
            std::cout << "ival = " << ival << std::endl;
        }
    }
}

int main()
{
    bar(1);
    return 0;
}
