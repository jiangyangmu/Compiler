#include <string>
#include <vector>
#include <deque>
#include <iostream>
#include <sstream>
#include <fstream>
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

    cout << "-------------- lexer --------------" << endl;
    StringBuf sb(source.data(), source.size());
    lex.tokenize(sb);
    lex.debugPrint();

    cout << "-------------- parser --------------" << endl;
    Parser p;
    p.parse(lex);

    // system("pause");
    return 0;
}
