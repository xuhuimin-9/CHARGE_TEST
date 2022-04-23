/*
*
* 文件： 和充电桩沟通的文件。 供应商为昀迪
* 作者： 费晓行
* 日期： 2022年3月22日
*
*============================================
*
* 端口是4001， tcp协议，
* 设置电流，0x3是数据高位，0x4是数据低位
* 控制指令这块，目前只有充电、放电两个功能, 其他都是状态的获取
*
* 注：
	接收充电后一般10s内，不要再充电，10s后可以再次进行
	充电手臂伸出后停止，就可以立马结束
	最多尝试3次机会，一般不会有问题
*/

/* windows头文件 */

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>

#include <iostream>
using namespace std;

#include "Core/charge_station_yundi.h"

#pragma comment(lib,"ws2_32.lib")

#if 1 /* 下面的地址只是充电桩的默认地址 */
#define  PORT 4001
#define  IP_ADDRESS "192.168.0.101" /* 本地ip设置成了192.168.1.200 */
#define  GATEWAY    "192.168.0.1"
#define  DNS        "192.168.0.1"
#define  NETMASK    "255.255.255.0"
#endif

charge_station* p_cs = NULL;

/*
* 构造函数
*/

charge_station::charge_station(int id, const char* ip, int port)
{
	this->id_ = id;
	this->ip_ = ip;
	this->port_ = port;

	this->p_queue_ = alloc_queue();
	this->hSem_ = CreateSemaphore(NULL, 1, 1, NULL);//创建信号量
}

/*
* 析构函数
*/

charge_station::~charge_station()
{
	free_queue(this->p_queue_);

	CloseHandle(this->hSem_);
}


/*
* 函数： 获取错误状态
* 参数： 错误码
* 信息： 字符信息
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
* 函数： 计算校验值
* 参数： p_char
* 结果： 校验值, 这部分内容需要和充电桩firmware一起修改，当前版本为0,0,0,0
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
* 函数： 申请charge状态
* 参数： 数据， 长度
* 结果： 真或假
*/

boolean charge_station::request_charge_state()
{
	unsigned char buf[22];
	unsigned char* p_char = &buf[0];

	/* 头字符, 头字符byte1 - byte4 */

	p_char[0] = 0xff;
	p_char[1] = 0xff;
	p_char[2] = 0xff;
	p_char[3] = 0xff;

	/* 数据内容 word1 ->word7, 也就是 byte5 - byte18*/

	/* word1 */
	p_char[4] = 0x00;
	p_char[5] = this->id_;

	/* word2, charge */
	p_char[6] = 0x00;
	p_char[7] = 0x00;

	/* word3, arm control, 机械臂控制*/
	p_char[8] = 0x00;
	p_char[9] = 0x00;

	/* word4, charge voltage, 充电电压*/
	p_char[10] = 0x00;
	p_char[11] = 0x00;

	/* word5, charge current, 充电电流*/
	p_char[12] = 0x00;
	p_char[13] = 0x00;

	/*word6- word7, reserved and for crc*/
	p_char[14] = 0x00;
	p_char[15] = 0x00;
	p_char[16] = 0x00;
	p_char[17] = 0x00;

	/*尾字符*/
	p_char[18] = 0xee;
	p_char[19] = 0xee;
	p_char[20] = 0xee;
	p_char[21] = 0xee;

	return send_packet(p_char, 22);
}


/*
* 函数： 获取charge状态
* 参数： 数据， 长度， 状态
* 结果： 真或假
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

	/* 头字符 byte1- byte4*/

	ASSERT_DATA(p_char[0] == 0xff);
	ASSERT_DATA(p_char[1] == 0xff);
	ASSERT_DATA(p_char[2] == 0xff);
	ASSERT_DATA(p_char[3] == 0xff);

	/* 获取电压  */

	*p_voltage = (p_char[6] << 0x8) + p_char[7];
	*p_current = (p_char[8] << 0x8) + p_char[9];

	arm_state = p_char[11];
	arm_mode = p_char[13];
	*p_state =  (arm_state << 0x8) + arm_mode;


	/* 尾字符 byte23 - byte26*/

	ASSERT_DATA(p_char[22] == 0xee);
	ASSERT_DATA(p_char[23] == 0xee);
	ASSERT_DATA(p_char[24] == 0xee);
	ASSERT_DATA(p_char[25] == 0xee);

	return TRUE;
}

/*
* 函数： 申请控制充电
* 参数： 数据， 长度
* 结果： 真或假
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

	/* 头字符, 头字符byte1 - byte4 */

	p_char[0] = 0xff;
	p_char[1] = 0xff;
	p_char[2] = 0xff;
	p_char[3] = 0xff;

	/* 数据内容 word1 ->word7, 也就是 byte5 - byte18*/

	/* word1 */
	p_char[4] = 0x00;
	p_char[5] = this->id_;

	/* word2, charge */
	p_char[6] = 0x00;
	p_char[7] = control;

	/* word3, arm control */
	//如果电流大于0则伸出
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

	/*尾字符*/
	p_char[18] = 0xee;
	p_char[19] = 0xee;
	p_char[20] = 0xee;
	p_char[21] = 0xee;

	return TRUE;
}


