# Script lexer: string literal decoded from freed preprocessor token memory

`Token_InitFromPreprocessor()` (`engine/source/scriptlib/Lexer.c:80`) passes the preprocessor token's string
literal pointer to `ScriptString_DecodeLiteral()` (`engine/source/scriptlib/ScriptVariant.c:260:30`), which
reads the literal's first byte (`cursor[0]`). In the reduced input the lexer is called from `Parser_Error2()`
(error recovery) and the literal buffer has already been freed (ASan `heap-use-after-free`, READ of size 1).

`string-after-error.c` is the delta-reduced crasher (`""__FILE__"`).

Reproduce: `/mayhem/fuzz_script-standalone mayhem/fuzz_script/known-findings/script-string-literal-uaf/string-after-error.c`

Covers the `ScriptVariant.c:260` defect of the 300s runs.
