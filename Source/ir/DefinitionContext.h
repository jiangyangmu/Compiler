#pragma once

#include "../Util/Common.h"
#include "../IR/TypeContext.h"

namespace Language {

enum DefinitionType
{
    OBJECT_DEFINITION,
    FUNCTION_DEFINITION,
    ENUM_CONST_DEFINITION,
    TYPE_ALIAS_DEFINITION,
    TYPE_TAG_DEFINITION,
    GOTO_LABEL_DEFINITION,
};
struct Definition
{
    StringRef       name;
    DefinitionType  type;
};

// TODO: remove visibility concept
enum ObjectStorageType
{
    LOCAL_OBJECT,
    GLOBAL_OBJECT,        // s//STATIC
    GLOBAL_EXPORT_OBJECT, // s//STATIC_EXPORT
    IMPORT_OBJECT,
    PARAM_OBJECT,
    FUNCTION_STATIC_OBJECT,
};
struct ObjectDefinition
{
    Definition          def;
    Type *              objType;
    ObjectStorageType   objStorageType;
    void *              objValue;
};

enum FunctionStorageType
{
    GLOBAL_FUNCTION,
    GLOBAL_EXPORT_FUNCTION,
    IMPORT_FUNCTION,
};
struct FunctionDefinition
{
    Definition          def;
    Type *              funcType;
    FunctionStorageType funcStorageType;
    bool                hasFuncBody;
};

struct EnumConstDefinition
{
    Definition          def;
    Type *              enumConstType;
    int                 enumConstValue;
};

struct TypeTagDefinition
{
    Definition          def;
    Type *              taggedType;
};

struct TypeAliasDefinition
{
    Definition          def;
    Type *              aliasedType;
};

struct GotoLabelDefinition
{
    Definition          def;
    bool                isDefined;
};

ObjectDefinition *      AsObjectDefinition(Definition * definition);
FunctionDefinition *    AsFunctionDefinition(Definition * definition);
EnumConstDefinition *   AsEnumConstDefinition(Definition * definition);
TypeTagDefinition *     AsTypeTagDefinition(Definition * definition);
TypeAliasDefinition *   AsTypeAliasDefinition(Definition * definition);
GotoLabelDefinition *   AsGotoLabelDefinition(Definition * definition);

// != GotoLabelDefinition
Type *                  ExtractDefinitionCType(Definition * definition);

// Build

struct DefinitionContext;

Definition * NewObjectDefinition(DefinitionContext * context,
                                 StringRef name,
                                 Type * objType,
                                 ObjectStorageType objStorageType);

Definition * NewFunctionDefinition(DefinitionContext * context,
                                   StringRef name,
                                   Type * funcType,
                                   FunctionStorageType funcStorageType,
                                   bool hasFuncBody);

Definition * NewEnumConstDefinition(DefinitionContext * context,
                                    StringRef name,
                                    Type * enumConstType,
                                    int enumConstValue);

Definition * NewTypeTagDefinition(DefinitionContext * context,
                                  StringRef name,
                                  Type * taggedType);

Definition * NewTypeAliasDefinition(DefinitionContext * context,
                                    StringRef name,
                                    Type * aliasedType);

Definition * NewGotoLabelDefinition(DefinitionContext * context,
                                    StringRef name,
                                    bool isDefined);

// Manage

enum DefinitionContextScope
{
    GLOBAL_SCOPE,
    FUNCTION_SCOPE,
    BLOCK_SCOPE,
};
enum DefinitionContextNamespace
{
    ID_NAMESPACE,
    TAG_NAMESPACE,
    LABEL_NAMESPACE,
};
struct DefinitionContext
{
    DefinitionContext *         parent;
    DefinitionContext *         next;
    DefinitionContext *         firstChild;
    DefinitionContextScope      scope;

    std::map<DefinitionContextNamespace, std::vector<Definition *>> definitions;
};

// To create root, set parent == nullptr.
DefinitionContext * CreateDefinitionContext(DefinitionContext * parent,
                                            DefinitionContextScope scope);

// Query

// searchParent only used for non-label definitions.
Definition * LookupDefinition(DefinitionContext * currentContext,
                              StringRef name,
                              DefinitionContextNamespace ns,
                              bool searchParent);

// Debug

void PrintDefinition(Definition * definition);
void PrintDefinitionContext(DefinitionContext * context);

}