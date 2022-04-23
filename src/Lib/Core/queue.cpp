/*
*
* 文件: queue.c
* 作者：费晓行
* 时间：2021年12月27日
*
*/

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Core/queue.h"

/*
* 创建队列
*/

struct QUEUE* alloc_queue()
{
	struct QUEUE* p_queue = (struct QUEUE*) malloc(sizeof(struct QUEUE));
	if (NULL == p_queue)
	{
		return NULL;
	}

	memset(p_queue, 0, sizeof(struct QUEUE));
	p_queue->h_mutex = CreateMutex(NULL,  /* default security atributes */
		FALSE,    /* initially not owned */
		NULL);    /* unamed mutex */

	return p_queue;
}

/*
* 写入消息
*/

boolean write_msg(struct QUEUE* p_queue, void* p_void, int len, int type)
{
	int pos;

	if (NULL == p_queue)
	{
		return FALSE;
	}

	WaitForSingleObject(p_queue->h_mutex, INFINITE);
	if (p_queue->num == QUEUE_MSG_SIZE)
	{
		ReleaseMutex(p_queue->h_mutex);
		return FALSE;
	}

	pos = p_queue->write;
	p_queue->msg[pos].p_void = p_void;
	p_queue->msg[pos].len = len;
	p_queue->msg[pos].type = type;

	p_queue->write += 1;
	p_queue->write = p_queue->write % QUEUE_MSG_SIZE;

	p_queue->num += 1;

	ReleaseMutex(p_queue->h_mutex);
	return TRUE;
}

/*
* 读取信息
*/

boolean read_msg(struct QUEUE* p_queue, void** pp_void, int* p_len, int* p_type)
{
	int pos;

	if (NULL == p_queue)
	{
		return FALSE;
	}

	if (NULL == pp_void || NULL == p_len || NULL == p_type)
	{
		return FALSE;
	}

	WaitForSingleObject(p_queue->h_mutex, INFINITE);
	if (p_queue->num == 0)
	{
		ReleaseMutex(p_queue->h_mutex);
		return FALSE;
	}

	pos = p_queue->read;
	*pp_void = p_queue->msg[pos].p_void;
	*p_len = p_queue->msg[pos].len;
	*p_type = p_queue->msg[pos].type;

	p_queue->read += 1;
	p_queue->read = p_queue->read % QUEUE_MSG_SIZE;

	p_queue->num -= 1;

	ReleaseMutex(p_queue->h_mutex);
	return TRUE;
}

/*
* 释放队列
*/

void free_queue(struct QUEUE* p_queue)
{
	int i;

	if (NULL == p_queue)
	{
		return;
	}

	WaitForSingleObject(p_queue->h_mutex, INFINITE);
	if (p_queue->num == 0)
	{
		goto final;
	}

	for (i = 0; i < p_queue->num; i++)
	{
		free((void*)(p_queue->msg[p_queue->read].p_void));

		p_queue->read += 1;
		p_queue->read = p_queue->read % QUEUE_MSG_SIZE;
	}

	final:

	ReleaseMutex(p_queue->h_mutex);
	CloseHandle(p_queue->h_mutex);
	free(p_queue);
}



