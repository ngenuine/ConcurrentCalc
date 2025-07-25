#include "backend.h"
#include "structs.h"

#include <QDebug>

#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <variant>

using namespace std::literals;

Result Solve(const Request& req)
{
    double result    = 0.0;
    char   currentOp = '+';

    for (const auto& entity : req.toEval)
    {
        if (std::holds_alternative<char>(entity))
        {
            currentOp = std::get<char>(entity);
        }
        else
        {
            double value = std::get<double>(entity);
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

struct Manager : public std::enable_shared_from_this<Manager>
{
    Manager();
    ~Manager();

    void Start();

    void Solver();
    void Printer();

    void Print(const Result& result, const std::string& prefix = "");
    void Print(const Request& request, const std::string& prefix = "");
    void PrintError(const std::string& error);

    bool m_flowFinished;

    std::mutex m_printMutex;

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

Manager::Manager()
    : m_flowFinished(false)
{
}

Manager::~Manager()
{
    m_flowFinished = true;

    // Уведомить висящие на cv потоки о завершении работы.
    m_cvRequests.notify_all();
    m_cvResults.notify_all();

    if (m_solver.joinable())
        m_solver.join();
    if (m_printer.joinable())
        m_printer.join();
}

void Manager::Start()
{
    m_flowFinished = false;
    m_solver       = std::thread(&Manager::Solver, this);
    m_printer      = std::thread(&Manager::Printer, this);
}

void Manager::Solver()
{
    while (!m_flowFinished)
    {
        Request request;

        {
            std::unique_lock lock(m_requestsMutex);
            m_cvRequests.wait(lock, [this] { return !m_requests.empty() || m_flowFinished; });

            if (m_flowFinished)
                break;  // Пришло уведомление из деструктора.

            // Пришло уведомлние о новой задаче.
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

        auto                pPromise     = std::make_shared<std::promise<Result>>();
        std::future<Result> futureResult = pPromise->get_future();
        {
            std::lock_guard guard(m_resultsMutex);
            m_results.push(std::move(futureResult));
        }

        std::weak_ptr<Manager> pWeakData = shared_from_this();

        std::string reqStr = request.ToString();

        auto functor = [this, pWeakData, pPromise, req = std::move(request), solve = Solve]() mutable
        {
            std::this_thread::sleep_for(req.timeout);
            Result result = solve(req);

            // Мы в отсоединенном потоке. Слабый указатель это способ узнать, есть ли нам кого уведомлять.
            if (auto sharedData = pWeakData.lock())
            {
                pPromise->set_value(result);
                m_cvResults.notify_one();  // Уведомить висящие на cv потоки (Printer) о готовности задачи.
            }
            else
            {
                std::cout << "[сработал weak, выражение " << "[functor id: " << std::this_thread::get_id() << " ] пропало]: " << result.ToString() << std::endl;
            }
        };
        std::thread t(std::move(functor));
        {
            std::lock_guard guard(m_printMutex);
            std::cout << "начало вычислений [functor id: " << t.get_id() << "]: " << reqStr << std::endl;
        }
        t.detach();
    }
}

void Manager::Printer()
{
    while (!m_flowFinished)
    {
        std::unique_lock lock(m_resultsMutex);
        m_cvResults.wait(lock, [this] { return !m_results.empty() || m_flowFinished; });
        if (m_flowFinished)
            break;  // Пришло уведомление из деструктора.

        // Пришло уведомление о готовности задачи.
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

// TODO: Разные места печати запросов и результатов. По идее мьютексы можно назные.
void Manager::Print(const Result& result, const std::string& prefix)
{
    std::lock_guard lock(m_printMutex);

    std::string sep = prefix.empty() ? ""s : ": "s;
    std::cout << "\033[32m" << prefix << sep << result.ToString() << "\033[0m" << std::endl;
}

void Manager::Print(const Request& request, const std::string& prefix)
{
    std::lock_guard lock(m_printMutex);
    std::string     sep = prefix.empty() ? ""s : ": "s;
    std::cout << "\033[33m" << prefix << sep << request.ToString() << "\033[0m" << std::endl;
}

void Manager::PrintError(const std::string& error)
{
    std::lock_guard lock(m_printMutex);

    std::cout << "\033[31m" << error << "\033[0m" << std::endl;
}

Backend::Backend(QObject* parent)
    : QObject(parent)
    , m_pData(std::make_shared<Manager>())
{
    m_pData->Start();  // Завести драндулет.
}

Backend::~Backend()
{
}

void Backend::Submit(Request request)
{
    std::lock_guard lock(m_pData->m_requestsMutex);
    m_pData->m_requests.push(std::move(request));  // Поставить задачу в очередь.
    m_pData->Print(m_pData->m_requests.back(), "Submit"s);
    m_pData->m_cvRequests.notify_one();  // Уведомить висящие на cv потоки (Solver) о новой задаче.
}
