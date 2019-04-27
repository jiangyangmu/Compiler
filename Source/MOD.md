Lexer
* Input: string
* Output: vector<Token>

Ast
* Input: vector<Token>
* Output: Ast tree

IR - Type
* Construct type object
* Support "incomplete" concept - hole in type object
    * For type struct/union
    * Create "incomplete" type object
    * Build the "incomplete" type object into normal type object
    * Complete the "incomplete" type object transparently
        * require type object has value semantics
        * require no type object copying
* Manage type object, query type object prop
    * Build type context (a global struct)

IR - Definition
* Construct definition object
* Manage definition object
    * Organize definition objects as tree of definition object table
    * "scope" concept
* Query definition object
    * Query definition object by namespace, name, and table
* Insert definition object
    * Merge definition and incomplete definition
    * 
* LIMIT: function definition and declaration only occurs in global scope

IR - Constant
* Manage constants in source code (string literal, number)
* Assign unique label to constant

IR - Function
* Construct statement tree
    * Create & bind labels
* Construct expression tree
    * Check type, infer type
    * Insert cast node, cleanup node
    * Link definition in id expression
* Analyze stack usage, arrange stack layout
* Track non-volatile register usage

IR - CallingConvention
* Input: function type object, caller/callee
* Output: location of parameter/return-value

IR - Location

AstCompiler
* Input: Ast tree
* Output: Ast compile context
    * 1 Definition, 1 Type, 1 Constant, N Function

Translation
* Input: Ast coompile context
* Output: x64 program
* Save/load non-volatile register
* Function calling
* Boolean expression shortcut