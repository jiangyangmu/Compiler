#include "FunctionContext.h"

#include <iostream>

#include "CallingConvention.h"
#include "../Base/Bits.h"

namespace Language {

void    AddChild(Node * parent, Node * child)
{
    ASSERT(parent && child && !child->up);
    
    child->up = parent;
    
    if (!parent->down)
    {
        parent->down = child;
    }
    else
    {
        Node * last = parent->down;
        while (last->right)
            last = last->right;
        last->right = child;
    }
}

int     CountChild(Node * node)
{
    ASSERT(node);
    int n = 0;
    Node * next = node->down;
    while (next)
    {
        ++n;
        next = next->right;
    }
    return n;
}

Node *  LastChild(Node * parent)
{
    ASSERT(parent);

    Node * lastChild = parent->down;
    
    if (lastChild)
    {
        while (lastChild->right)
        {
            lastChild = lastChild->right;
        }
    }

    return lastChild;
}

Node *  MakeNode(NodeType type)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = type;

    if (type > BEGIN_STATEMENT && type < END_STATEMENT)
    {
        node->stmt.context = nullptr;
    }
    else if ((type > BEGIN_EXPRESSION && type < END_EXPRESSION) ||
             type == EMPTY_EXPRESSION)
    {
        node->expr.type = nullptr;
        node->expr.loc.type = NO_WHERE;
    }

    return node;
}

void    DestroyNodeTree(Node * root)
{
    if (root)
    {
        Node * child = root->down;
        while (child)
        {
            Node * nextChild = child->right;
            DestroyNodeTree(child);
            child = nextChild;
        }
        delete root;
    }
}

std::string NodeDebugString(Node * node)
{
    std::string s;
    switch (node->type)
    {
        case STMT_COMPOUND:     s = "compound"; break;
        case STMT_EXPRESSION:   s = "expression"; break;
        case STMT_IF:           s = "if"; break;
        case STMT_WHILE:        s = "while"; break;
        case STMT_DO_WHILE:     s = "do_while"; break;
        case STMT_FOR:          s = "for"; break;
        case STMT_BREAK:        s = "break"; break;
        case STMT_CONTINUE:     s = "continue"; break;
        case STMT_RETURN:       s = "return"; break;
        case STMT_SWITCH:       s = "switch"; break;
        case STMT_CASE:         s = "case"; break;
        case STMT_DEFAULT:      s = "default"; break;
        case EXPR_DATA:         s = "data"; break;
        case EXPR_CALL:         s = "call"; break;
        case EXPR_CVT_SI2SI:    s = "cvt_si2si"; break;
        case EXPR_CVT_SI2UI:    s = "cvt_si2ui"; break;
        case EXPR_CVT_UI2SI:    s = "cvt_ui2si"; break;
        case EXPR_CVT_UI2UI:    s = "cvt_ui2ui"; break;
        case EXPR_CVT_F2F:      s = "cvt_f2f"; break;
        case EXPR_CVT_SI2F:     s = "cvt_si2f"; break;
        case EXPR_CVT_F2SI:     s = "cvt_f2si"; break;
        case EXPR_CVT_I2B:      s = "cvt_i2b"; break;
        case EXPR_CVT_B2I:      s = "cvt_b2i"; break;
        case EXPR_CVT_F2B:      s = "cvt_f2b"; break;
        case EXPR_CVT_B2F:      s = "cvt_b2f"; break;
        case EXPR_CVT_REINTERP: s = "cvt_reinterp"; break;
        case EXPR_BOOL_NOT:     s = "bool_not"; break;
        case EXPR_BOOL_AND:     s = "bool_and"; break;
        case EXPR_BOOL_OR:      s = "bool_or"; break;
        case EXPR_IADD:         s = "iadd"; break;
        case EXPR_ISUB:         s = "isub"; break;
        case EXPR_IMUL:         s = "imul"; break;
        case EXPR_IDIV:         s = "idiv"; break;
        case EXPR_IMOD:         s = "imod"; break;
        case EXPR_INOT:         s = "inot"; break;
        case EXPR_IAND:         s = "iand"; break;
        case EXPR_IOR:          s = "ior"; break;
        case EXPR_IXOR:         s = "ixor"; break;
        case EXPR_ISHL:         s = "ishl"; break;
        case EXPR_ISHR:         s = "ishr"; break;
        case EXPR_IEQ:          s = "ieq"; break;
        case EXPR_INE:          s = "ine"; break;
        case EXPR_ILT:          s = "ilt"; break;
        case EXPR_ILE:          s = "ile"; break;
        case EXPR_IGE:          s = "ige"; break;
        case EXPR_IGT:          s = "igt"; break;
        case EXPR_FNEG:         s = "fneg"; break;
        case EXPR_FADD:         s = "fadd"; break;
        case EXPR_FSUB:         s = "fsub"; break;
        case EXPR_FMUL:         s = "fmul"; break;
        case EXPR_FDIV:         s = "fdiv"; break;
        case EXPR_FEQ:          s = "feq"; break;
        case EXPR_FNE:          s = "fne"; break;
        case EXPR_FLT:          s = "flt"; break;
        case EXPR_FLE:          s = "fle"; break;
        case EXPR_FGE:          s = "fge"; break;
        case EXPR_FGT:          s = "fgt"; break;
        case EXPR_PADDSI:       s = "paddsi"; break;
        case EXPR_PADDUI:       s = "paddui"; break;
        case EXPR_PSUBSI:       s = "psubsi"; break;
        case EXPR_PSUBUI:       s = "psubui"; break;
        case EXPR_PDIFF:        s = "pdiff"; break;
        case EXPR_PEQ:          s = "peq"; break;
        case EXPR_PNE:          s = "pne"; break;
        case EXPR_PLT:          s = "plt"; break;
        case EXPR_PLE:          s = "ple"; break;
        case EXPR_PGE:          s = "pge"; break;
        case EXPR_PGT:          s = "pgt"; break;
        case EXPR_PIND:         s = "pind"; break;
        case EXPR_PNEW:         s = "pnew"; break;
        case EXPR_MCOPY:        s = "mcopy"; break;
        case EXPR_MDUP:         s = "mdup"; break;
        case EXPR_MADDUI:       s = "maddui"; break;
        case EXPR_MADDSI:       s = "maddsi"; break;
        case EXPR_CONDITION:    s = "condition"; break;
        case EXPR_ELIST:        s = "elist"; break;
        case EMPTY_EXPRESSION:  s = "empty_expr"; break;
        default: ASSERT(false); break;
    }

    return s;
}

