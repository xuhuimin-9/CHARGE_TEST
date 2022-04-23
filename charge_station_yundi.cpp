/*
*
* �ļ��� �ͳ��׮��ͨ���ļ��� ��Ӧ��Ϊ����
* ���ߣ� ������
* ���ڣ� 2022��3��22��
*
*============================================
*
* �˿���4001�� tcpЭ�飬
* ���õ�����0x3�����ݸ�λ��0x4�����ݵ�λ
* ����ָ����飬Ŀǰֻ�г�硢�ŵ���������, ��������״̬�Ļ�ȡ
*
* ע��
	���ճ���һ��10s�ڣ���Ҫ�ٳ�磬10s������ٴν���
	����ֱ������ֹͣ���Ϳ����������
	��ೢ��3�λ��ᣬһ�㲻��������
*/

/* windowsͷ�ļ� */

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>

#include <iostream>
using namespace std;

#include "Core/charge_station_yundi.h"

#pragma comment(lib,"ws2_32.lib")

#if 1 /* ����ĵ�ַֻ�ǳ��׮��Ĭ�ϵ�ַ */
#define  PORT 4001
#define  IP_ADDRESS "192.168.0.101" /* ����ip���ó���192.168.1.200 */
#define  GATEWAY    "192.168.0.1"
#define  DNS        "192.168.0.1"
#define  NETMASK    "255.255.255.0"
#endif

charge_station* p_cs = NULL;

/*
* ���캯��
*/

charge_station::charge_station(int id, const char* ip, int port)
{
	this->id_ = id;
	this->ip_ = ip;
	this->port_ = port;

	this->p_queue_ = alloc_queue();
	this->hSem_ = CreateSemaphore(NULL, 1, 1, NULL);//�����ź���
}

/*
* ��������
*/

charge_station::~charge_station()
{
	free_queue(this->p_queue_);

	CloseHandle(this->hSem_);
}


/*
* ������ ��ȡ����״̬
* ������ ������
* ��Ϣ�� �ַ���Ϣ
*/

const char* charge_station::get_error_info(unsigned char state)
{
	const char* info;

	switch (state)
	{
	case IDLE_STATE:
		info = "idle_state";
		break;

	case CHARGE_STATE:
		info = "charge_state";
		break;

	case CHARGE_FINISH_STATE:
		info = "charge_finish_state";
		break;

	case MODULE_ERROR_STATE:
		info = "module_error_state";
		break;

	case CONTACT_FINISH_STATE:
		info = "contact_finish_state";
		break;

	case FORWARD_NOT_OK_STATE:
		info = "forward_not_ok_state";
		break;

	case ARM_ERROR_STATE:
		info = "arm_error_state";
		break;

	case CHARGE_LOOP_BREAK_STATE:
		info = "charge_loop_break_state";
		break;

	case CHARGE_LOOP_NOT_OPEN_OR_ABNORMAL_VOLTAGE_SATTE:
		info = "charge_loop_not_open_or_abnormal_voltage";
		break;

	default:
		info = "unknown_error";
		break;
	}

	return info;
}


/*
* ������ ����У��ֵ
* ������ p_char
* ����� У��ֵ, �ⲿ��������Ҫ�ͳ��׮firmwareһ���޸ģ���ǰ�汾Ϊ0,0,0,0
*/

unsigned char charge_station::calculate_crc(unsigned char* p_char)
{
	unsigned int data = 0;
	unsigned char crc[4];

	for (int i = 0; i < 14; i++)
	{
		data += p_char[i];
	}

	crc[0] = (unsigned char)(data >> 24);
	crc[1] = (unsigned char)(data >> 16);
	crc[2] = (unsigned char)(data >> 8);
	crc[3] = (unsigned char)(data );

	return 0;
}

/*
* ������ ����charge״̬
* ������ ���ݣ� ����
* ����� ����
*/

