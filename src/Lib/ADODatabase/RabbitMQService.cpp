#include "RabbitMQService.h"
#include "RabbitMqClient.h"
#include "Manage/Storage_Manage.h"
#include "Manage/Config_Manage.h"
#include "comm/simplelog.h"

#include <deque>
#include <string.h>

#include "QJsonParseError"
#include "QJsonDocument"
#include <QJsonObject>

RabbitMQService::~RabbitMQService() {

	//mqClient_->DisConnect();
	delete mqClient_;
	mqClient_ = nullptr;

	delete exchange_;
	exchange_ = nullptr;
	
	for each (auto var in all_queue_)
	{
		delete var.second;
		var.second = nullptr;
	}
	StopScan();
}

void RabbitMQService::setRabbitMQServiceNotifier(RabbitMQ_Service_Notifier * notifier) {
	rabbitmq_service_notifier = notifier;
}

bool RabbitMQService::Init() {
	
	if (!ReadRabbitMQService())
	{
		log_error("Cant read Rabbit Service Config");
	}
	
	// 定义操作函数
	launcher["agv.request.wc"] = &RabbitMQService::InsertOrder;
	launcher["agv.request.check"] = &RabbitMQService::checkUnit;
	launcher["agv.request.receiving"] = &RabbitMQService::checkGetGoods;

	// 开启线程监听
	rabbitmq_service_listen_.reset(new boost::thread(boost::bind(&RabbitMQService::timerEvent, this)));
	return true;
}

bool RabbitMQService::ReadRabbitMQService() {
	storage_type_ = {
	{""  , ""},
	{" " , " " },
	{"A1", "F9-Z1-1" },
	{"A2", "F9-Z1-2" },
	{"A3", "F9-Z2" },
	{"A4", "F9-Z3-1" },
	{"A5", "F9-Z3-2" },
	{"A6", "F9-Z4" },
	{"A7", "F9-B1-1" },
	{"A8", "F9-B1-2" },
	{"A9", "F9-B2" },
	{"A10", "F9-B3-1" },
	{"A11", "F9-B3-2" },
	{"A12", "F9-B4" }
	};
	const std::vector<std::string> all_storage = {
		"A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "A10", "A11", "A12",
		"B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "B10", "B11", "B12","B13", "B14", "B15", "B16",
		"C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "C10", "C11", "C12","C13", "C14", "C15", "C16", "C17", "C18", "C19", "C20", "C21", "C22", "C23", "C24", "C25", "C26", "C27", "C28",
		"D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "D10", "D11", "D12","D13", "D14", "D15", "D16", "D17", "D18", "D19", "D20",
	};

	for each (auto var in all_storage)
	{
		CONFIG_MANAGE.readConfig(var, &storage_type_[var]);
	}
	CONFIG_MANAGE.readConfig("Rabbit_Mq_Debug", &rabbit_debug);
	CONFIG_MANAGE.readConfig("Rabbit_Mq_IP", &host);
	return CONFIG_MANAGE.readConfig("Rabbit_Mq_Host", &virtualHost);
}

bool RabbitMQService::StopScan() {
	if (rabbitmq_service_listen_)
	{
		rabbitmq_service_listen_->interrupt();
		rabbitmq_service_listen_->join();
	}
	return true;
}

