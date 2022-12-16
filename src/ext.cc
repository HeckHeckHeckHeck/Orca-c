#include "ext.hh"
#include <iostream>

namespace Orca {
    int counter{ 0 };
}

void test_cxx(Glyph* gbuf, Mark* mbuf, Usz height, Usz width, Usz tick_number)
{
    Orca::counter++;
    std::cout << "test_cxx()" << Orca::counter << std::endl;
}