boolean charge_station::request_charge_state()
{
	unsigned char buf[22];
	unsigned char* p_char = &buf[0];

	/* ͷ�ַ�, ͷ�ַ�byte1 - byte4 */

	p_char[0] = 0xff;
	p_char[1] = 0xff;
	p_char[2] = 0xff;
	p_char[3] = 0xff;

	/* �������� word1 ->word7, Ҳ���� byte5 - byte18*/

	/* word1 */
	p_char[4] = 0x00;
	p_char[5] = this->id_;

	/* word2, charge */
	p_char[6] = 0x00;
	p_char[7] = 0x00;

	/* word3, arm control, ��е�ۿ���*/
	p_char[8] = 0x00;
	p_char[9] = 0x00;

	/* word4, charge voltage, ����ѹ*/
	p_char[10] = 0x00;
	p_char[11] = 0x00;

	/* word5, charge current, ������*/
	p_char[12] = 0x00;
	p_char[13] = 0x00;

	/*word6- word7, reserved and for crc*/
	p_char[14] = 0x00;
	p_char[15] = 0x00;
	p_char[16] = 0x00;
	p_char[17] = 0x00;

	/*β�ַ�*/
	p_char[18] = 0xee;
	p_char[19] = 0xee;
	p_char[20] = 0xee;
	p_char[21] = 0xee;

	return send_packet(p_char, 22);
}


/*
* ������ ��ȡcharge״̬
* ������ ���ݣ� ���ȣ� ״̬
* ����� ����
*/

boolean charge_station::get_charge_state(unsigned short* p_state, unsigned int* p_voltage, unsigned char* p_current)
{
	unsigned char buf[26];
	unsigned char* p_char = &buf[0];
	int size = 25;

	unsigned char arm_state = 0;
	unsigned char arm_mode = 0;

	if (NULL == p_state || NULL == p_voltage || NULL == p_current)
	{
		return FALSE;
	}

	if (TRUE != recv_packet(p_char, 26))
	{
		return FALSE;
	}

	/* ͷ�ַ� byte1- byte4*/

	ASSERT_DATA(p_char[0] == 0xff);
	ASSERT_DATA(p_char[1] == 0xff);
	ASSERT_DATA(p_char[2] == 0xff);
	ASSERT_DATA(p_char[3] == 0xff);

	/* ��ȡ��ѹ  */

	*p_voltage = (p_char[6] << 0x8) + p_char[7];
	*p_current = (p_char[8] << 0x8) + p_char[9];

	arm_state = p_char[11];
	arm_mode = p_char[13];
	*p_state =  (arm_state << 0x8) + arm_mode;


	/* β�ַ� byte23 - byte26*/

	ASSERT_DATA(p_char[22] == 0xee);
	ASSERT_DATA(p_char[23] == 0xee);
	ASSERT_DATA(p_char[24] == 0xee);
	ASSERT_DATA(p_char[25] == 0xee);

	return TRUE;
}

/*
* ������ ������Ƴ��
* ������ ���ݣ� ����
* ����� ����
*/

boolean charge_station::_request_charge_control(unsigned char* p_char, int size, unsigned char control, unsigned int current)
{
	if(NULL == p_char)
	{
		return FALSE;
	}

	if(22 != size)
	{
		return FALSE;
	}

	if(0x01 != control && 0x02 != control)
	{
		return FALSE;
	}

	if(current > 120)
	{
		return FALSE;
	}

	/* ͷ�ַ�, ͷ�ַ�byte1 - byte4 */

	p_char[0] = 0xff;
	p_char[1] = 0xff;
	p_char[2] = 0xff;
	p_char[3] = 0xff;

	/* �������� word1 ->word7, Ҳ���� byte5 - byte18*/

	/* word1 */
	p_char[4] = 0x00;
	p_char[5] = this->id_;

	/* word2, charge */
	p_char[6] = 0x00;
	p_char[7] = control;

	/* word3, arm control */
	//�����������0�����
	/*if (current > 0)
	{
		p_char[8] = 0x00;
		p_char[9] = 0x01;
	}
	else
	{
		p_char[8] = 0x00;
		p_char[9] = 0x02;
	}*/

	p_char[8] = 0x00;
	p_char[9] = 0x00;

	/* word4, charge voltage*/

	if(current)
	{
		p_char[10] = (30 * 10) >> 0x8;
		p_char[11] = (30 * 10) & 0xff;
	}
	else
	{
		p_char[10] = 0x00;
		p_char[11] = 0x00;
	}

	/* word5, charge current */
	if(current)
	{
		p_char[12] = (current * 10) >> 0x8;
		p_char[13] = (current * 10) & 0xff;
	}
	else
	{
		p_char[12] = 0x00;
		p_char[13] = 0x00;
	}

	/*word6- word7, reserved and for crc*/
	p_char[14] = 0x00;
	p_char[15] = 0x00;
	p_char[16] = 0x00;
	p_char[17] = 0x00;

	/*β�ַ�*/
	p_char[18] = 0xee;
	p_char[19] = 0xee;
	p_char[20] = 0xee;
	p_char[21] = 0xee;

	return TRUE;
}


