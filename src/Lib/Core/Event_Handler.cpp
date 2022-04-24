#include "Core/Event_Handler.h" 
#include "Core/Task_Chain.h"
#include "Manage/AGV_Manage.h"
#include "Manage/Storage_Manage.h"
#include "Manage/PreSearch_Manage.h"
#include "Manage/Area_Manage.h"
#include <Windows.h>
#include "comm/simplelog.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include "ADODatabase/ApiClient.h"
#include "Core/charge_station.h"
#include "Core/agv_battery.h"

Event_Handler::Event_Handler()
{	
	if (!Executing_List_.empty())
	{
		Executing_List_.erase(Executing_List_.begin(), Executing_List_.end());
	}
}

Event_Handler::~Event_Handler()
{

}

void Event_Handler::Initial(){
	
	WCS_CON_ = DATABASE_MANAGE.Get_WCS_DB();    //数据库连接初始化;
	AGV_MANAGE.Initial();						//AGV初始化;
	ORDER_MANAGE.Initial();
	TASK_CHAIN_MANAGE.Initial();
	DATABASE_MANAGE.Initial(); 
	STORAGE_MANAGE.Initial();
	PARKING_MANAGE.Initial();

	init_sys_charge();//初始化充电桩
	init_sys_battery();//初始化电池

	existstorages = new std::map<std::string, int>;
}

void Event_Handler::Data_Update_Event()
{
	// 接取工单后直接分解并下发
	EVENT_HANDLER.Task_Fetcher_Event();
	EVENT_HANDLER.Task_Dispatch_Event();

	ORDER_MANAGE.Download_Order_Info_From_Database_Event();
	
	TASK_CHAIN_MANAGE.Task_Status_Check_Event();

	Feedback_Event();

	//API_CLIENT.updateAllStorageView();
	//Executing_List_ = WCS_CON_->updateTaskOrderInfo(Executing_List_);
}

void Event_Handler::High_Priority_Battery_Event()
{
	BATTERY_MANAGE.High_Priority_Battery_Monitor_System();
}

void Event_Handler::Low_Priority_Battery_Event()
{
	BATTERY_MANAGE.Low_Priority_Battery_Monitor_Subsystem();
	
	BATTERY_MANAGE.Charging_AGV_Monitor();
}

void Event_Handler::Parking_Event()
{
	PARKING_MANAGE.Auto_Parking_Monitor();
}

void Event_Handler::Feedback_Event(){
	for each (auto var in TASK_CHAIN_MANAGE.Active_Task_List_)
	{
		var->Set_Status(WCS_CON_->readTaskStatus(var->Task_ID_));
	}
}

void Event_Handler::Task_Dispatch_Event()
{
	std::vector<AGV*> All_AGV_List;

	AGV_MANAGE.Get_All_AGV(All_AGV_List);

	for (std::vector<AGV*>::iterator it = All_AGV_List.begin(); it != All_AGV_List.end(); ++it)
	{
		AGV* current_AGV=*it;
					
		if(Task_Dispatcher_AGV(current_AGV))
		{		
			AGV_MANAGE.Set_Lock(current_AGV->ID_);
			AGV_MANAGE.setBusy(current_AGV->ID_);
		}			
	}
}

bool Event_Handler::Task_Dispatcher_AGV(AGV* current_AGV)  //AGV任务派发
{
	std::vector<Oreder_Task*>::iterator it;
	
	for (it = Executing_List_.begin(); it != Executing_List_.end();)
	{
		Oreder_Task *order_task = *it;
		
		if (current_AGV->AGV_ID_ == order_task->AGV_ID)
		{ 
			if (TASK_CHAIN_MANAGE.Create_Task_Chain(order_task->ORDER_ID, order_task->START_POINT, order_task->TARGET_POINT, order_task->MODE, order_task->TYPE, current_AGV,order_task->PALLETNO))
			{
				// TODO
				WCS_CON_->updateOrderStatusToDoing(order_task->ORDER_ID, order_task->AGV_ID);
			
				delete(order_task);
				it = Executing_List_.erase(it);
				return true;
			}
			else
			{
				static int a = 0;
				a++;
				if (a == 100)
				{
					a = 0;
				}
				if (a == 5)
				{
					log_info("TASK_CHAIN_MANAGE.Create_Task_Chain : fail : " + std::to_string(current_AGV->AGV_ID_));
				}
				it++;
			}
		}
		else
		{
			it++;
		}
	}
	return false;
}

