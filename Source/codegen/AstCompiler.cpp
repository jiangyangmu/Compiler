#include <iostream>

#include <vector>
#include <deque>

#include "AstCompiler.h"
#include "../Logging/Logging.h"

AstCompileContext * CreateAstCompileContext()
{
    AstCompileContext * context = new AstCompileContext;
    context->currentFunctionDefinitionContext = nullptr;
    context->currentFunctionContext = nullptr;
    context->typeContext = Language::CreateTypeContext();
    context->constantContext = Language::CreateConstantContext();
    context->globalDefinitionContext = nullptr;
    return context;
}

class TypeSpecifiersBuilder {
    // collect type-specifier set
    // bits:
    //      void,char,short,int,
    //      long,float,double,signed
    //      unsigned,struct/union,enum,typedef-name
    int type_specifier_allow_, type_specifier_has_;
    // for struct/union, enum, typedef-name
    Language::Type * m_Type;

    Language::Type * builtType;

public:
    TypeSpecifiersBuilder()
        : type_specifier_allow_(0xffffFFFF)
        , type_specifier_has_(0)
        , m_Type(0)
        , builtType(nullptr) {
    }

    void Add(Token::Type tokenType, Language::Type * typeHandle = nullptr) {
        ASSERT(!builtType);

#define __check_set(value)                                             \
    if ((type_specifier_allow_ & (value)) == 0 ||                      \
        type_specifier_has_ & (value))                                 \
        SyntaxError("TypeSpecifiersBuilder: unexpected token");        \
    else                                                               \
        type_specifier_has_ |= (value);

        switch (tokenType)
        {
            case Token::KW_VOID:
                __check_set(0x1);
                type_specifier_allow_ &= 0x1;
                break;
            case Token::KW_CHAR:
                __check_set(0x2);
                type_specifier_allow_ &= 0x182; // char, signed, unsigned
                break;
            case Token::KW_SHORT:
                __check_set(0x4);
                type_specifier_allow_ &= 0x18c; // short, signed, unsigned, int
                break;
            case Token::KW_INT:
                __check_set(0x8);
                type_specifier_allow_ &= 0x19c; // int, signed, unsigned, short, long
                break;
            case Token::KW_LONG:
                __check_set(0x10);
                type_specifier_allow_ &= 0x1d8; // long, signed, unsigned, int, double
                break;
            case Token::KW_FLOAT:
                __check_set(0x20);
                type_specifier_allow_ &= 0x20;
                break;
            case Token::KW_DOUBLE:
                __check_set(0x40);
                type_specifier_allow_ &= 0x50; // double, long
                break;
            case Token::KW_SIGNED:
                __check_set(0x80);
                type_specifier_allow_ &= 0x9e; // signed, char, short, int, long
                break;
            case Token::KW_UNSIGNED:
                __check_set(0x100);
                type_specifier_allow_ &= 0x11e; // unsigned, char, short, int, long
                break;
            case Token::KW_STRUCT:
                __check_set(0x200);
                type_specifier_allow_ &= 0x200;
                m_Type = typeHandle;
                break;
                // case Token::KW_UNION:
                //    __check_set(0x400);
                //    type_specifier_allow_ &= 0x400;
                //    // TODO: type_ = new UnionType();
                //    break;
            case Token::KW_ENUM:
                __check_set(0x800);
                type_specifier_allow_ &= 0x800;
                m_Type = typeHandle;
                break;
            default:
                SyntaxError("TypeSpecifiersBuilder: unexpected token");
                break;
        }

#undef __check_set

        assert((~type_specifier_allow_ & type_specifier_has_) == 0);
    }

    Language::Type * Build(Language::TypeContext * context) {
        if (builtType)
            return builtType;

        if (type_specifier_has_ & 0xe00) // struct/union/enum/typedef
            return builtType = (CHECK(m_Type != 0), m_Type);   // TODO: copy?
        else if (type_specifier_has_ == 0x1) // void
            return builtType = &Language::MakeVoid(context)->type;
        else if (type_specifier_has_ == 0x2) // char
            return builtType = &Language::MakeChar(context)->type;
        else if (type_specifier_has_ == 0x20) // float
            return builtType = &Language::MakeFloat(context)->type;
        else if (type_specifier_has_ & 0x40) // double/long double
            // (type_specifier_has_ & 0x10) ? "ld" : "d"
            return builtType = &Language::MakeFloat(context)->type;
        else
        {
            //char * desc = new char[4];
            //char * d = desc;
            //if (type_specifier_has_ & 0x80)
            //    *(d++) = 'S';
            //if (type_specifier_has_ & 0x100)
            //    *(d++) = 'U';
            //if (type_specifier_has_ & 0x2)
            //    *(d++) = 'c';
            //if (type_specifier_has_ & 0x4)
            //    *(d++) = 's';
            //if (type_specifier_has_ & 0x10)
            //    *(d++) = 'l';
            //if (type_specifier_has_ & 0x8)
            //    *(d++) = 'i';
            //*d = '\0';
            return builtType = &Language::MakeInt(context)->type;
        }
    }
};

// Scope DSL:
void     BeginScope(AstCompileContext * context, ASTScope scope)
{
    context->scope.push_back(scope);
}
ASTScope GetScope(AstCompileContext * context)
{
    return context->scope.back();
}
void     EndScope(AstCompileContext * context)
{
    context->scope.pop_back();
}

