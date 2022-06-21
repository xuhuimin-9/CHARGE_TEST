/*
*
* 文件： 和电池沟通的文件
* 作者： 费晓行
* 日期： 2021年12月23日
*
*/

#include <windows.h>
#include <stdio.h>

#include <iostream>
#include <vector>

#include "ADODatabase/DatabaseManage.h"
#include "ADODatabase/ACS_Database.h"
#include "Manage/AGV_Manage.h"  
#include "comm/simplelog.h"

using namespace std;

#include "Core/agv_battery.h"

#define READ_COMMAND 0x1
#define WRITE_COMMAND 0x2

#define SOC_VALUE                (34-1)
#define CURRENT_VALUE            (33-1)
#define LOW_VOLTAGE_VALUE        (32-1)
#define HIGH_VOLTAGE_VALUE       (31-1)
#define REALTIME_TEMPERATURE     (30-1)
#define RUN_STATE_VALUE          (29-1)
#define MAX_CURRENT_VALUE        (28-1)
#define VOLTAGE_VALUE            (27-1)
#define CHARGE_OPERATION_VALUE   (26-1)
#define CHARGE_STATE_VALUE       (25-1)
#define TARGET_OK_VALUE          (24-1)

//extern QUEUE* p_main_queue;
agv_battery* p_battery[] = { 0 };
int battery_index = 0;

/*
* 构造函数
*/

agv_battery::agv_battery(int id)
{
	ACS_CON_ = DATABASE_MANAGE.Get_ACS_DB();
	WCS_CON_ = DATABASE_MANAGE.Get_WCS_DB();    //数据库连接初始化;

	this->agv_id_ = id;
	set_agv_battery_info();

	/*this->soc_ = 0;
	this->current_ = 0;
	this->low_voltage_ = 0;
	this->high_voltage_ = 0;
	this->temperature_ = 0;
	this->run_state_ = 0;
	this->max_current_ = 0;
	this->voltage_ = 0;
	this->target_ok_ = 0;*/

	this->target_ok_flag_ = false;
	this->current_flag_ = false;

	this->hMutex_ = CreateMutex(NULL, FALSE, NULL);

	//init_sys_battery();
}

/*
* 析构函数
*/

agv_battery::~agv_battery()
{
	CloseHandle(this->hMutex_);
}


/*
* 发送消息
*/

bool agv_battery::send_main_thread_msg(QUEUE* p_queue, int opr, int offset, int data)
{
	KEY_G_MSG* p_msg = NULL;
	bool result = false;

	WaitForSingleObject(p_queue->h_mutex, INFINITE);

	/* 申请内存 */
	p_msg = (KEY_G_MSG*)malloc(sizeof(KEY_G_MSG));
	memset(p_msg, 0, sizeof(KEY_G_MSG));

	/* 命令 */
	p_msg-> agv_id = agv_id_;
	p_msg-> read_or_write = opr;
	p_msg-> offset = offset;
	p_msg-> write_data = data;

	/* 发送消息 */
	result = write_msg(p_queue, p_msg, sizeof(KEY_G_MSG), KEY_G_MESSAGE_MACRO);

	/* 判断返回结果 */
	if (false == result)
	{
		free(p_msg);
		ReleaseMutex(p_queue->h_mutex);
		return false;
	}

	ReleaseMutex(p_queue->h_mutex);
	return true;
}

/*
* 配置当前agv信息
*/

bool agv_battery::set_agv_battery_info()
{
	AGV_Status battery_info;
	if (ACS_CON_->getCurrentAGVBatteryInfo(agv_id_, battery_info))
	{
		this->soc_ = battery_info.BATTERY_SOC_;
		this->current_ = battery_info.CHARGE_CURRENT_;//充电电流
		this->low_voltage_ = battery_info.LOW_VOLTAGE_;
		this->high_voltage_ = battery_info.HIGH_VOLTAGE_;
		this->temperature_ = battery_info.BATTERY_TEMP_;
		this->run_state_ = battery_info.BATTERY_STATUS_;
		this->max_current_ = battery_info.MAX_CURRENT_;
		this->voltage_ = battery_info.BATTERY_VOLTAGE_;
		return true;
	}
	return false;
}

