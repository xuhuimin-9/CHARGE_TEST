#include "Manage/AGV_Manage.h"  
#include "Manage/Parking_Manage.h"
#include "Core/Task_Chain_Manage.h"
#include "ADODatabase/DatabaseManage.h"
#include "iostream"
#include <sstream>
#include <fstream>
#include <vector>
#include "comm/simplelog.h"
#include "ADODatabase/ApiClient.h"
#include "Manage/Area_Manage.h"
#include "Manage/Config_Manage.h"
#include "Core/charge_task.h"

AGV_Manage::AGV_Manage()
{
	getBatteryConfig();
}

AGV_Manage::~AGV_Manage()
{

}

void AGV_Manage::Initial()
{
	ACS_CON_ = DATABASE_MANAGE.Get_ACS_DB();
	WCS_CON_ = DATABASE_MANAGE.Get_WCS_DB();
}

void AGV_Manage::Update_AGV_Data_From_ACS_Event()
{
	std::vector<AGV*> agv_list;
	ACS_CON_->Download_AGV_Info(agv_list);

	for(std::vector<AGV*>::iterator it=agv_list.begin(); it!=agv_list.end(); ++it)
	{
		AGV* current_agv=*it;
		//for each (auto var in all_area_)
		//{
		//	if (isInArea(current_agv, var.first))
		//	{
		//		log_info((var.first));
		//	}
		//}
		std::map<int, AGV*>::iterator it2=AGV_List_.find(current_agv->ID_);
		if(it2!=AGV_List_.end())
		{
			AGV* found_agv=it2->second;
			
			found_agv->AGV_ID_=current_agv->AGV_ID_;
			found_agv->AGV_Status_=current_agv->AGV_Status_;
			found_agv->Error_Code_=current_agv->Error_Code_;
			found_agv->AGV_Type_=current_agv->AGV_Type_;
			found_agv->AGV_Pos_=current_agv->AGV_Pos_;
			found_agv->AGV_In_Station_ = current_agv->AGV_In_Station_;
			found_agv->Parking_Point = current_agv->Parking_Point;
			found_agv->Is_Assign_=current_agv->Is_Assign_;
			found_agv->Is_Online_=current_agv->Is_Online_;
			found_agv->Battery_Level_=current_agv->Battery_Level_;   
			found_agv->Is_Serving_=current_agv->Is_Serving_;
			found_agv->AGV_at_node = current_agv->AGV_at_node;
			found_agv->Oreder_Status_ = RESOURCE_STATUS::RESOURCE_IDLE;
			if (found_agv->AGV_In_Station_.size() == 2 && found_agv->AGV_In_Station_[0] == 'P')
			{
				if (found_agv->Is_Online_ == 1 && found_agv->Resource_Status_ != RESOURCE_STATUS::LOCKED)
				{
					PARKING_MANAGE.changeParkingStatus(found_agv->AGV_In_Station_, "BUSY");
				}
			}
			
			//log_info("AGV:" + std::to_string(current_agv->AGV_ID_));
			//for each (auto var in AREA_MANAGE.areasInfoOnAgv(current_agv->AGV_ID_))
			//{
			//	log_info(var.first);
			//}
			delete(current_agv);
		}
		else{
			current_agv->Resource_Status_=RESOURCE_STATUS::UNLOCKED;
			current_agv->Is_Charging_ = CHARGING_STATUS::CHANRING_STOP;
			current_agv->Invoke_AGV_Parking_ = INVOKE_STATUS::STOP_INVOKE;
			AGV_List_[current_agv->ID_]=current_agv;
			if (current_agv->AGV_In_Station_.size() == 2 && current_agv->AGV_In_Station_[0] == 'P')
			{
				if (current_agv->Is_Online_ == 1 && current_agv->Resource_Status_ != RESOURCE_STATUS::LOCKED)
				{
					PARKING_MANAGE.changeParkingStatus(current_agv->AGV_In_Station_, "BUSY");
				}
			}
		}
	}
	//log_info("");
}

