IR Generation
===

TODO:
* stmt tree: implement
  * in: SDT
  * out: stmt structure

* expr tree -> ir instructions
* stmt tree -> ir instructions
* expr tree: merge cast node with unary node.
* expr tree: complete type conversion support.
* ir: review instruction set.



+ Eliminated C semantics

> _Only memory and operations._

    1. type casting op.
    2. sizeof op.
    3. symbol, object, type concept.

+ expression tree (eliminate 2,3)

    + structure
        binary-operator operand1 operand2
        unary-operator operand
        cast-operator operand
    + use
		Type Checking

            n1 = node_pop();
            n2 = node_pop();
            typ_check(n1, is(scalar()));
            typ_check(...);
            ...
            n = new_binary_node();
            n.type = typ_from_op(op, n1.type, n2.type);
            n.operator = op;
            n.operand1 = n1;
            n.operand2 = n2;

        Cast Node Insertion

			n1 = node_pop();
			n2 = node_pop();
			...
			n = new_binary_node();
			n.type = typ_from_op(op, n1.type, n2.type);
			if (need_cast_node(to:n.type, from:n1.type))
				n1 = new_cast_node(n.type, n1.type, n1);
			if (need_cast_node(to:n.type, from:n2.type))
				n2 = new_cast_node(n.type, n2.type, n2);
			n.operator = op;
			n.operand1 = n1;
			n.operand2 = n2;

        Code Emit

			eval(node_pop());
			eval_value(node_pop());
			eval_address(node_pop());

    + build

			node_push(Op(node_pop(), node_pop()));
			// Op() knows type checking, cast node insertion

+ the elements of IR instruction set (eliminate 1)

    + path control
        test, cmp
        jmp, jxx
        call, ret
    + numeric computation
        boolean op
        bit op
        relation op
        basic arithmetic op (+-*/%)
    + storage manipulation
        load, store
        push, pop, alloc, free

+ the elements of IR abstract machine

    + storage model: register + memory(stack + heap)
    + thread model: single thread
    + function call model