bool Event_Handler::Sub_Task_Dispatch_Event(Sub_Task* current_sub_task,Task_Chain* task, bool pre_search /*= true*/, std::string & ErrorMsg /*= std::string("")*/)
{
	if (task->Associate_AGV_->Is_Serving_ != 1)
	{
		// AGVS反馈：AGV不可用
		ErrorMsg = "AGVS已锁定该车";
		AGV_MANAGE.setAgvMsg(task->Associate_AGV_->AGV_ID_, "AGVS已锁定该车");
		return false;
	}
	task->dispatch_status = true;
	std::string msg = "";
	if (ForbidPathByReserveStorage(current_sub_task->task_from_, current_sub_task->task_to_, msg))
	{
		static int num2 = 0;
		if (++num2 = 100)
		{
			num2 = 0;
		}
		if (num2 == 5)
		{
			log_error("AGV " + std::to_string(task->Associate_AGV_->AGV_ID_) + " at " + current_sub_task->task_from_ + " to " + current_sub_task->task_to_ + " but other agv before");
		}
		AGV_MANAGE.setAgvMsg(task->Associate_AGV_->AGV_ID_, current_sub_task->task_from_ + "去往" + current_sub_task->task_to_ + ":库位有AGV经过:" + msg);
		return false;
	}
	if (current_sub_task->task_from_ == "P7" && AREA_MANAGE.agvNumInArea("D_MIDDLE_P7") != 0)
	{
		// 等待区域空闲
		AGV_MANAGE.setAgvMsg(task->Associate_AGV_->AGV_ID_, "预搜路管制:前方区域有AGV经过");
		return false;
	}
	else if (current_sub_task->task_from_ == "P6" && AREA_MANAGE.agvNumInArea("C_MIDDLE_P6") != 0)
	{
		AGV_MANAGE.setAgvMsg(task->Associate_AGV_->AGV_ID_, "预搜路管制:前方区域有AGV经过");
		return false;
	}
	if (current_sub_task->task_from_ == "None")
	{
		//不在站点的AGV
		static int num = 0;
		if (++num = 100)
		{
			num = 0;
		}
		if (num == 5)
		{
			log_error("AGV " + std::to_string(task->Associate_AGV_->AGV_ID_) + " is on the station named None");
			AGV_MANAGE.setAgvMsg(task->Associate_AGV_->AGV_ID_, "预搜路管制:AGV不在站点");
		}
		return false;
	}

	if (pre_search)
	{
		if (!PRESEARCH_MANAGE.subTaskSearchEvent(current_sub_task, task))
		{
			// 预搜失败,退出
			return false;
		}
	}
	
	Task_Chain* current_task = task;
	Sub_Task *Current_Sub_Task = current_sub_task;
	std::string task_from = current_sub_task->task_from_;
	std::string task_to = current_sub_task->task_to_;

	Task_Dispatch_ = Current_Sub_Task->Dispatch(task_from, task_to);

	current_task->Task_ID_ = Task_Dispatch_.task_id_;

	/* 昀迪充电桩测试，如果是假原地充电任务直接返回true，不下发给数据库 */
	if (Task_Dispatch_.task_type_ == 1041)
	{
		return true;
	}

	if ( Task_Dispatch_.dispatched == false)
	{
		current_sub_task->dispatched = WCS_CON_->insertTask(Task_Dispatch_.task_id_, Task_Dispatch_.task_from_, Task_Dispatch_.task_to_, Task_Dispatch_.priority_, Task_Dispatch_.agv_type_, Task_Dispatch_.task_type_, Task_Dispatch_.auto_dispatch_, Task_Dispatch_.agv_id_, Task_Dispatch_.task_extra_param_type_, Task_Dispatch_.task_extra_param_);
	}
	return true;
}