/*
* ����: ���
* ������������С
* ���������
*/

boolean charge_station::run_charge(unsigned int current)
{
	unsigned char packet[22];

	_request_charge_control(&packet[0], 22, 0x1, current);

	return send_packet(&packet[0], 22);
}


/*
* ������ֹͣ���
* ��������
* ���������
*/

boolean charge_station::stop_charge()
{
	unsigned char packet[22];

	_request_charge_control(&packet[0], 22, 0x2, 0x0);

	return send_packet(&packet[0], 22);
}

/*
* ������ ������Ƴ��׮
* ������ ���ݣ� ����
* ����� ����
*/

boolean charge_station::_request_arm_control(unsigned char* p_char, int size, unsigned char control)
{
	if(NULL == p_char)
	{
		return FALSE;
	}

	if(22 != size)
	{
		return FALSE;
	}

	if(0x01 != control && 0x02 != control)
	{
		return FALSE;
	}

	/* ͷ�ַ�, ͷ�ַ�byte1 - byte4 */

	p_char[0] = 0xff;
	p_char[1] = 0xff;
	p_char[2] = 0xff;
	p_char[3] = 0xff;

	/* �������� word1 ->word7, Ҳ���� byte5 - byte18*/

	/* word1 */
	p_char[4] = 0x00;
	p_char[5] = this->id_;

	/* word2, charge */
	p_char[6] = 0x00;
	p_char[7] = 0x00;

	/* word3, arm control */
	p_char[8] = 0x00;
	p_char[9] = control;

	/* word4, charge voltage*/
	p_char[10] = 0x00;
	p_char[11] = 0x00;

	/* word5, charge current */
	p_char[12] = 0x00;
	p_char[13] = 0x00;

	/*word6- word7, reserved and for crc*/
	p_char[14] = 0x00;
	p_char[15] = 0x00;
	p_char[16] = 0x00;
	p_char[17] = 0x00;

	/*β�ַ�*/
	p_char[18] = 0xee;
	p_char[19] = 0xee;
	p_char[20] = 0xee;
	p_char[21] = 0xee;

	return TRUE;
}


/*
* ����: ����ֱ�
* ��������
* ���������
*/

boolean charge_station::strech_out_arm()
{
	unsigned char packet[22];

	_request_arm_control(&packet[0], 22, 0x1);

	return send_packet(&packet[0], 22);
}


/*
* �����������ֱ�
* ��������
* ���������
*/
boolean charge_station::take_back_arm()
{
	unsigned char packet[22];

	_request_arm_control(&packet[0], 22, 0x2);

	return send_packet(&packet[0], 22);
}




/*
* ������ ��ȡcontrol״̬
* ������ ���ݣ� ���ȣ� ״̬
* ����� ����
*/