void AGV_Manage::updateParkingManage()
{
	std::vector<AGV*> agv_list;
	ACS_CON_->Download_AGV_Info(agv_list);
	if (agv_list.size() != 0)
	{
		for (std::vector<AGV*>::iterator it = agv_list.begin(); it != agv_list.end(); ++it)
		{
			AGV* current_agv = *it;

			if (current_agv->AGV_In_Station_ == "P1" || current_agv->AGV_In_Station_ == "P2" || current_agv->AGV_In_Station_ == "P3" ||
				current_agv->AGV_In_Station_ == "P4" || current_agv->AGV_In_Station_ == "P5" || current_agv->AGV_In_Station_ == "P6" || current_agv->AGV_In_Station_ == "P7")
			{
				WCS_CON_->reserveCurrentParkingStation(current_agv->AGV_In_Station_);
			}
		}
	}

}

AGV* AGV_Manage::getPointAgv(int agv_id) {
	if (AGV_List_.find(agv_id) != AGV_List_.end())
	{
		return AGV_List_[agv_id];
	}
	return nullptr;
}

bool AGV_Manage::Set_Lock(int agv_id)
{
	std::map<int, AGV*>::iterator it=AGV_List_.find(agv_id);
	if(it!=AGV_List_.end())
	{
		AGV * current_AGV=it->second;
		current_AGV->Resource_Status_=RESOURCE_STATUS::LOCKED;
		return true;
	}
	return false;
}

bool AGV_Manage::Set_Unlock(int agv_id)
{
	std::map<int, AGV*>::iterator it=AGV_List_.find(agv_id);
	if(it!=AGV_List_.end())
	{
		AGV * current_AGV=it->second;
		if (current_AGV->Oreder_Status_==RESOURCE_STATUS::RESOURCE_IDLE)
		{
			current_AGV->Resource_Status_=RESOURCE_STATUS::UNLOCKED;
			
			return true;
		}
	}
	return false;
}

bool AGV_Manage::setManualUnlock(int agv_id)
{
	std::map<int, AGV*>::iterator it = AGV_List_.find(agv_id);
	if (it != AGV_List_.end())
	{
		AGV * current_AGV = it->second;
		if (current_AGV->Oreder_Status_ == RESOURCE_STATUS::RESOURCE_IDLE)
		{
			current_AGV->Resource_Status_ = RESOURCE_STATUS::UNLOCKED;
			return true;
		}
	}
	return false;
}

bool AGV_Manage::setStopCharging(int agv_id)
{
	std::map<int, AGV*>::iterator it = AGV_List_.find(agv_id);
	if (it != AGV_List_.end())
	{
		AGV * current_AGV = it->second;
		current_AGV->Is_Charging_ = CHARGING_STATUS::CHANRING_STOP;
		return true;
	}
	return false;
}

bool AGV_Manage::invokeParking(int agv_id)
{
	std::map<int, AGV*>::iterator it = AGV_List_.find(agv_id);
	if (it != AGV_List_.end())
	{
		AGV * current_AGV = it->second;
		
		current_AGV->Invoke_AGV_Parking_ = INVOKE_STATUS::BEGING_INVOKE;
		return true;
		
	}
	return false;
}

bool AGV_Manage::setBusy(int agv_id)
{
	std::map<int, AGV*>::iterator it=AGV_List_.find(agv_id);
	if(it!=AGV_List_.end())
	{
		AGV * current_AGV=it->second;
		current_AGV->Oreder_Status_=RESOURCE_STATUS::BUSY;
		return true;
	}
	return false;
}

bool AGV_Manage::setIdle(int agv_id)
{
	std::map<int, AGV*>::iterator it=AGV_List_.find(agv_id);
	if(it!=AGV_List_.end())
	{
		AGV * current_AGV=it->second;
		current_AGV->Oreder_Status_=RESOURCE_STATUS::RESOURCE_IDLE;
		return true;
	}
	return false;
}

void AGV_Manage::setAgvPauseContinue(int agv_id,std::string op)
{
	WCS_CON_->insertStopTaskToTable(agv_id,op);
}

