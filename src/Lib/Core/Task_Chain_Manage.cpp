#include <iostream>
#include <sstream>
#include "ADODatabase/WCS_Database.h"
#include "Core/Task_Chain_Manage.h"  
#include "Core/Event_Handler.h"
#include "comm/simplelog.h"
#include "time.h"
#include "Manage/Confirm_Manage.h"
#include "Manage/Config_Manage.h"
#include "Manage/PreSearch_Manage.h"
#include "Manage/Area_Manage.h"
#include "ApiClient.h"
#include "core/charge_task.h"


Task_Chain_Manage::Task_Chain_Manage()
{

}

Task_Chain_Manage::~Task_Chain_Manage()
{

}

void Task_Chain_Manage::Initial()
{
	Task_ID_ = 1;

	now_parking_ = 0;

	WCS_DB_ = DATABASE_MANAGE.Get_WCS_DB();

	Task_ID_ = WCS_DB_->getMaxTaskId();
}

int Task_Chain_Manage::Generate_New_Sub_Task_ID()
{
	Task_ID_++;
	return Task_ID_;
}

bool Task_Chain_Manage::Create_Task_Chain(std::string order_id, std::string start, std::string target, std::string mode, std::string type, AGV* associate_AGV, std::string palletno)
{
	Order_ID_ = order_id;

	int index = Full_Task_List_.size();

	Task_Chain* New_Task = new Task_Chain(WCS_DB_);

	TASK_CHAIN_STATUS result = New_Task->Task_Chain_Initial(index, Order_ID_, start, target, mode, type, associate_AGV, Task_ID_, palletno);

	switch (result)
	{
	case TASK_CHAIN_STATUS::BEGIN:
		Add_to_active_task_list(New_Task);
		//	Add_to_full_task_list(New_Task);

		return true;
		break;
	case TASK_CHAIN_STATUS::ERR:
		delete(New_Task);
		New_Task = nullptr;
		return false;
		break;
	case TASK_CHAIN_STATUS::BLOCKED:
		delete(New_Task);
		New_Task = nullptr;
		return false;
		break;
	}

	return false;
}

bool Task_Chain_Manage::Create_Parking_Task_Chain(AGV* associate_AGV)
{
	std::string enter_time = getStartTime();

	WCS_DB_->addParkingChargingOrder(associate_AGV->AGV_In_Station_, associate_AGV->Parking_Station_, "PARKING", associate_AGV->AGV_ID_);

	int index = Full_Task_List_.size();

	Task_Chain* New_Task = new Task_Chain(WCS_DB_);

	TASK_CHAIN_STATUS result = New_Task->Parking_Initial(index, associate_AGV);

	switch (result)
	{
	case TASK_CHAIN_STATUS::BEGIN:
		Add_to_active_task_list(New_Task);
		//Add_to_full_task_list(New_Task);
		return true;
		break;
	case TASK_CHAIN_STATUS::ERR:
		//	Add_to_inactive_task_list(New_Task);
		//	Add_to_full_task_list(New_Task);
		return false;
		break;
	case TASK_CHAIN_STATUS::BLOCKED:
		delete(New_Task);
		New_Task = nullptr;
		return false;
		break;
	}

	return false;
}

bool Task_Chain_Manage::createForceParkingTaskChain(AGV* associate_AGV)
{
	std::string enter_time = getStartTime();

	WCS_DB_->addParkingChargingOrder(associate_AGV->AGV_In_Station_, associate_AGV->Parking_Station_, "PARKING", associate_AGV->AGV_ID_);

	int index = Full_Task_List_.size();

	Task_Chain* New_Task = new Task_Chain(WCS_DB_);

	TASK_CHAIN_STATUS result = New_Task->forceParkingInitial(index, associate_AGV);

	switch (result)
	{
	case TASK_CHAIN_STATUS::BEGIN:
		Add_to_active_task_list(New_Task);
		//	Add_to_full_task_list(New_Task);
		return true;
		break;
	case TASK_CHAIN_STATUS::ERR:
		//	Add_to_inactive_task_list(New_Task);
		//	Add_to_full_task_list(New_Task);
		return false;
		break;
	case TASK_CHAIN_STATUS::BLOCKED:
		delete(New_Task);
		New_Task = nullptr;
		return false;
		break;
	}

	return false;
}

bool Task_Chain_Manage::Create_Charging_Task_Chain(AGV* associate_AGV)
{
	std::string enter_time = getStartTime();
	WCS_DB_->addParkingChargingOrder(associate_AGV->AGV_In_Station_, associate_AGV->Parking_Station_, "CHARGING", associate_AGV->AGV_ID_);

	int index = Full_Task_List_.size();

	Task_Chain* New_Task = new Task_Chain(WCS_DB_);

	TASK_CHAIN_STATUS result = New_Task->Charging_Initial(index, associate_AGV);

	switch (result)
	{
	case TASK_CHAIN_STATUS::BEGIN:
		Add_to_active_task_list(New_Task);
		//Add_to_full_task_list(New_Task);
		return true;
		break;
	case TASK_CHAIN_STATUS::ERR:
		//Add_to_inactive_task_list(New_Task);
		//Add_to_full_task_list(New_Task);
		return false;
		break;
	case TASK_CHAIN_STATUS::BLOCKED:
		delete(New_Task);
		New_Task = nullptr;
		return false;
		break;
	}

	return false;
}

