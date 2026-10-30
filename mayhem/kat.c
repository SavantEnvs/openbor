/*
 * Known-answer tests for the OpenBOR engine pieces the fuzz targets exercise: PAK container reads,
 * PNG decoding and the script interpreter. Built with the engine's normal flags (no sanitizers).
 * Prints "KAT PASS <name>" / "KAT FAIL <name>" per check and a final "KAT SUMMARY passed=N failed=M".
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "types.h"
#include "bitmap.h"
#include "screen.h"
#include "packfile.h"
#include "sdlport.h"
#include "openborscript/config.h"
#include "Interpreter.h"

/* globals.h redirects printf() into the engine log file; the KAT reports on real stdout */
#undef printf

extern List theFunctionList;
void Script_Global_Init(void);
int pak_init(void);
void pak_term(void);
int loadscreen32(char *filename, char *packfile, s_screen **screen);
int loadimagepalette(char *filename, char *packfile, unsigned char *pal);

static int passed, failed;

static void check(int ok, const char *name)
{
    printf("KAT %s %s\n", ok ? "PASS" : "FAIL", name);
    ok ? passed++ : failed++;
}

static int read_entry(const char *name, const char *pak, char *buf, int cap)
{
    int h = openpackfile((char *)name, (char *)pak);
    int total = 0, n;

    if(h < 0) {
        return -1;
    }
    while(total < cap && (n = readpackfile(h, buf + total, cap - total)) > 0) {
        total += n;
    }
    closepackfile(h);
    return total;
}

static void test_pak(const char *pak, const char *label)
{
    char buf[256], name[96];
    int n;

    snprintf(name, sizeof(name), "%s read hero.txt", label);
    n = read_entry("data/models/hero.txt", pak, buf, sizeof(buf));
    check(n == 21 && memcmp(buf, "name hero\nhealth 100\n", 21) == 0, name);

    snprintf(name, sizeof(name), "%s read blob.bin", label);
    n = read_entry("DATA\\Sprites\\blob.bin", pak, buf, sizeof(buf));
    check(n == 16 && memcmp(buf, "ABCDEFGHIJKLMNOP", 16) == 0, name);

    snprintf(name, sizeof(name), "%s script entry", label);
    n = read_entry("data/scripts/test.c", pak, buf, sizeof(buf));
    check(n == 26 && memcmp(buf, "void main(){ int a = 1; }\n", 26) == 0, name);

    snprintf(name, sizeof(name), "%s missing entry rejected", label);
    check(read_entry("data/models/nobody.txt", pak, buf, sizeof(buf)) < 0, name);
}

static void test_png(void)
{
    s_screen *screen = NULL;
    unsigned char pal[1024];
    unsigned int *px;

    check(loadscreen32("mayhem/kat/rgba4x2.png", NULL, &screen) && screen != NULL, "png rgba load");
    if(screen) {
        px = (unsigned int *)screen->data;
        check(screen->width == 4 && screen->height == 2, "png rgba dimensions");
        check(px[0] == 0xff0000ffu && px[1] == 0xff00ff00u && px[2] == 0x80ff0000u, "png rgba row0 pixels");
        check(px[3] == 0x04030201u && px[7] == 0xffffffffu, "png rgba row0 tail and row1 last pixel");
        freescreen(&screen);
    }
    check(loadimagepalette("mayhem/kat/indexed4x4.png", NULL, pal) != 0, "png palette load");
    check(pal[4] == 255 && pal[9] == 255 && pal[14] == 255 && pal[17] == 10 && pal[18] == 20 &&
          pal[20] == 30 && pal[21] == 40 && pal[22] == 50, "png palette entries (4 bytes each)");
}

static LONG reported;
static int report_calls;

static HRESULT kat_report(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    *pretvar = NULL;
    if(paramCount != 1 || FAILED(ScriptVariant_IntegerValue(varlist[0], &reported))) {
        return E_FAIL;
    }
    report_calls++;
    return S_OK;
}

static int run_script(const char *text, LONG *out)
{
    Interpreter interp;
    char *copy = strdup(text);
    int ok = 0;

    reported = -1;
    report_calls = 0;
    Interpreter_Init(&interp, "kat", &theFunctionList);
    if(SUCCEEDED(Interpreter_ParseText(&interp, copy, 1, "kat.c")) &&
       SUCCEEDED(Interpreter_CompileInstructions(&interp))) {
        Interpreter_Reset(&interp);
        ok = SUCCEEDED(Interpreter_EvaluateImmediate(&interp)) && report_calls == 1;
        *out = reported;
    }
    Interpreter_Clear(&interp);
    free(copy);
    return ok;
}

static void test_script(void)
{
    LONG v = -1;

    check(run_script("int tri(int n){ int t = 0; while(n > 0){ t += n; n--; } return t; }\n"
                     "void main(){ int r = tri(10); katreport(r); }\n", &v) && v == 55, "script function call tri(10) == 55");
    check(run_script("void main(){ int i; int s = 0; for(i = 1; i <= 100; i++){ s += i; } katreport(s); }\n", &v) &&
          v == 5050, "script loop sum == 5050");
    check(run_script("void main(){ katreport((0x1F << 2) | 3); }\n", &v) && v == 127, "script bit ops == 127");
    check(run_script("#define BASE 40\nvoid main(){ katreport(BASE + abs(0 - 2)); }\n", &v) && v == 42,
          "script preprocessor define and native abs == 42");
    check(!run_script("void main( { int x = ; if x > 3 }", &v), "script syntax error rejected");
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    Script_Global_Init();
    List_InsertAfter(&theFunctionList, (void *)kat_report, "katreport");
    test_pak("mayhem/kat/sample32.pak", "pak32");
    test_pak("mayhem/kat/sample64.pak", "pak64");
    test_png();
    test_script();
    printf("KAT SUMMARY passed=%d failed=%d\n", passed, failed);
    return failed ? 1 : 0;
}