void Event_Handler::Task_Fetcher_Event()         //订单分配 :kh_order_info ==> kh_report_order_info
{ 
	Task_Chain_Info new_task_in;

	//std::vector<AGV*> Free_AGV_List;

	int priority = 1;
	int agv_type = 1;
	int agv_id = 1;
	std::string parking_station;

	if (WCS_CON_->copyOrderInfo(new_task_in))
	{
#if 0
		if (new_task_in.MODE_ == "TRAY")
		{
			switch (RABBITMQ_SERVICE.ApplyUnit(new_task_in.START_, new_task_in.ORDER_ID_))
			{
			case 1:
				AGV_MANAGE.setAgvMsg(new_task_in.AGV_ID_, "等待确认滚筒线无空轮:" + new_task_in.START_);
				break;
			case 2:
				WCS_CON_->orderErrMsg(new_task_in.ORDER_ID_, "已存在空轮,取消");
				ORDER_MANAGE.Revoke_Order(new_task_in.ORDER_ID_);
				return;
			case 0:
				AGV_MANAGE.setAgvMsg(new_task_in.AGV_ID_, "等待确认滚筒线无空轮:" + new_task_in.START_);
				return;
			default:
				return;
			}
		}
#endif
		std::string start_area = TASK_CHAIN_MANAGE.get_storage_area(new_task_in.START_);
		std::string target_area = TASK_CHAIN_MANAGE.get_storage_area(new_task_in.TARGET_);
		
		WCS_CON_->setDatabaseCurrentComfirmStatus(start_area, "BUSY", new_task_in.AGV_ID_);
		WCS_CON_->setDatabaseCurrentComfirmStatus(target_area, "BUSY", new_task_in.AGV_ID_);
		//WCS_CON_->setDatabaseCurrentComfirmStatus(start_area, "BUSY");
		//WCS_CON_->setDatabaseCurrentComfirmStatus(target_area, "BUSY");

		agv_id = new_task_in.AGV_ID_;
		if (AGV_MANAGE.getCurrentAgvFreeStatus(agv_id))
		{
			AGV_MANAGE.setAgvMsg(new_task_in.AGV_ID_, "派发任务:" + new_task_in.START_ + ":" + new_task_in.MODE_);
			AGV_MANAGE.Set_Lock(agv_id);   //分配任务后锁定车辆;
			AGV_MANAGE.setBusy(agv_id);

			std::stringstream ss;
			ss << "AGV_" << agv_id << "Assign AGV Order Locked";
			log_info(ss.str().c_str());

			// 此处提前预定库位
			WCS_CON_->setCurrentTableStorageStatus("s_storage_station_status", "GOING", new_task_in.TARGET_, false);

			WCS_CON_->updateCurrentOrderOver(new_task_in.ORDER_ID_, agv_id);
			WCS_CON_->addOrder(new_task_in.ORDER_ID_, new_task_in.START_, new_task_in.TARGET_, new_task_in.PRIORITY_, "NEW",
				new_task_in.MODE_, new_task_in.TYPE_, agv_id);

			Oreder_Task* order_task = new Oreder_Task;

			order_task->ORDER_ID = new_task_in.ORDER_ID_;
			order_task->AGV_ID = new_task_in.AGV_ID_;
			order_task->START_POINT = new_task_in.START_;
			order_task->TARGET_POINT = new_task_in.TARGET_;
			order_task->MODE = new_task_in.MODE_;
			order_task->TYPE = new_task_in.TYPE_;
			order_task->PALLETNO = new_task_in.PALLETNO_;
			Executing_List_.push_back(order_task);
		}
		else
		{
			AGV_MANAGE.setAgvMsg(agv_id, "AGV dispatched task but not free");
		}
	}
	
	return;
}

