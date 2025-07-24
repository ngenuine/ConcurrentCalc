#include "backend.h"
#include "structs.h"

#include <condition_variable>
#include <iostream>
#include <queue>
#include <thread>
#include <variant>

using namespace std::literals;

Result Solve(const Request& req)
{
    double result    = 0.0;
    char   currentOp = '+';  // первая операция всегда '+'

    for (const auto& entity : req.toEval)
    {
        if (std::holds_alternative<char>(entity))
        {
            currentOp = std::get<char>(entity);  // сохраняем операцию
        }
        else
        {
            double value = std::get<double>(entity);  // следующее число
            switch (currentOp)
            {
            case '+':
                result += value;
                break;
            case '-':
                result -= value;
                break;
            case '*':
                result *= value;
                break;
            case '/':
                result /= value;
                break;
            default:
                result += value;
                break;
            }
        }
    }

    return Result{req.ToString(), result};
}

Backend::Backend() = default;

Backend::~Backend()
{
    flowFinished = true;
    m_cvRequests.notify_all();
    m_cvResults.notify_all();

    if (m_solver.joinable())
        m_solver.join();
    if (m_printer.joinable())
        m_printer.join();
}

void Backend::Start()
{
    flowFinished = false;
    m_solver     = std::thread(&Backend::Solver, this);
    m_printer    = std::thread(&Backend::Printer, this);
}

void Backend::Submit(Request request)
{
    std::lock_guard lock(m_requestsMutex);
    m_requests.push(std::move(request));  // Пополнить очередь.
    Print(m_requests.back(), "Submit"s);
    m_cvRequests.notify_one();  // Уведомить висящие на cv потоки (Evaluator).
}

void Backend::Solver()
{
    while (!flowFinished)
    {
        Request request;

        {
            std::unique_lock lock(m_requestsMutex);
            m_cvRequests.wait(lock, [this] { return !m_requests.empty() || flowFinished; });
            if (!m_requests.empty())
            {
                request = std::move(m_requests.front());
                m_requests.pop();
            }
            else
            {
                continue;
            }
        }

        std::promise<Result> promise;
        std::future<Result>  futureResult = promise.get_future();
        {

            std::lock_guard guard(m_resultsMutex);
            m_results.push(std::move(futureResult));
        }

        auto functor = [this, prom = std::move(promise), req = std::move(request), solve = Solve]() mutable {
            std::this_thread::sleep_for(req.timeout);
            Result result = solve(req);
            prom.set_value(result);

            m_cvResults.notify_one();
        };
        std::thread t(std::move(functor));
        t.detach();
    }
}

void Backend::Printer()
{
    while (!flowFinished)
    {
        std::unique_lock lock(m_resultsMutex);
        m_cvResults.wait(lock, [this] { return !m_results.empty() || flowFinished; });
        if (!m_results.empty())
        {
            Result result = m_results.front().get();
            Print(result, "Printer");
            m_results.pop();
        }
        else
        {
            PrintError("Printer: Очередь пуста"s);
        }
    }
}

void Backend::Print(const Result& result, const std::string& prefix)
{
    std::lock_guard lock(m_printMutex);

    std::string sep = prefix.empty() ? ""s : ": "s;
    std::cout << "\033[32m" << prefix << sep << result.ToString() << "\033[0m" << std::endl;
}

void Backend::Print(const Request& request, const std::string& prefix)
{
    std::lock_guard lock(m_printMutex);
    std::string     sep = prefix.empty() ? ""s : ": "s;
    std::cout << "\033[33m" << prefix << sep << request.ToString() << "\033[0m" << std::endl;
}

void Backend::PrintError(const std::string& error)
{
    std::lock_guard lock(m_printMutex);

    std::cout << "\033[31m" << error << "\033[0m" << std::endl;
}
