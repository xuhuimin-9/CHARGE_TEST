#ifndef _WCS_Database_EDWARD_
#define _WCS_Database_EDWARD_
#ifndef no_init_all
#define no_init_all deprecated
#endif // no_init_all
#include <map>
#include "ADODatabase/ADODatabase.h"
#include "Manage/Manage_Define.h"
#include <vector>
#include "Core/Task_Define.h"
#include "Core/ModelStationParking.h" 
//#include "Core/ModelBridge.h"

class Task_Chain;

class WCS_Database
{

public:
		//void insert_sql_data();
	WCS_Database();

	~WCS_Database();

	std::map<std::string, int> agv_in_area;

	void setWcsAdo(ADOcon ADO);

	int getMaxTaskId();

	int getMaxWcsTaskId();//得到WCS中最大的ID;

	bool checkOrderInfoCount(int &max_id);

	std::vector<Oreder_Task*> updateTaskOrderInfo(std::vector<Oreder_Task*>Executing_List);

	bool updateOrderInfo();

	bool updateNewOrderInfo();

	void setCurrentStorageStatus(std::string table_name, std::string state, std::string storage_name);

	bool updateOrderStatusToDoing(std::string order_id,int agv_id);

	void updateOrderStatusToDone(Task_Chain* the_task, std::string order_type, std::string sql_table_name);

	void updateOrderStatusToHalfDone(Task_Chain* the_task, std::string order_type, std::string sql_table_name);

	void updateOrderStatusToErr(Task_Chain* the_task, std::string order_type, std::string sql_table_name);

	bool updateOrderStatus(std::string new_status, std::string old_status, std::string start);

	void updateOrderTargetStorageInfo(Task_Chain* the_task, std::string order_type, std::string sql_table_name);

	bool copyOrderInfo(Task_Chain_Info &order_in);

	bool orderErrMsg(std::string order_id, std::string msg);

	bool agvErrMsg(int agv_id, std::string msg);
	bool getSameLineTray(std::vector<Task_Chain_Info> all_order, Task_Chain_Info & order_in);

	bool areaCount(std::string station_name, bool urgent = false);
	// num 数量限制,urgent 寻找两小时前的任务
	//bool getOrder(std::vector<Task_Chain_Info> all_order, Task_Chain_Info & order_in, bool num = true, bool urgent = false);
	bool getOrder(std::vector<Task_Chain_Info> all_order, std::string area_id,  Task_Chain_Info & order_in, std::string mode = "", std::string now_area = "", bool num = true);

	std::vector<Task_Chain_Info> getRecordsetOrderList(_RecordsetPtr&);

	void deleteCurrentOrder(std::string order_id);

	void splitString2(const std::string& s, std::vector<std::string>& v, const std::string& c);

	int timestamp;

	int readTaskStatus(int taskid);

	bool stopCurrentCharging(AGV* agv);

	void abortTask(int id);

	bool insertTask(int task_id_, std::string task_from_, std::string task_to_, int priority_, int agv_type_, int task_type_, int auto_dispatch_, int agv_id_, int task_extra_param_type, std::string task_extra_param);

	bool addOrder(std::string ORDER_ID, std::string START, std::string TARGET, int PRIORITY, std::string STATUS, std::string MODE, std::string TYPE, int AGV_ID);

	//void updateCurrentOrderOver(std::string order_id);

	void updateCurrentOrderOver(std::string order_id, int agv_id);

	bool addParkingChargingOrder(std::string START, std::string TARGET, std::string TYPE, int AGV_ID);

	bool insertOrderToDb(std::string START,std::string TARGET, int PRIORITY, std::string STATUS, std::string MODE,std::string TYPE);
	
	bool insertOrderToDb(std::string ORDER_ID, std::string START, std::string TARGET, int PRIORITY, std::string STATUS, std::string MODE, std::string TYPE);
	
	bool insertOrderToDb(std::string ORDER_ID, std::string START, std::string TARGET, int PRIORITY, std::string STATUS, std::string MODE, std::string TYPE, int agvID);
	
	bool insertOrderToDb(std::string ORDER_ID, std::string START, std::string TARGET, int PRIORITY, std::string STATUS, std::string MODE, std::string TYPE, int agvID, std::string palletNo);

	bool checkNewOrderList(AGV* current_AGV);

	bool revokeDodOrder(std::string wcs_id);

	bool redoErrOrder(std::string order_id);

	void setErrStorageOrder(int wcs_task_id);

	bool unlockGoingTaskStorage(std::string table_name, std::string resource_status, std::string storage_name);

	bool setCurrentTableStorageStatus(std::string table_name, std::string resource_status, std::string storage_name, bool change_side = true);

	bool setCurrentTableGoodsStorageStatus(std::string table_name, std::string resource_status, std::string storage_name, bool change_side = true);

	bool setCurrentStationStatus(std::string station_name, std::string status);

	bool getCurrentParkingStation(std::string &station_name, std::string &is_charging, int agv_type);

	bool getAllParkingStation(std::map<std::string, ParkingStation> &all_parking);

	void reserveCurrentParkingStation(std::string station_name);

	float getDatabaseCurrentWeightInfo();

	void updateCurrentWeightInfo(double weight,std::string status);

	void releaseCurrentParkingStation(std::string station_name);

	bool setDatabaseCurrentComfirmStatus(std::string current_confirm, std::string status, int agv_id, std::string from = "", std::string to = "");

	bool setDatabaseCurrentComfirmStatus(std::string current_confirm, std::string status);