FunctionContext * CreateFunctionContext(DefinitionContext * functionDefinitionContext,
                                        FunctionType * functionType,
                                        ConstantContext * constantContext,
                                        TypeContext * typeContext,
                                        StringRef functionName)
{
    FunctionContext * functionContext = new FunctionContext;

    functionContext->functionName = functionName.toString();
    functionContext->functionType = functionType;
    functionContext->constantContext = constantContext;
    functionContext->typeContext = typeContext;
    functionContext->functionDefinitionContext = functionDefinitionContext;

    functionContext->functionBody = nullptr;

    functionContext->nextUniqueLabel = 0;

    return functionContext;
}

Node * WrapCastNode(Node * node, Type * toType)
{
    Type * fromType = node->expr.type;

    if (TypeEqual(fromType, toType))
    {
        return node;
    }

    Node * castNode = nullptr;

    // integer  -> integer  = widen/narrow + sign
    // float    -> float    = widen/narrow
    // float   <-> integer  = data lose
    // *        -> bool     = 0/1
    // pointer  -> pointer  = interpret change

    if (IsIntegral(fromType))
    {
        if (IsIntegral(toType))
            castNode = MakeNode(EXPR_CVT_SI2SI);
        else if (IsFloating(toType))
            castNode = MakeNode(EXPR_CVT_SI2F);
        else if (IsBool(toType))
            castNode = MakeNode(EXPR_CVT_I2B);
    }
    else if (IsFloating(fromType))
    {
        if (IsIntegral(toType))
            castNode = MakeNode(EXPR_CVT_F2SI);
        else if (IsFloating(toType))
            castNode = MakeNode(EXPR_CVT_F2F);
        else if (IsBool(toType))
            castNode = MakeNode(EXPR_CVT_F2B);
    }
    else if (IsPointer(fromType) && IsPointer(toType))
    {
        castNode = MakeNode(EXPR_CVT_REINTERP);
    }
    ASSERT(castNode);

    castNode->expr.type = toType;
    castNode->expr.loc.type = castNode->type == EXPR_CVT_REINTERP
        ? SAME_AS_FIRST_CHILD
        : NEED_ALLOC;

    AddChild(castNode, node);

    return castNode;
}

Node * EmptyExpression(FunctionContext * context)
{
    Node * node = MakeNode(EMPTY_EXPRESSION);
    return node;
}

Location GetArgumentLocation(FunctionType * functionType,
                             StringRef argumentName)
{
    ParameterPassingCalleeProtocol calleeProtocol(functionType);

    size_t argumentIndex = calleeProtocol.IsReturnValueAddressAsFirstParameter()
        ? 1
        : 0;

    while (true)
    {
        ASSERT(functionType->memberType[argumentIndex]);
        if (argumentName == functionType->memberName[argumentIndex])
            break;
        else
            ++argumentIndex;
    }

    Location loc;
    loc.type = BP_OFFSET;
    loc.offsetValue = 16 + argumentIndex * 8;
    return loc;
    //return calleeProtocol.GetParameterLocation(argumentIndex);
}
Node * IdExpression(FunctionContext * context,
                    StringRef id)
{
    Definition * definition = LookupDefinition(context->currentDefinitionContext.back(),
                                               id,
                                               DefinitionContextNamespace::ID_NAMESPACE,
                                               true);

    ASSERT(definition);
    
    if (definition->type == ENUM_CONST_DEFINITION)
    {
        return ConstantExpression(context, AsEnumConstDefinition(definition)->enumConstValue);
    }

    ASSERT(definition->type == OBJECT_DEFINITION || definition->type == FUNCTION_DEFINITION);

    Node * node = MakeNode(EXPR_DATA);

    // function                 -> label
    // import object            -> label
    // global (export) object   -> label
    // function static object   -> label
    // argument object          -> GetArgumentLocation(functionType, argumentIndex)
    // local object             -> delay (search localObjectOffsets, not ready yet)
    // EX: array object         -> alloc for pointer
    if (definition->type == FUNCTION_DEFINITION)
    {
        node->expr.loc.type = LocationType::LABEL;
        node->expr.loc.labelValue = new StringRef(id);
    }
    else
    {
        ObjectDefinition * objectDefinition = AsObjectDefinition(definition);
        switch (objectDefinition->objStorageType)
        {
            case IMPORT_OBJECT:
            case GLOBAL_OBJECT:
            case GLOBAL_EXPORT_OBJECT:
            case FUNCTION_STATIC_OBJECT:
                node->expr.loc.type = LocationType::LABEL;
                node->expr.loc.labelValue = new StringRef(id);
                break;
            case PARAM_OBJECT:
                node->expr.loc = GetArgumentLocation(context->functionType, id);
                break;
            case LOCAL_OBJECT:
                node->expr.loc.type = LocationType::SEARCH_LOCAL_DEFINITION_TABLE;
                node->expr.loc.definitionValue = definition;
                break;
            default:
                ASSERT(false);
                break;
        }
    }

    Type * definitionType = ExtractDefinitionCType(definition);

    node->expr.type = definitionType;

    Type * decayedType = DecayType(context->typeContext, definitionType);

    if (!TypeEqual(decayedType, definitionType))
    {
        Node * addrNode = MakeNode(EXPR_PNEW);
        addrNode->expr.type = decayedType;
        addrNode->expr.loc.type = NEED_ALLOC;
        AddChild(addrNode, node);

        node = addrNode;
    }

    return node;
}

