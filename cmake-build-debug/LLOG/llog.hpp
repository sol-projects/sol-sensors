#pragma once

#if __cplusplus < 202002L
  #error LLOG ERROR: C++20 Required
#endif

#include <string>
#include <string_view>
#include <concepts>
#include <iterator>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <source_location>
#include <algorithm>

#if defined(_WIN32) && defined(LLOG_COLORS_ENABLED)
    #include "windows.h"
    #define LLOG_COLOR_WINDOWS
#endif

namespace llog
{
    template<typename T, typename ...U>
    concept IsAnyOf = (std::same_as<T, U> || ...);

    template<typename T>
    concept Iterable = std::forward_iterator<typename T::const_iterator>;

    template<typename T>
    concept Iterator = std::forward_iterator<T>;

    template<typename T>
    concept Numeric = std::floating_point<T> || std::integral<T>;

    template<typename T>
    concept String = IsAnyOf<std::remove_cvref_t<std::remove_pointer_t<std::decay_t<T>>>, char, wchar_t, std::string, std::string_view>;

    template<typename T>
    concept Array = std::is_array_v<std::remove_reference_t<T>>;

    template<typename T>
    concept Outputable = requires (T x) { std::cout << x; };

    template<typename T>
    concept Inputable = requires (T x) { std::cin >> x; };

    template<typename T>
    concept Printable = Outputable<T> && (!Array<T> || String<T>);

    template<typename T>
    concept NotString = !String<T>;

    template<typename T>
    concept Container = (Iterable<T> || Array<T>) && NotString<T>;

    enum class Color
    {
        #ifdef LLOG_COLOR_WINDOWS
            BLACK = 0x00, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, WHITE, GREY, LIGHT_BLUE,
            LIGHT_GREEN, LIGHT_CYAN, LIGHT_RED, LIGHT_MAGENTA, YELLOW, HIGH_INTENSITY_WHITE, DEFAULT
        #endif

        #if !defined(LLOG_COLOR_WINDOWS)
            BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, HIGH_INTENSITY_WHITE, DEFAULT
        #endif
    };

    struct PrintTemplate
    {
        std::ostream* os;
        const char* start;
        const char* delimiter;
        const char* end;
        Color color = Color::HIGH_INTENSITY_WHITE;
    };

    namespace
    {
        PrintTemplate m_printTemplate
        {
            .os = &std::cout,
            .start = "",
            .delimiter = " ",
            .end = "\n",
        };

        PrintTemplate m_arrayTemplate
        {
            .os = &std::cout,
            .start = "",
            .delimiter = "\n",
            .end = "\n",
        };

        #if defined(LLOG_COLOR_WINDOWS) && defined(LLOG_COLORS_ENABLED) && defined(LLOG_ENABLED)
            const HANDLE m_WindowsConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        #endif

        Color m_defaultColor = Color::HIGH_INTENSITY_WHITE;
        void SetColor([[maybe_unused]]Color color = m_defaultColor)
        {
            #if defined(LLOG_COLOR_WINDOWS) && defined(LLOG_COLORS_ENABLED) && defined(LLOG_ENABLED)
                SetConsoleTextAttribute(m_WindowsConsoleHandle, static_cast<int>(color));
            #endif

            #if !defined(LLOG_COLOR_WINDOWS) && defined(LLOG_COLORS_ENABLED) && defined(LLOG_ENABLED)
                std::cout << "\033[" << static_cast<int>(color) << "m";
            #endif
        }

        bool firstArg = false;

        #ifdef LLOG_ENABLED
            auto print = [](std::ostream* os, const auto& ...x) { (*os << ... << x); };
            auto printWithSpace = []([[maybe_unused]]const auto& ...x) { ((std::cout << x << ' '), ...); };
            auto input = [](auto& type, auto& ...x) { (type >> ... >> x); };
        #endif

        #if !defined(LLOG_ENABLED)
            auto print = []([[maybe_unused]]const auto& ...x) { };
            auto printWithSpace = []([[maybe_unused]]const auto& x) { };
            auto input = []([[maybe_unused]]auto& type, [[maybe_unused]]auto& ...x) { };
        #endif
    }