void RabbitMQService::timerEvent() {
	try
	{
		const std::string queue_name = "agv.request.*";
		const std::string bindingskey = "zc." + queue_name;
		std::string message;
		std::string routing_key;
		uint64_t delivery_tag;

		CRabbitMqClient client(host, 5672, username, password);
		std::string err;
		if ( 0 != client.Connect(virtualHost, err))
		{
#if 0
			log_error("RabbitMQService::timerEvent: reconnect fail , please check your network" + err);
#endif
		}
		CExchange exchange(exchange_name_, "topic");
		CQueue queue(bindingskey);

		client.DeclareQueue(queue);
		client.BindQueueToExchange(queue, exchange, queue_name);
		client.StateConsumer(bindingskey);

		while (!rabbitmq_service_listen_->interruption_requested())
		{
			//中断点,interrupted退出点
			boost::this_thread::interruption_point();
			message = "";
			routing_key = "";
			delivery_tag = 0;
			if (client.ConsumerMessage(routing_key, message, delivery_tag, bindingskey))
			{
				//delete client;
				client = CRabbitMqClient(host, 5672, username, password);
				if ( 0 != client.Connect(virtualHost, err))
				{
					boost::this_thread::sleep(boost::chrono::milliseconds(2000));
#if 0
					log_error("RabbitMQService::timerEvent: reconnect fail , please check your network" + err + host + username + password);
#endif
				}
				client.DeclareQueue(queue);
				client.BindQueueToExchange(queue, exchange, queue_name);
				client.StateConsumer(bindingskey);
				continue;
			}

			// 转JSON
			if (!message.empty())
			{
				QJsonParseError jsonError;
				QJsonDocument doucment = QJsonDocument::fromJson(message.c_str(), &jsonError);
				if (!doucment.isNull() && (jsonError.error == QJsonParseError::NoError))
				{ // JSON读取没有出错
					if (doucment.isObject())
					{
						// 接收传入数据
						QJsonObject object = doucment.object();
						if (launcher.find(routing_key) != launcher.end())
						{
							(this->*(launcher[routing_key]))(object);
						}
						else
						{
							// 未知的操作:unknown routing_key
							log_error("unknown operation : " + routing_key);
						}
					}
				}
				else
				{
					// 解析出错
					log_error("RabbitMQService: fail read json : Message:" + message + " ,\nrouting_key" + routing_key);
				}
			}
			if (delivery_tag != 0)
			{
				client.AckMessage(delivery_tag);
			}
		}
	}
	catch (...)
	{
		log_info("RabbitMQService::timerEvent interrupted!");
	}
}

CRabbitMqClient* RabbitMQService::InitClient(std::string queue_name) {
	std::string err;
	auto client = new CRabbitMqClient(host);
	if (client && 0 != client->Connect(virtualHost, err))
	{
		log_error(err.c_str());
		return nullptr;
	}
	return client;
}

// 消费者声明交换机-Exchange
CExchange* RabbitMQService::InitExchange(CRabbitMqClient* client) {
	if (!client)
	{
		client = InitClient();
	}
	std::string err;
	auto exchange = new CExchange("RabbitMQExchange", "topic");
	if (client && 0 != client->DeclareExchange(*exchange, err))
	{
		log_error(err.c_str());
		return nullptr;
	}
	return exchange;
}

/// <summary>
/// Reads the string from JsonObject
/// </summary>
/// <param name="object">The object.</param>
/// <param name="key">The key.</param>
/// <returns></returns>
/// Creator:MeiXu
/// CreateDate:2021/2/19
std::string RabbitMQService::ReadString(QJsonObject object, QString key){
	std::string value = "";
	if (object.contains(key))
	{  // 包含指定的 key
		QJsonValue json_value;
		json_value = object.value(key);
		if (json_value.isString())
		{  // 判断 value 是否为字符串
			value = object.value(key).toString().toLocal8Bit().toStdString();
		}
	}
	return value;
}

/// <summary>
/// Reads the int.
/// </summary>
/// <param name="object">The object.</param>
/// <param name="key">The key.</param>
/// <returns></returns>
/// Creator:MeiXu
/// CreateDate:2021/2/19
double RabbitMQService::ReadNumber(QJsonObject object, QString key) {
	double value = -1;
	if (object.contains(key))
	{  // 包含指定的 key
		QJsonValue json_value;
		json_value = object.value(key);
		if (json_value.isDouble())
		{  // 判断 value 是否为数字
			value = json_value.toDouble();
		}
	}
	return value;
}

/// <summary>
/// Reads the bool.
/// </summary>
/// <param name="object">The object.</param>
/// <param name="key">The key.</param>
/// <returns></returns>
/// Creator:MeiXu
/// CreateDate:2021/2/20
/// 
bool RabbitMQService::ReadBool(QJsonObject object, QString key) {
	bool value = false;
	if (object.contains(key))
	{  // 包含指定的 key
		QJsonValue json_value;
		json_value = object.value(key);
		if (json_value.isBool())
		{  // 判断 value 是否为数字
			value = json_value.toBool();
		}
	}
	return value;
}

