#ifndef EXPRESSIONSLISTMODEL_H
#define EXPRESSIONSLISTMODEL_H

#include <QAbstractListModel>
#include <QColor>
#include <QObject>

class ExpressionsListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ExpressionsListModel(QObject* parent = nullptr);

    void     AddItem(const QString& item);
    int      rowCount(const QModelIndex&) const override;
    void     setItems(const QVector<QString>& items);
    QVariant data(const QModelIndex& index, int role) const override;
    void     setColor(QColor color);
    void     Clear();

private:
    QVector<QString> m_items;
    QColor           m_color;
};

#endif  // EXPRESSIONSLISTMODEL_H
