#include "arithmetic.h"

#include <QDebug>
#include <QString>

#include <dlfcn.h>

constexpr double EPSILON = 1e-9;

using DoItFunc                = double (*)(int, double, double);
static DoItFunc g_pDoIt       = &LocalDoIt;
void*           g_pLibHandle  = nullptr;
bool            isOutsideDoIt = false;

std::vector<ExpressionEntity> Simplify(const std::vector<ExpressionEntity>& input)
{
    std::vector<ExpressionEntity> output;

    for (size_t i = 0; i < input.size();)
    {
        if (std::holds_alternative<char>(input[i]) && std::get<char>(input[i]) == '-')
        {
            // Подсчитываем количество подряд идущих '-'.
            size_t j          = i;
            int    minusCount = 0;

            while (j < input.size() && std::holds_alternative<char>(input[j]) && std::get<char>(input[j]) == '-')
            {
                minusCount++;
                j++;
            }

            // Заменяем группу '-' на один знак.
            char simplifiedOp = (minusCount % 2 == 0) ? '+' : '-';
            output.emplace_back(simplifiedOp);

            i = j;  // Пропускаем все обработанные минусы.
        }
        else
        {
            output.push_back(input[i]);
            ++i;
        }
    }

    // Применить унарные минусы к числам, если перед числом с унарным минусом '*' или '/'.
    bool                isPrevMulOrDiv = false;
    std::vector<size_t> toRemove;

    for (size_t i = 0; i < output.size(); ++i)
    {
        if (std::holds_alternative<char>(output[i]) &&
            (std::get<char>(output[i]) == '*' || std::get<char>(output[i]) == '/'))
        {
            isPrevMulOrDiv = true;
        }
        else if (std::holds_alternative<char>(output[i]) && std::get<char>(output[i]) == '-' && isPrevMulOrDiv &&
                 i + 1 < output.size())
        {
            if (std::holds_alternative<double>(output[i + 1]))
            {
                double minusValue = -std::get<double>(output[i + 1]);
                output[i + 1]     = minusValue;
                toRemove.push_back(i);
                isPrevMulOrDiv = false;
                ++i;  // Пропускаем уже обработанное число.
            }
        }
        else
        {
            isPrevMulOrDiv = false;
        }
    }

    // Удаляем все '-' по индексам.
    for (auto it = toRemove.rbegin(); it != toRemove.rend(); ++it)
    {
        output.erase(output.begin() + *it);
    }

    return output;
}

bool TryLoadDoIt(const std::string& path)
{
    if (g_pLibHandle)
        return true;

    g_pLibHandle = dlopen(path.c_str(), RTLD_LAZY);
    if (!g_pLibHandle)
    {
        qWarning() << "dlopen failed:" << dlerror();
        g_pDoIt = LocalDoIt;
        return false;
    }

    qDebug() << "Функция DoIt загружена из" << QString::fromStdString(path);

    dlerror();
    return true;
}

void UnloadDoIt()
{
    if (g_pLibHandle)
    {
        dlclose(g_pLibHandle);
        g_pLibHandle = nullptr;
    }

    g_pDoIt = LocalDoIt;
}

bool SwithcImplementation()
{
    if (isOutsideDoIt)
    {
        g_pDoIt       = LocalDoIt;
        isOutsideDoIt = false;
        return isOutsideDoIt;
    }

    dlerror();
    void*       sym = dlsym(g_pLibHandle, "DoIt");
    const char* err = dlerror();
    if (err)
    {
        qWarning() << "dlsym failed:" << dlerror();
        g_pDoIt = LocalDoIt;
        return false;
    }

    g_pDoIt       = reinterpret_cast<DoItFunc>(sym);
    isOutsideDoIt = true;

    return isOutsideDoIt;
}

double LocalDoIt(int TypeWork, double OperandA, double OperandB) noexcept(false)
{
    char currentOp = TypeWork;

    if (currentOp != '+' && currentOp != '-' && currentOp != '*' && currentOp != '/')
        throw std::logic_error("Такой операции не существует");

    switch (currentOp)
    {
    case '+': {
        OperandA += OperandB;
        break;
    }
    case '-': {
        OperandA -= OperandB;
        break;
    }
    case '*': {
        OperandA *= OperandB;
        break;
    }
    case '/': {
        if (std::abs(OperandB) < EPSILON)
            throw std::logic_error("(LocalDoIt) Деление на 0");
        OperandA /= OperandB;
        break;
    }
    default: {
        OperandA += OperandB;
        break;
    }
    }

    return OperandA;
}

Result Solve(const Request& req)
{
    double result    = 0.0;
    char   currentOp = '+';

    auto simple = Simplify(req.toEval);

    for (const auto& entity : simple)
    {
        if (std::holds_alternative<char>(entity))
        {
            currentOp = std::get<char>(entity);
        }
        else
        {
            double value = std::get<double>(entity);
            result       = g_pDoIt(currentOp, result,
                                   value);  // Переключаемая реализация: LocalDoIt или из динамической библиотеки.
        }
    }

    return Result{req.ToString(), result};
}
