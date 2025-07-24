#include "calcmainwindow.h"
#include "./ui_calcmainwindow.h"

#include <QKeyEvent>

#include "Backend/backend.h"
#include "Backend/structs.h"

struct CalcMainWindow::Impl
{
    Impl();
    ~Impl();

    void                     HandleExpression() const;
    Ui::CalcMainWindow*      ui;
    std::unique_ptr<Backend> m_pBackend;
};

CalcMainWindow::Impl::Impl()
    : ui(new Ui::CalcMainWindow)
    , m_pBackend(std::make_unique<Backend>(nullptr))
{
}

CalcMainWindow::Impl::~Impl()
{
    delete ui;
}

CalcMainWindow::CalcMainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_pImpl(std::make_unique<Impl>())
{
    m_pImpl->ui->setupUi(this);

    // Устанавливаем фильтр на всё приложение.
    qApp->installEventFilter(this);
}

CalcMainWindow::~CalcMainWindow()
{
}

bool CalcMainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Equal)
        {
            qDebug() << "=";

            return true;
        }
    }

    return QMainWindow::eventFilter(obj, event);
}
