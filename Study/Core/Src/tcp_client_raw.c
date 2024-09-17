/*
 * tcp_client_raw.c
 *
 *  Created on: Jul 27, 2024
 *      Author: Danil
 */


#include "tcp_client_raw.h"


struct tcp_pcb *cppcb;


ip4_addr_t remote_ipaddr;
ip4_addr_t remote_netmask;
ip4_addr_t remote_gw;

extern struct dhcp* sdhcp;

//uint8_t REMOTE_IP_ADDRESS[4];
//uint8_t REMOTE_NETMASK_ADDRESS[4];
//uint8_t REMOTE_GATEWAY_ADDRESS[4];



static err_t tcp_client_connect(void *arg, struct tcp_pcb *newpcb, err_t err);
//static err_t tcp_client_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_client_sent(void *arg, struct tcp_pcb *newpcb, u16_t len);
static err_t tcp_client_recive(void *arg, struct tcp_pcb *newpcb, struct pbuf *p, err_t err);
static err_t tcp_client_poll(void *arg, struct tcp_pcb *newpcb);
static void tcp_client_err(void *arg, err_t err);




void tcp_client_init(void)
{
	err_t ret_err;

	dhcp_set_struct(&gnetif, sdhcp);
	dhcp_start(&gnetif);


	entry = (struct entry_struct *) mem_malloc(sizeof(struct entry_struct *));



	entry->NumColum = 3;
	entry->NumStr = (char)sizeof(REMOTE_IP_ADDRESS);
	entry->pstore = (uint8_t *)&REMOTE_IP_ADDRESS;


	input_config_data(entry, (char *)"Enter IP address: ", strlen("Enter IP address: "), 7, byteNum);



	IP4_ADDR(&remote_ipaddr, REMOTE_IP_ADDRESS[0], REMOTE_IP_ADDRESS[1], REMOTE_IP_ADDRESS[2], REMOTE_IP_ADDRESS[3]);

	entry->NumColum = 2;
	entry->NumStr = (char)sizeof(REMOTE_PORT);
	entry->pstore = (uint8_t *)&REMOTE_PORT;

	input_config_data(entry, (char *)"Enter PORT: ", strlen("Enter PORT: "), 1, byteNum);


//	memset(REMOTE_IP_ADDRESS, '\0', 4);
//	IP4_ADDR(&remote_ipaddr, REMOTE_IP_ADDRESS[0], REMOTE_IP_ADDRESS[1], REMOTE_IP_ADDRESS[2], REMOTE_IP_ADDRESS[3]);
//
//	REMOTE_PORT[0] = 0;

	cppcb = tcp_new();

	if (cppcb != NULL)
	{
		ret_err = tcp_connect(cppcb, &remote_ipaddr, REMOTE_PORT[0], tcp_client_connect);

		//ret_err = ERR_OK;

	}
	else
	{
		memp_free(MEMP_TCP_PCB, cppcb);

	}
}

static err_t tcp_client_connect(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	err_t ret_err;

	struct tcp_client_struct *es;

	es = (struct tcp_client_struct *)memp_malloc(sizeof(struct tcp_client_struct *));

//	es = (struct tcp_client_struct *)arg;

	if (es != NULL)
	{

		es->p = NULL;
		es->pcb = newpcb;
		es->state = ES_C_CONNECT;

		//*pwr = 1;

		tcp_arg(newpcb, es);

		tcp_recv(newpcb, tcp_client_recive);

		tcp_err(newpcb, tcp_client_err);

		tcp_poll(newpcb, tcp_client_poll, 5);

		esBuff = es;

		ret_err = ERR_OK;

	}
	else
	{

		tcp_client_connectin_close(newpcb, es);

		ret_err = ERR_MEM;
	}

	return ret_err;

}


static err_t tcp_client_sent(void *arg, struct tcp_pcb *newpcb, u16_t len)
{

	err_t ret_err;
	struct tcp_client_struct *es;

	es = (struct tcp_client_struct *)arg;

	if (es->p != NULL)
	{

		tcp_sent(newpcb, tcp_client_send);

		tcp_client_send(newpcb, es);

		ret_err = ERR_OK;

	}
	else
	{
		if (es->state == ES_C_CLOSE)
		{
			tcp_client_connectin_close(newpcb, es);

			ret_err = ERR_OK;

		}

	}

	return ret_err;

}

static err_t tcp_client_recive(void *arg, struct tcp_pcb *newpcb, struct pbuf *p, err_t err)
{
	err_t ret_err;
	struct tcp_client_struct *es;

	es = (struct tcp_client_struct *)arg;

	if (es == NULL)
	{
		es->state = ES_C_CLOSE;
		if (es->p == NULL)
		{
			tcp_client_connectin_close(newpcb, es);

		}

		ret_err = ERR_OK;
	}
	else if (err != ERR_OK)
	{
		ret_err = err;

		pbuf_free(es->p);

	}
	else
	{
		switch (es->state)
		{
			case ES_C_CONNECT:

				es->p = p;

				tcp_recved(newpcb, p->tot_len);

				tcp_client_handler(newpcb, es);

				pbuf_free(p);

				ret_err = ERR_OK;

				break;

			case ES_C_CLOSE:

				tcp_recved(newpcb, p->tot_len);

				pbuf_free(es->p);

				pbuf_free(p);

				break;

			default:
				break;
		}

	}



	return ret_err;

}



static err_t tcp_client_poll(void *arg, struct tcp_pcb *newpcb)
{
	struct tcp_client_struct *es;

	struct tcp_client_struct *esTX;

	es = (struct tcp_client_struct *) arg;


	if (es != NULL)
	{
		if (wr == 1)
		{
			esTX = es;

			//esTX->p = es->p;

			esTX->p = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct tcp_client_struct), PBUF_POOL);

			esTX->p->payload = (void *)"Hello from Client\n\r";

			esTX->p->len = sizeof("Hello from Client\n\r");

			esTX->p->tot_len = esTX->p->tot_len - esTX->p->len + sizeof("Hello from Client\n\r");
			//tcp_sent(newpcb, tcp_client_sent);

			tcp_client_send(newpcb, esTX);

			pbuf_free(esTX->p);

			esTX->p = NULL;

		}
		else if (es->p != NULL)
		{
			//char buffTX[25];

			//memcpy(buffTX, es->p->payload, 25);

//			tcp_sent(newpcb, tcp_client_sent);
//
//			tcp_client_send(newpcb, es);

			pbuf_free(es->p);

			//es->p = NULL;

		}

		else
		{
			if (es->state == ES_C_CLOSE)
			{
				tcp_client_connectin_close(newpcb, es);

				mem_free(es);

			}
		}

		esBuff->state = es->state;

	}

	return ERR_OK;

}
static void tcp_client_err(void *arg, err_t err)
{
	struct tcp_client_struct *es;

	es = (struct tcp_client_struct *) arg;

	mem_free(es);

}
