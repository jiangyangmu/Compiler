#include "parser_api.h"

#include <iostream>

#define TOKEN(s) PRODUCTION(TokenFromString(s))

// clang-format off
int main(int argc, char * argv[])
{
    GM_BEGIN(G);
    GM_ADD(G, declaration);

    GM_ADD(G, declaration_specifiers);
    GM_ADD(G, storage_class_specifier);
    GM_ADD(G, type_specifier);
    GM_ADD(G, type_qualifier);

    GM_ADD(G, declarator);

    static std::vector<std::string> decl;

    declaration =
          declaration_specifiers &
          declarator & GM_CODE({ for (auto d : decl) std::cout << d << std::endl; }) &
          TOKEN(";")
        ;
    declaration_specifiers =
          (storage_class_specifier | type_specifier | type_qualifier) &
          *(storage_class_specifier | type_specifier | type_qualifier)
        ;
    storage_class_specifier =
          TOKEN("typedef")  & GM_CODE({ decl.push_back("typedef"); })
        | TOKEN("extern")   & GM_CODE({ decl.push_back("extern"); })
        | TOKEN("static")   & GM_CODE({ decl.push_back("static"); })
        | TOKEN("auto")     & GM_CODE({ decl.push_back("auto"); })
        | TOKEN("register") & GM_CODE({ decl.push_back("register"); })
        ;
    type_specifier =
          TOKEN("int")      & GM_CODE({ decl.push_back("int"); })
        | TOKEN("float")    & GM_CODE({ decl.push_back("float"); })
        | TOKEN("char")     & GM_CODE({ decl.push_back("char"); })
        ;
    type_qualifier =
          TOKEN("const")
        | TOKEN("volatile")
        ;
    declarator =
          TOKEN("id")       & GM_CODE({ decl.push_back("id"); })
        ;

    GM_END(G);

    SourceSanner scanner("extern int typedef float char test;");
    Tokenizer tokenizer;
    tokenizer.compile(scanner);
    TokenIterator tokens = tokenizer.getIterator();
    GM_RUN(G, tokens);

    return 0;
}
// clang-format on
