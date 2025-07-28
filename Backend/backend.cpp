#include "backend.h"
#include "arithmetic.h"
#include "structs.h"

#include <QDebug>

#include "arithmetic.h"
#include <condition_variable>

#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <variant>

using namespace std::literals;

struct Manager
{
    Manager();
    ~Manager();

    void Initialize(std::weak_ptr<Manager> pWeakSelf, Backend* pBackend = nullptr);

    void Solver();
    void Printer();

    void Print(const Result& result, const std::string& prefix = "");
    void Print(const Request& request, const std::string& prefix = "");
    void PrintError(const std::string& error);

    std::weak_ptr<Manager> m_pWeakSelf;
    Backend*               m_pBackend;

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

void Manager::Initialize(std::weak_ptr<Manager> pWeakSelf, Backend* pBackend)
{
    m_pWeakSelf    = pWeakSelf;
    m_pBackend     = pBackend;
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

        emit m_pBackend->RequestAccepted();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        auto                pPromise     = std::make_shared<std::promise<Result>>();
        std::future<Result> futureResult = pPromise->get_future();
        {
            std::lock_guard guard(m_resultsMutex);
            m_results.push(std::move(futureResult));
        }

        emit m_pBackend->ResultPromised();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        std::weak_ptr<Manager> pWeakData = m_pWeakSelf;

        std::string reqStr = request.ToString();

        auto functor = [this, pWeakData, pPromise, req = std::move(request), solve = Solve]() mutable
        {
            std::this_thread::sleep_for(req.timeout);

            Result result;

            try
            {
                result = solve(req);
            }
            catch (const std::logic_error& e)
            {
                pPromise->set_exception(std::make_exception_ptr(std::logic_error(e.what())));
                m_cvResults.notify_one();
                return;
            }

            // В отсоединенном потоке слабый указатель позволит узнать, что данные бэкэнда вышли из области видимости.
            if (auto sharedData = pWeakData.lock())
            {
                pPromise->set_value(result);
                m_cvResults.notify_one();  // Уведомить висящие на cv потоки (Printer) о готовности задачи.
            }
            else
            {
                std::stringstream message;

                message << "[сработал weak, выражение [functor id: " << std::this_thread::get_id()
                        << " ] пропало]: " << result.ToString();
                std::string sMessage = message.str();
                std::cout << sMessage << std::endl;

                pPromise->set_exception(std::make_exception_ptr(std::runtime_error(sMessage)));
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
            Result result;
            try
            {
                result = m_results.front().get();
            }
            catch (const std::runtime_error& e)
            {
                std::cout << "runtime_error: " << e.what() << std::endl;
                m_results.pop();
                continue;
            }
            catch (const std::logic_error& e)
            {
                std::cout << "logic_error: " << e.what() << std::endl;
                if (m_pBackend)
                    emit m_pBackend->LogError(QString::fromStdString(e.what()));
                m_results.pop();
                continue;
            }
            catch (...)
            {
                m_results.pop();
                continue;
            }

            if (m_pBackend)
                emit m_pBackend->LogResult(QString::fromStdString(result.ToString()));
            Print(result, "Printer");
            m_results.pop();
        }
        else
        {
            emit m_pBackend->LogResult(QString::fromStdString("Очередь пуста"));
            PrintError("Printer: Очередь пуста"s);
        }
    }
}

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
    m_pData->Initialize(m_pData, this);  // Завести драндулет.
}

Backend::~Backend()
{
    UnloadDoIt();
}

void Backend::Submit(Request request)
{
    std::lock_guard lock(m_pData->m_requestsMutex);
    m_pData->m_requests.push(std::move(request));  // Поставить задачу в очередь.
    m_pData->Print(m_pData->m_requests.back(), "Submit"s);
    m_pData->m_cvRequests.notify_one();  // Уведомить висящие на cv потоки (Solver) о новой задаче.
}

void Backend::SwitchArith()
{
    TryLoadDoIt("../LibDoit/libdoit.so");
    SwithcImplementation();
}
