# 現在のEBNF構文

ただ、メソッドのみ実装して中身がないものもある……

```
global      = funcdef*
funcef      = ident "(" ")" multi_stmt
multi_stmt  = "{" {stmt} "}"
stmt        = multi_stmt
            | "if" "(" expr ")" stmt ["else" stmt]
            | "for" "(" [expr] ";" [expr] ";" [expr] ")" stmt
            | "while" "(" expr ")" stmt
            | "return" [expr] ";"
            | ";"
            | expr ";"
expr        = assign
assign      = conditional
            | conditional "=" assign
            | conditional "+=" assign
            | conditional "-=" assign
            | conditional "*=" assign
            | conditional "/=" assign
            | conditional "%=" assign
conditional = logical_or
logical_or  = logical_and
logical_and = bit_or
bit_or      = bit_xor
bit_or      = logical_or
bit_xor     = bit_and
bit_and     = equality
equality    = comparison {"==" comparison | "!=" comparison}
comparison  = shift {"<" comparison | "<=" comparison | ">" comparison | ">=" comparison}
shift       = add
add         = mul {"+" mul | "-" mul}
mul         = cast {"*" mul | "/" mul | "%" mul}
cast        = monomial
monomial    = ["+" | "-"] term
            | "&" monomial
            | "*" monomial
term        = "(" expr ")"
            | num
            | ident "(" {expr} ")"
            | ident
ident       = letter | "_" {letter | digit | "_"}
num         = digit {digit}

digit       = "0" | "1" | ... | "9"
letter      = "a" | "b" | ... | "z" | "A" | "B" | ... | "Z"
```

```

# BNFについて

https://ja.wikipedia.org/wiki/EBNF

省略可能かつ繰り返し可能な部分は、中括弧 { … } で表される。

```
digit excluding zero = "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
digit                = "0" | digit excluding zero
natural number = digit excluding zero { digit }
```

省略可能な部分は大括弧 [ … ] で表される。

```
integer = "0" | [ "-" ] natural number
```

終端記号は文字や数字のことである。
終端記号を任意の規則で定義したものが非終端記号である。

終端記号や非終端記号を任意の並びで記載するときはカンマで区切られるらしいがこのドキュメントではそこまでの厳密性は求めない。
その他にもいくつか細かい文法を無視しているが、雰囲気は伝わるであろう。
