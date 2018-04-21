Lexical Analysis
===

* Character Sets

    + source and execution:
        alphabet: a-z, A-Z
        digit: 0-9
        graphic:  !  "  #  %  &  '  (  )  *  +  ,  -  .  /  :
                  ;  <  =  >  ?  [  \  ]  ^  _  {  |  }  ~
        space: ' '
        control:
            \h - horizontal tab
            \v - vertical tab
            \f - form feed
    + source only:
        end-of-line:
            \n - unix style
            \r\n - windows style
    + execution only:
        control:
            \a - alert
            \b - backspace
            \r - carriage_return
            \n - new_line
    + impl defined:
        graphic: $ @
        other non-display characters...

* API design

    + SourceContext - source code scanner
        + readChar() -> char
        + readLine() -> StringRef
        + line() -> int

    + Token struct
        + type() -> Token::Type
        + value() -> T

    + Tokenizer - convert source into tokens
        + compile(SourceContext cxt) -> void
        + getIterator() -> TokenIterator

    + TokenIterator - iterate tokens
        + has() -> bool
        + next() -> Token
        + peak() -> Token
        + peakN(size_t n) -> Token

    Tokenizer::compile(SourceContext cxt) -> void
    {
        // for each line of code
            // find next token start
            // find next token end
            // recognize token
    }
