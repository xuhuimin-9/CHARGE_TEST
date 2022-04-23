#pragma once
#ifndef MODE_MANAGE_H
#define MODE_MANAGE_H

#include <vector>
#include <string>

#include <boost/serialization/singleton.hpp>

#include "DatabaseManage.h"
#include "ADODatabase/WCS_Database.h"
#include "Manage/Manage_Define.h"
#include "ADODatabase/ACS_Database.h"
#include "WCS_Database.h"

class Mode_Manage
{

public:
	
	Mode_Manage();
	~Mode_Manage();

	void Initial();

	std::vector<std::string>  getAllTaskName();//获取所有任务名，加载界面时调用

private: 	
	WCS_Database *wcs_db_;

	std::string tableName = "mode_manage";

	std::string listName = "TASK_NAME";

	std::vector<std::string> taskNameList;

};
typedef boost::serialization::singleton<Mode_Manage> Singleton_Mode_Manage;
#define MODE_MANAGE Singleton_Mode_Manage::get_mutable_instance()

#endif

