#include "Function.h"

#include "../ir/CallingConvention.h"
#include "../util/Bit.h"

namespace Language {

ConstantContext * CreateConstantContext()
{
    return new ConstantContext();
}

// TODO: impl
Location LocateString(ConstantContext * context,
                      StringRef strValue)
{
    ASSERT(false);
    Location loc;
    loc.type = NO_WHERE;
    return loc;
}
// TODO: impl
Location LocateFloat(ConstantContext * context,
                     float fltValue)
{
    ASSERT(false);
    Location loc;
    loc.type = NO_WHERE;
    return loc;
}

FunctionContext * CreateFunctionContext(DefinitionContext * functionDefinitionContext,
                                        FunctionType * functionType,
                                        ConstantContext * constantContext,
                                        TypeContext * typeContext,
                                        StringRef functionName)
{
    FunctionContext * functionContext = new FunctionContext;
    
    functionContext->functionDefinitionContext = functionDefinitionContext;
    functionContext->functionType = functionType;
    functionContext->nextUniqueLabel = 0;
    functionContext->constantContext = constantContext;
    functionContext->typeContext = typeContext;
    functionContext->functionName = functionName.toString();
    functionContext->localZoneSize = 0;
    functionContext->maxTempZoneSize = 0;
    functionContext->maxCallZoneSize = 0;
    functionContext->stackAllocSize = 0;
    functionContext->stackFrameSize = 0;
    functionContext->registerZoneEndOffset = 0;
    functionContext->localZoneEndOffset = 0;
    functionContext->tempZoneBeginOffset = 0;
    functionContext->swapZoneEndOffset = 0;
    functionContext->callZoneEndOffset = 0;
    functionContext->dirtyRegisters = 0;
    functionContext->functionBody = nullptr;

    return functionContext;
}

void AddChild(Node * parent, Node * child)
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

int CountChild(Node * node)
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

Node * LastChild(Node * parent)
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

Node * MakeNode(NodeType type)
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

void DestroyNodeTree(Node * root)
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

    if (IsInt(fromType))
    {
        if (IsInt(toType))
            castNode = MakeNode(EXPR_CVT_I2I);
        else if (IsFloating(toType))
            castNode = MakeNode(EXPR_CVT_I2F);
        else if (IsBool(toType))
            castNode = MakeNode(EXPR_CVT_I2B);
    }
    else if (IsFloating(fromType))
    {
        if (IsInt(toType))
            castNode = MakeNode(EXPR_CVT_F2I);
        else if (IsFloating(toType))
            castNode = MakeNode(EXPR_CVT_F2F);
        else if (IsBool(toType))
            castNode = MakeNode(EXPR_CVT_F2B);
    }
    else if (IsPointer(fromType) && IsPointer(toType))
    {
        castNode = MakeNode(EXPR_CVT_NOOP);
    }
    ASSERT(castNode);

    castNode->expr.type = toType;

    AddChild(castNode, node);

    return castNode;
}

size_t ComputeCallZoneSize(Node * call)
{
    ASSERT(call->down);

    size_t size = 0;

    // int/float/block(<=64)    -> register/value-on-stack
    // block(>64)               -> address-on-stack
    size += (CountChild(call) - 1) * 8;

    // large return value
    Type * returnType = call->down->expr.type;
    if (TypeSize(returnType) > 8)
    {
        size += 8;
    }

    return size;
}

void   TypeChecking_Call(Node * call)
{
    ASSERT(call->down);

    // num match, type match
    FunctionType * functionType;
    std::vector<Type *> arguments;

    Node * next = call->down;
    functionType = AsFunction(AsPointer(next->expr.type)->target);
    while (next->right)
    {
        next = next->right;
        arguments.push_back(next->expr.type);
    }

    ASSERT(IsMatchedCall(functionType, arguments));
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

    return calleeProtocol.GetParameterLocation(argumentIndex);
}

