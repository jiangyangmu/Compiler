#include "ir.h"

// Simplify control statements to test-jump
//  1. know location for break/continue [context]
//  2. know location for goto label [map]
//  3. know location for
//      node: next, prev
//      node-list: begin, end, before-begin, before-end
//      [extra-guard-nodes]
//  4. how switch-case? [context]

// Simplify expression tree
//  1. only binary tree: convert unary/multiary node into binary node
//  2. only element op: each node has a known compute op (not a set of)
//  3. compute sizeof
//      ast_derive_type(ast)
//      ast_to_ct(ast)
//      ct_derive_type(ct)

// Type Derivation
//  1. get leaf node type
//      SymbolTable
//  2. synthesize internal node type
//      synthesize_type(op, t1, t2)
//  3. insert implicit cast node
//      implicit_cast(op, t1, t2)

// in type creation
//  ast_to_ctype(ast)
//      1. declaration_specifiers => Type*
//          new tag type & symbol (struct/union/enum/enum-const)
//              (all output in declarator)
//          new typedef name
//      2. declarator => Type*, Name
//          (all output in declaration)
//      3. type-name => Type*

struct __context
{
    struct __switch
    {
        NodeList *add_case_branch;    // add case branch code after this
        Node *change_default_branch;  // to change default branch destination
    };
    Node *continue_here;               // loop
    Node *break_here;                  // loop/switch
    map<StringRef, Node *> *goto_map;  // all contexts in a tree share goto map
    vector<pair<Node *, StringRef>> *goto_fix;
    // TODO: symboltable
};

using NB = NodeBuilder;

ComputeType compute_type(const Ast *node)
{
    ComputeTree ct = COMPUTE_unknown;
    switch (node->nodeType())
    {
        case SN_COMMA_EXPRESSION: ct = COMPUTE_comma; break;
        case SN_ASSIGN_EXPRESSION:
            ct =
                COMPUTE_assign +
                (dynamic_cast<const sn_assign_expression *>(node)->op - ASSIGN);
            break;
        case SN_COND_EXPRESSION: ct = COMPUTE_cond; break;
        case SN_OR_EXPRESSION: ct = COMPUTE_or; break;
        case SN_AND_EXPRESSION: ct = COMPUTE_and; break;
        case SN_BITOR_EXPRESSION: ct = COMPUTE_bit_or; break;
        case SN_BITXOR_EXPRESSION: ct = COMPUTE_bit_xor; break;
        case SN_BITAND_EXPRESSION: ct = COMPUTE_bit_and; break;
        case SN_EQ_EXPRESSION:
            ct = (dynamic_cast<const sn_eq_expression *>(node)->op == REL_EQ)
                     ? COMPUTE_eq
                     : COMPUTE_ne;
            break;
        case SN_REL_EXPRESSION:
            ct = COMPUTE_gt +
                 (dynamic_cast<const sn_rel_expression *>(node)->op - REL_GT);
            break;
        case SN_SHIFT_EXPRESSION:
            ct = (dynamic_cast<const sn_shift_expression *>(node)->op ==
                  BIT_SLEFT)
                     ? COMPUTE_shl
                     : COMPUTE_shr;
            break;
        case SN_ADD_EXPRESSION:
            ct = (dynamic_cast<const sn_add_expression *>(node)->op == OP_ADD)
                     ? COMPUTE_add
                     : COMPUTE_sub;
            break;
        case SN_MUL_EXPRESSION:
            ct = COMPUTE_mul +
                 (dynamic_cast<const sn_mul_expression *>(node)->op - OP_MUL);
            break;
        case SN_CAST_EXPRESSION: ct = COMPUTE_cast; break;
        case SN_UNARY_EXPRESSION:
            switch (dynamic_cast<const sn_unary_expression *>(node)->op)
            {
                case OP_INC: ct = COMPUTE_inc; break;
                case OP_DEC: ct = COMPUTE_dec; break;
                case BIT_AND: ct = COMPUTE_address; break;
                case OP_MUL: ct = COMPUTE_indirect; break;
                case OP_ADD: ct = COMPUTE_pos; break;
                case OP_SUB: ct = COMPUTE_neg; break;
                case BIT_NOT: ct = COMPUTE_bit_not; break;
                case BOOL_NOT: ct = COMPUTE_not; break;
                case SIZEOF: ct = COMPUTE_sizeof; break;
                default: break;
            }
            break;
        case SN_POSTFIX_EXPRESSION:
            switch (dynamic_cast<const sn_unary_expression *>(node)->op)
            {
                case LSB: ct = COMPUTE_subscript; break;
                case LP: ct = COMPUTE_call; break;
                case REFER_TO: ct = COMPUTE_member_ref; break;
                case POINT_TO: ct = COMPUTE_member_ind; break;
                case OP_INC: ct = COMPUTE_post_inc; break;
                case OP_DEC: ct = COMPUTE_post_dec; break;
                default: break;
            }
            break;
        default: break;
    }
    return ct;
}

