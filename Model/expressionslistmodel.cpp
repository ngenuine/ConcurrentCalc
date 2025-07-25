#include "expressionslistmodel.h"

#include <QColor>

ExpressionsListModel::ExpressionsListModel(QObject* parent)
    : QAbstractListModel{parent}
    , m_color(Qt::gray)
{
}

void ExpressionsListModel::AddItem(const QString& item)
{
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.push_back(item);
    endInsertRows();
}

int ExpressionsListModel::rowCount(const QModelIndex&) const
{
    return m_items.size();
}

void ExpressionsListModel::setItems(const QVector<QString>& items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}

QVariant ExpressionsListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant{};

    const QString& item = m_items.at(index.row());

    if (role == Qt::DisplayRole)
        return item;
    else if (role == Qt::ForegroundRole)
        return m_color;

    return {};
}

void ExpressionsListModel::setColor(QColor color)
{
    beginResetModel();
    m_color = color;
    endResetModel();
}
