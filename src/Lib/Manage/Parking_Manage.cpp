#include "Manage/Parking_Manage.h"
#include "Manage/AGV_Manage.h"
#include "Manage/Area_Manage.h"
#include "Manage/Battery_Manage.h"
#include "Core/Task_Chain_Manage.h"
#include "Manage/Order_Manage.h"
#include "Manage/Config_Manage.h"
#include "comm//simplelog.h"
#include "Core/ModelStationConfirm.h"
#include "Core/ModelStationConfirm.h"
#include "Core/charge_task.h"
#include "Core/charge_station.h"

Parking_Manage::Parking_Manage()
{
	getBatteryConfig();
}

Parking_Manage::~Parking_Manage()
{

}

void Parking_Manage::setStationStatusNotifier(Station_Status_Notifier *notifier)
{
	station_status_notifier_ = notifier;
}


void Parking_Manage::getBatteryConfig()
{
	if (Config_.FileExist("config.txt"))
	{
		Config_.ReadFile("config.txt");
	}
	if (Config_.KeyExists("Battery_High_Range"))
	{
		std::stringstream battery_high_range(Config_.Read<std::string>("Battery_High_Range"));
		battery_high_range >> battery_high_range_;
	}
	if (Config_.KeyExists("Battery_Low_Range"))
	{
		std::stringstream battery_low_range(Config_.Read<std::string>("Battery_Low_Range"));
		battery_low_range >> battery_low_range_;
	}
	if (Config_.KeyExists("Unlock_Battery_Level"))
	{
		std::stringstream unlock_battery_level(Config_.Read<std::string>("Unlock_Battery_Level"));
		unlock_battery_level >> unlock_battery_level_;
	}
}

/**
 * 初始化:加载数据库,获取站点名列表
 */
void Parking_Manage::Initial()
{
	wcs_db_ = DATABASE_MANAGE.Get_WCS_DB();
	ModelStationParkingList = loadParkingStation();
	ModelEquipmentList = loadEquipment();
	
}

std::vector< ModelStationParking*>  Parking_Manage::getAllParkingStation()//获取所有停车点名,加载界面时调用
{
	return ModelStationParkingList;
}

std::map<std::string, ModelEquipment*>  Parking_Manage::getAllEquipment()//获取所有停车确认点名,加载界面时调用
{
	return ModelEquipmentList;
}

void Parking_Manage::freeParkingList(std::map<std::string, ModelStationParking*> parkingList) {
	auto iter = parkingList.begin();
	while (iter != parkingList.end())
	{
		iter = parkingList.erase(iter);
	}
}

void Parking_Manage::freeConfirmList(std::map<std::string, ModelStationConfirm*> confirmList) {
	auto iter = confirmList.begin();
	while (iter != confirmList.end())
	{
		iter = confirmList.erase(iter);
	}
}

/**
 * 加载所有停车点
 */
std::vector< ModelStationParking*>  Parking_Manage::loadParkingStation() {
	std::vector<ModelStationParking*> allParkingStation;
	wcs_db_->getParkingStorage(&allParkingStation);
	return allParkingStation;
}

/**
 * 加载所有确认点
 */
std::map<std::string, ModelStationConfirm*>  Parking_Manage::loadConfirmStation() {
	std::map<std::string, ModelStationConfirm*>  allConfirmStation;
	
	std::vector<std::string> nameList;
	std::vector<std::string> statusList;
	wcs_db_->loadListFromDB(nameList, ModelStationConfirm_TableName_, ModelStationConfirm_NameListName_);
	wcs_db_->loadListFromDB(statusList, ModelStationConfirm_TableName_, ModelStationConfirm_StatusListName_);

	for (unsigned int i = 0; i < nameList.size() && i < statusList.size(); i++)
	{
		ModelStationConfirm *tmp = new ModelStationConfirm(nameList.at(i), statusList.at(i));
		delete allConfirmStation[tmp->getName()];
		allConfirmStation[tmp->getName()] = tmp;
	}
	return allConfirmStation;
}

/**
 * 加载所有设备
 */
