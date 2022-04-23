#include "Area_Manage.h"
#include <boost/algorithm/string.hpp>
#include "comm/simplelog.h"

#include "Manage/AGV_Manage.h"
#include "ADODatabase/DatabaseManage.h"

bool Area_Manage::initial() {
	WCS_CON_ = DATABASE_MANAGE.Get_WCS_DB();
	MAP_CON_ = DATABASE_MANAGE.Get_MAP_DB();
	return WCS_CON_ && MAP_CON_;
}

//************************************
// Method:     inArea
// FullName:   Area_Manage::inArea
// Access:     public 
// Returns:    bool
// Parameter:  AGV point_agv
// Parameter:  std::string area_id
// Author:     Meixu
// Date:	   2021/08/04
// Description:判断车是否在编号为 area_id 的区域中
//************************************
bool Area_Manage::isInArea(AGV point_agv, std::string area_id) {
	if (all_area_.empty())
	{
		if (!readAreaInfo())
		{
			return false;
		}
	}
	if (all_area_.find(area_id) == all_area_.end())
	{
		return false;
	}
	return isPointInPolygon(point_agv.AGV_Pos_, area_id);
}


bool Area_Manage::isPointInPolygon(Pos_Info pos, std::string area_id) {
	if (all_area_.empty())
	{
		if (!readAreaInfo())
		{
			return false;
		}
	}
	return isPointInPolygon(pos, all_area_[area_id].pos_list);
}

//************************************
// Method:     agvsInArea
// FullName:   Area_Manage::agvsInArea
// Access:     public 
// Returns:    std::map<int, AGV*>
// Parameter:  int area_id
// Author:     Meixu
// Date:	   2021/08/04
// Description:
//************************************
std::map<int, AGV*> Area_Manage::agvsInfoInArea(std::string area_id) {
	std::map<int, AGV*> result;
	if (all_area_.empty())
	{
		if (!readAreaInfo())
		{
			return result;
		}
	}
	std::map<int, AGV*> AGV_List_ = AGV_MANAGE.Get_All_AGV();
	for each (auto point_agv in AGV_List_)
	{
		if (isInArea(point_agv.second, area_id))
		{
			result.insert(point_agv);
		}
	}
	return result;
}

std::map<std::string, Area_Info> Area_Manage::areasInfoOnAgv(int agv_id) {
	std::map<std::string, Area_Info> result;
	std::map<int, AGV*> AGV_List_ = AGV_MANAGE.Get_All_AGV();
	for each (auto var in all_area_)
	{
		if (isInArea(AGV_List_[agv_id], var.first))
		{
			result.insert(var);
		}
	}
	return result;
}

bool Area_Manage::getCurrentAreaExistAGV(std::string area_id)
{
	std::map<int, AGV*> AGV_List_ = AGV_MANAGE.Get_All_AGV();
	for (std::map<int, AGV*>::iterator it = AGV_List_.begin(); it != AGV_List_.end(); ++it)
	{
		AGV* current_agv = it->second;

		if (isInArea(current_agv, area_id))
		{
			return true;
		}
	}
	return false;
}

//************************************
// Method:     readAreaInfo
// FullName:   Area_Manage::readAreaInfo
// Access:     private 
// Returns:    bool
// Author:     Meixu
// Date:	   2021/08/04
// Description:更新所有区域信息
//************************************
bool Area_Manage::readAreaInfo() {
	if (!WCS_CON_)
	{
		initial();
	}
	if (WCS_CON_)
	{
		all_area_.clear();
		return WCS_CON_->readAreaInfo(all_area_);
	}
	return false;
}

//************************************
// Method:     isPointInPolygon
// FullName:   Area_Manage::isPointInPolygon
// Access:     private 
// Returns:    bool
// Parameter:  Pos_Info p
// Parameter:  std::vector<Pos_Info> points
// Author:     Meixu
// Date:	   2021/08/04
// Description: 判断点是否在闭合区域内
//************************************
bool Area_Manage::isPointInPolygon(Pos_Info p, std::vector<Pos_Info> points)
{
	//vector<Point> points:表示闭合区域由这些点围成
	if (points.empty())
	{
		return false;
	}
	double minX = points[0].x;
	double maxX = points[0].x;
	double minY = points[0].y;
	double maxY = points[0].y;
	for (unsigned int i = 1; i < points.size(); i++)
	{
		Pos_Info q = points[i];
		minX = min(q.x, minX);
		maxX = max(q.x, maxX);
		minY = min(q.y, minY);
		maxY = max(q.y, maxY);
	}

	if (p.x < minX || p.x > maxX || p.y < minY || p.y > maxY)
	{
		return false;
	}

	bool inside = false;
	for (unsigned int i = 0, j = points.size() - 1; i < points.size(); j = i++)
	{
		if ((points[i].y > p.y) != (points[j].y > p.y) &&
			p.x < (points[j].x - points[i].x) * (p.y - points[i].y) / (points[j].y - points[i].y) + points[i].x)
		{
			inside = !inside;
		}
	}

	return inside;
}


