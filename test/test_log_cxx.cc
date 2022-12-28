//#include <iostream>
//#include <stdio.h>
#include <iostream>
#include <cstring>
#include "../src/valog.h"
#include <string>

int main()
{
    // C++ usage
    ORCA_LOG_ERR();
    ORCA_LOG_ERR("HELLO");
    std::string str{ "fds" };
    ORCA_LOG_ERR("string: %s", str.c_str());
    //    {
    //        // C++ usage
    //        ORCA_LOG_ERR("%s",std::string("test: " + std::string(str) + "end").c_str());
    //    }
}
