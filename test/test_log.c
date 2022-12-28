//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/valog.h"


int main()
{
    // C usage
    ORCA_LOG_ERR();
    ORCA_LOG_ERR("HELLO");
    char *str = strdup("fds");
    ORCA_LOG_ERR("string: %s", str);

    free(str);
}
