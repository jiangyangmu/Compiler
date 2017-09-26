#include "type.h"

std::string Type::toString() const
{
    return "";
    // std::string s;
    // s += '<';
    // if (isIncomplete()) s += 'I';
    // if (isConst()) s += 'C';
    // if (isVolatile()) s += 'B';
    // if (isLvalue()) s += 'L';
    // if (isIntegral()) s += 'i';
    // if (isArithmetic()) s += 'A';
    // if (isScalar()) s += 'S';
    // if (isObject()) s += 'O';
    // if (isFunction()) s += 'F';
    // if (s.empty()) s += '-';
    // s += ':';
    // s += std::to_string(getSize());
    // s += ':';
    // s += std::to_string(getAlignment());
    // s += '>';
    // return s;
}
std::string VoidType::toString() const
{
    return "void" + Type::toString();
}
std::string CharType::toString() const
{
    return "char" + Type::toString();
}
std::string IntegerType::toString() const
{
    std::string s;
    for (const char *p = _desc; *p != '\0'; ++p)
    {
        if (!s.empty())
            s += ' ';
        switch (*p)
        {
            case 'S': s += "signed"; break;
            case 'U': s += "unsigned"; break;
            case 'c': s += "char"; break;
            case 's': s += "short"; break;
            case 'l': s += "long"; break;
            case 'i': s += "int"; break;
            default: break;
        }
    }
    return s + Type::toString();
}
std::string FloatingType::toString() const
{
    std::string s;
    for (const char *p = _desc; *p != '\0'; ++p)
    {
        if (!s.empty())
            s += ' ';
        switch (*p)
        {
            case 'f': s += "float"; break;
            case 'd': s += "double"; break;
            case 'l': s += "long"; break;
            default: break;
        }
    }
    return s + Type::toString();
}
std::string PointerType::toString() const
{
    return "pointer to " + (_t ? _t->toString() : "null") +
        Type::toString();
}
std::string ArrayType::toString() const
{
    return "array of " + (_t ? _t->toString() : "null") + Type::toString();
}
std::string FuncType::toString() const
{
    return "function returns " + (_t ? _t->toString() : "null") +
        Type::toString();
}
std::string TagType::toString() const
{
    std::string s;
    switch (_impl_type)
    {
        case T_ENUM: s += "enum"; break;
        case T_STRUCT: s += "struct"; break;
        case T_UNION: s += "union"; break;
        case T_TYPEDEF: s += "typedef"; break;
        default: s += "unknown_tag_type"; break;
    }
    s += ' ';
    s += _name.toString();
    s += Type::toString();
    s += " {";
    if (_impl != nullptr)
    {
        s += ' ';
        s += _impl->toString();
        s += ' ';
    }
    s += '}';
    return s;
}
std::string EnumConstType::toString() const
{
    return "enum-const";
}
std::string EnumTypeImpl::toString() const
{
    std::string s;
    for (auto m : _members)
    {
        s += m->getName().toString();
        s += '(';
        s += std::to_string(m->getValue());
        s += ')';
        s += ',';
    }
    if (!s.empty())
        s.pop_back();
    return s + Type::toString();
}
std::string StructTypeImpl::toString() const
{
    std::string s;
    for (auto t : _member_type)
    {
        s += t->toString();
        s += ':';
    }
    if (!s.empty())
        s.pop_back();
    return s + Type::toString();
}
std::string TypedefTypeImpl::toString() const
{
    return "~<typedef-impl~>";
}
std::string LabelType::toString() const
{
    return "label";
}

void ArrayType::setTargetType(Type *t)
{
    DerivedType::setTargetType(t);
    if (_n != 0 && !t->isIncomplete())
    {
        _size = t->getSize() * _n;
        _align = t->getAlignment();
        unsetProp(TP_INCOMPLETE);
    }
}
bool ArrayType::equal(const Type &o) const
{
    if (!DerivedType::equal(o))
        return false;
    const ArrayType &a = dynamic_cast<const ArrayType &>(o);
    return _n == a._n;
}

