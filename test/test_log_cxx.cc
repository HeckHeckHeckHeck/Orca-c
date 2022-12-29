//#include <iostream>
//#include <stdio.h>
#include <iostream>
#include <cstring>
#include "../src/log.h"
#include <string>
#include <vector>

// ORCA LOG C++ usage

int main()
{
    // Test Levels
    {
        ORCA_LOG_ERR("SHOULD NOT SEE THIS");

        Orca::Log::set_level(ORCA_LOG_LEVEL_ALL);
        ORCA_LOG_ERR("");
        ORCA_LOG_WARN("");
        ORCA_LOG_INFO("");

        Orca::Log::set_level(ORCA_LOG_LEVEL_ERROR);
        ORCA_LOG_ERR("");
        ORCA_LOG_WARN("SHOULD NOT SEE THIS");
        ORCA_LOG_INFO("SHOULD NOT SEE THIS");
    }

    // BASIC USAGE MACRO
    {
        Orca::Log::set_level(ORCA_LOG_LEVEL_INFO);
        // no message
        ORCA_LOG_INFO("");

        // literals
        ORCA_LOG_INFO("literal");

        // string concats
        std::string str{ "casts" };
        ORCA_LOG_ERR("everything " + str + " to a string");

        // operator<< overloads
        int i{ 23 };
        ORCA_LOG_ERR(i);
    }

    // BASIC USAGE FUNCTIONS
    {
        Orca::Log::set_level(ORCA_LOG_LEVEL_INFO);
        // no message
        Orca::Log::log("");

        // literals
        Orca::Log::log("literal");

        // string concats
        std::string str{ "casts" };
        Orca::Log::log("everything " + str + " to a string");

        // operator<< overloads
        // WONT compile, needs std::string
        // int i{ 23 };
        // Orca::Log::log(i);

        Orca::Log::logH1("HEADING 1");
        Orca::Log::logH2("HEADING 2");
        Orca::Log::logH3("HEADING 3");
    }

    // Colors functions only
    {
        Orca::Log::logH1("Colors functions only");
        Orca::Log::log("BLUE LOG", ORCA_LOG_COLOR_BLUE);
        Orca::Log::log("DEFAULT COLOR");
        Orca::Log::set_color(ORCA_LOG_COLOR_GREEN);
        Orca::Log::log("DEFAULT COLOR AFTER SET GREEN");
        Orca::Log::log("YELLOW ONE OFF", ORCA_LOG_COLOR_YELLOW);
        Orca::Log::log("DEFAULT COLOR AGAIN");
    }

    // Colors using Macros
    {
        Orca::Log::set_color(ORCA_LOG_COLOR_DEFAULT);
        Orca::Log::logH1("Colors using Macros");
        ORCA_LOG_ERR("DEFAULT COLOR");
        Orca::Log::log("BLUE LOG", ORCA_LOG_COLOR_BLUE);
        ORCA_LOG_ERR("DEFAULT COLOR AGAIN");
        Orca::Log::set_color(ORCA_LOG_COLOR_GREEN);
        Orca::Log::log("YELLOW ONE OFF", ORCA_LOG_COLOR_YELLOW);
        ORCA_LOG_ERR("DEFAULT COLOR GREEN");
    }


    // FUNCTIONS
}
