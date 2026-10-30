#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "openborscript/config.h"
#include "Interpreter.h"

extern List theFunctionList;
void Script_Global_Init(void);

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    Script_Global_Init();
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char *text;
    Interpreter *interp;
    int parsed;

    text = malloc(size + 1);
    interp = malloc(sizeof(*interp));
    if(!text || !interp) {
        free(text);
        free(interp);
        return -1;
    }
    memcpy(text, data, size);
    text[size] = '\0';

    Interpreter_Init(interp, "fuzz", &theFunctionList);
    parsed = SUCCEEDED(Interpreter_ParseText(interp, text, 1, "fuzz.c"));
    if(parsed) {
        Interpreter_CompileInstructions(interp);
    }
    Interpreter_Clear(interp);

    free(interp);
    free(text);
    return parsed ? 0 : -1;
}
