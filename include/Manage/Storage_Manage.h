#ifndef _STORAGE_MANAGE_HSQ
#define _STORAGE_MANAGE_HSQ
#include <string>  
#include <map> 
#include <vector>
#include <iostream>  
#ifndef Q_MOC_RUN
#include <boost/serialization/singleton.hpp>
#include <boost/thread.hpp>
#endif
#include "Manage/Manage_Define.h"
#include "ADODatabase/WCS_Database.h"
#include "ADODatabase/ACS_Database.h"
//#include "ADODatabase/XML_Database.h"

class Storage_Status_Notifier
{
public:
	virtual void OnstorageStatusChanged(Storage) = 0;
};

class Storage_Manage 
{  
public:  

	Storage_Manage();  

	~Storage_Manage();

	void Initial();

	bool setCurrentStorageStatus(std::string table_name, std::string resource_status, std::string storage_name);

	int GetAgvExeGoodsTask(std::string palletNo);

	std::vector<Sql_Table_Attri> getSqlableList() const;

	std::map<std::string, std::map<int, Storage>> getStorageListMap();

	void setStorageStatusNotifier(Storage_Status_Notifier *notifier);

	void checkChange();
	
	std::string readStorageStatus(std::string storage_name);
	//从界面修改数据库的数据;
	void changeStorageStatus(std::string storage_name, std::string status_name, std::string status);

	bool changeStorageTrayType(std::string storage_name, std::string type);
	std::vector<std::string> getAllStationName();//获取所有站点名，加载界面时调用

	std::vector<std::string> getAllStationName(std::string table_name /*= ""*/);
	std::vector<std::string> getStandardStorage(const std::string & status);

	std::string getStorageStatus(const std::string & storage_name);

protected:  


private:

	std::map<int, AGV*> AGV_List_;

	WCS_Database* WCS_CON_;
	ACS_Database* ACS_CON_;
	//XML_Database* XML_CON_;

	//接口类实例，用作更新界面显示的库位状态
	Storage_Status_Notifier *storage_status_notifier_;

	//从数据库加载的需要从数据库加载的表的结构体;
	std::vector<Sql_Table_Attri> sql_table_list_;

	//用来盛装创建用来盛装Storage结构体的map表的map
	std::map<std::string, std::map<int, Storage>> storage_list_map_;

	std::map<std::string, std::map<int, Storage>> loadStorageMapList();

}; 

typedef boost::serialization::singleton<Storage_Manage> Singleton_Storage_Manage;
#define STORAGE_MANAGE Singleton_Storage_Manage::get_mutable_instance()

#endif //_AGV_MANAGE_ED_H