
#ifndef _CHARGE_TASK_H
#define _CHARGE_TASK_H

#include "Manage/Battery_Manage.h"
#include "Core/charge_station.h"
#include "Core/agv_battery.h"
#include "ADODatabase/DatabaseManage.h"

enum STATE
{
	TASK_IDLE_STATE = 0,
	TASK_ARM_OUT_STATE,
	TASK_WAIT_ARM_OUT_STATE,
	TASK_CONNECTOR_OPEN_STATE,
	TASK_CHARGE_STATE,
	TASK_ARM_IN_STATE,
	TASK_WAIT_ARM_IN_STATE,
	TASK_CONNECTOR_CLOSE_STATE,
	TASK_REPORT_CHARGE_FINISH_STATE,
	TASK_WAIT_PLC_PROCESS_STATE,
	TASK_CLEAN_CHARGE_FINISH_STATE,
	TASK_CHARGE_CURRENT,
	TASK_CHARGE_OVER,
	TASK_CHARGE_START,
	TASK_CHARGE_STOP,
	TASK_STATION_START,
	TASK_STATION_STOP,
	TASK_OVER,
	TASK_ERR
};

class charge_task
{
public:

	charge_task(agv_battery* p_battery, charge_station* p_charge);
	~charge_task();
	static boolean run_entry(charge_task* p_task);
	
	agv_battery* get_battery();
	charge_station* get_charge();
	QUEUE* get_queue();
	int get_stop();
	void set_stop(int data);
	STATE get_state();
	void set_state(STATE state);
	int get_charge_period();
	void set_charge_period(int period);
	
private:

	agv_battery* p_battery_;
	charge_station* p_charge_;
	QUEUE* p_queue_;
	int stop_;
	STATE state_;
	int charge_period_;
	double soc_;

private:

	charge_task(charge_task&);
};

void add_charge_task(int agvid, charge_task* p_work);
void del_charge_task_by_agvid(int agvid);
void del_charge_task_by_ptr(charge_task* p_work);
charge_task* find_charge_task(int agvid);
bool find_charge_task_state(int agvid);

#endif
