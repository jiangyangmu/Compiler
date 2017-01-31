#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <string>
#include <vector>
using namespace std;

#define ELEMENT_COUNT(a) (sizeof(a) / sizeof((a)[0]))

class StringBuf
{
   public:
    explicit StringBuf(const char *data)
    {
        assert(data != nullptr);

        size_t count = static_cast<size_t>(strlen(data));
        char *data2 = new char[count + 1];

        copy(data, data + count, data2);
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

        copy(data, data + count, data2);
        data2[count] = '\0';

        begin = now = data2;
        end = begin + count;
    }

    ~StringBuf()
    {
        delete[] begin;
    }
    char peak(size_t offset = 0)
    {
        if (now + offset < end)
            return now[offset];
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
    bool empty()
    {
        return now == end;
    }
    size_t size()
    {
        return end - now;
    }
    const char *data()
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
        begin = end = nullptr;
    }
    explicit StringRef(const char *data)
    {
        assert(data != nullptr);

        begin = data;
        end = data + strlen(begin);
    }
    // fast or safety ? fast
    StringRef(const char *data, size_t n)
    {
        assert(data != nullptr);

        begin = data;
        end = data + n;
    }

    void clear()
    {
        begin = end = nullptr;
    }

    size_t size() const
    {
        return end - begin;
    }

    string toString() const
    {
        string s;
        if (end > begin)
            s.assign(begin, end - begin);
        return s;
    }

    const char *data() const
    {
        return begin;
    }

    char operator[](size_t offset) const
    {
        assert((begin + offset) < end);
        return begin[offset];
    }

    bool operator==(const StringRef &other) const
    {
        const char *p1 = begin, *p2 = other.begin;
        while (p1 != end && p2 != other.end)
        {
            if (*p1 != *p2)
                break;
            ++p1, ++p2;
        }
        return (p1 == end) && (p2 == other.end);
    }

    friend bool operator==(const StringRef &s1, const char *s2)
    {
        assert(s2 != nullptr);

        const char *p1 = s1.begin, *p2 = s2;
        while (p1 != s1.end && *p2 != '\0')
        {
            if (*p1 != *p2)
                return false;
            ++p1, ++p2;
        }
        return p1 == s1.end && *p2 == '\0';
    }
    friend bool operator==(const char *s1, const StringRef &s2)
    {
        return s2 == s1;
    }
    friend ostream &operator<<(ostream &o, const StringRef &s)
    {
        copy(s.begin, s.end, ostream_iterator<char>(o));
        return o;
    }

   private:
    const char *begin, *end;
};

template <typename T>
class TreeLike
{
    T *parent_;
    std::vector<T *> children;

   public:
    TreeLike<T>() : parent_(nullptr)
    {
    }
    virtual ~TreeLike<T>()
    {
    }
    bool isRoot() const
    {
        return parent_ == nullptr;
    }
    void setParent(T *p)
    {
        parent_ = p;
        if (p)
            p->children.push_back(dynamic_cast<T *>(this));
    }
    T *parent() const
    {
        return parent_;
    }
    std::vector<T *> const &getChildren() const
    {
        return children;
    }
};

template <typename T>
class ListLike
{
    T *next_, *prev_;

   public:
    ListLike<T>() : next_(nullptr), prev_(nullptr)
    {
    }
    virtual ~ListLike<T>()
    {
    }
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

#ifndef DEBUG_UTILS
#define DEBUG_UTILS

// TODO: easy way to print enum constant as string

#define NON_COPYABLE(type)   \
    type() = default;        \
    type(type &&) = default; \
    type(const type &) = default;

#define SyntaxWarning(msg)                                      \
    do                                                          \
    {                                                           \
        cout << "Warning: " << msg << " at " << __FILE__ << ':' \
             << to_string(__LINE__) << endl;                    \
    } while (0)

#ifdef HAS_LEXER
#define SyntaxWarningEx(msg)                                         \
    do                                                               \
    {                                                                \
        cout << "Warning: " << msg << " at " << __FILE__ << ':'      \
             << to_string(__LINE__) << '\t' << "'" << lex.peakNext() \
             << "' at " << lex.peakNext().line << endl;              \
    } while (0)
#else
#define SyntaxWarningEx(msg) SyntaxWarning(msg)
#endif

#define SyntaxError(msg)                                       \
    do                                                         \
    {                                                          \
        cout << "Syntax: " << msg << " at " << __FILE__ << ':' \
             << to_string(__LINE__) << endl;                   \
        exit(1);                                               \
    } while (0)

#define SyntaxErrorEx(msg)                                           \
    do                                                               \
    {                                                                \
        cout << "Syntax: " << msg << " at " << __FILE__ << ':'       \
             << to_string(__LINE__) << '\t' << "'" << lex.peakNext() \
             << "' at " << lex.peakNext().line << endl;              \
        exit(1);                                                     \
    } while (0)

#define SyntaxErrorDebug(msg)                                     \
    do                                                            \
    {                                                             \
        cout << "Syntax: " << msg << " at " << __FILE__ << ':'    \
             << to_string(__LINE__) << endl;                      \
        string s;                                                 \
        while (cin >> s && lex.hasNext())                         \
        {                                                         \
            if (s == "n")                                         \
            {                                                     \
                cout << "Token " << lex.peakNext() << " at line " \
                     << lex.peakNext().line << endl;              \
                lex.getNext();                                    \
            }                                                     \
            else                                                  \
            {                                                     \
                break;                                            \
            }                                                     \
        }                                                         \
        exit(1);                                                  \
    } while (0)

#define EXPECT(token_type)                                                    \
    do                                                                        \
    {                                                                         \
        if (!lex.hasNext() || lex.peakNext().type != (token_type))            \
        {                                                                     \
            cout << "Syntax: Expect " << Token::DebugTokenType(token_type)    \
                 << ", Actual "                                               \
                 << Token::DebugTokenType(lex.hasNext() ? lex.peakNext().type \
                                                        : NONE)               \
                 << " at " << __FILE__ << ':' << to_string(__LINE__) << endl; \
            exit(1);                                                          \
        }                                                                     \
        lex.getNext();                                                        \
    } while (0)

#define EXPECT_GET(token_type)                                                \
    ((!lex.hasNext() || lex.peakNext().type != (token_type))                  \
         ? (cout << "Syntax: Expect " << Token::DebugTokenType(token_type)    \
                 << ", Actual "                                               \
                 << Token::DebugTokenType(lex.hasNext() ? lex.peakNext().type \
                                                        : NONE)               \
                 << " at " << __FILE__ << ':' << to_string(__LINE__) << endl, \
            exit(1), Token())                                                 \
         : lex.getNext())

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
        cout << "Lex: " << msg << " at " << __FILE__ << ':' \
             << to_string(__LINE__) << endl;                \
        exit(1);                                            \
    } while (0)

#endif
