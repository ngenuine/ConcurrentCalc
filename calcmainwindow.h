#ifndef CALCMAINWINDOW_H
#define CALCMAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui
{
class CalcMainWindow;
}
QT_END_NAMESPACE

class CalcMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    CalcMainWindow(QWidget* parent = nullptr);
    ~CalcMainWindow();

private:
    Ui::CalcMainWindow* ui;
};
#endif  // CALCMAINWINDOW_H
