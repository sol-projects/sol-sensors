#include <DOCTEST/doctest.h>
#include "tests/config.hpp"
#include "tests/sensors.hpp"

int main()
{
    doctest::Context context;
    int result = context.run();

    if(context.shouldExit())
    {
        return result;
    }

    return result;
}