boolean charge_station::get_control_state(unsigned short* p_error)
{
	unsigned char buf[26];
	unsigned char* p_char = &buf[0];
	int size = 26;

	if (NULL == p_error)
	{
		return FALSE;
	}

	if (TRUE != recv_packet(p_char, 26))
	{
		return FALSE;
	}

	/* ͷ�ַ� byte1- byte4*/

	ASSERT_DATA(p_char[0] == 0xff);
	ASSERT_DATA(p_char[1] == 0xff);
	ASSERT_DATA(p_char[2] == 0xff);
	ASSERT_DATA(p_char[3] == 0xff);

	/* ��ȡ״̬  */

	*p_error = (p_char[14] << 0x8) + p_char[15];  /* byte1�Ǹ�λ��byte2�ǵ�λ��bit1-bit8�Ǵ��ҵ���*/

	/* β�ַ� byte23 - byte26*/

	ASSERT_DATA(p_char[22] == 0xee);
	ASSERT_DATA(p_char[23] == 0xee);
	ASSERT_DATA(p_char[24] == 0xee);
	ASSERT_DATA(p_char[25] == 0xee);

	return TRUE;
}

/*
* ������ ���ͱ���
* ������ ���ݣ� ���ȣ�
* ����� �ɹ�����ʧ��
*/

boolean charge_station::send_packet(void* p_packet, int size)
{
	int left = size;
	int num = 0;
	const char* p_buf = (const char*)p_packet;

	while (1)
	{
		num = send(client_socket, p_buf, left, 0);

		if (num == SOCKET_ERROR)
		{
			printf("error happened in send_packet��%d\n", WSAGetLastError());
			socket_state = FALSE;
			return FALSE;
		}

		else if (num == left)
		{
			break;
		}

		p_buf += num;
		left -= num;
	}

	return TRUE;
}

/*
* ������ ���ձ���
* ������ ���ݣ� ���ȣ�
* ����� �ɹ�����ʧ��
*/

boolean charge_station::recv_packet(void* p_packet, int size)
{
	boolean res = FALSE;
	int total = 0;;

	while (1)
	{
		int num = recv(client_socket, (char*)p_packet, size, 0);

		if (num == 0)
		{
			printf("link offline\n");
			socket_state = FALSE;
			break;
		}

		else if (num == SOCKET_ERROR)
		{
			printf("error happened in recv_packet��%d\n", WSAGetLastError());
			socket_state = FALSE;
			break;
		}

		total += num;
		if (26 == total)
		{
			res = TRUE;
			break;
		}
	}

	return res;
}

/*
* ����������socket
* ��������
* ���������
*/


boolean charge_station::connect_socket()
{
	struct sockaddr_in ClientAddr;
	int Ret = 0;
	unsigned long ul_FALSE = 0;
	unsigned long ul = 1;

	ClientAddr.sin_family = AF_INET;//������ ���׽���
	ClientAddr.sin_addr.s_addr = inet_addr(ip_);//������ip��ַ
	ClientAddr.sin_port = htons(port_);//�������˿�
	memset(ClientAddr.sin_zero, 0x00, 8);

	/* set blocking */

	ioctlsocket(client_socket, FIONBIO, &ul_FALSE); //��Ϊ����

	/* Connect socket */

	Ret = connect(client_socket, (struct sockaddr*)&ClientAddr, sizeof(ClientAddr));
	if (Ret == SOCKET_ERROR)
	{
		printf("Connect Error::%d\n", GetLastError());
		return FALSE;
	}

	/* Set socket unblock mode*/

	Ret = ioctlsocket(client_socket, FIONBIO, (unsigned long *)&ul);// set unblock mode
	if (Ret == SOCKET_ERROR)
	{
		return FALSE;
	}

	return TRUE;
}

/*
* ����������socket
* ��������
* ���������
*/

boolean charge_station::init_socket()
{
	/* Create Socket */

	client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//Э����ΪAF_INETЭ�飨��ipv4��ַ��32λ�ģ���˿ںţ�16λ�ģ�����ϣ������׽������ͣ�ָ��TCPЭ��
	if (client_socket == INVALID_SOCKET)//��Ч�׽���
	{
		printf("Create Socket Failed::%d\n", GetLastError());
		return FALSE;
	}

	if (connect_socket())
	{
		socket_state = TRUE;
		return TRUE;
	}

	return FALSE;
}