// Integer -> inline
// Float -> add to const table, label
// String -> add to const table, labal
Node * ConstantExpression(FunctionContext * context, int value)
{
    Node * node = MakeNode(EXPR_DATA);
    node->expr.type = &MakeInt(context->typeContext)->type;
    node->expr.loc.type = INLINE;
    node->expr.loc.inlineValue = value;

    return node;
}
Node * ConstantExpression(FunctionContext * context, size_t value)
{
    Node * node = MakeNode(EXPR_DATA);
    node->expr.type = &MakeInt(context->typeContext, 8)->type;
    node->expr.loc.type = INLINE;
    node->expr.loc.inlineValue = value;

    return node;
}
Node * ConstantExpression(FunctionContext * context, float value)
{
    Node * node = MakeNode(EXPR_DATA);
    node->expr.type = &MakeFloat(context->typeContext)->type;;
    node->expr.loc = LocateFloat(context->constantContext, value);

    return node;
}
Node * ConstantExpression(FunctionContext * context, StringRef value)
{
    Node * node = MakeNode(EXPR_DATA);

    PointerType * pointer = MakePointer(context->typeContext);
    pointer->target = &MakeChar(context->typeContext)->type;
    node->expr.type = &pointer->type;

    Location stringLocation;
    Location stringPointerLocation;
    LocateString(context->constantContext, value, &stringLocation, &stringPointerLocation);

    node->expr.loc = stringPointerLocation;

    return node;
}

Node * IncExpression(FunctionContext * context, Node * expr)
{
    ASSERT(IsScalar(expr->expr.type) &&
           IsAssignable(expr->expr.type) &&
           !IsConst(expr->expr.type));

    // increment inplace, output location equals to input location, to satisfy lvalue semantics

    // integer  -> iadd 1
    // float    -> fadd 1.0f
    // pointer  -> padd 1

    Node * node;

    Type * type = expr->expr.type;

    if (IsIntegral(type))
    {
        node = MakeNode(EXPR_IADD);
        node->expr.type = type;

        AddChild(node, expr);
        AddChild(node, ConstantExpression(context, 1));
    }
    else if (IsFloating(type))
    {
        node = MakeNode(EXPR_FADD);
        node->expr.type = type;

        AddChild(node, expr);
        AddChild(node, ConstantExpression(context, 1.0f));
    }
    else
    {
        ASSERT(IsPointer(type));
        node = MakeNode(EXPR_PADDUI);
        node->expr.type = type;

        AddChild(node, expr);
        AddChild(node, ConstantExpression(context, 1ull));
    }

    node->expr.loc.type = SAME_AS_FIRST_CHILD;

    return node;
}
Node * PostIncExpression(FunctionContext * context, Node * expr)
{
    // TODO: implement
    return IncExpression(context, expr);
}
Node * DecExpression(FunctionContext * context, Node * expr)
{
    ASSERT(IsScalar(expr->expr.type) && IsAssignable(expr->expr.type) && !IsConst(expr->expr.type));

    // decrement inplace, output location equals to input location, to satisfy lvalue semantics

    // integer  -> dec
    // float    -> fsub 1.0f
    // pointer  -> psub 1

    Node * node;

    Type * type = expr->expr.type;

    if (IsIntegral(type))
    {
        node = MakeNode(EXPR_ISUB);
        node->expr.type = type;

        AddChild(node, expr);
        AddChild(node, ConstantExpression(context, 1));
    }
    else if (IsFloating(type))
    {
        node = MakeNode(EXPR_FSUB);
        node->expr.type = type;

        AddChild(node, expr);
        AddChild(node, ConstantExpression(context, 1.0f));
    }
    else
    {
        ASSERT(IsPointer(type));
        node = MakeNode(EXPR_PSUBUI);
        node->expr.type = type;

        AddChild(node, expr);
        AddChild(node, ConstantExpression(context, 1ull));
    }

    node->expr.loc.type = SAME_AS_FIRST_CHILD;

    return node;
}
Node * PostDecExpression(FunctionContext * context, Node * expr)
{
    // TODO: implement
    return DecExpression(context, expr);
}

Node * MemberOfExpression(FunctionContext * context,
                          Node * structOrUnion,
                          StringRef memberName)
{
    Type * memberType = GetMemberType(structOrUnion->expr.type, memberName);
    size_t memberOffset = GetMemberOffset(structOrUnion->expr.type, memberName);

    Node * addrNode = MakeNode(EXPR_PNEW);
    addrNode->expr.type = &(MakePointer(context->typeContext, structOrUnion->expr.type)->type);
    addrNode->expr.loc.type = NEED_ALLOC;
    AddChild(addrNode, structOrUnion);

    Node * addNode = MakeNode(EXPR_MADDUI);
    addNode->expr.type = &(MakePointer(context->typeContext, memberType)->type);
    addNode->expr.loc.type = SAME_AS_FIRST_CHILD;
    AddChild(addNode, addrNode);
    AddChild(addNode, ConstantExpression(context, memberOffset));

    return IndirectExpression(context, addNode);
}
Node * IndirectMemberOfExpression(FunctionContext * context,
                                  Node * pointerToStructOrUnion,
                                  StringRef memberName)
{
    Node * structOrUnion;

    structOrUnion = IndirectExpression(context, pointerToStructOrUnion);

    return MemberOfExpression(context, structOrUnion, memberName);
}

// num match, type match
bool   CheckCallExpression(Node * call)
{
    ASSERT(call->down);

    FunctionType * functionType;
    std::vector<Type *> arguments;

    Node * next = call->down;
    functionType = AsFunction(AsPointer(next->expr.type)->target);
    while (next->right)
    {
        next = next->right;
        arguments.push_back(next->expr.type);
    }

    return IsMatchedCall(functionType, arguments);
}
Node * CallExpression(FunctionContext * context,
                      Node * pointerToFunction,
                      std::vector<Node *> arguments)
{
    FunctionType * functionType = AsFunction(AsPointer(pointerToFunction->expr.type)->target);

    Node * node = MakeNode(EXPR_CALL);
    node->expr.type = functionType->target;
    node->expr.loc = GetReturnValueLocation(functionType->target);

    AddChild(node, pointerToFunction);
    Type ** paramTypeIter = functionType->memberType;
    for (Node * argument : arguments)
    {
        if (*paramTypeIter)
        {
            AddChild(node,
                     WrapCastNode(
                         WrapCastNode(argument, DecayType(context->typeContext, argument->expr.type)),
                         *paramTypeIter));
        }
        else
        {
            ASSERT(functionType->isVarList);
            Type * decayedType = DecayType(context->typeContext, argument->expr.type);
            Type * promoType = DefaultArgumentPromotion(context->typeContext, decayedType);
            AddChild(node,
                     WrapCastNode(
                         WrapCastNode(argument, decayedType),
                         promoType));
        }

        ++paramTypeIter;
    }

    ASSERT(CheckCallExpression(node));

    if (!IsVoid(functionType->target))
    {
        Node * dupNode = MakeNode(EXPR_MDUP);
        dupNode->expr.type = node->expr.type;
        dupNode->expr.loc.type = NEED_ALLOC;
        AddChild(dupNode, node);

        node = dupNode;
    }

    return node;
}

