#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "lexer.h"
#include "parser.h"

int main(int argc, char *argv[])
{
    Lexer lex;
    string line, source, print_source;
    ifstream in(argv[1]);

    source.reserve(4096);
    print_source.reserve(4096);
    int lnum = 1;
    while (in.good())
    {
        getline(in, line);
        if (lnum < 10)
        {
            print_source += ' ';
        }
        print_source += to_string(lnum++);
        source += ' ', print_source += ' ';
        source += line, print_source += line;
        source += '\n', print_source += '\n';
    }

    cout << "-------------- source --------------" << endl
         << print_source << endl;
    // remove comment
    bool incomment = false;
    const char *end = source.data() + source.size();
    char *ptr = const_cast<char *>(source.data());
    while (ptr != end)
    {
        if (*ptr == '/')
        {
            if (*(ptr + 1) == '/')
            {
                *ptr++ = ' ';
                *ptr++ = ' ';
                while (ptr != end && *ptr != '\n') *ptr++ = ' ';
                if (ptr != end) ++ptr;
            }
            else if (*(ptr + 1) == '*')
            {
                *ptr++ = ' ';
                *ptr++ = ' ';
                while (ptr != end && *ptr != '*' && *(ptr + 1) != '/') *ptr++ = ' ';
                if (ptr != end)
                {
                    *ptr++ = ' ';
                    *ptr++ = ' ';
                }
            }
            else ++ptr;
        }
        else ++ptr;
    }

    cout << "-------------- lexer --------------" << endl;
    StringBuf sb(source.data(), source.size());
    lex.tokenize(sb);
    // lex.debugPrint();

    cout << "-------------- parser --------------" << endl;
    Parser p;
    p.parse(lex);

    // system("pause");
    return 0;
}
