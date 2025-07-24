#ifndef STRUCTS_H
#define STRUCTS_H

#include <chrono>
#include <vector>
#include <variant>
#include <string>

using ExpressionEntity = std::variant<double, char>;

struct Request
{
    std::vector<ExpressionEntity> toEval;
    std::chrono::seconds               timeout;

    Request();

    // TODO: добавить валидацию на синтаксическую правильность (две подряд операции, два подря числа без операции,
    // неизвестный символ), на деление на 0
    Request(std::string request, std::chrono::seconds sec);

    std::string ToString() const;

    void Print() const;
};

struct Result
{
    std::string expression;
    double      result;

    std::string ToString() const;
};

#endif  // STRUCTS_H