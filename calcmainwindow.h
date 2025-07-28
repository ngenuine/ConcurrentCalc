#ifndef CALCMAINWINDOW_H
#define CALCMAINWINDOW_H

#include <QMainWindow>

class CalcMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    CalcMainWindow(QWidget* parent = nullptr);
    ~CalcMainWindow();

signals:
    void ClearModels();

private slots:
    void on_clear_models_triggered();
    void on_switch_arith_triggered();

private:
    struct Impl;
    std::unique_ptr<Impl> m_pImpl;
};
#endif  // CALCMAINWINDOW_H
