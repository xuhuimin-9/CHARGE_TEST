#include "SendOrderParkingLabel.h"
#include <QMenu>
#include <QAction>
#include <QPoint>

SendOrderParkingLabel::SendOrderParkingLabel(const QString &text1, QWidget *parent) :SendOrderLabelWidget::SendOrderLabelWidget(text1, parent) {

}

SendOrderParkingLabel::~SendOrderParkingLabel() {

}

void SendOrderParkingLabel::popMenu(const QPoint&)
{
	QAction set_idle("Change To IDLE", this);
	QAction set_busy("Change To BUSY", this);
	connect(&set_idle, SIGNAL(triggered(bool)), this, SLOT(setIdle(bool)));
	connect(&set_busy, SIGNAL(triggered(bool)), this, SLOT(setBusy(bool)));

	QPoint pos;
	QMenu menu(this);
	menu.addAction(&set_idle);
	menu.addAction(&set_busy);
	menu.exec(QCursor::pos());
}

void SendOrderParkingLabel::setBusy(bool flag)
{
	emit changeStorageSignal(this->storage_name_, "BUSY");
}

void SendOrderParkingLabel::setIdle(bool flag)
{
	emit changeStorageSignal(this->storage_name_, "IDLE");
}