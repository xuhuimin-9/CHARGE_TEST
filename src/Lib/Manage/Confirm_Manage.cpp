#include "Manage/Confirm_Manage.h"  
#include "ADODatabase/DatabaseManage.h"
#include "ADODatabase/RabbitMQService.h"

Confirm_Manage::Confirm_Manage()
{
}

Confirm_Manage::~Confirm_Manage()
{

}

void Confirm_Manage::setConfirmStatusNotifier(Confirm_Status_Notifier *notifier)
{
	confirm_status_notifier_ = notifier;
}

void Confirm_Manage::Initial()
{
	WCS_CON_ = DATABASE_MANAGE.Get_WCS_DB();
	WCS_CON_->loadConfirmStationFromDB(&confirm_list);
}

std::map<std::string, Confirm> Confirm_Manage::getAllConfirmStation()
{
	if (confirm_list.empty())
	{
		Initial();
	}
	return confirm_list;
}

//************************************
// Method:     checkStationStatus
// FullName:   Confirm_Manage::checkStationStatus
// Access:     public 
// Returns:    bool
// Author:     Meixu
// Date:	   2021/08/09
// Description:检查状态更改
//************************************
bool Confirm_Manage::checkStationStatus()
{
	if (confirm_list.empty() || WCS_CON_ == nullptr)
	{
		Initial();
		return false;
	}
	bool result = false;

	std::map<std::string, Confirm> allConfirmStation;
	WCS_CON_->loadConfirmStationFromDB(&allConfirmStation);
	for each (auto var in allConfirmStation)
	{
		Confirm station = var.second;
		std::string stationName = var.first;
		if (station.Storage_Status_ != confirm_list[station.Storage_Name_].Storage_Status_)
		{// 状态被改变了
			confirm_status_notifier_->OnConfirmStatusChanged(station);
		}
	}
	confirm_list = allConfirmStation;
	return result;
}

//************************************
// Method:     getStationStatus
// FullName:   Confirm_Manage::getStationStatus
// Access:     public 
// Returns:    std::string
// Parameter:  std::string station_name
// Author:     Meixu
// Date:	   2021/08/09
// Description:获取确认点状态
//************************************
std::string Confirm_Manage::getStationStatus(std::string station_name)
{
	return confirm_list.find(station_name) != confirm_list.end() ? confirm_list[station_name].Storage_Status_ : "";
}

//************************************
// Method:     setStationStatus
// FullName:   Confirm_Manage::setStationStatus
// Access:     public 
// Returns:    bool
// Parameter:  std::string station_name
// Parameter:  std::string status
// Author:     Meixu
// Date:	   2021/08/09
// Description:更改确认点为指定状态
//************************************
bool Confirm_Manage::setStationStatus(std::string station_name, std::string status)
{
	bool result = false;
	if (confirm_list.find(station_name) != confirm_list.end())
	{
		if (confirm_list[station_name].Storage_Status_ != status)
		{
			confirm_list[station_name].Storage_Status_ = status;
			confirm_status_notifier_->OnConfirmStatusChanged(confirm_list[station_name]);
		}
		result = WCS_CON_->setCurrentStationStatus(station_name, status);
	}
	return result;
}

//************************************
// Method:     getStationStatus
// FullName:   Confirm_Manage::getStationStatus
// Access:     public 
// Returns:    bool
// Parameter:  std::string station_name
// Parameter:  std::string status
// Author:     Meixu
// Date:	   2021/08/09
// Description:确认点状态是否为指定状态
//************************************
bool Confirm_Manage::getStationStatus(std::string station_name, std::string status) {
	return getStationStatus(station_name) == status;
}

//************************************
// Method:     changeEquipmentStatus
// FullName:   Confirm_Manage::changeEquipmentStatus
// Access:     public 
// Returns:    void
// Parameter:  std::string equipment_id_
// Parameter:  std::string status
// Author:     Meixu
// Date:	   2021/08/09
// Description:更改确认点状态
//************************************
void Confirm_Manage::changeEquipmentStatus(std::string equipment_id_, std::string status)
{
	WCS_CON_->setDatabaseEquipmentStatus(equipment_id_, status);
}