/*
* 获取当前agv_battery的ID
*/

int agv_battery::get_agv_battery_id()
{
	return this->agv_id_;
}

/*
* 获取最高电压
*/

bool agv_battery::get_high_voltage(unsigned short& value)
{
	bool result = true;
	int opr = READ_COMMAND;
	int offset = HIGH_VOLTAGE_VALUE;
	
	//result = send_main_thread_msg(p_main_queue, opr, offset, 0);

	value = high_voltage_;
	return result;
}


/*
* 获取最小电压
*/

bool agv_battery::get_low_voltage(unsigned short& value)
{
	bool result = true;
	int opr = READ_COMMAND;
	int offset = LOW_VOLTAGE_VALUE;

	//result = send_main_thread_msg(p_main_queue, opr, offset, 0);

	value = low_voltage_;
	return result;
}


/*
* 获取当前电流
*/

bool agv_battery::get_current(short& value)
{
	if (ACS_CON_->getCurrentBattery(agv_id_ - 1, "current", value))
	{
		value = -value;
		log_info("get agv battery current : %d", value);
		return true;
	}
	return false;
}


/*
* 获取soc数值
*/

bool agv_battery::get_soc_value(unsigned short& value)
{
	if (ACS_CON_->getCurrentBattery(agv_id_ - 1, "soc", value))
	{
		log_info("agv soc : %d", value);
		return true;
	}
	return false;
}

/*
* 获取实时温度
*/

bool agv_battery::get_temperature(short& value)
{
	bool result = true;
	int opr = READ_COMMAND;
	int offset = REALTIME_TEMPERATURE;

	//result = send_main_thread_msg(p_main_queue, opr, offset, 0);

	value = (short)(temperature_) / 10;
	return result;
}

/*
* 获取电池总电压
*/

bool agv_battery::get_voltage(unsigned short& value)
{
	bool result = true;
	int opr = READ_COMMAND;
	int offset = VOLTAGE_VALUE;

	//result = send_main_thread_msg(p_main_queue, opr, offset, 0);

	value = voltage_ / 10;
	return result;
}

/*
* 获取电池运行状态
*/

bool agv_battery::get_battery_state(unsigned short& value)
{
	bool result = true;
	int opr = READ_COMMAND;
	int offset = RUN_STATE_VALUE;

	//result = send_main_thread_msg(p_main_queue, opr, offset, 0);

	value = run_state_;
	return result;
}


/*
* 获取max current数值
*/

bool agv_battery::get_max_current_value(unsigned short& value)
{
	if (ACS_CON_->getCurrentBattery(agv_id_ - 1, "max_current", value))
	{
		log_info("agv max current : %d", value);
		return true;
	}
	else
	{
		return false;
	}
}

/*
* 获取running state数值
*/

bool agv_battery::get_running_state_value(unsigned short& value)
{
	if (ACS_CON_->getCurrentBattery(agv_id_ - 1, "running_state", value))
	{
		log_info("AGV_ID : %d , running_state : %d ", agv_id_, value);
		return true;
	}
	else
	{
		return false;
	}
}

/*
* 获取on target信号
*/

bool agv_battery::get_target_ok(unsigned short& value)
{
	int opr = READ_COMMAND;
	int offset = TARGET_OK_VALUE;

	if (get_target_ok_flag())
	{
		value = target_ok_;
		set_target_ok_flag(false);
		return true;
	}

	//send_main_thread_msg(p_main_queue, opr, offset, 0);
	return false;
}


/*
* 发送继电器闭合
*/

bool agv_battery::send_start_charge(int task_id)
{

	WCS_CON_->insertTask(task_id, "NA", "NA", 999, 2, AGV_IN_SITU_CHARGING, 0, agv_id_,-1 , "");

	return true;

}

