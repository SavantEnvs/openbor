# Script parser: prefix `++`/`--` and assignment dereference a NULL instruction

Two sites in `engine/source/scriptlib/Parser.c` take `List_Retrieve(pparser->pIList)` after parsing an operand
and read `pInstruction->theToken` without a NULL check:

- `Parser_Unary_expr()` for prefix `++`/`--` (`:1835`, and the same code for `--` at `:1845`)
- `Parser_Assignment_expr2()` for an assignment whose left side emitted no instruction (`:1321`)

When the operand emitted no instruction (the parser keeps going after a syntax error) the list is empty and
the result is NULL (UBSan `null`, a real null dereference without the sanitizer).

- `prefix-incdec-after-error.c`: delta-reduced crasher for `:1835` (`void m(){switch=0;i++`)
- `assign-after-error.c`: delta-reduced crasher for `:1321` (`void m(){for=0;;+)=`)

Reproduce: `/mayhem/fuzz_script-standalone mayhem/fuzz_script/known-findings/parser-null-instruction-deref/prefix-incdec-after-error.c`

Covers every `Parser.c:1835` and `Parser.c:1321` defect of the 300s runs.
