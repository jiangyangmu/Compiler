#pragma once

#include "common.h"

class Environment;
class SyntaxNode;

enum EObjectLocation
{
    OBJ_LOC_NONE,
    OBJ_LOC_STATIC,
    OBJ_LOC_LOCAL,
    OBJ_LOC_PARAM
};

// Object: Everything about storage, instance specialty
class Object
{
   protected:
    size_t _size;
    EObjectLocation _loc;

   public:
    size_t size() const
    {
        return _size;
    }
    virtual EObjectLocation location() const
    {
        return OBJ_LOC_NONE;
    }
    virtual string toString() const
    {
        return "Object";
    }
};

class IntegerObject : public Object
{
   public:
    IntegerObject(size_t size)
    {
        _size = size;
    }
    virtual string toString() const
    {
        return "Integer";
    }
};
class FloatObject : public Object
{
   public:
    FloatObject(size_t size)
    {
        _size = size;
    }
    virtual string toString() const
    {
        return "Float";
    }
};
class PointerObject : public Object
{
   public:
    PointerObject(size_t size)
    {
        _size = size;
    }
    virtual string toString() const
    {
        return "Pointer";
    }
};
class ArrayObject : public Object
{
    vector<size_t> _dim;

   public:
    ArrayObject(size_t size)
    {
        _size = size;
    }
    virtual string toString() const
    {
        return "Array";
    }
};
class StructObject : public Object
{
    struct MemberDesc
    {
        StringRef name;
        size_t offset;
    };

    vector<MemberDesc> _members;
    // Environment *_member_env;

   public:
    StructObject(size_t size)
    {
        _size = size;
    }
    virtual string toString() const
    {
        return "struct";
    }
};
// Union object
class EnumObject : public Object
{
   public:
    EnumObject(size_t size)
    {
        _size = size;
    }
};
class FuncObject : public Object
{
    struct ParamDesc
    {
        StringRef name;
        size_t location;
    };

    vector<ParamDesc> _params;
    SyntaxNode *_body;
    Environment *_env;

   public:
    FuncObject() : _body(nullptr), _env(nullptr)
    {
        _size = 0;
    }
    void addParameter(StringRef name, size_t location)
    {
        ParamDesc desc = {name, location};
        _params.push_back(desc);
    }
    SyntaxNode * getFuncBody()
    {
        return _body;
    }
    void setFuncBody(SyntaxNode *body)
    {
        _body = body;
    }
    Environment *getFuncEnv()
    {
        return _env;
    }
    void setFuncEnv(Environment *env)
    {
        _env = env;
    }
    virtual string toString() const
    {
        return "function";
    }
};

class ObjectFactory
{
   public:
    // static void StructMember(StructObject &obj, Symbol *s)
    // {
    // assert( s );
    // struct MemberDesc md = {s->name, 0};
    // obj._members.push_back(md);
    // obj.env->add(s);
    // }
};
