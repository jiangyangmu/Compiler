#include "../Source/Testing/Tester.h"

/*
#include "../Source/ir/Ast.h"

class AstTest : public Tester
{
public:
    virtual void setUp() {}
    
    virtual void shutDown()
    {
        arrayOfTokenArray.clear();
        for (Ast * n : astNodes)
        {
            delete n;
            n = nullptr;
        }
        astNodes.clear();
    }

    // Test ast
    Ast * BuildAst(StringRef source)
    {
        arrayOfTokenArray.emplace_back(Tokenize(source));
        return ParseTranslationUnit(TokenIterator(arrayOfTokenArray.back()));
    }

    // Control ast
    Ast * N(AstType type,
            Ast * leftChild = nullptr,
            Ast * rightSibling = nullptr)
    {
        Ast * node = new Ast;
        node->type = type;
        node->parent = nullptr;
        node->leftChild = leftChild;
        if (leftChild) leftChild->parent = node;
        node->rightSibling = rightSibling;
        if (rightSibling) rightSibling->parent = node;
        astNodes.push_back(node);
        return node;
    }

    bool Verify(StringRef source, Ast * controlAst)
    {
        Ast * testAst = BuildAst(source);
        
        bool isEqual = IsEqualAst(controlAst, testAst);
        if (!isEqual)
        {
            std::cout << "Expect:" << std::endl;
            DebugPrintAst(controlAst);
            std::cout << "Actual:" << std::endl;
            DebugPrintAst(testAst);
        }

        return isEqual;
    }

private:
    std::vector<std::vector<Token>> arrayOfTokenArray;
    std::vector<Ast *> astNodes;
};

TEST_F(AstTest, Basic)
{
    EXPECT_EQ(true, Verify(
        "",
        N(TRANSLATION_UNIT)
    ));
}
*/
