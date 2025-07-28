#include "structs.h"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

Request::Request() = default;

Request::Request(std::string request, std::chrono::seconds sec)
    : timeout(sec)
{
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
        {
            double value = std::get<double>(entity);
            if (value == static_cast<int>(value))
                oss << static_cast<int>(value);
            else
                oss << std::fixed << std::setprecision(2) << value;
        }
        else
        {
            oss << ' ' << std::get<char>(entity) << ' ';
        }
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
    oss << expression << " = ";

    if (result == static_cast<int>(result))
        oss << static_cast<int>(result);
    else
        oss << std::fixed << std::setprecision(2) << result;

    return oss.str();
}
