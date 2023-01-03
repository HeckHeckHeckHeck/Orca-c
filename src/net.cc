#include "net.h"
#include <iostream>
#include <pEp/std_utils.hh>

namespace Orca {
    int counter{ 0 };
}


void net_init() {
    pEp::Utils::file_create("field.comm");
}

void net_test_cxx(Glyph* gbuf, Mark* mbuf, Usz height, Usz width, Usz tick_number)
{
    Orca::counter++;
    std::cout << "test_cxx()" << Orca::counter << std::endl;
}