// expr ast -> ComputeTree
// this is recursive.
ComputeTree *ast_to_ct(const Ast *expr)
{
    ComputeTree *root = nullptr, *curr = nullptr;
    switch (expr->nodeType())
    {
        case SN_COMMA_EXPRESSION:
            for (auto e : expr->getChildren())
            {
                if (root)
                {
                    curr->right =
                        NB::ComputeOp(compute_type(e), ast_to_ct(e), nullptr);
                    curr = curr->right;
                }
                else
                {
                    root =
                        NB::ComputeOp(compute_type(e), ast_to_ct(e), nullptr);
                    curr = root;
                }
            }
            break;
        case SN_ASSIGN_EXPRESSION:
            root = NB::ComputeOp(compute_type(expr),
                                 ast_to_ct(expr->getFirstChild()),
                                 ast_to_ct(expr->getLastChild()));
            break;
        case SN_COND_EXPRESSION:
            root = NB::ComputeOp(
                compute_type(expr), ast_to_ct(expr->getChild(0)),
                NB::ComputeOp(compute_type(expr), ast_to_ct(expr->getChild(1)),
                              ast_to_ct(expr->getChild(2))));
            break;
        case SN_OR_EXPRESSION:
        case SN_AND_EXPRESSION:
        case SN_BITOR_EXPRESSION:
        case SN_BITXOR_EXPRESSION:
        case SN_BITAND_EXPRESSION:
        case SN_EQ_EXPRESSION:
        case SN_REL_EXPRESSION:
        case SN_SHIFT_EXPRESSION:
        case SN_ADD_EXPRESSION:
        case SN_MUL_EXPRESSION:
            root = NB::ComputeOp(compute_type(expr),
                                 ast_to_ct(expr->getFirstChild()),
                                 ast_to_ct(expr->getLastChild()));
            break;
        case SN_CAST_EXPRESSION:   // TODO: type-name
                                   // root->ctype = ast_to_ctype(...)
        case SN_UNARY_EXPRESSION:  // TODO: type-name
            root = NB::ComputeOp(compute_type(expr), nullptr,
                                 ast_to_ct(expr->getFirstChild()));
            break;
        case SN_POSTFIX_EXPRESSION:
            root = NB::ComputeOp(
                compute_type(expr), ast_to_ct(expr->getFirstChild()),
                expr->getChildrenCount() > 1 ? ast_to_ct(expr->getLastChild())
                                             : nullptr);
            break;
        case SN_PRIMARY_EXPRESSION:
            root = NB::ComputeLeaf(
                dynamic_cast<const sn_primary_expression *>(expr)->t);
            break;
        case SN_IDENTIFIER:
            root =
                NB::ComputeLeaf(dynamic_cast<const sn_identifier *>(expr)->id);
            break;
    }
    // assert root != nullptr
    return root;
}

