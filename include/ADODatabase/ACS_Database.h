#ifndef _ACS_DATABASE_EDWARD_
#define _ACS_DATABASE_EDWARD_


#include "ADODatabase/ADODatabase.h"

class ACS_Database
{



public:
	ACS_Database();
	~ACS_Database();



	void set_ACS_ADO(ADOcon ADO);

	bool Download_AGV_Info(std::vector<AGV*> &agv_list);

	//bool Download_AGV_Error_Info(std::vector<AGV_Error_Log*> &agv_error_list);
	bool Download_AGV_Error_Info();

	bool Update_Error_Info_Exist_Status(int ACS_ID,std::string STATUS);

	bool getCurrentChargingTask(AGV* agv, int& task_id);

	bool getCurrentAGVIsOnline(int agv_id);

	bool getCurrentAGVBatteryInfo(int agv_id, AGV_Status &battery_info);

	bool getCurrentBattery(int agv_id, std::string type, short &value);

	bool getCurrentBattery(int agv_id, std::string type, unsigned short &value);

private:

	ADOcon ACS_ADO_;
	_RecordsetPtr recordPtr_;
};

#endif