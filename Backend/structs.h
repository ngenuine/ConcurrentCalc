#ifndef STRUCTS_H
#define STRUCTS_H

#include <chrono>
#include <string>
#include <variant>
#include <vector>

using ExpressionEntity = std::variant<double, char>;

struct Request
{
    std::vector<ExpressionEntity> toEval;
    std::chrono::seconds          timeout;

    Request();

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