// 生产者
bool RabbitMQService::SendMsg(std::string msg, std::string route_key, bool t1, bool t2) {
	bool result = false;
	
	if (rabbit_debug == "True")
	{
		return true;
	}

	if (mqClient_ == nullptr) {
		mqClient_ = new CRabbitMqClient(host);
		if (0 != mqClient_->Connect(virtualHost))
		{
			log_error("RabbitMQService::Init: connect fail , please check your network");
		}
	}

	// 组装 properties
	amqp_basic_properties_t properties;
	memset(&properties, 0, sizeof(properties));
	properties._flags = AMQP_BASIC_DELIVERY_MODE_FLAG
		| AMQP_BASIC_CONTENT_TYPE_FLAG		//content_type属性有效
		| AMQP_BASIC_HEADERS_FLAG			//headers 属性有效
		| AMQP_BASIC_TYPE_FLAG;
	properties.delivery_mode = AMQP_DELIVERY_NONPERSISTENT;	//消息持久(即使mq重启,还是会存在,前提是队列也是持久化的)
	properties.content_type = amqp_cstring_bytes("application/json");
	properties.type = amqp_cstring_bytes("java.lang.String");

	CMessage message(msg, properties, route_key, t1, t2);
	std::string err;

	//log_info_color(log_color::black, (route_key + " : send : " + msg).c_str());

	if(0 != mqClient_->PublishMessage(message, route_key, err))
	{
		log_error(err.c_str());
		if (0 != mqClient_->Connect(virtualHost, err))
		{
			log_error(err.c_str());
		}
		result = false;
	}
	else
	{
		result = true;
	}

	return result;
}

// 无需调用,通知库位状态更改:
bool RabbitMQService::ReportStorageStatus(std::string CageName, int CageType, int CageState) {
	if (rabbit_debug == "True")
	{
		return true;
	}
	bool result = false;
	const std::string queue_name = "agv.response.cachestate";
	if (storage_type_.find(CageName) != storage_type_.end())
	{
		QJsonObject json_data;
		json_data.insert("CageName", storage_type_.at(CageName).c_str());
		json_data.insert("CageType", CageType);
		json_data.insert("CageState", CageState);
		result = SendMsg(QString(QJsonDocument(json_data).toJson()).toStdString().c_str(), queue_name, true, false);
	}
	else
	{
		log_error("RabbitMQService::ReportStorageStatus: cant find point storage in storage_type_:" + CageName);
	}

	return result;
}

/*
调用形式
//RABBITMQ_SERVICE.ReportStorageStatus("CageName", 1, 1);
//RABBITMQ_SERVICE.ReportOrderStatus("TaskId", 1, 1, "PalletNo", "Location");
//RABBITMQ_SERVICE.ReportAgvAction("CageName", 1,1,"PalletNo", 1);
//RABBITMQ_SERVICE.ApplyUnit("UnitId", "TaskId");
//RABBITMQ_SERVICE.ApplyGetGoods("UnitId", "TaskId", 1);
*/
// （同步）通知工单执行状态,返回是否发送成功
// Location 没有时,必须传入""或" "
bool RabbitMQService::ReportOrderStatus(std::string TaskId, int TaskStatus, int CarId, std::string PalletNo, std::string Location) {
	//if (rabbit_debug == "True")
	//{
	//	return true;
	//}
	bool result = false;
	static const std::string queue_name = "agv.response.wc";
	
	QJsonObject json_data;
	json_data.insert("TaskId", TaskId.c_str());
	json_data.insert("TaskStatus", TaskStatus);
	json_data.insert("CarId", CarId%10);
	json_data.insert("PalletNo", PalletNo.c_str());
	json_data.insert("Location", storage_type_.at(Location).c_str());
	result = SendMsg(QString(QJsonDocument(json_data).toJson()).toStdString().c_str(), queue_name, true ,false);

	return result;
}

// （同步）通知AGV进出储位,返回是否发送成功
bool RabbitMQService::ReportAgvAction(std::string CageName, int CageType, int WorkType, std::string PalletNo, int CarId) {
	if (rabbit_debug == "True")
	{
		return true;
	}
	bool result = false;
	const std::string queue_name = "agv.response.cachepallet";

	QJsonObject json_data;
	json_data.insert("CageName", storage_type_.at(CageName).c_str());
	json_data.insert("CageType", CageType);
	json_data.insert("WorkType", WorkType);
	json_data.insert("PalletNo", PalletNo.c_str());
	json_data.insert("CarId", CarId%10);
	result = SendMsg(QString(QJsonDocument(json_data).toJson()).toStdString().c_str(), queue_name, true, false);

	return result;
}

