//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/log.h"
#include <stdbool.h>

// ORCA LOG C usage

int main(void)
{
    // Test Levels
    {
        ORCA_LOG_ERR("SHOULD NOT SEE THIS");

        orca_log_level_set(ORCA_LOG_LEVEL_ALL);
        ORCA_LOG_ERR();
        ORCA_LOG_WARN();
        ORCA_LOG_INFO();

        orca_log_level_set(ORCA_LOG_LEVEL_ERROR);
        ORCA_LOG_ERR();
        ORCA_LOG_WARN("SHOULD NOT SEE THIS");
        ORCA_LOG_INFO("SHOULD NOT SEE THIS");
    }

    // Logfile
    {
        const char *path = NULL;
        orca_log_logfile_path_get(&path);
        printf("LOGFILE PATH: %s\n", path);
        orca_log_logfile_path_set("newlogfile.log");
    }

    // BASIC USAGE MACRO
    {
        orca_log_level_set(ORCA_LOG_LEVEL_INFO);
        // no message
        ORCA_LOG_INFO();

        // simple string literal
        ORCA_LOG_INFO("HELLO");

        // sprintf style formatting
        char *str2 = strdup("best");
        ORCA_LOG_INFO("The %s number is %i", str2, 23);
        free(str2);

        // does not compile, 1st must be literal fmt string
        // ORCA_LOG_WARN(str1);
    }

    // BASIC USAGE FUNC
    {
        orca_log_level_set(ORCA_LOG_LEVEL_INFO);
        // literal
        orca_log("Literal RAW");

        // c-string
        char *str2 = strdup("c-string RAW");
        orca_log(str2);
        free(str2);

        orca_log_h1("HEADING 1");
        orca_log_h2("HEADING 2");
        orca_log_h3("HEADING 3");
    }

    // Colors functions only
    {
        orca_log_h1("Colors functions only");
        orca_log("DEFAULT COLOR");
        orca_log_color_set(ORCA_LOG_COLOR_GREEN);
        orca_log("DEFAULT COLOR AFTER SET GREEN");
        orca_log_color_set(ORCA_LOG_COLOR_YELLOW);
        strdup("YELLOW ONE OFF");
        orca_log(strdup("YELLOW ONE OFF"));
        orca_log_color_set(ORCA_LOG_COLOR_RESET);
        orca_log("COLOR RESET");
    }

    // Colors using Macros
    {
        orca_log_h1("Colors using Macros");
        orca_log("DEFAULT COLOR");
        orca_log_color_set(ORCA_LOG_COLOR_GREEN);
        ORCA_LOG_ERR("DEFAULT COLOR AFTER SET GREEN");
        orca_log_color_set(ORCA_LOG_COLOR_YELLOW);
        ORCA_LOG_ERR("YELLOW ONE OFF");
        orca_log_color_set(ORCA_LOG_COLOR_RESET);
        ORCA_LOG_ERR("COLOR RESET");
    }

    orca_log("ALL TEST SUCCESSFUL");
}