//************************************
// Method:     nearestAgv
// FullName:   Area_Manage::nearestAgv
// Access:     public 
// Returns:    int
// Parameter:  std::string station_name
// Author:     Meixu
// Date:	   2021/08/05
// Description:获取指定station附近最近的AGV编号；没有时返回nullptr
//************************************
AGV* Area_Manage::nearestAgv(std::string station_name) {
	int result = -1;
	for each (auto var in sortByDistance(station_name))
	{
		result = var;
		break;
	}
	std::map<int, AGV*> AGV_List_ = AGV_MANAGE.Get_All_AGV();
	return result == -1 ? nullptr : AGV_List_.at(result);
}

//************************************
// Method:     farthestFreeAgv
// FullName:   Area_Manage::farthestFreeAgv
// Access:     public 
// Returns:    int
// Parameter:  std::string station_name
// Author:     Meixu
// Date:	   2021/08/05
// Description:获取指定station附近最远的空闲AGV编号；没有时返回nullptr
//************************************
AGV* Area_Manage::farthestFreeAgv(std::string station_name) {
	int result = -1;
	std::map<int, AGV*> AGV_List_ = AGV_MANAGE.Get_All_AGV();
	for each (auto var in sortByDistance(station_name))
	{
		if (AGV_List_.at(var)->Resource_Status_ == RESOURCE_STATUS::UNLOCKED && AGV_List_.at(var)->Is_Online_ == 1 && AGV_List_.at(var)->Is_Serving_ == 1 && AGV_List_.at(var)->AGV_Status_ == 0) {
			result = var;
		}
	}
	return result == -1 ? nullptr : AGV_List_.at(result);
}

//************************************
// Method:     farthestAgv
// FullName:   Area_Manage::farthestAgv
// Access:     public 
// Returns:    int
// Parameter:  std::string station_name
// Author:     Meixu
// Date:	   2021/08/05
// Description:获取指定station附近最远的AGV编号；没有时返回nullptr
//************************************
AGV* Area_Manage::farthestAgv(std::string station_name) {
	int result = -1;
	for each (auto var in sortByDistance(station_name))
	{
		result = var;
	}
	std::map<int, AGV*> AGV_List_ = AGV_MANAGE.Get_All_AGV();
	return result == -1 ? nullptr : AGV_List_.at(result);
}

//************************************
// Method:     sortByDistance
// FullName:   Area_Manage::sortByDistance
// Access:     public 
// Returns:    std::vector<int>
// Parameter:  std::string station_name
// Author:     Meixu
// Date:	   2021/08/05
// Description:获取指定station名,按距离排序的所有AGV
//************************************
std::vector<int> Area_Manage::sortByDistance(std::string station_name)
{
	std::vector<int> result;// 结果
	Pos_Info station;
	if (stationPos(station_name, station))
	{
		std::map<int, double> distance;// AGV号对应的距离
		std::map<int, AGV*> AGV_List_ = AGV_MANAGE.Get_All_AGV();
		for each (auto var in AGV_List_)
		{
			distance[var.first] = calDistance(var.second->AGV_Pos_, station);
		}
		// 冒泡
		while (distance.size() >= 2)
		{
			int max = distance.begin()->first, min = distance.begin()->first;
			for each (auto var in distance)
			{
				if (distance.at(max) < var.second)
				{
					max = var.first;
				}
				else if (distance.at(min) > var.second)
				{
					min = var.first;
				}
			}
			result.insert(result.begin() + (int)((AGV_List_.size() - distance.size()) / 2), min);
			result.insert(result.begin() + (int)((AGV_List_.size() - distance.size()) / 2 + 1), max);
			distance.erase(max);
			distance.erase(min);
		}

	}

	return result;
}

double Area_Manage::getDistance(std::string station_name, int agv_id) {
	Pos_Info station;
	if (stationPos(station_name, station))
	{
		std::map<int, AGV*> AGV_List_ = AGV_MANAGE.Get_All_AGV();
		return calDistance(station, AGV_List_[agv_id]->AGV_Pos_);
	}
	else
	{
		return -1;
	}
}