Node * SubscriptExpression(FunctionContext * context,
                           Node * a,
                           Node * b)
{
    return IndirectExpression(context, AddExpression(context, a, b));
}

Node * GetAddressExpression(FunctionContext * context, Node * expr)
{
    ASSERT(IsAddressable(expr->expr.type));

    Node * node = MakeNode(EXPR_PNEW);
    node->expr.type = &(MakePointer(context->typeContext, expr->expr.type)->type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, expr);

    return node;
}

Node * IndirectExpression(FunctionContext * context, Node * expr)
{
    ASSERT(IsPointer(expr->expr.type));

    Type * type = CloneType(context->typeContext, AsPointer(expr->expr.type)->target);
    TypeSetAssignable(type);
    TypeSetAddressable(type);

    Node * node = MakeNode(EXPR_PIND);
    node->expr.type = type;
    node->expr.loc.type = REGISTER_INDIRECT;
    node->expr.loc.registerType = RCX;

    AddChild(node, expr);

    return node;
}

Node * PositiveExpression(FunctionContext * context, Node * expr)
{
    ASSERT(IsArithmetic(expr->expr.type));

    Type * newType = IsIntegral(expr->expr.type)
        ? IntegralPromotion(context->typeContext, expr->expr.type)
        : expr->expr.type;
    return WrapCastNode(expr, newType);
}
Node * NegativeExpression(FunctionContext * context, Node * expr)
{
    ASSERT(IsArithmetic(expr->expr.type));

    Type * type = expr->expr.type;
    Type * newType = IsIntegral(type)
        ? IntegralPromotion(context->typeContext, type)
        : type;

    Node * node;

    if (IsIntegral(type))
    {
        node = MakeNode(EXPR_ISUB);

        node->expr.type = newType;
        node->expr.loc.type = NEED_ALLOC;

        AddChild(node, ConstantExpression(context, 0));
        AddChild(node, WrapCastNode(expr, newType));
    }
    else
    {
        node = MakeNode(EXPR_FNEG);

        node->expr.type = newType;
        node->expr.loc.type = NEED_ALLOC;

        AddChild(node, WrapCastNode(expr, newType));
    }

    return node;
}

Node * SizeOfExpression(FunctionContext * context, Node * expr)
{
    ASSERT(expr->expr.type);

    Type * type = expr->expr.type;

    DestroyNodeTree(expr);

    return ConstantExpression(context, TypeSize(type));;
}

Node * SizeOfExpression(FunctionContext * context, Type * type)
{
    ASSERT(type);
    return ConstantExpression(context, TypeSize(type));;
}

Node * CastExpression(Node * expr, Type * type)
{
    ASSERT(
        (IsVoid(type)) ||
        (IsScalar(type) && IsScalar(expr->expr.type))
    );

    Type * fromType = expr->expr.type;
    Type * toType = type;

    if (TypeEqual(fromType, toType))
    {
        return expr;
    }

    Node * node = nullptr;

    if (IsVoid(toType))
    {
        node = MakeNode(EXPR_CVT_REINTERP);
        node->expr.loc.type = SAME_AS_FIRST_CHILD;
    }
    else
    {
        // pointer/integer/float/bool
        // TODO: integer signed-ness
        if (IsPointer(fromType))
        {
            if (IsPointer(toType))
            {
                node = MakeNode(EXPR_CVT_REINTERP);
                node->expr.loc.type = SAME_AS_FIRST_CHILD;
            }
            else if (IsIntegral(toType) && TypeSize(fromType) == TypeSize(toType))
            {
                node = MakeNode(EXPR_CVT_REINTERP);
                node->expr.loc.type = SAME_AS_FIRST_CHILD;
            }
        }
        else if (IsIntegral(fromType))
        {
            if (IsIntegral(toType))
            {
                node = MakeNode(EXPR_CVT_SI2SI);
                node->expr.loc.type = NEED_ALLOC;
            }
            else if (IsFloating(toType))
            {
                node = MakeNode(EXPR_CVT_SI2F);
                node->expr.loc.type = NEED_ALLOC;
            }
            else if (IsBool(toType))
            {
                node = MakeNode(EXPR_CVT_I2B);
                node->expr.loc.type = NEED_ALLOC;
            }
            else if (IsPointer(toType))
            {
                ASSERT(TypeSize(fromType) == TypeSize(toType));
                node = MakeNode(EXPR_CVT_REINTERP);
                node->expr.loc.type = SAME_AS_FIRST_CHILD;
            }
        }
        else if (IsFloating(fromType))
        {
            if (IsIntegral(toType))
                node = MakeNode(EXPR_CVT_F2SI);
            else if (IsFloating(toType))
                node = MakeNode(EXPR_CVT_F2F);
            else if (IsBool(toType))
                node = MakeNode(EXPR_CVT_F2B);
            node->expr.loc.type = NEED_ALLOC;
        }
        else if (IsBool(fromType))
        {
            if (IsIntegral(toType))
                node = MakeNode(EXPR_CVT_B2I);
            else if (IsFloating(toType))
                node = MakeNode(EXPR_CVT_B2F);
            node->expr.loc.type = NEED_ALLOC;
        }
    }
    ASSERT(node);

    node->expr.type = toType;

    AddChild(node, expr);

    return node;
}