void Task_Chain_Manage::Add_to_full_task_list(Task_Chain* the_task)
{
	Full_Task_List_.push_back(the_task);
}

void Task_Chain_Manage::Add_to_active_task_list(Task_Chain* the_task)
{
	Active_Task_List_.push_back(the_task);
}

void Task_Chain_Manage::Add_to_inactive_task_list(Task_Chain* the_task)
{
	Inactive_Task_List_.push_back(the_task);
}

bool Task_Chain_Manage::Stop_Charging(int agv_id) {
	for each (auto var in Active_Task_List_)
	{
		Task_Chain* current_task = var;
		if (current_task->getMode() == "CHARGING" && current_task->Associate_AGV_->AGV_ID_ == agv_id)
		{
			if (current_task->Get_Status() == TASK_CHAIN_STATUS::BEGIN)
			{
				current_task->Set_Status(TASK_CHAIN_STATUS::OVER);
				return true;
			}
		}
	}
	return false;
}

void Task_Chain_Manage::Task_Status_Check_Event()
{
	// 计算各区域正在执行的AGV数量
	//agv_in_area.clear();

	std::vector<Task_Chain*>::iterator it;
	std::string x_confirm;
	CONFIG_MANAGE.readConfig("X-CONFIRM", &x_confirm, "config.txt", true);
	for (it = Active_Task_List_.begin(); it != Active_Task_List_.end(); )
	{
		
		Task_Chain* current_task = *it;
		*current_task->Associate_AGV_ = *(AGV_MANAGE.getPointAgv(current_task->Associate_AGV_->AGV_ID_));
		AGV* current_agv = current_task->Associate_AGV_;
		//std::string dispatch_mode;
		//CONFIG_MANAGE.readConfig("DispatchMode", &dispatch_mode, "config.txt", true);
		if (current_task->getType() == "CARRY" || current_task->getType() == "PARKING")
		{
			AGV_MANAGE.Set_Lock(current_agv->AGV_ID_);
			AGV_MANAGE.setBusy(current_agv->AGV_ID_);
		}
		Sub_Task *Current_Sub_Task;
		if (!current_task->Sub_Task_Chain_.empty())
		{
			Current_Sub_Task = current_task->Sub_Task_Chain_.front();
		}
		else
		{
			//std::cout<<"Sub_Task_Chain_ IS Empty!!!"<<std::endl;
			break;
		}
		current_task->dispatch_status = false;
		//状态判断;
		//把begin变成to_get,to_put,parking,is_charging;
		//把     变成mid;
		//把     变成over;
		switch (current_task->Get_Status())
		{

		case TASK_CHAIN_STATUS::BEGIN:
		{
			
			current_task->Check_Feedback(Current_Sub_Task);

			if (Current_Sub_Task->task_mode_ == "CHARGING")
			{
				if (EVENT_HANDLER.Sub_Task_Dispatch_Event(Current_Sub_Task, current_task, false))
				{
					current_task->Set_Status(TASK_CHAIN_STATUS::IS_CHARGING);
				}
			}
			else if (Current_Sub_Task->task_mode_ == "PARKING" || Current_Sub_Task->task_mode_ == "FOCE_PARKING")
			{
				if (!current_task->getTargeted().empty() && current_task->getTargeted()[0] == 'P')
				{
					current_task->Set_Status(TASK_CHAIN_STATUS::PARKING);
				}
				else
				{
					// 停车任务的开头不是P
				}
			}
#if 0
			else if (Current_Sub_Task->task_mode_ == "FOCE_PARKING")
			{
				ORDER_MANAGE.releaseParkingStation(current_agv->AGV_In_Station_);
				current_task->Set_Status(TASK_CHAIN_STATUS::PARKING);
			}
#endif
			else if (Current_Sub_Task->task_mode_ == "IN"|| Current_Sub_Task->task_mode_ == "OUT" || Current_Sub_Task->task_mode_ == "MOVE")
			{
				current_task->Set_Status(EQUIP_GET_CONFIRM);

			}
			if (current_task->Get_Status() != TASK_CHAIN_STATUS::BEGIN)
			{
				log_info("agv id " + std::to_string(current_agv->AGV_ID_) + ", at " + current_agv->AGV_In_Station_ + ", mode =" + current_task->getMode() + ",start=" + current_task->getStart() + ", target=" + current_task->getTargeted());
			}
			it++;
			break;
		}
#pragma region get and confirm

		case TASK_CHAIN_STATUS::EQUIP_GET_CONFIRM:
		{
			Current_Sub_Task = current_task->Move_To_Next_Sub_Task(Current_Sub_Task, "EQUIP_GET_CONFIRM");

			if (Current_Sub_Task && Current_Sub_Task->carry_type_ == "EQUIP_GET_CONFIRM")
			{
				if (EVENT_HANDLER.Sub_Task_Dispatch_Event(Current_Sub_Task, current_task))
				{
					if (current_agv->AGV_In_Station_ == "P1" || current_agv->AGV_In_Station_ == "P2")
					{
						ORDER_MANAGE.releaseParkingStation(current_agv->AGV_In_Station_);   //释放停车点
						current_agv->Parking_Is_Charging_ = "";
						current_agv->Parking_Station_ = "";
						std::stringstream ss;
						ss << "release Parking Station: " << current_agv->AGV_In_Station_;
						log_info_color(log_color::red, ss.str().c_str());
					}

					current_task->Set_Status(TASK_CHAIN_STATUS::EQUIP_GET_CONFIRM_CHECK);
					//AGV在接到任务后就将库位置空
					std::string start_area = get_storage_area(current_task->getStart());
					std::string target_area = get_storage_area(current_task->getTargeted());
					std::string agv_in_station = get_storage_area(current_agv->AGV_In_Station_);
					//如果当前AGV不在某一区域内,不需要进行区域释放
					if (agv_in_station != "")
					{
						//如果工单取货点与当前所在区域 或 放货点与当前所在区域 相同，也不需要释放，继续占用
						if (start_area != agv_in_station && target_area != agv_in_station)
						{
							WCS_DB_->setDatabaseCurrentComfirmStatus(agv_in_station, "IDLE");
						}
					}
				}
			}
			it++;
			break;
		}
		case TASK_CHAIN_STATUS::EQUIP_GET_CONFIRM_CHECK:
		{
			//在PARKING里面判断子任务的状态;
			current_task->Check_Feedback(Current_Sub_Task);

			if (Current_Sub_Task->sub_task_status_ == DONE)
			{
				current_task->Set_Status(TASK_CHAIN_STATUS::EQUIP_GET);
			}
			else if (Current_Sub_Task->sub_task_status_ == REJECTED || Current_Sub_Task->sub_task_status_ == FAILED)
			{
				current_task->Set_Status(ERR);
			}
			else if (Current_Sub_Task->sub_task_status_ == ABORTED)
			{
				current_task->Set_Status(ABORT);
			}
			it++;
			break;
		}
		case TASK_CHAIN_STATUS::EQUIP_GET:
		{
			Current_Sub_Task = current_task->Move_To_Next_Sub_Task(Current_Sub_Task, "EQUIP_GET");

			//DLA是WMS入库口，DLK是WMS出库口

			//2.1询问能否取放货 【1：取货】
			if (API_CLIENT.QueryIsCanGetOrPut(current_task->Order_ID_, current_task->getStart(), "1"))//本地测试暂时关掉
			//if (true)
			{
				if (Current_Sub_Task && Current_Sub_Task->carry_type_ == "EQUIP_GET")
				{
					if (EVENT_HANDLER.Sub_Task_Dispatch_Event(Current_Sub_Task, current_task))
					{
						current_task->Set_Status(TASK_CHAIN_STATUS::EQUIP_GET_CHECK);
					}
				}
			}
			it++;
			break;
		}

		case TASK_CHAIN_STATUS::EQUIP_GET_CHECK:
		{
			//在PARKING里面判断子任务的状态;
			current_task->Check_Feedback(Current_Sub_Task);

			if (Current_Sub_Task->sub_task_status_ == DONE)
			{
				//2.2上报动作完成 【1：取货完成】//本地测试暂时关掉
				API_CLIENT.TaskFinishReport(current_task->Order_ID_, std::to_string(Current_Sub_Task->agv_id_), current_task->getTargeted(), "1");
				current_task->Set_Status(TASK_CHAIN_STATUS::EQUIP_PUT_CONFIRM);
			}
			else if (Current_Sub_Task->sub_task_status_ == REJECTED || Current_Sub_Task->sub_task_status_ == FAILED)
			{
				//2.2上报动作完成 【9：上报取货任务失败】
				API_CLIENT.TaskFinishReport(current_task->Order_ID_, std::to_string(Current_Sub_Task->agv_id_), current_task->getStart(), "8");
				current_task->Set_Status(ERR);
			}
			else if (Current_Sub_Task->sub_task_status_ == ABORTED)
			{
				current_task->Set_Status(ABORT);
			}
			it++;
			break;
		}
#pragma endregion

#pragma region put and confirm
		case TASK_CHAIN_STATUS::EQUIP_PUT_CONFIRM:
		{
			Current_Sub_Task = current_task->Move_To_Next_Sub_Task(Current_Sub_Task, "EQUIP_PUT_CONFIRM");
			
			if (Current_Sub_Task && Current_Sub_Task->carry_type_ == "EQUIP_PUT_CONFIRM")
			{
				if (EVENT_HANDLER.Sub_Task_Dispatch_Event(Current_Sub_Task, current_task))
				{
					current_task->Set_Status(TASK_CHAIN_STATUS::EQUIP_PUT_CONFIRM_CHECK);

					if (current_task->getStart() != "DLL")
					{
						std::string start_area = get_storage_area(current_task->getStart());
						if (start_area != "" && !WCS_DB_->getDatabaseCurrentComfirmStatus(start_area))
						{
							WCS_DB_->setDatabaseCurrentComfirmStatus(start_area, "IDLE");
						}
					}
				}
			}
			it++;
			break;
		}

		case TASK_CHAIN_STATUS::EQUIP_PUT_CONFIRM_CHECK:
		{
			//在PARKING里面判断子任务的状态;
			current_task->Check_Feedback(Current_Sub_Task);

			if (Current_Sub_Task->sub_task_status_ == DONE)
			{
				current_task->Set_Status(TASK_CHAIN_STATUS::EQUIP_PUT);
			}
			else if (Current_Sub_Task->sub_task_status_ == REJECTED || Current_Sub_Task->sub_task_status_ == FAILED)
			{
				current_task->Set_Status(ERR);
			}
			else if (Current_Sub_Task->sub_task_status_ == ABORTED)
			{
				current_task->Set_Status(ABORT);
			}
			it++;
			break;
		}
		case TASK_CHAIN_STATUS::EQUIP_PUT:
		{
			Current_Sub_Task = current_task->Move_To_Next_Sub_Task(Current_Sub_Task, "EQUIP_PUT");

			//2.1询问能否取放货 【2：放货】
			if (API_CLIENT.QueryIsCanGetOrPut(current_task->Order_ID_, current_task->getTargeted(), "2"))//本地测试暂时关掉
			//if (true)
			{
				if (Current_Sub_Task && Current_Sub_Task->carry_type_ == "EQUIP_PUT")
				{
					if (EVENT_HANDLER.Sub_Task_Dispatch_Event(Current_Sub_Task, current_task))
					{
						current_task->Set_Status(TASK_CHAIN_STATUS::EQUIP_PUT_CHECK);

						if (current_task->getStart() != "DLL")
						{
							std::string start_area = get_storage_area(current_task->getStart());
							if (start_area != "" && !WCS_DB_->getDatabaseCurrentComfirmStatus(start_area))
							{
								WCS_DB_->setDatabaseCurrentComfirmStatus(start_area, "IDLE");
							}
						}
					}
				}
			}
			it++;
			break;
		}
		case TASK_CHAIN_STATUS::EQUIP_PUT_CHECK:
		{
			//在PARKING里面判断子任务的状态;
			current_task->Check_Feedback(Current_Sub_Task);

			if (Current_Sub_Task->sub_task_status_ == DONE)
			{
				//2.2上报动作完成 【2：放货完成】//本地测试暂时关掉
				API_CLIENT.TaskFinishReport(current_task->Order_ID_, std::to_string(Current_Sub_Task->agv_id_), current_task->getTargeted(), "2");
				current_task->Set_Status(OVER);
			}
			else if (Current_Sub_Task->sub_task_status_ == REJECTED || Current_Sub_Task->sub_task_status_ == FAILED)
			{
				//2.2上报动作完成 【9：上报放货任务失败】
				API_CLIENT.TaskFinishReport(current_task->Order_ID_, std::to_string(Current_Sub_Task->agv_id_), current_task->getTargeted(), "9");
				current_task->Set_Status(ERR);
			}
			else if (Current_Sub_Task->sub_task_status_ == ABORTED)
			{
				current_task->Set_Status(ABORT);
			}
			it++;
			break;
		}
		
#pragma endregion

#pragma region wait area
		case TASK_CHAIN_STATUS::WAIT_AREA:
		{
			Current_Sub_Task = current_task->Move_To_Next_Sub_Task(Current_Sub_Task, "WAIT_AREA");
			if (current_agv->AGV_In_Station_ == "P1" || current_agv->AGV_In_Station_ == "P2")
			{
				ORDER_MANAGE.releaseParkingStation(current_agv->AGV_In_Station_);   //释放停车点
				current_agv->Parking_Is_Charging_ = "";
				current_agv->Parking_Station_ = "";
				std::stringstream ss;
				ss << "release Parking Station: " << current_agv->AGV_In_Station_;
				log_info_color(log_color::red, ss.str().c_str());
			}
			if (Current_Sub_Task && Current_Sub_Task->carry_type_ == "WAIT_AREA")
			{
				if (EVENT_HANDLER.Sub_Task_Dispatch_Event(Current_Sub_Task, current_task))
				{
					current_task->Set_Status(TASK_CHAIN_STATUS::WAIT_AREA_CHECK);
					std::string from_area = get_storage_area(Current_Sub_Task->task_from_);
					if (from_area != "" && !WCS_DB_->getDatabaseCurrentComfirmStatus(from_area))
					{
						WCS_DB_->setDatabaseCurrentComfirmStatus(from_area, "IDLE");
					}
				}
			}
			it++;
			break;
		}
		case TASK_CHAIN_STATUS::WAIT_AREA_CHECK:
		{
			//在PARKING里面判断子任务的状态;
			current_task->Check_Feedback(Current_Sub_Task);

			if (Current_Sub_Task->sub_task_status_ == DONE)
			{
				current_task->Set_Status(TASK_CHAIN_STATUS::EQUIP_GET);
			}
			else if (Current_Sub_Task->sub_task_status_ == REJECTED || Current_Sub_Task->sub_task_status_ == FAILED)
			{
				current_task->Set_Status(ERR);
			}
			else if (Current_Sub_Task->sub_task_status_ == ABORTED)
			{
				current_task->Set_Status(ABORT);
			}
			it++;
			break;
		}
#pragma endregion

#pragma region pard and charge
		case TASK_CHAIN_STATUS::PARKING:
		{
			Current_Sub_Task = current_task->Move_To_Next_Sub_Task(Current_Sub_Task, "PARKING");

			if (Current_Sub_Task->carry_type_ == "PARKING")
			{
				if (EVENT_HANDLER.Sub_Task_Dispatch_Event(Current_Sub_Task, current_task))
				{
					if (current_agv->AGV_In_Station_ == "P1" || current_agv->AGV_In_Station_ == "P2")
					{
						ORDER_MANAGE.releaseParkingStation(current_agv->AGV_In_Station_);   //释放停车点
						current_agv->Parking_Is_Charging_ = "";
						current_agv->Parking_Station_ = "";
						std::stringstream ss;
						ss << "release Parking Station: " << current_agv->AGV_In_Station_;
						log_info_color(log_color::red, ss.str().c_str());
					}
					std::stringstream ss;
					ss << "task_chain_manager:Now parking=" << BATTERY_MANAGE.Now_Parking_;
					log_info_color(log_color::green, ss.str().c_str());
					current_task->Set_Status(TASK_CHAIN_STATUS::PARKING_CHECK);

					std::string from_area = get_storage_area(Current_Sub_Task->task_from_);
					if (from_area != "" && !WCS_DB_->getDatabaseCurrentComfirmStatus(from_area))
					{
						WCS_DB_->setDatabaseCurrentComfirmStatus(from_area, "IDLE");
					}
				}
			}
			it++;
			break;
		}

		case TASK_CHAIN_STATUS::PARKING_CHECK:
		{
			//在PARKING里面判断子任务的状态;
			current_task->Check_Feedback(Current_Sub_Task);
	
			if (Current_Sub_Task->sub_task_status_ == DONE)
			{
				current_task->Set_Status(OVER);
				current_agv->Invoke_AGV_Parking_ = INVOKE_STATUS::STOP_INVOKE;
			}
			else if (Current_Sub_Task->sub_task_status_ == REJECTED || Current_Sub_Task->sub_task_status_ == FAILED)
			{
				current_task->Set_Status(ERR);
			}
			else if (Current_Sub_Task->sub_task_status_ == ABORTED)
			{
				current_task->Set_Status(ABORT);
			}
			it++;
			break;
		}

		case TASK_CHAIN_STATUS::IS_CHARGING:
		{
			//在Charging里面判断子任务的状态;
			current_task->Check_Feedback(Current_Sub_Task);
			if (find_charge_task_state(current_task->Associate_AGV_->AGV_ID_))
			{
				current_task->Set_Status(OVER);
			}
			else if (Current_Sub_Task->sub_task_status_ == DONE)
			{
				current_task->Set_Status(OVER);
			}
			else if (Current_Sub_Task->sub_task_status_ == CHARGING)
			{
				if (current_agv->Is_Charging_ != CHARGING_STATUS::CHARGING_PAUSE)
				{
					current_agv->Is_Charging_ = CHARGING_STATUS::CHANRING_CHARGING;
				}
			}
			else if (Current_Sub_Task->sub_task_status_ == REJECTED || Current_Sub_Task->sub_task_status_ == FAILED)
			{
				current_agv->Is_Charging_ = CHARGING_STATUS::CHANRING_FAILED;
				current_task->Set_Status(ERR);

				std::stringstream ss;
				ss << current_task->Task_ID_ << ":CHANRING FAILED !";
				log_info_color(log_color::purple, ss.str().c_str());
			}
			else if (Current_Sub_Task->sub_task_status_ == ABORTED)
			{
				current_agv->Is_Charging_ = CHARGING_STATUS::CHANRING_FAILED;
				current_task->Set_Status(ABORT);

				std::stringstream ss;
				ss << current_task->Task_ID_ << ":CHANRING FAILED !";
				log_info_color(log_color::purple, ss.str().c_str());
			}
			it++;
			break;
		}
#pragma endregion
		case  TASK_CHAIN_STATUS::OVER:
		{
			if (current_task->getType() == "CHARGING")
			{
				current_task->Set_Over_Time_And_Duration();
				current_agv->Is_Charging_ = CHARGING_STATUS::CHANRING_STOP;

				std::stringstream ss;
				ss << current_task->Task_ID_ << ":Charging task completed !";
				log_info_color(log_color::purple, ss.str().c_str());
				for each (auto var in current_task->Sub_Task_Chain_)
				{
					delete var;
					var = nullptr;
				}
				current_task->Sub_Task_Chain_.clear();
				delete current_task;
				current_task = nullptr;
				it = Active_Task_List_.erase(it);
			}
			else if (current_task->getType() == "CARRY")
			{
				current_task->Set_Over_Time_And_Duration();
				WCS_DB_->updateOrderStatusToDone(current_task, "CARRY", "kh_report_order_info");
				ORDER_MANAGE.Delete_DONE_From_Full_Order_List(current_task->Order_ID_);
				AGV_MANAGE.setIdle(Current_Sub_Task->agv_id_);
				AGV_MANAGE.Set_Unlock(Current_Sub_Task->agv_id_);

				std::stringstream ss;
				ss << "Carry order ID:" << current_task->Order_ID_ << ",is from:" << current_task->getStart() << ",to:" << current_task->getTargeted() << " completed !";
				log_info_color(log_color::purple, ss.str().c_str());
				for each (auto var in current_task->Sub_Task_Chain_)
				{
					delete var;
					var = nullptr;
				}
				current_task->Sub_Task_Chain_.clear();
				delete current_task;
				current_task = nullptr;
				it = Active_Task_List_.erase(it);
			}
			else if (current_task->getType() == "PARKING")
			{
				AGV_MANAGE.setIdle(Current_Sub_Task->agv_id_);
				AGV_MANAGE.Set_Unlock(Current_Sub_Task->agv_id_);

				std::stringstream ss;
				ss << "Parking task completed !";
				log_info_color(log_color::purple, ss.str().c_str());
				for each (auto var in current_task->Sub_Task_Chain_)
				{
					delete var;
					var = nullptr;
				}
				current_task->Sub_Task_Chain_.clear();
				delete current_task;
				current_task = nullptr;
				it = Active_Task_List_.erase(it);
			}
			else
			{
				std::stringstream ss;
				ss << Current_Sub_Task->task_id_ << ":Abnormal  task finish !";
				log_info_color(log_color::red, ss.str().c_str());
				for each (auto var in current_task->Sub_Task_Chain_)
				{
					delete var;
					var = nullptr;
				}
				current_task->Sub_Task_Chain_.clear();
				delete current_task;
				current_task = nullptr;
				it = Active_Task_List_.erase(it);
			}
			break;
		}

		case  TASK_CHAIN_STATUS::ABORT:
		{

			AGV_MANAGE.setAgvMsg(current_agv->AGV_ID_, "任务被取消,请在停回停车点后解锁");

			AGV_MANAGE.setIdle(Current_Sub_Task->agv_id_);
			if (current_task->getType() == "CARRY")
			{

				WCS_DB_->updateOrderStatusToErr(current_task, current_task->getType(), "kh_report_order_info");
				WCS_DB_->redoErrStart(current_task->getStart());
#if 1 // 暂时不汇报失败
				if (current_task->getMode() == "GOODS")
				{
					RABBITMQ_SERVICE.ReportOrderStatus(current_task->Order_ID_, AGV_RESPONSE_WC::AGV_RESPONSE_FAILED, current_agv->AGV_ID_, current_task->getPalletno(), current_task->getGoodsPut());//满轮任务失败,上报MES
				}
				else if (current_task->getMode() == "TRAY")
				{
					RABBITMQ_SERVICE.ReportOrderStatus(current_task->Order_ID_, AGV_RESPONSE_WC::AGV_RESPONSE_FAILED, current_agv->AGV_ID_, "", current_task->getTrayGet());//空轮任务失败,上报MES
				}

#endif
				PRESEARCH_MANAGE.removePreSearchTaskResult(current_agv->AGV_ID_);
				WCS_DB_->unlockGoingTaskStorage("s_storage_station_status", "IDLE", current_task->getTargeted());
				WCS_DB_->unlockGoingTaskStorage("s_storage_station_status", "IDLE", current_task->getExitTrayPut());//释放放回的缓存位
				WCS_DB_->unlockGoingTaskStorage("s_storage_station_status", "IDLE", current_task->getTrayGet(true));//释放之前取空轮缓存位时预定的库位
				WCS_DB_->unlockGoingTaskStorage("s_storage_station_status", "IDLE", current_task->getGoodsPut());//释放取满轮的预定库位

				std::stringstream ss2;
				ss2 << "RABBITMQ_SERVICE:ReportOrderStatus" << ",Task ID:" << current_task->Order_ID_ << ",STAUTS:" << AGV_RESPONSE_WC::AGV_RESPONSE_FAILED << ",AGV_ID:" << current_agv->AGV_ID_
					<< ",PALLETNO:" << current_task->getPalletno() << ",Location:" << current_task->getGoodsPut();
				log_info_color(log_color::purple, ss2.str().c_str());

			}
			if (current_task->getType() == "CHARGING")
			{
				AGV_MANAGE.stopCharging(current_agv);
			}
			std::stringstream ss;
			ss << current_task->Task_ID_ << ":ABORTED Task !!!";
			log_info_color(log_color::blue, ss.str().c_str());



			delete current_task;
			delete Current_Sub_Task;
			it = Active_Task_List_.erase(it);
			break;
		}

		case  TASK_CHAIN_STATUS::ERR:
		{
			//AGV_MANAGE.setAgvMsg(current_agv->AGV_ID_, "任务失败,请在停回停车点后解锁");
			if (current_task->getType() == "CARRY")
			{
				WCS_DB_->updateOrderStatusToErr(current_task, current_task->getType(), "kh_report_order_info");

				WCS_DB_->redoErrStart(current_task->getStart());
#if 0 // 暂时不汇报失败
				if (current_task->getMode() == "GOODS")
				{
					RABBITMQ_SERVICE.ReportOrderStatus(current_task->Order_ID_, AGV_RESPONSE_WC::AGV_RESPONSE_FAILED, current_agv->AGV_ID_, current_task->getPalletno(), current_task->getGoodsPut());//满轮任务失败,上报MES
				}
				else if (current_task->getMode() == "TRAY")
				{
					RABBITMQ_SERVICE.ReportOrderStatus(current_task->Order_ID_, AGV_RESPONSE_WC::AGV_RESPONSE_FAILED, current_agv->AGV_ID_, "", current_task->getTrayGet());//空轮任务失败,上报MES
				}

#endif
				PRESEARCH_MANAGE.removePreSearchTaskResult(current_agv->AGV_ID_);
				// TODO TEST
				WCS_DB_->unlockGoingTaskStorage("s_storage_station_status", "IDLE", current_task->getTargeted());
				WCS_DB_->unlockGoingTaskStorage("s_storage_station_status", "IDLE", current_task->getExitTrayPut());//释放放回的缓存位
				WCS_DB_->unlockGoingTaskStorage("s_storage_station_status", "IDLE", current_task->getTrayGet(true));//释放之前取空轮缓存位时预定的库位
				WCS_DB_->unlockGoingTaskStorage("s_storage_station_status", "IDLE", current_task->getGoodsPut());//释放取满轮的预定库位

				std::stringstream ss2;
				ss2 << "RABBITMQ_SERVICE:ReportOrderStatus" << ",Task ID:" << current_task->Order_ID_ << ",STAUTS:" << AGV_RESPONSE_WC::AGV_RESPONSE_FAILED << ",AGV_ID:" << current_agv->AGV_ID_
					<< ",PALLETNO:" << current_task->getPalletno() << ",Location:" << current_task->getGoodsPut();
				log_info_color(log_color::purple, ss2.str().c_str());

			}
			AGV_MANAGE.setIdle(Current_Sub_Task->agv_id_);

			std::stringstream ss;
			ss << current_task->Task_ID_ << ":ERR Task !!!";
			log_info_color(log_color::red, ss.str().c_str());
			for each (auto var in current_task->Sub_Task_Chain_)
			{
				delete var;
				var = nullptr;
			}
			current_task->Sub_Task_Chain_.clear();
			delete current_task;
			current_task = nullptr;
			it = Active_Task_List_.erase(it);
			break;
		}

		default:
			AGV_MANAGE.setIdle(Current_Sub_Task->agv_id_);

			std::stringstream ss;
			ss << Current_Sub_Task->task_id_ << ":TASK_CHAIN_STATUS Default !!!";
			log_info_color(log_color::blue, ss.str().c_str());
			for each (auto var in current_task->Sub_Task_Chain_)
			{
				delete var;
				var = nullptr;
			}
			current_task->Sub_Task_Chain_.clear();
			delete current_task;
			current_task = nullptr;
			it = Active_Task_List_.erase(it);
			break;
		}
		if (Current_Sub_Task && !Current_Sub_Task->dispatched)
		{
			// 子任务没有下发过
			if (Current_Sub_Task->pre_search)
			{
				// 子任务预搜过
				if (current_task && !current_task->dispatch_status)
				{
					// 子任务不满足下发条件
					// 需要清空预搜路
					Current_Sub_Task->pre_search = false;
					PRESEARCH_MANAGE.removePreSearchTaskResult(current_agv->AGV_ID_);
				}
			}
		}
	}

}

