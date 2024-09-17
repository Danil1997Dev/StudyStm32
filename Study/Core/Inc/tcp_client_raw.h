/*
 * tcp_client_raw.h
 *
 *  Created on: Jul 27, 2024
 *      Author: Danil
 */

#ifndef INC_TCP_CLIENT_RAW_H_
#define INC_TCP_CLIENT_RAW_H_


#endif /* INC_TCP_CLIENT_RAW_H_ */

#define APP_RX_DATA_SIZE  2048
#define APP_TX_DATA_SIZE  2048

#include "tcp.h"
#include <string.h>
#include <stdlib.h>
#include "global_def.h"
#include "dhcp.h"

enum tcp_client_state
{
	ES_C_NONE = 0,
	ES_C_CONNECT = 1,
	ES_C_TRANSMIT = 2,
	ES_C_RECIVE = 3,
	ES_C_CLOSE = 4

};
extern struct netif gnetif;
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
extern uint8_t RX_buff[30];
extern int byteNum;
extern char trg;
extern uint8_t wr;
extern UART_HandleTypeDef huart3;

uint8_t REMOTE_IP_ADDRESS[4];
uint8_t REMOTE_PORT[1];

uint8_t tcpRX[15];

uint8_t wr = 0;
uint8_t *pwr = &wr;

int i = 15;
int *pi = &i;

struct tcp_client_struct
{
	enum tcp_client_state state;
	struct pbuf *p;
	struct tcp_pcb *pcb;
};

struct entry_struct
{
	char NumStr;
	char NumColum;
	uint8_t *pstore;
};


struct entry_struct *entry;

//struct tcp_client_struct *esTX;
struct tcp_client_struct *esBuff;

void tcp_client_init(void);
static void tcp_client_send(struct tcp_pcb *newpcb, struct tcp_client_struct *es);
static void tcp_client_connectin_close(struct tcp_pcb *newpcb, struct tcp_client_struct *es);
static void tcp_client_handler(struct tcp_pcb *newpcb, struct tcp_client_struct *es);
static void input_config_data();

static void tcp_client_send(struct tcp_pcb *newpcb, struct tcp_client_struct *es)
{
	struct pbuf *ptrBuf;
	err_t wr_err = ERR_OK;


	while ((wr_err == ERR_OK) && (es->p != NULL) && (es->p->len <= newpcb->snd_buf))
	{
		ptrBuf = es->p;

		wr_err = tcp_write(newpcb, ptrBuf->payload, ptrBuf->len, TCP_WRITE_FLAG_COPY);

		if (wr_err == ERR_OK)
		{
			u16_t len;
			int cnt;

			len = ptrBuf->len;

			es->p = ptrBuf->next;

			if (es->p != NULL)
			{
				pbuf_ref(es->p);

			}

			do
			{
				cnt = pbuf_free(ptrBuf);

			}
			while(cnt == 0);

			tcp_recved(newpcb, len);

		}

	}

}

static void tcp_client_handler(struct tcp_pcb *newpcb, struct tcp_client_struct *es)
{
	//struct tcp_client_struct *esTX;

	char bufRX[es->p->len];

	memset((void *)bufRX,'\0', es->p->len);

	strncpy (bufRX, es->p->payload, es->p->len);

	strncpy (tcpRX, es->p->payload, es->p->len);

	HAL_UART_Transmit_DMA(&huart3, bufRX, es->p->len);

	//*pwr = 1;

	//tcp_client_send(newpcb, es);

	//pbuf_free(es->p);

	esBuff = es;




}

static void tcp_client_connectin_close(struct tcp_pcb *newpcb, struct tcp_client_struct *es)
{

	tcp_arg(newpcb, NULL);
	tcp_recv(newpcb, NULL);
	tcp_sent(newpcb, NULL);
	tcp_poll(newpcb, NULL, 0);
	tcp_err(newpcb, NULL);

	mem_free(es);

	tcp_close(newpcb);

}

static void input_config_data(struct entry_struct *newEntry, char *msg, int msgSize, int del, const int byteN)
{

	char NumStr = newEntry->NumStr;//REMOTE_IP_ADDRESS{Num = 0,Num = 1,Num = 2,Num = 3}, one Num of REMOTE_IP_ADDRESS is three strCnt of UserRxBufferFS
	char NumColum = newEntry->NumColum;
	uint8_t *pStore = newEntry->pstore;

	int Num = 0;
	int strCntRX = 0;//UserRxBufferFS{strCnt = 0,strCnt = 1,strCnt = 2,strCnt = 3,strCnt = ...}
	char ipBuffchr[NumColum];
	int ipBuffint = 0;
	int strCntBuff = 0;

	memset(pStore, '\0', NumStr);
	memset(ipBuffchr, '\0', NumColum);

	HAL_Delay(del*1000);

#ifdef COM_PORT
	CDC_Transmit_FS((uint8_t *)msg, (uint16_t)msgSize);
#else
	HAL_UART_Transmit_DMA(&huart3, (uint8_t *)msg, (uint16_t)msgSize);
#endif

	while (byteNum < byteN + 1)
	{
		;
	}

	if (wr)
	{
		while (Num < NumStr)
		{
#ifdef COM_PORT
			while ((char)UserRxBufferFS[strCntRX] != '.' & (char)UserRxBufferFS[strCntRX] != '\0' )
#else
				while ((char)RX_buff[strCntRX] != '.' & (char)RX_buff[strCntRX] != '\0' )
#endif
			{
#ifdef COM_PORT
				ipBuffchr[strCntBuff] = (char)UserRxBufferFS[strCntRX];
#else
				ipBuffchr[strCntBuff] = (char)RX_buff[strCntRX];
#endif

				strCntRX++;
				strCntBuff++;
			}

			strCntBuff = 0;
			strCntRX++;


			for (char n = 0;n < NumColum;n++)
			{
				ipBuffint |= (ipBuffchr[n] << 8*n);


			}

			*pStore = atoi((char *)&ipBuffint);

			pStore++;
			Num++;

			memset(ipBuffchr, '\0', sizeof(ipBuffchr));
			ipBuffint = 0;
		}

		wr = 0;
#ifdef COM_PORT
		UserRxBufferFS[strCntRX-1] = '\n';
		UserRxBufferFS[strCntRX] = '\r';
		CDC_Transmit_FS((uint8_t *)UserRxBufferFS, (uint16_t) (strCntRX+2));
		memset(UserRxBufferFS, '\0', sizeof(UserRxBufferFS));
#else
		RX_buff[strCntRX-1] = '\n';
		RX_buff[strCntRX] = '\r';
		HAL_UART_Transmit_DMA(&huart3, (uint8_t *)RX_buff, (uint16_t) (strCntRX+2));
		HAL_Delay(10);
		memset(RX_buff, '\0', sizeof(RX_buff));
#endif

		memset(pStore, '\0', NumStr);
		memset(ipBuffchr, '\0', NumColum);

	}
}

