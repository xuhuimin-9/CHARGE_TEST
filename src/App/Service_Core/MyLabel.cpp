#include "MyLabel.h"
#include<string>
#include<Windows.h>
#include <qmessagebox.h>
#include <QPalette>
#include <QAction>
#include <QMenu>
#include "Manage/Order_Manage.h"
#include "comm/simplelog.h"

MyLabel::MyLabel(const QString &labelText, QWidget *parent) {
	click_click = false;
	Selected_ = false;
	storage_name_ = labelText.toStdString();
	setText(labelText);
	belong_column_ = "";
	this->setAlignment(Qt::AlignCenter);

	connect(this, SIGNAL(clicked()), this, SLOT(slotClicked()));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popMenu(const QPoint&)));
}

MyLabel::~MyLabel() {

}

void MyLabel::slotClicked()
{
	/*QMessageBox::information(NULL, QString::fromLocal8Bit("单击"),
	QString::fromLocal8Bit("12345上山打老虎？"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);*/
	click_click = true;
	Selected_ = true;
}


void MyLabel::mouseMoveEvent(QMouseEvent *event)
{
	/*if(resizeRegion!= Default)
	{
	handleResize();
	return;
	}
	if(m_move)
	{
	move(event->globalPos()-dragPos);
	return ;
	}

	QPoint clientCursorPos = event->pos();

	QRect r = this->rect();

	QRect resizeInnerRect(resizeBorderWidth,resizeBorderWidth,r.width()-2*resizeBorderWidth,
	r.height()-2*resizeBorderWidth);

	if(r.contains(clientCursorPos)&&!resizeInnerRect.contains(clientCursorPos))
	{
	ResizeRegion resizeReg = getResizeRegion(clientCursorPos);
	setResizeCursor(resizeReg);
	if(m_drag&&(event->buttons()&Qt::LeftButton))
	{
	resizeRegion = resizeReg;
	handleResize();
	}
	}else
	{
	setCursor(Qt::ArrowCursor);
	if(m_drag&&(event->buttons()&Qt::LeftButton))
	{
	m_move = true;
	move(event->globalPos()-dragPos);
	}
	}*/
}

void MyLabel::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::RightButton)
	{
		emit customContextMenuRequested(event->pos());
	}
	/*if(event->button() == Qt::LeftButton)
	{
	this->m_drag = true;
	this->dragPos = event->pos();
	this->resizeDownPos = event->globalPos();
	this->mouseDownRect = this->rect();
	}*/
}

void MyLabel::mouseReleaseEvent(QMouseEvent *event)
{
	emit clicked();
}

void MyLabel::handleMove(QPoint pt)
{
	
}
void MyLabel::handleResize()
{
	
}


void MyLabel::set_click(bool flag)
{
	click_click = flag;
}
bool MyLabel::get_click()
{
	return click_click;
}

bool MyLabel::set_label_select_status(bool flag)
{
	if (resource_status_ != "IDLE")
	{
		set_background_color_of_label(resource_status_, flag, false);
	}
	else {
		set_background_color_of_label(storage_status_, flag, false);
	}
	return true;
}

/**
 * 第一个判断是否选中,第二个判断是否需要缩小字体（点击时再放大字体）
 */