/*
* �������ر�socket
* ��������
* �������
*/

void charge_station::close_socket()
{
	closesocket(client_socket);
}

/*
* ����session����
*/

boolean charge_station::connect_session()
{
	if (socket_state == TRUE)
	{
		return TRUE;
	}

	/* �ر�socket */
	close_socket();

	/* ����socket */
	return init_socket();
}


/*
* ��ȡ����
*/

struct QUEUE* charge_station::get_queue()
{
	return p_queue_;
}


/*
* �߳����
*/

int charge_station::run_entry(charge_station* p_charge)
{
	unsigned short error;
	int stop = 0;
	bool result = false;

	void* p_void;
	int len;
	int type;

	if (p_charge->init_socket() == FALSE)
	{
		return -1;
	}

	while (!stop)
	{
		while (FALSE == p_charge->connect_session())
		{
			Sleep(100);
		}

		if (TRUE == read_msg(p_charge->get_queue(), &p_void, &len, &type))
		{
			switch (type)
			{
			case TERMINATE_CHARGE_MSG_MACRO://��ֹ���

				stop = 1;
				break;

			case GET_CHARGE_STATE_MSG_MACRO://��ȡ���״̬

				unsigned short state;
				unsigned int voltage;
				unsigned char current;

				state = voltage = current = 0;

				result = p_charge->request_charge_state();
				printf("result = %s\n", result == true ? "ok" : "error");
				Sleep(500);
				result = p_charge->get_charge_state(&state, &voltage, &current);
				printf("result = %s\n", result == true ? "ok" : "error");

				printf("state = %d\n", state);
				printf("voltage = %d\n", voltage);
				printf("current = %d\n", current);

				break;

			case START_CHARGE_MSG_MACRO:

				result = p_charge->run_charge(100);//ָ��������Ϊ50A
				printf("result = %s\n", result == true ? "ok" : "error");
				Sleep(500);
				result = p_charge->get_control_state(&error);
				printf("result = %s\n", result == true ? "ok" : "error");

				printf("control state info is %d\n", error);
				break;

			case STOP_CHARGE_MSG_MACRO:

				result = p_charge->stop_charge();
				printf("result = %s\n", result == true ? "ok" : "error");
				Sleep(500);
				result = p_charge->get_control_state(&error);
				printf("result = %s\n", result == true ? "ok" : "error");

				printf("control state info is %d\n", error);
				break;

			case ARM_OUT_MSG_MACRO:

				result = p_charge->strech_out_arm();
				printf("stretch out arm\n");
				Sleep(500);
				result = p_charge->get_control_state(&error);
				printf("result = %s\n", result == true ? "ok" : "error");

				printf("control state info is %d\n", error);
				break;

			case ARM_IN_MSG_MACRO:

				result = p_charge->take_back_arm();
				printf("take back arm\n");
				Sleep(500);
				result = p_charge->get_control_state(&error);
				printf("result = %s\n", result == true ? "ok" : "error");

				printf("control state info is %d\n", error);
				break;

			default:
				break;
			}
		}
		else
		{
			Sleep(100);
		}
	}

	/* close socket */
	p_charge->close_socket();

	return 0;
}

/*
* ��ʼ�����׮
*/

void init_sys_charge()
{
	DWORD ThreadID;

	p_cs = new charge_station(1, "192.168.0.101", 4001);

	CreateThread(NULL,
		0,
		(LPTHREAD_START_ROUTINE)&charge_station::run_entry,
		p_cs,
		0,
		&ThreadID);

}


/*
* ������׮
*/

void clean_sys_charge()
{
	if (p_cs)
	{
		write_msg(p_cs->get_queue(), NULL, 0, TERMINATE_CHARGE_MSG_MACRO);
		delete p_cs;
	}
}