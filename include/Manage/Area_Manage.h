#ifndef AREA_MANAGE_H_
#define AREA_MANAGE_H_
#include <string>
#include <map>

#include <boost/serialization/singleton.hpp>

#include "Manage/Manage_Define.h"
#include "ADODatabase/WCS_Database.h"
#include "ADODatabase/MAP_Database.h"

class Area_Manage
{
public:
	
    Area_Manage::Area_Manage() {};
	Area_Manage::~Area_Manage() {};

	bool initial();
	// AGV是否在区域中
	bool isInArea(AGV point_agv, std::string area_id);

	inline bool isInArea(AGV *point_agv, std::string area_id) { return isInArea(*point_agv, area_id); };

	// 区域中所有AGV信息
	std::map<int, AGV*> agvsInfoInArea(std::string area_id);
	// AGV所在的所有的区域
	std::map<std::string, Area_Info> areasInfoOnAgv(int agv_id);
	// 区域中AGV数量
	inline int agvNumInArea(std::string area_id) { return agvsInfoInArea(area_id).size(); };

	// 区域中仅有指定AGV
	inline bool onlyPointAgvInArea(AGV point_agv, std::string area_id) { return agvNumInArea(area_id) == 1 && isInArea(point_agv, area_id); };
	inline bool onlyPointAgvInArea(AGV *point_agv, std::string area_id) { return onlyPointAgvInArea(*point_agv, area_id); };

	// 指定站点最近的AGV
	AGV* nearestAgv(std::string station_name);
	// 指定站点最近的空闲AGV
	AGV* nearestFreeAgv(std::string station_name);
	// 指定站点最远的AGV
	AGV* farthestAgv(std::string station_name);
	// 指定站点最远的空闲AGV
	AGV* farthestFreeAgv(std::string station_name);
	// 与指定站点距离排序
	std::vector<int> sortByDistance(std::string station_name);
	// 与指定站点距离
	double getDistance(std::string station_name, int agv_id);
	// 指定站点位置
	bool stationPos(std::string station_name, Pos_Info& pos);
	std::map<std::string, Pos_Info> all_station;
	// 计算两点距离的平方
	double calDistance(Pos_Info a, Pos_Info b);

	bool areaControl();

	bool areaControlOrderDispatch(std::string station_name, AGV point_agv);

	bool isPointInPolygon(Pos_Info p, std::vector<Pos_Info> points);

	bool isPointInPolygon(Pos_Info pos, std::string area_id);
	
	bool getCurrentAreaExistAGV(std::string area_id);

private:

	WCS_Database* WCS_CON_;
	MAP_Database* MAP_CON_;

	bool readAreaInfo();

	std::map<std::string, Area_Info> all_area_;
};

typedef boost::serialization::singleton<Area_Manage> Singleton_Area_Manage;
#define AREA_MANAGE Singleton_Area_Manage::get_mutable_instance()

#endif // #ifndef API_CLIENT_H_