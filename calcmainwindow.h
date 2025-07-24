#ifndef CALCMAINWINDOW_H
#define CALCMAINWINDOW_H

#include <QMainWindow>

class CalcMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    CalcMainWindow(QWidget* parent = nullptr);
    ~CalcMainWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_pImpl;
};
#endif  // CALCMAINWINDOW_H
