#include "Definition.h"

namespace Language {

ObjectDefinition * AsObjectDefinition(Definition * definition)
{
    ASSERT(definition && definition->type == DefinitionType::OBJECT_DEFINITION);
    return reinterpret_cast<ObjectDefinition *>(definition);
}
FunctionDefinition * AsFunctionDefinition(Definition * definition)
{
    ASSERT(definition && definition->type == DefinitionType::FUNCTION_DEFINITION);
    return reinterpret_cast<FunctionDefinition *>(definition);
}
EnumConstDefinition * AsEnumConstDefinition(Definition * definition)
{
    ASSERT(definition && definition->type == DefinitionType::ENUM_CONST_DEFINITION);
    return reinterpret_cast<EnumConstDefinition *>(definition);
}
TypeTagDefinition * AsTypeTagDefinition(Definition * definition)
{
    ASSERT(definition && definition->type == DefinitionType::TYPE_TAG_DEFINITION);
    return reinterpret_cast<TypeTagDefinition *>(definition);
}
TypeAliasDefinition * AsTypeAliasDefinition(Definition * definition)
{
    ASSERT(definition && definition->type == DefinitionType::TYPE_ALIAS_DEFINITION);
    return reinterpret_cast<TypeAliasDefinition *>(definition);
}
GotoLabelDefinition * AsGotoLabelDefinition(Definition * definition)
{
    ASSERT(definition && definition->type == DefinitionType::GOTO_LABEL_DEFINITION);
    return reinterpret_cast<GotoLabelDefinition *>(definition);
}

Type * ExtractDefinitionCType(Definition * definition)
{
    Type * type = nullptr;
    switch (definition->type)
    {
        case OBJECT_DEFINITION:        type = reinterpret_cast<ObjectDefinition *>(definition)->objType; break;
        case FUNCTION_DEFINITION:      type = reinterpret_cast<FunctionDefinition *>(definition)->funcType; break;
        case ENUM_CONST_DEFINITION:    type = reinterpret_cast<EnumConstDefinition *>(definition)->enumConstType; break;
        case TYPE_ALIAS_DEFINITION:    type = reinterpret_cast<TypeAliasDefinition *>(definition)->aliasedType; break;
        case TYPE_TAG_DEFINITION:      type = reinterpret_cast<TypeTagDefinition *>(definition)->taggedType; break;
        case GOTO_LABEL_DEFINITION: ASSERT(false); break;
    }
    return type;
}

void DeleteDefinition(Definition * definition)
{
    switch (definition->type)
    {
        case OBJECT_DEFINITION:        delete reinterpret_cast<ObjectDefinition *>(definition); break;
        case FUNCTION_DEFINITION:      ASSERT(!AsFunctionDefinition(definition)->hasFuncBody);
                                            delete reinterpret_cast<FunctionDefinition *>(definition); break;
        case ENUM_CONST_DEFINITION:    delete reinterpret_cast<EnumConstDefinition *>(definition); break;
        case TYPE_ALIAS_DEFINITION:    delete reinterpret_cast<TypeAliasDefinition *>(definition); break;
        case TYPE_TAG_DEFINITION:      delete reinterpret_cast<TypeTagDefinition *>(definition); break;
        case GOTO_LABEL_DEFINITION:    delete reinterpret_cast<GotoLabelDefinition *>(definition); break;
    }
}

// If succeed, a and return value are merge result, b is destroied.
// If fail, returns nullptr, a and b unchanged.
// a b order matters.
Definition * MergeDefinition(Definition * a, Definition * b)
{
    // object:
    //          IMPORT x IMPORT = IMPORT
    // function:
    //          extern-nb X extern-nb = extern-nb
    //          extern-b X extern-nb = extern-b
    //          extern-nb X extern-b = extern-b
    //          static-nb X extern-nb = static-nb
    //          static-b X extern-nb = static-b
    //          static-nb X extern-b = static-b
    //          static-nb X static-nb = static-nb
    //          static-b X static-nb = static-b
    //          static-nb X static-b = static-b
    // type tag:
    //          no-type X no-type = no-type
    //          type X no-type = type
    //          no-type X type = type
    // goto label:
    //          no-def X no-def = no-def
    //          def X no-def = def
    //          no-def X def = def
    ASSERT(a && b && a->name == b->name);

    if (a->type == DefinitionType::OBJECT_DEFINITION && b->type == DefinitionType::OBJECT_DEFINITION)
    {
        ObjectDefinition * oa = AsObjectDefinition(a);
        ObjectDefinition * ob = AsObjectDefinition(b);

        if (TypeEqual(oa->objType, ob->objType) &&
            oa->objStorageType == ObjectStorageType::IMPORT_OBJECT &&
            ob->objStorageType == ObjectStorageType::IMPORT_OBJECT)
        {
            DeleteDefinition(b);
            return a;
        }
    }
    else if (a->type == DefinitionType::FUNCTION_DEFINITION && b->type == DefinitionType::FUNCTION_DEFINITION)
    {
        FunctionDefinition * fa = AsFunctionDefinition(a);
        FunctionDefinition * fb = AsFunctionDefinition(b);

        if (TypeEqual(fa->funcType, fb->funcType))
        {
            int i = 0;
            
            // {extern, extern} = 1, {static, extern} = 2, {static, static} = 3
            if (fa->funcStorageType == FunctionStorageType::IMPORT_FUNCTION &&
                fb->funcStorageType == FunctionStorageType::IMPORT_FUNCTION)
                i = 1;
            else if (fa->funcStorageType == FunctionStorageType::GLOBAL_FUNCTION)
            {
                if (fb->funcStorageType == FunctionStorageType::IMPORT_FUNCTION)
                    i = 2;
                else
                    i = 3;
            }

            // {nobody, nobody} = ?*3-2, {body, nobody} = ?*3-1, {nobody, body} = ?*3
            if (!fa->hasFuncBody && !fb->hasFuncBody)
                i = i + i + i - 2;
            else if (fa->hasFuncBody && !fb->hasFuncBody)
                i = i + i + i - 1;
            else if (!fa->hasFuncBody && fb->hasFuncBody)
                i = i + i + i;

            Definition * result = nullptr;
            // valid i: [1, 9]
            switch (i)
            {
                case 3:
                case 6:
                case 9:
                    fa->hasFuncBody = true;
                    fb->hasFuncBody = false;
                case 1:
                case 2:
                case 4:
                case 5:
                case 7:
                case 8:
                    DeleteDefinition(b);
                    result = a;
                    break;
                default:
                    break;
            }
            if (result)
                return result;
        }
    }
    else if (a->type == DefinitionType::TYPE_TAG_DEFINITION &&
             b->type == DefinitionType::TYPE_TAG_DEFINITION)
    {
        TypeTagDefinition * ta = AsTypeTagDefinition(a);
        TypeTagDefinition * tb = AsTypeTagDefinition(b);

        if (!tb->taggedType)
        {
            DeleteDefinition(b);
            return a;
        }
        else if (!ta->taggedType)
        {
            DeleteDefinition(a);
            return b;
        }
    }
    else if (a->type == DefinitionType::GOTO_LABEL_DEFINITION &&
             b->type == DefinitionType::GOTO_LABEL_DEFINITION)
    {
        GotoLabelDefinition * ga = AsGotoLabelDefinition(a);
        GotoLabelDefinition * gb = AsGotoLabelDefinition(b);

        if (!gb->isDefined)
        {
            DeleteDefinition(b);
            return a;
        }
        else if (!ga->isDefined)
        {
            DeleteDefinition(a);
            return b;
        }
    }

    return nullptr;
}

typedef Definition ** DefinitionLocation;

DefinitionContextNamespace DefinitionType2DefinitionContextNamespace(DefinitionType type)
{
    DefinitionContextNamespace ns;
    switch (type)
    {
        case OBJECT_DEFINITION:
        case FUNCTION_DEFINITION:
        case ENUM_CONST_DEFINITION:
        case TYPE_ALIAS_DEFINITION:
            ns = ID_NAMESPACE;
            break;
        case TYPE_TAG_DEFINITION:
            ns = TAG_NAMESPACE;
            break;
        case GOTO_LABEL_DEFINITION:
            ns = LABEL_NAMESPACE;
            break;
    }
    return ns;
}

DefinitionLocation FindDefinitionLocation(DefinitionContext * currentContext,
                                          StringRef name,
                                          DefinitionContextNamespace ns,
                                          bool searchParent)
{
    ASSERT(currentContext);

    if (ns == DefinitionContextNamespace::LABEL_NAMESPACE)
    {
        // Find label in function context.
        DefinitionContext * context = currentContext;

        while (context->scope != DefinitionContextScope::FUNCTION_SCOPE)
        {
            context = context->parent;
            ASSERT(context);
        }

        DefinitionLocation location = nullptr;
        for (Definition *& definition : context->definitions[ns])
        {
            if (definition->name == name)
            {
                location = &definition;
                break;
            }
        }
        return location;
    }
    else
    {
        // Find tag/id in current or parent context.
        DefinitionContext * context = currentContext;

        DefinitionLocation location = nullptr;
        do
        {
            for (Definition *& definition : context->definitions[ns])
            {
                if (definition->name == name)
                {
                    location = &definition;
                    break;
                }
            }
            if (searchParent)
                context = context->parent;
            else
                break;
        } while (!location && context);

        return location;
    }
}

Definition * InsertDefinition(DefinitionContext * context,
                              Definition * definition)
{
    DefinitionContextNamespace ns = DefinitionType2DefinitionContextNamespace(definition->type);

    DefinitionLocation existingDefinitionLocation = FindDefinitionLocation(context,
                                                                           definition->name,
                                                                           ns,
                                                                           false);

    Definition * existingDefinition = existingDefinitionLocation ? *existingDefinitionLocation
                                                                 : nullptr;
    if (existingDefinition)
    {
        Definition * mergedDefinition = MergeDefinition(definition, existingDefinition);
        ASSERT(mergedDefinition);

        *existingDefinitionLocation = mergedDefinition;
        
        return mergedDefinition;
    }
    else
    {
        context->definitions[ns].push_back(definition);
        
        return definition;
    }
}

Definition * NewObjectDefinition(DefinitionContext * context,
                                 StringRef name,
                                 Type * objType,
                                 ObjectStorageType objStorageType)
{
    ObjectDefinition * objDef = new ObjectDefinition;
    objDef->def.name = name;
    objDef->def.type = DefinitionType::OBJECT_DEFINITION;
    objDef->objType = objType;
    objDef->objStorageType = objStorageType;
    objDef->objValue = nullptr;

    return InsertDefinition(context, &objDef->def);
}

Definition * NewFunctionDefinition(DefinitionContext * context,
                                   StringRef name,
                                   Type * funcType,
                                   FunctionStorageType funcStorageType,
                                   bool hasFuncBody)
{
    FunctionDefinition * funcDef = new FunctionDefinition;
    funcDef->def.name = name;
    funcDef->def.type = DefinitionType::FUNCTION_DEFINITION;
    funcDef->funcType = funcType;
    funcDef->funcStorageType = funcStorageType;
    funcDef->hasFuncBody = hasFuncBody;

    return InsertDefinition(context, &funcDef->def);
}

Definition * NewEnumConstDefinition(DefinitionContext * context,
                                    StringRef name,
                                    Type * enumConstType,
                                    int enumConstValue)
{
    EnumConstDefinition * enumConstDef = new EnumConstDefinition;
    enumConstDef->def.name = name;
    enumConstDef->def.type = DefinitionType::ENUM_CONST_DEFINITION;
    enumConstDef->enumConstType = enumConstType;
    enumConstDef->enumConstValue = enumConstValue;

    return InsertDefinition(context, &enumConstDef->def);
}

Definition * NewTypeTagDefinition(DefinitionContext * context,
                                  StringRef name,
                                  Type * taggedType)
{
    TypeTagDefinition * typeTagDef = new TypeTagDefinition;
    typeTagDef->def.name = name;
    typeTagDef->def.type = DefinitionType::TYPE_TAG_DEFINITION;
    typeTagDef->taggedType = taggedType;

    return InsertDefinition(context, &typeTagDef->def);
}

Definition * NewTypeAliasDefinition(DefinitionContext * context,
                                    StringRef name,
                                    Type * aliasedType)
{
    TypeAliasDefinition * typeAliasDef = new TypeAliasDefinition;
    typeAliasDef->def.name = name;
    typeAliasDef->def.type = DefinitionType::TYPE_ALIAS_DEFINITION;
    typeAliasDef->aliasedType = aliasedType;

    return InsertDefinition(context, &typeAliasDef->def);
}

Definition * NewGotoLabelDefinition(DefinitionContext * context,
                                    StringRef name,
                                    bool isDefined)
{
    GotoLabelDefinition * gotoLabelDef = new GotoLabelDefinition;
    gotoLabelDef->def.name = name;
    gotoLabelDef->def.type = DefinitionType::GOTO_LABEL_DEFINITION;
    gotoLabelDef->isDefined = isDefined;

    return InsertDefinition(context, &gotoLabelDef->def);
}

DefinitionContext * CreateDefinitionContext(DefinitionContext * parent,
                                            DefinitionContextScope scope)
{
    DefinitionContext * context = new DefinitionContext;
    context->parent = parent;
    context->next = nullptr;
    context->firstChild = nullptr;
    context->scope = scope;

    if (parent)
    {
        DefinitionContext * insert = parent->firstChild;
        if (!insert)
        {
            parent->firstChild = context;
        }
        else
        {
            while (insert->next)
                insert = insert->next;
            insert->next = context;
        }
    }

    return context;
}

Definition * LookupDefinition(DefinitionContext * currentContext,
                              StringRef name,
                              DefinitionContextNamespace ns,
                              bool searchParent)
{
    DefinitionLocation location = FindDefinitionLocation(currentContext,
                                                         name,
                                                         ns,
                                                         searchParent);
    return location ? *location : nullptr;
}

void PrintDefinition(Definition * definition)
{
    // name
    std::cout << definition->name << '\t';

    // type, ...
    switch (definition->type)
    {
        case OBJECT_DEFINITION:
            std::cout << "OBJ \t" << TypeDebugString(AsObjectDefinition(definition)->objType);
            break;
        case FUNCTION_DEFINITION:
            std::cout << "FUNC\t" << TypeDebugString(AsFunctionDefinition(definition)->funcType);
            break;
        case ENUM_CONST_DEFINITION:
            std::cout << "ENUM\t" << AsEnumConstDefinition(definition)->enumConstValue;
            break;
        case TYPE_ALIAS_DEFINITION:
            std::cout << "ALIA\t" << TypeDebugString(AsTypeAliasDefinition(definition)->aliasedType);
            break;
        case TYPE_TAG_DEFINITION:
            std::cout << "TAG \t" << TypeDebugString(AsTypeTagDefinition(definition)->taggedType);
            break;
        case GOTO_LABEL_DEFINITION:
            std::cout << "LABL\t" << AsGotoLabelDefinition(definition)->isDefined;
            break;
    }
    std::cout << '\n';
}

void PrintDefinitionContextImpl(DefinitionContext * context, std::string indent)
{
    std::cout << indent << "{ scope = ";
    switch (context->scope)
    {
        case DefinitionContextScope::GLOBAL_SCOPE:      std::cout << "global"; break;
        case DefinitionContextScope::FUNCTION_SCOPE:    std::cout << "function"; break;
        case DefinitionContextScope::BLOCK_SCOPE:       std::cout << "block"; break;
    }
    std::cout << "\n";

    {
        std::string newIndent = indent + "  ";

        std::cout << newIndent << "ID\n";
        for (Definition * definition : context->definitions[ID_NAMESPACE])
        {
            std::cout << newIndent << "  ";
            PrintDefinition(definition);
        }
        std::cout << newIndent << "TAG\n";
        for (Definition * definition : context->definitions[TAG_NAMESPACE])
        {
            std::cout << newIndent << "  ";
            PrintDefinition(definition);
        }
        std::cout << newIndent << "LABEL\n";
        for (Definition * definition : context->definitions[LABEL_NAMESPACE])
        {
            std::cout << newIndent << "  ";
            PrintDefinition(definition);
        }

        if (context->firstChild)
            PrintDefinitionContextImpl(context->firstChild, newIndent);
    }

    std::cout << indent << "}\n";
    if (context->next)
        PrintDefinitionContextImpl(context->next, indent);
}

void PrintDefinitionContext(DefinitionContext * context)
{
    PrintDefinitionContextImpl(context, "");
}

}