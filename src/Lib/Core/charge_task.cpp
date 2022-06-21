/*
*
* 文件： 自动充电任务的文件
* 作者： 费晓行
* 日期： 2021年12月23日
*
*/

#include <windows.h>

#include <iostream>
#include <vector>
using namespace std;

#include "Core/charge_task.h"
#include "Core/queue.h"
#include "Core/charge_station.h"
#include "Core/agv_battery.h"
#include "Manage/AGV_Manage.h"
#include "Manage/Config_Manage.h"
#include "comm/simplelog.h"
#include "Core/Task_Chain_Manage.h"

//extern QUEUE* p_main_queue;

struct runnig_charge_task
{
	int agvid;
	charge_task* p_work;
};

static vector<runnig_charge_task> g_charge_task;

/*
* 添加任务到charging task
*/

void add_charge_task(int agvid, charge_task* p_work)
{
	runnig_charge_task r;
	r.agvid = agvid;
	r.p_work = p_work;

	g_charge_task.push_back(r);
}

/*
* 根据agv删除数据
*/

void del_charge_task_by_agvid(int agvid)
{
	vector< runnig_charge_task>::iterator it;

	for (it = g_charge_task.begin(); it != g_charge_task.end(); it++)
	{
		if (it->agvid == agvid)
		{
			g_charge_task.erase(it);
			break;
		}
	}
}

/*
* 根据agv删除数据
*/

void del_charge_task_by_ptr(charge_task* p_work)
{
	vector< runnig_charge_task>::iterator it;

	for (it = g_charge_task.begin(); it != g_charge_task.end(); it++)
	{
		if (it->p_work == p_work)
		{
			g_charge_task.erase(it);
			break;
		}
	}
}

/*
* 根据agv查找charging task
*/

charge_task* find_charge_task(int agvid)
{
	vector< runnig_charge_task>::iterator it;

	for (it = g_charge_task.begin(); it != g_charge_task.end(); it++)
	{
		if (it->agvid == agvid)
		{
			return it->p_work;
		}
	}

	return NULL;
}

/*
* 根据agv查找charging task 是否完成【任务列表没有】
* 数据 AGV id
* 结果 true为完成，false为未完成
*/

bool find_charge_task_is_over(int agvid)
{
	charge_task* p_task = find_charge_task(agvid);
	if (p_task)
	{
		return false;
	}
	return true;

}


/*
* 构造函数
*/

charge_task::charge_task(agv_battery* p_battery, charge_station* p_charge)
{
	p_battery_ = p_battery;
	p_charge_ = p_charge;
	p_queue_ = alloc_queue();
	stop_ = 0;
	state_ = TASK_CHARGE_START; 
	charge_period_ = 100;  /* seconds */
	getBatteryConfig();
}

/*
* 析构函数
*/

charge_task::~charge_task()
{
	free_queue(this->p_queue_);
}

/*
* 获取电池指针
*/

agv_battery* charge_task::get_battery()
{
	
	return this->p_battery_;
}

/*
* 获取充电桩指针
*/

charge_station* charge_task::get_charge()
{
	return this->p_charge_;
}

/*
* 获取消息指针
*/

QUEUE* charge_task::get_queue()
{
	return this->p_queue_;
}

/*
* 获取退出标识符
*/

int charge_task::get_stop()
{
	return this->stop_;
}

/*
* 设置退出标志
*/

void charge_task::set_stop(int data)
{
	this->stop_ = data;
}

/*
* 获取状态
*/

STATE charge_task::get_state()
{
	return this->state_;
}

/*
* 设置状态
*/

void charge_task::set_state(STATE state)
{
	this->state_ = state;
}

/*
* 获得充电时间
*/

int charge_task::get_charge_period()
{
	return this->charge_period_;
}

/*
* 配置充电时间
*/

void charge_task::set_charge_period(int period)
{
	this->charge_period_ = period;
}

void charge_task::getBatteryConfig()
{
	CONFIG_MANAGE.readConfig("Stop_Charge_Level", &stop_charge_soc_);
}


/*
* 充电任务
*/

