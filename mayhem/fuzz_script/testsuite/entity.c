void main()
{
    void self = getlocalvar("self");
    float x = getentityproperty(self, "x");
    int hp = getentityproperty(self, "health");
    char name = getentityproperty(self, "name");
    if (hp <= 0) { killentity(self); return; }
    changeentityproperty(self, "position", x + 1.5, 0, 0);
    setglobalvar("lastname", name);
}