bool MyLabel::set_background_color_of_label( std::string status, bool selected, bool scaling)
{
	/*
	RESERVE AGV正在前往/人工预定 大红
	WAIT 相邻库位有AGV正在前往 蓝色
	GOING 有AGV预定了该库位 黄色
	IDLE 正常 无
	*/
	if (selected)
	{
		if (status.compare("EMPTY") == 0)
		{
			this->setStyleSheet("border:2px solid #000000;border-radius:10px;background-color:#FFFFFF;color:#000000");
		}
		else if (status.compare("FULL") == 0)
		{
			this->setStyleSheet("border:2px solid #000000;border-radius:10px;background-color:#FF7F50");
		}
		else if (status.compare("RESERVE") == 0)
		{
			//this->setStyleSheet("border:2px solid #000000;border-radius:10px;background-color:#00BFFF");
			this->setStyleSheet("border:2px solid #000000;border-radius:10px;background-color:#DC143C");
		}
		else if (status.compare("IDLE") == 0)
		{
			this->setStyleSheet("border:2px solid #000000;border-radius:10px;background-color:#7FFF00");
		}
		else if (status.compare("BUSY") == 0)
		{
			this->setStyleSheet("border:2px solid #000000;border-radius:10px;background-color:#DC143C");
		}
		else if (status.compare("TRAY") == 0)
		{
			this->setStyleSheet("border:2px solid #000000;border-radius:10px;background-color:#F0E68C");
		}
		else if (status.compare("APPLY_PICKUP") == 0 || status.compare("WAIT") == 0)
		{
			//this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#DC143C");
			this->setStyleSheet("border:2px solid #000000;border-radius:10px;background-color:#00BFFF");
		}
		else if (status.compare("APPLY_PUTDOWN") == 0 || status.compare("GOING") == 0)
		{
			this->setStyleSheet("border:2px solid #000000;border-radius:10px;background-color:#F0E68C");
		}
	}
	else 
	{
		if (scaling)
		{
			if (status.compare("EMPTY") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#FFFFFF;font-size:10px");
			}
			else if (status.compare("FULL") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#FF7F50;font-size:10px");
			}
			else if (status.compare("RESERVE") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#7FFF00;font-size:10px");
			}	
			else if (status.compare("IDLE") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#00BFFF;font-size:10px");
			}
			else if (status.compare("BUSY") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#FF7F50;font-size:10px");
			}
			else if (status.compare("TRAY") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#00BFFF;font-size:10px");
			}
			else if (status.compare("APPLY_PICKUP") == 0 || status.compare("WAIT") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#DC143C");
			}
			else if (status.compare("APPLY_PUTDOWN") == 0 || status.compare("GOING") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#00BFFF;font-size:10px");
			}
		}
		else
		{
			if (status.compare("EMPTY") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#FFFFFF;color:#000000");
			}
			else if (status.compare("FULL") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#FF7F50");
			}	
			else if (status.compare("RESERVE") == 0) 
			{
				//this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#00BFFF");
				// 改为大红
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#DC143C");
			}
			else if (status.compare("IDLE") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#7FFF00");
			}
			else if (status.compare("BUSY") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#DC143C");
			}
			else if (status.compare("TRAY") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#F0E68C");
			}
			else if (status.compare("APPLY_PICKUP") == 0 || status.compare("WAIT") == 0)
			{
				//this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#DC143C");
				// 改为蓝色
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#00BFFF");
			}
			else if (status.compare("APPLY_PUTDOWN") == 0 || status.compare("GOING") == 0)
			{
				this->setStyleSheet("border:2px solid #FFFFFF;border-radius:10px;background-color:#F0E68C");
			}
		}
	}
		
	return true;
}

bool MyLabel::set_storage_label_name(std::string name)
{
	storage_name_ = name;
	return true;
}
std::string MyLabel::get_storage_label_name()
{
	return storage_name_;
}
bool MyLabel::set_storage_label_status(std::string status)
{
	storage_status_ = status;
	return true;
}
std::string MyLabel::get_storage_label_status()
{
	return storage_status_;
}

bool MyLabel::set_storage_label_belong_column(std::string belong_column)
{
	belong_column_ = belong_column;
	return true;
}
std::string MyLabel::get_storage_label_belong_column()
{
	return belong_column_;
}

bool MyLabel::set_storage_type(std::string storage_type)
{
	storage_type_ = storage_type;
	return true;
}
std::string MyLabel::get_storage_type()
{
	return storage_type_;
}

bool MyLabel::set_storage_resource_status(std::string resource_status)
{
	resource_status_ = resource_status;
	return true;
}
std::string MyLabel::get_storage_resource_status()
{
	return resource_status_;
}

void MyLabel::setnumber(int aa)
{
	number_ = aa;
}

