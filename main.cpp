#include <string>
#include <vector>
#include <deque>
#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;

#include "lexer.h"
#include "parser.h"
//#include "Statement.h"

/*
class Parser
{
public:
	void parse(Lexer &lex)
	{
		Stmt *stmt = Stmt::buildTree(lex);
		stmt->execute();
	}
};
*/

int main(int argc, char *argv[])
{
	Lexer lex;
	string line, source, print_source;
	ifstream in(argv[1]);

	/*int a, a2;
	int b;
	b = 3 + (a = 2, a2 = 3);
	(a > a2 ? a : a2) = 100;
	(1 > 2 ? a = 1 : a = 2);
	a = 1, 2, 3, 4;*/

	source.reserve(4096);
	print_source.reserve(4096);
    int lnum = 1;
	while (in.good())
	{
		getline(in, line);
        if (lnum < 10) print_source += ' ';
        print_source += to_string(lnum++);
        source += ' '; print_source += ' ';
		source += line; print_source += line;
		source += '\n'; print_source += '\n';
	}
	cout << "-------------- source --------------" << endl
        << print_source << endl;

	cout << "-------------- lexer --------------" << endl;
    StringBuf sb(source.data(), source.size());
	lex.tokenize(sb);
    while (lex.hasNext())
    {
        Token t = lex.peakNext();
        cout << "Token: " << t;
        if (t.type == SYMBOL) cout << ':' << lex.symbolName(t.symid);
        cout << endl;
        lex.getNext();
    }

	cout << "-------------- parser --------------" << endl;
    Parser p;
    p.parse(lex);

	//system("pause");
	return 0;
}
