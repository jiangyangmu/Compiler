#pragma once

#include "../common.h"

// enum ETypeOperations

enum ETypeProperty {
    // properties
    TP_INCOMPLETE = 1,
    TP_CONST = (1 << 2),
    TP_VOLATILE = (1 << 3),
    TP_LVALUE = (1 << 4),
    // categorys
    TP_IS_INTEGRAL = (1 << 5),
    TP_IS_ARITHMETIC = (1 << 6),
    TP_IS_SCALAR = (1 << 7),
    // categorys - object type
    TP_IS_OBJECT = (1 << 8),
    TP_IS_FUNCTION = (1 << 9),
};

enum ETypeClass {
    T_NONE,
    T_VOID,
    T_CHAR,
    T_INT,
    T_FLOAT,
    T_POINTER,
    T_ARRAY,
    T_TAG,
    T_STRUCT,
    T_UNION,
    T_ENUM,
    T_ENUM_CONST,
    T_FUNCTION,
    T_TYPEDEF,
    T_LABEL
};

class Type {
protected:
    ETypeClass tc_;
    // properties
    int prop_;
    // representation
    size_t size_;
    size_t align_;

    Type(ETypeClass tc, int prop, size_t size = 0, size_t align = 0)
        : tc_(tc)
        , prop_(prop)
        , size_(size)
        , align_(align) {
    }
    void setProp(int p) {
        prop_ |= p;
    }
    void unsetProp(int p) {
        prop_ &= ~p;
    }

public:
    virtual ~Type() = default;
    bool isIncomplete() const {
        return prop_ & TP_INCOMPLETE;
    }
    bool isConst() const {
        return prop_ & TP_CONST;
    }
    bool isVolatile() const {
        return prop_ & TP_VOLATILE;
    }
    bool isLvalue() const {
        return prop_ & TP_LVALUE;
    }
    bool isIntegral() const {
        return prop_ & TP_IS_INTEGRAL;
    }
    bool isArithmetic() const {
        return prop_ & TP_IS_ARITHMETIC;
    }
    bool isScalar() const {
        return prop_ & TP_IS_SCALAR;
    }
    bool isObject() const {
        return prop_ & TP_IS_OBJECT;
    }
    bool isFunction() const {
        return prop_ & TP_IS_FUNCTION;
    }
    bool isModifiable() const {
        return isLvalue() && !isIncomplete() && !isConst();
    }
    ETypeClass getClass() const {
        return tc_;
    }
    size_t getSize() const {
        return size_;
    }
    size_t getAlignment() const {
        return align_;
    }

    void setLvalue() {
        setProp(TP_LVALUE);
    }
    void unsetLvalue() {
        unsetProp(TP_LVALUE);
    }
    void setQualifier(int qualifiers) {
        prop_ |= (qualifiers & (TP_CONST | TP_VOLATILE));
    }
    int getQualifier() const {
        return prop_ & (TP_CONST | TP_VOLATILE);
    }
    void unsetQualifier(int qualifiers) {
        prop_ &= ~(qualifiers & (TP_CONST | TP_VOLATILE));
    }
    void markComplete() {
        unsetProp(TP_INCOMPLETE);
    }

    virtual std::string toString() const {
        if (prop_ & TP_CONST)
            return "const ";
        else if (prop_ & TP_VOLATILE)
            return "volatile ";
        else
            return "";
    }
};

class IntegralType : public Type {
protected:
    bool signed_;

    IntegralType(ETypeClass tc, int prop, size_t size, size_t align)
        : Type(tc,
               prop | TP_IS_INTEGRAL | TP_IS_ARITHMETIC | TP_IS_SCALAR,
               size,
               align)
        , signed_(false) {
    }

public:
    bool isSigned() const {
        return signed_;
    }
};

class DerivedType : public Type {
protected:
    Type * t_;

    DerivedType(ETypeClass tc, int prop, size_t size = 0, size_t align = 0)
        : Type(tc, prop, size, align)
        , t_(nullptr) {
    }

public:
    virtual void setTargetType(Type * t) {
        // assert(t_ == nullptr);
        assert(t != nullptr);
        t_ = t;
    }
    Type * getTargetType() {
        return t_;
    }

    virtual std::string toString() const {
        return Type::toString();
    }
};

class VoidType : public Type {
public:
    VoidType()
        : Type(T_VOID, TP_INCOMPLETE | TP_IS_OBJECT) {
    }

    virtual std::string toString() const {
        return Type::toString() + "void";
    }
};

class CharType : public IntegralType {
public:
    CharType()
        : IntegralType(T_CHAR, TP_IS_OBJECT, 1, 1) {
        signed_ = true;
    }

    virtual std::string toString() const {
        return Type::toString() + "char";
    }
};