bool Area_Manage::stationPos(std::string station_name, Pos_Info &pos) {
	if (all_station.find(station_name) == all_station.end())
	{
		pos = { DBL_MAX ,DBL_MAX ,DBL_MAX };
		std::map<std::string, Pos_Info> station;
		if (!MAP_CON_)
		{
			initial();
		}
		if (MAP_CON_ && MAP_CON_->getStationPos(station))
		{
			all_station = station;
		}
		else
		{
			// 获取点位失败
			MAP_CON_ = DATABASE_MANAGE.Get_MAP_DB("tmp" + station_name);
			log_error("get pos fail, reconnect :" + station_name);
			return false;
		}
	}
	else
	{
		pos = all_station[station_name];
	}
	return true;
}

// 计算两点间距离
double Area_Manage::calDistance(Pos_Info a, Pos_Info b) {
	return ((a.x - b.x)*(a.x - b.x)) + (a.y - b.y)*(a.y - b.y);
}

bool Area_Manage::areaControl() {
	readAreaInfo();
	for each (auto var in all_area_)
	{
		auto area = var.second;
		if (area.area_type.find("area control") != std::string::npos)
		{
			// 获取同时允许运行数量
			unsigned int control_num  = 1;
			std::vector<std::string> area_control_num;
			boost::split(area_control_num, area.area_type, boost::is_any_of(":"));
			if (area_control_num.size() == 2)
			{
				control_num = std::atoi(area_control_num[1].c_str());
			}
			// 获取区域之前的AGV信息
			std::vector<std::string> running_agv_list;
			if (!area.info.empty())
			{
				boost::split(running_agv_list, area.info, boost::is_any_of(","));
			}
			// 获取当前区域内所有AGV信息
			std::map<int, AGV*> agv_list = agvsInfoInArea(area.area_name);
			// 不存在的AGV出队
			for (auto i = running_agv_list.begin(); i != running_agv_list.end();)
			{
				// 遍历之前保存的信息
				if (!i->empty() && agv_list.find(std::atoi(i->c_str())) == agv_list.end())
				{
					// 发现不在列表中了,退出队列,并通知继续运行
					AGV_MANAGE.areaSetAgvContinue(std::atoi(i->c_str()));
					log_info("[Area_Manage] : agv " + *i + " exit area :" + area.area_name);
					i = running_agv_list.erase(i);
				}
				else
				{
					i++;
				}
			}
			// 新加入的AGV并入队列
			for each (auto new_agv in agv_list)
			{
				if (std::find(running_agv_list.begin(), running_agv_list.end(), std::to_string(new_agv.second->AGV_ID_)) == running_agv_list.end())
				{
					// 之前没有该车,入队
					running_agv_list.push_back(std::to_string(new_agv.second->AGV_ID_));
					log_info("[Area_Manage] : agv " + std::to_string(new_agv.second->AGV_ID_) + " enter area : " + area.area_name);
				}
			}
			area.info = "";
			// 所有区域内符合数量要求的,继续,否则,停下
			for (unsigned int i = 0; i < running_agv_list.size(); i++)
			{
				if (!running_agv_list.at(i).empty())
				{
					if (i < control_num)
					{
						AGV_MANAGE.areaSetAgvContinue(std::atoi(running_agv_list.at(i).c_str()));
					}
					else
					{
						AGV_MANAGE.areaSetAgvPause(std::atoi(running_agv_list.at(i).c_str()));
					}
					area.info += running_agv_list.at(i) + ",";
				}
			}
			// 写入数据库
			WCS_CON_->setAreaInfo(area.area_name, area.info);
		}
	}
	return true;
}

// 需要管制返回true
bool Area_Manage::areaControlOrderDispatch(std::string station_name, AGV point_agv) {
	Pos_Info station_pos;
	if (!stationPos(station_name, station_pos))
	{
		// 没有找到对应站点，直接退出，并返回没有管制
		return false;
	}
	for each (auto var in all_area_)
	{
		auto area_id = var.first;
		if (var.second.area_type == "order dispatch control")
		{
			// 找到管制区域
			if (isPointInPolygon(station_pos, area_id))
			{
				// 任务在区域内
				if (isPointInPolygon(point_agv.AGV_Pos_, area_id))
				{
					// AGV也在该区域。允许
					return false;
				}
				else
				{
					// AGV不在管制区域内，禁止
					return true;
				}
			}
		}
	}
	// 没有管制区域或者起点不在管制区域内
	return false;
}