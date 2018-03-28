EBNF parser

TODO:
    1. support match-only and run-only

+ terms

    language: a set of token sequences.

    grammer: such a thing, defines the structure of a language. It uses two
        tools, "terminal symbol" and "non-terminal production".

    terminal symbol: repr a token.

    non-terminal production: repr structures, like concatenation, alternation,
        repetition, etc.

    parse tree: the real structure of a token sequence.

+ terms: feature

    inline code: the third element added to EBNF grammer, so we can do
        something when parsing.

    inline code context: what inline code can see. For now it knows self's Ast
        node, thus know itself and direct children's Ast node properties.

    customizable parse tree node: by specifying Ast factory.

    meta grammer: how we want to describe grammer. grammer of the grammer.

+ grammer syntax

    terminal symbol:
      <symb> -> "..."
    inline code:
      <code> -> {...}
    non-terminal production:
      <prod> -> (<symb> | <code> | <prod>)+ with structure {AND, OR, OPT, REP}

    NOTE: <code> should be removable (all children of AND,OR,OPT,REP should be
    solid, that is, contains <symb> in self or child)

+ code syntax

    internal:
      P -> (AND P' ...)   P' = {not CODE-only}
      P -> (OR P' ...)    P' = {not CODE}
      P -> (OPT P')       P' = {not CODE}
      P -> (REP P')       P' = {not CODE}
    leaf:
      P -> (SYM "...")
      P -> (CODE {...})
      P -> (PROD P')      P' = {not CODE}

    output ast:
      (P1
        (P2 (SYM "...")
            (P3
              (SYM "...")))
        (P4 (CODE {...})
            (SYM "...")))

+ code interface

    // grammer definition macros
    API gm_begin(PL, name...) \
        Production name...; \
        ProductionList PL = {name...}
    API gm_end(PL) \
        sanity_check(PL); \
        compute_FIRST_FOLLOW(PL)

    // grammer matching
    API match(ProductionList PL, TokenList TL) -> Ast
    API run(ProductionList PL, TokenList TL)
    API match_run(ProductionList PL, TokenList TL) -> Ast
    {
        return match_run_impl(PL[0], TL, null);
    }

    // customize Ast node (e.g. add property)
    API class AstFactory;

+ code interface: impl

    // 1. before match
    sanity_check(ProductionList PL)
    {
        // CODE should only appear in AND, and AND is not CODE-only
        for P in PL:
          for P' in P.production_tree():
              if P' is SYM:
                  continue
              elif P' is CODE:
                  continue
              elif P' is PROD:
                  continue
              elif P' is AND:
                  check_gt(P'.children.size(), P'.children.count(CODE))
              elif P' is OR:
                  check_zero(P'.children.count(CODE))
              elif P' is OPT:
                  check_false(P'.child.is(CODE))
              elif P' is REP:
                  check_false(P'.child.is(CODE))
    }

    // FIRST generated in SYM, propagate to PROD,AND,OR,OPT,REP
    // FOLLOW generated in AND, propagate to SYM,PROD,OR,OPT,REP
    compute_FIRST_FOLLOW(ProductionList PL)
    {
        D <- {}
        for P in PL:
          for P' in P.production_tree():
            if P' is SYM:

                P'.FIRST <- [P'.symbol]
                P'.FOLLOW <- []

            elif P' is CODE:

                continue

            elif P' is PROD:

                // first rule
                D += (P'.FIRST <- P'.production.FIRST)
                // follow rule
                D += (P'.FOLLOW <- P'.production.FOLLOW)

            elif P' is AND:

                solid_children <- P'.children.filter(CODE)
                // first rule
                D += (P'.FIRST <- solid_children[0].FIRST)
                // follow rule
                for P1, P2 in solid_children.adjacent_iterator:
                    D += (P1.FOLLOW <- P2.FIRST)

            elif P' is OR:

                for child in P'.children:
                    // first rule
                    D += (P'.FIRST <- child.FIRST)
                    // follow rule
                    D += (child.FOLLOW <- P'.FOLLOW)

            elif P' is OPT:

                // first rule
                D += (P'.FIRST <- P'.child.FIRST)
                D += (P'.FIRST <- P'.FOLLOW)
                // follow rule: none

            elif P' is REP:

                // first rule
                D += (P'.FIRST <- P'.child.FIRST)
                D += (P'.FIRST <- P'.FOLLOW)
                // follow rule: none

        // {p1.follow, p3.follow, p1.first, ...}
        FL <- TopoSort(D)

        for each F in FL:
            for each F' in dependent(D, F):
                F' += F
    }

    // 2. do match

    // AND, OR, OPT, REP are just structures,
    // CODE is for execution,
    // we only care about SYM, PROD in Ast.
    match_run_impl(Production P, TokenList TL, Ast &A') -> Ast
    {
        // create Ast node for SYM, PROD
        if P is SYM:

            // finally, match leaf node
            check_eq(P.symbol, TL.next())
            A <- AstFactory::Create(P.name, SYM, P.symbol)

            if A' is null:
                return A
            else:
                A'.children += A
                return null

        elif P is PROD:

            A <- AstFactory::Create(P.name, PROD)
            match_run_impl(P.production, TL, A)

            if A' is null:
                return A
            else:
                A'.children += A
                return null

        // execute CODE
        elif P is CODE:

            P.code.run()
            return null

        // resolve AND, OR, OPT, REP
        elif P is AND:

            for child in P.children:
                match_run_impl(child, TL, A')
            return null

        elif P is OR:

            for child in P.children:
                if match_FIRST(child, TL.peek()):
                    match_run_impl(child, TL, A')
                    break
            return null

        elif P is OPT:

            if match_FIRST(P.child, TL.peek()):
                match_run_impl(P.child, TL, &A')
            return null

        elif P is REP:

            while match_FIRST(P.child, TL.peek()):
                match_run_impl(P.child, TL, A')
            return null
    }

    // only SYM, PROD has stable FIRST
    match_FIRST(Production P, Token T) -> bool
    {
        if P.FIRST includes T:
            return true
        else:
            return false
    }

+ workflow

    tokens() -> TokenList
    {
        return ... tokenize source code ...
    }

    grammer() -> ProductionList
    {
        gm_begin(PL, start, expr, term, fact)

        start ->  (PROD expr)
        expr  ->  (AND
                    (PROD term)
                    (REP
                      (SYM "*")
                      (PROD term)
                      (CODE { emit("*"); })))
        term  ->  (AND
                    (PROD fact)
                    (REP
                      (SYM "+")
                      (PROD fact)
                      (CODE { emit("+"); })))
        fact  ->  (OR
                    (SYM "1-9")
                    (CODE { emit($0.symbol); }))

        gm_end(PL)
        return PL
    }

    main()
    {
        TL <- tokens()
        PL <- grammer()

        // generate Ast only
        A <- match(PL, TL)
        // run CODE only
        run(PL, TL)
        // both
        A <- match_run(PL, TL)
    }

