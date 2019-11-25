#pragma once

#include "../Base/Containers.h"
#include "../Base/String.h"
#include "../Memory/FreeListAllocator.h"

#include <memory>

// TODO: RegexStruct, RegexStructFactory, Element is res

namespace v2 {

namespace re {

// Regex structure

class RegexStruct
{
public:
    class Element;
    class Visitor;

public:
    RegexStruct(const RegexStruct &) = delete;
    RegexStruct(RegexStruct &&);
    ~RegexStruct();

    void Accept(Visitor & visitor);

private:
    struct Node
    {
        Node * pLeft;
        Node * pRight;
        Element * pElement;
    };
    Node * pRoot;

    explicit RegexStruct(Node * pNode) : pRoot(pNode) {}
    static void FreeRegexStruct(Node * pNode);
    static void VisitRegexStruct(const Node * pNode, Visitor & visitor);
    friend class RegexStructFactory;
};

class RegexStructFactory
{
public:
    // Build manually
    class PostfixBuilder
    {
    public:
        PostfixBuilder & Ascii(int ch);
        PostfixBuilder & Concat();
        PostfixBuilder & Alter();
        PostfixBuilder & KleeneStar();
        RegexStruct Build();
    private:
        containers::Array<RegexStruct::Node *> stack;
    };
    static PostfixBuilder CreateBuilder();
    
    // Build from string
    static RegexStruct CreateFromString(StringView svRegex);

private:
    static RegexStruct::Node * NewAsciiNode(int ch);
    static RegexStruct::Node * NewConcatNode(RegexStruct::Node * pLeft, RegexStruct::Node * pRight);
    static RegexStruct::Node * NewAlterNode(RegexStruct::Node * pLeft, RegexStruct::Node * pRight);
    static RegexStruct::Node * NewKleeneStarNode(RegexStruct::Node * pNode);
};

// Regex handle

class Regex
{
public:
    class Impl;

public:
    Regex(RegexStruct rs);

private:
    std::unique_ptr<Impl> pImpl;

    friend class PImplAccessor;
};

// Regex match

class MatchResult
{
public:
    MatchResult(String content, int maxScanLen, int maxAcceptLen);

    bool IsValid() const;
    StringView Content() const;
    int MaxScanLen() const;
    int MaxAcceptLen() const;

private:
    String content;
    int maxScanLen;
    int maxAcceptLen;
};

class MatchResultIterator
{
public:
    bool More();
    MatchResult Next();

    // Error handling.
    int IgnoreCharacter();

    class Impl;
private:
    std::unique_ptr<Impl> pImpl;

    friend class PImplAccessor;
};

MatchResultIterator IterateMatches(const Regex & regex, StringView input);

}

}
