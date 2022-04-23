
#include "DataModel.h"
#include <QAbstractTableModel>
#include <QBrush>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <sstream>
#include "iostream"
#include "comdef.h"
DataModel::DataModel(QObject *parent) :QAbstractTableModel(parent)
{

}

DataModel::~DataModel()
{

}


int DataModel::rowCount(const QModelIndex &parent) const
{
	return m_data.size();
}

int DataModel::columnCount(const QModelIndex &parent) const
{
	return m_HorizontalHeader.count();
}

QVariant DataModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role == Qt::TextAlignmentRole)
	{
		return int(Qt::AlignHCenter | Qt::AlignVCenter);
	}
	if (role == Qt::DisplayRole) {
		int ncol = index.column();
		int nrow = index.row();
		if (nrow < m_data.size())
		{
			QVariantList values = m_data.at(nrow);

			if (values.size() > ncol)
				return values.at(ncol);
			else
				return QVariant();
		}
	}
	if (role == Qt::BackgroundRole)
	{

	}
	return QVariant();
}

Qt::ItemFlags DataModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	Qt::ItemFlags flag = QAbstractItemModel::flags(index);

	// flag|=Qt::ItemIsEditable // 设置单元格可编辑,此处注释,单元格无法被编辑
	return flag;
}

void DataModel::setHorizontalHeader(const QVariantList &headers)
{
	m_HorizontalHeader = headers;
}


QVariant DataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		return m_HorizontalHeader.at(section);
	}
	return QAbstractTableModel::headerData(section, orientation, role);
}

void DataModel::setData(const QVector<QVariantList> &data)
{
	try
	{
		m_data = data;
		emit layoutAboutToBeChanged();
		emit layoutChanged();
	}
	catch (_com_error &e) {

		std::stringstream ss;
		ss << e.ErrorMessage() << "setData" << e.Description();
	}
}

QVector<QVariantList> DataModel::DataVector()
{
	return m_data;

}

SpecialDataModel::SpecialDataModel(QObject *parent) :DataModel(parent)
{

}

SpecialDataModel::~SpecialDataModel()
{

}

QVariant SpecialDataModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role == Qt::TextAlignmentRole)
	{
		return int(Qt::AlignHCenter | Qt::AlignVCenter);
	}
	if (role == Qt::DisplayRole) {
		int ncol = index.column();
		int nrow = index.row();
		if (nrow < m_data.size())
		{
			QVariantList values = m_data.at(nrow);

			if (values.size() > ncol)
				return values.at(ncol);
			else
				return QVariant();
		}
	}
	if (role == Qt::BackgroundRole)
	{
		
		if (index.column() == 4 && m_data.at(index.row()).at(4).toString() == "ERR")
		{
			return QColor(Qt::red);
		}
		else if (index.column() == 4 && m_data.at(index.row()).at(4).toString() == "DOING")
		{
			return QColor(Qt::yellow);
		}
		else if (index.column() == 4 && (m_data.at(index.row()).at(4).toString() == "DONE" || m_data.at(index.row()).at(4).toString() == "PUT"))
		{
			return QColor(Qt::green);
		}
		else if (index.column() == 4 && m_data.at(index.row()).at(4).toString() == "NEW")
		{
			return QColor(Qt::cyan);
		}
	}
	return QVariant();
}

void SpecialDataModel::setDataColor(Qt::GlobalColor data)
{
	QCdata_ = data;
}