Node * IdExpression(FunctionContext * context,
                    StringRef id)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE ||
           context->currentIntention.back() == WANT_ADDRESS);

    Definition * definition = LookupDefinition(context->currentDefinitionContext.back(),
                                               id,
                                               DefinitionContextNamespace::ID_NAMESPACE,
                                               true);
    ASSERT(definition &&
           (
               definition->type == OBJECT_DEFINITION ||
               definition->type == FUNCTION_DEFINITION
            ));

    Type * definitionType = ExtractDefinitionCType(definition);

    Node * node = MakeNode(EXPR_ID);

    node->expr.type = DecayType(context->typeContext, definitionType);

    // function                 -> label
    // import object            -> label
    // global (export) object   -> label
    // function static object   -> label
    // argument object          -> GetArgumentLocation(functionType, argumentIndex)
    // local object             -> delay (search localObjectOffsets, not ready yet)
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

    return node;
}

// Integer -> inline
// Float -> add to const table, label
// String -> add to const table, labal
Node * ConstantExpression(FunctionContext * context, int value)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE);

    Node * node = MakeNode(EXPR_CONSTANT);
    node->expr.type = &MakeInt(context->typeContext)->type;
    node->expr.loc.type = INLINE;
    node->expr.loc.inlineValue = value;

    return node;
}
Node * ConstantExpression(FunctionContext * context, size_t value)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE);

    Node * node = MakeNode(EXPR_CONSTANT);
    node->expr.type = &MakeInt(context->typeContext)->type;
    node->expr.loc.type = INLINE;
    node->expr.loc.inlineValue = value;

    return node;
}
Node * ConstantExpression(FunctionContext * context, float value)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE);

    Node * node = MakeNode(EXPR_CONSTANT);
    node->expr.type = &MakeFloat(context->typeContext)->type;;
    node->expr.loc = LocateFloat(context->constantContext, value);

    return node;
}
Node * ConstantExpression(FunctionContext * context, StringRef value)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE);

    Node * node = MakeNode(EXPR_CONSTANT);
    
    PointerType * pointer = MakePointer(context->typeContext);
    pointer->target = &MakeChar(context->typeContext)->type;
    node->expr.type = &pointer->type;
    
    node->expr.loc = LocateString(context->constantContext, value);

    return node;
}

