#include "calcmainwindow.h"
#include "./ui_calcmainwindow.h"
#include "Model/expressionslistmodel.h"

#include <QKeyEvent>
#include <QLineEdit>
#include <QString>
#include <QTimer>

#include "Backend/backend.h"
#include "Backend/structs.h"

struct CalcMainWindow::Impl
{
    Ui::CalcMainWindow*   ui;
    ExpressionsListModel* m_pRequestsModel;
    ExpressionsListModel* m_pResultsModel;

    std::unique_ptr<Backend> m_pBackend;
    std::unique_ptr<QTimer>  m_timer;

    QString m_cacheExpression;
    QString m_lastError;

    Impl(CalcMainWindow* pMainWindow);
    ~Impl();
    QString validateExpression(const QString& expr);
    bool    isOperator(QChar c);
};

CalcMainWindow::Impl::Impl(CalcMainWindow* pMainWindow)
    : ui(new Ui::CalcMainWindow)
    , m_pBackend(std::make_unique<Backend>(nullptr))
    , m_timer(std::make_unique<QTimer>())
{
    ui->setupUi(pMainWindow);
    ui->expressionEdit->setFocus();

    m_pRequestsModel = new ExpressionsListModel(pMainWindow);
    m_pRequestsModel->setColor(Qt::darkGreen);

    m_pResultsModel = new ExpressionsListModel(pMainWindow);
    m_pResultsModel->setColor(QColor("#007acc"));

    ui->inputView->setModel(m_pRequestsModel);
    ui->outputView->setModel(m_pResultsModel);

    QObject::connect(m_pBackend.get(), &Backend::LogResult,
                     [this](const QString& msg) { m_pResultsModel->AddItem(msg); });

    m_timer->setSingleShot(true);

    QObject::connect(m_timer.get(), &QTimer::timeout,
                     [this]
                     {
                         if (!m_lastError.isEmpty())
                         {
                             ui->expressionEdit->blockSignals(true);
                             ui->expressionEdit->setText(m_cacheExpression);
                             ui->expressionEdit->blockSignals(false);
                             ui->expressionEdit->setStyleSheet("");

                             m_cacheExpression.clear();
                             m_lastError.clear();
                         }
                     });

    auto expressionHandler = [this](const QString& expr)
    {
        if (expr.endsWith('='))
        {
            QString withoutEq = expr;
            withoutEq.chop(1);
            ui->expressionEdit->blockSignals(true);
            ui->expressionEdit->setText(withoutEq);
            ui->expressionEdit->blockSignals(false);

            if (!m_lastError.isEmpty())
                return;

            QString error = validateExpression(withoutEq);
            if (error.isEmpty())
            {
                std::string          expression = withoutEq.toStdString();
                std::chrono::seconds delay{ui->delaySpin->value()};
                Request              request{expression, delay};

                m_pRequestsModel->AddItem(QString::fromStdString(request.ToString()));
                m_pBackend->Submit(std::move(request));
                ui->expressionEdit->clear();
            }
            else
            {
                m_cacheExpression = withoutEq;
                m_lastError       = error;

                // Информируем пользователя о том, что он ошибся.
                ui->expressionEdit->setStyleSheet("QLineEdit { background-color: #ffc9c9; }");
                ui->expressionEdit->blockSignals(true);
                ui->expressionEdit->setText(m_lastError);
                ui->expressionEdit->blockSignals(false);

                // Заводим таймер, чтобы он убрал ошибку из поля ввода, если пользователь бездействует.
                m_timer->start(700);
            }

            return;
        }

        if (!m_lastError.isEmpty())
        {
            m_timer->stop();  // Пользователь ввел что-то, поэтому таймер уже не нужен.

            QChar additionalSymbol;
            if (m_lastError.size() + 1 == expr.size())
                additionalSymbol = expr.back();  // Во время отображения ошибки ввели символ.

            QString newText = m_cacheExpression + additionalSymbol;

            m_lastError.clear();
            m_cacheExpression.clear();

            ui->expressionEdit->setText(newText);
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

QString CalcMainWindow::Impl::validateExpression(const QString& expr)
{
    bool lastWasOperator = false;
    bool numberHasDot    = false;
    bool numberHasDigit  = false;

    int pos = 0;
    int len = expr.size();

    if (expr.trimmed().isEmpty())
    {
        return "Выражение пустое";
    }

    while (pos < len)
    {
        QChar c = expr[pos];

        if (c.isSpace())
        {
            ++pos;
            continue;
        }

        if (c.isDigit())
        {
            lastWasOperator = false;
            numberHasDigit  = true;
            ++pos;
            continue;
        }

        if (c == '.')
        {
            if (numberHasDot)
            {
                return "Ошибка: число содержит более одной точки";
            }
            if (!numberHasDigit)
            {
                return "Ошибка: точка должна следовать после цифры";
            }
            numberHasDot = true;
            ++pos;
            continue;
        }

        if (isOperator(c))
        {
            if (lastWasOperator)
            {
                // Разрешаем унарный минус в начале или после другого оператора.
                if (!(c == '-' && (pos == 0 || isOperator(expr[pos - 1]) || expr[pos - 1].isSpace())))
                {
                    return "Ошибка: два оператора подряд или некорректное использование оператора";
                }
            }

            if (pos == len - 1)
            {
                return "Ошибка: выражение не может заканчиваться оператором";
            }

            lastWasOperator = true;
            numberHasDot    = false;
            numberHasDigit  = false;
            ++pos;
            continue;
        }

        return QString("Ошибка: недопустимый символ '%1'").arg(c);
    }

    if (lastWasOperator)
    {
        return "Ошибка: выражение не может заканчиваться оператором";
    }

    return {};  // Пустая строка это валидное выражение.
}

bool CalcMainWindow::Impl::isOperator(QChar c)
{
    switch (c.toLatin1())
    {
    case '+':
    case '-':
    case '*':
    case '/':
        return true;
    default:
        return false;
    }
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
