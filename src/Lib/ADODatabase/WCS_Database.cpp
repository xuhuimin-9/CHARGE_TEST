#ifndef no_init_all
#define no_init_all deprecated
#endif // no_init_all
#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "stdlib.h"
#include "ADODatabase/DatabaseManage.h"
#include "ADODatabase/WCS_Database.h"
#include "Manage/Order_Manage.h"
#include "Manage/Storage_Manage.h"
#include "Manage/Config_Manage.h"
#include "Manage/Parking_Manage.h"
#include "Manage/Area_Manage.h"
#include "Core/Task_Chain_Manage.h"
#include "Core/Task_Chain.h"
#include "Core/Event_Handler.h"
#include "comm/simplelog.h"
#include <Windows.h>
#include <regex>

WCS_Database::WCS_Database()
{

}

WCS_Database::~WCS_Database()
{

}

void WCS_Database::setWcsAdo(ADOcon ADO)
{
	WCS_ADO_ = ADO;
}

//linde_order_info---->kh_report_order_info

bool WCS_Database::copyOrderInfo(Task_Chain_Info &order_in)
{
	agv_in_area.clear();

	for each (auto var in EVENT_HANDLER.Executing_List_)
	{
		AGV_MANAGE.Set_Lock(var->AGV_ID);
		AGV_MANAGE.setBusy(var->AGV_ID);
	}
	for each (auto var in TASK_CHAIN_MANAGE.Active_Task_List_)
	{
		Task_Chain* current_task = var;
		*current_task->Associate_AGV_ = *(AGV_MANAGE.getPointAgv(current_task->Associate_AGV_->AGV_ID_));
		AGV* current_agv = current_task->Associate_AGV_;
	}

	std::string a;
	int row_count = 0;
	std::stringstream ss;
	std::stringstream ss2;
	char szSq1[10240] = { 0 };
	char szSq12[10240] = { 0 };
	std::vector<Task_Chain_Info> all_order;// ORDER_ID ,ORDER
	std::vector<Task_Chain_Info> start_lock_order;// ORDER_ID ,ORDER
	std::vector<Task_Chain_Info> targeted_lock_order;// ORDER_ID ,ORDER
	std::vector<Task_Chain_Info> no_lock_order;// ORDER_ID ,ORDER
	std::vector<AGV*> Free_AGV_List;
	//if (AGV_MANAGE.Get_Free_AGV(Free_AGV_List))//根据上层指派的AGV找一个空闲车
	if (AGV_MANAGE.getFreeHighBatteryAGV(Free_AGV_List,2))//找电量最高的空闲AGV
	{
		try
		{
			ss << "SELECT COUNT(*) FROM kh_order_info WHERE STATUS = 'NEW' OR STATUS = 'IDLE' LIMIT 58";
			sprintf_s(szSq1, "%s", ss.str().c_str());
			recordPtr_ = WCS_ADO_.GetRecordSet((_bstr_t)szSq1);
			if (recordPtr_)
			{
				row_count = (int)recordPtr_->GetCollect("COUNT(*)");
			}
			else
			{
				std::stringstream ss;
				ss << "Cant get Recordset!";
				log_info_color(log_color::red, ss.str().c_str());
			}
			if (row_count != 0)
			{
				ss2 << "SELECT * FROM kh_order_info WHERE STATUS = 'NEW' OR STATUS = 'IDLE' ORDER BY PRIORITY asc,ENTERDATE asc LIMIT 58";

				sprintf_s(szSq12, "%s", ss2.str().c_str());
				recordPtr_ = WCS_ADO_.GetRecordSet((_bstr_t)szSq12);

				recordPtr_->MoveFirst();
				while (recordPtr_->adoEOF == VARIANT_FALSE)
				{
					order_in.ORDER_ID_ = (std::string)((_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("ORDER_ID")))->Value);
					order_in.START_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("START"))->Value);
					order_in.TARGET_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("TARGET"))->Value);
					order_in.STATUS_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("STATUS"))->Value);
					order_in.PRIORITY_ = atoi((_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("PRIORITY"))->Value));
					order_in.MODE_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("MODE"))->Value);
					order_in.TYPE_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("TYPE"))->Value);
					order_in.PALLETNO_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("PALLETNO"))->Value);
					order_in.AGV_ID_ = atoi((_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("AGV_ID"))->Value));

					std::regex data_regex("/(\\d)(?!\\d)");
					std::regex hours_regex("\\s(\\d)(?!\\d)");// 时间的正则
					std::regex min_regex(":(\\d)(?!\\d)");// 时间的正则

					order_in.Timestamp_ = std::regex_replace((std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("ENTERDATE"))->Value), data_regex, "/0$1");
					order_in.Timestamp_ = std::regex_replace(order_in.Timestamp_, hours_regex, " 0$1");
					order_in.Timestamp_ = std::regex_replace(order_in.Timestamp_, min_regex, ":0$1");

					std::string start_area = TASK_CHAIN_MANAGE.get_storage_area(order_in.START_);
					std::string target_area = TASK_CHAIN_MANAGE.get_storage_area(order_in.TARGET_);

					bool start_status = getDatabaseCurrentComfirmStatus(start_area);
					bool targeted_status = getDatabaseCurrentComfirmStatus(target_area);
					if (start_status && !targeted_status)
					{
						targeted_lock_order.push_back(order_in);//终点被锁，但起没锁
					}
					else if (!start_status && targeted_status || (order_in.START_=="DLL" && !start_status))
					{
						start_lock_order.push_back(order_in);//起点被锁，但终点没锁
					}
					else if (start_status && targeted_status)
					{
						no_lock_order.push_back(order_in);//都没锁
					}
					//all_order.push_back(order_in);
					recordPtr_->MoveNext();
						
				}
			}
			// 此时，all_order只剩下状态为NEW，且可以执行的任务
			for each (auto it in Free_AGV_List)
			{
				auto current_agv = *it;
				AGV* parking_agv = new AGV;
				*parking_agv = current_agv;
				if (BATTERY_MANAGE.Check_Low_Power(parking_agv))
				{
					log_info(std::to_string(current_agv.AGV_ID_) + " low power parking");
					agvErrMsg(current_agv.AGV_ID_, "low power parking");
					if (!PARKING_MANAGE.Auto_Parking_System(parking_agv))
					{
						delete parking_agv;
						parking_agv = nullptr;
					}
					continue;
				}
				else if (parking_agv->Invoke_AGV_Parking_ == INVOKE_STATUS::BEGING_INVOKE)
				{
					AGV_MANAGE.stopInvokeParking(parking_agv->AGV_ID_);
					agvErrMsg(current_agv.AGV_ID_, "manual parking");
					log_info(std::to_string(current_agv.AGV_ID_) + " manual parking");
					if (!PARKING_MANAGE.Auto_Parking_System(parking_agv))
					{
						delete parking_agv;
						parking_agv = nullptr;
					}
					continue;
				}

				// 接工单
#if 0
				if ((!no_lock_order.empty() || !start_lock_order.empty() || !targeted_lock_order.empty()) && EVENT_HANDLER.start_run)
				{
					std::string area = "";

					//起点和终点均未被锁，直接下发
					if (!no_lock_order.empty())
					{
						for each (auto var in no_lock_order)
						{
							order_in = var;
							// 没有被管制，下发工单
							order_in.AGV_ID_ = current_agv.AGV_ID_;
							delete parking_agv;
							parking_agv = nullptr;
							return true;
						}
					}
					//起点被锁，判断起点是否被当前空闲的AGV锁住的，如果是，则下发
					else if (!start_lock_order.empty())
					{
						for each (auto var in start_lock_order)
						{
							order_in = var;
							area = TASK_CHAIN_MANAGE.get_storage_area(order_in.START_);
							if (getDatabaseCurrentComfirmStatus(area, parking_agv->AGV_ID_))
							{
								// 没有被管制，下发工单
								order_in.AGV_ID_ = current_agv.AGV_ID_;
								delete parking_agv;
								parking_agv = nullptr;
								return true;
							}
						}
					}
					//终点被锁，判断终点是否被当前空闲的AGV锁住的，如果是，则下发
					else if (!targeted_lock_order.empty())
					{
						for each (auto var in targeted_lock_order)
						{
							order_in = var;
							area = TASK_CHAIN_MANAGE.get_storage_area(order_in.TARGET_);
							if (getDatabaseCurrentComfirmStatus(area, parking_agv->AGV_ID_))
							{
								// 没有被管制，下发工单
								order_in.AGV_ID_ = current_agv.AGV_ID_;
								delete parking_agv;
								parking_agv = nullptr;
								return true;
							}
						}
					}
				}
#endif
				for each (auto var in all_order)
				{
					order_in = var;
					// 没有被管制，下发工单
					order_in.AGV_ID_ = current_agv.AGV_ID_;
					delete parking_agv;
					parking_agv = nullptr;
					return true;
				}

				// 没接到工单，停车去
				if (parking_agv->AGV_In_Station_[0] != 'P')
				{
					agvErrMsg(parking_agv->AGV_ID_, "no task dispatched, go to parking");
					if (!PARKING_MANAGE.Auto_Parking_System(parking_agv))
					{
						delete parking_agv;
						parking_agv = nullptr;
					}
				}
				else
				{
					agvErrMsg(parking_agv->AGV_ID_, "no task dispatched");
					delete parking_agv;
					parking_agv = nullptr;
				}
			}
			return false;
		}
		catch (_com_error &e) {

			std::stringstream ss;
			ss << e.ErrorMessage() << "copyOrderInfo" << e.Description();
			log_info_color(log_color::red, ss.str().c_str());
		}
	}
	return false;
}
bool WCS_Database::orderErrMsg(std::string order_id , std::string msg) {

	try {
		std::stringstream ss3;
		ss3 << "UPDATE kh_order_info SET MSG = '" << msg << "' WHERE ORDER_ID = '" << order_id << "'";
		_bstr_t SQL5 = ss3.str().c_str();
		static int i = 0;
		if (i == 210)
		{
			i = 0;
			log_info_color(log_color::red, SQL5);
		}
		return WCS_ADO_.ExecuteSQL(SQL5);

	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "orderErrMsg" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

bool WCS_Database::agvErrMsg(int agv_id, std::string msg) {

	try {

		std::stringstream ss3;
		ss3 << "UPDATE kh_agv_data SET msg = '" << msg << "' WHERE agv_id = " << agv_id;
		_bstr_t SQL5 = ss3.str().c_str();
		static int i = 0;
		if (i++ == 210)
		{
			i = 0;
			log_info_color(log_color::red, SQL5);
		}
		return WCS_ADO_.ExecuteSQL(SQL5);

	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "agvErrMsg" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

bool WCS_Database::getSameLineTray(std::vector<Task_Chain_Info> all_order , Task_Chain_Info & order_in) {
	if (order_in.MODE_ == "TRAY")
	{
		return true;
	}
	for each (auto var in all_order) {
		if (order_in.START_ == var.START_ && order_in.MODE_ != var.MODE_)
		{
			log_error(order_in.START_ + ":has goods but tray first");
			order_in = var;
			return true;
		}
	}
	return false;
}

bool WCS_Database::getOrder(std::vector<Task_Chain_Info> all_order, std::string area_id, Task_Chain_Info & order_in, std::string mode, std::string now_area /*= ""*/, bool num /*= true*/) {
	for each (auto var in all_order)
	{
		auto current_order = var;
		if (current_order.MODE_ == mode || mode == "")
		{
			Pos_Info a;
			std::string start = current_order.START_;
			if (current_order.MODE_ == "TRAY")
			{
				start += "-P";
			}
			else
			{
				start += "-G";
			}
			if (AREA_MANAGE.stationPos(start, a))
			{
				if (AREA_MANAGE.isPointInPolygon(a, area_id))
				{
					// 区域数量限制
					std::string one_agv_area = "";// 单车区域
					std::string agv_area = "";// AGV大区
					bool num_flag = false;// 数量尚未超限
					if (num)
					{
						// 大区管制
						static std::vector<std::string> num_limit_1 = {
							A_AREA, B_FAR, C_FAR, D_FAR,
						};
						std::string b_limit;
						std::string c_limit;
						std::string d_limit;
						CONFIG_MANAGE.readConfig("B_Limit", &b_limit);
						CONFIG_MANAGE.readConfig("C_Limit", &c_limit);
						CONFIG_MANAGE.readConfig("D_Limit", &d_limit);
						std::map<std::string, int> num_limit = {
							{B_AREA, std::atoi(b_limit.c_str())},
							{C_AREA, std::atoi(c_limit.c_str())},
							{D_AREA, std::atoi(d_limit.c_str())},
						};
						for each (auto var in num_limit_1)
						{
							// 仅允许单车的区域有
							if (AREA_MANAGE.isPointInPolygon(a, var))
							{
								// 在单车区域
								one_agv_area = var;
								// 可能需要跳过
								if (agv_in_area[var] < 1)
								{
									// 无任务
									switch (AREA_MANAGE.agvNumInArea(var)) {
										case 0:
										{
											// 无车
											// 可
											num_flag = false;
											agv_in_area[var] ++;
											log_info_color(purple, "[WCS]no agv in area " + var + " , try dispatch order");
											break;
										}
										case 1:
										{
											// 有车
											if (var == now_area)
											{
												// 可
												num_flag = false;
												agv_in_area[var] ++;
												log_info_color(purple, "[WCS]agv already in area " + var + " , try dispatch order");
											}
											else
											{
												// 禁止
												num_flag = true;
												log_info_color(purple, "[WCS]to much agvs already in area " + var + " : " + start);
												orderErrMsg(current_order.ORDER_ID_, "等待AGV");
											}
											break;
										}
										default: {
											num_flag = true;
											log_error("area agv num error : " + var);
											orderErrMsg(current_order.ORDER_ID_, "等待AGV");
											break;
										}
									}
								}
								else
								{
									// 禁止
									num_flag = true;
									orderErrMsg(current_order.ORDER_ID_, var + ":远端产线已分配AGV，等待中");
								}
								break;
							}
						}
						if (!num_flag)
						{
							for each (auto var in num_limit)
							{
								if (!num_flag && AREA_MANAGE.isPointInPolygon(a, var.first))
								{
									agv_area = var.first;
									// 需要跳过
									if (agv_in_area[var.first] >= var.second || now_area == var.first)
									{
										// 清除单区的允许结果
										agv_in_area[one_agv_area] --;
										orderErrMsg(current_order.ORDER_ID_, var.first + ":区域不再分配车辆");
										num_flag = true;
									}
									else
									{
										// 允许
										agv_in_area[var.first] ++;
										num_flag = false;
									}
									break;
								}
							}
						}
					}
					if (!num_flag)
					{
						order_in = current_order;
						if (order_in.START_ != "F9WC-2")
						{
							getSameLineTray(all_order, order_in);
						}
						return true;
					}
				}
			}
		}
	}
	return false;
}

std::vector<Task_Chain_Info> WCS_Database::getRecordsetOrderList(_RecordsetPtr& record_ptr) {
	std::vector<Task_Chain_Info> order_in_list;

	return order_in_list;
}

bool WCS_Database::dispatchToAgv(std::vector<Task_Chain_Info> order_in_list, Task_Chain_Info &order_in, int agv_id /*= -1*/)
{
	bool result = false;
	std::vector<AGV*> Free_AGV_List;
	if (!order_in_list.empty() )
	{
		// 存在工单,且有AGV
		for each (auto var in order_in_list)
		{
			 //= var.first;
			if (var.MODE_ == "TRAY")
			{
				std::string storage_group;
				std::string tray_type;
				std::string tray_get;
				if (getCurrentEquipToStorage(storage_group, tray_type, var.START_))//获取到当前水箱距离较近的缓存区
				{
					if (order_in.START_ == "F9WC-2")
					{
						storage_group = "B";
					}
					if (getCurrentTableStorageNameNOResource("s_storage_station_status", "FULL", tray_get, "TRAY", storage_group, tray_type))//就近找一个空轮（对应区域,对应型号）
					{
						if (AGV_MANAGE.getFreeHighBatteryAGV(Free_AGV_List, 2))//根据上层指派的AGV找一个空闲车
						{
							for (unsigned int i = 0; i < Free_AGV_List.size() && agv_id != -1; i++)
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
							order_in = var;
							order_in.AGV_ID_ = agv_id != -1 ? agv_id : (*Free_AGV_List.begin())->AGV_ID_;

							order_in.TARGET_ = tray_get;
							return true;
						}
					}
				}
			}
			else if (var.MODE_ == "GOODS")
			{
				if (AGV_MANAGE.getFreeHighBatteryAGV(Free_AGV_List, 2))//空轮任务需找一个空闲车
				{
					for (unsigned int i = 0; i < Free_AGV_List.size() && agv_id != -1; i++)
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
					order_in = var;
					order_in.AGV_ID_ = agv_id != -1 ? agv_id : (*Free_AGV_List.begin())->AGV_ID_;
					return true;
				}
			}
			else if (var.MODE_ == "IN" || var.MODE_ == "OUT")
			{
				if (AGV_MANAGE.getFreeHighBatteryAGV(Free_AGV_List, 2))//空轮任务需找一个空闲车
				{
					for (unsigned int i = 0; i < Free_AGV_List.size() && agv_id != -1; i++)
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
					order_in = var;
					order_in.AGV_ID_ = agv_id != -1 ? agv_id : (*Free_AGV_List.begin())->AGV_ID_;
					return true;
				}
			}
		}
	}
	else
	{
		return result;
	}
	return result;
}

void WCS_Database::deleteCurrentOrder(std::string order_id)
{
	try
	{
		std::stringstream ss;
		ss << "DELETE FROM kh_order_info WHERE ORDER_ID = '" << order_id << "' ";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "deleteCurrentOrder" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}

void WCS_Database::setCurrentStorageStatus(std::string table_name, std::string state, std::string storage_name)   //改变当前库位状态;
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE " << table_name << " SET storage_status = '" << state << "' WHERE storage_name = '" << storage_name << "'";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "setCurrentStorageStatus" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}
//NEW->DOING;
bool WCS_Database::updateOrderStatusToDoing(std::string order_id, int agv_id)
{
	try {
		int row_count = 0;
		std::stringstream ss;
		std::stringstream ss2;
		std::stringstream ss3;

		ss << "SELECT COUNT(*) FROM kh_report_order_info WHERE ORDER_ID = '" << order_id << "'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);

		if (recordPtr_)
		{
			row_count = (int)recordPtr_->GetCollect("COUNT(*)");
		}
		if (row_count != 0)
		{
			ss3 << "UPDATE kh_report_order_info SET STATUS = 'DOING' WHERE ORDER_ID = '" << order_id << "'";
			_bstr_t SQL5 = ss3.str().c_str();
			WCS_ADO_.ExecuteSQL(SQL5);
			return true;
		}
		else
		{
			return false;
		}

	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "updateOrderStatusToDoing" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}

}
//DOING->DONE;
void WCS_Database::updateOrderStatusToDone(Task_Chain* the_task, std::string order_type, std::string sql_table_name) {

	int row_count = 0;
	std::stringstream ss;
	std::stringstream ss2;
	std::stringstream ss3;

	try {
		ss << "SELECT COUNT(*) FROM " << sql_table_name << " WHERE ORDER_ID = '" << the_task->Order_ID_ << "' AND TYPE = '" << order_type << "'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);

		if (recordPtr_) {
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		if (row_count != 0)
		{

			ss3 << "UPDATE " << sql_table_name << " SET STATUS = 'DONE',FINISHDATE = '" << the_task->Get_Over_Time() << "' WHERE ORDER_ID = '" << the_task->Order_ID_ << "' AND TYPE = '" << order_type << "'";
			_bstr_t SQL4 = ss3.str().c_str();
			WCS_ADO_.ExecuteSQL(SQL4);

		}
		else
		{
			return;
		}
	}
	catch (_com_error &e) {

		std::stringstream ss;
		ss << e.ErrorMessage() << "updateOrderStatusToDone" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}

}

// 满轮任务,取好了,去放中
void WCS_Database::updateOrderStatusToHalfDone(Task_Chain* the_task, std::string order_type, std::string sql_table_name) {

	int row_count = 0;
	std::stringstream ss;
	std::stringstream ss2;
	std::stringstream ss3;

	try {
		ss << "SELECT COUNT(*) FROM " << sql_table_name << " WHERE ORDER_ID = '" << the_task->Order_ID_ << "' AND TYPE = '" << order_type << "'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);

		if (recordPtr_) {
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		if (row_count != 0)
		{

			ss3 << "UPDATE " << sql_table_name << " SET STATUS = 'PUT',FINISHDATE = '" << the_task->Get_Over_Time() << "' WHERE ORDER_ID = '" << the_task->Order_ID_ << "' AND TYPE = '" << order_type << "'";
			_bstr_t SQL4 = ss3.str().c_str();
			WCS_ADO_.ExecuteSQL(SQL4);

		}
		else
		{
			return;
		}
	}
	catch (_com_error &e) {

		std::stringstream ss;
		ss << e.ErrorMessage() << "updateOrderStatusToDone" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}

}

//DOING->ERR;
void WCS_Database::updateOrderStatusToErr(Task_Chain* the_task, std::string order_type, std::string sql_table_name)
{

	int row_count = 0;
	std::stringstream ss;
	std::stringstream ss2;

	ss << "SELECT COUNT(*) FROM " << sql_table_name << " WHERE STATUS = 'DOING' AND TYPE = '" << order_type << "' AND ORDER_ID = '" << the_task->Order_ID_ << "'";
	_bstr_t SQL = ss.str().c_str();
	recordPtr_ = WCS_ADO_.GetRecordSet(SQL);

	try {
		if (recordPtr_) {
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		if (row_count != 0)
		{
			ss2 << "UPDATE " << sql_table_name << " SET STATUS = 'ERR',FINISHDATE = 'null' WHERE ORDER_ID = '" << the_task->Order_ID_ << "' AND TYPE = '" << order_type << "'";
			_bstr_t SQL2 = ss2.str().c_str();
			WCS_ADO_.ExecuteSQL(SQL2);
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "updateOrderStatusToErr" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}


void WCS_Database::updateOrderTargetStorageInfo(Task_Chain* the_task, std::string order_type, std::string sql_table_name)
{

	int row_count = 0;
	std::stringstream ss;
	std::stringstream ss2;

	ss << "SELECT COUNT(*) FROM " << sql_table_name << " WHERE  TYPE = '" << order_type << "' AND ORDER_ID = '" << the_task->Order_ID_ << "'";
	_bstr_t SQL = ss.str().c_str();
	recordPtr_ = WCS_ADO_.GetRecordSet(SQL);

	try {
		if (recordPtr_) {
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		if (row_count != 0)
		{
			if (the_task->getMode()=="GOODS")
			{
				ss2 << "UPDATE " << sql_table_name << " SET TARGET = '" << the_task->getGoodsPut() << "' WHERE ORDER_ID = '" << the_task->Order_ID_ << "' AND TYPE = '" << order_type << "'";
			}
			else if (the_task->getMode() == "TRAY")
			{
				ss2 << "UPDATE " << sql_table_name << " SET TARGET = '" << the_task->getTrayGet() << "' WHERE ORDER_ID = '" << the_task->Order_ID_ << "' AND TYPE = '" << order_type << "'";
			}
			_bstr_t SQL2 = ss2.str().c_str();
			WCS_ADO_.ExecuteSQL(SQL2);
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "updateOrderTargetStorageInfo" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}

std::vector<Oreder_Task*> WCS_Database::updateTaskOrderInfo(std::vector<Oreder_Task*>Executing_List) {
	try {

		int row_count = 0;
		_bstr_t SQL = "SELECT COUNT(*) FROM kh_report_order_info WHERE STATUS = 'NEW'";
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);

		if (recordPtr_)
		{
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		if (row_count != 0)
		{
			std::string sql = "SELECT * FROM kh_report_order_info WHERE STATUS = 'NEW'";
			_bstr_t SQL = sql.c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL);

			recordPtr_->MoveFirst();
			while (recordPtr_->adoEOF == VARIANT_FALSE) {
				Oreder_Task* order_task = new Oreder_Task;
				order_task->ID = atoi(_bstr_t(recordPtr_->Fields->GetItem(_variant_t("ID"))->Value));
				order_task->ORDER_ID = atoi(_bstr_t(recordPtr_->Fields->GetItem(_variant_t("ORDER_ID"))->Value));
				order_task->AGV_ID = atoi(_bstr_t(recordPtr_->Fields->GetItem(_variant_t("AGV_ID"))->Value));
				order_task->START_POINT = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("START"))->Value);
				order_task->TARGET_POINT = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("TARGET"))->Value);
				order_task->MODE = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("MODE"))->Value);
				order_task->TYPE = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("TYPE"))->Value);

				std::vector<Oreder_Task *>::iterator ite;
				bool data = true;
				for (ite = Executing_List.begin(); ite != Executing_List.end(); ite++)       //判断当前executing_list是否有重复;
				{
					Oreder_Task* p = *ite;

					if (p->ORDER_ID == order_task->ORDER_ID || (p->START_POINT == order_task->START_POINT && p->MODE == order_task->MODE))
					{
						data = false;
						delete(order_task);
						break;
					}
					else
					{
						data = true;
						continue;
					}
				}
				if (data)
				{
					Executing_List.push_back(order_task);
				}
				recordPtr_->MoveNext();
			}
		}
		return Executing_List;

	}
	catch (_com_error &e) {

		std::stringstream ss;
		ss << e.ErrorMessage() << "updateTaskOrderInfo" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return Executing_List;
	}
}

int WCS_Database::getMaxTaskId()
{

	_bstr_t SQL = "select count(*) from task_in_history";
	try
	{
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);                               //get the task(RecordSet) with the highest priority

		if (recordPtr_)
		{
			if ((int)recordPtr_->GetCollect("count(*)") == 0)
				return 0;
			else
			{
				SQL = "select task_id from task_in_history";
				recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
				recordPtr_->MoveLast();
				return (int)recordPtr_->GetCollect("task_id");
			}
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "getMaxTaskId" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return false;
}

int WCS_Database::getMaxWcsTaskId()
{
	_bstr_t SQL = "select count(*) from kh_parking_charging_info";
	try
	{
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);                               //get the task(RecordSet) with the highest priority

		if (recordPtr_)
		{
			if ((int)recordPtr_->GetCollect("count(*)") == 0)
			{
				return 1;
			}
			else
			{
				SQL = "select ID from kh_parking_charging_info";
				recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
				recordPtr_->MoveLast();
				return (int)recordPtr_->GetCollect("ID");
			}
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "getMaxWcsTaskId" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return false;
}

bool WCS_Database::checkOrderInfoCount(int &max_id)
{
	_bstr_t SQL = "select count(*) from kh_order_info";
	try
	{
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);                               //get the task(RecordSet) with the highest priority

		if (recordPtr_)
		{
			if ((int)recordPtr_->GetCollect("count(*)") == 0)
			{
				return false;
			}
			else
			{
				SQL = "select ORDER_ID from kh_order_info";
				recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
				recordPtr_->MoveLast();
				max_id = (int)recordPtr_->GetCollect("ORDER_ID");
				return true;
			}
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "checkOrderInfoCount" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return false;
}

bool WCS_Database::insertTask(int task_id_, std::string task_from_, std::string task_to_, int priority_, int agv_type_, int task_type_, int auto_dispatch_, int agv_id_, int task_extra_param_type, std::string task_extra_param)
{
	try
	{
		std::stringstream ssTask;
		std::stringstream ssTask_History;

		ssTask << "INSERT INTO task_in_table (task_id,task_from,task_to,priority,agv_type,task_type,auto_dispatch,agv_id,task_extra_param_type,task_extra_param)VALUES(" << task_id_ << ",'" << task_from_ << "','"
			<< task_to_ << "'," << priority_ << "," << agv_type_ << "," << task_type_ << "," << auto_dispatch_ << "," << agv_id_ << "," << task_extra_param_type << ",'" << task_extra_param << "')";
		_bstr_t SQL = ssTask.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);

		ssTask_History << "INSERT INTO task_in_history (task_id,task_from,task_to,priority,agv_type,task_type,task_extra_param)VALUES(" << task_id_ << ",'" << task_from_ << "','" << task_to_ << "'," << priority_ << "," << agv_type_ << "," << task_type_ << ",'" << task_extra_param << "')";
		_bstr_t SQL2 = ssTask_History.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL2);

		std::stringstream ss;
		ss << "Task_ID:" << task_id_ << "," << "From:" << task_from_ << "," << "To:" << task_to_ << "," << "Priority:" << priority_ << "," << "AGV_Type:" << agv_type_ << "," << "task_type:" << task_type_ << "," << "AGV_ID:" << agv_id_ << "," << "TASK_EXTRA_PARAM:" << task_extra_param;
		log_info_color(log_color::blue, ss.str().c_str());
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "insertTask" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
	return true;
}

bool WCS_Database::addOrder(std::string ORDER_ID, std::string START, std::string TARGET, int PRIORITY, std::string STATUS, std::string MODE, std::string TYPE, int AGV_ID)

{
	try
	{
		std::stringstream ssOrder;

		ssOrder << "INSERT INTO kh_report_order_info (ORDER_ID,AGV_ID,PRIORITY,STATUS,MODE,TYPE,START,TARGET)VALUES('" <<
			ORDER_ID << "'," << AGV_ID << ",'" << PRIORITY << "','" << STATUS << "','" 
			<< MODE << "','" << TYPE << "','" << START << "','" << TARGET << "')";
		_bstr_t SQL = ssOrder.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);

		std::stringstream ss;
		ss << "Add_Task_To_Order_List: " << "AGV_ID:" << AGV_ID << "," << "START:" << START << "," << "TARGET:" << TARGET << "," << "PRIORITY:" << PRIORITY << "," << "STATUS:" << STATUS << "," << "MODE:" << MODE << "," << "TYPE:" << TYPE;
		log_info_color(log_color::green, ss.str().c_str());
		return true;
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "addOrder" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

void WCS_Database::updateCurrentOrderOver(std::string order_id, int agv_id)
{
	try
	{
		std::stringstream ss1;
		std::stringstream ss2;
		// 已执行的工单置为OVER
		ss1 << "UPDATE kh_order_info SET STATUS = 'OVER' WHERE ORDER_ID = '" << order_id << "'";
		// 将相同起点的其他工单STATUS置为IDLE, AGVID置为指定AGV
		ss2 << "UPDATE kh_order_info JOIN(SELECT `ORDER_ID` FROM kh_order_info JOIN (SELECT `START` FROM kh_order_info WHERE ORDER_ID = '" << order_id << "') b ON kh_order_info.`START`=b.`START` WHERE `STATUS`='NEW') C ON kh_order_info.ORDER_ID=C.ORDER_ID SET `STATUS`='IDLE',AGV_ID=" << agv_id;
		_bstr_t SQL1 = ss1.str().c_str();
		_bstr_t SQL2 = ss2.str().c_str();
		log_info(SQL2);
		WCS_ADO_.ExecuteSQL(SQL2);
		WCS_ADO_.ExecuteSQL(SQL1);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "updateCurrentOrderOver" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}

bool WCS_Database::addParkingChargingOrder(std::string START, std::string TARGET, std::string TYPE, int AGV_ID)

{
	try
	{
		std::stringstream ssOrder;

		ssOrder << "INSERT INTO kh_parking_charging_info (AGV_ID,TYPE,START,TARGET)VALUES(" << AGV_ID << ",'" << TYPE << "','" << START << "','" << TARGET << "')";
		_bstr_t SQL = ssOrder.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);

		std::stringstream ss;
		ss << "Add_Task_To_Order_List: " << "AGV_ID:" << AGV_ID << "," << "START:" << START << "," << "TARGET:" << TARGET << "," << "TYPE:" << TYPE;
		log_info_color(log_color::green, ss.str().c_str());
		return true;
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "addParkingChargingOrder" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

bool WCS_Database::insertOrderToDb(std::string START, std::string TARGET, int PRIORITY, std::string STATUS, std::string MODE, std::string TYPE)
{
	try
	{
		std::stringstream ssOrder;
		ssOrder << "INSERT INTO kh_order_info (START,TARGET,PRIORITY,STATUS,MODE,TYPE)VALUES('" << START << "','" << TARGET << "'," << PRIORITY << ",'" << STATUS << "','" << MODE << "','" << TYPE << "')";
		_bstr_t SQL = ssOrder.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);

		return true;
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "insertOrderToDb" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

bool WCS_Database::insertOrderToDb(std::string ORDER_ID, std::string START, std::string TARGET, int PRIORITY, std::string STATUS, std::string MODE, std::string TYPE)
{
	try
	{
		std::stringstream ssOrder;
		ssOrder << "INSERT INTO kh_order_info (ORDER_ID, START,TARGET,PRIORITY,STATUS,MODE,TYPE,PALLETNO)VALUES('" << ORDER_ID <<"','"<< START << "','" << TARGET << "'," << PRIORITY << ",'" << STATUS << "','" << MODE << "','" << TYPE << "','666')";
		_bstr_t SQL = ssOrder.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);

		return true;
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "insertOrderToDb" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

bool WCS_Database::insertOrderToDb(std::string ORDER_ID, std::string START, std::string TARGET, int PRIORITY, std::string STATUS, std::string MODE, std::string TYPE, int agvID)
{
	try
	{
		std::stringstream ssOrder;
		ssOrder << "INSERT INTO kh_order_info (ORDER_ID, START,TARGET,PRIORITY,STATUS,MODE,TYPE,AGV_ID)VALUES('" << ORDER_ID << "','" << START << "','" << TARGET << "'," << PRIORITY << ",'" << STATUS << "','" << MODE << "','" << TYPE << "'," << agvID << ")";
		_bstr_t SQL = ssOrder.str().c_str();
		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "insertOrderToDb" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

bool WCS_Database::insertOrderToDb(std::string ORDER_ID, std::string START, std::string TARGET, int PRIORITY, std::string STATUS, std::string MODE, std::string TYPE, int agvID, std::string palletNo)
{
	try
	{
		std::stringstream ssOrder;
		ssOrder << "INSERT INTO kh_order_info (ORDER_ID, START,TARGET,PRIORITY,STATUS,MODE,TYPE,AGV_ID,PALLETNO)VALUES('" << ORDER_ID << "','" << START << "','" << TARGET << "'," << PRIORITY << ",'" << STATUS << "','" << MODE << "','" << TYPE << "'," << agvID << ",'" << palletNo <<"')";
		_bstr_t SQL = ssOrder.str().c_str();
		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "insertOrderToDb" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

int WCS_Database::readTaskStatus(int taskid)
{
	int ts = -3;

	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "select count(*) FROM task_feedback_table WHERE task_id=" << taskid << "";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);                               //get the task(RecordSet) with the highest priority

		if (recordPtr_)
		{
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		if (row_count != 0)
		{

			std::stringstream ssTask;
			ssTask << "SELECT task_status FROM task_feedback_table WHERE task_id=" << taskid << "";
			_bstr_t SQL2 = ssTask.str().c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL2);
			recordPtr_->MoveLast();
			ts = (int)recordPtr_->GetCollect("task_status");
		}
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "readTaskStatus" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return ts;
}

bool WCS_Database::stopCurrentCharging(AGV* agv)
{
	try
	{
		std::stringstream ssTask;
		ssTask << "INSERT INTO task_op_interface (object,agv_id,task_id,op_code) VALUES('agv'," << agv->AGV_ID_ << ",0,'stop charging')";
		_bstr_t SQL = ssTask.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
		return true;
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "stopCurrentCharging" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

bool WCS_Database::updateOrderInfo()
{
	try
	{
		int row_count = 0;
		_bstr_t SQL = "SELECT COUNT(*) FROM kh_order_info, kh_report_order_info WHERE kh_order_info.TYPE = 'CARRY' AND kh_order_info.ORDER_ID=kh_report_order_info.ORDER_ID ORDER BY kh_report_order_info.ENTERDATE desc limit 20";
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);

		if (recordPtr_)
		{
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		if (row_count != 0)
		{
			std::string sql = "SELECT kh_report_order_info.*, kh_order_info.MSG AS MSG, kh_order_info.ENTERDATE AS INDATE FROM kh_order_info, kh_report_order_info WHERE kh_order_info.TYPE = 'CARRY' AND kh_order_info.ORDER_ID=kh_report_order_info.ORDER_ID ORDER BY kh_report_order_info.ENTERDATE desc limit 20";
			_bstr_t SQL = sql.c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
			recordPtr_->MoveFirst();
			while (recordPtr_->adoEOF == VARIANT_FALSE) {
				Order current_order;
				current_order.ORDER_ID_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("ORDER_ID"))->Value);
				current_order.AGV_ID_ = atoi((_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("AGV_ID"))->Value));
				current_order.START_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("START"))->Value);
				current_order.TARGETED_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("TARGET"))->Value);
				current_order.PRIORITY_ = atoi((_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("PRIORITY"))->Value));
				current_order.STATUS_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("STATUS"))->Value);
				current_order.MODE_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("MODE"))->Value);
				current_order.TYPE_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("TYPE"))->Value);
				std::regex data_regex("/(\\d)(?!\\d)");
				std::regex hours_regex("\\s(\\d)(?!\\d)");// 时间的正则
				std::regex min_regex(":(\\d)(?!\\d)");// 时间的正则
				// 接取任务时间
				current_order.ENTERDATE_ = std::regex_replace((std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("INDATE"))->Value), data_regex, "/0$1");
				current_order.ENTERDATE_ = std::regex_replace(current_order.ENTERDATE_, hours_regex, " 0$1");
				current_order.ENTERDATE_ = std::regex_replace(current_order.ENTERDATE_, min_regex, ":0$1");
				// 入队时间认为开始时间
				current_order.STARTDATE_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("ENTERDATE"))->Value);
				current_order.STARTDATE_ = std::regex_replace(current_order.STARTDATE_, data_regex, "/0$1");
				//current_order.STARTDATE_ = std::regex_replace(current_order.FINISHDATE_, hours_regex, " 0$1");
				//current_order.STARTDATE_ = std::regex_replace(current_order.FINISHDATE_, min_regex, ":0$1");
				// 最后更新时间认为是完成时间
				current_order.FINISHDATE_ = std::regex_replace((std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("FINISHDATE"))->Value), data_regex, "/0$1");
				current_order.FINISHDATE_ = std::regex_replace(current_order.FINISHDATE_, hours_regex, " 0$1");
				current_order.FINISHDATE_ = std::regex_replace(current_order.FINISHDATE_, min_regex, ":0$1");

				current_order.Msg_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("MSG"))->Value);

				ORDER_MANAGE.Add_to_full_list_if_not_exist(current_order);
				recordPtr_->MoveNext();
			}
		}
		else {
			return false;
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "updateOrderInfo" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}

	return true;
}