// Context DSL:
// for definition context tree.
Language::DefinitionContext * NewDefinitionContext(AstCompileContext * context,
                                                   Language::DefinitionContextScope scope)
{
    return Language::CreateDefinitionContext(
        context->definitionContexts.empty() ? nullptr : context->definitionContexts.back(),
        scope
    );
}
void EnterDefinitionContext(AstCompileContext * context,
                            Language::DefinitionContext * definitionContext)
{
    context->definitionContexts.push_back(definitionContext);
}
Language::DefinitionContext * CurrentDefinitionContext(AstCompileContext * context)
{
    return context->definitionContexts.back();
}
void ExitDefinitionContext(AstCompileContext * context)
{
    context->definitionContexts.pop_back();
}
// for function context list.
void BeginFunctionContext(AstCompileContext * context,
                          StringRef functionName,
                          Language::FunctionType * functionType,
                          Language::DefinitionContext * functionDefinitionContext)
{
    ASSERT(!context->currentFunctionContext);
    context->currentFunctionContext = CreateFunctionContext(functionDefinitionContext,
                                                            functionType,
                                                            context->constantContext,
                                                            context->typeContext,
                                                            functionName);
    ASSERT(!context->currentFunctionDefinitionContext);
    context->currentFunctionDefinitionContext = functionDefinitionContext;
}
void EndFunctionContext(AstCompileContext * context)
{
    ASSERT(context->currentFunctionContext);
    context->functionContexts.push_back(context->currentFunctionContext);
    context->currentFunctionContext = nullptr;
    ASSERT(context->currentFunctionDefinitionContext);
    context->currentFunctionDefinitionContext = nullptr;
}

Language::ObjectStorageType ComputeObjectStorageType(ASTScope scope,
                                                     Token::Type storageToken)
{
    ASSERT(storageToken == Token::UNKNOWN ||
           storageToken == Token::KW_EXTERN ||
           storageToken == Token::KW_STATIC);

    if (storageToken == Token::KW_EXTERN)
    {
        ASSERT(scope == IN_FILE);
        return Language::ObjectStorageType::IMPORT_OBJECT;
    }
    else if (storageToken == Token::KW_STATIC)
    {
        ASSERT(scope == IN_FILE||
               scope == IN_COMPOUND_STATEMENT);
        if (scope == IN_FILE)
            return Language::ObjectStorageType::GLOBAL_OBJECT;
        else
            return Language::ObjectStorageType::FUNCTION_STATIC_OBJECT;
    }
    else
    {
        if (scope == IN_FILE)
            return Language::ObjectStorageType::GLOBAL_EXPORT_OBJECT;
        else if (scope == IN_PARAM_LIST)
            return Language::ObjectStorageType::PARAM_OBJECT;
        else // IN_COMPOUND_STATEMENT
            return Language::ObjectStorageType::LOCAL_OBJECT;
    }
}

// Simple DSL:
// for id
void PushId(AstCompileContext * context, StringRef id)
{
    context->ids.push_back(id);
}
StringRef PopId(AstCompileContext * context)
{
    StringRef id = context->ids.back();
    context->ids.pop_back();
    return id;
}
void PushNode(AstCompileContext * context, Language::Node * node)
{
    context->irNodes.push_back(node);
}
Language::Node * PopNode(AstCompileContext * context)
{
    Language::Node * node = context->irNodes.back();
    context->irNodes.pop_back();
    return node;
}

// Storage specifier DSL
// 1. set/get must witin (begin, end).
// 2. nested (begin, end) is allowed, be cautious which one is active, current or parent.
// 3. set only once.
void        BeginStorage(AstCompileContext * context, std::vector<Token::Type> && allowedStorageToken)
{
    context->storageTokens.push_back(Token::UNKNOWN);
    context->allowedStorageTokens.emplace_back(allowedStorageToken);
}
void        SetStorage(AstCompileContext * context, Token::Type storageToken)
{
    ASSERT(!context->storageTokens.empty() &&
           context->storageTokens.back() == Token::UNKNOWN);

    bool allowed = false;
    for (Token::Type allowedType : context->allowedStorageTokens.back())
    {
        if (allowedType == storageToken)
        {
            allowed = true;
            break;
        }
    }
    ASSERT(allowed);

    context->storageTokens.back() = storageToken;
}
Token::Type GetStorage(AstCompileContext * context)
{
    ASSERT(!context->storageTokens.empty());
    return context->storageTokens.back();
}
void        EndStorage(AstCompileContext * context)
{
    context->storageTokens.pop_back();
    context->allowedStorageTokens.pop_back();
}

// Type specifier DSL
// 1. add/get must within (begin, end).
// 2. nested (begin, end) is allowed, be cautious which one is active, current or parent.
// 3. combination rule is checked in add.
void             BeginTypeSpecifier(AstCompileContext * context)
{
    context->typeSpecifierBuilders.emplace_back(new TypeSpecifiersBuilder());
}
void             AddTypeSpecifier(AstCompileContext * context,
                                  Token::Type typeSpecifierToken,
                                  Language::Type * typeSpecifierType = nullptr)
{
    context->typeSpecifierBuilders.back()->Add(typeSpecifierToken, typeSpecifierType);
}
Language::Type * GetTypeSpecifier(AstCompileContext * context)
{
    return context->typeSpecifierBuilders.back()->Build(context->typeContext);
}
void             EndTypeSpecifier(AstCompileContext * context)
{
    delete context->typeSpecifierBuilders.back();
    context->typeSpecifierBuilders.pop_back();
}

// Type qualifier DSL
// 1. add/get must within (begin, end).
// 2. nested (begin, end) is allowed, be cautious which one is active, current or parent.
// 3. combination rule is checked in add.
void BeginTypeQualifier(AstCompileContext * context)
{
    context->typeQualifiers.push_back(0);
}
void AddTypeQualifier(AstCompileContext * context, Token::Type typeQualifierToken)
{
    ASSERT(typeQualifierToken == Token::KW_CONST);
    context->typeQualifiers.back() |= Language::TP_CONST;
}
int  GetTypeQualifier(AstCompileContext * context)
{
    return context->typeQualifiers.back();
}
void EndTypeQualifier(AstCompileContext * context)
{
    context->typeQualifiers.pop_back();
}

