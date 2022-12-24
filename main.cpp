#include <cstring>

#ifdef NOGUI
#include "nogui/include/nogui/nogui.hpp"
#include "nogui/include/nogui/options.hpp"
#ifdef TESTS
#include "tests/nogui.hpp"
#endif
#endif

#ifdef TESTS
#include "tests/config.hpp"
#include "tests/csv.hpp"
#include "tests/sensors.hpp"
#include "tests/stats.hpp"
#include <DOCTEST/doctest.h>
#endif

#ifdef GUI
#include "gui/include/gui/gui.hpp"
#endif
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    if (argc > 1)
    {
        if (!std::strcmp(argv[1], "run_tests"))
        {
#ifdef TESTS
            doctest::Context doctestContext;
            int result = doctestContext.run();

            if (doctestContext.shouldExit())
            {
                return result;
            }

            return result;
#else
            return EXIT_FAILURE;
#endif
        }
        else
        {
#ifdef NOGUI
            nogui::run(argc, argv);
#else
            return EXIT_FAILURE;
#endif
        }
    }
    else
    {
#ifdef GUI
        gui::run(argc, argv);
#else
        return EXIT_FAILURE;
#endif
    }

    return EXIT_SUCCESS;
}
