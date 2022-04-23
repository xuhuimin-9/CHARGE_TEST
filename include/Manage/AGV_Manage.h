
#ifndef _AGV_MANAGE_ED_H
#define _AGV_MANAGE_ED_H
#include <string>  
#include <map> 
#include <vector>
#include <iostream>  
#include <boost/serialization/singleton.hpp>
#include "Common/Config.h"
#include "Manage/Manage_Define.h"
#include "ADODatabase/WCS_Database.h"
#include "ADODatabase/ACS_Database.h"
#include "ADODatabase/MAP_Database.h"

#define P1_AGV_ID 51
#define P2_AGV_ID 52
#define P3_AGV_ID 53
#define P4_AGV_ID 54
#define P5_AGV_ID 55

class AGV_Manage 
{  
public:  

	AGV_Manage();  

	~AGV_Manage();  

	void Initial();

	bool Get_Free_AGV(std::vector<AGV*> &free_agv_list);

	bool stopInvokeParking(int agv_id);

	bool getFreeHighBatteryAGV(std::vector<AGV*> &free_agv_list, int order_agv_type);

	bool getCurrentAgvFreeStatus(int agv_id);

	void updateParkingManage();

	AGV* getPointAgv(int agv_id);

	void Get_All_AGV(std::vector<AGV*> &all_agv_list);

	void Get_Locked_AGV(std::vector<AGV*> &locked_agv_list);

	bool Set_Lock(int agv_id);

	bool Set_Unlock(int agv_id);

	bool setManualUnlock(int agv_id);

	bool invokeParking(int agv_id);

	bool setBusy(int agv_id);

	bool setStopCharging(int agv_id);

	bool setIdle(int agv_id);

	void setAgvPauseContinue(int agv_id,std::string op);

	bool Get_AGV_Status(int agv_id, RESOURCE_STATUS &status);

	bool Get_AGV_Sub_Status(int agv_id, AGV_SUB_STATUS &status);

	bool areaSetAgvPause(int agv_id);

	bool areaSetAgvContinue(int agv_id);

	bool Is_Parking(AGV* agv);

	void stopCharging(AGV* agv);

	void Update_AGV_Data_From_ACS_Event();

	void Update_AGV_Data_To_WCS();

	bool All_agv_is_parking();

	bool getCurrentAGVIsOnlineStatus(int agv_id);

	void splitString2(const std::string& s, std::vector<std::string>& v, const std::string& c);

	void getBatteryConfig();
	
	bool resetAgvsLineLength(int agv_id);
	bool changeAgvsLineLength(int agv_id, std::string change_length);
	// 与指定站点距离
	long long getDistance(std::string station_name, int agv_id);

	std::map<int, AGV*> Get_All_AGV()
	{
		return AGV_List_;
	}

	bool setAgvMsg(int agv_id, std::string msg);
protected:


private:

	bool Is_Locked(int agv_id);

	std::string AGV_ID_;
	std::string Parking_Station_;

	std::map<int, AGV*> AGV_List_;
	std::map<int, AGV*> FAKE_CHARGING_AGV_List_;
	Config_File Config_;
	float battery_high_range_;
	float battery_low_range_;
	float unlock_battery_level_;
	WCS_Database* WCS_CON_;
	ACS_Database* ACS_CON_;
}; 

typedef boost::serialization::singleton<AGV_Manage> Singleton_AGV_Manage;
#define AGV_MANAGE Singleton_AGV_Manage::get_mutable_instance()

#endif //_AGV_MANAGE_ED_H