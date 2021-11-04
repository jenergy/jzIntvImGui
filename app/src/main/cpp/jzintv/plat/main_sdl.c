/* This file exists because, on some platforms, sdl.h redefines 'main'. */
#include "config.h"
#include "sdl_jzintv.h"

int main(int argc, char *argv[])
{
    return jzintv_entry_point(argc, argv);
}