/*
* 发送继电器断开
*/

bool agv_battery::send_stop_charge(int task_id)
{
	WCS_CON_->insertTask(task_id, "NA", "NA", 999, 2, AGV_IN_SITU_CHARGING_STOP, 0, agv_id_, -1, "");
	return true;
}

/*
* 通知充电完成
*/

bool agv_battery::send_charge_finished()
{
	int opr = WRITE_COMMAND;
	int offset = CHARGE_STATE_VALUE;

	//return send_main_thread_msg(p_main_queue, opr, offset, 1);
	return true;
}


/*
* 通知充电未完成
*/

bool agv_battery::send_charge_unfinished()
{
	int opr = WRITE_COMMAND;
	int offset = CHARGE_STATE_VALUE;

	//return send_main_thread_msg(p_main_queue, opr, offset, 0);
	return true;
}

/*
* 数据拷贝
*/

void agv_battery::set_soc(unsigned short soc)
{
	this->soc_ = soc;
}

void agv_battery::set_current(short current)
{
	this->current_ = current;
}

void agv_battery::set_low_voltage(unsigned short low_voltage)
{
	this->low_voltage_ = low_voltage;
}

void agv_battery::set_high_voltage(unsigned short high_voltage)
{
	this->high_voltage_ = high_voltage;
}

void agv_battery::set_temperature(short temperature)
{
	this->temperature_ = temperature;
}

void agv_battery::set_battery_state(unsigned short run_state)
{
	this->run_state_ = run_state;
}

void agv_battery::set_max_current(unsigned short max_current)
{
	this->max_current_ = max_current;
}

void agv_battery::set_voltage(unsigned short voltage)
{
	this->voltage_ = voltage;
}

void agv_battery::set_target_ok(unsigned short target_ok)
{
	this->target_ok_ = target_ok;
}

/*
* flag数据处理
*/

void agv_battery::set_target_ok_flag(bool flag)
{
	this->target_ok_flag_ = flag;
}

bool agv_battery::get_target_ok_flag()
{
	return this->target_ok_flag_;
}

void agv_battery::set_current_flag(bool flag)
{
	this->current_flag_ = flag;
}

bool agv_battery::get_current_flag()
{
	return this->current_flag_;
}

/*
* 初始化battery的相关数据
*/

void init_sys_battery()
{
	/*std::map<int, AGV*> AGV_List_ = AGV_MANAGE.Get_All_AGV();
	vector<string> agvs;
	for each (auto agv in AGV_List_)
	{
		agvs.push_back("" + agv.second->AGV_ID_);//int转string
	}*/

	vector<string> agvs = { "1" };
	for (int i = 0; i < agvs.size(); i++)
	{
		p_battery[i] = new agv_battery(stoi(agvs[i]));

		/*if (agvs[index] == "1")
		{
			p_battery[index] = new agv_battery(1);
		}
		else
		{
			p_battery[index] = NULL;
		}*/
	}
}

/*
* 初始化指定battery的相关数据
* 返回位置
*/

int init_sys_battery(int agv_id)
{
	for (int i = 0; i < battery_index; i++)
	{
		if (p_battery[i] && p_battery[i]->get_agv_battery_id() == agv_id)
		{
			return i; //已存在
		}
	}
	for (int i = 0; i < battery_index; i++)
	{
		if (p_battery[i] == 0)
		{
			p_battery[i] = new agv_battery(agv_id);
			return i;
		}
	}
	p_battery[battery_index] = new agv_battery(agv_id);
	battery_index++;
	return battery_index - 1;
}

/*
* 清理battery的相关数据
*/

/*void clean_sys_battery()
{
	delete []p_battery;
}*/

/*
* 清理指定battery的相关数据
*/

bool clean_sys_battery(int agv_id)
{
	for (int i = 0; i < battery_index; i++)
	{
		if (p_battery[i] && p_battery[i]->get_agv_battery_id() == agv_id)
		{
			p_battery[i] = 0;
			return true;
		}
	}
	return false;
}