	void setDatabaseCurrentBridgeStatus(std::string current_confirm, std::string status);

	void setDatabaseCurrentParkingStatus(std::string current_parking, std::string status);

	void setDatabaseEquipmentStatus(std::string equipment_id_, std::string status);

	bool getDatabaseCurrentComfirmStatus(std::string current_confirm);

	bool getDatabaseCurrentComfirmStatus(std::string current_confirm,int agv_id);

	bool getDatabaseCurrentBridgeStatus(std::string current_bridge);

	void getConfirmStorage();

	bool getParkingStorage(std::vector<ModelStationParking*> *parkings );

	bool getCurrentStationIsCharging(std::string current_station);

	bool getCurrentMustChargingStation(std::string &station_name);

	void setDatabaseCurrentEquipmentInfoStatus(std::string current_equipment, std::string equip_status);

	void setDatabaseCurrentEquipmentAppleyStatus(std::string current_equipment, std::string status);

	std::string getDatabaseCurrentEquipmentInfoStatus(std::string current_equipment);

	std::string getCurrentAGVIsBack(int agv_id);

	bool getDatabaseCurrentNextNewOrder(std::string statioin,std::string agv_type);
	// PLC 相关流程
	bool LoadPLCMap(std::map<std::string, std::map<int, std::string>>* plc_map_, std::string table_name, std::vector<std::string> table_fields_);

	bool updatePLCStatus(std::string dev_name, std::string plc_status);

	bool updatePLCMsgStatus(std::string plc_status);

	bool updatePLCReadStatus(std::string plc_status);

	bool updatePLCMsgStatus(std::string dev_name, std::string plc_status);
	
	bool updatePLCReadStatus(std::string plc_name, std::string plc_status);

	bool loadFromDB(std::string table_name, std::vector<std::string> table_fields, std::vector<std::vector<std::string>>* result_list);
	// END-PLC

	bool getCurrentEquipToStorage(std::string &storage_group, std::string &tray_type, std::string equip_name);

	bool getCurrentTableStorageName(std::string table_name, std::string storage_status, std::string &storage_name, std::string storage_type, std::string storage_group, std::string tray_type, std::string current_storage);

	bool getCurrentTableStorageName(std::string table_name, std::string storage_status, std::string &storage_name, std::string storage_type, std::string storage_group, std::string tray_type, bool getorder);

	bool getCurrentTableStorageNameNOResource(std::string table_name, std::string storage_status, std::string &storage_name, std::string storage_type, std::string storage_group, std::string tray_type);

	int GetAgvExeGoodsTask(std::string palletNo, std::string status, std::string mode);
	
	bool plcForbidAllStorage();

	void insertStopTaskToTable(int agv_id, std::string op_code);

	bool updateOrderStatusToErr();

	bool readAreaInfo(std::map<int, std::vector<Pos_Info>> *all_area);

	bool readAreaInfo(std::map<std::string, Area_Info> &all_area);

	bool loadConfirmStationFromDB(std::map<std::string, Confirm> *confirm_list);
	//小保当盘点接口
	bool changeAgvMode(std::string equip_mode);
	bool agvBackSignal(int agv_id, std::string mode);

	//耿奕旻 写入等待预搜索的任务
	bool insertPreSearchTask(int taskId, std::string from, std::string to, int priority, std::string agv_type, std::string task_type,  int agvId, std::string status,std::string error_info);
	//耿奕旻 读取任务预搜索结果
	bool readPreSearchTaskResult(int taskId, std::string &target, int &status);
	//耿奕旻 清除预搜索结果
	bool RemovePreSearchTaskResult(int agvId);
	bool cancelRemovePreSearchTask(int agvId);
	bool changeStorageTrayType(std::string storage_name, std::string type);
	bool changeStorageMsg(std::string storage_name, std::string msg);
	void clearPreSearchFeedback();
	
	
	bool releaseCurrentConfirmStation(int AGV_ID_, std::string confirm_station);
	bool changeAgvsLineLength(int agv_id, std::string change_length);
private:
	std::vector<int>WCS_TASK_ID_;
	std::vector<std::string>storage_up_list;
	std::vector<std::string>storage_down_list;

	bool dispatchToAgv(std::vector<Task_Chain_Info>, Task_Chain_Info&, int agv_id = -1);
private:

	ADOcon WCS_ADO_;
	_RecordsetPtr recordPtr_;
	_RecordsetPtr recordPtr2_;
	_RecordsetPtr recordPtr3_;
	_RecordsetPtr recordPtr4_;

	int confirm1;
	int confirm2;
	int confirm3;
	int confirm4;
	int confirm5;

public:
	bool loadWcsTableListFromDB(std::vector<Sql_Table_Attri> &sql_table_list_);
	bool getStorageList(std::map<int, Storage> &storage_map, std::string sql_table_name);
	void updateWcsStorageStatus(std::string sql_table_name,std::string storage_name,std::string status_name ,std::string status);
	bool loadParkingStationNameListFromDB(std::vector<std::string> &parkingStationNameList);
	bool loadListFromDB(std::vector<std::string> &nameList, std::string tableName, std::string listName);
	std::string getStorageStatus(std::string table_name, std::string &storage_name);

	void redoErrStart(std::string start);
private:
	std::vector<Sql_Table_Attri> sql_table_list_;
public:
	bool setAreaInfo(std::string area_name, std::string info);
	//bool resetAgvsLineLength(int agv_id);
};

#endif