bool WCS_Database::updateNewOrderInfo()
{
	try
	{
		std::map<std::string, Order> order_map = ORDER_MANAGE.getNewOrder();

		order_map.clear();

		ORDER_MANAGE.setNewOrder(order_map);

		int row_count = 0;
		_bstr_t SQL = "SELECT COUNT(*) FROM kh_order_info WHERE STATUS ='IDLE' OR STATUS ='NEW' ORDER BY `STATUS`, ENTERDATE desc limit 58";
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);

		if (recordPtr_)
		{
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		if (row_count != 0)
		{

			std::string sql = "SELECT * FROM kh_order_info WHERE STATUS ='IDLE' OR STATUS ='NEW' ORDER BY `STATUS`, ENTERDATE desc limit 58";
			_bstr_t SQL = sql.c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
			recordPtr_->MoveFirst();
			while (recordPtr_->adoEOF == VARIANT_FALSE) {
				Order current_order;
				current_order.ORDER_ID_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("ORDER_ID"))->Value);
				current_order.START_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("START"))->Value);
				current_order.TARGETED_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("TARGET"))->Value);
				current_order.PRIORITY_ = atoi((_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("PRIORITY"))->Value));
				current_order.STATUS_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("STATUS"))->Value);
				current_order.MODE_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("MODE"))->Value);
				current_order.TYPE_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("TYPE"))->Value);
				std::regex data_regex("/(\\d)(?!\\d)");// 日期的正则
				std::regex hours_regex("\\s(\\d)(?!\\d)");// 时间的正则
				std::regex min_regex(":(\\d)(?!\\d)");// 时间的正则
				current_order.ENTERDATE_ = std::regex_replace((std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("ENTERDATE"))->Value), data_regex, "/0$1");
				current_order.ENTERDATE_ = std::regex_replace(current_order.ENTERDATE_, hours_regex, " 0$1");
				current_order.ENTERDATE_ = std::regex_replace(current_order.ENTERDATE_, min_regex, ":0$1");
				current_order.Msg_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("MSG"))->Value);
				ORDER_MANAGE.Get_All_NEW_Order(current_order);
				recordPtr_->MoveNext();
			}
			return true;
		}
		else {
			return false;
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "updateDodInfo" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return false;
}

