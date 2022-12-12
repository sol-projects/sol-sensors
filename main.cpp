#include "nogui/include/nogui/nogui.hpp"
#include "tests/config.hpp"
#include "tests/csv.hpp"
#include "tests/sensors.hpp"
#include "tests/stats.hpp"
#include "tests/nogui.hpp"
#include <DOCTEST/doctest.h>
#include <cargs.h>
#include "nogui/include/nogui/options.hpp"

int main(int argc, char* argv[])
{
    nogui::run(argc, argv);

    doctest::Context doctestContext;
    int result = doctestContext.run();

    if (doctestContext.shouldExit())
    {
        return result;
    }

    return result;
}