bool Event_Handler::Assign_AGV_Current_Order(int &agv_id)
{
	std::vector<AGV*> Free_AGV_List;
	if (AGV_MANAGE.Get_Free_AGV(Free_AGV_List))
	{
		for (unsigned int i = 0; i < Free_AGV_List.size(); i++)
		{
			for (unsigned int j = i; j < Free_AGV_List.size(); j++)
			{
				if (Free_AGV_List[j]->Battery_Level_ > Free_AGV_List[i]->Battery_Level_)
				{
					AGV* temp = Free_AGV_List[i];
					Free_AGV_List[i] = Free_AGV_List[j];
					Free_AGV_List[j] = temp;
				}
			}
		}
		std::vector<AGV*>::iterator it = Free_AGV_List.begin();
		AGV* current_AGV = *it;
		agv_id = current_AGV->AGV_ID_;
		return true;
	}
	else
	{
		agv_id = -1;
		return false;
	}
}

void Event_Handler::splitString2(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}


/// <summary>
/// Gets the standard storage.
/// </summary>
/// <param name="storage">The storage.</param>
/// <param name="status">The status.</param>
/// <returns></returns>
/// Creator:MeiXu
/// CreateDate:2021/1/21
/// 
bool Event_Handler::getStandardStorage(std::string &storage, const std::string& status) {
	storage = "";
	// 库位长宽

	const int row = 13, col = 19;
	int storages[row + 1][col + 1] = { {0} };
	// 标记库位
	// 获取所有符合状态的库位
	std::vector<std::string> standard_storages = STORAGE_MANAGE.getStandardStorage(status);
	for each (std::string var in standard_storages)
	{
		// 库位名获取库位位置
		long storage_name;
		std::istringstream is(var);
		is >> storage_name;
		if ((storage_name) %1000000000 / 10000  == 20501)
		{// 是二楼的托盘库位
			int s_row = (storage_name % 10000) / 100;
			int s_col = storage_name % 100;
			// 标记
			storages[s_row][s_col] ++;
		}
	}

	if (status == "EMPTY")
	{// 找EMPTY代表入库放货
		// 找到最深的EMPTY库位（左上角）
		int c_row[col + 1] = {};
		for (int i = 1; i <= col; i++)
		{// 从第一列开始查
			for (int j = 1; j <= row; j++)
			{ // 第一行
				if (storages[j][i] != 0)
				{// 从下往上找符合条件的库位
					c_row[i] = j;
					//break;
				}
				else
				{// 有货,以上的不能放
					break;
				}
			}
		}
		// 深度
		int max = 0;
		// 最深库位的列
		int dep_col = 1;
		for (int i = 1; i <= col; i++)
		{
			if (max < c_row[i])
			{
				max = c_row[i];
				dep_col = i;
			}			
		}
		if (max != 0)
		{
			storage = createFloor2StorageName(max, dep_col);
		}
	}
	else if (status == "TRAY")
	{// 找托盘代表出库
		// 找到最浅的标记（右下角）
		for (int i = 1; i <= row; i++)
		{
			for (int j = col; j >= 1; j--)
			{
				if (storages[i][j] != 0)
				{
					storage = createFloor2StorageName(i, j);
					break;
				}
			}
			if (storage != "")
			{
				break;
			}
		}
	}
	return storage == "" ? false : true;
}

/// <summary>
/// Creates the name of the floor2 storage.
/// </summary>
/// <param name="row">The row.</param>
/// <param name="col">The col.</param>
/// <returns></returns>
/// Creator:MeiXu
/// CreateDate:2021/1/21
/// 
std::string Event_Handler::createFloor2StorageName(int row, int col) {
	return "0" + std::to_string(205010000 + row * 100 + col);
}

