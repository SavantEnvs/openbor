#import "data/scripts/lib.c"
void main()
{
    char s = "hello" + " " + "world";
    int a = 0x1F;
    int b = a << 2 | 3 & ~1 ^ 5;
    int c = (a > b) ? a : b;
    float f = 1.5e2 + .25;
    log(s); log("\n");
    drawstring(10, 10, 0, s);
}
