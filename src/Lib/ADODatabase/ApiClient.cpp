#include "ApiClient.h"

#include <boost/serialization/singleton.hpp>
#include <boost/algorithm/string.hpp>
#include "DatabaseManage.h"
#include "comm/simplelog.h"
#include "Common/Common.h"
#include <QJsonObject>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonParseError>
#include "Core/Task_Chain_Manage.h"
#include <Manage/Order_Manage.h>
#include <Manage/AGV_Manage.h>
#include <Manage/Storage_Manage.h>
#include <Manage/Config_Manage.h>
#include <Config_Manage.h>
#include <ADODatabase/ApiServer.h>

#include <QJsonArray>
#include <QTextCodec>

//class ClientMessage;
/// <summary>
/// APIs this instance.
/// </summary>
/// <returns></returns>
/// Creator:MeiXu
/// CreateDate:2021/2/19
/// 

std::map<std::string, int> ApiClient::reponse_result = {};

ApiClient::ApiClient(QObject *parent) {
	// 默认前往 -1
	API_SERVER.InitialLauncher(-1, &this->UnknownOperation);
	// 请确保操作函数return前，msg->generateReturnBody调用完毕 [7/8/2021 Meixu]
	// 请确保返回内容不会嵌套层数过多，递归转化为xml时未作层数限制 [7/8/2021 Meir]
	API_SERVER.InitialLauncher("RECEIVE_CARRY_TASK", &this->InsertOrder);
	API_SERVER.InitialLauncher("PALLET_TASK_CANCEL", &this->RevokeOrder);
	//API_SERVER.InitialLauncher("TASK_PRIORITY_CHANGE", &this->UpdatePriority);
	API_SERVER.InitialLauncher("PALLET_PICK_DROP_TASK", &this->QueryIsCanGetOrPutReponse); 
	API_SERVER.InitialLauncher("REQUEST_DEVICE_INFO", &this->QueryAGVInfo);
	API_SERVER.InitialLauncher("REQUEST_ALL_DEVICE_INFO", &this->QueryAllAGVInfo);
	//自测任务完成接口调用
	API_SERVER.InitialLauncher("FORKLIFT_ACTION_COMPLETE", &this->TaskFinishReportTest);
	//自测任务完成接口调用
	API_SERVER.InitialLauncher("FORKLIFT_PICK_PUT", &this->QueryIsCanGetOrPutTest);
	CONFIG_MANAGE.readConfig("IPAddressAndPort", &IPAddressAndPort, "config.txt");
	if ("" == IPAddressAndPort)
	{
		//log_error_color(red, "read IPAddressAndPort happen error");
	}
}

/// <summary>
/// Finalizes an instance of the <see cref="Api"/> class.
/// </summary>
/// Creator:MeiXu
/// CreateDate:2021/2/19
/// 
ApiClient::~ApiClient()
{
}

/// <summary>
/// Gets the server ip.
/// </summary>
/// <returns></returns>
/// Creator:MeiXu
/// CreateDate:2021/2/19
/// 
QString ApiClient::get_server_ip()
{
	return server_ip;
}

/// <summary>
/// Sets the server ip.
/// </summary>
/// <param name="ip_address">The ip address.</param>
/// Creator:MeiXu
/// CreateDate:2021/2/19
/// 
void ApiClient::set_server_ip(QString ip_address)
{
	server_ip = ip_address;
}

