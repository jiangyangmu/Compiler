#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include <unistd.h>

#include "lexer.h"
#include "parser.h"
#include "codegen.h"

static bool lflag = false; // show Lexer output
static bool sflag = false; // show Syntax output
static bool eflag = false; // show Environment output

void options(int &argc, char ** &argv)
{
    int ch;
    while ((ch = getopt(argc, argv, "lse")) != -1) {
        switch (ch) {
            case 'l':
                lflag = true;
                break;
            case 's':
                sflag = true;
                break;
            case 'e':
                eflag = true;
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;
}

int main(int argc, char *argv[])
{
    options(argc, argv);

    if (argc == 0)
    {
        cout << "No input file." << endl;
        return 1;
    }

    Lexer lex;
    string line, source, print_source;
    ifstream in(argv[0]);

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

    if (lflag)
    {
        cout << "-------------- source --------------" << endl;
        cout << print_source << endl;
    }
    // remove comment
    // bool incomment = false;
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

    if (lflag) cout << "-------------- lexer --------------" << endl;
    StringBuf sb(source.data(), source.size());
    lex.tokenize(sb);
    if (lflag)
    {
        lex.debugPrint();
        return 0;
    }

    if (sflag || eflag) cout << "-------------- parser --------------" << endl;
    Parser p(lex);
    p.parse();
    if (sflag)
        p.DebugPrintSyntaxTree();
    if (eflag)
        p.DebugPrintEnvironment();

    // cout << "-------------- code --------------" << endl;
    p.emit();
    // cout << Emitted() << endl;

    // system("pause");
    return 0;
}
