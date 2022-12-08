#pragma once
#include <DOCTEST/doctest.h>
#include <shared/stats/stats.hpp>

TEST_CASE("Testing statistics measuring.")
{
    std::vector vec { 1, 10, 5, 13, 45 };

    // manual sum
    int sum = 0;
    for (auto el : vec)
    {
        sum += el;
    }

    SUBCASE("Testing sum and average")
    {
        CHECK(stats::sum(std::begin(vec), std::end(vec)) == sum);
        CHECK(stats::average(std::begin(vec), std::end(vec)) == sum / std::size(vec));
    }

    SUBCASE("Testing median values")
    {
        std::vector evenMedianVec { 6, 5, 3, 4, 5, 6 };
        CHECK(stats::median(std::begin(evenMedianVec), std::end(evenMedianVec)) == (5 + 5) / 2);
        std::vector oddMedianVec { 5, 6, 3, 4, 7 };
        CHECK(stats::median(std::begin(oddMedianVec), std::end(oddMedianVec)) == 5);
    }

    SUBCASE("Testing standard deviation.")
    {
        std::vector standardDeviationVec { 600, 470, 170, 430, 300 };
        CHECK(static_cast<int>(stats::standardDeviation(std::begin(standardDeviationVec), std::end(standardDeviationVec))) == 147);
    }
}
