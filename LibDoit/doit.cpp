#include "doit.h"
#include <stdexcept>

double DoIt(int TypeWork, double OperandA, double OperandB) noexcept(false)
{
    if (OperandB == 0 && static_cast<char>(TypeWork) == '/')
        throw std::logic_error("(LibDoIt) Деление на 0");
    return 42;
}
