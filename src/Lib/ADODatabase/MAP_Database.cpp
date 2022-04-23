#ifndef no_init_all
#define no_init_all deprecated
#endif // no_init_all
#include "MAP_Database.h"

#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "stdlib.h"
#include "ADODatabase/DatabaseManage.h"
#include "comm/simplelog.h"
#include <Windows.h>
#include "Manage/Config_Manage.h"

MAP_Database::MAP_Database()
{

}

MAP_Database::~MAP_Database()
{

}

void MAP_Database::setMapAdo(ADOcon ADO)
{
	MAP_ADO_ = ADO;
}

bool MAP_Database::getStationPos(std::map<std::string, Pos_Info> &station)
{
	bool result = false; 
	try
	{
		std::stringstream ss;
		ss << "SELECT COUNT(*) FROM station_info";
		_bstr_t SQL = ss.str().c_str();
		recordPtr_ = MAP_ADO_.GetRecordSet(SQL);
		if (recordPtr_)
		{
			if ((int)recordPtr_->GetCollect("COUNT(*)") != 0) {
				std::stringstream ss2;
				ss2 << "SELECT * FROM station_info";
				_bstr_t SQL2 = ss2.str().c_str();

				recordPtr2_ = MAP_ADO_.GetRecordSet(SQL2);
				recordPtr2_->MoveFirst();

				while (recordPtr2_->adoEOF == VARIANT_FALSE) {

					Pos_Info pos;
					pos.x = atof((_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("pos_x"))->Value));
					pos.y = atof((_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("pos_y"))->Value));
					std::string station_name = (std::string)(_bstr_t)(recordPtr2_->Fields->GetItem(_variant_t("station_name"))->Value);
					// 这里直接对 storage_map 写入
					station[station_name] = pos;
					//Current_Storage_List[new_storage.ID_] = new_storage;
					recordPtr2_->MoveNext();
				}
				//storage_map = Current_Storage_List;
				return true;


				result = true;
				
			}
		}
	}
	catch (_com_error &e) {
		std::stringstream ss;
		ss << e.ErrorMessage() << "getStationPos" << e.Description();
		log_info_color(log_color::red, ss.str().c_str());
	}
	return result;
}