Node * AddExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(
        (IsArithmetic(a->expr.type) && IsArithmetic(b->expr.type)) ||
        (IsPointerToObject(a->expr.type) && IsIntegral(b->expr.type)) ||
        (IsIntegral(a->expr.type) && IsPointerToObject(b->expr.type))
    );

    Node * node;

    if (IsArithmetic(a->expr.type) && IsArithmetic(b->expr.type))
    {
        bool useFloating = IsFloating(a->expr.type) || IsFloating(b->expr.type);

        node = MakeNode(useFloating ? EXPR_FADD : EXPR_IADD);
        node->expr.type = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);
        node->expr.loc.type = NEED_ALLOC;

        AddChild(node, WrapCastNode(a, node->expr.type));
        AddChild(node, WrapCastNode(b, node->expr.type));
    }
    else
    {
        if (IsIntegral(a->expr.type))
        {
            Node * t = a;
            a = b;
            b = t;
        }

        if (TypeSize(b->expr.type) < 8)
        {
            Type * type = &MakeInt(context->typeContext, 8)->type;
            b = WrapCastNode(b, type);
        }

        node = MakeNode(EXPR_PADDSI);
        node->expr.type = a->expr.type;
        node->expr.loc.type = NEED_ALLOC;

        AddChild(node, a);
        AddChild(node, b);
    }

    return node;
}
Node * SubExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(
        (IsArithmetic(a->expr.type) && IsArithmetic(b->expr.type)) ||
        (IsPointerToObject(a->expr.type) && IsIntegral(b->expr.type)) ||
        (IsPointerToObject(a->expr.type) && IsPointerToObject(b->expr.type))
    );

    Node * node;

    if (IsArithmetic(a->expr.type) && IsArithmetic(b->expr.type))
    {
        bool useFloating = IsFloating(a->expr.type) || IsFloating(b->expr.type);

        node = MakeNode(useFloating ? EXPR_FSUB : EXPR_ISUB);
        node->expr.type = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);
        node->expr.loc.type = NEED_ALLOC;

        AddChild(node, WrapCastNode(a, node->expr.type));
        AddChild(node, WrapCastNode(b, node->expr.type));
    }
    else if (IsIntegral(b->expr.type))
    {
        if (TypeSize(b->expr.type) < 8)
        {
            Type * type = &MakeInt(context->typeContext, 8)->type;
            b = WrapCastNode(b, type);
        }

        node = MakeNode(EXPR_PSUBSI);
        node->expr.type = a->expr.type;
        node->expr.loc.type = NEED_ALLOC;

        AddChild(node, a);
        AddChild(node, b);
    }
    else
    {
        node = MakeNode(EXPR_PDIFF);
        node->expr.type = PtrDiffType(context->typeContext);
        node->expr.loc.type = NEED_ALLOC;

        AddChild(node, a);
        AddChild(node, b);
    }

    return node;
}
Node * MulExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(IsArithmetic(a->expr.type) &&
           IsArithmetic(b->expr.type));

    bool useFloating = IsFloating(a->expr.type) || IsFloating(b->expr.type);

    Node * node = MakeNode(useFloating ? EXPR_FMUL : EXPR_IMUL);
    node->expr.type = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, WrapCastNode(a, node->expr.type));
    AddChild(node, WrapCastNode(b, node->expr.type));

    return node;
}
Node * DivExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(IsArithmetic(a->expr.type) &&
           IsArithmetic(b->expr.type));

    bool useFloating = IsFloating(a->expr.type) || IsFloating(b->expr.type);

    Node * node = MakeNode(useFloating ? EXPR_FDIV : EXPR_IDIV);
    node->expr.type = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, WrapCastNode(a, node->expr.type));
    AddChild(node, WrapCastNode(b, node->expr.type));

    return node;
}
Node * ModExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(IsIntegral(a->expr.type) &&
           IsIntegral(b->expr.type));

    Node * node = MakeNode(EXPR_IMOD);
    node->expr.type = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, WrapCastNode(a, node->expr.type));
    AddChild(node, WrapCastNode(b, node->expr.type));

    return node;
}

Node * BitNotExpression(FunctionContext * context, Node * expr)
{
    ASSERT(IsIntegral(expr->expr.type));

    Node * node = MakeNode(EXPR_INOT);
    node->expr.type = IntegralPromotion(context->typeContext, expr->expr.type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, WrapCastNode(expr, node->expr.type));

    return node;
}
Node * BitAndExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(IsIntegral(a->expr.type) &&
           IsIntegral(b->expr.type));

    Node * node = MakeNode(EXPR_IAND);
    node->expr.type = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, WrapCastNode(a, node->expr.type));
    AddChild(node, WrapCastNode(b, node->expr.type));

    return node;
}
Node * BitXorExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(IsIntegral(a->expr.type) &&
           IsIntegral(b->expr.type));

    Node * node = MakeNode(EXPR_IXOR);
    node->expr.type = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, WrapCastNode(a, node->expr.type));
    AddChild(node, WrapCastNode(b, node->expr.type));

    return node;
}
Node * BitOrExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(IsIntegral(a->expr.type) &&
           IsIntegral(b->expr.type));

    Node * node = MakeNode(EXPR_IOR);
    node->expr.type = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, WrapCastNode(a, node->expr.type));
    AddChild(node, WrapCastNode(b, node->expr.type));

    return node;
}
Node * ShiftLeftExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(IsIntegral(a->expr.type) &&
           IsIntegral(b->expr.type));

    Node * node = MakeNode(EXPR_ISHL);

    Type * aPromoType = IntegralPromotion(context->typeContext, a->expr.type);
    Type * bPromoType = IntegralPromotion(context->typeContext, b->expr.type);

    node->expr.type = aPromoType;
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, WrapCastNode(a, aPromoType));
    AddChild(node, WrapCastNode(b, bPromoType));

    return node;
}
Node * ShiftRightExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(IsIntegral(a->expr.type) &&
           IsIntegral(b->expr.type));

    Node * node = MakeNode(EXPR_ISHR);

    Type * aPromoType = IntegralPromotion(context->typeContext, a->expr.type);
    Type * bPromoType = IntegralPromotion(context->typeContext, b->expr.type);

    node->expr.type = aPromoType;
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, WrapCastNode(a, aPromoType));
    AddChild(node, WrapCastNode(b, bPromoType));

    return node;
}