std::map<std::string, ModelEquipment*>  Parking_Manage::loadEquipment() {
	std::map<std::string, ModelEquipment*>  allEquipment;

	std::vector<std::string> id_list;
	std::vector<std::string> e_status_list;
	std::vector<std::string> s_status_list;
	do {
		wcs_db_->loadListFromDB(id_list, ModelEquipment_TableName_, ModelEquipment_IdListName_);
		wcs_db_->loadListFromDB(e_status_list, ModelEquipment_TableName_, ModelEquipment_EquipStatusListName_);
		wcs_db_->loadListFromDB(s_status_list, ModelEquipment_TableName_, ModelEquipment_StationStatusListName_);
	} while (id_list.size() != e_status_list.size() || id_list.size() != s_status_list.size());

	for (unsigned int i = 0; i < id_list.size() && i < e_status_list.size() && i < s_status_list.size(); i++)
	{
		ModelEquipment *tmp = new ModelEquipment(id_list.at(i), e_status_list.at(i), s_status_list.at(i));
		delete allEquipment[tmp->getId()];
		allEquipment[tmp->getId()] = tmp;
	}
	return allEquipment;
}


void Parking_Manage::checkStationStatus()
{
	
	// 加载所有停车点信息
	std::vector<ModelStationParking*> allParkingStation = loadParkingStation();
	// 触发主窗口信号
	for (unsigned int i = 0; i < ModelStationParkingList.size(); i++)
	{// 遍历之前的停车点,因此新加入不会导致异常
		if (std::find(allParkingStation.begin(), allParkingStation.end(), allParkingStation.at(i)) != allParkingStation.end())
		{
			if (ModelStationParkingList.at(i)->getStatus() != allParkingStation.at(i)->getStatus())
			{// 状态被改变了,触发信号,更改记录的状态
				ModelStationParkingList.at(i)->setStatus(allParkingStation.at(i)->getStatus());
				station_status_notifier_->OnStationStatusChanged(ModelStationParkingList.at(i));
			}
		}
	}
	for each (auto var in allParkingStation)
	{
		delete var;
	}

	std::map<std::string, ModelEquipment*> allEquipment = loadEquipment();
	for each (auto var in allEquipment)
	{
		ModelEquipment *equipment = var.second;
		std::string equipment_name = var.first;
		if (equipment->getStatus() != ModelEquipmentList[equipment_name]->getStatus())
		{// 状态被改变了
			ModelEquipmentList[equipment_name]->setStationStatus(equipment->getStatus());
			station_status_notifier_->OnStationStatusChanged(ModelEquipmentList[equipment_name]);
		}
		delete equipment;
	}
}

void Parking_Manage::Auto_Parking_Monitor()
{
	//std::vector<AGV*> Free_AGV_List;
	//AGV_MANAGE.Get_Free_AGV(Free_AGV_List);

	//for (unsigned int i = 0; i < Free_AGV_List.size(); i++)
	//{
	//	for (unsigned int j = i+1; j < Free_AGV_List.size(); j++)
	//	{
	//		if (Free_AGV_List[j]->Battery_Level_ < Free_AGV_List[i]->Battery_Level_)
	//		{
	//			AGV* temp = Free_AGV_List[i];
	//			Free_AGV_List[i] = Free_AGV_List[j];
	//			Free_AGV_List[j] = temp;
	//		}
	//	}
	//}
	//
	//for (AGV* current_AGV : Free_AGV_List)
	//{
	//	if (/*!ORDER_MANAGE.checkOrderList(current_AGV) ||*/ BATTERY_MANAGE.Check_Low_Power(current_AGV) || current_AGV->Invoke_AGV_Parking_ == INVOKE_STATUS::BEGING_INVOKE)
	//	{
	//		Auto_Parking_System(current_AGV);
	//	}
	//}
}

