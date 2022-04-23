#ifndef _PRESEARCH_MANAGE_H
#define _PRESEARCH_MANAGE_H
#include "Core/Sub_Task.h"
#include"Core/Task_Chain.h"
#include "ADODatabase/WCS_Database.h"
#include "ADODatabase/DatabaseManage.h"
#include <list>
#include <boost/serialization/singleton.hpp>
#include <boost/thread.hpp>
#include "Manage/AGV_Manage.h"
#include "Manage/Config_Manage.h"


class PreSearch_Manage{

	typedef enum {
		SUCCESS = 0,
		FAILED,
		ERRORED,
		INIT = 10,
	}PRE_SEARCH_STATUS;
public:
	PreSearch_Manage();
	~PreSearch_Manage();
	//WCS从任务组合模块中获取新子任务并下发,预搜索成功返回 true ，预搜索失败返回 false 。
	bool subTaskSearchEvent(Sub_Task* current_sub_task, Task_Chain* task_chain);
	bool insertPreSearchTask(Sub_Task* current_sub_task);

	bool removePreSearchTaskResult(int agv_id_);
	bool cancelRemovePreSearchTask(int agv_id);
private:
	WCS_Database* WCS_CON_;
	Sub_Task* m_current_sub_task;
	Task_Chain* m_task_chain;
	int m_ExecNum = 120;//循环次数用于控制预搜索间隔时间
	std::map<std::string, int> m_Task_ExecNum_Map;
	void initial();
};

typedef boost::serialization::singleton<PreSearch_Manage> Singleton_PreSearch_Manage;
#define PRESEARCH_MANAGE Singleton_PreSearch_Manage::get_mutable_instance()

#endif// _PRESEARCH_MANAGE_H