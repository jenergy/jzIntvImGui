#include <iostream>

extern "C" int asm_main(int argc, char *argv[]);
int main(int argc, char *argv[])
{
    try
    {
        return asm_main(argc, argv);
    } catch (...)
    {
        std::cerr << "UNHANDLED EXCEPTION during as1600" << std::endl;
    }

    return 1;
}