boolean charge_task::run_entry(charge_task* p_task)
{
	unsigned short max_current;
	unsigned char error;
	unsigned short battery_state;
	unsigned char stub_state;
	boolean result = FALSE;
	unsigned short soc_value;
	unsigned int voltage;
	short current;
	unsigned short target_ok;
	int index = 0;
	int err_current_cnt = 0;
	int err_soc_cnt = 0;

	if (NULL == p_task)
	{
		return FALSE;
	}

	/* 获取关键的电池和充电桩数据 */
	//电池信息从数据库获取
	//充电桩信息实时读
	agv_battery* p_battery;
	charge_station* p_charge;

	p_battery = p_task->get_battery();//电池
	p_charge = p_task->get_charge();//充电桩

	/* 判断数据合法性 */

	if(NULL == p_battery || NULL == p_charge)
	{
		return FALSE;
	}

	/* 获取数据 */
	void* p_void;
	int len;
	int type;

	while (1)
	{
		/* 判断是否要退出 */
		if (p_task->get_stop())
		{
			break;
		}

		/* 判断是否有消息要处理 */
		if (TRUE == read_msg(p_task->get_queue(), &p_void, &len, &type))
		{
			switch (type)
			{
			case STOP_CHARGE_TASK_MACRO:

				index = 0;
				p_task->set_state(TASK_ARM_IN_STATE);
				break;

			default:
				break;
			}

			if (p_void)
			{
				free(p_void);
			}
		}
		/* 执行任务状态机 */
		switch (p_task->get_state())
		{

		case TASK_CHARGE_START:

			/* AGV电池打开吸合器 */

			p_battery->send_start_charge(TASK_CHAIN_MANAGE.Generate_New_Sub_Task_ID());
			p_charge->set_charge_current(20);
			Sleep(1000);
			log_info("current task state is TASK_CHARGE_START , will stwich to TASK_ARM_OUT_STATE!");
			p_task->set_state(TASK_ARM_OUT_STATE); //伸出机械臂
			break;

		case TASK_ARM_OUT_STATE:

			/* 伸出机械臂 */
			if (index < 3)
			{
				write_msg(p_charge->get_queue(), NULL, 0, ARM_OUT_MSG_MACRO);

				Sleep(100);
				index += 1;
				continue;
			}
		
			index = 0;
			log_info("current task state is TASK_ARM_OUT_STATE , will stwich to TASK_WAIT_ARM_OUT_STATE!");
			p_task->set_state(TASK_WAIT_ARM_OUT_STATE);
			break;

		case TASK_WAIT_ARM_OUT_STATE:

			/* 确保机械臂伸出去 */

			if (index < 150)
			{
				Sleep(100);
				index += 1;
				continue;
			}

			index = 0;
			log_info("current task state is TASK_WAIT_ARM_OUT_STATE , will stwich to TASK_CHARGE_CURRENT!");
			p_task->set_state(TASK_CHARGE_CURRENT);
			break;

		case TASK_CHARGE_CURRENT:

			/* 设定充电电流 */

			write_msg(p_charge->get_queue(), NULL, 0, START_CHARGE_MSG_MACRO);

			index = 0;
			Sleep(1000);
			log_info("current task state is TASK_CHARGE_CURRENT , will stwich to TASK_CHARGE_STATE!");
			p_task->set_state(TASK_CHARGE_STATE);
			break;

		case TASK_CHARGE_STATE:

			/* 充电状态监控 */

			Sleep(100);
			index++;

			if (index == 0)
			{
				log_info("TASK STATE : TASK_CHARGE_STATE!");
			}
			else if (0 == index % 100)
			{
				log_info("sleep %d\n", index / 10);
			}
			else if (150 == index % 200)
			{
				if (p_task->get_battery()->get_current(current))
				{
					//如果电流为放电电流 即充电失败
					if (current < 0 )
					{
						err_current_cnt += 1;
						if (2 == err_current_cnt)
						{
							log_info("exceptional current was found!");

							p_charge->set_charge_current(0);
							write_msg(p_charge->get_queue(), NULL, 0, START_CHARGE_MSG_MACRO);
							p_battery->get_running_state_value(battery_state);

							goto exit;
						}
					}
					else
					{
						err_current_cnt = 0;
					}
				}
			}
			else if (160 == index % 200)
			{
				if (p_task->get_battery()->get_soc_value(soc_value))
				{
					if (soc_value > p_task->stop_charge_soc_) //当电量达85则自动断开
					{
						err_soc_cnt += 1;

						if (2 == err_soc_cnt)
						{
							log_info("soc target was reached already!");
							goto exit;
						}
					}
					else
					{
						err_soc_cnt = 0;
					}
				}
			}
			else if (180 == index % 200)
			{
				if (p_task->get_battery()->get_max_current_value(max_current))
				{
					if (max_current <= 10 )
					{
						goto exit;
					}
					else if(max_current > 120)
					{
						max_current = 120 ;
					}
					p_charge->set_max_current(max_current);
					int charge_current = p_charge->get_charge_current();
					if ((charge_current + 10) < max_current)
					{
						log_info("current : %d A, max current : %d A!", charge_current, max_current);
						//int value = (max_current - 10 - charge_current) * 0.3;
						charge_current += 10;
						p_charge->set_charge_current(charge_current);
						write_msg(p_charge->get_queue(), NULL, 0, START_CHARGE_MSG_MACRO);
						break;
					}
					else if (charge_current > max_current)
					{
						charge_current -= 10;
						p_charge->set_charge_current(charge_current);
						write_msg(p_charge->get_queue(), NULL, 0, START_CHARGE_MSG_MACRO);
					}
				}
			}
			continue;
		exit:
			index = 0;
			err_current_cnt = 0;
			err_soc_cnt = 0;
			p_task->set_state(TASK_ARM_IN_STATE);
			break;

		case TASK_ARM_IN_STATE:

			/* 收回充电桩 */

			if (index < 3)
			{

				write_msg(p_charge->get_queue(), NULL, 0, ARM_IN_MSG_MACRO);

				Sleep(100);
				index += 1;
				continue;
			}

			index = 0;
			log_info("TASK STATE : TASK_ARM_IN_STATE!");
			p_task->set_state(TASK_WAIT_ARM_IN_STATE);
			break;

		case TASK_WAIT_ARM_IN_STATE:

			/* 确保充电桩缩回去 */

			if (index < 60)
			{
				index++;
				Sleep(100);
				continue;
			}

			index = 0;
			log_info("TASK STATE : TASK_WAIT_ARM_IN_STATE!");
			p_task->set_state(TASK_CHARGE_STOP);
			break;

		case TASK_CHARGE_STOP:

			/* AGV关闭吸合器 */

			p_battery->send_stop_charge(TASK_CHAIN_MANAGE.Generate_New_Sub_Task_ID());
			Sleep(500);
			p_task->set_state(TASK_OVER);
			break;

		case TASK_OVER:

			/* 任务结束 */

			log_info("AGV CHARGE TASK OVER!");
			p_task->set_stop(1);
			clean_sys_battery(p_battery->get_agv_battery_id());

			break;
		default:
			break;

		}
	}

	/* 释放内存 */
	del_charge_task_by_ptr(p_task);
	delete p_task;

	return result;

}