Node * IncExpression(FunctionContext * context, Node * expr)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE ||
           context->currentIntention.back() == WANT_ADDRESS);
    ASSERT(IsScalar(expr->expr.type) &&
           IsAssignable(expr->expr.type) &&
           !IsConst(expr->expr.type));

    // increment inplace, output location equals to input location, to satisfy lvalue semantics

    // integer  -> inc
    // float    -> fadd 1.0f
    // pointer  -> padd 1

    Node * node;
    
    Type * type = expr->expr.type;

    if (IsIntegral(type))
    {
        node = MakeNode(EXPR_IINC);
        node->expr.type = type;

        AddChild(node, expr);
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
        node = MakeNode(EXPR_PADD);
        node->expr.type = type;

        AddChild(node, expr);
        AddChild(node, ConstantExpression(context, 1));
    }

    node->expr.loc.type = SAME_AS_FIRST_CHILD;

    return node;
}
Node * PostIncExpression(FunctionContext * context, Node * expr)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE);
    ASSERT(IsScalar(expr->expr.type) &&
           IsAssignable(expr->expr.type) &&
           !IsConst(expr->expr.type));

    // copy to output location, then increment to input location

    // integer  -> inc
    // float    -> fadd 1.0f
    // pointer  -> padd 1

    Node * node;

    Type * type = expr->expr.type;

    Node * dupNode = MakeNode(EXPR_MDUP);
    dupNode->expr.type = type;
    dupNode->expr.loc.type = NEED_ALLOC;
    AddChild(dupNode, expr);

    if (IsIntegral(type))
    {
        Node * addNode = MakeNode(EXPR_IINC);
        addNode->expr.type = type;
        addNode->expr.loc.type = SAME_AS_FIRST_GRANDCHILD;

        AddChild(addNode, dupNode);
        node = addNode;
    }
    else if (IsFloating(type))
    {
        Node * addNode = MakeNode(EXPR_FADD);
        addNode->expr.type = type;
        addNode->expr.loc.type = SAME_AS_FIRST_GRANDCHILD;

        AddChild(addNode, dupNode);
        AddChild(addNode, ConstantExpression(context, 1.0f));
        node = addNode;
    }
    else
    {
        ASSERT(IsPointer(type));
        Node * addNode = MakeNode(EXPR_PADD);
        addNode->expr.type = type;
        addNode->expr.loc.type = SAME_AS_FIRST_GRANDCHILD;

        AddChild(addNode, dupNode);
        AddChild(addNode, ConstantExpression(context, 1));
        node = addNode;
    }

    return node;
}
Node * DecExpression(FunctionContext * context, Node * expr)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE ||
           context->currentIntention.back() == WANT_ADDRESS);
    ASSERT(IsScalar(expr->expr.type) && IsAssignable(expr->expr.type) && !IsConst(expr->expr.type));

    // decrement inplace, output location equals to input location, to satisfy lvalue semantics

    // integer  -> dec
    // float    -> fsub 1.0f
    // pointer  -> psub 1

    Node * node;

    Type * type = expr->expr.type;

    if (IsIntegral(type))
    {
        node = MakeNode(EXPR_IDEC);
        node->expr.type = type;

        AddChild(node, expr);
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
        node = MakeNode(EXPR_PSUB);
        node->expr.type = type;

        AddChild(node, expr);
        AddChild(node, ConstantExpression(context, 1));
    }

    node->expr.loc.type = SAME_AS_FIRST_CHILD;

    return node;
}
Node * PostDecExpression(FunctionContext * context, Node * expr)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE);
    ASSERT(IsScalar(expr->expr.type) && IsAssignable(expr->expr.type) && !IsConst(expr->expr.type));

    // copy to output location, then decrement to input location

    // integer  -> dec
    // float    -> fsub 1.0f
    // pointer  -> psub 1

    Node * node;

    Type * type = expr->expr.type;

    Node * dupNode = MakeNode(EXPR_MDUP);
    dupNode->expr.type = type;
    dupNode->expr.loc.type = NEED_ALLOC;
    AddChild(dupNode, expr);

    if (IsIntegral(type))
    {
        Node * subNode = MakeNode(EXPR_IDEC);
        subNode->expr.type = type;
        subNode->expr.loc.type = SAME_AS_FIRST_GRANDCHILD;

        AddChild(subNode, dupNode);
        node = subNode;
    }
    else if (IsFloating(type))
    {
        Node * subNode = MakeNode(EXPR_FSUB);
        subNode->expr.type = type;
        subNode->expr.loc.type = SAME_AS_FIRST_GRANDCHILD;

        AddChild(subNode, dupNode);
        AddChild(subNode, ConstantExpression(context, 1.0f));
        node = subNode;
    }
    else
    {
        ASSERT(IsPointer(type));
        Node * subNode = MakeNode(EXPR_PSUB);
        subNode->expr.type = type;
        subNode->expr.loc.type = SAME_AS_FIRST_GRANDCHILD;

        AddChild(subNode, dupNode);
        AddChild(subNode, ConstantExpression(context, 1));
        node = subNode;
    }

    return node;
}

Node * MemberOfExpression(FunctionContext * context,
                          Node * structOrUnion,
                          StringRef memberName)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE ||
           context->currentIntention.back() == WANT_ADDRESS);

    Type * memberType   = GetMemberType(structOrUnion->expr.type, memberName);
    size_t memberOffset = GetMemberOffset(structOrUnion->expr.type, memberName);

    Node * addrNode = MakeNode(EXPR_MADDR);
    addrNode->expr.type = &(MakePointer(context->typeContext, structOrUnion->expr.type)->type);
    addrNode->expr.loc.type = NEED_ALLOC;
    AddChild(addrNode, structOrUnion);
    
    Node * addNode = MakeNode(EXPR_MADD);
    addNode->expr.type = memberType;
    addNode->expr.loc.type = SAME_AS_FIRST_CHILD;
    AddChild(addNode, addrNode);
    AddChild(addNode, ConstantExpression(context, memberOffset));

    return addNode;
}
Node * IndirectMemberOfExpression(FunctionContext * context,
                                  Node * pointerToStructOrUnion,
                                  StringRef memberName)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE ||
           context->currentIntention.back() == WANT_ADDRESS);

    Node * structOrUnion;

    context->currentIntention.push_back(WANT_VALUE);
    structOrUnion = IndirectExpression(context, pointerToStructOrUnion);
    context->currentIntention.pop_back();

    return MemberOfExpression(context, structOrUnion, memberName);
}

