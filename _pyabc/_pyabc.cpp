namespace pyabc
{
    void init();
}

extern "C"
void init_pyabc()
{
    pyabc::init();
}