// （异步）申请水箱单元,0是发送中,1是返回滚筒线没有货物,2是滚筒线有货物
int RabbitMQService::ApplyUnit(std::string UnitId, std::string TaskId) {
	if (rabbit_debug == "True")
	{
		return 1;
	}
	int result = 0;
	static const std::string queue_name = "agv.response.check";
	std::string key = UnitId + TaskId;
	boost::unique_lock<boost::shared_mutex> lk(apply_unit_mutex_);
	if (all_apply_unit_status_.find(key) == all_apply_unit_status_.end())
	{
		all_apply_unit_status_[key] = 0;
		all_apply_unit_status_count_[key] = 0;
	}
	else {

		QJsonObject json_data;
		// 0代表初始化,1代表已发送
		switch (all_apply_unit_status_[key])
		{
		case -20:
		case 0:
			//初始化,发送
			json_data.insert("UnitId", UnitId.c_str());
			json_data.insert("TaskId", TaskId.c_str());
			SendMsg(QString(QJsonDocument(json_data).toJson()).toStdString().c_str(), queue_name, true, false);
			result = 0;
			all_apply_unit_status_[key] = -1;
			log_info("RabbitMQService::ApplyUnit: apply " + UnitId);
			break;
		case 1:
			// 已发送,返回无货,返回正确,出栈
			all_apply_unit_status_[key] = 0;
			all_apply_unit_status_count_[key] ++;
			log_info("RabbitMQService::ApplyUnit: no tray in point station :" + UnitId);
			if (all_apply_unit_status_count_[key] >= 5)
			{
				all_apply_unit_status_.erase(key);
				all_apply_unit_status_count_.erase(key);
				result = 1;
			}
			else
			{
				result = 0;
			}
			break;
		case 2:
			// 已发送,返回有货
			all_apply_unit_status_.erase(key);
			all_apply_unit_status_count_.erase(key);
			log_error("RabbitMQService::ApplyUnit: has tray in " + UnitId);
			result = 2;
			break;
		default:
			// 已发送,等待
			static int a = 0;
			a++;
			if (a == 50)
			{
				a = 0;
			}
			if (a == 5)
			{
				log_info("tray waiting response : " + UnitId);
			}
			all_apply_unit_status_[key] --;
			result = 0;
			break;
			break;
		}
	}
	return result;
}


// （异步）满轮开始取货确认（true:确定能取货,AGV进入滚筒线取满轮）

bool RabbitMQService::ApplyGetGoods(std::string UnitId, std::string TaskId) {
	if (rabbit_debug == "True")
	{
		return true;
	}
	bool result = false;
	static const std::string queue_name = "agv.response.receiving";
	std::string key = UnitId + TaskId;

	// 开始取料通知
	boost::unique_lock<boost::shared_mutex> lk(apply_get_goods_mutex_);
	if (all_apply_get_goods_status_.find(key) == all_apply_get_goods_status_.end())
	{
		// 不存在,则创建
		all_apply_get_goods_status_[key] = -1;
		result = false;
	}
	else {
		// 0代表初始化,1代表已发送
		QJsonObject json_data;
		switch (all_apply_get_goods_status_[key])
		{
		case -1:
		case 0:
			json_data.insert("UnitId", UnitId.c_str());
			json_data.insert("TaskId", TaskId.c_str());
			json_data.insert("Status", 1);
			if (SendMsg(QString(QJsonDocument(json_data).toJson()).toStdString().c_str(), queue_name, true, false))
			{
				all_apply_get_goods_status_[key] = 0;
			}
			result = false;
			break;
			//正在发送
			result = false;
			break;
		case 1:
			// 已发送,可以取,返回正确
			result = true;
			break;
		case 2:
			// 已发送,不可取,返回错误
		default:
			all_apply_get_goods_status_[key] = -1;
			result = false;
			
			break;
		}
	}

	return result;
}

