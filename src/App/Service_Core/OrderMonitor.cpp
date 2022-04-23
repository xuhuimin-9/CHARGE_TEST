#include <QtGui>
#include <QMainWindow>
#include <QStringList>
#include <QVector>
#include <QTimer>
#include <QDebug>
#include "iostream"
#include <vector>
#include <QHeaderView>
#include <QAction>
#include <QPoint>
#include <QMenu>

#include <QApplication> 
#include "iostream"

#include "progressbardelegate.h"
#include "OrderMonitor.h"
#include "DataModel.h"

#include "Manage/Order_Manage.h"
#include "ADODatabase/RabbitMQService.h"
#include <map>
#include <string>
#include <QDesktopWidget>
#include <qtextcodec.h> 



Order_Monitor::Order_Monitor(QWidget *parent) :QTableView(parent)
{
	int screenWidth = QApplication::desktop()->screenGeometry().width();
	int screenHeight = QApplication::desktop()->screenGeometry().height();
	float factorx = screenWidth/1000;
	float factory = screenHeight/760;

	this->setAlternatingRowColors(true);
	this->setEditTriggers(QAbstractItemView::NoEditTriggers);
	this->setSelectionMode(QAbstractItemView::ExtendedSelection);
	this->setObjectName("Order_Monitor");

	QHeaderView* hHeader = this->horizontalHeader();
	hHeader->setObjectName("hHeader");

	QHeaderView* vHeader = this->verticalHeader();
	vHeader->setObjectName("vHeader");

	this->horizontalHeader()->setStretchLastSection(true);
	//this->horizontalHeader()->setBaseSize(30,20);
	this->verticalHeader()->hide();

	iniData();
	initTimer();
	this->setFont(QFont("Times", 8, QFont::Black));

	this->setSelectionBehavior(QAbstractItemView::SelectRows);
	this->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popMenu(const QPoint&)));

	codec = QTextCodec::codecForName("GBK");
//	Database_Reader_=DATABASE_MANAGE.Get_DatabaseReader();
}

Order_Monitor::~Order_Monitor()
{
	delete m_model;
}

void Order_Monitor::popMenu(const QPoint&)
{
	QModelIndexList list = this->selectedIndexes();
	if (list.count() > 1)
	{
		QAction redoOrder(tr("Redo"), this);
		QAction reportFinish(tr("Report Success"), this);

		connect(&redoOrder, SIGNAL(triggered()), this, SLOT(RedoOrder()));
		connect(&reportFinish, SIGNAL(triggered()), this, SLOT(ReportSuccessOrder()));

		QPoint pos;
		QMenu menu(this);
		menu.addAction(&redoOrder);
		menu.addAction(&reportFinish);
		menu.exec(QCursor::pos());
	}
}

void Order_Monitor::RedoOrder()
{
	QModelIndexList list = this->selectedIndexes();

	if (list.count() > 1)
	{
		std::string order_id = list[0].data().toString().toStdString();
		ORDER_MANAGE.RedoErrOrder(order_id);
	}
}

void Order_Monitor::ReportSuccessOrder()
{
	QModelIndexList list = this->selectedIndexes();

	if (list.count() > 1)
	{
		RABBITMQ_SERVICE.ReportOrderStatus(list[0].data().toString().toStdString(), 1, list[5].data().toInt(), " ", list[2].data().toString().toStdString());
	}
}

void Order_Monitor::iniData()
{
	//set data model
	m_model = new SpecialDataModel(this);
	// 这里必须先绑定数据模型,再设置表头,否则导致字体设置无效
	QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
	proxyModel->setSourceModel(m_model);
	this->setModel(proxyModel);   //绑定数据模型
	
	//this->setModel(m_model);

	//set table header
	QVariantList headers;
	headers << tr("ID") << tr("Equip") << tr("Storage") << tr("Priority") << tr("Status") << tr("AGV ID") << tr("Mode") << tr("Type") << tr("Enter Date") << tr("Begin Date") << tr("Finish Date") << tr("Msg");
	m_model->setHorizontalHeader(headers);

	this->setSortingEnabled(true);

	//this->sortByColumn(0,Qt::AscendingOrder);
      
	/*m_progressBarDelegate = new ProgressBarDelegate(this);
	this->setItemDelegate(m_progressBarDelegate);*/

}

void Order_Monitor::initTimer()
{
	timer = new QTimer(this);
	timer->setInterval(1000);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateProgressValue()));

	timer->start();
}

void Order_Monitor::translatorUI()
{
	QVariantList headers;
	headers << tr("ID") << tr("Equip") << tr("Storage") << tr("Priority") << tr("Status") << tr("AGV ID") << tr("Mode") << tr("Type") << tr("Enter Date") << tr("Begin Date") << tr("Finish Date") << tr("Msg");
	m_model->setHorizontalHeader(headers);


}

void Order_Monitor::updateProgressValue()
{
	QVector<QVariantList> data= m_model->DataVector();
	data.clear();

	std::map<std::string, Order> order_list=ORDER_MANAGE.Get_All_Order();

	std::map<std::string, Order>::iterator it=order_list.begin();

	for( ; it!=order_list.end(); ++it)
	{  
		Order current_order=it->second;
		QVariantList rowset;
		rowset.push_back(QString::fromStdString(current_order.ORDER_ID_));
		rowset.push_back(QString::fromStdString(current_order.START_));
		rowset.push_back(QString::fromStdString(current_order.TARGETED_));
		rowset.push_back(QString::number(current_order.PRIORITY_));
		rowset.push_back(QString::fromStdString(current_order.STATUS_));	
		rowset.push_back(QString::number(current_order.AGV_ID_)); 
		rowset.push_back(QString::fromStdString(current_order.MODE_));
		rowset.push_back(QString::fromStdString(current_order.TYPE_));
		rowset.push_back(QDateTime::fromString(current_order.ENTERDATE_.c_str(), "yyyy/MM/dd hh:mm:ss"));
		rowset.push_back(QDateTime::fromString(current_order.STARTDATE_.c_str(), "yyyy/MM/dd hh:mm:ss"));
		rowset.push_back(QDateTime::fromString(current_order.FINISHDATE_.c_str(), "yyyy/MM/dd hh:mm:ss"));

		//w.setWindowTitle(codec->toUnicode("中文按钮"));
		rowset.push_back(codec->toUnicode(current_order.Msg_.c_str()));
		data.append(rowset);
	}
	m_model->setData(data);
}

void Order_Monitor::changeEvent(QEvent *event)
{
	switch (event->type())
	{
	case QEvent::LanguageChange:
	{
		translatorUI();
	}
	break;
	default:
		QTableView::changeEvent(event);
		break;
	}
}