Node * BoolNotExpression(FunctionContext * context, Node * expr)
{
    ASSERT(IsScalar(expr->expr.type));

    Node * node = MakeNode(EXPR_BOOL_NOT);
    node->expr.type = &(MakeBool(context->typeContext)->type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, WrapCastNode(expr, node->expr.type));

    return node;
}
Node * BoolAndExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(IsScalar(a->expr.type) && IsScalar(b->expr.type));

    Node * node = MakeNode(EXPR_BOOL_AND);
    node->expr.type = &(MakeBool(context->typeContext)->type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, WrapCastNode(a, node->expr.type));
    AddChild(node, WrapCastNode(b, node->expr.type));

    return node;
}
Node * BoolOrExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(IsScalar(a->expr.type) && IsScalar(b->expr.type));

    Node * node = MakeNode(EXPR_BOOL_OR);
    node->expr.type = &(MakeBool(context->typeContext)->type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, WrapCastNode(a, node->expr.type));
    AddChild(node, WrapCastNode(b, node->expr.type));

    return node;
}

Node * CmpExpression(FunctionContext * context, NodeType op, Node * a, Node * b)
{
    Type * atype = a->expr.type;
    Type * btype = b->expr.type;
    ASSERT(
        (IsArithmetic(atype) && IsArithmetic(btype)) ||
        (IsPointer(atype) && IsPointer(btype) && TypeEqual(AsPointer(atype)->target, AsPointer(btype)->target))
    );

    Node * node = MakeNode(op);
    node->expr.type = &(MakeBool(context->typeContext)->type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, a);
    AddChild(node, b);

    return node;
}
Node * EqExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT((IsArithmetic(a->expr.type) && IsArithmetic(b->expr.type)) ||
           TypeEqual(a->expr.type, b->expr.type)); // arithmetic or same type pointer

    NodeType cmpType;

    if (IsArithmetic(a->expr.type) && IsArithmetic(b->expr.type))
    {
        Type * promoType = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);

        a = WrapCastNode(a, promoType);
        b = WrapCastNode(b, promoType);

        cmpType = IsFloating(promoType) ? EXPR_FEQ : EXPR_IEQ;
    }
    else
    {
        cmpType = EXPR_PEQ;
    }

    return CmpExpression(context, cmpType, a, b);
}
Node * NeExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT((IsArithmetic(a->expr.type) && IsArithmetic(b->expr.type)) ||
           TypeEqual(a->expr.type, b->expr.type)); // arithmetic or same type pointer

    NodeType cmpType;

    if (IsArithmetic(a->expr.type) && IsArithmetic(b->expr.type))
    {
        Type * promoType = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);

        a = WrapCastNode(a, promoType);
        b = WrapCastNode(b, promoType);

        cmpType = IsFloating(promoType) ? EXPR_FNE : EXPR_INE;
    }
    else
    {
        cmpType = EXPR_PNE;
    }

    return CmpExpression(context, cmpType, a, b);
}
Node * LtExpression(FunctionContext * context, Node * a, Node * b)
{
    Type * atype = a->expr.type;
    Type * btype = b->expr.type;
    ASSERT(
        (IsArithmetic(atype) && IsArithmetic(btype)) ||
        (IsPointer(atype) && IsPointer(btype) && TypeEqual(AsPointer(atype)->target, AsPointer(btype)->target))
    );

    NodeType cmpType;

    if (IsArithmetic(a->expr.type) && IsArithmetic(b->expr.type))
    {
        Type * promoType = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);

        a = WrapCastNode(a, promoType);
        b = WrapCastNode(b, promoType);

        cmpType = IsFloating(promoType) ? EXPR_FLT : EXPR_ILT;
    }
    else
    {
        cmpType = EXPR_PLT;
    }

    return CmpExpression(context, cmpType, a, b);
}
Node * LeExpression(FunctionContext * context, Node * a, Node * b)
{
    Type * atype = a->expr.type;
    Type * btype = b->expr.type;
    ASSERT(
        (IsArithmetic(atype) && IsArithmetic(btype)) ||
        (IsPointer(atype) && IsPointer(btype) && TypeEqual(AsPointer(atype)->target, AsPointer(btype)->target))
    );

    NodeType cmpType;

    if (IsArithmetic(a->expr.type) && IsArithmetic(b->expr.type))
    {
        Type * promoType = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);

        a = WrapCastNode(a, promoType);
        b = WrapCastNode(b, promoType);

        cmpType = IsFloating(promoType) ? EXPR_FLE : EXPR_ILE;
    }
    else
    {
        cmpType = EXPR_PLE;
    }

    return CmpExpression(context, cmpType, a, b);
}
Node * GeExpression(FunctionContext * context, Node * a, Node * b)
{
    Type * atype = a->expr.type;
    Type * btype = b->expr.type;
    ASSERT(
        (IsArithmetic(atype) && IsArithmetic(btype)) ||
        (IsPointer(atype) && IsPointer(btype) && TypeEqual(AsPointer(atype)->target, AsPointer(btype)->target))
    );

    NodeType cmpType;

    if (IsArithmetic(a->expr.type) && IsArithmetic(b->expr.type))
    {
        Type * promoType = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);

        a = WrapCastNode(a, promoType);
        b = WrapCastNode(b, promoType);

        cmpType = IsFloating(promoType) ? EXPR_FGE : EXPR_IGE;
    }
    else
    {
        cmpType = EXPR_PGE;
    }

    return CmpExpression(context, cmpType, a, b);
}
Node * GtExpression(FunctionContext * context, Node * a, Node * b)
{
    Type * atype = a->expr.type;
    Type * btype = b->expr.type;
    ASSERT(
        (IsArithmetic(atype) && IsArithmetic(btype)) ||
        (IsPointer(atype) && IsPointer(btype) && TypeEqual(AsPointer(atype)->target, AsPointer(btype)->target))
    );

    NodeType cmpType;

    if (IsArithmetic(a->expr.type) && IsArithmetic(b->expr.type))
    {
        Type * promoType = UsualArithmeticConversion(context->typeContext, a->expr.type, b->expr.type);

        a = WrapCastNode(a, promoType);
        b = WrapCastNode(b, promoType);

        cmpType = IsFloating(promoType) ? EXPR_FGT : EXPR_IGT;
    }
    else
    {
        cmpType = EXPR_PGT;
    }

    return CmpExpression(context, cmpType, a, b);
}

