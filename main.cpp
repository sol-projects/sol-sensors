#include "tests/config.hpp"
#include "tests/sensors.hpp"
#include "tests/csv.hpp"
#include <DOCTEST/doctest.h>

int main()
{
    doctest::Context context;
    int result = context.run();

    if (context.shouldExit())
    {
        return result;
    }

    return result;
}
