namespace pyzz
{
    void zz_init();
}

extern "C"
void init_pyzz()
{
    pyzz::zz_init();
}
