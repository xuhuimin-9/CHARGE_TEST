#ifndef ModelStationParking_H_
#define ModelStationParking_H_
#include <string>
class ModelStationParking
{
public:
	ModelStationParking();
	ModelStationParking(std::string name, std::string charging, std::string status , std::string ip);
	~ModelStationParking();

	std::string ModelStationParking::getName();
	std::string ModelStationParking::getCharging();
	std::string ModelStationParking::getStatus();
	std::string ModelStationParking::getIp();
	void ModelStationParking::setStatus(std::string status);
private:

	int station_id_;

	std::string station_name;

	std::string station_status;

	std::string is_charging_;

	std::string ip_address_;
};

#endif