// Type DSL
void             PushType(AstCompileContext * context, Language::Type * type)
{
    context->types.push_back(type);
}
Language::Type * GetType(AstCompileContext * context)
{
    return context->types.back();
}
Language::Type * PopType(AstCompileContext * context)
{
    Language::Type * type = context->types.back();
    context->types.pop_back();
    return type;
}
Language::Type * ConcatType(AstCompileContext * context,
                            Language::Type * typeSpecifier,
                            int typeQualifier,
                            Language::Type * declarator)
{
    // declarator = {nullptr, array x pointer x function}
    Language::TypeSetProperties(typeSpecifier, typeQualifier);
    if (declarator)
    {
        Language::Type * endType = declarator;
        Language::Type * targetType = nullptr;
        while ((targetType = Language::GetTargetType(endType)) != nullptr)
        {
            endType = targetType;
        }

        switch (endType->name)
        {
            case Language::ARRAY:
                Language::ArraySetTarget(context->typeContext,
                                         Language::AsArray(endType),
                                         typeSpecifier);
                break;
            case Language::POINTER:
                Language::PointerSetTarget(context->typeContext,
                                           Language::AsPointer(endType),
                                           typeSpecifier);
                break;
            case Language::FUNCTION:
                Language::FunctionSetTarget(Language::AsFunction(endType),
                                            typeSpecifier);
                break;
            default: ASSERT(false); break;
        }
        return declarator;
    }
    else
    {
        return typeSpecifier;
    }
}

// rules
// 1. if an ast node generates: at head define local variables, at tail push them on stack.
//      * type specifier, type qualifier, storage use DSL for more check
// 2. ensure region like DSL is correctly used.
// 3. in function body, both DSL and FunctioContext handles current DefinitionContext.

