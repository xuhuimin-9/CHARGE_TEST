#ifndef SendOrderParkingLabel_H_
#define SendOrderParkingLabel_H_

#include "SendOrderLabelWidget.h"

/**
 *
 */
class SendOrderParkingLabel :public SendOrderLabelWidget
{
	Q_OBJECT
public:

	explicit SendOrderParkingLabel(QWidget *parent = 0);
	explicit SendOrderParkingLabel(const QString &text, QWidget *parent = 0);
	~SendOrderParkingLabel();

	void popMenu(const QPoint&);

	//bool SendOrderParkingLabel::set_background_color_of_label(std::string status, bool scaling = false);

signals:

public slots:

	void setBusy(bool flag);
	void setIdle(bool flag);
protected:

private:
	int id;
	std::string stationName;
	std::string isCharging;
	std::string status;
};
#endif