Node * AssignExpression(FunctionContext * context, Node * a, Node * b)
{
    // arithmetic/struct/union/pointer
    ASSERT(
        TypeEqual(a->expr.type, b->expr.type) ||
        (IsIntegral(a->expr.type) && IsIntegral(b->expr.type)) ||
        (PrintType(a->expr.type), PrintType(b->expr.type), false));
    ASSERT(IsAssignable(a->expr.type));

    Node * node = MakeNode(EXPR_MCOPY);
    node->expr.type = a->expr.type;
    node->expr.loc.type = SAME_AS_FIRST_CHILD;

    AddChild(node, a);
    AddChild(node, WrapCastNode(b, a->expr.type));

    Node * dupNode = MakeNode(EXPR_MDUP);
    dupNode->expr.type = a->expr.type;
    dupNode->expr.loc.type = NEED_ALLOC;

    AddChild(dupNode, node);

    return dupNode;
}

Node * ConditionExpression(FunctionContext * context, Node * a, Node * b, Node * c)
{
    Type * t1 = a->expr.type;
    Type * t2 = b->expr.type;
    Type * t3 = c->expr.type;
    ASSERT(
        IsScalar(t1) &&
        (
            TypeEqual(t2, t3) &&
            (IsArithmetic(t2) || IsStructOrUnion(t2) || IsVoid(t2) || IsPointer(t2))
            )
    );

    Node * node = MakeNode(EXPR_CONDITION);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, a);

    // arithmetic/struct/union/pointer/void

    if (IsArithmetic(t2) && IsArithmetic(t3))
    {
        node->expr.type = UsualArithmeticConversion(context->typeContext, t2, t3);

        AddChild(node, WrapCastNode(b, node->expr.type));
        AddChild(node, WrapCastNode(c, node->expr.type));
    }
    else if ((t2->name == Language::STRUCT || t2->name == Language::UNION || t2->name == Language::VOID) &&
             TypeEqual(t2, t3))
    {
        node->expr.type = t2;

        AddChild(node, b);
        AddChild(node, c);
    }
    else
    {
        // pointer...
        node->expr.type = t2;

        AddChild(node, b);
        AddChild(node, c);
    }

    return node;
}

Node * CommaExpression(std::vector<Node *> & exprs)
{
    Node * node = MakeNode(EXPR_ELIST);
    node->expr.type = exprs.back()->expr.type;
    node->expr.loc.type = NEED_ALLOC;

    for (Node * expr : exprs)
    {
        AddChild(node, expr);
    }

    return node;
}

Node * CompoundStatement_Begin(FunctionContext * context, DefinitionContext * definitionContext)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_COMPOUND;
    node->stmt.context = definitionContext;

    context->currentDefinitionContext.push_back(definitionContext);

    return node;
}
void   CompoundStatement_AddStatement(Node * compoundStmt, Node * stmt)
{
    AddChild(compoundStmt, stmt);
}
void   CompoundStatement_End(FunctionContext * context)
{
    context->currentDefinitionContext.pop_back();
}

Node * ExpressionStatement(Node * expr)
{
    Node * node = new Node;
    node->down = expr;
    node->right = node->up = nullptr;
    node->type = STMT_EXPRESSION;
    node->stmt.context = nullptr;
    return node;
}

Node * ReturnStatement(Node * expr)
{
    Node * node = new Node;
    node->down = expr;
    node->right = node->up = nullptr;
    node->type = STMT_RETURN;
    node->stmt.context = nullptr;
    return node;
}

Node * IfStatement_Begin()
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_IF;
    node->stmt.context = nullptr;
    return node;
}
void   IfStatement_SetExpr(Node * ifStmt, Node * expr)
{
    ASSERT(ifStmt && CountChild(ifStmt) == 0);
    AddChild(ifStmt, expr);
}
void   IfStatement_SetIfBody(Node * ifStmt, Node * ifBody)
{
    ASSERT(ifStmt && CountChild(ifStmt) == 1);
    AddChild(ifStmt, ifBody);
}
void   IfStatement_SetElseBody(Node * ifStmt, Node * elseBody)
{
    ASSERT(ifStmt && CountChild(ifStmt) == 2);
    AddChild(ifStmt, elseBody);
}
void   IfStatement_End(Node * ifStmt)
{
}

Node * WhileStatement_Begin(FunctionContext * context)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_WHILE;
    node->stmt.context = nullptr;

    std::string beginLabel = std::string("@L") + std::to_string(context->nextUniqueLabel++);
    std::string endLabel = std::string("@L") + std::to_string(context->nextUniqueLabel++);
    context->targetToLabels.insert({
        node,
                                   { beginLabel, endLabel }
                                   });

    context->currentBreakTarget.push_back(node);
    context->currentContinueTarget.push_back(node);

    return node;
}
void   WhileStatement_SetExpr(Node * whileStmt, Node * expr)
{
    ASSERT(whileStmt && CountChild(whileStmt) == 0);
    AddChild(whileStmt, expr);
}
void   WhileStatement_SetBody(Node * whileStmt, Node * body)
{
    ASSERT(whileStmt && CountChild(whileStmt) == 1);
    AddChild(whileStmt, body);
}
void   WhileStatement_End(FunctionContext * context)
{
    context->currentBreakTarget.pop_back();
    context->currentContinueTarget.pop_back();
}

