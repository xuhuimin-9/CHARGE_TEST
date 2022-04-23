#include "Common/Common.h"
#include "ADODatabase/DatabaseManage.h"
#include "Manage/Order_Manage.h"  
#include "Manage/Storage_Manage.h"
#include "comm/simplelog.h"

Order_Manage::Order_Manage()
{

}

Order_Manage::~Order_Manage()
{

}

void Order_Manage::Initial()
{
	WCS_CON_ = DATABASE_MANAGE.Get_WCS_DB();
	//所有 doing 全部error
	WCS_CON_->updateOrderStatusToErr();
}

void Order_Manage::Download_Order_Info_From_Database_Event()
{
	WCS_CON_->updateOrderInfo();
	WCS_CON_->updateNewOrderInfo();
}

bool Order_Manage::Add_to_full_list_if_not_exist(Order current_order)
{
	std::map<std::string, Order>::iterator it=Full_Order_List_.find(current_order.ORDER_ID_);
	if(it!=Full_Order_List_.end())
	{
		it->second = current_order;
		return false;
	}
	else{
		Full_Order_List_[current_order.ORDER_ID_]=current_order;
		return true;
	}
}

bool Order_Manage::QueryOrderInfo(std::string order_id, Order *order_in) {
	if (Full_Order_List_.find(order_id) != Full_Order_List_.end())
	{
		*order_in = Full_Order_List_[order_id];
		return true;
	}
	else
	{
		return false;
	}
}

bool Order_Manage::Get_All_NEW_Order(Order current_order)
{	
	std::map<std::string, Order>::iterator it= New_Order_List_.find(current_order.ORDER_ID_);
	if(it!= New_Order_List_.end())
	{
		it->second = current_order;
		return false;
	}
	else
	{
		New_Order_List_[current_order.ORDER_ID_]=current_order;
		return true;
	}
}

bool Order_Manage::Delete_DONE_From_Full_Order_List(std::string order_id)
{
	Full_Order_List_.erase(order_id);
	return true;
}

std::map<std::string, Order> Order_Manage::Get_All_Order()
{
	return Full_Order_List_;
}
 
std::map<std::string, Order> Order_Manage::getNewOrder()
{
	return New_Order_List_;
}

void Order_Manage::setNewOrder(std::map<std::string, Order> order_map)
{
	New_Order_List_ = order_map;
}

bool Order_Manage::checkOrderList(AGV* current_AGV)
{
	if (WCS_CON_->checkNewOrderList(current_AGV))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Order_Manage::Revoke_Order(std::string order_id)
{
	return WCS_CON_->revokeDodOrder(order_id);
}

bool Order_Manage::RedoErrOrder(std::string order_id)
{
	return WCS_CON_->redoErrOrder(order_id);
}

bool Order_Manage::getParkingStation(std::string &parking_station, std::string &is_charging, int agv_type)
{
	if (WCS_CON_->getCurrentParkingStation(parking_station, is_charging, agv_type))
	{
		std::stringstream ss;
		ss << "get Current Parking Station successful " << parking_station;
		log_info_color(log_color::green, ss.str().c_str());
		return true;
	}
	else
	{
		return false;
	}
}

void Order_Manage::reserveParkingStation(std::string parking_station)
{
	WCS_CON_->reserveCurrentParkingStation(parking_station);
}

void Order_Manage::releaseParkingStation(std::string parking_station)
{
	WCS_CON_->releaseCurrentParkingStation(parking_station);
}

bool Order_Manage::getStationIsCharging(std::string parking_station)     //true:当前停车点没有充电桩;
{
	if (WCS_CON_->getCurrentStationIsCharging(parking_station))
	{
		/*std::stringstream ss;
		ss << "get Current Station Is Charging successful " << parking_station;
		log_info_color(log_color::green, ss.str().c_str());*/
		return true;
	}
	else
	{
		/*std::stringstream ss;
		ss << "get Current Station Is Charging failed ";
		log_info_color(log_color::red, ss.str().c_str());*/
		return false;
	}
}


bool Order_Manage::getMustChargingStation(std::string &parking_station)
{
	if (WCS_CON_->getCurrentMustChargingStation(parking_station))
	{
		std::stringstream ss;
		ss << "get Current Must Charging Station successful " << parking_station;
		log_info_color(log_color::green, ss.str().c_str());
		return true;
	}
	else
	{
		/*std::stringstream ss;
		ss << "get Current Must Charging Station failed ";
		log_info_color(log_color::red, ss.str().c_str());*/
		return false;
	}
}

bool Order_Manage::Insert_Order(std::string start, std::string target, int priority, std::string status, std::string mode, std::string type)
{
	return WCS_CON_->insertOrderToDb(start, target, priority, status, mode, type);
}

bool Order_Manage::Insert_Order(std::string order_id, std::string start, std::string target, int priority, std::string status, std::string mode, std::string type)
{
	return WCS_CON_->insertOrderToDb(order_id, start, target, priority, status, mode, type);
}


bool Order_Manage::Insert_Order(std::string order_id, std::string start, std::string target, int priority, std::string status, std::string mode, std::string type, int agvID)
{
	return WCS_CON_->insertOrderToDb(order_id, start, target, priority, status, mode, type, agvID);
}

bool Order_Manage::Insert_Order(std::string order_id, std::string start, std::string target, int priority, std::string status, std::string mode, std::string type, int agvID, std::string palletNo)
{
	return WCS_CON_->insertOrderToDb(order_id, start, target, priority, status, mode, type, agvID, palletNo);
}

int Order_Manage::getMaxTaskId()
{
	int max_order_id = 1;
	if (!WCS_CON_->checkOrderInfoCount(max_order_id))
	{
		max_order_id = WCS_CON_->getMaxWcsTaskId();
	}
	else
	{
		int report_max_id = WCS_CON_->getMaxWcsTaskId();

		if (max_order_id < report_max_id)
		{
			max_order_id = report_max_id;
		}
	}
	return max_order_id;
}

bool Order_Manage::updateOrderStatus(std::string new_status, std::string old_status, std::string start) {
	log_info(start + " : change line status to " + new_status);
	return WCS_CON_->updateOrderStatus(new_status, old_status, start);
}