bool Event_Handler::ForbidPathByReserveStorage(std::string task_from_, std::string task_to_, std::string &ErrorReturn /*= std::string("")*/)
{
	static std::map<std::string, std::vector<std::string>> storages_no_impact = {
	{"F9WC-1-2-G-W", { "F9WC-1-2-G", "F9WC-1-2-G-W-D"}},
	{"F9WC-2-G-W", {"F9WC-2-G", "F9WC-2-G-W-D"}},
	{"F9WC-2-P-W", {"F9WC-2-P", "F9WC-2-P-W-D", "AB-RELEASE", "P1"}},
	{"F9WC-3-2-P-W", {"F9WC-3-2-P", "F9WC-3-2-P-W-D", "AB-RELEASE", "P1"}},
	{"F9WC-3-2-G-W", { "F9WC-3-2-G", "F9WC-3-2-G-W-D"}},
	{"F9WC-4-G-W", { "F9WC-4-G", "F9WC-4-G-W-D"}},
	{"F9WC-4-P-W", { "F9WC-4-P", "F9WC-4-P-W-D", "P2"}},
	{"F9WC-5-2-P-W", { "F9WC-5-2-P", "F9WC-5-2-P-W-D"}},
	{"F9WC-5-2-G-W", { "F9WC-5-2-G", "F9WC-5-2-G-W-D"}},
	{"F9WC-6-G-W", { "F9WC-6-G", "F9WC-6-G-W-D"}},
	{"F9WC-8-G-W", { "F9WC-8-G", "F9WC-8-G-W-D"}},
	{"F9WC-9-2-P-W", { "F9WC-9-2-P", "F9WC-9-2-P-W-D"}},
	{"F9WC-9-2-G-W", { "F9WC-9-2-G", "F9WC-9-2-G-W-D"}},
	{"F9WC-10-G-W", { "F9WC-10-G", "F9WC-10-G-W-D"}},
	{"AB-CONFIRM", {"P2"}},
	{"P2", {"AB-CONFIRM", "F9WC-4-P-W"}},
	{"F9WC-4-P-W", {"F9WC-4-P", "F9WC-4-P-W-D", "P2", "AB-CONFIRM"}},
	};// 指定起点,哪些目的地不影响任务继续
	static std::map<std::string, std::vector<std::string>> forbid_storage = {
	{"F9WC-1-2-G-W", { "A5", "A6", "A7", "A8", "A9"}},
	{"F9WC-2-G-W", { "A9", "A10", "A11", "A12", "A13"}},
	{"F9WC-2-P-W", { "B1", "B2", "B3", "B4"}},
	{"F9WC-3-2-P-W", { "B1", "B2", "B3", "B4"}},
	{"F9WC-3-2-G-W", { "B10", "B11", "B12", "B13", "B14"}},
	{"F9WC-4-G-W", { "B14", "B15", "B16"}},
	{"F9WC-4-P-W", { "C1"}},
	{"F9WC-5-2-P-W", { "C2", "C3", "C4", "C5", "C6"}},
	{"F9WC-5-2-G-W", { "C10", "C11", "C12", "C13", "C14"}},
	{"F9WC-6-G-W", { "C13", "C14", "C15", "C16"}},
	{"F9WC-8-G-W", { "D1", "D2"}},
	{"F9WC-9-2-P-W", { "D2","D3", "D4", "D5"}},
	{"F9WC-9-2-G-W", { "D9", "D10", "D11", "D12"}},
	{"F9WC-10-G-W", { "D11", "D12", "D13", "D14"}},
	{"AB-CONFIRM", {"C1"}},
	{"P2", {"C1"}},
	{"F9WC-4-P-W", {"C1"}},
	};// 哪些库位有车禁止任务
	static std::map<std::string, std::vector<std::string>> target_storages_no_impact = {
	{ "AB-RELEASE", {"F9WC-3-2-P-W", "F9WC-2-P-W", "B2"}},
	{ "F9WC-3-2-P-W", {"AB-RELEASE", "F9WC-2-P-W", "B2"}},
	{ "F9WC-2-P-W", {"AB-RELEASE", "F9WC-3-2-P-W", "B2"}},
	// C 区
	{"F9WC-5-2-P-W", {"F9WC-5-2-P", "C3", "C4", "C5", "C6"}},
	{"F9WC-5-2-G-W", {"F9WC-5-2-G", "C10", "C11", "C12"}},
	{"F9WC-6-G-W", {"C15", "C16", "F9WC-6-G"}},
	{"F9WC-6-P-W", {"P5", "F9WC-6-P", "C17", "C18"}},
	{"P5", {"F9WC-6-P-W", "F9WC-6-P-W-D", "C17", "C18"}},
	{"F9WC-7-2-P-W", {"F9WC-7-2-P", "C19", "C20", "C21", "C22"}},
	{"F9WC-7-2-G-W", {"F9WC-7-2-G", "C25", "C26", "C27", "C28"}},
	};// 指定终点,哪些起点不影响任务继续
	static std::map<std::string, std::vector<std::string>> target_forbid_storage = {
	{"AB-RELEASE", {"B2"}},
	{"F9WC-3-2-P-W", {"B2"}},
	{"F9WC-2-P-W", {"B2"}},

	// C 区
	{"F9WC-5-2-P-W", {"C3", "C4", "C5", "C6"}},
	{"F9WC-5-2-G-W", {"C10", "C11", "C12"}},
	{"F9WC-6-G-W", {"C15", "C16"}},
	{"F9WC-6-P-W", {"C17", "C18"}},
	{"P5", {"C17", "C18"}},
	{"F9WC-7-2-P-W", {"C19", "C20", "C21", "C22"}},
	{"F9WC-7-2-G-W", {"C25", "C26", "C27", "C28"}},
	};// 哪些库位有车,禁止任务
	if (forbid_storage.find(task_from_) != forbid_storage.end() )
	{
		if (std::find(storages_no_impact[task_from_].begin(), storages_no_impact[task_from_].end(), task_to_) == storages_no_impact[task_from_].end())
		{
			// 目的地不在“不影响库位列表”
			// 获取所有库位信息
			std::map<int, Storage> storage_map;
			WCS_CON_->getStorageList(storage_map, "s_storage_station_status");
			for each (auto var in storage_map)
			{
				if (std::find(forbid_storage[task_from_].begin(), forbid_storage[task_from_].end(), var.second.Storage_Name_) != forbid_storage[task_from_].end())
				{
					// 当前库位在“禁止有车的库位列表”中
					if (var.second.Resource_status_ == "RESERVE")
					{
						// 有车,禁止通行
						static int a = 0;
						a++;
						if (a == 100)
						{
							a = 0;
						}
						if (a == 5)
						{
							log_error("from " + task_from_ + " to storage " + task_to_ + ", but has agv in " + var.second.Storage_Name_);
						}
						ErrorReturn = var.second.Storage_Name_;
						return true;
					}
				}
			}
		}
	}
	if (target_forbid_storage.find(task_to_) != target_forbid_storage.end())
	{
		if (std::find(target_storages_no_impact[task_to_].begin(), target_storages_no_impact[task_to_].end(), task_from_) == target_storages_no_impact[task_to_].end())
		{
			// 起点不在“不影响库位列表”
			std::map<int, Storage> storage_map;
			WCS_CON_->getStorageList(storage_map, "s_storage_station_status");
			for each (auto var in storage_map)
			{
				if (std::find(target_forbid_storage[task_to_].begin(), target_forbid_storage[task_to_].end(), var.second.Storage_Name_) != target_forbid_storage[task_to_].end())
				{
					// 当前库位在“禁止有车的库位列表”中
					if (var.second.Resource_status_ == "RESERVE")
					{
						// 有车,禁止通行
						static int b = 0;
						b++;
						if (b == 100)
						{
							b = 0;
						}
						if (b == 5)
						{
							log_error("from " + task_from_ + " to storage " + task_to_ + ", but has agv in " + var.second.Storage_Name_);
						}
						ErrorReturn = var.second.Storage_Name_;
						return true;
					}
				}
			}
		}
	}
	return false;
}