#include "PreSearch_Manage.h"
#include "comm/simplelog.h"

using namespace std;
PreSearch_Manage::PreSearch_Manage() {
}


PreSearch_Manage::~PreSearch_Manage() {
}


//预搜索成功返回 true ,预搜索失败返回 false 。
bool PreSearch_Manage::subTaskSearchEvent(Sub_Task* current_sub_task, Task_Chain* task_chain)
{
	m_current_sub_task = current_sub_task;
	m_task_chain = task_chain;
	std::string key = std::to_string(current_sub_task->task_id_);
	if (WCS_CON_ == nullptr)
	{
		initial();
	}
	int status = PRE_SEARCH_STATUS::INIT;
	// 曾下发过,则查找结果
	std::string target; 
	
	bool search_res = WCS_CON_->readPreSearchTaskResult(current_sub_task->task_id_, target, status);
	if (current_sub_task->pre_search )
	{
		if (search_res) 
		{
			if (current_sub_task->task_to_ != target)
			{
				// 目的地与之前不一致, 清空该车并重发
				//removePreSearchTaskResult(current_sub_task->agv_id_);
				current_sub_task->pre_search = false;
				m_Task_ExecNum_Map[key] = 0;
			}
			else
			{
				auto val = m_Task_ExecNum_Map.find(key);
				switch (status)
				{
				case  PRE_SEARCH_STATUS::FAILED:
					// 预搜索失败,开始计数
					WCS_CON_->orderErrMsg(task_chain->Order_ID_, "预搜路管制:前方有对向AGV经过");
					AGV_MANAGE.setAgvMsg(task_chain->Associate_AGV_->AGV_ID_, "预搜路管制:前方有对向AGV经过");
					m_Task_ExecNum_Map[key] ++;
					if (m_Task_ExecNum_Map[key] >= m_ExecNum)
					{
						log_info_color(log_color::red, "AGV_id:" + std::to_string(current_sub_task->agv_id_) + " endNode :" + current_sub_task->task_to_);
						current_sub_task->pre_search = false;
						m_Task_ExecNum_Map.erase(key);
					}
					break;
				case PRE_SEARCH_STATUS::SUCCESS:
				case PRE_SEARCH_STATUS::ERRORED:
					log_info_color(log_color::green, "AGV_id:" + std::to_string(current_sub_task->agv_id_) + " endNode :" + current_sub_task->task_to_);
					// 成功,下发任务
					if (val != m_Task_ExecNum_Map.end())
					{
						m_Task_ExecNum_Map.erase(val);//搜索成功,删除key
					}
					//WCS_CON_->RemovePreSearchTaskResult(m_current_sub_task->agv_id_);
					return true;
					break;
				case PRE_SEARCH_STATUS::INIT:
					//设置循环次数用于控制预搜索间隔时间
					CONFIG_MANAGE.readConfig("Exec_Num", &m_ExecNum, "config.txt", true);
					m_Task_ExecNum_Map[key] = 0;
					break;
				default:
					break;
				}
			}
		}
		else
		{
			log_info_color(red,"%d query failed", current_sub_task->task_id_);
		}
		
	}
	

	if (m_Task_ExecNum_Map[key] <= 0)
	{
		// 没有下发过,则下发
		if (!current_sub_task->pre_search)
		{
			insertPreSearchTask(current_sub_task);
		}
	}
	return false;
}

//插入预搜索的任务
bool PreSearch_Manage::insertPreSearchTask(Sub_Task* current_sub_task)
{
	if (WCS_CON_ == nullptr)
	{
		initial();
	}
	bool result = false;
	if (current_sub_task != nullptr)
	{
		std::map<int, AGV*>agv_Map = AGV_MANAGE.Get_All_AGV();
		int startNode = agv_Map[current_sub_task->agv_id_]->AGV_at_node;
		if (startNode == -1)
		{
			AGV_MANAGE.setAgvMsg(current_sub_task->agv_id_, "AGV不在点上，请取消任务后重新拉至停车点");
			return false;
		}

		// 先取消“取消预搜路请求”，再下发任务
		cancelRemovePreSearchTask(current_sub_task->agv_id_);
		// 写入预搜索数据库,等待回应
		current_sub_task->pre_search = WCS_CON_->insertPreSearchTask(current_sub_task->task_id_,to_string(startNode), current_sub_task->task_to_,
			current_sub_task->priority_, "2", to_string((int)current_sub_task->task_type_), current_sub_task->agv_id_,
			to_string(current_sub_task->sub_task_status_),"");
	}
	return current_sub_task->pre_search;
}


//初始化
void PreSearch_Manage::initial()
{
	WCS_CON_ = DATABASE_MANAGE.Get_WCS_DB();
	WCS_CON_->clearPreSearchFeedback();
}

bool PreSearch_Manage::removePreSearchTaskResult(int agv_id_)
{
	if (WCS_CON_ == nullptr)
	{
		initial();
	}
	return WCS_CON_->RemovePreSearchTaskResult(agv_id_);
}

// 取消清空预搜路：向AGVS的请求列表中，可能有请求清空预搜路结果，取消这个请求
bool PreSearch_Manage::cancelRemovePreSearchTask(int agv_id) {
	if (WCS_CON_ == nullptr)
	{
		initial();
	}
	return WCS_CON_->cancelRemovePreSearchTask(agv_id);
}