bool Parking_Manage::Auto_Parking_System(AGV* current_AGV)
{
	current_AGV->Parking_Station_ = "";
	current_AGV->Parking_Is_Charging_ = "";
	std::map<std::string, ParkingStation> all_parking;
	if (wcs_db_->getAllParkingStation(all_parking)) {
		// CD7653421 AB3472165
		std::vector<std::string> need_parking;
		
		need_parking = { "P7", "P6", "P3", "P4", "P2", "P1", "P5" };

		if (!AGV_MANAGE.Is_Parking(current_AGV) && current_AGV->Is_Serving_ == 1)                  //先判断AGV是否在充电点位置
		{
			if (current_AGV->Battery_Level_ < battery_low_range_)
			{
				if (ORDER_MANAGE.getMustChargingStation(current_AGV->Parking_Station_))
				{
					for each (auto var in need_parking)
					{
						if (all_parking.find(var) != all_parking.end() && all_parking[var].Status == "IDLE" && all_parking[var].IsCharging == "TRUE")
						{
							current_AGV->Parking_Station_ = all_parking[var].StationName;
							current_AGV->Parking_Is_Charging_ = all_parking[var].IsCharging;
							break;
						}
					}
					ORDER_MANAGE.reserveParkingStation(current_AGV->Parking_Station_);
					AGV_MANAGE.Set_Lock(current_AGV->ID_);
					AGV_MANAGE.setBusy(current_AGV->ID_);
					return generateForceParkingChargingTask(current_AGV);
				}
			}
			if (ORDER_MANAGE.getParkingStation(current_AGV->Parking_Station_, current_AGV->Parking_Is_Charging_, current_AGV->AGV_Type_))
			{
				for each (auto var in need_parking)
				{
					if (all_parking.find(var) != all_parking.end() && all_parking[var].Status == "IDLE")
					{
						current_AGV->Parking_Station_ = all_parking[var].StationName;
						current_AGV->Parking_Is_Charging_ = all_parking[var].IsCharging;
						break;
					}
				}
				ORDER_MANAGE.reserveParkingStation(current_AGV->Parking_Station_);
				AGV_MANAGE.Set_Lock(current_AGV->ID_);
				AGV_MANAGE.setBusy(current_AGV->ID_);
				return Generate_Parking_Task(current_AGV);
			}
			else if (current_AGV->Parking_Is_Charging_.empty())
			{
				ORDER_MANAGE.getParkingStation(current_AGV->AGV_In_Station_, current_AGV->Parking_Is_Charging_, current_AGV->AGV_Type_);
			}
		}
		else if (AGV_MANAGE.Is_Parking(current_AGV) && ORDER_MANAGE.getStationIsCharging(current_AGV->AGV_In_Station_) && current_AGV->Battery_Level_ < unlock_battery_level_)
		{
			if (ORDER_MANAGE.getMustChargingStation(current_AGV->Parking_Station_))
			{
				for each (auto var in need_parking)
				{
					if (all_parking.find(var) != all_parking.end() && all_parking[var].Status == "IDLE" && all_parking[var].IsCharging == "TRUE")
					{
						current_AGV->Parking_Station_ = all_parking[var].StationName;
						current_AGV->Parking_Is_Charging_ = all_parking[var].IsCharging;
						break;
					}
				}
				ORDER_MANAGE.reserveParkingStation(current_AGV->Parking_Station_);
				AGV_MANAGE.Set_Lock(current_AGV->ID_);
				AGV_MANAGE.setBusy(current_AGV->ID_);
				return generateForceParkingChargingTask(current_AGV);
			}
		}
	}
}

bool Parking_Manage::Generate_Parking_Task(AGV* associate_AGV)
{
	if (TASK_CHAIN_MANAGE.Create_Parking_Task_Chain(associate_AGV))
	{
		std::stringstream ss;
		ss << "AGV:" << associate_AGV->AGV_ID_ << "  Go To Parking~~~ ";
		log_info_color(log_color::green, ss.str().c_str());
		return true;
	}
	else
	{
		std::stringstream ss;
		ss << "AGV:" << associate_AGV->AGV_ID_ << "  Parking failed~~~ ";
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

bool Parking_Manage::generateForceParkingChargingTask(AGV* associate_AGV)
{
	if (TASK_CHAIN_MANAGE.createForceParkingTaskChain(associate_AGV))
	{
		std::stringstream ss;
		ss << "AGV:" << associate_AGV->AGV_ID_ << "  Parking~~~Rush";
		log_info_color(log_color::green, ss.str().c_str());
		return true;
	}
	else
	{
		std::stringstream ss;
		ss << "AGV:" << associate_AGV->AGV_ID_ << "  Parking failed ";
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

void Parking_Manage::changeParkingStatus(std::string current_parking, std::string status)
{
	wcs_db_->setDatabaseCurrentParkingStatus(current_parking, status);
}