QString ApiClient::CurrentTime() {
	return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

/// <summary>
/// Creates the UUID.
/// </summary>
/// <param name="data">The data.</param>
/// <returns></returns>
/// Creator:MeiXu
/// CreateDate:2021/2/7
QString ApiClient::createUuid(QString data) {
	qint64 timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
	QUuid id;
	id = QUuid::createUuidV5(id, QString(data + timestamp));
	return id.toString().replace("{", "").replace("}", "");
}

//////////////////////////////////////////////////////////////////////////
// 以下为向外提供的接口 [8/2/2021 Meir]

//下发订单
bool ApiClient::InsertOrder(ApiMsg* msg) {
	bool result = false;
	// 处理请求
	Order order_in;
	order_in.ORDER_ID_ = msg->ReadString(msg->taskContent, "taskNo");
	order_in.START_ = msg->ReadString(msg->taskContent, "fromPoint");
	order_in.TARGETED_ = msg->ReadString(msg->taskContent, "toPoint");
	order_in.MODE_ = msg->ReadString(msg->taskContent, "mode");
	order_in.PRIORITY_ = (int)msg->ReadNumber(msg->taskContent, "priority");
	/*if (order_in.START_=="DLK")
	{
		order_in.MODE_ = "OUT";
	}
	else if (order_in.TARGETED_=="DLA")
	{
		order_in.MODE_ = "IN";
	}
	else
	{
		order_in.MODE_ = "MOVE";
	}*/
	order_in.TYPE_ = "CARRY";
	//order_in.PALLETNO = msg->ReadString(msg->taskContent, "containerNo");
	result = ORDER_MANAGE.Insert_Order(order_in.ORDER_ID_, order_in.START_, order_in.TARGETED_, order_in.PRIORITY_, "NEW", order_in.MODE_, order_in.TYPE_, -1);
	std::string name = msg->operationName;//调用方
	// 构建响应消息
	msg->generateReturnBody(result, name, DatabaseError, VNAME(DatabaseError));

	return result;
}


bool ApiClient::RevokeOrder(ApiMsg * msg)
{
	bool result = false;
	// 处理请求
	std::string order_id = msg->ReadString(msg->taskContent, "taskNo");
	result = ORDER_MANAGE.Revoke_Order(order_id);
	std::string name = msg->operationName;//调用方
	// 响应消息
	msg->generateReturnBody(result, name,DatabaseError, VNAME(DatabaseError));
	return result;
}

bool ApiClient::QueryOrderInfo(ApiMsg* msg)
{
	bool result = false;
	// 处理请求
	std::string order_id = msg->ReadString(msg->taskContent, "orderId");
	Order* order = new Order();
	result = ORDER_MANAGE.QueryOrderInfo(order_id, order);
	std::string name = msg->operationName;//调用方
	QJsonObject a;
	// 响应消息
	if (result)
	{
		a.insert("orderId", order->ORDER_ID_.c_str());
		a.insert("start", order->START_.c_str());
		a.insert("target", order->TARGETED_.c_str());
		a.insert("priority", order->PRIORITY_);
		a.insert("status", order->STATUS_.c_str());
		a.insert("mode", order->MODE_.c_str());
		a.insert("agvId", order->AGV_ID_);
	}
	delete order;
	QJsonObject b;
	b.insert("taskContent", a);
	msg->generateReturnBody(result, name, DatabaseError, VNAME(DatabaseError), &b);
	return result;
}

//查询AGV状态
bool ApiClient::QueryAllAGVInfo(ApiMsg * msg)
{
	bool result = true;
	// 处理请求
	int agv_id = (int)msg->ReadNumber(msg->taskContent, "deviceNo");
	std::map<int, AGV*> AGV_list = AGV_MANAGE.Get_All_AGV();

	QJsonObject b;

	QJsonArray  jsArray;
	// 响应消息
	QTextCodec *codec;
	codec = QTextCodec::codecForName("GBK");
	auto iter = AGV_list.begin();
	QJsonObject deviceInfo;
	QJsonObject taskContent;
	for (iter; iter != AGV_list.end(); iter++)
	{

		deviceInfo.insert("deviceNo", QJsonValue(iter->second->AGV_ID_));
		if (iter->second->Is_Online_ != 1)
		{
			// 不在线
			deviceInfo.insert("status", QJsonValue("2"));
			deviceInfo.insert("errMsg", QJsonValue(codec->toUnicode("离线")));
		}
		else if (iter->second->Is_Serving_ != 1)
		{
			// AGVS没解锁
			deviceInfo.insert("status", QJsonValue("2"));
			deviceInfo.insert("errMsg", QJsonValue(codec->toUnicode("锁定")));
		}
		else if (iter->second->Error_Code_ != 0)
		{
			// 有报错
			deviceInfo.insert("status", QJsonValue("2"));
			deviceInfo.insert("errMsg", QJsonValue(codec->toUnicode("异常")));
		}
		else
		{
			deviceInfo.insert("status", QJsonValue("1"));
			deviceInfo.insert("errMsg", QJsonValue("正常"));
		}
		QJsonDocument doc(deviceInfo);
		QString strJson(doc.toJson(QJsonDocument::Compact));
		jsArray.append(QJsonValue(strJson));
	}

	taskContent.insert("deviceList", QJsonValue(jsArray));
	b.insert("taskContent", QJsonValue(taskContent));
	QJsonDocument doc2(b);
	QString strJson2(doc2.toJson(QJsonDocument::Compact));
	log_info_color(red, strJson2.toStdString());

	std::string name = msg->operationName;//调用方

	msg->generateReturnBody(result, name, DatabaseError, VNAME(DatabaseError), &b);
	return result;
}

bool ApiClient::QueryAGVInfo(ApiMsg * msg)
{
	bool result = true;
	// 处理请求
	int agv_id = (int)msg->ReadNumber(msg->taskContent, "deviceNo");
	std::map<int, AGV*> AGV_list = AGV_MANAGE.Get_All_AGV();

	QJsonObject b;

	QJsonArray  jsArray;
	// 响应消息
	QTextCodec *codec;
	codec = QTextCodec::codecForName("GBK");
	QJsonObject deviceInfo;
	auto iter = AGV_list.begin();
	/*for (iter; iter != AGV_list.end(); iter++)
	{
		if (iter->second->AGV_ID_ == agv_id)
		{
			deviceInfo.insert("deviceNo", QJsonValue(iter->second->AGV_ID_));
			if (iter->second->Is_Online_ != 1)
			{
				// 不在线
				deviceInfo.insert("status", QJsonValue("2"));
				deviceInfo.insert("errMsg", QJsonValue(codec->toUnicode("离线")));
			}
			else if (iter->second->Is_Serving_ != 1)
			{
				// AGVS没解锁
				deviceInfo.insert("status", QJsonValue("2"));
				deviceInfo.insert("errMsg", QJsonValue(codec->toUnicode("锁定")));
			}
			else if (iter->second->Error_Code_ != 0)
			{
				// 有报错
				deviceInfo.insert("status", QJsonValue("2"));
				deviceInfo.insert("errMsg", QJsonValue(codec->toUnicode("异常")));
			}
			else
			{
				deviceInfo.insert("status", QJsonValue("1"));
				deviceInfo.insert("errMsg", QJsonValue(""));
			}
			break;
		}
	}*/
	if (AGV_list.find(agv_id) != AGV_list.end())
	{
		deviceInfo.insert("agvId", AGV_list[agv_id]->AGV_ID_);
		deviceInfo.insert("batteryLevel", AGV_list[agv_id]->Battery_Level_);
		deviceInfo.insert("posX", AGV_list[agv_id]->AGV_Pos_.x);
		deviceInfo.insert("posY", AGV_list[agv_id]->AGV_Pos_.y);
		deviceInfo.insert("status", AGV_list[agv_id]->AGV_Status_);
		//deviceInfo.insert("speed", "-1");
		result = true;
	}

	b.insert("taskconnect", QJsonValue(deviceInfo));

	std::string name = msg->operationName;//调用方

	msg->generateReturnBody(result, name, DatabaseError, VNAME(DatabaseError), &b);
	return result;
}

//处理询问是否可以取货的响应
bool ApiClient::QueryIsCanGetOrPutReponse(ApiMsg * msg) {
	std::string taskNo = msg->ReadString(msg->taskContent, "taskNo");
	std::string point = msg->ReadString(msg->taskContent, "point");
	std::string flag = msg->ReadString(msg->taskContent, "flag");

	reponse_result[taskNo + point + flag + "QueryIsCanGetOrPut"] = 2;
	// 响应消息
	msg->generateReturnBody(true, "AGV",DatabaseError, VNAME(DatabaseError));
	return true;
}


//自测
bool ApiClient::TaskFinishReportTest(ApiMsg * msg)
{
	bool result = false;
	// 处理请求
	std::string order_id = msg->ReadString(msg->taskContent, "taskNo");
	std::string priority = msg->ReadString(msg->taskContent, "priority");
	result = true;
	QJsonObject b;
	b.insert("code", "demo111");
	b.insert("message", "1111111111111111111111111111111111111");
	// 响应消息
	msg->generateReturnBody(result, "AGV", DatabaseError, VNAME(DatabaseError), &b);
	return result;
}

//自测
bool ApiClient::QueryIsCanGetOrPutTest(ApiMsg * msg)
{
	bool result = false;
	// 处理请求
	std::string order_id = msg->ReadString(msg->taskContent, "taskNo");
	std::string priority = msg->ReadString(msg->taskContent, "priority");
	result = true;
	QJsonObject b;
	b.insert("code", "demo111");
	b.insert("message", "1111111111111111111111111111111111111");
	// 响应消息
	msg->generateReturnBody(result, "AGV", DatabaseError, VNAME(DatabaseError), &b);
	return result;
}




bool ApiClient::UnknownOperation(ApiMsg * msg)
{
	bool result = false;
	// TODO 打印日志
	// 响应消息
	std::string name = msg->operationName;//调用方
	msg->generateReturnBody(false, name, UnknownOperationCode, VNAME(UnknownOperationCode));
	return true;
}


//////////////////////////////////////////////////////////////////////////
// 以下为调用其他接口 [8/2/2021 Meixu]
bool ApiClient::EquipmentStatusChangeNotification(std::string agvId, std::string status, std::string errorCode, int agv_id) {
	// 组装消息
	QJsonObject json_data;
	json_data.insert("agvId", QJsonValue(QString::fromStdString(agvId)));
	QJsonObject body_data;
	body_data.insert("taskContent", json_data);

	// 发送请求并声明响应函数
	API_SERVER.send("127.0.0.1:625/", applicationsoapxml, body_data, &this->reply);
	return true;
}


//任务汇报完成
bool ApiClient::TaskFinishReport(std::string taskNo, std::string deviceNo, std::string point, std::string actionType)
{
	////判断是否为第一次调用
	//if (reponse_result.find(taskNo + "TaskFinishReport") == reponse_result.end())
	//{
	//	reponse_result.insert(std::map<std::string, int>::value_type(taskNo + "TaskFinishReport", 0));
	//}
	//else
	//{
	//	std::map<std::string, int>::iterator itor = reponse_result.find(taskNo + "TaskFinishReport");
	//	//已经收到回复
	//	if (2 == itor->second)
	//	{
	//		std::cout << "已经收到回复" << endl;
	//		return true;
	//	}
	//	//已经收到回复
	//	if (1 == itor->second)
	//	{
	//		return false;
	//	}

	//}
	// 组装消息
	QJsonObject json_data;

	json_data.insert("actionType", QJsonValue(QString::fromStdString(actionType)));
	json_data.insert("point", QJsonValue(QString::fromStdString(point)));
	json_data.insert("deviceNo", QJsonValue(QString::fromStdString(deviceNo)));
	json_data.insert("taskNo", QJsonValue(QString::fromStdString(taskNo)));
	//json_data.insert("containerNo", QJsonValue(QString::fromStdString(containerNo)));

	QJsonDocument doc(json_data);
	QString strJson(doc.toJson(QJsonDocument::Compact));

	QJsonObject body_data;
	//body_data.insert("taskJson", QJsonValue(strJson));
	body_data.insert("taskJson", QJsonValue(json_data));
	body_data.insert("bizType", QJsonValue("FORKLIFT_ACTION_COMPLETE"));
	body_data.insert("operatorName", QJsonValue("AGV"));
	body_data.insert("callCode", QJsonValue("ETG35IJHFIU19405"));
	if ("" != IPAddressAndPort)
	{
		// 发送请求并声明响应函数
		API_SERVER.send(IPAddressAndPort+"/Fms/fms_agv/AgvTaskReturn", applicationjson, body_data, &this->reply);
		QJsonDocument doc2(body_data);
		QString strJson2(doc2.toJson(QJsonDocument::Compact));
		log_info_color(red, strJson2.toStdString());
	}
	reponse_result[taskNo + "TaskFinishReport"] = 1;
	return true;
}

//询问取货、放货是否可以执行
bool ApiClient::QueryIsCanGetOrPut(std::string taskNo, std::string point, std::string flag) {

	if (all_request_mapping[taskNo] != point)
	{
		if (reponse_result.find(taskNo + point + flag+ "QueryIsCanGetOrPut") != reponse_result.end())
		{
			reponse_result.erase(taskNo + point + flag + "QueryIsCanGetOrPut");
		}
	}
	all_request_mapping[taskNo] = point;
	//判断是否为第一次调用
	if (reponse_result.find(taskNo + point + flag + "QueryIsCanGetOrPut") == reponse_result.end())
	{
		//if (reponse_result.size() >= 1000)
		//{
		//	reponse_result.clear();
		//}
		reponse_result.insert(std::map<std::string, int>::value_type(taskNo + point + flag + "QueryIsCanGetOrPut", 0));
	}
	else
	{
		std::map<std::string, int>::iterator itor = reponse_result.find(taskNo + point + flag + "QueryIsCanGetOrPut");
		//已收到回复
		if (2 == itor->second)
		{
			//reponse_result.erase(taskNo + "QueryIsCanGetOrPut");
			return true;
		}
		//未收到回复
		if (1 == itor->second)
		{
			return false;
		}
	}
	// 组装消息
	QJsonObject json_data;

	json_data.insert("flag", QJsonValue(std::atoi(flag.c_str())));
	json_data.insert("point", QJsonValue(QString::fromStdString(point)));
	json_data.insert("taskNo", QJsonValue(QString::fromStdString(taskNo)));
	QJsonObject body_data;
	QJsonDocument doc(json_data);
	QString strJson(doc.toJson(QJsonDocument::Compact));
	//body_data.insert("taskJson", QJsonValue(strJson));
	body_data.insert("taskJson", QJsonValue(json_data));
	body_data.insert("bizType", QJsonValue("FORKLIFT_PICK_PUT"));
	body_data.insert("operatorName", QJsonValue("AGV"));
	body_data.insert("callCode", QJsonValue("ETG35IJHFIU19405"));
	if ("" != IPAddressAndPort)
	{
		// 发送请求并声明响应函数
		API_SERVER.send(IPAddressAndPort + "/Fms/fms_agv/AgvCheck", applicationjson, body_data, &this->ReplyQueryIsCanGetOrPut);
		QJsonDocument doc2(body_data);
		QString strJson2(doc2.toJson(QJsonDocument::Compact));
		log_info_color(red, strJson2.toStdString());
	}
	else
	{
		log_error_color(red, "read IPAddressAndPort happen error");
	}

	reponse_result[taskNo + point + flag + "QueryIsCanGetOrPut"] = 1;
	return false;
}

//上报AGV状态
bool ApiClient::DeviceInfoReport(std::string deviceNo, std::string errorCode, std::string  message, std::string deviceType, int workMode)
{
	// 组装消息
	QJsonObject json_data;
	json_data.insert("deviceNo", QJsonValue(QString::fromStdString(deviceNo)));
	json_data.insert("errorCode", QJsonValue(QString::fromStdString(errorCode)));
	json_data.insert("message", QJsonValue(QString::fromStdString(message)));
	json_data.insert("deviceType", QJsonValue(QString::fromStdString(deviceType)));
	json_data.insert("workMode", QJsonValue(QString::number(workMode)));
	QJsonObject body_data;
	body_data.insert("taskJson", json_data);
	body_data.insert("bizType", QJsonValue("DEVICE_INFO_REPORT"));
	body_data.insert("operatorName", QJsonValue("AGV"));
	body_data.insert("callCode", QJsonValue("ETG35IJHFIU19405"));

	// 发送请求并声明响应函数
	API_SERVER.send("127.0.0.1:625/", applicationjson, body_data, reply);

	return true;
}


bool ApiClient::reply(QJsonObject response, QJsonObject body) {

	ApiMsg::ReadBool(response, "valid");

	return true;
}

bool ApiClient::replyTimeout(QJsonObject response, QJsonObject body)
{
	std::stringstream ss;
	ss << "API timeout!";
	log_info_color(log_color::purple, ss.str().c_str());
	return false;
}

//等待上位系统的回复
bool ApiClient::ReplyQueryIsCanGetOrPut(QJsonObject response, QJsonObject body) {

	std::string Is_Debug = "";
	CONFIG_MANAGE.readConfig("Is_Debug", &Is_Debug, "config.txt");
	if ("" == Is_Debug)
	{
		log_error_color(red, "read Is_Debug happen error");
	}
	if (Is_Debug == "TRUE")
	{
		ApiMsg::ReadBool(response, "valid");
		QJsonObject taskContent;
		std::string tmp_string = ApiMsg::ReadString(body, "taskJson");
		QJsonParseError jsonError;
		QJsonDocument task_doucment = QJsonDocument::fromJson(tmp_string.c_str(), &jsonError);
		if (!task_doucment.isNull() && (jsonError.error == QJsonParseError::NoError) && task_doucment.isObject()) {
			taskContent = task_doucment.object();
		}
		else
		{
			taskContent = ApiMsg::ReadObject(body, "taskContent");
		}
		auto taskNo = ApiMsg::ReadString(taskContent, "taskNo");
		auto point = ApiMsg::ReadString(taskContent, "point");
		auto flag = ApiMsg::ReadString(taskContent, "flag");

		reponse_result[taskNo + point + flag + "QueryIsCanGetOrPut"] = 2;
		return true;
	}
	return true;
}