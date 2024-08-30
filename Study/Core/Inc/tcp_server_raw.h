/*
 * tcp_server_raw.h
 *
 *  Created on: Jul 10, 2024
 *      Author: Danil
 */
/*This heder file used follow functions
 * struct tcp_pcb * tcp_new     (void);
 * err_t            tcp_bind    (struct tcp_pcb *pcb, const ip_addr_t *ipaddr,u16_t port);
 * #define          tcp_listen(pcb) tcp_listen_with_backlog(pcb, TCP_DEFAULT_LISTEN_BACKLOG);
 * void             tcp_accept  (struct tcp_pcb *pcb, tcp_accept_fn accept);
 * #define          tcp_accepted(pcb) do { LWIP_UNUSED_ARG(pcb); } while(0)  compatibility define, not needed any more
 * err_t            tcp_connect (struct tcp_pcb *pcb, const ip_addr_t *ipaddr,u16_t port, tcp_connected_fn connected);
 * USER FUNCTIONS
 * void tcp_srever_init(void);
 * static err_t tcp_server_accpet(void *arg,struct tcp_pcb *newpcb,err_t err);
 *
 *
 *
 * err_t            tcp_write   (struct tcp_pcb *pcb, const void *dataptr, u16_t len,u8_t apiflags);
 * void             tcp_sent    (struct tcp_pcb *pcb, tcp_sent_fn sent);
 * err_t            tcp_output  (struct tcp_pcb *pcb);
 * USER FUNCTIONS
 * static err_t tcp_server_send(void *arg,struct tcp_pcb *newpcb,err_t err);
 *
 *
 *
 * void             tcp_recv    (struct tcp_pcb *pcb, tcp_recv_fn recv);
 * void             tcp_recved  (struct tcp_pcb *pcb, u16_t len);
 * USER FUNCTIONS
 * static err_t tcp_server_recive(void *arg,struct tcp_pcb *newpcb,err_t err);
 *
 *
 *
 * void             tcp_poll    (struct tcp_pcb *pcb, tcp_poll_fn poll, u8_t interval);
 *
 *
 *
 * err_t            tcp_close   (struct tcp_pcb *pcb);
 * void             tcp_err     (struct tcp_pcb *pcb, tcp_err_fn err);
 * void             tcp_abort (struct tcp_pcb *pcb);
 *  */
#ifndef INC_TCP_SERVR_RAW_H_
#define INC_TCP_SERVR_RAW_H_

#define APP_RX_DATA_SIZE  2048
#define APP_TX_DATA_SIZE  2048

#endif /* INC_TCP_SERVR_RAW_H_ */
#include "tcp.h"
#include <string.h>

extern uint8_t USB_BufferFS[APP_TX_DATA_SIZE];

enum tcp_server_state
{
	ES_NONE = 0,
	ES_ACCEPT = 1,
	ES_RECIVED = 2,
	ES_CLOSE = 3

};

struct tcp_server_struct
{
	enum tcp_server_state state;
	struct tcp_pcb* pcb;
	struct pbuf* p;
};


void tcp_srever_init(void);

static void tcp_server_connect_close(struct tcp_pcb *newpcb, struct tcp_server_struct *es)
{
	err_t ret_err;

	tcp_arg(newpcb, NULL);
	//tcp_accept(newpcb, NULL);
	tcp_recv(newpcb, NULL);
	tcp_sent(newpcb, NULL);
	tcp_poll(newpcb, NULL, 0);
	tcp_err(newpcb, NULL);

	if (es != NULL)
	{

		mem_free(es);
	}

	ret_err = tcp_close(newpcb);

}

static void tcp_server_send(struct tcp_pcb *newpcb, struct tcp_server_struct *es)
{
	  struct pbuf *ptr;
	  err_t wr_err = ERR_OK;

	  while ((wr_err == ERR_OK) &&
	         (es->p != NULL) &&
	         (es->p->len <= tcp_sndbuf(newpcb)))
	  {

	    /* get pointer on pbuf from es structure */
	    ptr = es->p;

	    /* enqueue data for transmission */
	    wr_err = tcp_write(newpcb, ptr->payload, ptr->len, 1);

	    if (wr_err == ERR_OK)
	    {
	      u16_t plen;
	      u8_t freed;

	      plen = ptr->len;

	      /* continue with next pbuf in chain (if any) */
	      es->p = ptr->next;

	      if(es->p != NULL)
	      {
	        /* increment reference count for es->p */
	        pbuf_ref(es->p);
	      }

	     /* chop first pbuf from chain */
	      do
	      {
	        /* try hard to free pbuf */
	        freed = pbuf_free(ptr);
	      }
	      while(freed == 0);
	     /* we can read more data now */
	     tcp_recved(newpcb, plen);
	   }
	   else if(wr_err == ERR_MEM)
	   {
	      /* we are low on memory, try later / harder, defer to poll */
	     es->p = ptr;
	   }
	   else
	   {
	     /* other problem ?? */
	   }
	  }
}

static void tcp_server_hanler(struct tcp_pcb *newpcb, struct tcp_server_struct *es)
{
	struct tcp_server_struct *esTX;

//	ip4_addr_t inIP = newpcb->remote_ip;
//	uint8_t remPort = newpcb->remote_port;

	//char *remIP = ipaddr_ntoa(&inIP);

	esTX = es;

	if (esTX != NULL)
	{

		char *ptext;
		char text;


//		esTX->p = es->p;
//		esTX->pcb = es->pcb;
//		esTX->state = es->state;

		char buf[100];

		memset(buf,'\0',100);

		ptext = (char *)esTX->p->payload;
		text = *ptext;

		strncpy(buf, (char *)esTX->p->payload, esTX->p->tot_len);
		strcat(buf, "+ Hello from TCP SERVER\n");

		esTX->p->payload = (void *)USB_BufferFS;
		esTX->p->tot_len = (es->p->tot_len - es->p->len) + strlen (USB_BufferFS);
		esTX->p->len = strlen (USB_BufferFS);

		tcp_server_send(newpcb, esTX);

		pbuf_free(es->p);

	}

}

