#ifndef EXPRESSIONSTATE_H
#define EXPRESSIONSTATE_H

#include <string>
#include <QString>

// Отвечает на вопрос, возможен ли очередной символ.
// Эвристики такие:
// - первым символом может быть только число или символ '-' (isFirst) (ошибка=возможны '-' или цифра)
// - после цифры только цифра или точка (isDotAlreadyBe, ошибка=после цифры только цифра или точка)
    // - возможность постановки точки возникает только после оператора (сброс isDotAlreadyBe)
// - после точки только цифра (ошибка=после точки только цифра)
// - после пробела может идти только оператор (лишние пробелы -> ошибка=хватит пробелов)
// - после оператора может идти пробел, точка или цифра (.85 = 0.85 | 1.5 | 4) (ошибка=возможны ' ', '.' или цифра [пока без унарного минуса])
// - после / не может идти 0 (ошибка=сделаем вид, что этой попытки не было)
// - если выражение не оканчивается на цифру (isComplete), то оно считается невалидным (ошибка=выражение не сформировано)
class ExpressionState
{
public:
    ExpressionState();
    void Reset();
    bool isAllowed(char curr);
    void Stash(std::string toStash);
    bool Stashed() const;
    std::string StashPop();
    std::string What() const;
private:
    std::string m_errorText;
    std::string m_stash;
    bool m_stashed;
    bool m_isFirst;
    char m_last;
    bool m_dotSeen;

    bool isOp(char c) const
    {
        switch (c)
        {
            case '+':
            case '-':
            case '*':
            case '/':
                return true;
            default:
                return false;
        }

        return false;
    }
};

#endif // EXPRESSIONSTATE_H
