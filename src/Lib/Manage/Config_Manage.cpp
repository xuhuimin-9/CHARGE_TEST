#include "Config_Manage.h"
#include "comm/simplelog.h"

bool Config_Manage::readConfig(std::string key, std::string *value, std::string filename /*= "config.txt"*/, bool reread /*= false*/) {

	boost::shared_lock<boost::shared_mutex> lk(entry_mutex);

	bool result = false;
	
	if (reread)
	{
		reread_config_files[filename] = true;
	}
	
	if (all_config_files.find(filename) == all_config_files.end() )
	{
		// 没有读过该文件
		Config_File config;
		if (config.FileExist(filename))
		{
			config.ReadFile(filename);
			all_config_files[filename] = config;
		}
		else
		{
			// 不存在该文件
		}
	}

	if (all_config_files[filename].KeyExists(key))
	{
		result = true;
		*value = all_config_files[filename].Read<std::string>(key);
	}
	else
	{
		// 不存在该配置
	}
	return result;
}

bool Config_Manage::reReadConfig() {

	boost::shared_lock<boost::shared_mutex> lk(entry_mutex);
	for each (auto var in reread_config_files)
	{
		Config_File config;
		if (config.FileExist(var.first))
		{
			config.ReadFile(var.first);
			all_config_files[var.first] = config;
		}
		else
		{
			log_error("no config file names : " + var.first);
		}
	}
	return true;
}

bool Config_Manage::readConfig(std::string key, int *value, std::string filename /*= "config.txt"*/, bool reread /*= false*/) {
	std::string val_str;
	return readConfig(key, &val_str, filename, reread), (*value = atoi(val_str.c_str())) || true;
}

bool Config_Manage::readConfig(std::string key, double *value, std::string filename /*= "config.txt"*/, bool reread /*= false*/) {
	std::string val_str;
	return readConfig(key, &val_str, filename, reread), (*value = atof(val_str.c_str())) || true;
}

bool Config_Manage::readConfig(std::string key, float *value, std::string filename /*= "config.txt"*/, bool reread /*= false*/) {
	std::string val_str;
	return readConfig(key, &val_str, filename, reread), (*value = (float)atof(val_str.c_str())) || true;
}