#include "calculator.hpp"

struct calculator
{
    static double sum(double x, double y) { return x + y; }
    static double mul(double x, double y) { return x * y; }
    static double sqrt(double x) { return std::sqrt(x); }
};

DL_EXPORT(team::calculator, calculator)