// TODO: mechanism to check what's generated by a child is as expected, 1 StringRef? 1 Type?
void CompileAst(AstCompileContext * context, Ast * ast)
{
    Ast * child = ast->leftChild;

    if (ast->type == TRANSLATION_UNIT)
    {
        EnterDefinitionContext(context,
                               NewDefinitionContext(context, Language::GLOBAL_SCOPE));
        BeginScope(context, IN_FILE);

        while (child)
        {
            CompileAst(context, child);
            child = child->rightSibling;
        }

        EndScope(context);

        ASSERT(!context->globalDefinitionContext);
        context->globalDefinitionContext = context->definitionContexts.back();
        ExitDefinitionContext(context);
    }
    else if (ast->type == FUNCTION_DEFINITION)
    {
        BeginTypeSpecifier(context);
        BeginTypeQualifier(context);
        BeginStorage(context, { Token::KW_EXTERN, Token::KW_STATIC });
        
        // declaration specifiers
        CompileAst(context, child);
        child = child->rightSibling;

        Token::Type         storageToken = GetStorage(context);
        Language::Type *    typeSpecifier = GetTypeSpecifier(context);
        int                 typeQualifier = GetTypeQualifier(context);

        EndStorage(context);
        EndTypeQualifier(context);
        EndTypeSpecifier(context);

        Language::DefinitionContext * functionDefinitionContext = NewDefinitionContext(context,
                                                                                       Language::DefinitionContextScope::FUNCTION_SCOPE);
        
        EnterDefinitionContext(context, functionDefinitionContext);
        
        // declarator
        CompileAst(context, child);
        child = child->rightSibling;
        
        ExitDefinitionContext(context);

        StringRef           functionName = PopId(context);
        Language::Type *    declarator = PopType(context);


        Language::Type *    functionType = ConcatType(context,
                                                      typeSpecifier,
                                                      typeQualifier,
                                                      declarator);

        (void)NewFunctionDefinition(CurrentDefinitionContext(context),
                                    functionName,
                                    functionType,
                                    storageToken == Token::KW_STATIC
                                        ? Language::FunctionStorageType::GLOBAL_FUNCTION
                                        : Language::FunctionStorageType::GLOBAL_EXPORT_FUNCTION,
                                    true);

        EnterDefinitionContext(context, functionDefinitionContext);
        BeginScope(context, IN_COMPOUND_STATEMENT);
        BeginFunctionContext(context, functionName, Language::AsFunction(functionType), functionDefinitionContext);

        // compound statement
        CompileAst(context, child);
        context->currentFunctionContext->functionBody = PopNode(context);

        EndFunctionContext(context);
        EndScope(context);
        ExitDefinitionContext(context);
    }
    else if (ast->type == DECLARATION)
    {
        BeginTypeSpecifier(context);
        BeginTypeQualifier(context);
        BeginStorage(context, { Token::KW_TYPEDEF, Token::KW_EXTERN, Token::KW_STATIC });

        // declaration specifier
        CompileAst(context, child);
        child = child->rightSibling;

        // declarator initializer
        while (child)
        {
            CompileAst(context, child);
            child = child->rightSibling;
        }

        EndStorage(context);
        EndTypeQualifier(context);
        EndTypeSpecifier(context);
    }
    else if (ast->type == DECLARATOR_INITIALIZER)
    {
        Token::Type         storageToken = GetStorage(context);
        Language::Type *    typeSpecifier = GetTypeSpecifier(context);
        int                 typeQualifier = GetTypeQualifier(context);

        // declarator
        CompileAst(context, child);

        StringRef           id = PopId(context);
        Language::Type *    declarator = PopType(context);


        Language::Type *    type = ConcatType(context,
                                              typeSpecifier,
                                              typeQualifier,
                                              declarator);

        if (storageToken == Token::KW_TYPEDEF)
        {
            (void)Language::NewTypeAliasDefinition(CurrentDefinitionContext(context),
                                                   id,
                                                   type);
        }
        else if (type->name == Language::FUNCTION)
        {
            (void)NewFunctionDefinition(CurrentDefinitionContext(context),
                                        id,
                                        type,
                                        storageToken == Token::KW_STATIC
                                            ? Language::FunctionStorageType::GLOBAL_FUNCTION
                                            : Language::FunctionStorageType::IMPORT_FUNCTION,
                                        false);
        }
        else
        {
            Language::TypeSetAssignable(type);
            Language::TypeSetAddressable(type);
            (void)Language::NewObjectDefinition(CurrentDefinitionContext(context),
                                                id,
                                                type,
                                                ComputeObjectStorageType(GetScope(context), storageToken));
        }

        // TODO: initializer
    }
    else if (ast->type == DECLARATOR)
    {
        Language::Type * type = nullptr;
        StringRef id;

        // pointer list
        Language::PointerType * pointerType = nullptr;
        if (child->type == POINTER_LIST)
        {
            CompileAst(context, child);
            child = child->rightSibling;

            pointerType = AsPointer(PopType(context));
        }

        // id / declarator
        Language::Type * innerType = nullptr;
        if (child->type == IDENTIFIER)
        {
            ASSERT(!child->token.text.empty());

            id = child->token.text;
        }
        else
        {
            ASSERT(child->type == DECLARATOR);
            CompileAst(context, child);

            id = PopId(context);
            innerType = PopType(context);
        }
        child = child->rightSibling;

        // array:constant expression / function:parameter list
        // constrain: 1+ array or 1 function
        Language::Type * beginType = innerType;
        Language::Type * endType = innerType;
        int constrain = 0;
        while (child)
        {
            if (child->type == PRIMARY_EXPR)
            {
                ASSERT(constrain < 2);
                constrain = 1;
                ASSERT(child->token.type == Token::CONST_INT);

                Language::ArrayType * arrayType = Language::MakeArray(context->typeContext, child->token.ival);

                if (endType)
                {
                    Language::SetTargetType(context->typeContext, endType, &arrayType->type);
                }
                else
                {
                    ASSERT(!beginType);
                    beginType = &arrayType->type;
                }
                endType = &arrayType->type;
            }
            else
            {
                ASSERT(constrain == 0);
                constrain = 2;
                ASSERT(child->type == PARAMETER_LIST);

                Language::FunctionType * functionType = Language::MakeFunction(context->typeContext);
                
                // call FunctionAddParameter, FunctionSetVarList
                PushType(context, &functionType->type);
                CompileAst(context, child);
                (void)PopType(context);

                if (endType)
                {
                    Language::SetTargetType(context->typeContext, endType, &functionType->type);
                }
                else
                {
                    ASSERT(!beginType);
                    beginType = &functionType->type;
                }
                endType = &functionType->type;
            }
            child = child->rightSibling;
        }

        if (beginType)
        {
            if (pointerType)
            {
                Language::SetTargetType(context->typeContext, endType, &pointerType->type);
            }
            type = beginType;
        }
        else if (pointerType)
        {
            type = &pointerType->type;
        }

        // type can be null
        ASSERT(!id.empty());
        PushId(context, id);
        PushType(context, type);
    }
    else if (ast->type == DIRECT_DECLARATOR)
    {

    }
    else if (ast->type == ABSTRACT_DECLARATOR)
    {

    }
    else if (ast->type == DIRECT_ABSTRACT_DECLARATOR)
    {

    }
    else if (ast->type == PARAMETER_VAR_LIST)
    {

    }
    else if (ast->type == PARAMETER_LIST)
    {
        BeginScope(context, IN_PARAM_LIST);

        Language::FunctionType * functionType = Language::AsFunction(GetType(context));

        bool isFunctionDefinition = CurrentDefinitionContext(context)->scope == Language::DefinitionContextScope::FUNCTION_SCOPE;
        if (!isFunctionDefinition)
        {
            Language::DefinitionContext * functionDefinitionContext = NewDefinitionContext(context,
                                                                                           Language::DefinitionContextScope::FUNCTION_SCOPE);

            EnterDefinitionContext(context, functionDefinitionContext);
        }

        // parameter declaration / parameter var list
        for (; child; child = child->rightSibling)
        {
            if (child->type == PARAMETER_DECLARATION)
            {
                CompileAst(context, child);

                StringRef           id = PopId(context);
                Language::Type *    type = PopType(context);
                Language::FunctionAddParameter(functionType, id, type);
                (void)Language::NewObjectDefinition(CurrentDefinitionContext(context),
                                                    id,
                                                    type,
                                                    ComputeObjectStorageType(GetScope(context), Token::UNKNOWN));
            }
            else
            {
                ASSERT(child->type == PARAMETER_VAR_LIST);
                Language::FunctionSetVarList(functionType);
                break;
            }
        }
        ASSERT(!child || !child->rightSibling);

        if (!isFunctionDefinition)
        {
            ExitDefinitionContext(context);
        }

        EndScope(context);
    }
    else if (ast->type == PARAMETER_DECLARATION)
    {
        StringRef           id;
        Language::Type *    type;

        BeginTypeSpecifier(context);
        BeginTypeQualifier(context);
        BeginStorage(context, {});

        // declaration specifier
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Type *    typeSpecifier = GetTypeSpecifier(context);
        int                 typeQualifier = GetTypeQualifier(context);

        EndStorage(context);
        EndTypeQualifier(context);
        EndTypeSpecifier(context);

        // declarator
        ASSERT(child->type == DECLARATOR);
        CompileAst(context, child);

                            id = PopId(context);
        Language::Type *    declarator = PopType(context);
        
        type = ConcatType(context,
                          typeSpecifier,
                          typeQualifier,
                          declarator);

        ASSERT(!id.empty() && type);
        PushId(context, id);
        PushType(context, type);
    }
    else if (ast->type == IDENTIFIER)
    {

    }
    else if (ast->type == IDENTIFIER_LIST)
    {

    }
    else if (ast->type == TYPE_NAME)
    {

    }
    else if (ast->type == POINTER)
    {
        Language::Type * type = &Language::MakePointer(context->typeContext)->type;

        BeginTypeQualifier(context);

        // type qualifier
        if (child)
        {
            CompileAst(context, child);
            ASSERT(!child->rightSibling);
        }

        int typeQualifier = GetTypeQualifier(context);
        
        EndTypeQualifier(context);

        Language::TypeSetProperties(type, typeQualifier);

        PushType(context, type);
    }
    else if (ast->type == POINTER_LIST)
    {
        Language::Type * type;

        CompileAst(context, child);
        child = child->rightSibling;
        type = PopType(context);

        Language::Type * currType = type;
        Language::Type * nextType;
        while (child)
        {
            CompileAst(context, child);
            child = child->rightSibling;
            nextType = PopType(context);
            Language::PointerSetTarget(context->typeContext,
                                       Language::AsPointer(currType),
                                       nextType);
            currType = nextType;
        }

        PushType(context, type);
    }
    else if (ast->type == DECLARATION_SPECIFIERS)
    {
        while (child)
        {
            CompileAst(context, child);
            child = child->rightSibling;
        }
    }
    else if (ast->type == STORAGE_CLASS_SPECIFIER)
    {
        SetStorage(context, ast->token.type);
    }
    else if (ast->type == TYPE_QUALIFIER)
    {
        AddTypeQualifier(context, ast->token.type);
    }
    else if (ast->type == TYPE_SPECIFIER)
    {
        AddTypeSpecifier(context, ast->token.type);
    }
    else if (ast->type == STRUCT_SPECIFIER)
    {
        // identifier
        StringRef tag;
        if (child->type == IDENTIFIER)
        {
            tag = child->token.text;

            child = child->rightSibling;
        }


        Language::TypeTagDefinition * existingTagDefinition = nullptr;
        Language::TypeTagDefinition * existingParentTagDefinition = nullptr;
        {
            Language::Definition * definition =
                Language::LookupDefinition(CurrentDefinitionContext(context),
                                           tag,
                                           Language::TAG_NAMESPACE,
                                           false);

            existingTagDefinition =
                definition ? Language::AsTypeTagDefinition(definition) : nullptr;

            if (existingTagDefinition == nullptr)
            {
                definition =
                    Language::LookupDefinition(CurrentDefinitionContext(context),
                                               tag,
                                               Language::TAG_NAMESPACE,
                                               true);

                existingParentTagDefinition =
                    definition ? Language::AsTypeTagDefinition(definition) : nullptr;
            }
        }

        // 1. old exists
        //  type
        //      old-incomplete x new-incomplete  => reuse old-incomplete
        //      old-incomplete x new-complete    => make old-incomplete complete
        //      old-complete x new-incomplete    => reuse old-complete
        //      old-complete x new-complete      => error
        //  tag definition
        //      reuse
        // 2. parent-complete exists
        //  type
        //      parent-complete x new-incomplete => reuse parent-complete
        //      parent-complete x new-complete   => create new-complete
        //  tag definition
        //      create
        // 3. otherwise
        //  type
        //      new-incomplete                  => create new-incomplete
        //      new-complete                    => create new-complete
        //  tag definition
        //      create

        Language::StructType * structType = nullptr;
        
        bool needCreateTagDefinition = false;

        if (existingTagDefinition != nullptr)
        {
            bool isOldIncomplete = Language::IsIncomplete(existingTagDefinition->taggedType);
            bool isNewIncomplete = (child == nullptr);

            ASSERT(isOldIncomplete || isNewIncomplete);

            structType = Language::AsStruct(existingTagDefinition->taggedType);
        }
        else if (existingParentTagDefinition != nullptr &&
                 !Language::IsIncomplete(existingParentTagDefinition->taggedType))
        {
            bool isNewIncomplete = (child == nullptr);

            if (isNewIncomplete)
                structType = Language::AsStruct(existingParentTagDefinition->taggedType);
            else
                structType = Language::MakeStruct(context->typeContext);

            needCreateTagDefinition = true;
        }
        else
        {
            structType = Language::MakeStruct(context->typeContext);
            needCreateTagDefinition = true;
        }

        if (needCreateTagDefinition)
        {
            (void)NewTypeTagDefinition(CurrentDefinitionContext(context),
                                       tag,
                                       &structType->type);
        }

        // struct declarations
        if (child)
        {
            do
            {
                // struct declaration
                {
                    Ast * subChild = child->leftChild;
                    BeginTypeSpecifier(context);
                    BeginTypeQualifier(context);

                    // type specifier / type qualifier
                    ASSERT(subChild && (IsAstTypeSpecifier(subChild->type) ||
                                        IsAstTypeQualifier(subChild->type)));
                    do
                    {
                        CompileAst(context, subChild);
                        subChild = subChild->rightSibling;
                    } while (subChild && (IsAstTypeSpecifier(subChild->type) ||
                                          IsAstTypeQualifier(subChild->type)));

                    Language::Type *    typeSpecifier = GetTypeSpecifier(context);
                    int                 typeQualifier = GetTypeQualifier(context);

                    EndTypeQualifier(context);
                    EndTypeSpecifier(context);

                    // struct declarator
                    do
                    {
                        ASSERT(subChild && subChild->type == STRUCT_DECLARATOR);
                        CompileAst(context, subChild);
                        subChild = subChild->rightSibling;

                        StringRef           mname = PopId(context);
                        Language::Type *    declarator = PopType(context);

                        Language::Type *    mtype = ConcatType(context,
                                                               typeSpecifier,
                                                               typeQualifier,
                                                               declarator);
                        Language::StructAddMember(structType, mname, mtype);
                    } while (subChild);
                }
                child = child->rightSibling;
            } while (child);
            Language::StructDone(structType);
        }

        AddTypeSpecifier(context, Token::KW_STRUCT, &structType->type);
    }
    else if (ast->type == UNION_SPECIFIER)
    {
        // TODO: support union specifier

    }
    else if (ast->type == STRUCT_DECLARATION)
    {
        ASSERT(false);
    }
    else if (ast->type == STRUCT_DECLARATOR)
    {
        ASSERT(child && child->type == DECLARATOR);
        CompileAst(context, child);
    }
    else if (ast->type == ENUM_SPECIFIER)
    {
        // identifier
        StringRef tag;
        if (child->type == IDENTIFIER)
        {
            tag = child->token.text;

            child = child->rightSibling;
        }

        // enum constant
        Language::EnumType *    enumType = Language::MakeEnum(context->typeContext);
        bool                    isEnumDefinition = (child != nullptr);
        if (child)
        {
            StringRef enumConstName;
            int nextEnumConstValue = 0;
            for (ASSERT(child);
                 child;
                 ++nextEnumConstValue, child = child->rightSibling)
            {
                ASSERT(child->type == ENUM_CONSTANT);

                enumConstName = child->token.text;
                if (child->leftChild)
                {
                    ASSERT(child->leftChild->type == CONSTANT_EXPR);
                    nextEnumConstValue = child->leftChild->token.ival;
                }

                Language::EnumAddConst(enumType,
                                       enumConstName,
                                       nextEnumConstValue);

                (void)NewEnumConstDefinition(CurrentDefinitionContext(context),
                                             enumConstName,
                                             isEnumDefinition
                                                ? &enumType->type
                                                : nullptr,
                                             nextEnumConstValue);
            }
        }

        (void)NewTypeTagDefinition(CurrentDefinitionContext(context),
                                   tag,
                                   &enumType->type);

        AddTypeSpecifier(context, Token::KW_ENUM, &enumType->type);
    }
    else if (ast->type == ENUM_CONSTANT)
    {

    }
    else if (ast->type == TYPEDEF_NAME)
    {

    }
    else if (ast->type == INITIALIZER)
    {

    }
    else if (ast->type == INITIALIZER_LIST)
    {
    }
    else if (ast->type == STMT)
    {

    }
    else if (ast->type == STMT_LIST)
    {

    }
    else if (ast->type == LABELED_STMT)
    {

    }
    else if (ast->type == LABEL_STMT)
    {
        StringRef label = child->token.text;

        (void)Language::NewGotoLabelDefinition(context->currentFunctionDefinitionContext,
                                               label,
                                               true);

        child = child->rightSibling;
        CompileAst(context, child);

        Language::Node * statement = PopNode(context);

        Language::Node * labelStatement = Language::LabelStatement(context->currentFunctionContext,
                                                                   label,
                                                                   statement);

        PushNode(context, labelStatement);
    }
    else if (ast->type == CASE_STMT)
    {
        //ASSERT(child->type == CONSTANT_EXPR);
        int caseValue = child->token.ival;

        child = child->rightSibling;
        CompileAst(context, child);

        Language::Node * statement = PopNode(context);

        Language::Node * caseStatement = Language::CaseStatement(context->currentFunctionContext,
                                                                 (u64)caseValue,
                                                                 statement);

        PushNode(context, caseStatement);
    }
    else if (ast->type == DEFAULT_STMT)
    {
        CompileAst(context, child);

        Language::Node * statement = PopNode(context);

        Language::Node * defaultStatement = Language::DefaultStatement(context->currentFunctionContext,
                                                                       statement);

        PushNode(context, defaultStatement);
    }
    else if (ast->type == COMPOUND_STMT)
    {
        Language::DefinitionContext * blockDefinitionContext = NewDefinitionContext(context,
                                                                                       Language::BLOCK_SCOPE);

        EnterDefinitionContext(context, blockDefinitionContext);

        Language::Node * compoundStatement = Language::CompoundStatement_Begin(context->currentFunctionContext,
                                                                               blockDefinitionContext);

        // declaration*
        while (child && child->type == DECLARATION)
        {
            CompileAst(context, child);
            child = child->rightSibling;
        }

        // statement*
        while (child)
        {
            CompileAst(context, child);
            child = child->rightSibling;

            Language::CompoundStatement_AddStatement(compoundStatement,
                                                     PopNode(context));
        }

        Language::CompoundStatement_End(context->currentFunctionContext);

        ExitDefinitionContext(context);

        PushNode(context, compoundStatement);
    }
    else if (ast->type == EXPRESSION_STMT)
    {
        Language::Node * exprStatement = nullptr;

        if (child)
        {
            CompileAst(context, child);

            exprStatement = Language::ExpressionStatement(PopNode(context));
        }
        else
        {
            exprStatement = Language::ExpressionStatement(
                Language::EmptyExpression(context->currentFunctionContext)
            );
        }

        PushNode(context, exprStatement);
    }
    else if (ast->type == SELECTION_STMT)
    {

    }
    else if (ast->type == IF_ELSE_STMT)
    {
        Language::Node * ifStatement = Language::IfStatement_Begin();

        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * expr = PopNode(context);

        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * ifBody = PopNode(context);

        Language::Node * elseBody = nullptr;
        if (child)
        {
            CompileAst(context, child);

            elseBody = PopNode(context);
        }

        Language::IfStatement_SetExpr(ifStatement, expr);
        Language::IfStatement_SetIfBody(ifStatement, ifBody);
        if (elseBody)
            Language::IfStatement_SetElseBody(ifStatement, elseBody);
        Language::IfStatement_End(ifStatement);

        PushNode(context, ifStatement);
    }
    else if (ast->type == SWITCH_STMT)
    {
        Language::Node * switchStatement = Language::SwitchStatement_Begin(context->currentFunctionContext);

        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * expr = PopNode(context);

        CompileAst(context, child);

        Language::Node * body = PopNode(context);

        Language::SwitchStatement_SetExpr(switchStatement, expr);
        Language::SwitchStatement_SetBody(switchStatement, body);
        Language::SwitchStatement_End(context->currentFunctionContext);

        PushNode(context, switchStatement);
    }
    else if (ast->type == ITERATION_STMT)
    {

    }
    else if (ast->type == WHILE_STMT)
    {
        Language::Node * whileStatement = Language::WhileStatement_Begin(context->currentFunctionContext);

        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * expr = PopNode(context);

        CompileAst(context, child);

        Language::Node * body = PopNode(context);

        Language::WhileStatement_SetExpr(whileStatement, expr);
        Language::WhileStatement_SetBody(whileStatement, body);
        Language::WhileStatement_End(context->currentFunctionContext);

        PushNode(context, whileStatement);
    }
    else if (ast->type == DO_WHILE_STMT)
    {
        Language::Node * doWhileStatement = Language::DoWhileStatement_Begin(context->currentFunctionContext);

        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * body = PopNode(context);

        CompileAst(context, child);

        Language::Node * expr = PopNode(context);

        Language::DoWhileStatement_SetBody(doWhileStatement, body);
        Language::DoWhileStatement_SetExpr(doWhileStatement, expr);
        Language::DoWhileStatement_End(context->currentFunctionContext);

        PushNode(context, doWhileStatement);
    }
    else if (ast->type == FOR_STMT)
    {
        Language::Node * forStatement = Language::ForStatement_Begin(context->currentFunctionContext);

        Language::Node * preExpr = nullptr;
        if (child->type != EXPR)
        {
            CompileAst(context, child);
            preExpr = PopNode(context);
        }
        else
        {
            preExpr = Language::EmptyExpression(context->currentFunctionContext);
        }
        child = child->rightSibling;

        Language::Node * loopExpr = nullptr;
        if (child->type != EXPR)
        {
            CompileAst(context, child);
            loopExpr = PopNode(context);
        }
        else
        {
            loopExpr = Language::EmptyExpression(context->currentFunctionContext);
        }
        child = child->rightSibling;

        Language::Node * postExpr = nullptr;
        if (child->type != EXPR)
        {
            CompileAst(context, child);
            postExpr = PopNode(context);
        }
        else
        {
            postExpr = Language::EmptyExpression(context->currentFunctionContext);
        }
        child = child->rightSibling;

        CompileAst(context, child);
        
        Language::Node * body = PopNode(context);

        Language::ForStatement_SetPreExpr(forStatement, preExpr);
        Language::ForStatement_SetLoopExpr(forStatement, loopExpr);
        Language::ForStatement_SetPostExpr(forStatement, postExpr);
        Language::ForStatement_SetBody(forStatement, body);
        Language::ForStatement_End(context->currentFunctionContext);

        PushNode(context, forStatement);
    }
    else if (ast->type == JUMP_STMT)
    {

    }
    else if (ast->type == GOTO_STMT)
    {
        Language::Node * gotoStatement = Language::GotoStatement(context->currentFunctionContext,
                                                                 child->token.text);

        PushNode(context, gotoStatement);
    }
    else if (ast->type == CONTINUE_STMT)
    {
        Language::Node * continueStatement = Language::ContinueStatement(context->currentFunctionContext);

        PushNode(context, continueStatement);
    }
    else if (ast->type == BREAK_STMT)
    {
        Language::Node * breakStatement = Language::BreakStatement(context->currentFunctionContext);

        PushNode(context, breakStatement);
    }
    else if (ast->type == RETURN_STMT)
    {
        Language::Node * expr = nullptr;

        if (child)
        {
            CompileAst(context, child);
            
            expr = PopNode(context);
        }
        else
        {
            expr = Language::EmptyExpression(context->currentFunctionContext);
        }

        Language::Node * returnStatement = Language::ReturnStatement(expr);

        PushNode(context, returnStatement);
    }
    else if (ast->type == EXPR)
    {

    }
    else if (ast->type == PRIMARY_EXPR)
    {
        Language::Node * node = nullptr;

        switch (ast->token.type)
        {
            case Token::ID:             node = Language::IdExpression(context->currentFunctionContext, ast->token.text); break;
            case Token::CONST_INT:      node = Language::ConstantExpression(context->currentFunctionContext, ast->token.ival); break;
            case Token::CONST_CHAR:     node = Language::ConstantExpression(context->currentFunctionContext, (int)ast->token.cval); break;
            case Token::CONST_FLOAT:    node = Language::ConstantExpression(context->currentFunctionContext, (float)ast->token.fval); break;
            case Token::STRING:         node = Language::ConstantExpression(context->currentFunctionContext, ast->token.text); break;
            default:                    ASSERT(false); break;
        }

        PushNode(context, node);
    }
    else if (ast->type == POSTFIX_EXPR)
    {
        // unary expr
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * primary = PopNode(context);

        Language::Node * postfix = nullptr;
        if (child)
        {
            if (child->type == IDENTIFIER) // a.id, a->id
            {
                if (ast->token.type == Token::OP_DOT)
                {
                    postfix = Language::MemberOfExpression(context->currentFunctionContext,
                                                           primary,
                                                           child->token.text);
                }
                else
                {
                    postfix = Language::IndirectMemberOfExpression(context->currentFunctionContext,
                                                                   primary,
                                                                   child->token.text);
                }
            }
            else // a(), a[]
            {
                if (child->type == ARGUMENT_EXPR_LIST)
                {
                    std::vector<Language::Node *> arguments;
                    for (Ast * arg = child->leftChild;
                         arg;
                         arg = arg->rightSibling)
                    {
                        CompileAst(context, arg);
                        arguments.push_back(PopNode(context));
                    }

                    postfix = Language::CallExpression(context->currentFunctionContext,
                                                       primary,
                                                       arguments);
                }
                else
                {
                    CompileAst(context, child);

                    Language::Node * index = PopNode(context);

                    postfix = Language::SubscriptExpression(context->currentFunctionContext,
                                                            primary,
                                                            index);
                }
            }
        }
        else // ++, --
        {
            if (ast->token.type == Token::OP_INC)
            {
                postfix = Language::PostIncExpression(context->currentFunctionContext,
                                                      primary);
            }
            else if (ast->token.type == Token::OP_DEC)
            {
                postfix = Language::PostDecExpression(context->currentFunctionContext,
                                                      primary);
            }
        }
        ASSERT(postfix);

        PushNode(context, postfix);
    }
    else if (ast->type == ARGUMENT_EXPR_LIST)
    {
        ASSERT(false);
    }
    else if (ast->type == CONSTANT_EXPR)
    {
        ASSERT(false);
    }
    else if (ast->type == UNARY_EXPR)
    {
        CompileAst(context, child);

        Language::Node * postfix = PopNode(context);

        Language::Node * unary = nullptr;
        switch (ast->token.type)
        {
            case Token::OP_INC:     unary = Language::IncExpression(context->currentFunctionContext, postfix); break;
            case Token::OP_DEC:     unary = Language::DecExpression(context->currentFunctionContext, postfix); break;
            case Token::BIT_AND:    unary = Language::GetAddressExpression(context->currentFunctionContext, postfix); break;
            case Token::OP_MUL:     unary = Language::IndirectExpression(context->currentFunctionContext, postfix); break;
            case Token::OP_ADD:     unary = Language::PositiveExpression(context->currentFunctionContext, postfix); break;
            case Token::OP_SUB:     unary = Language::NegativeExpression(context->currentFunctionContext, postfix); break;
            case Token::BIT_NOT:    unary = Language::BitNotExpression(context->currentFunctionContext, postfix); break;
            case Token::BOOL_NOT:   unary = Language::BoolNotExpression(context->currentFunctionContext, postfix); break;
            case Token::KW_SIZEOF:  unary = Language::SizeOfExpression(context->currentFunctionContext, postfix); break;
            default:                ASSERT(false); break;
        }
        
        PushNode(context, unary);
    }
    else if (ast->type == CAST_EXPR)
    {
        // TODO: wait parser support
    }
    else if (ast->type == MUL_EXPR)
    {
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * left = PopNode(context);

        CompileAst(context, child);

        Language::Node * right = PopNode(context);

        Language::Node * mul = nullptr;
        switch (ast->token.type)
        {
            case Token::OP_MUL: mul = Language::MulExpression(context->currentFunctionContext, left, right); break;
            case Token::OP_DIV: mul = Language::DivExpression(context->currentFunctionContext, left, right); break;
            case Token::OP_MOD: mul = Language::ModExpression(context->currentFunctionContext, left, right); break;
            default: ASSERT(false); break;
        }

        PushNode(context, mul);
    }
    else if (ast->type == ADD_EXPR)
    {
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * left = PopNode(context);

        CompileAst(context, child);

        Language::Node * right = PopNode(context);

        Language::Node * add = ast->token.type == Token::OP_ADD
                               ? Language::AddExpression(context->currentFunctionContext, left, right)
                               : Language::SubExpression(context->currentFunctionContext, left, right);
        PushNode(context, add);
    }
    else if (ast->type == SHIFT_EXPR)
    {
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * left = PopNode(context);

        CompileAst(context, child);

        Language::Node * right = PopNode(context);

        Language::Node * shift = nullptr;
        if (ast->token.type == Token::BIT_SHL)
        {
            shift = Language::ShiftLeftExpression(context->currentFunctionContext,
                                                  left,
                                                  right);
        }
        else
        {
            shift = Language::ShiftRightExpression(context->currentFunctionContext,
                                                   left,
                                                   right);
        }

        PushNode(context, shift);
    }
    else if (ast->type == REL_EXPR)
    {
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * left = PopNode(context);

        CompileAst(context, child);

        Language::Node * right = PopNode(context);

        Language::Node * rel = nullptr;
        switch (ast->token.type)
        {
            case Token::REL_GT: rel = Language::GtExpression(context->currentFunctionContext, left, right); break;
            case Token::REL_GE: rel = Language::GeExpression(context->currentFunctionContext, left, right); break;
            case Token::REL_LT: rel = Language::LtExpression(context->currentFunctionContext, left, right); break;
            case Token::REL_LE: rel = Language::LeExpression(context->currentFunctionContext, left, right); break;
            default:            ASSERT(false); break;
        }

        PushNode(context, rel);
    }
    else if (ast->type == EQ_EXPR)
    {
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * left = PopNode(context);

        CompileAst(context, child);

        Language::Node * right = PopNode(context);

        Language::Node * eq = nullptr;
        switch (ast->token.type)
        {
            case Token::REL_EQ: eq = Language::EqExpression(context->currentFunctionContext, left, right); break;
            case Token::REL_NE: eq = Language::NeExpression(context->currentFunctionContext, left, right); break;
            default:            ASSERT(false); break;
        }

        PushNode(context, eq);
    }
    else if (ast->type == BIT_AND_EXPR)
    {
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * left = PopNode(context);

        CompileAst(context, child);

        Language::Node * right = PopNode(context);

        Language::Node * bitAnd = Language::BitAndExpression(context->currentFunctionContext,
                                                             left,
                                                             right);
        PushNode(context, bitAnd);
    }
    else if (ast->type == BIT_XOR_EXPR)
    {
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * left = PopNode(context);

        CompileAst(context, child);

        Language::Node * right = PopNode(context);

        Language::Node * bitXor = Language::BitXorExpression(context->currentFunctionContext,
                                                             left,
                                                             right);
        PushNode(context, bitXor);
    }
    else if (ast->type == BIT_OR_EXPR)
    {
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * left = PopNode(context);

        CompileAst(context, child);

        Language::Node * right = PopNode(context);

        Language::Node * bitOr = Language::BitOrExpression(context->currentFunctionContext,
                                                           left,
                                                           right);
        PushNode(context, bitOr);
    }
    else if (ast->type == AND_EXPR)
    {
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * left = PopNode(context);

        CompileAst(context, child);

        Language::Node * right = PopNode(context);

        Language::Node * boolAnd = Language::BoolAndExpression(context->currentFunctionContext,
                                                               left,
                                                               right);
        PushNode(context, boolAnd);
    }
    else if (ast->type == OR_EXPR)
    {
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * left = PopNode(context);

        CompileAst(context, child);

        Language::Node * right = PopNode(context);

        Language::Node * boolOr = Language::BoolOrExpression(context->currentFunctionContext,
                                                             left,
                                                             right);
        PushNode(context, boolOr);
    }
    else if (ast->type == COND_EXPR)
    {
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * a = PopNode(context);

        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * b = PopNode(context);
        
        CompileAst(context, child);

        Language::Node * c = PopNode(context);

        Language::Node * cond = Language::ConditionExpression(context->currentFunctionContext,
                                                              a,
                                                              b,
                                                              c);
        PushNode(context, cond);
    }
    else if (ast->type == ASSIGN_EXPR)
    {
        CompileAst(context, child);
        child = child->rightSibling;

        Language::Node * left = PopNode(context);

        CompileAst(context, child);

        Language::Node * right = PopNode(context);

        Language::Node * assign = Language::AssignExpression(context->currentFunctionContext,
                                                             left,
                                                             right);

        PushNode(context, assign);
    }
    else if (ast->type == COMMA_EXPR)
    {
        std::vector<Language::Node *> exprs;

        Ast * first = ast;
        while (first->leftChild->type == COMMA_EXPR)
            first = first->leftChild;

        CompileAst(context, first->leftChild);
        exprs.push_back(PopNode(context));

        while (true)
        {
            CompileAst(context, first->leftChild->rightSibling);
            exprs.push_back(PopNode(context));

            if (first == ast)
                break;
            else
                first = first->parent;
        }

        Language::Node * comma = Language::CommaExpression(exprs);

        PushNode(context, comma);
    }
    else
    {
    }
}

