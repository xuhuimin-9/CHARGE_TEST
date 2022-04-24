#ifndef _CHARGE_STATION_H
#define _CHARGE_STATION_H

#include <windows.h>
#include <iostream>
#include <string>
using namespace std;
#include "Core/queue.h"


/* assert �궨�� */

#define ASSERT_DATA(a) \
	do{ \
		if(!(a)) \
		{ \
			return FALSE; \
		}\
	}while(0)

/* ״̬�궨�� */

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

	boolean run_charge(unsigned int current);          /* ��ʼ��� */
	boolean stop_charge();                             /* ֹͣ��� */
	boolean strech_out_arm();                          /* ���arm */
	boolean take_back_arm();                           /* �ջ�arm */
	boolean set_max_current(int current);              /* ������������ */
	boolean set_charge_current(int current);           /* ���ó����� */
	boolean get_control_state(unsigned short* p_error); /* ��ȡ����״̬ */
	boolean request_charge_state();    /* �����ȡcharge״̬ */
	boolean get_charge_state(unsigned short* p_state, unsigned int* p_voltage, unsigned char* p_current);
	/* ����״̬ */
	QUEUE* get_queue();                        /* ��ȡ��Ϣ����ָ�� */
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
* ��������
*/

void init_sys_charge();
void clean_sys_charge();


#endif