void FuncType::addParam(const Type *t, StringRef name)
{

    _param_t.push_back(t);
    _param_name.push_back(name);
}
void FuncType::fixParamType(size_t i, const Type *t)
{

    assert(i < _param_t.size());
    _param_t[i] = t;
}
void FuncType::setVarArg()
{
    _var_arg = true;
}
bool FuncType::equal(const Type &o) const
{
    if (!DerivedType::equal(o))
        return false;

    const FuncType &f = dynamic_cast<const FuncType &>(o);
    if (_var_arg != f._var_arg || _param_t.size() != f._param_t.size())
        return false;
    for (size_t i = 0; i < _param_t.size(); ++i)
    {
        assert(_param_t[i] && f._param_t[i]);
        if (!_param_t[i]->equal(*f._param_t[i]))
            return false;
    }
    return true;
}

void TagType::setImpl(const Type *impl)
{
    assert(impl != nullptr);  // && t->getClass() == T_ENUM_IMPL);

    if (impl->getClass() != _impl_type)
    {
        SyntaxError("TagType: tag type mismatch.");
    }

    _impl = impl;
    _size = impl->getSize();
    _align = impl->getAlignment();
    unsetProp(TP_INCOMPLETE);
    DebugLog("TagType: set impl of: " + _name.toString());
}
const Type *TagType::getImpl() const
{
    return _impl;
}
bool TagType::equal(const Type &o) const
{
    if (!Type::equal(o))
        return false;

    const TagType &t = dynamic_cast<const TagType &>(o);
    if (_name != t._name || _impl_type != t._impl_type)
        return false;

    assert(_impl && t._impl);
    return _impl->equal(*t._impl);
}

bool EnumConstType::equal(const Type &o) const
{
    if (!Type::equal(o))
        return false;

    const EnumConstType &e = dynamic_cast<const EnumConstType &>(o);
    return _name == e._name && _value == e._value;
}
bool EnumTypeImpl::equal(const Type &o) const
{
    SyntaxError("not implemented.");
    return false;
}
void StructTypeImpl::addMember(StringRef name, const Type *ct)
{
    Type *t = const_cast<Type *>(ct);

    if (t->isIncomplete())
        SyntaxError("struct: can't add incomplete type.");

    _member_name.push_back(name);
    _member_type.push_back(t);

    _align = (_align >= t->getAlignment()) ? _align : t->getAlignment();

    if (_tc == T_STRUCT)
    {
        assert(_align > 0);
        if (_member_offset.empty())
            _member_offset.push_back(0);
        else
            _member_offset.push_back(_size +
                    (_align - _size % _align) % _align);
        _size = _member_offset.back() + t->getSize();
    }
    else
    {
        _size = (_size >= t->getSize()) ? _size : t->getSize();
    }
}
const Type *StructTypeImpl::getMemberType(StringRef name) const
{
    const Type *t = nullptr;
    size_t i = 0;
    for (StringRef nm : _member_name)
    {
        if (nm == name)
        {
            t = _member_type[i];
            break;
        }
        ++i;
    }
    if (i == _member_name.size())
    {
        SyntaxError("member not found.");
    }
    return t;
}
size_t StructTypeImpl::getMemberOffset(StringRef name) const
{
    if (_tc == T_STRUCT)
    {
        size_t i = 0;
        while (i < _member_name.size())
        {
            if (_member_name[i] == name)
                break;
            ++i;
        }
        if (i == _member_name.size())
            SyntaxError("member not found.");
        return _member_offset[i];
    }
    else
        return 0;
}
bool StructTypeImpl::equal(const Type &o) const
{
    SyntaxError("not implemented.");
    return false;
}
bool TypedefTypeImpl::equal(const Type &o) const
{
    SyntaxError("not implemented.");
    return false;
}

