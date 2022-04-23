#ifndef CONFIG_MANAGE_H_
#define CONFIG_MANAGE_H_
#include <string>
#include <map>

#include <boost/serialization/singleton.hpp>
#include "Common/Config.h"
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

class Config_Manage
{
public:
	Config_Manage::Config_Manage() {};
	Config_Manage::~Config_Manage() {};

	// reread 用于注册需要重读的配置文件
	bool readConfig(std::string key, std::string *value, std::string filename = "config.txt", bool reread = false);
	
	bool readConfig(std::string key, int *value, std::string filename = "config.txt", bool reread = false);

	bool readConfig(std::string key, double *value, std::string filename = "config.txt", bool reread = false);

	bool readConfig(std::string key, float *value, std::string filename = "config.txt", bool reread = false);

	bool reReadConfig();
private:
	std::map<std::string, Config_File> all_config_files;
	std::map<std::string, bool> reread_config_files;
	mutable boost::shared_mutex entry_mutex;
};

typedef boost::serialization::singleton<Config_Manage> Singleton_Config_Manage;
#define CONFIG_MANAGE Singleton_Config_Manage::get_mutable_instance()

#endif // #ifndef API_CLIENT_H_