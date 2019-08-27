#include "WinMem.h"
#include "CxxException.h"
#include "Lexer.h"

int main(void)
{
    //Test_CxxISO_ErrorHandling();
    Test_ConvertOnePattern();
    Test_ConvertOnePattern2();
    Test_ConvertMultiplePattern();
    Test_ConvertMultiplePattern2();
    Test_ConvertMultiplePattern3();
    Test_CLex();
    Test_Compile("a+");
    Test_Compile("a|b");
    Test_Compile("(a|b)+");
    return 0;
}
