# Script compiler: strcpy of a NULL label (`break`/`continue` outside a loop)

`Parser_Jump_stmt()` (`Parser.c:1166`) looks up the innermost loop's label. Outside a loop it records a parse
error but still emits the jump through `Parser_AddInstructionViaLabel()` with the NULL label, and
`Instruction_InitViaLabel()` (`engine/source/scriptlib/Instruction.c:61:25`) passes it to `strcpy` (UBSan
`nonnull-attribute`, a real null dereference without the sanitizer).

- `break-outside-loop.c`: the minimal input, `void m(){break;}` (`continue` fails the same way)
- `null-label.c`: the first reproducer, the first, unreduced fuzzer crasher

Reproduce: `/mayhem/fuzz_script-standalone mayhem/fuzz_script/known-findings/parser-null-label-strcpy/break-outside-loop.c`

Covers every `Instruction.c:61` defect of the 300s runs. The other parser findings are in
`../parser-null-instruction-deref` and `../parser-arg-list-null-node`.
