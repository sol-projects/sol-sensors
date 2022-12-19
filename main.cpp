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

int main(int argc, char* argv[])
{
    #ifdef NOGUI
        nogui::run(argc, argv);
    #endif

    #ifdef TESTS
        doctest::Context doctestContext;
        int result = doctestContext.run();

        if (doctestContext.shouldExit())
        {
            return result;
        }

        return result;
    #endif

    return 0;
}
ja
