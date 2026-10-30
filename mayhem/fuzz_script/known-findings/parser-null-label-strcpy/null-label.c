#define LIMIT 8
int fib(int n)
{
    if (n < 2) { return n; }
    return fib(n - 1) + fib(n - 2);
}

void update(void ent, float dx)
{
    int i = 0;
    while (i < LIMIT)
    s
        i++;
        if (i == 3) { continue; }
        if (i > 6) { break; }
    }
    switch (i)
    {
        case 1:dx  = dx * 2; break;
        default: dx = -dx;
    }
}

void main()
{
    int r = fib(7);
    update(getlocalvar("self"), r * 0.5);
}
