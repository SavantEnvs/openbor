# Script parser: argument-list parsing walks back from an empty instruction list

`Parser_Arg_expr_list2()` (`Parser.c:2070`) calls `List_GotoPrevious()` on the instruction list, and
`List_GotoPrevious()` (`engine/source/scriptlib/List.c:989`) reads `list->current->prev` with no check that
`list->current` is set. After a syntax error the list can be empty, so a call whose argument list is cut
short (`0(` at the end of the input) dereferences NULL (UBSan `null` at `List.c:989:24`).

`call-after-error.c` is the delta-reduced crasher (`void m(){y;for(v);0("`).

Reproduce: `/mayhem/fuzz_script-standalone mayhem/fuzz_script/known-findings/parser-arg-list-null-node/call-after-error.c`

Covers the `List.c:989` defect of the 300s runs.
