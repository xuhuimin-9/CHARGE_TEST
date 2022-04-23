
#ifndef _MANAGE_DEFINE_H
#define _MANAGE_DEFINE_H

#include "Core/Task_Define.h"
#include <vector>
typedef enum{
	STOPED,
	HALT,
	SERVING
}SYSTEM_STATUS;


typedef struct _Pos_Info
{
	double x;
	double y;
	double theta;

} Pos_Info;

typedef struct _Area_Info
{
	std::string area_name;
	std::vector<Pos_Info> pos_list;
	std::string area_type;
	std::string info;
} Area_Info;

typedef struct _Order
{
	std::string ORDER_ID_;
	std::string START_;
	std::string Belong_Column_;     //库位所属列;
	std::string TARGETED_;
	int PRIORITY_;
	std::string STATUS_;
	std::string MODE_;
	std::string TYPE_;
	int AGV_ID_;
	std::string ENTERDATE_;
	std::string STARTDATE_;
	std::string FINISHDATE_;
	std::string Msg_;
}Order;

typedef struct _S2Store
{
	int name_id;
	std::string name;
	int idle;
}S2Store;

typedef struct _Oreder_Task
{
	int ID;
	std::string ORDER_ID;
	std::string START_POINT;
	std::string TARGET_POINT;
	std::string MODE;
	std::string TYPE;
	std::string PALLETNO;
	int AGV_ID;
	
}Oreder_Task;

typedef struct _Pallet_Task
{
	int ID_;
	std::string EQUIP_NAME_;
	int PRIORITY_;
	std::string ENTERDATE_;
	std::string FINISHDATE_;
	std::string STATUS_;
	std::string DIRFLG_;
}Pallet_Task;

typedef struct _Task_Dispatch
{
	int task_id_;
	std::string task_from_;
	std::string task_to_;
	int priority_;
	int agv_type_;
	int task_type_;
	int auto_dispatch_;
	int agv_id_;
	bool dispatched;
	int task_extra_param_type_;
	std::string task_extra_param_;
}Task_Dispatch;

typedef struct _Resoure 
{
	int ID_;
	int Resource_Status_;
	int Oreder_Status_;
}Resoure;

typedef struct _AGV: Resoure
{
	int AGV_ID_;
	int AGV_Type_;
	int AGV_Status_;
	int Error_Code_;
	int Is_Online_;
	int Is_Assign_;
	int Is_Serving_;
	int AGV_at_node;//耿奕旻加，预搜索用
	CHARGING_STATUS Is_Charging_;
	INVOKE_STATUS Invoke_AGV_Parking_;
	Pos_Info AGV_Pos_;
	std::string AGV_In_Station_;
	std::string Parking_Point;
	double Battery_Level_;
	bool Charging_Status_;
	std::string Parking_Station_;
	std::string Parking_Is_Charging_;
	int Area_Running = 0;// 区域管制类型 0为初始化，1为继续执行，2为暂停执行
	std::string msg_ = "";
}AGV;

typedef struct _AGV_Status : Resoure
{
	int AGV_ID_;
	double BATTERY_SOC_;//电量
	double BATTERY_TEMP_;//电池温度
	double BATTERY_VOLTAGE_;//电压
	int BATTERY_STATUS_;//电池状态
	int BATTERY_ERR_CODE_;
	double CHARGE_CURRENT_;//充电电流
	double DISCHARGE_CURRENT_;//放电电流
	double MAX_CURRENT;//最大电流
	double HIGH_VOLTAGE;//最高电压
	double LOW_VOLTAGE;//最低电压
	int GLOBAL_PLANNER_STATUS_;
	int LOCAL_PLANNER_STATUS_;
	int OBSTACLE_DETECT_STATUS_;
	int AMCL_STATUS_;
	int OUT_OF_LINE_STATUS_;
	int	STOPPING_STATUS_;
	int	OBSTACLE_STOP_STATUS_;
	int	COMMAND_STOP_STATUS_;
	int COLLISION_STOP_STATUS_;
}AGV_Status;

typedef struct _Storage: Resoure
{
	int ID_;
	std::string Storage_Name_;
	int Storage_Row_;
	int Storage_Column_;
	int Storage_Depth_;
	std::string Storage_Status_;
	std::string Belong_Column_;
	int All_Storage_Num_;
	int Full_Storage_Num_;
	std::string Belong_Area_;
	std::string Area_Lock_;
	std::string Belong_Station_;
	std::string Storage_Type_;
	std::string Resource_status_;
	std::string sql_table_name_;
}Storage;

typedef struct _ParkingStation : Resoure
{
	int ID_;
	std::string StationName;
	std::string IsCharging;
	std::string Status;
	int AgvType;

}ParkingStation;

typedef struct _Confirm : Storage
{
}Confirm;

typedef struct _AGV_Error_Log
{
	int ID_;
	int AGV_ID_;
	int CODE_;
	std::string DESCRIPTION_;
	std::string STATUS_;
	std::string TIMESTAMP_;
}AGV_Err_Info;

typedef struct _AGVS_Error_Log
{
	int ID_;
	int TASK_ID_;
	int CODE_;
	std::string DESCRIPTION_;
	std::string TIMESTAMP_;
}AGVS_Error_Log;

typedef enum {
	UNLOCKED=0,
	LOCKED=1,
	BUSY=2,
	RESOURCE_IDLE =3
}RESOURCE_STATUS;

typedef enum {
	IDLE=0,
	RUNNING=2
}AGV_SUB_STATUS;

struct Sql_Table_Attri
{
	std::string sql_table_name;
	std::string sql_table_type;
	std::string group_name;
	int columnSpan;
	int rowSpan;
	int widget_column;
	int widget_row;
	int status_group;
};

#endif //_MANAGE_DEFINE_H
