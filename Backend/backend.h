#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>

#include "structs.h"

struct Manager;

Result Solve(const Request& req);

// double DoIt(int TypeWork, double OperandA, double OperandB) noexcept(false)
// {
//     return 42;
// }

class Backend : public QObject
{
    Q_OBJECT
public:
    Backend(QObject* parent = nullptr);
    ~Backend();

    void Submit(Request request);

private:
    std::shared_ptr<Manager> m_pData;
};

#endif  // BACKEND_H
