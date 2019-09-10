#pragma once

#include <iostream>
using namespace std;

#define SyntaxWarning(msg)                                      \
    do                                                          \
    {                                                           \
        cerr << "Warning: " << msg << " at " << __FILE__ << ':' \
             << to_string(__LINE__) << endl;                    \
    } while (0)

#ifdef HAS_LEXER
#define SyntaxWarningEx(msg)                                         \
    do                                                               \
    {                                                                \
        cerr << "Warning: " << msg << " at " << __FILE__ << ':'      \
             << to_string(__LINE__) << '\t' << "'" << lex.peakNext() \
             << "' at " << lex.peakNext().line << endl;              \
    } while (0)
#else
#define SyntaxWarningEx(msg) SyntaxWarning(msg)
#endif

#define SyntaxError(msg)                                       \
    do                                                         \
    {                                                          \
        cerr << "Syntax: " << msg << " at " << __FILE__ << ':' \
             << to_string(__LINE__) << endl;                   \
        std::exit(EXIT_FAILURE);                               \
    } while (0)

#ifdef HAS_LEXER
#define SyntaxErrorEx(msg)                                           \
    do                                                               \
    {                                                                \
        cerr << "Syntax: " << msg << " at " << __FILE__ << ':'       \
             << to_string(__LINE__) << '\t' << "'" << lex.peakNext() \
             << "' at " << lex.peakNext().line << endl;              \
        std::exit(EXIT_FAILURE);                                     \
    } while (0)
#else
#define SyntaxErrorEx(msg) SyntaxError(msg)
#endif

#define SyntaxErrorDebug(msg)                                     \
    do                                                            \
    {                                                             \
        cerr << "Syntax: " << msg << " at " << __FILE__ << ':'    \
             << to_string(__LINE__) << endl;                      \
        string s;                                                 \
        while (cin >> s && lex.hasNext())                         \
        {                                                         \
            if (s == "n")                                         \
            {                                                     \
                cerr << "Token " << lex.peakNext() << " at line " \
                     << lex.peakNext().line << endl;              \
                lex.getNext();                                    \
            }                                                     \
            else                                                  \
            {                                                     \
                break;                                            \
            }                                                     \
        }                                                         \
        std::exit(EXIT_FAILURE);                                  \
    } while (0)

#define EXPECT(token_type)                                                    \
    do                                                                        \
    {                                                                         \
        if (!lex.hasNext() || lex.peakNext().type != (token_type))            \
        {                                                                     \
            cerr << "Syntax: Expect " << Token::DebugTokenType(token_type)    \
                 << ", Actual "                                               \
                 << Token::DebugTokenType(lex.hasNext() ? lex.peakNext().type \
                                                        : NONE)               \
                 << " at " << __FILE__ << ':' << to_string(__LINE__) << endl; \
            std::exit(EXIT_FAILURE);                                          \
        }                                                                     \
        lex.getNext();                                                        \
    } while (0)

#define EXPECT_GET(token_type)                                                \
    ((!lex.hasNext() || lex.peakNext().type != (token_type))                  \
         ? (cerr << "Syntax: Expect " << Token::DebugTokenType(token_type)    \
                 << ", Actual "                                               \
                 << Token::DebugTokenType(lex.hasNext() ? lex.peakNext().type \
                                                        : NONE)               \
                 << " at " << __FILE__ << ':' << to_string(__LINE__) << endl, \
            assert(false), Token())                                           \
         : lex.getNext())

#define SKIP(token_type)                                     \
    (lex.hasNext() && lex.peakNext().type == (token_type) && \
     lex.getNext().type == (token_type))

#define EXPECT_TYPE_IS(ptr, type_)                                            \
    do                                                                        \
    {                                                                         \
        if ((ptr) == nullptr || (ptr)->type() != (type_))                     \
            SyntaxWarningEx("Expect '" + TypeBase::DebugTypeClass(type_) +    \
                            "', but get '" + TypeBase::DebugType(ptr) + "'"); \
    } while (0)

#define EXPECT_TYPE_WITH(ptr, op)                                 \
    do                                                            \
    {                                                             \
        if ((ptr) == nullptr || !(ptr)->hasOperation(op))         \
            SyntaxWarningEx("Type '" + TypeBase::DebugType(ptr) + \
                            "' don't support " #op);              \
    } while (0)