std::vector<Task_Chain*>  Task_Chain_Manage::Get_Full_Task_List()
{
	return Full_Task_List_;
}

std::vector<Task_Chain*>  Task_Chain_Manage::Get_Active_Task_List()
{
	return Active_Task_List_;
}

std::vector<Task_Chain*>  Task_Chain_Manage::Get_Inactive_Task_List()
{
	return Inactive_Task_List_;
}

void Task_Chain_Manage::Add_to_full_list_if_not_exist(int id, int state)
{
	Full_Sub_Task_List_[id] = state;
}

std::string Task_Chain_Manage::getReportPoint(std::string target_point)
{
	std::string result = "";
	std::map<std::string, std::string> ::iterator it = Report_Point_Map_.find(target_point);
	if (it != Report_Point_Map_.end())
	{
		return it->second;
	}
	else {
		return result;
	}
}

float Task_Chain_Manage::ReadNum(std::string s)
{
	unsigned int i = 0, n = 0;
	unsigned int point_index = s.find('.');
	float result = 0, under_0 = 0;//under_0存储小数部分  
	if (count(s.begin(), s.end(), '.') > 1)
	{
		return 0;//字符串里只能有1个或0个小数点,不然出错退出; 
	}
	if (s.find('.') != -1)//字符串里有小数点;
	{
		if ((point_index == 0) || (point_index == s.size() - 1))//小数点位置合理,不能在字符串第1位,且不能在最后一位;  
		{
			return 0;
		}
		for (i = 0; i <= point_index - 1; i++)//计算整数部分;  
		{
			if (s[i] >= '0'&&s[i] <= '9')
			{
				result = result * 10 + s[i] - 48;
			}
		}
		for (i = s.size() - 1; i >= point_index - 1; i--)//计算小数部分;  
		{
			if (s[i] >= '0'&&s[i] <= '9')
			{
				if (i == point_index - 1)
				{
					under_0 = under_0 * 0.1 + 0;//i=小数点前一位,under_0+0; 
				}
				else
				{
					under_0 = under_0 * 0.1 + s[i] - 48;
				}
			}
		}
		result = result + under_0;//把整数部分和小数部分相加;  
	}
	else//字符串只有整数部分;  
	{
		for (i = 0; i <= s.size() - 1; i++)
		{
			if (s[i] >= '0'&&s[i] <= '9')
			{
				result = result * 10 + s[i] - 48;
			}
		}
	}

	return result;

}