// （异步）获取是否可以继续取货
bool RabbitMQService::FeedbackUnitStatus(std::string UnitId, std::string TaskId) {
	if (rabbit_debug == "True")
	{
		return true;
	}
	std::string key = UnitId + TaskId;
	static const std::string queue_name = "agv.response.receiving";
	boost::shared_lock<boost::shared_mutex> lk(apply_get_goods_mutex_);
	if (all_apply_get_goods_status_[key] == 1)
	{
		return true;
	}
	else
	{
		QJsonObject json_data;
		json_data.insert("UnitId", UnitId.c_str());
		json_data.insert("TaskId", TaskId.c_str());
		json_data.insert("Status", 1);
		if (SendMsg(QString(QJsonDocument(json_data).toJson()).toStdString().c_str(), queue_name, true, false))
		{
			all_apply_get_goods_status_[key] = 0;
		}
		return false;
	}
}

// （同步）通知取货完成
bool RabbitMQService::ReportStopGetGoods(std::string UnitId, std::string TaskId) {
	if (rabbit_debug == "True")
	{
		return true;
	}
	bool result = true;
	const std::string queue_name = "agv.response.receiving";

	QJsonObject json_data;
	json_data.insert("UnitId", UnitId.c_str());
	json_data.insert("TaskId", TaskId.c_str());
	json_data.insert("Status", 2);
	result = SendMsg(QString(QJsonDocument(json_data).toJson()).toStdString().c_str(), queue_name, true, false);
	std::string key = UnitId + TaskId;
	// 加锁 map 中删除指定 key 
	boost::unique_lock<boost::shared_mutex> lk(apply_get_goods_mutex_);
	all_apply_get_goods_status_.erase(key);
	return result;
}

bool RabbitMQService::InsertOrder(QJsonObject object) {
	bool valid = false;
	std::string taskId = ReadString(object, "taskId");
	int taskStatus = (int)ReadNumber(object, "taskStatus");
	std::string unitId = ReadString(object, "unitId");
	if (unitId.find("F9WC-") != std::string::npos)
	{
		int taskType = (int)ReadNumber(object, "taskType");
		std::string palletNo = ReadString(object, "palletNo");
		rabbitmq_service_notifier->RabbitMQInsertOrder(taskId, taskType, taskStatus, unitId, palletNo);
		valid = true;
		//log_info("RabbitMQService::InsertOrder:taskId=" + taskId + ",taskType=" + std::to_string(taskType) + ",taskStatus=" + std::to_string(taskStatus) + ",unitId=" + unitId + ",palletNo=" + palletNo);
	}
	return valid;
}

bool RabbitMQService::checkUnit(QJsonObject object) {
	bool valid = false;
	std::string UnitId = ReadString(object, "UnitId");
	std::string TaskId = ReadString(object, "TaskId");
	int Status = (int)ReadNumber(object, "Status");
	std::string key = UnitId + TaskId;
	if (UnitId.find("F9WC-") != std::string::npos)
	{
		boost::unique_lock<boost::shared_mutex> lk(apply_unit_mutex_);
		if (all_apply_unit_status_.find(key) != all_apply_unit_status_.end()) {
			if (all_apply_unit_status_[key] != Status)
			{
				//log_info("RabbitMQService::checkUnit:UnitId=" + UnitId + ",TaskId=" + TaskId + ",Status=" + std::to_string(Status));
			}
			all_apply_unit_status_[key] = Status;
			valid = true;
		}
	}
	return valid;
}

bool RabbitMQService::checkGetGoods(QJsonObject object) {
	// 接收传入数据
	bool valid = false;
	std::string UnitId = ReadString(object, "UnitId");
	std::string TaskId = ReadString(object, "TaskId");
	int Status = (int)ReadNumber(object, "Status");
	std::string key = UnitId + TaskId;
	if (UnitId.find("F9WC-") != std::string::npos)
	{
		boost::unique_lock<boost::shared_mutex> lk(apply_get_goods_mutex_);
		if (all_apply_get_goods_status_.find(key) != all_apply_get_goods_status_.end())
		{
			if (all_apply_get_goods_status_[key] != Status)
			{
				//log_info("RabbitMQService::checkGetGoods:UnitId=" + UnitId + ",TaskId=" + TaskId + ",Status=" + std::to_string(Status));
			}
			all_apply_get_goods_status_[key] = Status;
			valid = true;
		}
	}
	return valid;
}