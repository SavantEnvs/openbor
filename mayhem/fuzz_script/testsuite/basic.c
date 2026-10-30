void main()
{
    int i;
    int sum = 0;
    for (i = 0; i < 10; i++)
    {
        sum += i;
    }
    if (sum > 40) { sum = 40; } else { sum = 0; }
}
