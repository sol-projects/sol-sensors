#pragma once
#include "LLOG/llog.hpp"
#include "nogui/nogui.hpp"
#include <DOCTEST/doctest.h>
#include <filesystem>
#include <thread>
#include <type_traits>

TEST_CASE("Testing the console application.")
{
    SUBCASE("Outputing help.")
    {
        const char* argv[] = { "", "-h", nullptr };
        auto argc = std::size(argv);

        // std::ostringstream oss;
        // std::streambuf* p_cout_streambuf = std::cout.rdbuf();
        // std::cout.rdbuf(oss.rdbuf());

        nogui::run(argc, const_cast<char**>(argv));

        // std::cout << oss.str();
        // std::cout.rdbuf(p_cout_streambuf);
    }

    SUBCASE("Writing load to stdout from RAM continously.")
    {
        const char* argv[] = { "", "-l"
                                   "-d=ram"
                                   "-i=200",
            nullptr };
        auto argc = std::size(argv);

        // std::ostringstream oss;
        // std::streambuf* p_cout_streambuf = std::cout.rdbuf();
        // std::cout.rdbuf(oss.rdbuf());

        nogui::run(argc, const_cast<char**>(argv));

        // std::cout << oss.str();
        // std::cout.rdbuf(p_cout_streambuf);
    }
}