Node * CallExpression(FunctionContext * context,
                      Node * pointerToFunction,
                      std::vector<Node *> arguments)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE);

    FunctionType * functionType = AsFunction(AsPointer(pointerToFunction->expr.type)->target);

    Node * node = MakeNode(EXPR_CALL);
    node->expr.type = functionType->target;
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, pointerToFunction);
    Type ** paramTypeIter = functionType->memberType;
    for (Node * argument : arguments)
    {
        AddChild(node,
                 WrapCastNode(
                     WrapCastNode(argument, DecayType(context->typeContext, argument->expr.type)),
                     *(paramTypeIter++)));
    }

    TypeChecking_Call(node);

    return node;
}

Node * SubscriptExpression(FunctionContext * context,
                           Node * a,
                           Node * b)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE ||
           context->currentIntention.back() == WANT_ADDRESS);

    return IndirectExpression(context, AddExpression(context, a, b));
}

Node * GetAddressExpression(FunctionContext * context, Node * expr)
{
    ASSERT(context->currentIntention.back() == WANT_VALUE);

    Node * node = MakeNode(EXPR_MADDR);
    node->expr.type = &(MakePointer(context->typeContext, expr->expr.type)->type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, expr);

    return node;
}

Node * IndirectExpression(FunctionContext * context, Node * expr)
{
    ASSERT(IsPointer(expr->expr.type));

    Node * node = MakeNode(EXPR_PVAL);
    node->expr.type = AsPointer(expr->expr.type)->target;
    node->expr.loc.type = REGISTER_INDIRECT;

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

    Type * type    = expr->expr.type;
    Type * newType = IsIntegral(type)
                     ? IntegralPromotion(context->typeContext, type)
                     : type;

    Node * node    = MakeNode(IsIntegral(type) ? EXPR_INEG : EXPR_FNEG);

    node->expr.type = newType;
    node->expr.loc.type = NEED_ALLOC;
    
    AddChild(node, WrapCastNode(expr, newType));

    return node;
}