Node * DoWhileStatement_Begin(FunctionContext * context)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_DO_WHILE;
    node->stmt.context = nullptr;

    std::string beginLabel = std::string("@L") + std::to_string(context->nextUniqueLabel++);
    std::string endLabel = std::string("@L") + std::to_string(context->nextUniqueLabel++);
    context->targetToLabels.insert({
        node,
                                   { beginLabel, endLabel }
                                   });

    context->currentBreakTarget.push_back(node);
    context->currentContinueTarget.push_back(node);

    return node;
}
void   DoWhileStatement_SetBody(Node * doWhileStmt, Node * body)
{
    ASSERT(doWhileStmt && CountChild(doWhileStmt) == 0);
    AddChild(doWhileStmt, body);
}
void   DoWhileStatement_SetExpr(Node * doWhileStmt, Node * expr)
{
    ASSERT(doWhileStmt && CountChild(doWhileStmt) == 1);
    AddChild(doWhileStmt, expr);
}
void   DoWhileStatement_End(FunctionContext * context)
{
    context->currentBreakTarget.pop_back();
    context->currentContinueTarget.pop_back();
}

Node * ForStatement_Begin(FunctionContext * context)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_FOR;
    node->stmt.context = nullptr;

    std::string beginLabel = std::string("@L") + std::to_string(context->nextUniqueLabel++);
    std::string endLabel = std::string("@L") + std::to_string(context->nextUniqueLabel++);
    context->targetToLabels.insert({
        node,
                                   { beginLabel, endLabel }
                                   });

    context->currentBreakTarget.push_back(node);
    context->currentContinueTarget.push_back(node);

    return node;
}
void   ForStatement_SetPreExpr(Node * forStmt, Node * preExpr)
{
    ASSERT(forStmt && CountChild(forStmt) == 0);
    AddChild(forStmt, preExpr);
}
void   ForStatement_SetLoopExpr(Node * forStmt, Node * loopExpr)
{
    ASSERT(forStmt && CountChild(forStmt) == 1);
    AddChild(forStmt, loopExpr);
}
void   ForStatement_SetPostExpr(Node * forStmt, Node * postExpr)
{
    ASSERT(forStmt && CountChild(forStmt) == 2);
    AddChild(forStmt, postExpr);
}
void   ForStatement_SetBody(Node * forStmt, Node * body)
{
    ASSERT(forStmt && CountChild(forStmt) == 3);
    AddChild(forStmt, body);
}
void   ForStatement_End(FunctionContext * context)
{
    context->currentBreakTarget.pop_back();
    context->currentContinueTarget.pop_back();
}

Node * BreakStatement(FunctionContext * context)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_BREAK;
    node->stmt.context = nullptr;

    ASSERT(!context->currentBreakTarget.empty());
    context->nodeToTarget.insert({
        node,
                                 { context->currentBreakTarget.back() , 1 }
                                 });

    return node;
}

Node * ContinueStatement(FunctionContext * context)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_CONTINUE;
    node->stmt.context = nullptr;

    ASSERT(!context->currentContinueTarget.empty());
    context->nodeToTarget.insert({
        node,
                                 { context->currentContinueTarget.back() , 0 }
                                 });

    return node;
}

Node * SwitchStatement_Begin(FunctionContext * context)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_SWITCH;
    node->stmt.context = nullptr;

    std::string endLabel = std::string("@L") + std::to_string(context->nextUniqueLabel++);
    context->targetToLabels.insert({
        node,
                                   { "", endLabel }
                                   });
    context->switchToChildren.insert({ node,{ nullptr } });

    context->currentBreakTarget.push_back(node);
    context->currentSwitch.push_back(node);

    return node;
}
void   SwitchStatement_SetExpr(Node * switchStmt, Node * expr)
{
    ASSERT(switchStmt && CountChild(switchStmt) == 0);
    AddChild(switchStmt, expr);
}
void   SwitchStatement_SetBody(Node * switchStmt, Node * body)
{
    ASSERT(switchStmt && CountChild(switchStmt) == 1);
    AddChild(switchStmt, body);
}
void   SwitchStatement_End(FunctionContext * context)
{
    context->currentBreakTarget.pop_back();
    context->currentSwitch.pop_back();
}

Node * CaseStatement(FunctionContext * context, u64 caseValue, Node * stmt)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_CASE;
    node->stmt.caseValue = caseValue;

    AddChild(node, stmt);

    ASSERT(!context->currentSwitch.empty());
    Node * switchNode = context->currentSwitch.back();
    std::vector<std::string> & switchLabels = context->targetToLabels[switchNode];

    std::string caseLabel = std::string("@L") + std::to_string(context->nextUniqueLabel++);
    switchLabels.push_back(caseLabel);

    context->nodeToTarget.insert({
        node,
                                 { switchNode, static_cast<int>(switchLabels.size() - 1) }
                                 });
    ASSERT(context->switchToChildren.find(switchNode) != context->switchToChildren.end());
    context->switchToChildren[switchNode].push_back(node);

    return node;
}

Node * DefaultStatement(FunctionContext * context, Node * stmt)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_DEFAULT;
    node->stmt.context = nullptr;

    AddChild(node, stmt);

    ASSERT(!context->currentSwitch.empty());
    Node * switchNode = context->currentSwitch.back();
    std::vector<std::string> & switchLabels = context->targetToLabels[switchNode];

    std::string defaultLabel = std::string("@L") + std::to_string(context->nextUniqueLabel++);
    switchLabels[0] = defaultLabel;

    context->nodeToTarget.insert({
        node,
                                 { switchNode, 0 }
                                 });
    ASSERT(context->switchToChildren.find(switchNode) != context->switchToChildren.end() && context->switchToChildren[switchNode][0] == nullptr);
    context->switchToChildren[switchNode][0] = node;

    return node;
}

void PrintNodeTreeImpl(Node * root, std::string indent)
{
    ASSERT(root);

    std::cout << indent << NodeDebugString(root) << std::endl;

    if (root->down)
        PrintNodeTreeImpl(root->down, indent + "  ");

    if (root->right)
        PrintNodeTreeImpl(root->right, indent);
}

void PrintNodeTree(Node * root)
{
    PrintNodeTreeImpl(root, "");
}

void PrintFunctionContext(FunctionContext * context)
{
    PrintNodeTree(context->functionBody);
}

}