// labeled stmt -> nodes
void labeled_stmt_to_nodes(NodeList &n, const Ast *stmt,
                           const __context &context)
{
    // label : stmt
    if (stmt->getFirstChild()->nodeType() == SN_IDENTIFIER)
    {
        NodeList s;

        // goto label
        (*context.goto_map)[label] = s.begin();

        stmt_ast_to_nodes(s, stmt->getLastChild(), context);

        n.append(s);
    }
    // 'case' expr : stmt
    else if (stmt->getFirstChild()->nodeType() == SN_CONST_EXPRESSION)
    {
        // SyntaxError: context.__switch.add_case_branch == nullptr

        NodeList s;
        stmt_ast_to_nodes(s, stmt->getLastChild(), context);

        // TODO
        // add case control node
        // 1. create compute node
        //      Node *cmp = NB::Cmp(context.__switch.e,
        //      NB::Const(stmt));
        // 2. create control node
        //      Node *c = NB::JumpIfEq(cmp);
        // 3. insert both nodes into proper location
        //      context.__switch.add_case_branch.add(cmp);
        //      context.__switch.add_case_branch.add(c);

        n.append(s);
    }
    // 'default' : stmt
    else
    {
        // SyntaxError: context.__switch.change_default_branch == nullptr

        NodeList s;
        stmt_ast_to_nodes(s, stmt->getLastChild(), context);

        // change default destination
        context.__switch.change_default_branch->ctrl.dest = s.end();

        n.append(s);
    }

    return n;
}
// expression stmt -> nodes
void expression_stmt_to_nodes(NodeList &n, const Ast *stmt,
                              const __context &context)
{
    // generate compute node
    if (stmt->getChildrenCount() == 1)
    {
        Node *e;

        e = NB::ComputeNode(stmt->getFirstChild());

        n.add(e);
    }
}
// selection stmt -> nodes
void selection_stmt_to_nodes(NodeList &n, const Ast *stmt,
                             const __context &context)
{
    // if (E) S
    //      generate compute node E
    //      generate control node -> S.next
    //      generate stmt S
    if (dynamic_cast<const sn_selection_statement *>(stmt)->t == IF &&
        stmt->getChildrenCount() == 2)
    {
        Node *e, *c;
        NodeList s;

        e = NB::ComputeNode(stmt->getFirstChild());
        c = NB::JumpIfFalse(s.end());
        stmt_ast_to_nodes(s, stmt->getLastChild(), context);

        n.add(e);
        n.add(c);
        n.append(s);
    }
    // if (E) S1 else S2
    //      generate compute node E
    //      generate control node -> S2
    //      generate stmt S1
    //      generate control node -> S2.next
    //      generate stmt S2
    else if (dynamic_cast<const sn_selection_statement *>(stmt)->t == IF &&
             stmt->getChildrenCount() == 3)
    {
        Node *e, *c1, *c2;
        NodeList s1, s2;

        e = NB::ComputeNode(stmt->getChild(0));
        c1 = NB::JumpIfFalse(s2.begin());
        c2 = NB::Jump(s2.end());
        stmt_ast_to_nodes(s1, stmt->getChild(1), context);
        stmt_ast_to_nodes(s2, stmt->getChild(2), context);

        n.add(e);
        n.add(c1);
        n.append(s1);
        n.add(c2);
        n.append(s2);
    }
    // switch (E) S
    //      generate compute node E
    //      generate branch node sequence [need know case node]
    //      generate stmt S
    else
    {
        Node *default_jump;
        NodeList e, s;

        e.add(NB::ComputeNode(stmt->getFirstChild()));
        default_jump = NB::Jump(nullptr);

        // set context
        struct __context switch_context;
        {
            switch_context = context;
            switch_context.__switch.add_case_branch = &e;
            switch_context.__switch.change_default_branch = default_jump;
            switch_context.continue_here = nullptr;
            switch_context.break_here = s.end();
        }

        stmt_ast_to_nodes(s, stmt->getLastChild(), switch_context);

        if (default_jump->ctrl.dest == nullptr)
            default_jump->ctrl.dest = s.end();

        n.append(e);
        n.add(default_jump);
        n.append(s);
    }
}
// iteration stmt -> nodes
void iteration_stmt_to_nodes(NodeList &n, const Ast *stmt,
                             const __context &context)
{
    // while (E) S
    //      generate compute node E
    //      generate control node -> S.next
    //      generate stmt S
    if (dynamic_cast<const sn_iteration_statement *>(stmt)->t == WHILE)
    {
        Node *e, *c;
        NodeList s;

        e = NB::ComputeNode(stmt->getFirstChild());
        c = NB::JumpIfFalse(s.end());

        // set context
        struct __context loop_context;
        {
            loop_context = context;
            loop_context.__switch.add_case_branch = nullptr;
            loop_context.__switch.change_default_branch = nullptr;
            loop_context.continue_here = e;
            loop_context.break_here = s.end();
        }

        stmt_ast_to_nodes(s, stmt->getLastChild(), loop_context);

        n.add(e);
        n.add(c);
        n.append(s);
    }
    // do S while (E)
    //      generate stmt S
    //      generate compute node E
    //      generate control node -> S
    else if (dynamic_cast<const sn_iteration_statement *>(stmt)->t == DO)
    {
        Node *e, *c;
        NodeList s;

        e = NB::ComputeNode(stmt->getLastChild());
        c = NB::JumpIfTrue(s.begin());

        // set context
        struct __context loop_context;
        {
            loop_context = context;
            loop_context.__switch.add_case_branch = nullptr;
            loop_context.__switch.change_default_branch = nullptr;
            loop_context.continue_here = e;
            loop_context.break_here = s.end();
        }

        stmt_ast_to_nodes(s, stmt->getFirstChild(), loop_context);

        n.append(s);
        n.add(e);
        n.add(c);
    }
    // for ([E1]; [E2]; [E3]) S
    //      generate compute node E1
    //      generate compute node E2
    //      generate control node -> END
    //      generate stmt S
    //      generate compute node E3
    //      generate control node -> E2
    //    END:
    else
    {
        Node *e1, *e2, *e3, *c1, *c2;
        NodeList s;

        e1 = e2 = e3 = nullptr;
        size_t i = 0;
        if (dynamic_cast<const sn_iteration_statement *>(stmt)->pre)
            e1 = NB::ComputeNode(stmt->getChild(i++));
        if (dynamic_cast<const sn_iteration_statement *>(stmt)->mid)
            e2 = NB::ComputeNode(stmt->getChild(i++));
        if (dynamic_cast<const sn_iteration_statement *>(stmt)->post)
            e3 = NB::ComputeNode(stmt->getChild(i++));

        // set context
        struct __context loop_context;
        {
            loop_context = context;
            loop_context.__switch.add_case_branch = nullptr;
            loop_context.__switch.change_default_branch = nullptr;
            loop_context.continue_here = e;
            loop_context.break_here = s.end();
        }

        stmt_ast_to_nodes(s, stmt->getLastChild(), loop_context);

        c1 = e2 ? NB::JumpIfFalse(n.end()) : nullptr;
        c2 = NB::Jump(e2 ? e2 : s.begin());

        if (e1)
            n.add(e1);
        if (e2)
            n.add(e2);
        if (c1)
            n.add(c1);
        n.append(s);
        if (e3)
            n.add(e3);
        n.add(c2);
    }
}
// jump stmt -> nodes
void jump_stmt_to_nodes(NodeList &n, const Ast *stmt, const __context &context)
{
    // goto L
    //      generate control node -> L
    if (dynamic_cast<const sn_jump_statement *>(stmt)->t == GOTO)
    {
        Node *c;

        c = NB::Jump(nullptr);

        // add goto fix
        StringRef label =
            dynamic_cast<const sn_identifier *>(stmt->getFirstChild())
                ->name_info_;
        (*context.goto_fix).push_back({c, label});

        n.add(c);
    }
    // break
    //      generate control node -> END of loop/switch
    else if (dynamic_cast<const sn_jump_statement *>(stmt)->t == BREAK)
    {
        // SyntaxError: context.break_here == nullptr

        Node *c;

        c = NB::Jump(context.break_here);

        n.add(c);
    }
    // continue
    //      generate control node -> BEGIN of loop
    else if (dynamic_cast<const sn_jump_statement *>(stmt)->t == CONTINUE)
    {
        // SyntaxError: context.continue_here == nullptr

        Node *c;

        c = NB::Jump(context.continue_here);

        n.add(c);
    }
    // return [E]
    //      generate compute node E
    //      generate control node -> EXIT
    else
    {
        Node *e, *c;

        e = nullptr;
        if (stmt->getChildrenCount() == 1)
            e = stmt_ast_to_nodes(stmt->getFirstChild());

        c = e ? NB::ReturnWith(e) : NB::Return();

        if (e)
            n.add(e);
        n.add(c);
    }
}

