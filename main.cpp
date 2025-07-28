#include "calcmainwindow.h"

#include "Backend/arithmetic.h"
#include "Backend/backend.h"
#include "Backend/structs.h"

#include <iostream>
#include <thread>

#include <QApplication>

using namespace std::literals;
using namespace std::chrono_literals;

void TestRequest();
void TestBackend();

int main(int argc, char* argv[])
{
    QApplication   a(argc, argv);
    CalcMainWindow w;

    // TestRequest();
    // TestBackend();

    w.show();

    return a.exec();
}

void TestRequest()
{
    std::cout << "\n=== TestRequest ===\n"s;
    std::vector<Request> requests;

    requests.emplace_back("-12 - 3.5 * 4 + 6 / 2"s, 1s);  // -28
    requests.emplace_back("12 + 3.5 * 4 - 6 / 2"s, 1s);   // 28
    requests.emplace_back("7.25 * 8 + 9 / 3 - 1"s, 2s);   // 21.33
    requests.emplace_back("100 / 5 + 20.0 - 6.75"s, 1s);  // 33.25
    requests.emplace_back("5 + 10 * 2 - 8 / 4"s, 2s);     // 5.50
    requests.emplace_back("3.14 * 2 + 1.86 / 2"s, 2s);    // 4.07
    requests.emplace_back("50 - 25 + 10 * 3"s, 2s);       // 105.00
    requests.emplace_back("9 / 3 + 6 * 2 - 4"s, 1s);      // 14.00
    requests.emplace_back("7 * 7 - 20 / 5 + 3"s, 1s);     // 8.80

    for (const auto& request : requests)
        request.Print();

    std::cout << std::endl;

    for (const auto& request : requests)
        std::cout << Solve(request).ToString() << std::endl;
    std::cout << std::endl;
}

void TestBackend()
{
    std::cout << "\n=== TestBackend ===\n"s;

    Backend backend;

    backend.Submit({"12 + 3.5 * 4 - 6 / 2"s, 1s});
    std::this_thread::sleep_for(1s);

    backend.Submit({"7.25 * 8 + 9 / 3 - 1"s, 2s});
    std::this_thread::sleep_for(2s);

    backend.Submit({"100 / 5 + 20.0 - 6.75"s, 1s});
    backend.Submit({"5 + 10 * 2 - 8 / 4"s, 2s});
    backend.Submit({"3.14 * 2 + 1.86 / 2"s, 2s});
    std::this_thread::sleep_for(1s);

    backend.Submit({"50 - 25 + 10 * 3"s, 2s});
    std::this_thread::sleep_for(2s);

    backend.Submit({"9 / 3 + 6 * 2 - 4"s, 1s});
    // backend.Submit({"7 * 7 - 20 / 5 + 3"s, 1s});
    std::this_thread::sleep_for(1s);

    // FYI:
    // В вывод приложения придет инфа, что последние 2 выражения пропали, если не дождаться.
    // std::this_thread::sleep_for(3s);

    std::cout << std::endl;
}
