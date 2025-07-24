#include "calcmainwindow.h"
#include "./ui_calcmainwindow.h"

CalcMainWindow::CalcMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CalcMainWindow)
{
    ui->setupUi(this);
}

CalcMainWindow::~CalcMainWindow()
{
    delete ui;
}
