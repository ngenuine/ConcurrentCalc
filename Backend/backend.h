#ifndef BACKEND_H
#define BACKEND_H

#include "structs.h"

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <string>

Result Solve(const Request& req);

// double DoIt(int TypeWork, double OperandA, double OperandB) noexcept(false)
// {
//     return 42;
// }

class Backend
{
public:
    Backend();
    ~Backend();

    void Start();
    void Submit(Request request);

private:
    bool flowFinished = false;

    void Solver();
    void Printer();

    std::mutex m_printMutex;
    void       Print(const Result& result, const std::string& prefix = "");
    void       Print(const Request& request, const std::string& prefix = "");
    void       PrintError(const std::string& error);
    
    // Запросы.
    std::condition_variable m_cvRequests;
    std::mutex              m_requestsMutex;
    std::queue<Request>     m_requests;

    // Результаты.
    std::condition_variable         m_cvResults;
    std::mutex                      m_resultsMutex;
    std::queue<std::future<Result>> m_results;

    std::thread m_solver;
    std::thread m_printer;
};

#endif  // BACKEND_H
