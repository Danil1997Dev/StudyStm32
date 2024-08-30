/*
 * tcp_server_raw.c
 *
 *  Created on: Jul 10, 2024
 *      Author: Danil
 */
#include "tcp_server_raw.h"


static err_t tcp_server_accpet(void *arg,struct tcp_pcb *newpcb,err_t err);
static err_t tcp_server_sent(void *arg, struct tcp_pcb *newpcb, u16_t len);
static err_t tcp_server_recive(void *arg,struct tcp_pcb *newpcb,struct pbuf *p,err_t err);
static err_t tcp_server_poll(void *arg, struct tcp_pcb *newpcb);
//static err_t tcp_server_connect(void *arg, struct tcp_pcb *tpcb, err_t err);
static void tcp_server_err(void *arg, err_t err);


struct tcp_pcb *sppcb;
extern ip4_addr_t ipaddr;
extern ip4_addr_t netmask;
extern ip4_addr_t gw;

uint8_t *p_ptr;
uint8_t p_val;

void tcp_server_init(void)
{
//	struct tcp_pcb *sppcb;

	sppcb = tcp_new();

	if (sppcb != NULL)
	{
		err_t err;
		err = tcp_bind(sppcb, &ipaddr, 7);

		if (err == ERR_OK)
		{
			sppcb = tcp_listen(sppcb);

			tcp_accept(sppcb, tcp_server_accpet);

		}
		else
		{
			memp_free(MEMP_TCP_PCB,sppcb);
		}
	}
}

static err_t tcp_server_accpet(void *arg,struct tcp_pcb *newpcb,err_t err)
{
	UNUSED(arg);
	UNUSED(err);

	err_t ret_err;
	struct tcp_server_struct *es;

	tcp_setprio(newpcb,TCP_PRIO_MIN);

	es = mem_malloc(sizeof(struct tcp_server_struct*));



	if (es != NULL)
	{
		es->state = ES_ACCEPT;
		es->pcb = newpcb;
		es->p = NULL;

		tcp_arg(newpcb,es);

		tcp_recv(newpcb, tcp_server_recive);

		tcp_err(newpcb, tcp_server_err);

		tcp_poll(newpcb, tcp_server_poll, 0);

		//CDC_Transmit_FS((uint8_t *)"Ready\n\r", (uint16_t) sizeof("Ready\n\r"));

		ret_err = ERR_OK;

	}
	else
	{
		tcp_server_connect_close(newpcb, es);

		ret_err = ERR_MEM;
	}






}


static err_t tcp_server_recive(void *arg,struct tcp_pcb *newpcb,struct pbuf *p,err_t err)
{
	struct tcp_server_struct *es;
	err_t ret_err;

	es = (struct tcp_server_struct *) arg;

	if (p == NULL)
	{
		es->state = ES_CLOSE;

		if (es->p == NULL)
		{
			tcp_server_connect_close(newpcb, es);

		}
		else
		{
			tcp_sent(newpcb, tcp_server_sent);

			tcp_server_send(newpcb, es);

		}

		ret_err = ERR_OK;

	}
	else if (err != ERR_OK)
	{
		es->p = NULL;

		pbuf_free(p);

		ret_err = err;

	}
	else if (es->state == ES_ACCEPT)
	{
		es->state = ES_RECIVED;

		es->p = p;

		tcp_sent(newpcb, tcp_server_sent);

		tcp_server_hanler(newpcb, es);

		ret_err = ERR_OK;

	}
	else if (es->state == ES_RECIVED)
	{
		if (es->p == NULL)
		{
			es->p = p;

			tcp_server_hanler(newpcb, es);

		}
		else
		{
			struct pbuf* buff;

			buff = es->p;
			pbuf_chain(buff, p);
			p_ptr = (uint8_t*)es->p->payload;
			p_val = *p_ptr;
		}
		ret_err = ERR_OK;


	}
	else if (es->state == ES_CLOSE)
	{
		tcp_recved(newpcb, p->tot_len);

		es->p = NULL;

		pbuf_free(p);

		ret_err = ERR_OK;


	}
	else
	{
		tcp_recved(newpcb, p->tot_len);

		es->p = NULL;

		pbuf_free(p);

		ret_err = ERR_OK;


	}

	return ret_err;



}


static err_t tcp_server_sent (void *arg, struct tcp_pcb *newpcb, u16_t len)
{

	struct tcp_server_struct *es;

	es = (struct tcp_server_struct *) arg;

	if (es->p != NULL)
	{
		tcp_sent(newpcb, tcp_server_send);

		tcp_server_send(newpcb, es);
	}
	else
	{
		if (es->state == ES_CLOSE)
		{
			tcp_server_connect_close(newpcb, es);

		}

	}

	return ERR_OK;

}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *newpcb)
{
	struct tcp_server_struct *es;
	err_t ret_err;

	es = (struct tcp_server_struct *) arg;



	if (es != NULL)
	{
		if (es->p != NULL)
		{
			tcp_sent(newpcb, tcp_server_sent);

			tcp_server_send(newpcb, es);

			ret_err = ERR_OK;

		}
		else
		{
			if (es->state == ES_CLOSE)
			{
				tcp_server_connect_close(newpcb, es);

				ret_err = ERR_OK;

			}


		}

	}
	else
	{
		tcp_abort(newpcb);

		ret_err = ERR_ABRT;

	}

	return ret_err;

}

static void tcp_server_err(void *arg, err_t err)
{
	struct tcp_server_struct *es;
	err_t ret_err;

	es = (struct tcp_server_struct *) arg;

	if (es != NULL)
	{
		mem_free(es);

	}

	ret_err = err;

}




