#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>

#define ELEMENT_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#include "util/String.h"

template <typename T>
class TreeLike
{
    T *parent_;
    std::vector<T *> children;

   public:
    TreeLike<T>() : parent_(nullptr) {}
    virtual ~TreeLike<T>() {}

    bool isRoot() const
    {
        return parent_ == nullptr;
    }
    bool isLeaf() const
    {
        return children.empty();
    }

    void setParent(T *p)
    {
        parent_ = p;
        if (p)
            p->children.push_back(dynamic_cast<T *>(this));
    }
    void addChild(T *c)
    {
        assert(c != nullptr);
        c->parent_ = dynamic_cast<T *>(this);
        children.push_back(c);
    }
    void replaceChild(size_t i, T *c)
    {
        assert(i < children.size() && c != nullptr);
        children[i] = c;
    }

    // TODO: s/parent/getParent
    T *parent() const
    {
        return parent_;
    }
    T *leftSibling() const
    {
        if (isRoot())
            return nullptr;
        size_t me = 0;
        for (auto child : children)
        {
            if (child == this)
                break;
            else
                ++me;
        }
        if (me > 0 && me < children.size() && children[me] == this)
            return children[me - 1];
        else
            return nullptr;
    }
    T *rightSibling() const
    {
        if (isRoot())
            return nullptr;
        size_t me = 0;
        for (auto child : children)
        {
            if (child == this)
                break;
            else
                ++me;
        }
        if (me + 1 < children.size() && children[me] == this)
            return children[me + 1];
        else
            return nullptr;
    }
    std::vector<T *> const &getChildren() const
    {
        return children;
    }
    size_t getChildrenCount() const
    {
        return children.size();
    }
    T *getFirstChild() const
    {
        assert(!children.empty());
        return children.front();
    }
    T *getLastChild() const
    {
        assert(!children.empty());
        return children.back();
    }
    T *getChild(size_t i) const
    {
        assert(i < children.size());
        return children[i];
    }

    std::vector<T *> const &all() const
    {
        return children;
    }
    size_t count() const
    {
        return children.size();
    }
    T *first() const
    {
        assert(!children.empty());
        return children.front();
    }
    T *last() const
    {
        assert(!children.empty());
        return children.back();
    }
    T *at(size_t i) const
    {
        assert(i < children.size());
        return children[i];
    }

    // traversal
    // TODO: need parameter passing
    // virtual void onVisit() {}
    // static void PostOrderTraversal(T *node)
    // {
    //     assert(node != nullptr);
    //     for (T *child : node->children)
    //     {
    //         PostOrderTraversal(child);
    //     }
    //     node->onVisit();
    // }
};

template <typename T>
class ListLike
{
    T *next_, *prev_;

   public:
    ListLike<T>() : next_(nullptr), prev_(nullptr) {}
    virtual ~ListLike<T>() {}
    T *next() const
    {
        return next_;
    }
    T *prev() const
    {
        return prev_;
    }
    // return new list head
    T *mergeAtHead(T *object)
    {
        T *head = dynamic_cast<T *>(this);
        T *tail = object;
        assert(head != nullptr);

        while (head->prev_)
            head = head->prev_;

        if (object == nullptr)
            return head;
        else
        {
            while (tail->next_)
                tail = tail->next_;
            head->prev_ = tail;
            tail->next_ = head;
            while (object->prev_)
                object = object->prev_;
            return object;
        }
    }
};

class Stringable
{
   public:
    virtual std::string toString() const
    {
        return "Stringable::null";
    }
    virtual std::string DebugString() const
    {
        return "Stringable::null";
    }
};

#define CHECK(x) assert(x)
#define CHECK_GT(a, b) assert((a) > (b))
#define CHECK_EQ(a, b) assert((a) == (b))

template <class T>
class TopoMap {
public:
    void add_dependency(T t, T dependent) {
        assert(t != dependent);
        dependents_[t].insert(dependent);
        dependents_.try_emplace(dependent, std::set<T>());
        indegree_.try_emplace(t, 0);
        indegree_[dependent]++;
    }
    std::set<T> & dependents(T t) {
        return dependents_[t];
    }
    // dependent last
    std::vector<T> sort() {
        std::vector<T> result;
        std::map<T, int> d1 = indegree_;
        std::map<T, int> d2 = indegree_;

        while (result.size() < dependents_.size())
        {
            size_t count = result.size();
            // DebugPrintValue(d1);
            for (auto & kv : d1)
            {
                T t = kv.first;
                int in = kv.second;
                // still has dependency not resolved.
                if (in != 0)
                    continue;

                result.emplace_back(t);

                for (T dependent : dependents_[t])
                {
                    assert(d2[dependent] > 0);
                    d2[dependent]--;
                }

                d2[t] = -1;
            }

            // circular dependency detected if false.
            assert(result.size() > count);

            d1 = d2;
        }

        return result;
    }

private:
    std::map<T, std::set<T>> dependents_;
    std::map<T, int> indegree_;
};


// Min, Max
// AlignUp(align, value), AlignDown(align, value)

#ifndef DEBUG_UTILS
#define DEBUG_UTILS

#include <iostream>
using namespace std;

// TODO: easy way to print enum constant as string

#define NON_COPYABLE(type)   \
    type() = default;        \
    type(type &&) = default; \
    type(const type &) = default;

#define DebugLog(msg)                     \
    do                                    \
    {                                     \
        cerr << "DEBUG: " << msg << endl; \
    } while (0)

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
        assert(false);                                         \
    } while (0)

#ifdef HAS_LEXER
#define SyntaxErrorEx(msg)                                           \
    do                                                               \
    {                                                                \
        cerr << "Syntax: " << msg << " at " << __FILE__ << ':'       \
             << to_string(__LINE__) << '\t' << "'" << lex.peakNext() \
             << "' at " << lex.peakNext().line << endl;              \
        assert(false);                                               \
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
        assert(false);                                            \
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
            assert(false);                                                    \
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

#define LexError(msg)                                       \
    do                                                      \
    {                                                       \
        cerr << "Lex: " << msg << " at " << __FILE__ << ':' \
             << to_string(__LINE__) << endl;                \
        assert(false);                                      \
    } while (0)

#define IRError(msg)                                       \
    do                                                     \
    {                                                      \
        cerr << "IR: " << msg << " at " << __FILE__ << ':' \
             << to_string(__LINE__) << endl;               \
        assert(false);                                     \
    } while (0)

#endif

// always valid assertion
#define ASSERT(e)                                                          \
    (void)((!!(e)) ||                                                      \
           (_wassert(                                                      \
                _CRT_WIDE(#e), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), \
            0))