    void Print(const PrintTemplate& pt, const Container auto& arr)
    {
        SetColor(pt.color);

        if(!firstArg)
        {
            print(pt.os, pt.start);
        }

        std::ranges::for_each(arr, printWithSpace);

        if(!firstArg)
        {
            print(pt.os, pt.end);
        }
    }

    void Print(const PrintTemplate& pt, const Container auto& arg, const Container auto&... args)
    {
        SetColor(pt.color);

        if(!firstArg)
        {
            firstArg = true;
            print(pt.os, pt.start);
        }

        if constexpr(sizeof...(args) > 0)
        {
            Print(pt, arg);
            print(pt.os, pt.delimiter);
            Print(pt, args...);
        }

        if constexpr(sizeof...(args) == 1)
        {
            print(pt.os, pt.end);
            firstArg = false;
        }

        SetColor();
    }

    void Print(const Container auto& arg, const Container auto&... args)
    {
        Print(m_arrayTemplate, arg, args...);
    }

    void Print(const PrintTemplate& pt, Printable auto&& arg, Printable auto&&... args)
    {
        SetColor(pt.color);

        print(pt.os, pt.start, arg);
        (print(pt.os, pt.delimiter, args), ...);
        print(pt.os, pt.end);

        SetColor();
    }

    void Print(Printable auto&& arg, Printable auto&&... args)
    {
        Print(m_printTemplate, arg, args...);
    }

    void Print(const PrintTemplate& pt, Iterator auto& itBegin, Iterator auto& itEnd)
    {
        SetColor(pt.color);

        print(pt.os, pt.start);
        for(;itBegin != itEnd; ++itBegin)
        {
            print(pt.os, *itBegin, pt.delimiter);
        }
        print(pt.os, pt.end);

        SetColor();
    }

    void Print(Iterator auto itBegin, Iterator auto itEnd)
    {
        Print(m_printTemplate, itBegin, itEnd);
    }

    void Input(Inputable auto&... args)
    {
        input(std::cin, args...);
    }

    template<typename ...Args>
    void InputLine(std::istream& input, Args&... args)
        requires (std::same_as<Args, std::string> && ...)
    {
        (std::getline(input, args), ...);
    }

    void PrintToFile([[maybe_unused]]std::ofstream& ofs, [[maybe_unused]]const PrintTemplate& pt, [[maybe_unused]]Printable auto&& arg, [[maybe_unused]]Printable  auto&&... args)
    {
        #ifdef LLOG_ENABLED
            ofs << pt.start << arg;
            ((ofs << pt.delimiter << args), ...);
            ofs << pt.end;
        #endif
    }

    void PrintToFile(std::ofstream& ofs, Printable auto&& arg, Printable auto&&... args)
    {
        PrintToFile(ofs, m_printTemplate, arg, args...);
    }

    void PrintToFile(const std::filesystem::path& path, Printable auto&& arg, Printable auto&&... args)
    {
        std::ofstream ofs(path);
        PrintToFile(ofs, m_printTemplate, arg, args...);
    }

    inline void PrintFromFile([[maybe_unused]]std::ifstream& ifs)
    {
        #ifdef LLOG_ENABLED
            if(ifs.is_open())
            {
                std::cout << ifs.rdbuf();
            }
        #endif
    }

    inline void PrintFromFile(const std::filesystem::path& path)
    {
        std::ifstream ifs(path);
        PrintFromFile(ifs);
    }

    [[nodiscard]]inline std::string Location(const std::source_location& location = std::source_location::current())
    {
        return "[FILE: "
                + std::string(location.file_name())
                + " LINE: "
                + std::to_string(location.line())
                + "] ";
    }

    namespace pt
    {
        constexpr PrintTemplate error
        {
            .os = &std::cerr,
            .start = "Error: ",
            .delimiter = " ",
            .end = "\n",
            .color = Color::RED
        };

        constexpr PrintTemplate warning
        {
            .os = &std::cerr,
            .start = "Warning: ",
            .delimiter = " ",
            .end = "\n",
            .color = Color::YELLOW
        };

        constexpr PrintTemplate message
        {
            .os = &std::cout,
            .start = "Message: ",
            .delimiter = " ",
            .end = "\n",
            .color = Color::GREEN
        };
    }
}
