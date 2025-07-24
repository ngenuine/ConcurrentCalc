#include "structs.h"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

Request::Request() = default;

// TODO: добавить валидацию на синтаксическую правильность (две подряд операции, два подря числа без операции,
// неизвестный символ), на деление на 0.
Request::Request(std::string request, std::chrono::seconds sec)
    : timeout(sec)
{
    // TODO: а если число с '-' начинается? Или идет оператор, а потом число с унарным минусом? Доработать парсер
    std::istringstream iss(request);
    char               ch;
    while (iss >> std::ws)
    {
        if (std::isdigit(iss.peek()) || iss.peek() == '.')
        {
            double value;
            iss >> value;
            toEval.emplace_back(value);
        }
        else
        {
            char op;
            iss >> op;
            if (op == '+' || op == '-' || op == '*' || op == '/')
                toEval.emplace_back(op);
        }
    }
}

std::string Request::ToString() const
{
    std::ostringstream oss;
    for (const auto& entity : toEval)
    {
        if (std::holds_alternative<double>(entity))
            oss << std::fixed << std::setprecision(2) << std::get<double>(entity);
        else
            oss << ' ' << std::get<char>(entity) << ' ';
    }

    return oss.str();
}

void Request::Print() const
{
    std::cout << "Request: " << ToString() << " | Timeout: " << timeout.count() << "s\n";
}

std::string Result::ToString() const
{
    std::ostringstream oss;
    oss << expression << " = " << std::fixed << std::setprecision(2) << result;
    return oss.str();
}
