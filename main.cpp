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
	string line, source;
	ifstream in(argv[1]);

	/*int a, a2;
	int b;
	b = 3 + (a = 2, a2 = 3);
	(a > a2 ? a : a2) = 100;
	(1 > 2 ? a = 1 : a = 2);
	a = 1, 2, 3, 4;*/

	source.reserve(4096);
	while (in.good())
	{
		getline(in, line);
		source += line;
		source += '\n';
	}
	cout << "-------------- source --------------" << endl
        << source << endl;

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
