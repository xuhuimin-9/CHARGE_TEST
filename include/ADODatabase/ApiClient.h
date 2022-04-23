#ifndef API_CLIENT_H_
#define API_CLIENT_H_
#include <string>

#include <boost/serialization/singleton.hpp>
#include <QTcpServer>
#include <QNetworkAccessManager>
#include <QQueue>
#include <QJsonObject>
#include <QString>

#include <Storage_Manage.h>
#include "ApiServer.h"

class ApiMsg;

class ApiClient : QObject
{
	Q_OBJECT
public:

	ApiClient(QObject *parent = nullptr);
	~ApiClient();

	QString get_server_ip();

	void set_server_ip(QString ip_address);

	bool EquipmentStatusChangeNotification(std::string deviceNo = "deviceNo", std::string status = "status", std::string errorCode = "errorCode", int agv_id = -1);
	bool TaskFinishReport(std::string taskNo, std::string deviceNo, std::string containerNo, std::string point, std::string actionType);
	bool TaskFinishReport(std::string taskNo, std::string deviceNo, std::string point, std::string actionType);
	//para1： 当前任务号（叉车身上正在执行任务的任务号码）para2: 取放货点   Para3:   1：取货  2：放货
	bool QueryIsCanGetOrPut(std::string taskNo, std::string point, std::string flag);
	std::map<std::string, std::string> all_request_mapping;
	static bool DeviceInfoReport(std::string deviceNo,std::string errorCode="000", std::string  message="", std::string deviceType = "FORKLIFT", int workMode = 1);
	static bool  replyTimeout(QJsonObject, QJsonObject);

private:

	QString server_ip = "127.0.0.1";
	std::string IPAddressAndPort="";
	std::map<int, std::string> agv_error_mag = {
	{0, ""},
	{1, "全局路径规划失败"},
	{2, "局部路径规划失败"},
	{3, "检测到障碍物"},
	{4, "定位错误"},
	{5, "偏离路径"},
	{6, "停车失败"},
	{7, "障碍物停车"},
	{8, "交通管制停车"},
	{9, "防撞条停车"}
	};

	//用于储存调用接口时的结果
	static std::map<std::string, int>reponse_result;

	QString createUuid(QString str);

	QString CurrentTime();

	static bool InsertOrder(ApiMsg*);

	static bool RevokeOrder(ApiMsg*);

	static bool QueryOrderInfo(ApiMsg*);

	static bool QueryAGVInfo(ApiMsg*);

	static bool QueryAllAGVInfo(ApiMsg*);

	static bool UpdatePriority(ApiMsg*);

	static bool QueryIsCanGetOrPutReponse(ApiMsg*);

	//自测接口
	static bool TaskFinishReportTest(ApiMsg*);
	//自测接口
	static bool QueryIsCanGetOrPutTest(ApiMsg*);


	static bool UnknownOperation(ApiMsg*);

	static bool reply(QJsonObject, QJsonObject);

	static bool ReplyQueryIsCanGetOrPut(QJsonObject, QJsonObject);
};

typedef boost::serialization::singleton<ApiClient> Singleton_Api_Client;
#define API_CLIENT Singleton_Api_Client::get_mutable_instance()

#endif // #ifndef API_CLIENT_H_