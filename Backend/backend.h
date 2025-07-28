#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>

#include <memory>

#include "structs.h"

struct Manager;

class Backend : public QObject
{
    Q_OBJECT
public:
    Backend(QObject* parent = nullptr);
    ~Backend();

    void Submit(Request request);
    void SwitchArith();

signals:
    void LogResult(const QString& solution);
    void LogError(const QString& error);
    void RequestAccepted();
    void ResultPromised();

private:
    std::shared_ptr<Manager> m_pData;
};

#endif  // BACKEND_H