class IntegerType : public IntegralType {
    const char * desc_;

public:
    IntegerType(const char * desc)
        : IntegralType(T_INT, TP_IS_OBJECT, 4, 4)
        , desc_(desc) {
        // assume desc is valid combination
        assert(desc != nullptr);

        // sign
        signed_ = true;
        if (*desc == 'S')
            ++desc;
        else if (*desc == 'U')
            signed_ = false, ++desc;
        // size
        switch (*desc)
        {
            case 'c':
                size_ = 1;
                break;
            case 's':
                size_ = 2;
                break;
            case 'l':
                size_ = sizeof(long);
                break;
            default:
                break;
        }
        align_ = size_;
    }

    virtual std::string toString() const {
        std::string s;
        for (const char * p = desc_; *p != '\0'; ++p)
        {
            if (!s.empty())
                s += ' ';
            switch (*p)
            {
                case 'S':
                    s += "signed";
                    break;
                case 'U':
                    s += "unsigned";
                    break;
                case 'c':
                    s += "char";
                    break;
                case 's':
                    s += "short";
                    break;
                case 'l':
                    s += "long";
                    break;
                case 'i':
                    s += "int";
                    break;
                default:
                    break;
            }
        }
        return Type::toString() + s;
    }
};

class FloatingType : public Type {
    const char * desc_;

public:
    FloatingType(const char * desc)
        : Type(T_FLOAT, TP_IS_ARITHMETIC | TP_IS_SCALAR | TP_IS_OBJECT, 4, 4)
        , desc_(desc) {
        // assume desc is valid combination
        assert(desc != nullptr);

        // size
        switch (*desc)
        {
            case 'd':
                align_ = size_ = 8;
                break;
            case 'l': // long double
                size_ = 10;
                align_ = 16;
                break;
            default:
                break;
        }
    }

    virtual std::string toString() const {
        std::string s;
        for (const char * p = desc_; *p != '\0'; ++p)
        {
            if (!s.empty())
                s += ' ';
            switch (*p)
            {
                case 'f':
                    s += "float";
                    break;
                case 'd':
                    s += "double";
                    break;
                case 'l':
                    s += "long";
                    break;
                default:
                    break;
            }
        }
        return Type::toString() + s;
    }
};

class PointerType : public DerivedType {
public:
    PointerType()
        : DerivedType(T_POINTER,
                      TP_IS_SCALAR | TP_IS_OBJECT,
                      sizeof(void *),
                      sizeof(void *)) {
    }

    virtual std::string toString() const {
        return DerivedType::toString() + "pointer to " +
            (t_ ? t_->toString() : "<unknown>");
    }
};

class ArrayType : public DerivedType {
    size_t n_;

public:
    ArrayType()
        : DerivedType(T_ARRAY, TP_CONST | TP_INCOMPLETE | TP_IS_OBJECT)
        , n_(0) {
    }
    ArrayType(size_t n)
        : DerivedType(T_ARRAY, TP_CONST | TP_INCOMPLETE | TP_IS_OBJECT)
        , n_(n) {
    }

    virtual std::string toString() const {
        return DerivedType::toString() + "array[" +
            (n_ > 0 ? std::to_string(n_) : "") + "] of " +
            (t_ ? t_->toString() : "<unknown>");
    }
};

// class EnumType : public IntegralType {
// public:
//    EnumType()
//        : IntegralType(T_ENUM, 0, 4, 4) {
//        signed_ = true;
//    }
//};

class StructType : public Type {
    vector<StringRef> member_names_;
    vector<Type *> member_types_;
    vector<size_t> member_offsets_;

public:
    StructType()
        : Type(T_STRUCT, 0, 0, 1) {
    }
    void addMember(StringRef name, Type * ct) {
        member_names_.push_back(name);
        member_types_.push_back(ct);
    }
    // const Type * getMemberType(StringRef name) const;
    // size_t getMemberOffset(StringRef name) const;

    virtual std::string toString() const {
        std::string s = Type::toString();
        s += "struct { ";
        for (size_t i = 0; i < member_types_.size(); ++i)
        {
            s += "\"" + member_names_[i].toString() + "\":(" +
                member_types_[i]->toString() + ") ";
        }
        s += "}";
        return s;
    }
};

//// TODO: UnionType

class FuncType : public DerivedType {
    vector<Type *> param_types_; // can be nullptr for identifier_list
    vector<StringRef> param_names_; // can be "" for parameter_type_list
    bool is_varlist_;

public:
    FuncType()
        : DerivedType(T_FUNCTION, TP_CONST | TP_INCOMPLETE | TP_IS_FUNCTION)
        , is_varlist_(false) {
    }

    void addParam(StringRef name, Type * ct) {
        param_names_.push_back(name);
        param_types_.push_back(ct);
    }
    // void fixParamType(size_t i, const Type * t);
    void enableVarList() {
        is_varlist_ = true;
    }

    virtual std::string toString() const {
        std::string s = Type::toString();
        s += "function(";
        for (size_t i = 0; i < param_types_.size(); ++i)
        {
            s += "\"" + param_names_[i].toString() + "\":(" +
                param_types_[i]->toString() + "), ";
        }
        if (is_varlist_)
            s += "...";
        else if (!param_types_.empty())
            s.pop_back(), s.pop_back();
        s += ") returns " + (t_ ? t_->toString() : "<unknown>");
        return s;
    }
};