// stmt -> nodes
// expand & dispatch.
void stmt_ast_to_nodes(NodeList &n, const Ast *stmt, const __context &context)
{
    // compound-stmt: expand
    if (stmt->nodeType() == SN_COMPOUND_STATEMENT)
    {
        if (stmt->getChildCount() > 0 &&
            stmt->getLastChild()->nodeType() == SN_STATEMENT_LIST)
        {
            NodeList s;
            // todo: set context SymbolTable
            stmt_ast_to_nodes(s, stmt->getLastChild(), context);
            n.append(s);
        }
    }
    // stmt-list: expand
    else if (stmt->nodeType() == SN_STATEMENT_LIST)
    {
        for (auto child_stmt : stmt->getChildren())
        {
            NodeList s;
            stmt_ast_to_nodes(s, child_stmt, context);
            n.append(s);
        }
    }
    // labeled-stmt: delegate
    else if (stmt->nodeType() == SN_LABEL_STATEMENT)
    {
        NodeList s;
        labeled_stmt_to_nodes(s, stmt, context);
        n.append(s);
    }
    // expression-stmt: delegate
    else if (stmt->nodeType() == SN_EXPRESSION_STATEMENT)
    {
        NodeList s;
        expression_stmt_to_nodes(s, stmt, context);
        n.append(s);
    }
    // selection-stmt: delegate
    else if (stmt->nodeType() == SN_SELECTION_STATEMENT)
    {
        NodeList s;
        selection_stmt_to_nodes(s, stmt, context);
        n.append(s);
    }
    // iteration-stmt: delegate
    else if (stmt->nodeType() == SN_ITERATION_STATEMENT)
    {
        NodeList s;
        iteration_stmt_to_nodes(s, stmt, context);
        n.append(s);
    }
    // jump-stmt: delegate
    else if (stmt->nodeType() == SN_JUMP_STATEMENT)
    {
        NodeList s;
        jump_stmt_to_nodes(s, stmt, context);
        n.append(s);
    }
}