Node * SizeOfExpression(FunctionContext * context, Node * expr)
{
    ASSERT(expr->expr.type);

    Type * type = expr->expr.type;

    DestroyNodeTree(expr);

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
        node = MakeNode(EXPR_CVT_NOOP);
        node->expr.loc.type = SAME_AS_FIRST_CHILD;
    }
    else
    {
        // pointer/integer/float/bool
        // TODO: integer signed-ness
        if (IsPointer(fromType) && IsPointer(toType))
        {
            node = MakeNode(EXPR_CVT_NOOP);
            node->expr.loc.type = SAME_AS_FIRST_CHILD;
        }
        else if (IsIntegral(fromType))
        {
            if (IsIntegral(toType))
                node = MakeNode(EXPR_CVT_I2I);
            else if (IsFloating(toType))
                node = MakeNode(EXPR_CVT_I2F);
            else if (IsBool(toType))
                node = MakeNode(EXPR_CVT_I2B);
            node->expr.loc.type = NEED_ALLOC;
        }
        else if (IsFloating(fromType))
        {
            if (IsIntegral(toType))
                node = MakeNode(EXPR_CVT_F2I);
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
        (IsArithmetic(a->expr.type)      && IsArithmetic(b->expr.type)) ||
        (IsPointerToObject(a->expr.type) && IsIntegral(b->expr.type))   ||
        (IsIntegral(a->expr.type)        && IsPointerToObject(b->expr.type))
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

        node = MakeNode(EXPR_PADD);
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
        (IsArithmetic(a->expr.type)      && IsArithmetic(b->expr.type)) ||
        (IsPointerToObject(a->expr.type) && IsIntegral(b->expr.type))   ||
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
        node = MakeNode(EXPR_PSUB);
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
    ASSERT(IsComparable(a->expr.type, b->expr.type)); // arithmetic or same type pointer

    Node * node = MakeNode(op);
    node->expr.type = &(MakeBool(context->typeContext)->type);
    node->expr.loc.type = NEED_ALLOC;

    AddChild(node, a);
    AddChild(node, b);

    return node;
}
Node * EqExpression(FunctionContext * context, Node * a, Node * b)
{
    ASSERT(CanTestEquality(a->expr.type, b->expr.type)); // arithmetic or same type pointer

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
    ASSERT(CanTestEquality(a->expr.type, b->expr.type)); // arithmetic or same type pointer

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
    ASSERT(IsComparable(a->expr.type, b->expr.type)); // arithmetic or same type pointer

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
    ASSERT(IsComparable(a->expr.type, b->expr.type)); // arithmetic or same type pointer

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
    ASSERT(IsComparable(a->expr.type, b->expr.type)); // arithmetic or same type pointer

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
    ASSERT(IsComparable(a->expr.type, b->expr.type)); // arithmetic or same type pointer

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
    ASSERT(TypeEqual(a->expr.type, b->expr.type));
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

void   Function_Begin(FunctionContext * context)
{
}

void   Function_End(FunctionContext * context)
{
    // check goto labels
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

Node * GotoStatement(FunctionContext * context, StringRef label)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_GOTO;
    node->stmt.label = new StringRef(label);

    auto iter = context->isLabelDefined.find(label.toString());
    if (iter == context->isLabelDefined.end())
    {
        context->isLabelDefined.insert(iter, {label.toString(), false});
    }

    return node;
}

Node * LabelStatement(FunctionContext * context, StringRef label)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_LABEL;
    node->stmt.label = new StringRef(label);

    context->isLabelDefined[label.toString()] = true;

    return node;
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
    context->switchToChildren.insert({node, {nullptr}});

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

Node * CaseStatement(FunctionContext * context, u64 caseValue)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_CASE;
    node->stmt.caseValue = caseValue;

    ASSERT(!context->currentSwitch.empty());
    Node * switchNode                       = context->currentSwitch.back();
    std::vector<std::string> & switchLabels = context->targetToLabels[switchNode];

    std::string caseLabel = std::string("@L") + std::to_string(context->nextUniqueLabel++);
    switchLabels.push_back(caseLabel);

    context->nodeToTarget.insert({
        node,
        { switchNode, switchLabels.size() - 1 }
    });
    ASSERT(context->switchToChildren.find(switchNode) != context->switchToChildren.end());
    context->switchToChildren[switchNode].push_back(node);

    return node;
}

Node * DefaultStatement(FunctionContext * context)
{
    Node * node = new Node;
    node->down = node->right = node->up = nullptr;
    node->type = STMT_DEFAULT;
    node->stmt.context = nullptr;

    ASSERT(!context->currentSwitch.empty());
    Node * switchNode                       = context->currentSwitch.back();
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

void GetAllLocalObjectsInDefinitionContext(DefinitionContext * context,
                                           std::vector<ObjectDefinition *> & objects)
{
    ASSERT(context);

    for (Definition * definition : context->definitions[ID_NAMESPACE])
    {
        if (definition->type == OBJECT_DEFINITION)
        {
            ObjectDefinition * objectDefinition = AsObjectDefinition(definition);
            if (objectDefinition->objStorageType == LOCAL_OBJECT)
            {
                objects.push_back(objectDefinition);
            }
        }
    }

    if (context->firstChild)
        GetAllLocalObjectsInDefinitionContext(context->firstChild, objects);

    if (context->next)
        GetAllLocalObjectsInDefinitionContext(context->next, objects);
}

void ForExpressionTreeInFunctionBody(FunctionContext * context,
                                     Node * body,
                                     void(*func)(FunctionContext *, Node *))
{
    ASSERT(body);
    if (body->type > BEGIN_EXPRESSION && body->type < END_EXPRESSION)
    {
        (*func)(context, body);
    }
    else
    {
        for (Node * child = body->down;
             child;
             child = child->right)
        {
            ForExpressionTreeInFunctionBody(context, child, func);
        }
    }
}

void UpdateMaxTempAndCallZone(FunctionContext * context, Node * exprTree)
{
    size_t tempZoneSize = 0;
    size_t callZoneSize = 0;

    // post-order
    std::vector<std::pair<Node *, bool>> st = { { exprTree, false } };
    while (!st.empty())
    {
        Node * node = st.back().first;
        if (!st.back().second)
        {
            st.back().second = true;

            std::vector<Node *> children;
            for (Node * child = node->down; child; child = child->right)
            {
                children.push_back(child);
            }
            for (auto it = children.rbegin(); it != children.rend(); ++it)
            {
                st.push_back({ *it, false });
            }
        }
        else
        {
            // compute tempZoneSize
            if (node->expr.loc.type == NEED_ALLOC)
            {
                size_t align = TypeAlignment(node->expr.type);
                size_t size = TypeSize(node->expr.type);

                ASSERT(align <= 8);

                tempZoneSize = (tempZoneSize + size + align - 1) / align * align;
            }
            // compute callZoneSize
            if (node->type == EXPR_CALL)
            {
                size_t newCallZoneSize = ComputeCallZoneSize(node);

                callZoneSize = callZoneSize < newCallZoneSize
                    ? newCallZoneSize
                    : callZoneSize;
            }

            st.pop_back();
        }
    }

    context->maxTempZoneSize = Max(context->maxTempZoneSize, tempZoneSize);
    context->maxCallZoneSize = Max(context->maxCallZoneSize, callZoneSize);
}

void FillLocalAndTempLocationInExpressionTree(FunctionContext * context,
                                              Node * exprTree)
{
    size_t offset = context->tempZoneBeginOffset;

    // post-order
    std::vector<std::pair<Node *, bool>> st = { { exprTree, false } };
    while (!st.empty())
    {
        Node * node = st.back().first;
        if (!st.back().second)
        {
            st.back().second = true;

            std::vector<Node *> children;
            for (Node * child = node->down; child; child = child->right)
            {
                children.push_back(child);
            }
            for (auto it = children.rbegin(); it != children.rend(); ++it)
            {
                st.push_back({ *it, false });
            }
        }
        else
        {
            // fill location
            if (node->expr.loc.type == NEED_ALLOC)
            {
                size_t align = TypeAlignment(node->expr.type);
                size_t size = TypeSize(node->expr.type);

                ASSERT(align <= 8);

                offset = (offset + size + align - 1) / align * align;

                node->expr.loc.type = ESP_OFFSET;
                node->expr.loc.offsetValue = context->stackFrameSize - offset;
            }
            else if (node->expr.loc.type == SAME_AS_FIRST_CHILD)
            {
                node->expr.loc = node->down->expr.loc;
            }
            else if (node->expr.loc.type == SAME_AS_FIRST_GRANDCHILD)
            {
                node->expr.loc = node->down->down->expr.loc;
            }
            else if (node->expr.loc.type == SEARCH_LOCAL_DEFINITION_TABLE)
            {
                node->expr.loc.type = ESP_OFFSET;
                node->expr.loc.offsetValue = context->stackFrameSize - context->localObjectOffsets.at(node->expr.loc.definitionValue);
            }

            st.pop_back();
        }
    }

    ASSERT(offset <= (context->tempZoneBeginOffset + context->maxTempZoneSize));
}

Location GetSwapLocation(FunctionContext * functionContext)
{
    ASSERT(functionContext->stackFrameSize != 0);
    Location location;
    location.type = ESP_OFFSET;
    location.offsetValue = functionContext->stackFrameSize - functionContext->swapZoneEndOffset;
    return location;
}

void    PrepareStack(FunctionContext * context)
{
    // step:
    //      1. scan for non-volatile registers, fill {dirtyRegisters, registerZoneEndOffset}
    //      2. scan for local objects, fill {localZoneSize, localZoneEndOffset, localObjectOffsets}
    //      3. scan for temp objects, fill {maxTempZoneSize, tempZoneBeginOffset}
    //      4. scan for call nodes, fill {maxCallZoneSize, callZoneEndOffset}
    //      5. fill {swapZoneEndOffset}
    //      6. fill {stackFrameSize, stackAllocSize}
    //      7. fill {local location in expression tree, temp location in expression tree}
    //          * both depends on stackFrameSize

    // "offset to previous stack frame"
    size_t offset = 8;

    context->dirtyRegisters = RBP_MASK | RSI_MASK | RDI_MASK;
    offset = context->registerZoneEndOffset = offset + 8 * CountBits(context->dirtyRegisters);

    // local zone
    std::vector<ObjectDefinition *> objectDefinitions;
    GetAllLocalObjectsInDefinitionContext(context->functionDefinitionContext, objectDefinitions);
    for (ObjectDefinition * objectDefinition : objectDefinitions)
    {
        size_t objectSize = TypeSize(objectDefinition->objType);
        size_t objectAlign = TypeAlignment(objectDefinition->objType);
        context->localZoneSize = (context->localZoneSize + objectSize + objectAlign - 1) / objectAlign * objectAlign;
        context->localObjectOffsets.emplace(&objectDefinition->def, offset + context->localZoneSize);
    }
    offset = context->localZoneEndOffset = offset + context->localZoneSize;

    // temp zone
    ForExpressionTreeInFunctionBody(context,
                                    context->functionBody,
                                    UpdateMaxTempAndCallZone);
    context->tempZoneBeginOffset = (offset + 7) / 8 * 8;
    offset = context->tempZoneBeginOffset + context->maxTempZoneSize;
    offset = (offset + 7) / 8 * 8;

    // swap zone
    offset = context->swapZoneEndOffset = offset + 8;

    // call zone
    offset = context->callZoneEndOffset = (offset + 15) / 16 * 16;

    // stack layout
    context->stackFrameSize = offset;
    context->stackAllocSize = offset - context->registerZoneEndOffset;

    // fill expression node location
    ForExpressionTreeInFunctionBody(context,
                                    context->functionBody,
                                    FillLocalAndTempLocationInExpressionTree);
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
        case STMT_GOTO:         s = "goto"; break;
        case STMT_LABEL:        s = "label"; break;
        case STMT_SWITCH:       s = "switch"; break;
        case STMT_CASE:         s = "case"; break;
        case STMT_DEFAULT:      s = "default"; break;
        case EXPR_ID:           s = "id"; break;
        case EXPR_CONSTANT:     s = "constant"; break;
        case EXPR_CALL:         s = "call"; break;
        case EXPR_CVT_I2I:      s = "cvt_i2i"; break;
        case EXPR_CVT_I2UI:     s = "cvt_i2ui"; break;
        case EXPR_CVT_UI2I:     s = "cvt_ui2i"; break;
        case EXPR_CVT_UI2UI:    s = "cvt_ui2ui"; break;
        case EXPR_CVT_F2F:      s = "cvt_f2f"; break;
        case EXPR_CVT_I2F:      s = "cvt_i2f"; break;
        case EXPR_CVT_F2I:      s = "cvt_f2i"; break;
        case EXPR_CVT_I2B:      s = "cvt_i2b"; break;
        case EXPR_CVT_B2I:      s = "cvt_b2i"; break;
        case EXPR_CVT_F2B:      s = "cvt_f2b"; break;
        case EXPR_CVT_B2F:      s = "cvt_b2f"; break;
        case EXPR_CVT_NOOP:     s = "cvt_noop"; break;
        case EXPR_BOOL_NOT:     s = "bool_not"; break;
        case EXPR_BOOL_AND:     s = "bool_and"; break;
        case EXPR_BOOL_OR:      s = "bool_or"; break;
        case EXPR_INEG:         s = "ineg"; break;
        case EXPR_IINC:         s = "iinc"; break;
        case EXPR_IDEC:         s = "idec"; break;
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
        case EXPR_PADD:         s = "padd"; break;
        case EXPR_PSUB:         s = "psub"; break;
        case EXPR_PDIFF:        s = "pdiff"; break;
        case EXPR_PVAL:         s = "pval"; break;
        case EXPR_PEQ:          s = "peq"; break;
        case EXPR_PNE:          s = "pne"; break;
        case EXPR_PLT:          s = "plt"; break;
        case EXPR_PLE:          s = "ple"; break;
        case EXPR_PGE:          s = "pge"; break;
        case EXPR_PGT:          s = "pgt"; break;
        case EXPR_MADDR:        s = "maddr"; break;
        case EXPR_MCOPY:        s = "mcopy"; break;
        case EXPR_MDUP:         s = "mdup"; break;
        case EXPR_MADD:         s = "madd"; break;
        case EXPR_CONDITION:    s = "condition"; break;
        case EXPR_ELIST:        s = "elist"; break;
        case EMPTY_EXPRESSION:  s = "empty_expr"; break;
        default: ASSERT(false); break;
    }

    return s;
}

void PrintNodeTree(Node * root, std::string indent)
{
    ASSERT(root);

    std::cout << indent << NodeDebugString(root) << std::endl;

    if (root->down)
        PrintNodeTree(root->down, indent + "  ");

    if (root->right)
        PrintNodeTree(root->right, indent);
}

void PrintStackAllocation(FunctionContext * context)
{
    std::cout << "return address:        8" << std::endl
              << "non-volatile register: " << CountBits(context->dirtyRegisters) * 8 << std::endl
              << "local zone:            " << context->localZoneSize << std::endl;
}

void PrintFunctionContext(FunctionContext * context)
{
    PrintNodeTree(context->functionBody, "");
    PrintStackAllocation(context);
}

}