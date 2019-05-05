Preprocessor
* Input: ByteArray
* Do
    > Charset check
    > Remove comments /**/
    ? Expand #include, macro
* Output: SourceContext

Lexer
* Input: SourceContext
* Do
    > Tokenize source line
    > Unescape char constant ex. '\n'
    > Unescape string constant ex. "\n"
* Output: vector<Token>

Ast Parser
* Input: vector<Token>
* Output: Ast tree

IR - Type Context
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

IR - Definition Context
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

IR - Constant Context
* Manage constants in source code (string literal, number)
* Assign unique label to constant

IR - Function Context
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

Ast Compiler
* Input: Ast tree
* Do
    > Build 1 Definition Context
    > Build 1 Type Context
    > Build 1 Constant Context
    > Build N Function Context
* Output: Ast compile context
    * 1 Definition, 1 Type, 1 Constant, N Function

Translation
* Input: Ast compile context
* Do
    > Save/load non-volatile register
    > Function calling
    > Boolean expression shortcut
* Output: x64 program