void WCS_Database::abortTask(int id)
{
	try
	{
		std::stringstream ssTask;
		ssTask << "INSERT INTO task_op_interface(object,agv_id,task_id,op_code) VALUES('task',0," << id << ",'abort')";
		_bstr_t SQL = ssTask.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "abortTask" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}

void WCS_Database::splitString2(const std::string& s, std::vector<std::string>& v, const std::string& c)
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

bool WCS_Database::checkNewOrderList(AGV* current_AGV)
{
	return false;
}

bool WCS_Database::revokeDodOrder(std::string order_id)
{
	try {
		int row_count = 0;
		std::stringstream ss;
		char szSq1[10240] = { 0 };
		ss << "SELECT COUNT(*) FROM kh_order_info WHERE ORDER_ID = '" << order_id << "'";
		sprintf_s(szSq1, "%s", ss.str().c_str());
		recordPtr2_ = WCS_ADO_.GetRecordSet((_bstr_t)szSq1);
		if (recordPtr2_)
		{
			row_count = (int)recordPtr2_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		if (row_count != 0)
		{
			std::stringstream ss2;
			ss2 << "UPDATE kh_order_info SET STATUS = 'REVOKE' WHERE ORDER_ID = '" << order_id << "'";
			_bstr_t SQL = ss2.str().c_str();
			
			return WCS_ADO_.ExecuteSQL(SQL);;
		}
		else
		{
			std::stringstream ss;
			ss << "Revoke DOD Order Error !!!!!! ";
			log_info_color(log_color::red, ss.str().c_str());
			return false;
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "revokeDodOrder" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return false;
}

bool WCS_Database::redoErrOrder(std::string order_id)
{
	try {

		std::stringstream ss2;
		ss2 << "UPDATE kh_order_info SET STATUS = 'NEW', AGV_ID = -1 WHERE ORDER_ID = '" << order_id << "' AND `STATUS`!='REVOKE'";
		_bstr_t SQL = ss2.str().c_str();

		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "revokeDodOrder" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return false;
}

void WCS_Database::setErrStorageOrder(int wcs_task_id)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE kh_order_info SET STATUS = 'ERR' WHERE ORDER_ID = '" << wcs_task_id << "'";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "setErrStorageOrder" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}

// 只解锁任务预定的库位
bool WCS_Database::unlockGoingTaskStorage(std::string table_name, std::string resource_status, std::string storage_name)
{
	if (storage_name.empty() || storage_name == "None" || storage_name == "NA")
	{
		return true;
	}
	try
	{
		// 只锁自己
		std::stringstream ss;
		ss << "UPDATE " << table_name << " SET resource_status = '" << resource_status << "' WHERE storage_name = '" << storage_name << "' AND resource_status='GOING'";
		_bstr_t SQL = ss.str().c_str();
		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "unlockGoingTaskStorage " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}

// 是否带着旁边一起锁
bool WCS_Database::setCurrentTableStorageStatus(std::string table_name, std::string resource_status, std::string storage_name, bool change_side /*= true*/)
{
	if (storage_name.empty() || storage_name == "None" || storage_name == "NA")
	{
		return true;
	}
	try
	{
		static std::vector<std::string> a_list = { "A1",
"A2",
"A3",
"A4",
"A5",
"A6",
"A7",
"A8",
"A9",
"A10",
"A11",
"A12", };
		static std::vector<std::string> b_list = { "B1",
"B2",
"B3",
"B4",
"B5",
"B6",
"B7",
"B8",
"B9",
"B10",
"B11",
"B12",
"B13",
"B14",
"B15",
"B16", };
		static std::vector<std::string> c_list = { "C1",
"C2",
"C3",
"C4",
"C5",
"C6",
"C7",
"C8",
"C9",
"C10",
"C11",
"C12",
"C13",
"C14",
"C15",
"C16",
"C17",
"C18",
"C19",
"C20",
"C21",
"C22",
"C23",
"C24",
"C25",
"C26",
"C27",
"C28",
		};
		static std::vector<std::string> d_list = { "D1",
"D2",
"D3",
"D4",
"D5",
"D6",
"D7",
"D8",
"D9",
"D10",
"D11",
"D12",
"D13",
"D14",
"D15",
"D16",
"D17",
"D18",
"D19",
"D20", };
		std::vector<std::string> area_list;
		if (change_side)
		{
			switch (storage_name[0])
			{
			case 'A':
				area_list = a_list;
				break;
			case 'B':
				area_list = b_list;
				break;
			case 'C':
				area_list = c_list;
				break;
			case 'D':
				area_list = d_list;
				break;
			default:
				break;
			}
			std::string next = "";
			std::string last = "";
			for (auto i = area_list.begin(); i != area_list.end(); i++)
			{
				if (*i == storage_name)
				{
					if (i + 1 != area_list.end())
					{
						next = *(i + 1);
					}
					if (i != area_list.begin() && i - 1 != area_list.end())
					{
						last = *(i - 1);
					}
					break;
				}
			}
			std::string before_status;
			std::string other_status;
			std::string other_before_status;
			if (resource_status == "IDLE")
			{
				// 中间库位从BUSY改为IDLE, 则两边从WAIT改为IDLE
				before_status = "RESERVE";

				other_before_status = "WAIT";
				other_status = "IDLE";
			}
			else if (resource_status == "RESERVE")
			{
				//中间从IDLE改为BUSY,则两边从IDLE改为WAIT
				before_status = "IDLE";

				other_before_status = "IDLE";
				other_status = "WAIT";
			}
			std::stringstream ss2;
			ss2 << "UPDATE " << table_name << " SET resource_status = '" << other_status << "' WHERE (storage_name = '" << last << "' OR storage_name = '" << next << "') AND resource_status ='" << other_before_status << "'";
			_bstr_t SQL2 = ss2.str().c_str();
			WCS_ADO_.ExecuteSQL(SQL2);

			std::stringstream ss;
			ss << "UPDATE " << table_name << " SET resource_status = '" << resource_status << "' WHERE storage_name = '" << storage_name << "'";
			_bstr_t SQL = ss.str().c_str();
			WCS_ADO_.ExecuteSQL(SQL);
		}
		else
		{
			// 只锁自己
			std::stringstream ss;
			ss << "UPDATE " << table_name << " SET resource_status = '" << resource_status << "' WHERE storage_name = '" << storage_name << "'";
			_bstr_t SQL = ss.str().c_str();
			WCS_ADO_.ExecuteSQL(SQL);
		}
		
		//}
		//else
		//{
		//	std::stringstream ss;
		//	ss << "UPDATE " << table_name << " SET resource_status = '" << resource_status << "' WHERE storage_name = '" << storage_name << "'";
		//	_bstr_t SQL = ss.str().c_str();
		//	WCS_ADO_.ExecuteSQL(SQL);
		//}
		//log_error(" release " + resource_status + " 123 "+ storage_name);
		return true;
	}
	catch (_com_error &e) {
		std::cout << "getCurrentTableStorageStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}

bool WCS_Database::setCurrentTableGoodsStorageStatus(std::string table_name, std::string resource_status, std::string storage_name, bool change_side /*= true*/)
{
	if (storage_name.empty())
	{
		return true;
	}
	try
	{
		static std::vector<std::string> a_list = { "A1",
"A2",
"A3",
"A4",
"A5",
"A6",
"A7",
"A8",
"A9",
"A10",
"A11",
"A12", };
		static std::vector<std::string> b_list = { "B1",
"B2",
"B3",
"B4",
"B5",
"B6",
"B7",
"B8",
"B9",
"B10",
"B11",
"B12",
"B13",
"B14",
"B15",
"B16", };
		static std::vector<std::string> c_list = { "C1",
"C2",
"C3",
"C4",
"C5",
"C6",
"C7",
"C8",
"C9",
"C10",
"C11",
"C12",
"C13",
"C14",
"C15",
"C16", };
		static std::vector<std::string> d_list = { "D1",
"D2",
"D3",
"D4",
"D5",
"D6",
"D7",
"D8",
"D9",
"D10",
"D11",
"D12",
"D13",
"D14",
"D15",
"D16",
"D17",
"D18",
"D19",
"D20", };
		std::vector<std::string> area_list;
		if (change_side)
		{
			switch (storage_name[0])
			{
			case 'A':
				area_list = a_list;
				break;
			case 'B':
				area_list = b_list;
				break;
			case 'C':
				area_list = c_list;
				break;
			case 'D':
				area_list = d_list;
				break;
			default:
				break;
			}
			std::string next = "";
			std::string last = "";
			for (auto i = area_list.begin(); i != area_list.end(); i++)
			{
				if (*i == storage_name)
				{
					if (i + 1 != area_list.end())
					{
						next = *(i + 1);
					}
					if (i != area_list.begin() && i - 1 != area_list.end())
					{
						last = *(i - 1);
					}
					break;
				}
			}
			std::string before_status;
			std::string other_status;
			std::string other_before_status;
			if (resource_status == "IDLE")
			{
				// 中间库位从BUSY改为IDLE, 则两边从WAIT改为IDLE
				before_status = "RESERVE";

				other_before_status = "WAIT";
				other_status = "IDLE";
			}
			else if (resource_status == "RESERVE")
			{
				//中间从IDLE改为BUSY,则两边从IDLE改为WAIT
				before_status = "IDLE";

				other_before_status = "IDLE";
				other_status = "WAIT";
			}
			std::stringstream ss2;
			ss2 << "UPDATE " << table_name << " SET resource_status = '" << other_status << "' WHERE (storage_name = '" << last << "' OR storage_name = '" << next << "') AND resource_status ='" << other_before_status << "'";
			_bstr_t SQL2 = ss2.str().c_str();
			WCS_ADO_.ExecuteSQL(SQL2);

			std::stringstream ss;
			ss << "UPDATE " << table_name << " SET resource_status = '" << resource_status << "' WHERE storage_name = '" << storage_name << "' AND storage_type='AREA'";
			_bstr_t SQL = ss.str().c_str();
			WCS_ADO_.ExecuteSQL(SQL);
		}
		else
		{
			// 只锁自己
			std::stringstream ss;
			ss << "UPDATE " << table_name << " SET resource_status = '" << resource_status << "' WHERE storage_name = '" << storage_name << "' AND storage_type='AREA'";
			_bstr_t SQL = ss.str().c_str();
			WCS_ADO_.ExecuteSQL(SQL);
		}

		return true;
	}
	catch (_com_error &e) {
		std::cout << "getCurrentTableStorageStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}


bool WCS_Database::setCurrentStationStatus(std::string station_name, std::string status) {
	bool result = false;
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE s_confirm_station_status SET STATUS = '" << status << "' WHERE STATION_NAME = '" << station_name << "'";
		_bstr_t SQL = ss.str().c_str();
		log_info(ss.str());
		result = WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "setCurrentStationStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}
	return result;
}

bool WCS_Database::setDatabaseCurrentComfirmStatus(std::string current_confirm, std::string status,int agv_id, std::string from , std::string to)
{
	std::string msg;
	/*if (status == "BUSY" )
	{
		static int num2 = 0;
		if (++num2 = 100)
		{
			num2 = 0;
		}
		if (num2 == 5)
		{
			log_error("AGV " + std::to_string(agv_id) + " at " + from + " to " + to + " but other agv before");
		}
		AGV_MANAGE.setAgvMsg(agv_id, from + "去往" + to + ":库位有AGV经过:" + msg);
		return false;
	}*/
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE s_confirm_station_status SET STATUS = '" << status << "',AGV_ID = '" << agv_id << "' WHERE STATION_NAME = '" << current_confirm << "'";
		_bstr_t SQL = ss.str().c_str();
		std::stringstream ss2;
		ss2 << "UPDATE s_confirm_station_status SET STATUS = '" << status << "',AGV_ID = '" << agv_id << "' WHERE STATION_NAME = '" << current_confirm << "'";
		log_info(ss2.str().c_str());
		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "setDatabaseCurrentComfirmStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}

bool WCS_Database::setDatabaseCurrentComfirmStatus(std::string current_confirm, std::string status)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE s_confirm_station_status SET STATUS = '" << status << "' WHERE STATION_NAME = '" << current_confirm << "'";
		_bstr_t SQL = ss.str().c_str();
		log_info(ss.str());
		WCS_ADO_.ExecuteSQL(SQL);
		return true;
	}
	catch (_com_error &e) {
		std::cout << "setDatabaseCurrentComfirmStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}

/// <summary>
/// 福沙 Sets the database current bridge status.
/// </summary>
/// <param name="current_confirm">The current confirm.</param>
/// <param name="status">The status.</param>
/// Creator:MeiXu
/// CreateDate:2021/2/3
/// 
void WCS_Database::setDatabaseCurrentBridgeStatus(std::string current_confirm, std::string status)
{
	try
	{
		std::stringstream ss;
		ss << "UPDATE kh_bridge_info SET STATUS = '" << status << "' WHERE BRIDGE_ID = '" << current_confirm << "'";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "setDatabaseCurrentBridgeStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}
}

void WCS_Database::setDatabaseCurrentParkingStatus(std::string current_parking, std::string status)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE parking_manage SET STATUS = '" << status << "' WHERE STATION_NAME = '" << current_parking << "'";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "setDatabaseCurrentComfirmStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}
}

void WCS_Database::setDatabaseEquipmentStatus(std::string id, std::string status) {
	try
	{
		std::string table_name = "kh_equipment_info";
		std::string equipment_id = "EQUIP_ID";
		std::string equipment_status = "EQUIP_STATUS";
		int row_count = 0;
		std::stringstream ss;
		ss << " UPDATE " << table_name << " SET " << equipment_status << " = '" << status << "' WHERE " << equipment_id << " = '" << id << "'";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "setDatabaseCurrentComfirmStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}
}

/*————————————————————————————————————分界线————————————————————*/

/********************************************************************
	created:	2020/06/11
	created:	11:6:2020   17:18
	filename: 	E:\work\Project\KunHouSimulator\WMS_Sets\wcs_aimosheng\src\Lib\ADODatabase\WCS_Database.cpp
	file path:	E:\work\Project\KunHouSimulator\WMS_Sets\wcs_aimosheng\src\Lib\ADODatabase
	file base:	WCS_Database
	file ext:	cpp
	author:		HSQ

	purpose:	load all table list from DB
*********************************************************************/
bool WCS_Database::loadWcsTableListFromDB(std::vector<Sql_Table_Attri> &sql_table_list_)
{
	_bstr_t SQL1;
	_bstr_t SQL2;
	_RecordsetPtr recordPtr;

	SQL1 = "select count(*) from A_wcs_table_list ";
	SQL2 = "select * from A_wcs_table_list";

	try {

		int row_count = 0;

		recordPtr = WCS_ADO_.GetRecordSet(SQL1);                               //get the task(RecordSet) with the highest priority

		if (recordPtr)
		{
			row_count = (int)recordPtr->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		if (row_count != 0)
		{
			std::stringstream ss;
			recordPtr = WCS_ADO_.GetRecordSet(SQL2);                               //get the task(RecordSet) with the highest priority

			recordPtr->MoveFirst();
			while (recordPtr->adoEOF == VARIANT_FALSE) {

				Sql_Table_Attri table_attri;

				table_attri.sql_table_name = (std::string)(_bstr_t)(recordPtr->Fields->GetItem(_variant_t("table_name"))->Value);
				table_attri.sql_table_type = (std::string)(_bstr_t)(recordPtr->Fields->GetItem(_variant_t("table_type"))->Value);
				table_attri.group_name = (std::string)(_bstr_t)(recordPtr->Fields->GetItem(_variant_t("group_name"))->Value);
				table_attri.columnSpan = atoi(_bstr_t(recordPtr->Fields->GetItem(_variant_t("columnSpan"))->Value));
				table_attri.rowSpan = atoi(_bstr_t(recordPtr->Fields->GetItem(_variant_t("rowSpan"))->Value));
				table_attri.widget_column = atoi(_bstr_t(recordPtr->Fields->GetItem(_variant_t("column"))->Value));
				table_attri.widget_row = atoi(_bstr_t(recordPtr->Fields->GetItem(_variant_t("row"))->Value));
				sql_table_list_.push_back(table_attri);
				recordPtr->MoveNext();
			}
			return true;
		}
		else {
			return false;
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "loadWcsTableListFromDB" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}

	return true;
}
/********************************************************************
	created:	2020/07/02
	created:	2:7:2020   11:45
	filename: 	E:\work\Project\KunHouSimulator\WMS_Sets\wcs_aimosheng\src\Lib\ADODatabase\WCS_Database.cpp
	file path:	E:\work\Project\KunHouSimulator\WMS_Sets\wcs_aimosheng\src\Lib\ADODatabase
	file base:	WCS_Database
	file ext:	cpp
	author:		HSQ
	
	purpose:	load all storage data from DB 
*********************************************************************/
bool WCS_Database::getStorageList(std::map<int, Storage> &storage_map, std::string sql_table_name)
{
	_bstr_t SQL1;
	_bstr_t SQL2;

	std::stringstream ss, ss2;
	ss << "select count(*) from " << sql_table_name;
	SQL1 = (_bstr_t)(ss.str().c_str());

	ss2 << "select * from " << sql_table_name;
	SQL2 = (_bstr_t)(ss2.str().c_str());

	try {
		/**
		 * 这里为了实现从多张表中读取 Storage , 取消使用 Current_Storage_List 进行传值
		 * 改为直接取值,然后写入 storage_map
		 */
		//std::map<int, Storage> Current_Storage_List;

		int row_count = 0;
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL1);                               //get the task(RecordSet) with the highest priority

		if (recordPtr_)
		{
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		if (row_count != 0)
		{
			std::stringstream ss;
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL2);                               //get the task(RecordSet) with the highest priority

			recordPtr_->MoveFirst();
			while (recordPtr_->adoEOF == VARIANT_FALSE) {

				Storage new_storage;
				new_storage.ID_ = atoi((_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("id"))->Value));
				new_storage.Storage_Name_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("storage_name"))->Value);
				new_storage.Storage_Row_ = atoi((_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("storage_row"))->Value));
				new_storage.Storage_Column_ = atoi((_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("storage_column"))->Value));
				new_storage.Storage_Status_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("storage_status"))->Value);
				new_storage.Storage_Type_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("storage_type"))->Value);
				new_storage.Resource_status_ = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("resource_status"))->Value);
				new_storage.sql_table_name_ = sql_table_name;
				// 这里直接对 storage_map 写入
				storage_map[new_storage.ID_] = new_storage;
				//Current_Storage_List[new_storage.ID_] = new_storage;
				recordPtr_->MoveNext();
			}
			//storage_map = Current_Storage_List;
			return true;
		}
		else {
			return false;
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		if (&e == nullptr)
		{
			ss << "getBufferStorageStatus" << e.Description();
		}
		else {
			ss << e.ErrorMessage() << "getBufferStorageStatus";
		}
		log_info_color(log_color::red, ss.str().c_str());
	}

	return true;
}

void WCS_Database::updateWcsStorageStatus(std::string sql_table_name, std::string storage_name,std::string status_name, std::string status)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE " << sql_table_name<<" SET "<<status_name<<" = '"<<status<<"' WHERE storage_name = '" << storage_name << "'";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "updateWcsStorageStatus" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}

bool WCS_Database::getCurrentParkingStation(std::string &station_name, std::string &is_charging, int agv_type)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		std::stringstream ss2;
		ss << "SELECT COUNT(*) FROM parking_manage WHERE STATUS='IDLE' AND AGV_TYPE='" << agv_type << "'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
		if (recordPtr_) {
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}

		if (row_count != 0)
		{
			ss2 << "select * FROM parking_manage WHERE STATUS = 'IDLE' AND AGV_TYPE='" << agv_type << "'";
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL2);

			station_name = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("STATION_NAME"))->Value);
			is_charging = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("IS_CHARGING"))->Value);
			return true;
		}
		else
		{
			return false;
		}
	}
	catch (_com_error &e) {
		std::cout << "getCurrentParkingStation " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}

