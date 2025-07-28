#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include "structs.h"

Result                        Solve(const Request& req);
double                        LocalDoIt(int TypeWork, double OperandA, double OperandB) noexcept(false);
std::vector<ExpressionEntity> Simplify(const std::vector<ExpressionEntity>& input);
bool                          TryLoadDoIt(const std::string& path);
void                          UnloadDoIt();
bool                          SwithcImplementation();

#endif  // ARITHMETIC_H
