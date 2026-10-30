# Preprocessor lexer: identifier or number longer than MAX_TOKEN_LENGTH overflows the token buffer

`pp_lexer_GetTokenIdentifier()` (`engine/source/preprocessorlib/pp_lexer.c:357`) and
`pp_lexer_GetTokenNumber()` (`:625`, the integer digit loop; `:648`, the digits after a `.`) call `CONSUMECHARACTER` in a loop with no length
check, or check only after writing. `CONSUMECHARACTER` stores the character and a NUL into
`theTokenSource[]`, a `CHAR[129]` (`MAX_TOKEN_LENGTH` is 128), so the 129th character writes one element past
the end (UBSan `array-bounds`, index 129 of `CHAR[129]`). This is independent of the NUL-terminator bug in
`../lexer-char-literal-overread`.

- `long-identifier.c`: `void m() { <140 x N>; }` (`:357`)
- `long-number.c`: `void m() { x = <140 x 3>; }` (`:625`)
- `long-fraction.c`: `void m() { x = 1.<140 x 3>; }` (`:648`)

Reproduce: `/mayhem/fuzz_script-standalone mayhem/fuzz_script/known-findings/lexer-long-token-overflow/long-identifier.c`

Covers every `pp_lexer.c:357`, `:625` and `:648` defect (first app frame) of the 300s runs.
