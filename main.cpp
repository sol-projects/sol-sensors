#include "nogui/include/nogui/nogui.hpp"
#include "tests/config.hpp"
#include "tests/csv.hpp"
#include "tests/sensors.hpp"
#include "tests/stats.hpp"
#include "tests/nogui.hpp"
#include <DOCTEST/doctest.h>
#include <cargs.h>

int main(int argc, char* argv[])
{
    doctest::Context context;
    int result = context.run();

    if (context.shouldExit())
    {
        return result;
    }

    return result;
}
