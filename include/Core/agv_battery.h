
#ifndef _AGV_BATTERY_H
#define _AGV_BATTERY_H

#include "Core/queue.h"
#include "ADODatabase/ACS_Database.h"
#include "ADODatabase/WCS_Database.h"

class agv_battery
{
public:

	agv_battery(int id);
	~agv_battery();

	bool get_max_current_value(unsigned short& value);   /* 获取最大电流*/
	bool get_soc_value(unsigned short& value);           /* 获取soc数值 */
	bool get_battery_state(unsigned short& value);       /* 获取电池状态 */
	bool send_start_charge(int task_id);                 /* 继电器闭合*/
	bool send_stop_charge(int task_id);                  /* 继电器断开 */
	bool send_charge_finished();                         /* 发送充电完成 */
	bool send_charge_unfinished();                        /* 清空充电完成 */

	//bool get_soc_value(short& value);	/*获得指定类型的值*/
	bool get_high_voltage(unsigned short& value);         /* 获取最大电压 */
	bool get_low_voltage(unsigned short& value);          /* 获取最小电压 */
	bool get_current(short& value);                       /* 获取实时电流 */
	bool get_temperature(short& value);                   /* 获取实时温度 */
	bool get_voltage(unsigned short& value);              /* 获取实时电压 */
	bool get_target_ok(unsigned short& value);            /* 获取是否达到目标点信号 */

	bool set_agv_battery_info();
	void set_soc(unsigned short soc);
	void set_current(short current);
	void set_low_voltage(unsigned short low_voltage);
	void set_high_voltage(unsigned short high_voltage);
	void set_temperature(short temperature);
	void set_battery_state(unsigned short run_state);
	void set_max_current(unsigned short max_current);
	void set_voltage(unsigned short voltage);
	void set_target_ok(unsigned short target_ok);

	void set_target_ok_flag(bool flag);
	bool get_target_ok_flag();
	void set_current_flag(bool flag);
	bool get_current_flag();

private:
	int agv_id_;
	HANDLE hMutex_;                                         /* 互斥量 */

	unsigned short soc_;
	short current_;
	unsigned short low_voltage_;
	unsigned short high_voltage_;
	short temperature_;
	unsigned short run_state_;
	unsigned short max_current_;
	unsigned short voltage_;
	unsigned short target_ok_;

	bool target_ok_flag_;
	bool current_flag_;

	ACS_Database* ACS_CON_;
	WCS_Database* WCS_CON_;

private:
	bool send_main_thread_msg(QUEUE* p_queue, int opr, int offset, int data);

private:
	agv_battery(agv_battery&);
};

/*
* 其他函数
*/

void init_sys_battery();
void clean_sys_battery();

#endif
