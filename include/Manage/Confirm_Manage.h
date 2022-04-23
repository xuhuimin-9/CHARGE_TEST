#ifndef _CONFIRM_MANAGE_H_
#define _CONFIRM_MANAGE_H_
#include <string>  
#include <map> 
#include <vector>
#include <iostream>  
#ifndef Q_MOC_RUN
#include <boost/serialization/singleton.hpp>
#include <boost/thread.hpp>
#endif
#include "Manage/Manage_Define.h"
#include "ADODatabase/WCS_Database.h"
#include "ADODatabase/ACS_Database.h"
//#include "ADODatabase/XML_Database.h"

class Confirm_Status_Notifier
{
public:
	virtual void OnConfirmStatusChanged(Confirm) = 0;
};


class Confirm_Manage 
{  
public:  

	Confirm_Manage();  

	~Confirm_Manage();  

	void Initial();

	std::map<std::string, Confirm> getAllConfirmStation();

	bool checkStationStatus();

	std::string getStationStatus(std::string station_name);

	bool setStationStatus(std::string station_name, std::string status);

	bool getStationStatus(std::string station_name, std::string status);

	void setConfirmStatusNotifier(Confirm_Status_Notifier *notifier);

	void changeEquipmentStatus(std::string equipment_id_, std::string signal_type_);

protected:  


private:

	WCS_Database* WCS_CON_;
	ACS_Database* ACS_CON_;
	//XML_Database* XML_CON_;

	//接口类实例，用作更新界面显示的库位状态
	Confirm_Status_Notifier *confirm_status_notifier_;

	std::map<std::string, Confirm> confirm_list;
}; 

typedef boost::serialization::singleton<Confirm_Manage> Singleton_Confirm_Manage;
#define CONFIRM_MANAGE Singleton_Confirm_Manage::get_mutable_instance()

#endif //_CONFIRM_MANAGE_H_