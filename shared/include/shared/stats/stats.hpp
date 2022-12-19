#pragma once
#include <algorithm>
#include <cmath>
#include <numeric>

namespace stats
{
    auto sum(auto itStart, auto itEnd)
    {
        return std::reduce(itStart, itEnd);
    }

    auto average(auto itStart, auto itEnd)
    {
        return sum(itStart, itEnd) / std::distance(itStart, itEnd);
    }

    auto median(auto itBegin, auto itEnd)
    {
        auto size = std::distance(itBegin, itEnd);
        auto n = size / 2;
        std::nth_element(itBegin, itBegin + n, itEnd);

        if (size % 2 == 0)
        {
            std::nth_element(itBegin, itBegin + n - 1, itEnd);
            return (*(itBegin + n) + *(itBegin + n - 1)) / 2;
        }

        return *(itBegin + n);
    }

    float standardDeviation(auto itBegin, auto itEnd)
    {
        auto size = std::distance(itBegin, itEnd);
        auto avg = average(itBegin, itEnd);

        auto variance = 0.0F;

        while (itBegin != itEnd)
        {
            auto diffFromAverage = *itBegin - avg;
            variance += diffFromAverage * diffFromAverage;
            ++itBegin;
        }

        variance /= size;

        return std::sqrt(variance);
    }
}
