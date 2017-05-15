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

    // lazy evaluate size
    bool _need_read;
    const TypeBase *_type;
    const Environment *_env;

    Object(size_t size, bool need_read = false, const TypeBase *type = nullptr,
           const Environment *env = nullptr)
        : _size(size), _need_read(need_read), _type(type), _env(env)
    {
    }

   public:
    size_t size();
    // size_t size() const
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
    IntegerObject(size_t size) : Object(size)
    {
    }
    virtual string toString() const
    {
        return "Integer";
    }
};
class FloatObject : public Object
{
   public:
    FloatObject(size_t size) : Object(size)
    {
    }
    virtual string toString() const
    {
        return "Float";
    }
};
class PointerObject : public Object
{
   public:
    PointerObject(size_t size) : Object(size)
    {
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
    ArrayObject(const TypeBase *type, const Environment *env)
        : Object(0, true, type, env)
    {
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
    StructObject(const TypeBase *type, const Environment *env)
        : Object(0, true, type, env)
    {
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
    EnumObject(size_t size) : Object(size)
    {
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
    Environment *_body_env;

   public:
    FuncObject() : Object(0), _body(nullptr), _body_env(nullptr)
    {
    }
    void addParameter(StringRef name, size_t location)
    {
        ParamDesc desc = {name, location};
        _params.push_back(desc);
    }
    SyntaxNode *getFuncBody()
    {
        return _body;
    }
    void setFuncBody(SyntaxNode *body)
    {
        _body = body;
    }
    Environment *getFuncEnv()
    {
        return _body_env;
    }
    void setFuncEnv(Environment *env)
    {
        _body_env = env;
    }
    virtual string toString() const
    {
        return "Function";
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