bool WCS_Database::getAllParkingStation(std::map<std::string, ParkingStation> &all_parking)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		std::stringstream ss2;
		ss << "SELECT COUNT(*) FROM parking_manage";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
		if (recordPtr_) {
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}

		if (row_count != 0)
		{
			ss2 << "select * FROM parking_manage";
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL2);

			recordPtr_->MoveFirst();
			while (recordPtr_->adoEOF == VARIANT_FALSE) {

				ParkingStation new_storage;
				new_storage.ID_ = atoi((_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("ID"))->Value));
				new_storage.AgvType = atoi((_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("AGV_TYPE"))->Value));
				new_storage.StationName = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("STATION_NAME"))->Value);
				new_storage.IsCharging = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("IS_CHARGING"))->Value);
				new_storage.Status = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("STATUS"))->Value);
				// 这里直接对 storage_map 写入
				all_parking[new_storage.StationName] = new_storage;
				//Current_Storage_List[new_storage.ID_] = new_storage;
				recordPtr_->MoveNext();
			}
			return true;
		}
		else
		{
			return false;
		}
	}
	catch (_com_error &e) {
		std::cout << "getAllParkingStation " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}

void WCS_Database::reserveCurrentParkingStation(std::string station_name)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE parking_manage SET STATUS = 'BUSY' WHERE STATION_NAME = '" << station_name << "'";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);

	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "reserveCurrentParkingStation" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}

