#include "calcmainwindow.h"
#include "./ui_calcmainwindow.h"

#include <thread>

#include <QKeyEvent>
#include <QLineEdit>
#include <QString>
#include <QTimer>
#include <QRegularExpressionValidator>

#include "Backend/backend.h"
#include "Backend/structs.h"
#include "expressionstate.h"

struct CalcMainWindow::Impl
{
    Impl(CalcMainWindow* pMainWindow);
    ~Impl();

    Ui::CalcMainWindow*      ui;
    std::unique_ptr<Backend> m_pBackend;
    ExpressionState state;
    std::unique_ptr<QTimer> m_timer;
};

CalcMainWindow::Impl::Impl(CalcMainWindow* pMainWindow)
    : ui(new Ui::CalcMainWindow)
    , m_pBackend(std::make_unique<Backend>(nullptr))
    , m_timer(std::make_unique<QTimer>())
{
    ui->setupUi(pMainWindow);
    ui->expressionEdit->setFocus();

    m_timer->setSingleShot(true);

    QObject::connect(m_timer.get(), &QTimer::timeout, [this]
    {
        // Время истекло. Если пользователь не вводил символов, то забираем из StashPop то, что было до некорректного символа.
        if (state.Stashed())
        {
            QString prevCorrect = QString::fromStdString(state.StashPop());
            ui->expressionEdit->blockSignals(true);
            ui->expressionEdit->setText(prevCorrect);
            ui->expressionEdit->blockSignals(false);
            ui->expressionEdit->setStyleSheet("");
        }
    });

    auto expressionHandler = [this](const QString& expr)
    {
        if (expr == " ") // Подавляем ведущие пробелы.
        {
            ui->expressionEdit->clear();
            return;
        }
        else if (expr.isEmpty())
        {
            state.Reset();
        }

        qDebug() << expr;
        if (expr.endsWith('='))
        {
            QString withoutEq = expr;
            withoutEq.chop(1);
            ui->expressionEdit->setText(withoutEq);

            if (state.isAllowed('='))
            {
                std::string expression = ui->expressionEdit->text().toStdString();
                std::chrono::seconds delay{ui->delaySpin->value()};
                Request request{expression, delay};
                m_pBackend->Submit(std::move(request));

                state.Reset();
                ui->expressionEdit->clear();
            }
        }

        // Если введен ошибочный символ, инициируем показ ошибки в lineEdit выражения.
        if (!expr.isEmpty() &&
            !state.isAllowed(expr.back().toLatin1()) &&
            !state.Stashed()) // Если Stashed() то в edit сейчас показывается ошибка,
                              // которую в теле этого if инициировали ранее.
        {
            // Сохраняем все, что есть без ошибочного символа.
            QString withoutErrorSym = expr;
            withoutErrorSym.chop(1);
            state.Stash(withoutErrorSym.toStdString());

            // Информируем пользователя о том, что он ошибся.
            ui->expressionEdit->setStyleSheet("QLineEdit { background-color: #ffc9c9; }");
            ui->expressionEdit->blockSignals(true);
            ui->expressionEdit->setText(QString::fromStdString(state.What()));
            ui->expressionEdit->blockSignals(false);

            // Заводим таймер, чтобы он убрал ошибку из поля ввода, если пользователь бездействует.
            m_timer->start(25 * state.What().size());
            return;
        }

        if (state.Stashed())
        {
            m_timer->stop(); // Пользователь ввел что-то, поэтому таймер уже не нужен.

            QString additionalSymbol;
            if (QString::fromStdString(state.What()).size() + 1 == expr.size())
                additionalSymbol = expr.back(); // Во время отображения ошибки ввели символ.

            QString prevCorrect = QString::fromStdString(state.StashPop());
            ui->expressionEdit->setText(prevCorrect + additionalSymbol);
            return;
        }

        ui->expressionEdit->setStyleSheet("");
        return;
    };

    QObject::connect(ui->expressionEdit, &QLineEdit::textChanged, expressionHandler);
}

CalcMainWindow::Impl::~Impl()
{
    delete ui;
}

CalcMainWindow::CalcMainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_pImpl(std::make_unique<Impl>(this))
{
    // Устанавливаем фильтр на всё приложение.
    qApp->installEventFilter(this);
}

CalcMainWindow::~CalcMainWindow()
{
}
