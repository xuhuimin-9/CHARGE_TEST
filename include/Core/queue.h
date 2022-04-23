#ifndef _QUEUE_H
#define _QUEUE_H

#define QUEUE_MSG_SIZE 32


/*
* 消息宏
*/

#define READ_BATTERY_MSG_MACRO          0x01
#define WRITE_BATTERY_MSG_MACRO         0x02

#define START_CHARGE_MSG_MACRO          0x03
#define STOP_CHARGE_MSG_MACRO           0x04
#define GET_CHARGE_STATE_MSG_MACRO      0x05
#define TERMINATE_CHARGE_MSG_MACRO      0x06
#define ARM_OUT_MSG_MACRO               0x07
#define ARM_IN_MSG_MACRO                0x08

#define KEY_L_MESSAGE_MACRO             0x09   /* charge station operation */
#define KEY_G_MESSAGE_MACRO             0x0a   /* read or write battery data */
#define KEY_S_MESSAGE_MACRO             0x0b   /* send new order */
#define KEY_C_MESSAGE_MACRO             0x0c   /* cancel order */
#define KEY_D_MESSAGE_MACRO             0x0d   /* display order */
#define KEY_H_MESSAGE_MACRO             0x0e   /* halt communication */
#define KEY_R_MESSAGE_MACRO             0x0f   /* resume communication */
#define KEY_E_MESSAGE_MACRO             0x10   /* redirect order */
#define KEY_Q_MESSAGE_MACRO             0x11   /* quit */
#define KEY_M_MESSAGE_MACRO             0x12   /* menu display */
#define KEY_P_MESSAGE_MACRO             0x13   /* goto park to charge */
#define KEY_T_MESSAGE_MACRO             0x14   /* charge command */

#define STOP_CHARGE_TASK_MACRO          0x15   /* stop running charge task */

#define GET_ID_MSG_MACRO				0x16   /* xineng get charging staton id */
#define GET_ERROR_MSG_MACRO				0x17   /* xineng get charging staton error code */
#define GET_TEMPERATURE_MSG_MACRO		0x18   /* xineng get charging staton temperature */
//#define START_CHARGE_MSG_MACRO			0x19   /* xineng charging staton start charge */
//#define STOP_CHARGE_MSG_MACRO			    0x20   /* xineng charging staton stop charge */
#define SET_VOLTAGE_MSG_MACRO			0x19   /* xineng charging staton set voltage */
#define SET_CURRENT_MSG_MACRO			0x20   /* xineng charging staton set current */

/*
* 消息类型
*/

struct CONN_MSG
{
	void* p_void;
	int len;
	int type;
};

/*
* 具体消息定义
*/

struct READ_BATTERY_MSG
{
	int agv_id;
	int offset;
	int id;
};

struct WRITE_BATTERY_MSG
{
	int agv_id;
	int offset;
	int data;
	int id;
};

struct KEY_L_MSG
{
	int agv_id;
	int charge_id;
	int cmd_id;
};

struct KEY_G_MSG
{
	int agv_id;
	int read_or_write;
	int offset;
	int write_data;
};

struct KEY_S_MSG
{
	int agv_id;
	int fetch_station;
	int put_station;
};

/*
* 消息队列
*/

struct QUEUE
{
	struct CONN_MSG  msg[QUEUE_MSG_SIZE];
	int write;
	int read;
	int num;
	HANDLE h_mutex;
};

struct QUEUE* alloc_queue();
boolean write_msg(struct QUEUE* p_queue, void* p_void, int len, int type);
boolean read_msg(struct QUEUE* p_queue, void** pp_void, int* p_len, int* p_type);
void free_queue(struct QUEUE* p_queue);

#endif