void WCS_Database::updateCurrentWeightInfo(double weight, std::string status)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE kh_report_weight_info SET WEIGHT = '" << weight << "', STATUS = '" << status << "'";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);

	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "updateCurrentWeightInfo" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}

float WCS_Database::getDatabaseCurrentWeightInfo()
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		std::stringstream ss2;
		ss << "SELECT COUNT(*) FROM kh_report_weight_info WHERE `STATUS`='SUCCESS'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
		if (recordPtr_)
		{
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}

		if (row_count != 0)
		{
			ss2 << "SELECT * FROM kh_report_weight_info";
			_bstr_t SQL1 = ss2.str().c_str();
			recordPtr2_ = WCS_ADO_.GetRecordSet(SQL1);
			std::string weight = (std::string)(_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("WEIGHT"))->Value);
			return std::atof(weight.c_str());
		}
		return -1;
		
	}
	catch (_com_error &e) {
		std::cout << "getDatabaseCurrentComfirmStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}
	return false;
}

void WCS_Database::releaseCurrentParkingStation(std::string station_name)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE parking_manage SET STATUS = 'IDLE' WHERE STATION_NAME = '" << station_name << "'";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);

	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "releaseCurrentParkingStation" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}

bool WCS_Database::getDatabaseCurrentComfirmStatus(std::string current_confirm)
{
	if (current_confirm == "")
	{
		return true;
	}
	std::string dispatch_mode;
	CONFIG_MANAGE.readConfig("DispatchMode", &dispatch_mode, "config.txt", true);
	if (dispatch_mode == "2" && (!current_confirm.empty() && current_confirm[0] == 'C'))
	{
		return true;
	}
	try
	{
		int row_count = 0;
		std::stringstream ss;
		std::stringstream ss2;
		ss << "SELECT * FROM s_confirm_station_status WHERE STATION_NAME='" << current_confirm << "'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
		std::string status = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("STATUS"))->Value);
		if (status == "IDLE")
		{
			std::stringstream ss3;
			ss3 << "SELECT * FROM s_confirm_station_status WHERE STATION_NAME='" << current_confirm << "' AND STATUS = '"<<status<<"'";
			log_info(ss3.str().c_str());
			return true;
		}
		else
		{
			return false;
		}
	}
	catch (_com_error &e) {
		std::cout << "getDatabaseCurrentComfirmStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}
	return false;
}

bool WCS_Database::getDatabaseCurrentComfirmStatus(std::string current_confirm,int agv_id)
{
	std::string dispatch_mode;
	CONFIG_MANAGE.readConfig("DispatchMode", &dispatch_mode, "config.txt", true);
	if (dispatch_mode == "2" && (!current_confirm.empty()))
	{
		return true;
	}
	try
	{
		int row_count = 0;
		std::stringstream ss;

		ss << "SELECT COUNT(*) FROM s_confirm_station_status WHERE STATION_NAME='" << current_confirm << "' AND AGV_ID ='" << agv_id << "'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
		if (recordPtr_)
		{
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}

		if (row_count == 1)
		{
			std::stringstream ss3;
			ss3 << "SELECT COUNT(*) FROM s_confirm_station_status WHERE STATION_NAME='" << current_confirm << "' AND AGV_ID ='" << agv_id << "'";
			log_info(ss3.str().c_str());
			return true;
		}
		else
		{
			return false;
		}
	}
	catch (_com_error &e) {
		std::cout << "getDatabaseCurrentComfirmStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}


/// <summary>
/// 福沙 Gets the current bridge status from database.
/// </summary>
/// <param name="current_confirm">The current bridge.</param>
/// <returns>Bridge is on the floor</returns>
/// Creator:MeiXu
/// CreateDate:2021/2/3
/// 
bool WCS_Database::getDatabaseCurrentBridgeStatus(std::string current_bridge)
{
	try
	{
		std::stringstream ss;
		ss << "SELECT * FROM kh_bridge_info WHERE BRIDGE_ID='" << current_bridge << "'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
		std::string status = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("BRIDGE_STATUS"))->Value);
		if (status == "DOWN")
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	catch (_com_error &e) {
		std::cout << "getDatabaseCurrentBridgeStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}
	return false;
}

bool WCS_Database::getCurrentStationIsCharging(std::string station_name)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;

		ss << "SELECT COUNT(*) FROM parking_manage WHERE STATION_NAME='" << station_name << "' AND IS_CHARGING='FALSE'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
		if (recordPtr_)
		{
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}

		if (row_count == 1)
		{
			return true;
		}
		else
		{
			return false;
		}

	}
	catch (_com_error &e) {
		std::cout << "getCurrentStationIsCharging " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}
	return false;
}