void Task_Chain_Manage::splitString2(const std::string& s, std::vector<std::string>& v, const std::string& c)
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

bool Task_Chain_Manage::Is_AGV_In_Parking_Station(AGV* current_agv)
{
	if (current_agv->AGV_In_Station_ == "P1" || current_agv->AGV_In_Station_ == "P2" || current_agv->AGV_In_Station_ == "P3" ||
		current_agv->AGV_In_Station_ == "P4" || current_agv->AGV_In_Station_ == "P5" || current_agv->AGV_In_Station_ == "P6" || current_agv->AGV_In_Station_ == "P7")
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::string Task_Chain_Manage::getStartTime()
{
	std::string order_start_time;
	//struct tm *tm_now;
	//time_t start_time;
	//time(&start_time);
	//tm_now = localtime_s(&start_time);


	time_t time_seconds = time(nullptr);
	struct tm now_time;
	localtime_s(&now_time, &time_seconds);

	std::stringstream ss;
	ss << now_time.tm_year + 1900 << "-" << now_time.tm_mon + 1 << "-" << now_time.tm_mday << " " << now_time.tm_hour << ":" << now_time.tm_min << ":" << now_time.tm_sec;
	order_start_time = ss.str();
	return order_start_time;
}

void Task_Chain_Manage::cancelTaskChain(std::string order_id, int task_id)
{
	WCS_DB_->abortTask(task_id);
	std::vector<Task_Chain*>::iterator it;
	for (it = Active_Task_List_.begin(); it != Active_Task_List_.end(); ++it)
	{
		Task_Chain* current_task = *it;

		if (current_task->Task_ID_ == task_id)
		{
			if (current_task->getType() == "CHARGING")
			{
				AGV_MANAGE.stopCharging(current_task->Associate_AGV_);
			}
			current_task->Set_Status(ABORT);
		}
	}

	std::stringstream ss;
	ss << "Order ID:" << order_id << "Cancel Order Successful!";
	log_info_color(log_color::blue, ss.str().c_str());
}


std::string Task_Chain_Manage::get_storage_area(std::string storage)
{
	std::string result = "";
	if (storage == "DLA" || storage == "DLK" || storage == "DLL")
	{
		result = "WMS";
	}
	else if (storage == "DLB" || storage == "DLC")
	{
		result = "PipeSocket";
	}
	else if (storage == "DLD" || storage == "DLE" || storage == "DLF" || storage == "DLG")
	{
		result = "3160";
	}
	else if (storage == "DLH" || storage == "DLI" || storage == "DLJ")
	{
		result = "Check";
	}
	return result;
}