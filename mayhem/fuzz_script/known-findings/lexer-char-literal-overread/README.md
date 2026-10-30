# Preprocessor lexer: character literal reads past the end of the text

`pp_lexer_GetNextToken()` (`engine/source/preprocessorlib/pp_lexer.c`) handles a `'` by calling
`CONSUMECHARACTER` for the quote, the character and the closing quote without checking for the NUL
terminator. A script that ends inside a character literal makes it consume the terminator and keep going
(ASan `heap-buffer-overflow`), and the error path consumes up to two more characters. Three sites, one bug:

- `unterminated-quote.c`: a single `'`; the closing-quote check reads past the buffer (`:242:18`)
- `unterminated-char.c`: `'a`; the second `CONSUMECHARACTER` of the error path reads past it (`:252:17`)
- `unterminated-char-2.c`: `'te`; the error path consumes the NUL, and the lexer's next call reads past the
  buffer in its end-of-input check (`:102:15`)

Reproduce: `/mayhem/fuzz_script-standalone mayhem/fuzz_script/known-findings/lexer-char-literal-overread/unterminated-quote.c`

Covers every `pp_lexer.c:242`, `:252` and `:102` defect (first app frame) of the 300s runs.