bool WCS_Database::getCurrentMustChargingStation(std::string &station_name)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		std::stringstream ss2;
		ss << "SELECT COUNT(*) FROM parking_manage WHERE Status='IDLE' AND IS_CHARGING='TRUE'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
		if (recordPtr_) {
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}

		if (row_count != 0)
		{
			ss2 << "SELECT * FROM parking_manage WHERE STATUS='IDLE' AND IS_CHARGING='TRUE'";
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL2);
			station_name = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("STATION_NAME"))->Value);
			return true;
		}
		else
		{
			return false;
		}
	}
	catch (_com_error &e) {
		std::cout << "getCurrentMustChargingStation " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}

/**
 * 从数据库获取 tableName 指向的表的 listName 列,并以 std::vector<std::string> 格式返回
 */
bool WCS_Database::loadListFromDB(std::vector<std::string> &valueList, std::string tableName, std::string listName)
{
	try
	{
		// 记录个数
		int row_count = 0;
		// SQL 语句
		std::stringstream ss;
		std::stringstream ss2;
		ss << "SELECT COUNT(*) FROM " << tableName;
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
		if (recordPtr_) {
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		// 存在可以读取的记录
		if (row_count != 0)
		{
			// 执行查询 SQL
			ss2 << "SELECT * FROM " << tableName;
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL2);
			// 遍历整个表
			while (recordPtr_->adoEOF == VARIANT_FALSE) {
				// 写入 valueList
				valueList.push_back((std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t(listName.c_str()))->Value));
				recordPtr_->MoveNext();
			}
			return true;
		}
		else
		{
			return false;
		}
	}
	catch (_com_error &e) {
		std::cout << "loadListFromDB " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}
	return true;
}

void WCS_Database::redoErrStart(std::string start)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE kh_order_info SET STATUS = 'NEW' WHERE START = '" << start << "' AND STATUS = 'IDLE'";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);

	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "releaseCurrentParkingStation" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}

bool WCS_Database::setAreaInfo(std::string area_name, std::string info)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE kh_region_info SET INFO = '" << info << "' WHERE ID = '" << area_name << "'";
		_bstr_t SQL = ss.str().c_str();
		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "setAreaInfo" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return false;
}

//bool WCS_Database::resetAgvsLineLength(int agv_id)
//{
//	try
//	{
//		std::stringstream ssTask;
//		std::stringstream ssTask_History;
//		ssTask << "INSERT INTO task_op_interface (object,agv_id,task_id,op_code)VALUES('line'," << agv_id << ",0,'reset line')";
//		_bstr_t SQL = ssTask.str().c_str();
//		return WCS_ADO_.ExecuteSQL(SQL);
//	}
//	catch (_com_error &e)
//	{
//		std::stringstream ss;
//		ss << e.ErrorMessage() << "resetAgvsLineLength" << e.Description();
//		log_info_color(log_color::red, ss.str().c_str());
//	}
//	return false;
//}

void WCS_Database::clearPreSearchFeedback()
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "DELETE * FROM pre_search_task_feedback_table";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);

	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "releaseCurrentParkingStation" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
}

bool WCS_Database::releaseCurrentConfirmStation(int AGV_ID_, std::string confirm_station)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE s_confirm_station_status SET STATUS = 'IDLE' WHERE AGV_ID = '" << AGV_ID_ << "' AND STATUS = 'BUSY' AND STATION_NAME = '" << confirm_station << "'";
		_bstr_t SQL = ss.str().c_str();
		log_info(SQL);
		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "setAreaInfo" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return false;
}

bool WCS_Database::changeAgvsLineLength(int agv_id, std::string change_length)
{
	try
	{
		std::stringstream ssTask;
		std::stringstream ssTask_History;
		ssTask << "INSERT INTO task_op_interface (object,agv_id,task_id,op_code)VALUES('line'," << agv_id << ",0,'" << change_length << "')";
		_bstr_t SQL = ssTask.str().c_str();
		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "changeAgvsLineLength" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return false;
}

/**
 * 获取库位名对应状态
 */
std::string WCS_Database::getStorageStatus(std::string table_name, std::string &storage_name) {
	int count = 0;
	std::stringstream ss;
	std::stringstream ss2;
	std::string status = "";
	try
	{
		ss << "SELECT COUNT(*) FROM " << table_name << " WHERE storage_name ='" << storage_name << "'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr2_ = WCS_ADO_.GetRecordSet(SQL);

		if (recordPtr2_)
		{
			count = (int)recordPtr2_->GetCollect("COUNT(*)");
		}
		else
		{
			std::stringstream ss3;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		if (count != 0)
		{
			ss2 << "SELECT * FROM " << table_name << " WHERE storage_name ='" << storage_name << "'";
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr2_ = WCS_ADO_.GetRecordSet(SQL2);
			status = (std::string)(_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("storage_status"))->Value);
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "getStorageStatus" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return status;
}

bool WCS_Database::getParkingStorage(std::vector<ModelStationParking*> *parkings) {
	std::string db_name = "parking_manage";
	try
	{
		// 记录个数
		int row_count = 0;
		// SQL 语句
		std::stringstream ss;
		std::stringstream ss2;
		ss << "SELECT COUNT(*) FROM " << db_name;
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
		if (recordPtr_) {
			row_count = (int)recordPtr_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		// 存在可以读取的记录
		if (row_count != 0)
		{
			// 执行查询 SQL
			ss2 << "SELECT * FROM "<< db_name <<" ORDER BY ID";
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL2);
			// 遍历整个表
			while (recordPtr_->adoEOF == VARIANT_FALSE) {
				// 写入 valueList
				//(std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t(listName.c_str()))->Value)
				//int id = recordPtr_->Fields->GetItem(_variant_t("ID"))->Value;
				std::string name = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("STATION_NAME"))->Value);
				std::string charging = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("IS_CHARGING"))->Value);
				std::string status = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("STATUS"))->Value);
				(*parkings).push_back(new ModelStationParking(name, charging, status));
				recordPtr_->MoveNext();
			}
			
		}
	}
	catch (_com_error &e) {
		std::cout << "getParkingStorage" << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}
	return ((*parkings).begin() == (*parkings).end());
}

/// <summary>
/// Gets the bridge from db.
/// </summary>
/// <param name="parkings">The parkings.</param>
/// <returns></returns>
/// Creator:MeiXu
/// CreateDate:2021/2/3
/// 


void WCS_Database::setDatabaseCurrentEquipmentInfoStatus(std::string current_equipment, std::string equip_status)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "UPDATE kh_equipment_info SET EQUIP_STATUS = '" << equip_status << "' WHERE EQUIP_ID = '" << current_equipment << "'";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "setDatabaseCurrentEquipmentInfoStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}
}


void WCS_Database::setDatabaseCurrentEquipmentAppleyStatus(std::string current_equipment, std::string status)
{
	try
	{
		std::stringstream ss;
		ss << "UPDATE kh_equipment_info SET STATUS = '" << status << "' WHERE EQUIP_ID = '" << current_equipment << "'";
		_bstr_t SQL = ss.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "setDatabaseCurrentEquipmentInfoStatus " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}
}

std::string WCS_Database::getDatabaseCurrentEquipmentInfoStatus(std::string current_equipment)
{
	int count = 0;
	std::stringstream ss;
	std::stringstream ss2;
	std::string EQUIP_STATUS="";
	try
	{
		ss << "SELECT COUNT(*) FROM kh_equipment_info WHERE EQUIP_ID='" << current_equipment << "'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);

		if (recordPtr_)
		{
			count = (int)recordPtr_->GetCollect("COUNT(*)");
		}
		else
		{
			std::stringstream ss3;
			ss3 << "Cant get Recordset!";
			log_info_color(log_color::red, ss3.str().c_str());
		}
		if (count != 0)
		{
			ss2 << "SELECT * FROM kh_equipment_info WHERE EQUIP_ID='" << current_equipment << "'";
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL2);
			EQUIP_STATUS = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("EQUIP_STATUS"))->Value);
			return EQUIP_STATUS;
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "getDatabaseCurrentEquipmentInfoStatus" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return EQUIP_STATUS;
	}
	return EQUIP_STATUS;
}

std::string WCS_Database::getCurrentAGVIsBack(int agv_id)
{
	int count = 0;
	std::stringstream ss;
	std::stringstream ss2;
	std::string SIGNAL = "";
	try
	{

		ss << "SELECT COUNT(*) FROM agv_back_signal where AGV_ID = " << agv_id << "";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);

		if (recordPtr_)
		{
			count = (int)recordPtr_->GetCollect("COUNT(*)");
		}
		else
		{
			std::stringstream ss3;
			ss3 << "Cant get Recordset!";
			log_info_color(log_color::red, ss3.str().c_str());
		}
		if (count != 0)
		{
			ss2 << "SELECT * FROM agv_back_signal where AGV_ID = " << agv_id << "";
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL2);
			SIGNAL = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("SIGNAL"))->Value);
			return SIGNAL;
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "getCurrentAGVIsBack" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		
	}
	return SIGNAL;
}

bool WCS_Database::getDatabaseCurrentNextNewOrder(std::string statioin, std::string agv_type)
{

	int row_count = 0;
	std::stringstream ss;
	std::stringstream ss2;
	try
	{
		ss << "SELECT COUNT(*) FROM kh_order_info WHERE STATUS = 'NEW' AND MODE='" << agv_type << "' ORDER BY ENTERDATE asc limit 1";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);
		if (recordPtr_)
		{
			row_count = (int)recordPtr_->GetCollect("COUNT(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		if (row_count != 0)
		{
			ss2 << "SELECT * FROM kh_order_info WHERE STATUS = 'NEW' AND MODE='" << agv_type << "' ORDER BY ENTERDATE asc limit 1";
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL2);

			recordPtr_->MoveFirst();
			while (recordPtr_->adoEOF == VARIANT_FALSE)
			{
				std::string  START = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t("START"))->Value);
			
				if (START.substr(0, 6) == statioin.substr(0,6))
				{
					return true;
				}
				else 
				{
					return false;
				}
				recordPtr_->MoveNext();
			}
			return false;
		}
		else
		{
			return false;
		}
	}
	catch (_com_error &e) {

		std::stringstream ss;
		ss << e.ErrorMessage() << "getDatabaseCurrentNextNewOrder" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

bool WCS_Database::LoadPLCMap(std::map<std::string, std::map<int, std::string>>* plc_map_, std::string table_name, std::vector<std::string> table_fields_) {
	bool result = false;
	int row_count = 0;
	// 构建SQL语句
	try
	{
		std::stringstream ss;
		std::stringstream ss2;
		ss << "SELECT COUNT(*) FROM " << table_name;
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);

		if (recordPtr_)
		{
			row_count = (int)recordPtr_->GetCollect("COUNT(*)");
		}
		else
		{
			std::stringstream ss3;
			ss3 << "Cant get Recordset!";
			log_info_color(log_color::red, ss3.str().c_str());
		}
		if (row_count != 0)
		{
			ss2 << "SELECT * FROM " << table_name;
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr_ = WCS_ADO_.GetRecordSet(SQL2);
			while (recordPtr_->adoEOF == VARIANT_FALSE) {
				std::string plc_name = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t(table_fields_[1].c_str()))->Value);
				std::map<int, std::string> pld_data;
				for (size_t i = 0; i < table_fields_.size(); i++)
				{
					pld_data[i] = (std::string)(_bstr_t)(recordPtr_->Fields->GetItem(_variant_t(table_fields_[i].c_str()))->Value);
				}
				recordPtr_->MoveNext();
				(*plc_map_)[plc_name] = pld_data;
			}
			
			return true;
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "LoadPLCMap " << e.Description();
		log_info_color(log_color::red, ss.str().c_str());

	}

	return result;
}

bool WCS_Database::updatePLCStatus(std::string dev_name, std::string plc_status) {
	bool result = false;
	// 构建SQL语句
	std::stringstream ss;
	ss << "UPDATE " << "kh_plc_data" << " SET " << "PLC_TRIGGER" << " = '" << plc_status << "' WHERE " << "DEV_NAME" << " = '" << dev_name << "'";
	_bstr_t SQL = ss.str().c_str();
	log_info_color(log_color::green, ss.str().c_str());
	try
	{
		result = WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "setPointField :" << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}

	return result;
}
bool WCS_Database::updatePLCMsgStatus(std::string plc_status) {
	bool result = false;
	// 构建SQL语句
	std::stringstream ss;
	ss << "UPDATE " << "kh_plc_data" << " SET " << "MSG_STATUS" << " = '" << plc_status << "'";
	_bstr_t SQL = ss.str().c_str();
	log_error(ss.str().c_str());
	try
	{
		result = WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "WCS_Database::updatePLCMsgStatus :" << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}

	return result;
}

