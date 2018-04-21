#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

#define ELEMENT_COUNT(a) (sizeof(a) / sizeof((a)[0]))

class StringBuf
{
   public:
    explicit StringBuf(const char *data)
    {
        assert(data != nullptr);

        size_t count = static_cast<size_t>(strlen(data));
        char *data2 = new char[count + 1];

        std::copy(data, data + count, data2);
        data2[count] = '\0';

        begin = now = data2;
        end = begin + count;
    }

    StringBuf(const char *data, size_t n)
    {
        assert(data != nullptr);

        size_t count = static_cast<size_t>(strlen(data));
        count = (count > n) ? n : count;
        char *data2 = new char[count + 1];

        std::copy(data, data + count, data2);
        data2[count] = '\0';

        begin = now = data2;
        end = begin + count;
    }

    ~StringBuf()
    {
        delete[] begin;
    }
    char peak(size_t offset = 0) const
    {
        if (now + offset < end)
            return now[offset];
        else
            return '\0';
    }
    char pop1()
    {
        if (now < end)
            return *(now++);
        else
            return '\0';
    }
    void pop(size_t count = 1)
    {
        const char *now2 = now + count;
        if (now2 < now || now2 > end)
            now = end;
        else
            now = now2;
    }
    bool empty() const
    {
        return now == end;
    }
    size_t size() const
    {
        return end - now;
    }
    const char *data() const
    {
        return now;
    }

   private:
    const char *begin, *end, *now;
};

class StringRef
{
   public:
    StringRef()
    {
        begin_ = end_ = "";
    }
    StringRef(const char *data)
    {
        assert(data != nullptr);

        begin_ = data;
        end_ = data + strlen(begin_);
    }
    // fast or safety ? fast
    StringRef(const char *data, size_t n)
    {
        assert(data != nullptr);

        begin_ = data;
        end_ = data + n;
    }
    StringRef &operator=(const char *data)
    {
        return (*this = StringRef(data));
    }

    void clear()
    {
        begin_ = end_ = "";
    }

    bool empty() const
    {
        return begin_ == end_;
    }

    size_t size() const
    {
        return end_ - begin_;
    }

    char front() const
    {
        assert(!empty());
        return *begin_;
    }

    char back() const
    {
        assert(!empty());
        return *(end_ - 1);
    }

    std::string toString() const
    {
        std::string s;
        if (end_ > begin_)
            s.assign(begin_, end_ - begin_);
        return s;
    }

    const char *data() const
    {
        return begin_;
    }

    const char *begin() const
    {
        return begin_;
    }

    const char *end() const
    {
        return end_;
    }

    char operator[](size_t offset) const
    {
        assert((begin_ + offset) < end_);
        return begin_[offset];
    }

    bool operator==(const StringRef &other) const
    {
        const char *p1 = begin_, *p2 = other.begin_;
        while (p1 != end_ && p2 != other.end_)
        {
            if (*p1 != *p2)
                break;
            ++p1, ++p2;
        }
        return (p1 == end_) && (p2 == other.end_);
    }
    bool operator!=(const StringRef &other) const
    {
        return !(*this == other);
    }

    friend bool operator==(const StringRef &s1, const char *s2)
    {
        assert(s2 != nullptr);

        const char *p1 = s1.begin_, *p2 = s2;
        while (p1 != s1.end_ && *p2 != '\0')
        {
            if (*p1 != *p2)
                return false;
            ++p1, ++p2;
        }
        return p1 == s1.end_ && *p2 == '\0';
    }
    friend bool operator==(const char *s1, const StringRef &s2)
    {
        return s2 == s1;
    }
    friend bool operator!=(const StringRef &s1, const char *s2)
    {
        return !(s1 == s2);
    }
    friend bool operator!=(const char *s1, const StringRef &s2)
    {
        return !(s2 == s1);
    }
    friend std::ostream &operator<<(std::ostream &o, const StringRef &s)
    {
        std::copy(s.begin_, s.end_, std::ostream_iterator<char>(o));
        return o;
    }

   private:
    const char *begin_, *end_;
};

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
