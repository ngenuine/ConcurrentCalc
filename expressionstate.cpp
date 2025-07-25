#include "expressionstate.h"

#include <cctype>

ExpressionState::ExpressionState()
    : m_stashed(false)
    , m_errorText("Нет ошибок")
    , m_isFirst(true)
    , m_dotSeen(false)
{
}

void ExpressionState::Reset()
{
    m_errorText = "Нет ошибок";
    m_stash.clear();
    m_stashed = false;
    m_isFirst = false;
    m_dotSeen = false;
}

// Отвечает на вопрос, возможен ли очередной символ.
// Эвристики такие:
// + после цифры только цифра, точка или ' ' (isDotAlreadyBe, ошибка=после цифры только цифра или точка)
    // - возможность постановки точки возникает только после оператора (сброс isDotAlreadyBe)
// - после точки только цифра (ошибка=после точки только цифра)
// - после пробела может идти только оператор (лишние пробелы -> ошибка=хватит пробелов)
// + после оператора может идти пробел или цифра (ошибка=возможны ' ' или цифра [пока без унарного минуса])
// - после / не может идти 0 (ошибка=сделаем вид, что этой попытки не было)
// - если выражение не оканчивается на цифру (isComplete), то оно считается невалидным (ошибка=выражение не сформировано)
bool ExpressionState::isAllowed(char curr) // TODO: Можно Ctrl + V невалидное выражение. Надо это обработать.
{
    bool isSuccess = false;

    if (m_isFirst)
    {
        if (curr == '+' || curr == '-' || std::isdigit(curr))
        {
            isSuccess = true;
            m_last = curr;
            m_isFirst = false;
        }
        else
        {
            m_errorText = "Первым символом могут быть: '+',  '-', цифра";
        }
    }
    else
    {
        if (isOp(curr))
        {
            if (std::isdigit(m_last) || m_last == ' ')
            {
                isSuccess = true;
                m_dotSeen = false;
                m_last = curr;
            }
            else
            {
                m_errorText = "Оператор может идти после цифры или ' '";
            }
        }
        else if (std::isdigit(curr))
        {
            if (curr == '0' && m_last == '/')
            {
                m_errorText = "На ноль делить нельзя";
            }
            else if (m_last == ' ' || m_last == '.' || std::isdigit(m_last) || isOp(m_last))
            {
                isSuccess = true;
                m_last = curr;
            }
            else
            {
                m_errorText = "Цифра может идти после цифры, '.', ' ' или оператора";
            }
        }
        else if (curr == '.')
        {
            if (curr == ' ' || std::isdigit(m_last) && !m_dotSeen)
            {
                isSuccess = true;
                m_last = curr;
                m_dotSeen = true;
            }
            else
            {
                m_errorText = "После '.' может идти цифра или ' '";
            }
        }
        else if (curr == ' ')
        {
            if (std::isdigit(m_last) || isOp(m_last))
            {
                isSuccess = true;
                m_last = curr;
                m_dotSeen = true;
            }
            else if (m_last == ' ')
            {
                m_errorText = "Хватит пробелов";
            }
            else
            {
                m_errorText = "Пробел может идти после цифры или оператора";
            }
        }
        else
        {
            m_errorText = "Недопустимый символ";
        }
    }


    if (isSuccess)
        m_errorText = "Нет ошибок";
    return isSuccess;
}

void ExpressionState::Stash(std::string toStash)
{
    m_stash = std::move(toStash);
    m_stashed = true;
}

bool ExpressionState::Stashed() const
{
    return m_stashed;
}

std::string ExpressionState::StashPop()
{
    m_stashed = false;
    return m_stash;
}

std::string ExpressionState::What() const
{
    return m_errorText;
}