bool WCS_Database::updatePLCReadStatus(std::string plc_status) {
	bool result = false;
	// 构建SQL语句
	std::stringstream ss;
	ss << "UPDATE " << "kh_plc_data" << " SET " << "READ_STATUS" << " = '" << plc_status << "' WHERE READ_STATUS!='AUTO'";
	_bstr_t SQL = ss.str().c_str();
	log_error(ss.str().c_str());
	try
	{
		result = WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "WCS_Database::updatePLCReadStatus :" << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}

	return result;
}
bool WCS_Database::updatePLCMsgStatus(std::string dev_name, std::string plc_status) {
	bool result = false;
	// 构建SQL语句
	std::stringstream ss;
	ss << "UPDATE " << "kh_plc_data" << " SET " << "MSG_STATUS" << " = '" << plc_status << "' WHERE " << "DEV_NAME" << " = '" << dev_name << "'";
	_bstr_t SQL = ss.str().c_str();
	log_error(ss.str().c_str());
	if (plc_status != "IDLE")
	{
		log_info(ss.str().c_str());
	}
	try
	{
		result = WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "WCS_Database::updatePLCMsgStatus :" << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}

	return result;
}

bool WCS_Database::updatePLCReadStatus(std::string dev_name, std::string plc_status) {
	bool result = false;
	// 构建SQL语句
	std::stringstream ss;
	ss << "UPDATE " << "kh_plc_data" << " SET " << "READ_STATUS" << " = '" << plc_status << "' WHERE " << "DEV_NAME" << " = '" << dev_name << "'";
	_bstr_t SQL = ss.str().c_str();
	log_error(ss.str().c_str());
	try
	{
		result = WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "WCS_Database::updatePLCReadStatus :" << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}

	return result;
}

bool WCS_Database::changeAgvMode(std::string equip_mode) {
	bool result = false;
	// 构建SQL语句
	std::stringstream ss;
	ss << "UPDATE " << "agv_mode" << " SET " << "EQUIP_MODE" << " = '" << equip_mode << "'";
	_bstr_t SQL = ss.str().c_str();

	try
	{
		result = WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "WCS_Database::changeAgvMode :" << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}

	return result;
}

bool WCS_Database::agvBackSignal(int agv_id, std::string mode) {
	bool result = false;
	// 构建SQL语句
	std::stringstream ss;
	ss << "UPDATE " << " agv_back_signal " << " SET " << " BACK_SIGNAL = '" << mode << "' WHERE " << "AGV_ID" << " = " << agv_id;
	_bstr_t SQL = ss.str().c_str();

	try
	{
		result = WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e) {
		std::cout << "WCS_Database::updatePLCReadStatus :" << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
	}

	return result;
}
/*
从 table_name 中,获取 table_fields 中的字段, 并放在 result_list 中
返回获取是否成功
*/
bool WCS_Database::loadFromDB(std::string table_name, std::vector<std::string> table_fields, std::vector<std::vector<std::string>>* result_list) {
	bool result = false;
	// 构建SQL语句
	std::stringstream ss1;
	std::stringstream ss2;
	ss1 << "SELECT COUNT(*) FROM " << table_name;
	ss2 << "SELECT * FROM " << table_name;
	_bstr_t SQL1 = ss1.str().c_str();
	_bstr_t SQL2 = ss2.str().c_str();

	try {
		int row_count = 0;
		recordPtr2_ = WCS_ADO_.GetRecordSet(SQL1);
		if (recordPtr2_)
		{
			row_count = (int)recordPtr2_->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		if (row_count != 0)
		{
			recordPtr2_ = WCS_ADO_.GetRecordSet(SQL2);
			recordPtr2_->MoveFirst();
			while (recordPtr2_->adoEOF == VARIANT_FALSE) {
				std::vector<std::string> record;
				for each (std::string field in table_fields)
				{
					record.push_back((std::string)(_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t(field.c_str()))->Value));
				}
				result_list->push_back(record);
				recordPtr2_->MoveNext();
			}
			result = true;
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "loadFromDB :" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return result;
}

bool WCS_Database::getCurrentEquipToStorage(std::string &storage_group, std::string &tray_type, std::string equip_name)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		ss << "SELECT COUNT(*) FROM s_equip_storage WHERE equip_name='" << equip_name << "'";
		_bstr_t SQL = ss.str().c_str();
		recordPtr2_ = WCS_ADO_.GetRecordSet(SQL);                               //get the task(RecordSet) with the highest priority

		if (recordPtr2_)
		{
			row_count = (int)recordPtr2_->GetCollect("count(*)");
		}
		if (row_count != 0)
		{
			std::stringstream ss2;
			ss2 << "SELECT * FROM s_equip_storage WHERE equip_name='" << equip_name << "'";
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr2_ = WCS_ADO_.GetRecordSet(SQL2);
			storage_group = (std::string)(_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("STORAGE_GROUP"))->Value);
			tray_type = (std::string)(_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("TRAY_TYPE"))->Value);
			return true;
		}
		else
		{
			return false;
		}

	}
	catch (_com_error &e) {
		std::cout << "getCurrentEquipToStorage " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}
