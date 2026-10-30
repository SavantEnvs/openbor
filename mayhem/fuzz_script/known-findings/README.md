# fuzz_script: triage of the 300s runs

`fuzz-script` reports a growing number of defects from run to run because the parser and lexer have many
unchecked paths after a syntax error, and each run reaches a few more. All of them are in engine code (none
in the harness), reached through `Interpreter_ParseText()` and `Interpreter_CompileInstructions()` the way
the engine loads a script. The defects seen so far (runs 1, 3 and 4: 8, 11 and 18 defects) are 11 distinct
first-application-frame sites that collapse to six root causes in two families. Each has a reduced
reproducer that was re-run against the committed image.

| First app frame(s) | Root cause | Reproducer directory |
|---|---|---|
| `pp_lexer.c:242:18`, `:252:17`, `:102:15` | char literal at end of text consumes the NUL and reads past the buffer | `lexer-char-literal-overread/` |
| `pp_lexer.c:357:9`, `:625:13`, `:648:17` | identifier or number over 128 chars writes `theTokenSource[129]` | `lexer-long-token-overflow/` |
| `Instruction.c:61:25` | `break`/`continue` outside a loop passes a NULL label to `strcpy` | `parser-null-label-strcpy/` |
| `Parser.c:1835:68`, `:1321:68` | prefix `++`/`--` and assignment use the `List_Retrieve()` result unchecked | `parser-null-instruction-deref/` |
| `List.c:989:24` | `Parser_Arg_expr_list2` walks back from a NULL current node | `parser-arg-list-null-node/` |
| `ScriptVariant.c:260:30` | string literal decoded from already-freed preprocessor token memory | `script-string-literal-uaf/` |

Family 1, lexer bounds: an unterminated `'` literal reads past the NUL, and a token longer than the
128-character buffer overflows it. They are two separate bugs.
Family 2, parser/script-lexer error recovery: the parser does not guard against an empty instruction list,
a missing loop label, or a freed token after a syntax error.