void CodeAst::generate(const Ast *ast)
{
    // assert(ast is stmt);
    map<StringRef, Node *> goto_map;
    vector<pair<Node *, StringRef>> goto_fix;
    struct __context context = {
        {nullptr, nullptr}, nullptr, nullptr, &goto_map, &goto_fix};

    stmt_ast_to_nodes(nodes, ast, context);

    // fix gotos
    for (auto nl : goto_fix)
    {
        // assert nl.first->type == ctrl
        // assert goto_map has nl.second
        nl.first->ctrl.dest = goto_map[nl.second];
    }
    // fix control node jump to guard nodes
    for (auto &node : nodes)
    {
        if (node->type == NT_CONTROL)
        {
            Node *n = node->ctrl.dest;
            if (node->ctrl.dest->type == NT_BEGIN_GUARD)
            {
                do
                {
                    n = n->prev;
                    // assert (n)
                } while (n->isGuard());
            }
            else if (node->ctrl.dest->type == NT_END_GUARD)
            {
                do
                {
                    n = n->next;
                    // assert (n)
                } while (n->isGuard());
            }
            node->ctrl.dest = n;
        }
    }
    // remove guard nodes
    for (auto &node : nodes)
    {
        if (node->type == NT_BEGIN_GUARD || node->type == NT_END_GUARD)
            nodes.erase(node);
    }
}