// 执行只执行IDLE的
// 就近找一个,相邻库位没有 RESERVE 的
bool WCS_Database::getCurrentTableStorageName(std::string table_name, std::string storage_status, std::string &storage_name, std::string storage_type, std::string storage_group, std::string tray_type, std::string current_storage)
{
//SELECT * FROM 
//(SELECT * FROM s_storage_station_status WHERE storage_status='EMPTY' AND storage_type='AREA' AND storage_group='A' AND resource_status!='RESERVE') A
//WHERE 
//A.id+1 NOT IN (SELECT id FROM s_storage_station_status WHERE resource_status='RESERVE')
//AND 
//A.id-1 NOT IN (SELECT id FROM s_storage_station_status WHERE resource_status='RESERVE')
	try
	{
		int row_count = 0;
		std::stringstream ss;
		if (storage_status=="EMPTY")
		{
			ss << "SELECT COUNT(*) FROM \
				(\
				SELECT * FROM " << table_name << " \
				WHERE\
				storage_status='" << storage_status << "' AND storage_type='" << storage_type << "' AND storage_group='" << storage_group << "' AND resource_status!='RESERVE'\
				) A\
				WHERE\
				A.id+1 NOT IN (SELECT id FROM s_storage_station_status WHERE resource_status='RESERVE' AND storage_name != '" << current_storage << "')\
				AND\
				A.id-1 NOT IN (SELECT id FROM s_storage_station_status WHERE resource_status='RESERVE' AND storage_name != '" << current_storage << "')";
		}
		else
		{
			ss << "SELECT COUNT(*) FROM \
				(\
				SELECT * FROM " << table_name << " \
				WHERE\
				storage_status='" << storage_status << "' AND storage_type='" << storage_type << "' AND storage_group='" << storage_group << "' AND resource_status!='RESERVE' AND tray_type='" << tray_type << "'\
				) A\
				WHERE\
				A.id+1 NOT IN (SELECT id FROM s_storage_station_status WHERE resource_status='RESERVE' AND storage_name != '" << current_storage << "')\
				AND\
				A.id-1 NOT IN (SELECT id FROM s_storage_station_status WHERE resource_status='RESERVE' AND storage_name != '" << current_storage << "')";
		}
		_bstr_t SQL = ss.str().c_str();
		recordPtr3_ = WCS_ADO_.GetRecordSet(SQL);                               //get the task(RecordSet) with the highest priority

		if (recordPtr3_)
		{
			row_count = (int)recordPtr3_->GetCollect("count(*)");
		}
		if (row_count != 0)
		{
			std::stringstream ss2;
			if (storage_status == "EMPTY")
			{
				// 满轮
				ss2 << "SELECT * FROM \
				(\
				SELECT * FROM " << table_name << " \
				WHERE\
				storage_status='" << storage_status << "' AND storage_type='" << storage_type << "' AND storage_group='" << storage_group << "' AND resource_status!='RESERVE'\
				) A\
				WHERE\
				A.id+1 NOT IN (SELECT id FROM s_storage_station_status WHERE resource_status='RESERVE' AND storage_name != '" << current_storage << "')\
				AND\
				A.id-1 NOT IN (SELECT id FROM s_storage_station_status WHERE resource_status='RESERVE' AND storage_name != '" << current_storage << "')";
			}
			else
			{
				ss2 << "SELECT * FROM \
				(\
				SELECT * FROM " << table_name << " \
				WHERE\
				storage_status='" << storage_status << "' AND storage_type='" << storage_type << "' AND storage_group='" << storage_group << "' AND resource_status!='RESERVE' AND tray_type='" << tray_type << "'\
				) A\
				WHERE\
				A.id+1 NOT IN (SELECT id FROM s_storage_station_status WHERE resource_status='RESERVE' AND storage_name != '" << current_storage << "')\
				AND\
				A.id-1 NOT IN (SELECT id FROM s_storage_station_status WHERE resource_status='RESERVE' AND storage_name != '" << current_storage << "')";
			}
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr3_ = WCS_ADO_.GetRecordSet(SQL2);
			storage_name = (std::string)(_bstr_t)(recordPtr3_->Fields->GetItem(_variant_t("storage_name"))->Value);
			return true;
		}
		else
		{
			return false;
		}

	}
	catch (_com_error &e) {
		std::cout << "getCurrentTableStorageName " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}
// get_order = true 接工单时,接取IDLE和WAIT 的；
// get_order = false 执行时,执行IDLE和WAIT和GOING的
bool WCS_Database::getCurrentTableStorageName(std::string table_name, std::string storage_status, std::string &storage_name, std::string storage_type, std::string storage_group, std::string tray_type, bool get_order)
{
	try
	{
		int row_count = 0;
		std::stringstream ss;
		std::string last;
		if (get_order)
		{
			last = "";
		}
		else {
			last = " OR resource_status='GOING'";
		}
		if (storage_status == "EMPTY")
		{
			ss << "SELECT COUNT(*) FROM " << table_name << " WHERE storage_status='" << storage_status << "' AND storage_type='" << storage_type << "' AND storage_group='" << storage_group << "' AND (resource_status='IDLE' OR resource_status='WAIT' " << last << ")";
		}
		else
		{
			ss << "SELECT COUNT(*) FROM " << table_name << " WHERE storage_status='" << storage_status << "' AND storage_type='" << storage_type << "' AND storage_group='" << storage_group << "' AND tray_type='" << tray_type << "' AND (resource_status='IDLE' OR resource_status='WAIT' " << last << " )";
		}
		_bstr_t SQL = ss.str().c_str();
		recordPtr3_ = WCS_ADO_.GetRecordSet(SQL);                               //get the task(RecordSet) with the highest priority

		if (recordPtr3_)
		{
			row_count = (int)recordPtr3_->GetCollect("count(*)");
		}
		if (row_count != 0)
		{
			std::stringstream ss2;
			if (storage_status == "EMPTY")
			{
				ss2 << "SELECT * FROM " << table_name << " WHERE storage_status='" << storage_status << "' AND storage_type='" << storage_type << "' AND storage_group='" << storage_group << "' AND (resource_status='IDLE' OR resource_status='WAIT'" << last << ")";
			}
			else
			{
				ss2 << "SELECT * FROM " << table_name << " WHERE storage_status='" << storage_status << "' AND storage_type='" << storage_type << "' AND storage_group='" << storage_group << "' AND tray_type='" << tray_type << "' AND (resource_status='IDLE' OR resource_status='WAIT' " << last << " )";
			}
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr3_ = WCS_ADO_.GetRecordSet(SQL2);
			storage_name = (std::string)(_bstr_t)(recordPtr3_->Fields->GetItem(_variant_t("storage_name"))->Value);
			return true;
		}
		else
		{
			return false;
		}

	}
	catch (_com_error &e) {
		std::cout << "getCurrentTableStorageName " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}


bool WCS_Database::getCurrentTableStorageNameNOResource(std::string table_name, std::string storage_status, std::string &storage_name, std::string storage_type, std::string storage_group, std::string tray_type)
{
	// 执行任务时,取IDLE（空闲）或者GOING（任务预定）的,不能取RESERVE（有车）和WAIT（旁边有车）的
	try
	{
		int row_count = 0;
		std::stringstream ss;
		if (storage_status == "EMPTY")
		{
			ss << "SELECT COUNT(*) FROM " << table_name << " WHERE storage_status='" << storage_status << "' AND storage_type='" << storage_type << "' AND storage_group='" << storage_group << "' AND (resource_status='IDLE' OR resource_status='GOING')";
		}
		else
		{
			ss << "SELECT COUNT(*) FROM " << table_name << " WHERE storage_status='" << storage_status << "' AND storage_type='" << storage_type << "' AND storage_group='" << storage_group << "' AND tray_type='" << tray_type << "' AND (resource_status='IDLE' OR resource_status='GOING')";
		}
		_bstr_t SQL = ss.str().c_str();
		recordPtr3_ = WCS_ADO_.GetRecordSet(SQL);                               //get the task(RecordSet) with the highest priority

		if (recordPtr3_)
		{
			row_count = (int)recordPtr3_->GetCollect("COUNT(*)");
		}
		if (row_count != 0)
		{
			std::stringstream ss2;
			if (storage_status == "EMPTY")
			{
				ss2 << "SELECT * FROM " << table_name << " WHERE storage_status='" << storage_status << "' AND storage_type='" << storage_type << "' AND storage_group='" << storage_group << "' AND (resource_status='IDLE' OR resource_status='GOING')";
			}
			else
			{
				ss2 << "SELECT * FROM " << table_name << " WHERE storage_status='" << storage_status << "' AND storage_type='" << storage_type << "' AND storage_group='" << storage_group << "' AND tray_type='" << tray_type << "' AND (resource_status='IDLE' OR resource_status='GOING')";
			}
			_bstr_t SQL2 = ss2.str().c_str();
			recordPtr3_ = WCS_ADO_.GetRecordSet(SQL2);
			storage_name = (std::string)(_bstr_t)(recordPtr3_->Fields->GetItem(_variant_t("storage_name"))->Value);
			return true;
		}
		else
		{
			return false;
		}

	}
	catch (_com_error &e) {
		std::cout << "getCurrentTableStorageName " << e.ErrorMessage() << std::endl;
		std::cout << e.Description() << std::endl;
		return false;
	}
}


int WCS_Database::GetAgvExeGoodsTask(std::string palletNo, std::string status, std::string mode)
{
	std::stringstream ss;
	ss << "select count(*) from kh_report_order_info WHERE STATUS = '" << status << "' AND MODE = '" << mode << "' ORDER BY ID DESC LIMIT 1";
	_bstr_t SQL = ss.str().c_str();
	try
	{
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);                               //get the task(RecordSet) with the highest priority

		if (recordPtr_)
		{
			if ((int)recordPtr_->GetCollect("count(*)") == 0)
				return 0;
			else
			{
				std::stringstream ss2;
				ss2 << "select * from kh_report_order_info WHERE STATUS = '" << status << "' AND MODE = '" << mode << "' ORDER BY ID DESC LIMIT 1";
				_bstr_t SQL2 = ss2.str().c_str();

				// SQL = "select task_id from kh_report_order_info";
				recordPtr_ = WCS_ADO_.GetRecordSet(SQL2);
				recordPtr_->MoveLast();
				return (int)recordPtr_->GetCollect("AGV_ID");
			}
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "GetAgvExeGoodsTask" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return 0;
}

bool WCS_Database::plcForbidAllStorage() {
	try
	{
		std::string table_name = "s_storage_station_status";

		int row_count = 0;
		std::stringstream ss;
		std::stringstream ss2;
		ss << "UPDATE " << table_name << " SET storage_status = '" << "EMPTY" << "' WHERE storage_type = '" << "TRAY" << "'";
		ss2 << "UPDATE " << table_name << " SET storage_status = '" << "FULL" << "' WHERE storage_type = '" << "AREA" << "'";
		_bstr_t SQL = ss.str().c_str();
		_bstr_t SQL2 = ss2.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
		WCS_ADO_.ExecuteSQL(SQL2);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "setCurrentStorageStatus" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return true;
}

void WCS_Database::insertStopTaskToTable(int agv_id, std::string op_code)
{
	try
	{
		std::stringstream ssTask;
		std::stringstream ssTask_History;
		ssTask << "INSERT INTO task_op_interface (object,agv_id,task_id,op_code)VALUES('agv'," << agv_id << ",999,'" << op_code << "')";
		_bstr_t SQL = ssTask.str().c_str();
		WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		if ((void*)&e != NULL)
		{
			std::stringstream ss;
			ss << e.ErrorMessage() << "insertStopTaskToTable" << e.Description();
			log_info_color(log_color::red, ss.str().c_str());
		}
		else
		{
			log_info_color(log_color::red, "Unknown exception happened in %s,%s,%d", __FILE__, __FUNCTION__, __LINE__);
		}
	}
}

bool WCS_Database::updateOrderStatusToErr() {
	try
	{
		std::string table_name = "kh_report_order_info";
		std::stringstream ss;
		ss << "UPDATE " << table_name << " SET STATUS = '" << "ERR" << "' WHERE STATUS != '" << "DONE" << "'";
		_bstr_t SQL = ss.str().c_str();
		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "setCurrentStorageStatus" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return true;
}

bool WCS_Database::updateOrderStatus(std::string new_status, std::string old_status, std::string start) {
	try
	{
		std::string table_name = "kh_order_info";
		std::stringstream ss;
		ss << "UPDATE " << table_name << " SET STATUS = '" << new_status << "' WHERE STATUS = '" << old_status << "' AND START= '" << start << "'";
		_bstr_t SQL = ss.str().c_str();
		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "updateOrderStatus" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return true;
}

bool WCS_Database::readAreaInfo(std::map<int, std::vector<Pos_Info>> *all_area) {
	bool result = false;

	try
	{
		std::stringstream ss;
		ss << "SELECT COUNT(*) FROM kh_region_info LIMIT 1";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);                               //get the task(RecordSet) with the highest priority

		if (recordPtr_)
		{
			if ((int)recordPtr_->GetCollect("COUNT(*)") != 0) {
				std::stringstream ss2;
				ss2 << "SELECT * FROM kh_region_info";
				_bstr_t SQL2 = ss2.str().c_str();

				recordPtr2_ = WCS_ADO_.GetRecordSet(SQL2);
				recordPtr2_->MoveFirst();
				result = true;
				while (recordPtr2_->adoEOF == VARIANT_FALSE) {
					std::vector<std::string> record;
					int id = atoi((_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("ID"))->Value));
					std::string region = (std::string)(_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("REGION"))->Value);

					// region -> num 个 Pos
					std::vector <std::string> point_row;
					boost::split(point_row, region, boost::is_any_of("("));
					for each (auto var in point_row)
					{
						std::vector <std::string> point_pos;
						boost::split(point_pos, var, boost::is_any_of(","));
						if (point_pos.size() == 2)
						{
							Pos_Info pos;
							pos.x = atof(point_pos.at(0).c_str());
							pos.y = atof(point_pos.at(1).replace(point_pos.at(1).find(")"), 1, "").c_str());
							(*all_area)[id].push_back(pos);
						}
					}

					recordPtr2_->MoveNext();
				}
			}
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "readAreaInfo" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return result;
}

bool WCS_Database::readAreaInfo(std::map<std::string, Area_Info> &all_area) {
	bool result = false;

	try
	{
		std::stringstream ss;
		ss << "SELECT COUNT(*) FROM kh_region_info LIMIT 1";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);                               //get the task(RecordSet) with the highest priority

		if (recordPtr_)
		{
			if ((int)recordPtr_->GetCollect("COUNT(*)") != 0) {
				std::stringstream ss2;
				ss2 << "SELECT * FROM kh_region_info";
				_bstr_t SQL2 = ss2.str().c_str();

				recordPtr2_ = WCS_ADO_.GetRecordSet(SQL2);
				recordPtr2_->MoveFirst();
				result = true;
				while (recordPtr2_->adoEOF == VARIANT_FALSE) {
					std::string area_name = (std::string)(_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("ID"))->Value);
					std::string region = (std::string)(_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("REGION"))->Value);
					std::string area_type = (std::string)(_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("TYPE"))->Value);
					std::string info = (std::string)(_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("INFO"))->Value);
					all_area[area_name].area_name = area_name;
					all_area[area_name].area_type = area_type;
					all_area[area_name].info = info;
					// region -> num 个 Pos
					std::vector <std::string> point_row;
					boost::split(point_row, region, boost::is_any_of("("));
					for each (auto var in point_row)
					{
						std::vector <std::string> point_pos;
						boost::split(point_pos, var, boost::is_any_of(","));
						if (point_pos.size() == 2)
						{
							Pos_Info pos;
							pos.x = atof(point_pos.at(0).c_str());
							pos.y = atof(point_pos.at(1).replace(point_pos.at(1).find(")"), 1, "").c_str());
							all_area[area_name].pos_list.push_back(pos);
						}
					}
					recordPtr2_->MoveNext();
				}
			}
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "readAreaInfo" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return result;
}

bool WCS_Database::loadConfirmStationFromDB(std::map<std::string, Confirm> *confirm_list)
{
	_bstr_t SQL1;
	_bstr_t SQL2;
	_RecordsetPtr recordPtr;

	SQL1 = "SELECT COUNT(*) FROM s_confirm_station_status ";
	SQL2 = "SELECT * FROM s_confirm_station_status";

	try {

		int row_count = 0;

		recordPtr = WCS_ADO_.GetRecordSet(SQL1);                               //get the task(RecordSet) with the highest priority

		if (recordPtr)
		{
			row_count = (int)recordPtr->GetCollect("count(*)");
		}
		else
		{
			std::stringstream ss;
			ss << "Cant get Recordset!";
			log_info_color(log_color::red, ss.str().c_str());
		}
		if (row_count != 0)
		{
			std::stringstream ss;
			recordPtr = WCS_ADO_.GetRecordSet(SQL2);                               //get the task(RecordSet) with the highest priority

			recordPtr->MoveFirst();
			while (recordPtr->adoEOF == VARIANT_FALSE) {

				Confirm station;
				station.ID_ = atoi(_bstr_t(recordPtr->Fields->GetItem(_variant_t("ID"))->Value));;
				station.Storage_Name_ = (std::string)(_bstr_t)(recordPtr->Fields->GetItem(_variant_t("STATION_NAME"))->Value);
				station.Storage_Status_ = (std::string)(_bstr_t)(recordPtr->Fields->GetItem(_variant_t("STATUS"))->Value);
				(*confirm_list)[station.Storage_Name_] = station;
				recordPtr->MoveNext();
			}
			return true;
		}
		else {
			return false;
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "loadWcsTableListFromDB" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}

	return false;
}

//耿奕旻 写入等待预搜索的任务
bool WCS_Database::insertPreSearchTask(int taskId, std::string from, std::string to, int priority, std::string agv_type, std::string task_type, int agvId, std::string status, std::string error_info)
{
	try
	{
		std::stringstream ssOrder;
		
		ssOrder << "INSERT INTO pre_search_task_in_table(task_id, task_from,task_to, priority, agv_type, task_type,agv_id,task_status,error_info)VALUES("
				<< taskId << ",'" << from << "','" << to << "'," << priority << ",'" << agv_type << "','" << task_type<< "'," << agvId << ",'"<<status<<"','"<<error_info<<"')";

		_bstr_t SQL = ssOrder.str().c_str();
		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "INSERT INTO pre_search_task_in_table" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}



//9: 成功  Case 10:预搜索失败
bool WCS_Database::readPreSearchTaskResult(int taskId, std::string &target, int &status)
{
	try
	{
		std::stringstream ss;
		ss << "SELECT COUNT(*) FROM pre_search_task_feedback_table where task_id=" << taskId << " ORDER BY id DESC";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = WCS_ADO_.GetRecordSet(SQL);                               //get the task(RecordSet) with the highest priority

		if (recordPtr_)
		{
			if ((int)recordPtr_->GetCollect("COUNT(*)") != 0) {
				std::stringstream ss2;
				ss2 << "SELECT * FROM pre_search_task_feedback_table where task_id=" << taskId << " ORDER BY id DESC LIMIT 1";//AND task_to='" << target << "'

				_bstr_t SQL2 = ss2.str().c_str();

				recordPtr2_ = WCS_ADO_.GetRecordSet(SQL2);
				recordPtr2_->MoveFirst();
				
				while (recordPtr2_->adoEOF == VARIANT_FALSE) {
					std::vector<std::string> record;
					int task_status = atoi((_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("task_status"))->Value));
					target = (std::string)(_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("task_to"))->Value);
					status = task_status;
					recordPtr2_->MoveNext();
				}
				return true;
			}
		}
	}
	catch (...) {
		log_info_color(log_color::red, "readPreSearchTaskResult query error:" + std::to_string(taskId) );
		return false;
	}
	return false;
}

//耿奕旻 清除预搜索结果
bool WCS_Database::RemovePreSearchTaskResult(int agvId) 
{
	try
	{
		// 先删除尚未下发的预搜索
		std::stringstream ss1;
		ss1 << "DELETE FROM pre_search_task_in_table WHERE agv_id=" << agvId;
		_bstr_t SQL1 = ss1.str().c_str();
		// 消除已下发的预搜索
		std::stringstream ss;
		ss << "INSERT INTO task_op_interface(object, agv_id, task_id, op_code) VALUES('map'," << agvId << ",0,'" << "reset agv"<<"')";
		_bstr_t SQL = ss.str().c_str();
		return WCS_ADO_.ExecuteSQL(SQL1) && WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "RemovePreSearchTaskResult" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

// 取消 “取消预搜路请求” 
bool WCS_Database::cancelRemovePreSearchTask(int agvId)
{
	try
	{
		std::stringstream ss1;
		ss1 << "DELETE FROM task_op_interface WHERE agv_id=" << agvId << " AND `object` = 'map' AND op_code='reset agv'";
		_bstr_t SQL1 = ss1.str().c_str();
		return WCS_ADO_.ExecuteSQL(SQL1);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "cancelRemovePreSearchTask" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
		return false;
	}
}

bool WCS_Database::changeStorageTrayType(std::string storage_name, std::string type)
{
	try
	{
		std::string table_name = "s_equip_storage";
		std::stringstream ss;
		ss << "UPDATE " << table_name << " SET TRAY_TYPE = '" << type << "' WHERE EQUIP_NAME = '" << storage_name << "'";
		_bstr_t SQL = ss.str().c_str();
		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "changeStorageTrayType" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return true;
}

bool WCS_Database::changeStorageMsg(std::string storage_name, std::string msg)
{
	try
	{
		std::string table_name = "s_storage_station_status";
		std::stringstream ss;
		ss << "UPDATE " << table_name << " SET msg = '" << msg << "' WHERE storage_name = '" << storage_name << "'";
		_bstr_t SQL = ss.str().c_str();
		return WCS_ADO_.ExecuteSQL(SQL);
	}
	catch (_com_error &e)
	{
		std::stringstream ss;
		ss << e.ErrorMessage() << "changeStorageMsg" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return true;
}