bool AGV_Manage::areaSetAgvPause(int agv_id)
{
	if (AGV_List_.find(agv_id) != AGV_List_.end() && AGV_List_[agv_id]->Area_Running != 2)
	{
		AGV_MANAGE.setAgvMsg(agv_id, "等待区域释放");
		AGV_List_[agv_id]->Area_Running = 2;
		setAgvPauseContinue(agv_id, "force pause");
		std::string distance;
		if (CONFIG_MANAGE.readConfig("Area_Manage_Distance", &distance, "config.txt", true))
		{
			changeAgvsLineLength(agv_id, distance);
			// TODO 这里为了安全,缩短探路线长度后再次要求AGV停车
			setAgvPauseContinue(agv_id, "force pause");
		}
		std::stringstream ss;
		ss << "[AGV_Manage] area control set " << "AGV_ID:" << agv_id << ",OP: force pause";
		log_info_color(log_color::red, ss.str().c_str());
		return true;
	}
	return false;
}

bool AGV_Manage::areaSetAgvContinue(int agv_id)
{
	if (AGV_List_.find(agv_id) != AGV_List_.end() && AGV_List_[agv_id]->Area_Running != 1)
	{
		AGV_List_[agv_id]->Area_Running = 1;
		resetAgvsLineLength(agv_id);
		setAgvPauseContinue(agv_id, "force continue");
		std::stringstream ss;
		ss << "[AGV_Manage] area control set " << "AGV_ID:" << agv_id << ",OP: force continue";
		log_info_color(log_color::green, ss.str().c_str());
		return true;
	}
	return false;
}

