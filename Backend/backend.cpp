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
                toRemove.push_back(i);  // запомнили индекс '-', чтобы удалить позже
                isPrevMulOrDiv = false;
                ++i;  // пропускаем уже обработанное число
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

    void Start(Backend* pBackend = nullptr);

    void Solver();
    void Printer();

    void Print(const Result& result, const std::string& prefix = "");
    void Print(const Request& request, const std::string& prefix = "");
    void PrintError(const std::string& error);

    Backend* m_pBackend;

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

void Manager::Start(Backend* pBackend)
{
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
                std::cout << "[сработал weak, выражение "
                          << "[functor id: " << std::this_thread::get_id() << " ] пропало]: " << result.ToString()
                          << std::endl;
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
    m_pData->Start(this);  // Завести драндулет.
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
