﻿#ifndef PLC_MANAGE_H
#define PLC_MANAGE_H

#include <vector>
#include <deque>

#include "DatabaseManage.h"

typedef enum _SEND_STATUS
{
	SEND_SUCCESS,// 发送成功
	SEND_WAIT,// 等待发送
	SEND_FAILED,// 发送失败
}SEND_STATUS;

#define AREA "AREA"
#define TRAY "TRAY"

class finsTcpCommand;

class PLC_Manage
{
	typedef enum _TABLE_FIELDS
	{
		DEV_ID,
		DEV_NAME,
		DEV_TYPE,
		PLC_IP,
		PLC_PORT,
		PLC_VIRTUAL_PORT,
		PLC_TRIGGER,
		PLC_REMOTE_NODE,
		MSG_STATUS,
		READ_STATUS
	}TABLE_FIELDS;// 数据库字段，更改时需与构造函数同步更改


public:

	PLC_Manage();

	~PLC_Manage();
	// 初始化
	bool Initial();
	// 测试连接，与plc_status_相关
	bool ReCononectPLC();
	// 最大错误连接次数
	unsigned int max_fail_connect_ = 5;

	// 发送消息至PLC
	bool SendMsg(std::string dev_name, int status, bool auto_reset = true);
	bool SendMsg(std::string dev_name, bool status, bool auto_reset = true);
	// 从PLC读消息
	bool ReadMsg(std::string dev_name, int * plc_status, bool auto_reset = true);
	bool ReadMsg(std::string dev_name, bool * plc_status, bool auto_reset = true);

	std::map<std::string, std::map<int, std::string>> GetPLCMap();

	// 重置PLC状态

	void ResetSendStatus(std::string plc_name);
	void ResetReadStatus(std::string plc_name);


	void UpdateStorage(WCS_Database *WCS_CON = nullptr);
	bool StopScan();// 关闭线程
private:

	const std::string table_name_ = "kh_plc_data";

	const std::vector<std::string> table_fields_ = {
	"DEV_ID",
	"DEV_NAME",
	"DEV_TYPE",
	"PLC_IP",
	"PLC_PORT",
	"PLC_VIRTUAL_PORT",
	"PLC_TRIGGER",
	"PLC_REMOTE_NODE",
	"MSG_STATUS",
	"READ_STATUS"
	};

	const std::string TRUE_STR = "TRUE";

	const std::string FALSE_STR = "FALSE";

	const std::string BOOL_TYPE = "BOOL";

	const std::string SHORT_TYPE = "SHORT";

	const std::string AUTO = "AUTO";

	const std::string IDLE = "IDLE";
	// 发送状态
	const std::string SENDING = "SENDING";
	const std::string SENDED = "SENDED";
	// 读取状态
	const std::string READING = "READING";
	const std::string READED = "READED";

	unsigned int max_scan = 10;

	unsigned int need_times = 8;

	WCS_Database *WCS_CON_;

	std::map<std::string, finsTcpCommand* > all_con;// 所有连接


	bool plc_status_;

	unsigned int fail_count_ = 0;

	// 获取连接，与all_con相关
	bool GetCononect();

	// 强行从数据库读数据
	bool ForcedReadMsg(std::string plc_name);

	bool ForcedReadIOMsg(std::map<std::string, std::map<std::string, std::vector<std::string>>> plc_list, bool auto_update = true);

	bool SendMsg(std::string dev_name, std::string status, bool auto_reset = true);

	void FailConnectCount();

	bool TestCononect();

	// 线程相关
	mutable boost::shared_mutex entry_mutex;
	
	std::map<std::string, std::map<int, std::string>> plc_map_;

	mutable boost::shared_mutex storage_mutex;

	std::map<std::string, std::deque<std::string>> storage_plc_ = {
	{"A1", {} },
	{"A2", {} },
	{"A3", {} },
	{"A4", {} },
	{"A5", {} },
	{"A6", {} },
	{"A7", {} },
	{"A8", {} },
	{"A9", {} },
	{"A10", {} },
	{"A11", {} },
	{"A12", {} }
	};

	boost::shared_ptr<boost::thread> plc_status_update_;

	void StartScan();// 开启线程，更新plc_names_need_update_ 中指定的PLC状态


	void timerEvent();

	bool DatabaseIO(WCS_Database *WCS_CON = nullptr);

	bool SendMsgToPLC();

	void ReadMsgFromPLC(bool auto_update = false);

	void SortReadIOPlcInfo(std::map<std::string, std::map<std::string, std::vector<std::string>>> &plc_lists, bool auto_update = false);
	// 线程
};

typedef boost::serialization::singleton<PLC_Manage> Singleton_PLC_Manage;
#define PLC_MANAGE Singleton_PLC_Manage::get_mutable_instance()

#endif //PLC_MANAGE_H