bool AGV_Manage::Is_Parking(AGV* agv)
{
	if (((agv->AGV_In_Station_ == "P1") || (agv->AGV_In_Station_ == "P2" || (agv->AGV_In_Station_ == "P3") || (agv->AGV_In_Station_ == "P4") || (agv->AGV_In_Station_ == "P5") || (agv->AGV_In_Station_ == "P6") || (agv->AGV_In_Station_ == "P7"))) && (agv->AGV_Status_ == 0))
	{
		if (WCS_CON_->getCurrentStationIsCharging(agv->AGV_In_Station_))
		{
			agv->Parking_Is_Charging_ = "NO";
		}
		else
		{
			agv->Parking_Is_Charging_ = "YES";
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool AGV_Manage::All_agv_is_parking()
{
	for(std::map<int, AGV*>::iterator it=AGV_List_.begin(); it!=AGV_List_.end(); ++it)
	{
		AGV* current_agv=it->second;

		if(!Is_Parking(current_agv))
		{
			return false;
		}	
	}
	return true;
}

void AGV_Manage::Get_All_AGV(std::vector<AGV*> &all_agv_list)    
{
	for (std::map<int, AGV*>::iterator it = AGV_List_.begin(); it != AGV_List_.end(); ++it)
	{
		AGV* current_agv = it->second;

		if (current_agv->Is_Online_ == 1 && current_agv->AGV_Status_ == 0)
		{
			all_agv_list.push_back(current_agv);
		}
	}
}

bool AGV_Manage::setAgvMsg(int agv_id, std::string msg)
{
	if (AGV_List_.find(agv_id) != AGV_List_.end())
	{
		if (msg != AGV_List_[agv_id]->msg_)
		{
			AGV_List_[agv_id]->msg_ = msg;
			WCS_CON_->agvErrMsg(agv_id, msg);
			log_error("Report AGV Msg: " + std::to_string(agv_id) + msg);
		}
		return true;
	}
	return false;
}

bool AGV_Manage::Get_Free_AGV(std::vector<AGV*> &free_agv_list)
{
	for(std::map<int, AGV*>::iterator it=AGV_List_.begin(); it!=AGV_List_.end(); ++it)
	{
		AGV* current_agv=it->second;

		if(current_agv->Resource_Status_==RESOURCE_STATUS::UNLOCKED && current_agv->Is_Online_==1&& current_agv->Is_Serving_ == 1 &&current_agv->AGV_Status_ == 0)
		{
			free_agv_list.push_back(current_agv);
		}
	}
	if (free_agv_list.size()==0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool AGV_Manage::stopInvokeParking(int agv_id) {
	if (AGV_List_.find(agv_id) == AGV_List_.end())
	{
		return false;
	}
	AGV_List_[agv_id]->Invoke_AGV_Parking_ = STOP_INVOKE;
	return true;
}
/**
 * 获取AGV列表:1、电量高于最低电量；2、空闲
 */
bool AGV_Manage::getFreeHighBatteryAGV(std::vector<AGV*> &free_agv_list, int order_agv_type)
{
	for (std::map<int, AGV*>::iterator it = AGV_List_.begin(); it != AGV_List_.end(); ++it)
	{
		AGV* current_agv = it->second;

		if (current_agv->Resource_Status_ == RESOURCE_STATUS::UNLOCKED && current_agv->Is_Online_ == 1 && current_agv->Is_Serving_ == 1
			&& current_agv->AGV_Status_ == 0 && current_agv->AGV_Type_ == order_agv_type )
		{
			free_agv_list.push_back(current_agv);
		}
	}
	if (free_agv_list.size() == 0)
	{
		return false;
	}
	else
	{
		for (unsigned int i = 0; i < free_agv_list.size(); i++)
		{
			for (unsigned int j = i; j < free_agv_list.size(); j++)
			{
				if (free_agv_list[j]->Battery_Level_ > free_agv_list[i]->Battery_Level_)
				{
					AGV* temp = free_agv_list[i];
					free_agv_list[i] = free_agv_list[j];
					free_agv_list[j] = temp;
				}
			}
		}
		return true;
	}
}

bool AGV_Manage::getCurrentAgvFreeStatus(int agv_id)
{
	for (std::map<int, AGV*>::iterator it = AGV_List_.begin(); it != AGV_List_.end(); ++it)
	{
		AGV* current_agv = it->second;

		if (current_agv->Resource_Status_ == RESOURCE_STATUS::UNLOCKED && current_agv->Is_Online_ == 1 && current_agv->Is_Serving_ == 1 && current_agv->AGV_Status_ == 0)
		{
			if (current_agv->AGV_ID_==agv_id)
			{
				return true;
			}
		}
	}
	return false;
}

void AGV_Manage::Get_Locked_AGV(std::vector<AGV*> &locked_agv_list)
{
	for(std::map<int, AGV*>::iterator it=AGV_List_.begin(); it!=AGV_List_.end(); ++it)
	{
		AGV* current_agv=it->second;

		if(current_agv->Resource_Status_==RESOURCE_STATUS::LOCKED && current_agv->Is_Online_==1)
		{
			locked_agv_list.push_back(current_agv);
		}
	}
}

void AGV_Manage::stopCharging(AGV* agv)
{
	//int current_charging_task;
	std::stringstream ss;
	if (false & TASK_CHAIN_MANAGE.Stop_Charging(agv->AGV_ID_))//充电桩控制流程中不存在尚未下发
	{
		// 尚未下发,已截断
	}
	else
	{
		charge_task* p_task = find_charge_task(agv->AGV_ID_);
		if (p_task)
		{
			write_msg(p_task->get_queue(), NULL, 0, STOP_CHARGE_TASK_MACRO);
		}

		//WCS_CON_->stopCurrentCharging(agv);
	}
	ss << "AGV:" << agv->AGV_ID_ << "Stop charging successful";

	log_info_color(log_color::green, ss.str().c_str());

}


bool AGV_Manage::getCurrentAGVIsOnlineStatus(int agv_id)
{
	if (ACS_CON_->getCurrentAGVIsOnline(agv_id))
	{
		return true;
	}
	else
	{
		return false;
	}
}
void AGV_Manage::splitString2(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}

void AGV_Manage::getBatteryConfig()
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

bool AGV_Manage::resetAgvsLineLength(int agv_id) {
	std::string distance;
	if (CONFIG_MANAGE.readConfig("Reset_Distance", &distance, "config.txt", true))
	{
		return changeAgvsLineLength(agv_id, distance);
	}
	return false;
}

bool AGV_Manage::changeAgvsLineLength(int agv_id, std::string change_length) {
	log_info("[AGV Manage]agv id = " + std::to_string(agv_id) + " change distance to " + change_length);
	return WCS_CON_->changeAgvsLineLength(agv_id, change_length);
}