void MyLabel::popMenu(const QPoint&)
{

	QAction FullAction(tr("Change To Full"), this);   // 库位状态为满;
	QAction EmptyAction(tr("Change To Empty"), this);    // 库位为空;
	QAction reserve_action(tr("Change To Reserve"), this); // 人工预定库位
	QAction idle_action(tr("Change To Idle"), this); // 库位内为托盘
	QAction ChangeTo220(tr("Change To 220"), this);
	QAction ChangeTo236(tr("Change To 236"), this);
	QAction ChangeTo255(tr("Change To 255"), this);
	connect(&FullAction, SIGNAL(triggered(bool)), this, SLOT(sendChangeFullSignal(bool)));
	connect(&EmptyAction, SIGNAL(triggered(bool)), this, SLOT(sendChangeEmptySignal(bool)));
	connect(&idle_action, SIGNAL(triggered(bool)), this, SLOT(sendChangeIdleSignal(bool)));
	connect(&reserve_action, SIGNAL(triggered(bool)), this, SLOT(sendChangeReserveSignal(bool)));
	connect(&ChangeTo220, SIGNAL(triggered(bool)), this, SLOT(sendChangeTo220(bool)));
	connect(&ChangeTo236, SIGNAL(triggered(bool)), this, SLOT(sendChangeTo236(bool)));
	connect(&ChangeTo255, SIGNAL(triggered(bool)), this, SLOT(sendChangeTo255(bool)));
	//QPoint pos;
	QMenu menu(this);

	if (storage_name_.find("F9WC-") != std::string::npos)
	{
		// 是产线的,需要改变产线状态,改变工字轮型号
		EmptyAction.setText("Close");
		FullAction.setText("Open");
		menu.addAction(&EmptyAction);
		menu.addAction(&FullAction);
		menu.addAction(&ChangeTo220);
		menu.addAction(&ChangeTo236);
		menu.addAction(&ChangeTo255);
	}
	else
	{
		menu.addAction(&EmptyAction);
		menu.addAction(&FullAction);
		menu.addAction(&idle_action);
		menu.addAction(&reserve_action);
	}

	menu.exec(QCursor::pos());
}

void MyLabel::sendChangeEmptySignal(bool flag)
{
	//this->storage_status_ = "EMPTY";
	// 智昌:关闭产线,原有的NEW工单改为FORBID
	log_info(storage_name_ + ": click to EMPTY");
	if (!storage_name_.empty() && storage_name_[0] == 'F')
	{
		ORDER_MANAGE.updateOrderStatus("FORBID", "NEW", storage_name_);
	}
	emit changeStorageSignal(storage_name_, "EMPTY");
}

void MyLabel::sendChangeTraySignal(bool flag)
{
	log_info(storage_name_ + ": click to TRAY");
	emit changeStorageSignal(storage_name_, "TRAY");
}

void MyLabel::sendChangeTo220(bool flag) {
	log_info(storage_name_ + ": click to 220");
	emit sendChangeTrayType(storage_name_, "220");
}

void MyLabel::sendChangeTo236(bool flag) {
	log_info(storage_name_ + ": click to 236");
	emit sendChangeTrayType(storage_name_, "236");
}

void MyLabel::sendChangeTo255(bool flag) {
	log_info(storage_name_ + ": click to 255");
	emit sendChangeTrayType(storage_name_, "255");
}

void MyLabel::sendChangeIdleSignal(bool flag)
{
	log_info(storage_name_ + ": click to IDLE");
	emit changeStorageSignal(storage_name_, "IDLE");
}

void MyLabel::sendChangeReserveSignal(bool flag) {
	log_info(storage_name_ + ": click to RESERVE");
	emit changeStorageSignal(storage_name_, "RESERVE");
}

void MyLabel::sendChangeBusySignal(bool flag)
{
	emit changeStorageBusySignal(number_);
}

void MyLabel::sendChangeFullSignal(bool flag)
{
	//this->storage_status_ = "FULL";
	// 智昌:打开产线,原有的FORBID工单改为NEW
	log_info(storage_name_ + ": click to FULL");
	if (!storage_name_.empty() && storage_name_[0] == 'F')
	{
		ORDER_MANAGE.updateOrderStatus("NEW", "FORBID", storage_name_);
	}
	emit changeStorageSignal(storage_name_, "FULL");
}
void MyLabel::sendselectedAsStartPointSignal(bool flag)
{
	emit selectedAsStartPointSignal(storage_name_);
}
void MyLabel::sendselectedAsTargetPointSignal(bool flag)
{
	emit selectedAsTargetPointSignal(storage_name_);
}