/*
* 函数: 充电
* 参数：电流大小
* 结果：真或假
*/

boolean charge_station::run_charge(unsigned int current)
{
	unsigned char packet[22];

	_request_charge_control(&packet[0], 22, 0x1, current);

	return send_packet(&packet[0], 22);
}


/*
* 函数：停止充电
* 参数：无
* 结果：真或假
*/

boolean charge_station::stop_charge()
{
	unsigned char packet[22];

	_request_charge_control(&packet[0], 22, 0x2, 0x0);

	return send_packet(&packet[0], 22);
}

/*
* 函数： 申请控制充电桩
* 参数： 数据， 长度
* 结果： 真或假
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

	/* 头字符, 头字符byte1 - byte4 */

	p_char[0] = 0xff;
	p_char[1] = 0xff;
	p_char[2] = 0xff;
	p_char[3] = 0xff;

	/* 数据内容 word1 ->word7, 也就是 byte5 - byte18*/

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

	/*尾字符*/
	p_char[18] = 0xee;
	p_char[19] = 0xee;
	p_char[20] = 0xee;
	p_char[21] = 0xee;

	return TRUE;
}


/*
* 函数: 伸出手臂
* 参数：无
* 结果：真或假
*/

boolean charge_station::strech_out_arm()
{
	unsigned char packet[22];

	_request_arm_control(&packet[0], 22, 0x1);

	return send_packet(&packet[0], 22);
}


/*
* 函数：缩回手臂
* 参数：无
* 结果：真或假
*/
boolean charge_station::take_back_arm()
{
	unsigned char packet[22];

	_request_arm_control(&packet[0], 22, 0x2);

	return send_packet(&packet[0], 22);
}




/*
* 函数： 获取control状态
* 参数： 数据， 长度， 状态
* 结果： 真或假
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

	/* 头字符 byte1- byte4*/

	ASSERT_DATA(p_char[0] == 0xff);
	ASSERT_DATA(p_char[1] == 0xff);
	ASSERT_DATA(p_char[2] == 0xff);
	ASSERT_DATA(p_char[3] == 0xff);

	/* 获取状态  */

	*p_error = (p_char[14] << 0x8) + p_char[15];  /* byte1是高位，byte2是地位，bit1-bit8是从右到左*/

	/* 尾字符 byte23 - byte26*/

	ASSERT_DATA(p_char[22] == 0xee);
	ASSERT_DATA(p_char[23] == 0xee);
	ASSERT_DATA(p_char[24] == 0xee);
	ASSERT_DATA(p_char[25] == 0xee);

	return TRUE;
}

/*
* 函数： 发送报文
* 参数： 数据， 长度，
* 结果： 成功或者失败
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
			printf("error happened in send_packet：%d\n", WSAGetLastError());
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
* 函数： 接收报文
* 参数： 数据， 长度，
* 结果： 成功或者失败
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
			printf("error happened in recv_packet：%d\n", WSAGetLastError());
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
* 函数：连接socket
* 参数：无
* 结果：真或假
*/


boolean charge_station::connect_socket()
{
	struct sockaddr_in ClientAddr;
	int Ret = 0;
	unsigned long ul_FALSE = 0;
	unsigned long ul = 1;

	ClientAddr.sin_family = AF_INET;//服务器 流套接字
	ClientAddr.sin_addr.s_addr = inet_addr(ip_);//服务器ip地址
	ClientAddr.sin_port = htons(port_);//服务器端口
	memset(ClientAddr.sin_zero, 0x00, 8);

	/* set blocking */

	ioctlsocket(client_socket, FIONBIO, &ul_FALSE); //变为阻塞

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
* 函数：创建socket
* 参数：无
* 结果：真或假
*/

boolean charge_station::init_socket()
{
	/* Create Socket */

	client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//协议域为AF_INET协议（用ipv4地址（32位的）与端口号（16位的）的组合），流套接字类型，指定TCP协议
	if (client_socket == INVALID_SOCKET)//无效套接字
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
* 函数：关闭socket
* 参数：空
* 结果：空
*/

void charge_station::close_socket()
{
	closesocket(client_socket);
}

/*
* 保持session连接
*/

boolean charge_station::connect_session()
{
	if (socket_state == TRUE)
	{
		return TRUE;
	}

	/* 关闭socket */
	close_socket();

	/* 重启socket */
	return init_socket();
}


/*
* 获取队列
*/

struct QUEUE* charge_station::get_queue()
{
	return p_queue_;
}


/*
* 线程入口
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
			case TERMINATE_CHARGE_MSG_MACRO://终止充电

				stop = 1;
				break;

			case GET_CHARGE_STATE_MSG_MACRO://获取充电状态

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

				result = p_charge->run_charge(100);//指定充电电流为50A
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
* 初始化充电桩
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
* 清理充电桩
*/

void clean_sys_charge()
{
	if (p_cs)
	{
		write_msg(p_cs->get_queue(), NULL, 0, TERMINATE_CHARGE_MSG_MACRO);
		delete p_cs;
	}
}