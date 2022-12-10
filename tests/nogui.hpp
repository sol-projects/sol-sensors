#pragma once
#include "LLOG/llog.hpp"
#include "nogui/nogui.hpp"
#include <DOCTEST/doctest.h>
#include <filesystem>
#include <thread>

TEST_CASE("Testing the console application.")
{
    SUBCASE("Writing load to stdout from RAM continously.")
    {
        char* argv[] = {"app", "-l", "-d=ram", "-i=200"};
        int argc = std::size(argv);

        nogui::run(argc, argv);
    }
}
