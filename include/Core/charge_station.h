#ifndef _CHARGE_STATION_H
#define _CHARGE_STATION_H

#include <windows.h>
#include <iostream>
#include <string>
using namespace std;
#include "Core/queue.h"


/* assert 宏定义 */

#define ASSERT_DATA(a) \
	do{ \
		if(!(a)) \
		{ \
			return FALSE; \
		}\
	}while(0)

/* 状态宏定义 */

#define IDLE_STATE                                      0x0
#define CHARGE_STATE                                    0x1
#define CHARGE_FINISH_STATE                             0x2
#define MODULE_ERROR_STATE                              0x3
#define CONTACT_FINISH_STATE                            0x4
#define FORWARD_NOT_OK_STATE                            0x5
#define ARM_ERROR_STATE                                 0x6
#define CHARGE_LOOP_BREAK_STATE                         0x7
#define CHARGE_LOOP_NOT_OPEN_OR_ABNORMAL_VOLTAGE_SATTE  0x9

class charge_station
{
public:

	charge_station(int id, std::string ip, int port);
	~charge_station();
	boolean init_socket();
	boolean connect_session();
	static int run_entry(charge_station* p_charge);

	boolean run_charge(unsigned int current);          /* 开始充电 */
	boolean stop_charge();                             /* 停止充电 */
	boolean strech_out_arm();                          /* 伸出arm */
	boolean take_back_arm();                           /* 收回arm */
	boolean set_max_current(int current);              /* 设置最大充电电流 */
	boolean set_charge_current(int current);           /* 设置充电电流 */
	boolean get_control_state(unsigned short* p_error); /* 获取控制状态 */
	boolean request_charge_state();    /* 申请获取charge状态 */
	boolean get_charge_state(unsigned short* p_state, unsigned int* p_voltage, unsigned char* p_current);
	/* 运行状态 */
	QUEUE* get_queue();                        /* 获取消息队列指针 */
	int get_id() { return id_; };
	int get_max_current();
	int get_charge_current();

private:

	int id_;
	string ip_;
	int port_;
	int max_current_;
	int charge_current_;

	boolean socket_state;
	SOCKET client_socket;

	QUEUE* p_queue_;
	HANDLE hSem_;

private:

	charge_station(charge_station&);

	const char* get_error_info(unsigned char state);
	unsigned char calculate_crc(unsigned char* p_char);
	boolean _request_charge_control(unsigned char* p_char, int size, unsigned char control, unsigned int current);
	boolean _request_arm_control(unsigned char* p_char, int size, unsigned char control);

	boolean send_packet(void* p_packet, int size);
	boolean recv_packet(void* p_packet, int size);
	boolean connect_socket();
	void close_socket();

};

/*
* 其他函数
*/

void init_sys_charge();
void clean_sys_charge();


#endif



