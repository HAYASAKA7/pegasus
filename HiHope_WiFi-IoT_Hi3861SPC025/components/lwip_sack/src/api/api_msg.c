/**
 * @file
 * Sequential API Internal module
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "lwip/opt.h"

#if LWIP_NETCONN /* don't build if not configured for use in lwipopts.h */

#include "lwip/priv/api_msg.h"
#include "lwip/sys.h"

#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/raw.h"

#include "lwip/memp.h"
#include "lwip/igmp.h"
#include "lwip/dns.h"
#include "lwip/mld6.h"
#include "lwip/nd6.h"
#include "lwip/priv/tcpip_priv.h"
#include "lwip/filter.h"
#include <string.h>
#include "lwip/priv/tcp_priv.h"


/* netconns are polled once per second (e.g. continue write on memory error) */
#define NETCONN_TCP_POLL_INTERVAL 2

#define SET_NONBLOCKING_CONNECT(conn, val)  do { if (val) { \
  (conn)->flags |= NETCONN_FLAG_IN_NONBLOCKING_CONNECT; \
} else { \
  (conn)->flags &= (u8_t)~NETCONN_FLAG_IN_NONBLOCKING_CONNECT; } } while (0)

#define CLEAR_NONBLOCKING_CONNECT(conn)  do { \
  (conn)->flags &= (u8_t)~NETCONN_FLAG_IN_NONBLOCKING_CONNECT; } while (0)
#define IN_NONBLOCKING_CONNECT(conn) (((conn)->flags & NETCONN_FLAG_IN_NONBLOCKING_CONNECT) != 0)

/* forward declarations */
#if LWIP_TCP
#if LWIP_TCPIP_CORE_LOCKING
#define WRITE_DELAYED         , 1
#define WRITE_DELAYED_PARAM   , u8_t delayed
#else /* LWIP_TCPIP_CORE_LOCKING */
#define WRITE_DELAYED
#define WRITE_DELAYED_PARAM
#endif /* LWIP_TCPIP_CORE_LOCKING */
static err_t lwip_netconn_do_writemore(struct netconn *conn  WRITE_DELAYED_PARAM);
static err_t lwip_netconn_do_close_internal(struct netconn *conn  WRITE_DELAYED_PARAM);

u8_t netconn_reset;

#endif

#if LWIP_TCPIP_CORE_LOCKING
#define TCPIP_APIMSG_ACK(m)   NETCONN_SET_SAFE_ERR((m)->conn, (m)->err)
#else /* LWIP_TCPIP_CORE_LOCKING */
#define TCPIP_APIMSG_ACK(m)   do { NETCONN_SET_SAFE_ERR((m)->conn, (m)->err); sys_sem_signal(LWIP_API_MSG_SEM(m)); } while (0)
#endif /* LWIP_TCPIP_CORE_LOCKING */

#if LWIP_TCP
u8_t netconn_aborted;
u8_t netconn_memory_err;
#endif /* LWIP_TCP */

#if LWIP_RAW

#ifndef IPPROTO_RAW
#define IPPROTO_RAW     255
#endif

/**
 * Receive callback function for RAW netconns.
 * Doesn't 'eat' the packet, only copies it and sends it to
 * conn->recvmbox
 *
 * @see raw.h (struct raw_pcb.recv) for parameters and return value
 */
static u8_t
recv_raw(void *arg, struct raw_pcb *pcb, struct pbuf *p,
         const ip_addr_t *addr)
{
  struct pbuf *q = NULL;
  struct netbuf *buf = NULL;
  struct netconn *conn = NULL;
#if LWIP_SO_RCVBUF
  int recv_avail;
#endif
#if LWIP_SOCK_FILTER && PF_PKT_SUPPORT
  s32_t ret_filter;
#endif /* LWIP_SOCK_FILTER && PF_PKT_SUPPORT */

  LWIP_UNUSED_ARG(addr);

  conn = (struct netconn *)arg;

  if ((conn != NULL) && sys_mbox_valid(&conn->recvmbox) && netconn_mbox_is_active(conn)) {
#if LWIP_SOCK_FILTER && PF_PKT_SUPPORT
    /* do filter at the very beginning to make decision to accept or drop packets */
    if ((NETCONNTYPE_GROUP(conn->type) == NETCONN_PKT_RAW) && (conn->sk_filter.filter) &&
        (pbuf_header(p, -ETH_PAD_SIZE) == 0)) {
      /* drop the PAD size bytes */
      ret_filter = sock_filter(conn, p);
      (void)pbuf_header(p, ETH_PAD_SIZE);
      if (ret_filter == EPERM) {
        return 0;
      }
    }
#endif /* LWIP_SOCK_FILTER && PF_PKT_SUPPORT */
#if LWIP_SO_RCVBUF
    SYS_ARCH_GET(conn->recv_avail, recv_avail);
    if ((recv_avail + (int)(p->tot_len)) > conn->recv_bufsize) {
      return 0;
    }
#endif /* LWIP_SO_RCVBUF */
    /* copy the whole packet into new pbufs */
    q = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
    if (q != NULL) {
      if (pbuf_copy(q, p) != ERR_OK) {
        (void)pbuf_free(q);
        q = NULL;
      }
    }

    if (q != NULL) {
      u16_t len;
      buf = (struct netbuf *)memp_malloc(MEMP_NETBUF);
      if (buf == NULL) {
        (void)pbuf_free(q);
        return 0;
      }

#if PF_PKT_SUPPORT
      /* To get the pkt type filtered in ethernet_input */
      q->flags = p->flags;
#if ETH_PAD_SIZE
      /* exclude the begining two padding bytes in struct eth_hdr */
      if ((NETCONNTYPE_GROUP(conn->type) == NETCONN_PKT_RAW) && (pbuf_header(q, -ETH_PAD_SIZE))) {
        (void)pbuf_free(q);
        memp_free(MEMP_NETBUF, buf);
        return 0;
      }
#endif
#endif

      buf->p = q;
      buf->ptr = q;
#if PF_PKT_SUPPORT
      /* IP addr is NULL only when RAW packets are received for PF_PACKET sockets */
      if (addr == NULL) {
        ip_addr_set_any(IP_IS_V6_VAL(buf->addr), &buf->addr);
        buf->port = eth_current_hdr()->type;
        buf->hatype = eth_current_netif()->link_layer_type;
        buf->netifindex = eth_current_netif()->ifindex;
      } else {
        ip_addr_copy(buf->addr, *ip_current_src_addr());
        buf->port = pcb->proto.protocol;
      }
#else
      ip_addr_copy(buf->addr, *ip_current_src_addr());
      buf->port = pcb->protocol;
#endif

      len = q->tot_len;
      if (sys_mbox_trypost(&conn->recvmbox, buf) != ERR_OK) {
        netbuf_delete(buf);
        return 0;
      } else {
#if LWIP_SO_RCVBUF
        SYS_ARCH_INC(conn->recv_avail, len);
#endif /* LWIP_SO_RCVBUF */
        /* Register event with callback */
        API_EVENT(conn, NETCONN_EVT_RCVPLUS, len);
        return 1;
      }
    }
  }

  return 0; /* do not eat the packet */
}
#endif /* LWIP_RAW */

#if LWIP_UDP
/**
 * Receive callback function for UDP netconns.
 * Posts the packet to conn->recvmbox or deletes it on memory error.
 *
 * @see udp.h (struct udp_pcb.recv) for parameters
 */
void
recv_udp(void *arg, struct udp_pcb *pcb, struct pbuf *p,
         const ip_addr_t *addr, u16_t port)
{
  struct netbuf *buf = NULL;
  struct netconn *conn = NULL;
  u16_t len;
#if LWIP_SO_RCVBUF
  int recv_avail;
#endif /* LWIP_SO_RCVBUF */

  LWIP_UNUSED_ARG(pcb); /* only used for asserts... */
  LWIP_ASSERT("recv_udp must have a pcb argument", pcb != NULL);
  LWIP_ASSERT("recv_udp must have an argument", arg != NULL);
  conn = (struct netconn *)arg;

  if (conn == NULL) {
    (void)pbuf_free(p);
    return;
  }

  LWIP_ASSERT("recv_udp: recv for wrong pcb!", conn->pcb.udp == pcb);

#if LWIP_SO_RCVBUF
  SYS_ARCH_GET(conn->recv_avail, recv_avail);
  if (!((netconn_mbox_is_active(conn) != 0) && sys_mbox_valid(&conn->recvmbox)) ||
      ((recv_avail + (int)(p->tot_len)) > conn->recv_bufsize)) {
#else /* LWIP_SO_RCVBUF */
  if (!(sys_mbox_valid(&conn->recvmbox) && (netconn_mbox_is_active(conn) != 0))) {
#endif /* LWIP_SO_RCVBUF */
    (void)pbuf_free(p);
    return;
  }

  buf = (struct netbuf *)memp_malloc(MEMP_NETBUF);
  if (buf == NULL) {
    (void)pbuf_free(p);
    return;
  } else {
    buf->p = p;
    buf->ptr = p;
    ip_addr_set(&buf->addr, addr);
    buf->port = port;
#if LWIP_NETBUF_RECVINFO
    {
      if ((conn->flags & NETCONN_FLAG_PKTINFO) != 0) {
        /* get the UDP header - always in the first pbuf, ensured by udp_input */
        const struct udp_hdr *udphdr = (const struct udp_hdr *)ip_next_header_ptr();
#if LWIP_CHECKSUM_ON_COPY
        buf->flags = NETBUF_FLAG_DESTADDR;
#endif /* LWIP_CHECKSUM_ON_COPY */
        ip_addr_set(&buf->toaddr, ip_current_dest_addr());
        buf->toport_chksum = udphdr->dest;
      }
    }
#endif /* LWIP_NETBUF_RECVINFO */
  }

  len = p->tot_len;
  if (sys_mbox_trypost(&conn->recvmbox, buf) != ERR_OK) {
    netbuf_delete(buf);
    return;
  } else {
#if LWIP_SO_RCVBUF
    SYS_ARCH_INC(conn->recv_avail, len);
#endif /* LWIP_SO_RCVBUF */
    /* Register event with callback */
    API_EVENT(conn, NETCONN_EVT_RCVPLUS, len);
  }
}
#endif /* LWIP_UDP */

#if LWIP_TCP
/**
 * Receive callback function for TCP netconns.
 * Posts the packet to conn->recvmbox, but doesn't delete it on errors.
 *
 * @see tcp.h (struct tcp_pcb.recv) for parameters and return value
 */
static err_t
recv_tcp(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
  struct netconn *conn = NULL;
  u16_t len;

  LWIP_UNUSED_ARG(pcb);
  LWIP_ASSERT("recv_tcp must have a pcb argument", pcb != NULL);
  LWIP_ASSERT("recv_tcp must have an argument", arg != NULL);
  conn = (struct netconn *)arg;

  if (conn == NULL) {
    return ERR_VAL;
  }
  LWIP_ASSERT("recv_tcp: recv for wrong pcb!", conn->pcb.tcp == pcb);

  if (!(sys_mbox_valid(&conn->recvmbox) && (netconn_mbox_is_active(conn) != 0))) {
    /* recvmbox already deleted */
    if (p != NULL) {
      tcp_recved(pcb, p->tot_len);
      (void)pbuf_free(p);
    }

    return ERR_OK;
  }
  /* Unlike for UDP or RAW pcbs, don't check for available space
     using recv_avail since that could break the connection
     (data is already ACKed) */
  /* don't overwrite fatal errors! */
  if (err != ERR_OK) {
    NETCONN_SET_SAFE_ERR_VAL(conn, err);
  }

  if (p != NULL) {
    len = p->tot_len;
  } else {
    conn->shutdown = RCV_SHUTDOWN;
    len = 0;
  }

  if (sys_mbox_trypost(&conn->recvmbox, p) != ERR_OK) {
    /* don't deallocate p: it is presented to us later again from tcp_fasttmr! */
    return ERR_MEM;
  } else {
#if LWIP_SO_RCVBUF
    SYS_ARCH_INC(conn->recv_avail, len);
#endif /* LWIP_SO_RCVBUF */
    /* Register event with callback */
    API_EVENT(conn, NETCONN_EVT_RCVPLUS, len);
  }

  return ERR_OK;
}

/**
 * Poll callback function for TCP netconns.
 * Wakes up an application thread that waits for a connection to close
 * or data to be sent. The application thread then takes the
 * appropriate action to go on.
 *
 * Signals the conn->sem.
 * netconn_close waits for conn->sem if closing failed.
 *
 * @see tcp.h (struct tcp_pcb.poll) for parameters and return value
 */
static err_t
poll_tcp(void *arg, struct tcp_pcb *pcb)
{
  struct netconn *conn = (struct netconn *)arg;

  LWIP_UNUSED_ARG(pcb);
  LWIP_ASSERT("conn != NULL", (conn != NULL));

  if (conn->state == NETCONN_WRITE) {
    (void)lwip_netconn_do_writemore(conn  WRITE_DELAYED);
  } else if (conn->state == NETCONN_CLOSE) {
#if !LWIP_SO_SNDTIMEO && !LWIP_SO_LINGER
    if (conn->current_msg && conn->current_msg->msg.sd.polls_left) {
      conn->current_msg->msg.sd.polls_left--;
    }
#endif /* !LWIP_SO_SNDTIMEO && !LWIP_SO_LINGER */
    (void)lwip_netconn_do_close_internal(conn  WRITE_DELAYED);
  }
  /* @todo: implement connect timeout here? */
  /* Did a nonblocking write fail before? Then check available write-space. */
  if (conn->flags & NETCONN_FLAG_CHECK_WRITESPACE) {
    /* If the queued byte- or pbuf-count drops below the configured low-water limit,
       let select mark this pcb as writable again. */
    if ((conn->pcb.tcp != NULL) && (tcp_sndbuf(conn->pcb.tcp) > conn->pcb.tcp->snd_buf_lowat) &&
        (tcp_sndqueuelen(conn->pcb.tcp) < conn->pcb.tcp->snd_queuelen_lowat)) {
      conn->flags &= (u32_t)~NETCONN_FLAG_CHECK_WRITESPACE;
      API_EVENT(conn, NETCONN_EVT_SENDPLUS, 0);
    }
  }

  return ERR_OK;
}

#if LWIP_LOWPOWER
/* check wether need to poll tcp */
u8_t
poll_tcp_needed(void *arg, struct tcp_pcb *pcb)
{
  struct netconn *conn = (struct netconn *)arg;
  u8_t ret = 0;

  LWIP_UNUSED_ARG(pcb);
  if (conn == NULL) {
    return 0;
  }
  if ((conn->state == NETCONN_WRITE) || (conn->state == NETCONN_CLOSE)) {
    ret = 1;
  }

  /* Did a nonblocking write fail before? Then check available write-space. */
  if ((conn->flags & NETCONN_FLAG_CHECK_WRITESPACE) != 0) {
    /* If the queued byte- or pbuf-count drops below the configured low-water limit,
       let select mark this pcb as writable again. */
    if ((conn->pcb.tcp != NULL) && (tcp_sndbuf(conn->pcb.tcp) > conn->pcb.tcp->snd_buf_lowat) &&
        (tcp_sndqueuelen(conn->pcb.tcp) < conn->pcb.tcp->snd_queuelen_lowat)) {
      ret = 1;
    }
  }
  return ret;
}
#endif

/*
 * Sent callback function for TCP netconns.
 * Signals the conn->sem and calls API_EVENT.
 * netconn_write waits for conn->sem if send buffer is low.
 *
 * @see tcp.h (struct tcp_pcb.sent) for parameters and return value
 */
static err_t
sent_tcp(void *arg, struct tcp_pcb *pcb, u16_t len)
{
  struct netconn *conn = (struct netconn *)arg;
  SYS_ARCH_DECL_PROTECT(lev);

  LWIP_UNUSED_ARG(pcb);

  if (conn != NULL) {
    if (conn->state == NETCONN_WRITE) {
      (void)lwip_netconn_do_writemore(conn  WRITE_DELAYED);
    } else if (conn->state == NETCONN_CLOSE) {
      (void)lwip_netconn_do_close_internal(conn  WRITE_DELAYED);
    }

    /* If the queued byte- or pbuf-count drops below the configured low-water limit,
       let select mark this pcb as writable again. */
    if ((conn->pcb.tcp != NULL) && (tcp_sndbuf(conn->pcb.tcp) > conn->pcb.tcp->snd_buf_lowat) &&
        (tcp_sndqueuelen(conn->pcb.tcp) < conn->pcb.tcp->snd_queuelen_lowat)) {
      SYS_ARCH_PROTECT(lev);
      conn->flags &= (u32_t)~NETCONN_FLAG_CHECK_WRITESPACE;
      SYS_ARCH_UNPROTECT(lev);
      API_EVENT(conn, NETCONN_EVT_SENDPLUS, len);
    }
  }

  return ERR_OK;
}

/**
 * Error callback function for TCP netconns.
 * Signals conn->sem, posts to all conn mboxes and calls API_EVENT.
 * The application thread has then to decide what to do.
 *
 * @see tcp.h (struct tcp_pcb.err) for parameters
 */
static void
err_tcp(void *arg, err_t err)
{
  struct netconn *conn = NULL;
  enum netconn_state old_state;

  conn = (struct netconn *)arg;
  LWIP_ASSERT("conn != NULL", (conn != NULL));

  if (err == ERR_RST) {
    conn->refused_data = conn->pcb.tcp->refused_data;
    conn->pcb.tcp->refused_data = NULL;
    conn->pending_error = NETCONN_PENDING_ERR_RST;
  }

  if (conn->pcb.tcp->tcp_pcb_flag & TCP_PBUF_FLAG_TCP_FIN_RECV_SYSPOST_FAIL) {
    conn->pending_error = NETCONN_PENDING_ERR_FIN_RST;
  }

  conn->pcb.tcp = NULL;

  /* reset conn->state now before waking up other threads */
  old_state = conn->state;
  conn->state = NETCONN_NONE;

  /* close both direction for tcp connection in err state */
  conn->shutdown = SHUTDOWN_MASK;

  if (old_state == NETCONN_CLOSE) {
    /* RST during close: let close return success & dealloc the netconn */
    err = ERR_OK;
    NETCONN_SET_SAFE_ERR_VAL(conn, ERR_OK);
  } else {
    /* no check since this is always fatal! */
    SYS_ARCH_SET(conn->last_err, err);
  }

  /* @todo: the type of NETCONN_EVT created should depend on 'old_state' */
  /* Notify the user layer about a connection error. Used to signal select. */
  API_EVENT(conn, NETCONN_EVT_ERROR, 0);
  /* Try to release selects pending on 'read' or 'write', too.
     They will get an error if they actually try to read or write. */
  API_EVENT(conn, NETCONN_EVT_RCVPLUS, 0);
  API_EVENT(conn, NETCONN_EVT_SENDPLUS, 0);

  /* pass NULL-message to recvmbox to wake up pending recv */
  if (sys_mbox_valid(&conn->recvmbox)) {
    /* use trypost to prevent deadlock */
    /*
     * [Bug: 50912] No need to worry about whether post is success or not here.
     * Scenario 1) When DATA is already made the recvmbox full and RST is received.
     * ----Flag pending_error is marked NETCONN_PENDING_ERR_RST which upon user calling
     *     recv() will ensure that the refused data will be given to user and the final recv() will
     *     return with -1 and errno -104
     *   Scenario 2) When DATA is already made the recvmbox full and FIN and RST is received (Both FIN post
     *     and RST post has failed)
     *   ---- Flag pending_error is marked NETCONN_PENDING_ERR_FIN_RST which upon user calling
     *          recv() will ensure that the refused data will be given to user and the subsequent first
     *          recv() will return 0 to user for FIN and mnest subsequent recv() will return with -1
     *          and errno -104
     */
    (void)sys_mbox_trypost(&conn->recvmbox, (err == ERR_RST) ? (void *)(&netconn_reset) : &netconn_aborted);
  }
  /* pass NULL-message to acceptmbox to wake up pending accept */
  if (sys_mbox_valid(&conn->acceptmbox)) {
    /* use trypost to preven deadlock */
    (void)sys_mbox_trypost(&conn->acceptmbox, NULL);
  }

  if ((old_state == NETCONN_WRITE) || (old_state == NETCONN_CLOSE) ||
      (old_state == NETCONN_CONNECT)) {
    /* calling lwip_netconn_do_writemore/lwip_netconn_do_close_internal is not necessary
       since the pcb has already been deleted! */
    int was_nonblocking_connect = IN_NONBLOCKING_CONNECT(conn);
    CLEAR_NONBLOCKING_CONNECT(conn);

    if (!was_nonblocking_connect) {
      sys_sem_t *op_completed_sem = NULL;
      /* set error return code */
      LWIP_ASSERT("conn->current_msg != NULL", conn->current_msg != NULL);
      conn->current_msg->err = err;
      op_completed_sem = LWIP_API_MSG_SEM(conn->current_msg);
      LWIP_ASSERT("inavlid op_completed_sem", sys_sem_valid(op_completed_sem));
      conn->current_msg = NULL;
      /* wake up the waiting task */
      NETCONN_SET_SAFE_ERR(conn, err);
      sys_sem_signal(op_completed_sem);
    }
  } else {
    LWIP_ASSERT("conn->current_msg == NULL", conn->current_msg == NULL);
  }
}

#if DRIVER_STATUS_CHECK
void
update_tcp_sndplus_event(void *arg, struct tcp_pcb *pcb)
{
  struct netconn *conn = (struct netconn *)arg;

  LWIP_UNUSED_ARG(pcb);
  LWIP_ASSERT("conn != NULL", (conn != NULL));

  if (netconn_is_nonblocking(conn)) {
    API_EVENT(conn, NETCONN_EVT_SENDPLUS, 0);
  }
  return;
}
#endif

/**
 * Setup a tcp_pcb with the correct callback function pointers
 * and their arguments.
 *
 * @param conn the TCP netconn to setup
 */
static void
setup_tcp(struct netconn *conn)
{
  struct tcp_pcb *pcb = NULL;

  pcb = conn->pcb.tcp;
  tcp_arg(pcb, conn);
  tcp_recv(pcb, recv_tcp);
  tcp_sent(pcb, sent_tcp);
  tcp_poll(pcb, poll_tcp, NETCONN_TCP_POLL_INTERVAL);
  tcp_err(pcb, err_tcp);

#if DRIVER_STATUS_CHECK
  pcb->sndplus = update_tcp_sndplus_event;
#endif
}

/*
 * Accept callback function for TCP netconns.
 * Allocates a new netconn and posts that to conn->acceptmbox.
 *
 * @see tcp.h (struct tcp_pcb_listen.accept) for parameters and return value
 */
static err_t
accept_function(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  struct netconn *newconn = NULL;
  struct netconn *conn = (struct netconn *)arg;

  if (conn == NULL) {
    return ERR_VAL;
  }

  if (NETCONNTYPE_GROUP(NETCONN_TYPE(conn)) != NETCONN_TCP) {
    return ERR_VAL;
  }

  if (!sys_mbox_valid(&conn->acceptmbox) || (netconn_mbox_is_active(conn) == 0)) {
    LWIP_DEBUGF(API_MSG_DEBUG, ("accept_function: acceptmbox already deleted or getting deleted\n"));
    return ERR_VAL;
  }

  if (newpcb == NULL) {
    /* out-of-pcbs during connect: pass on this error to the application */
    if (sys_mbox_trypost(&conn->acceptmbox, &netconn_aborted) == ERR_OK) {
      /* Register event with callback */
      API_EVENT(conn, NETCONN_EVT_RCVPLUS, 0);
    }
    return ERR_VAL;
  }

  LWIP_DEBUGF(API_MSG_DEBUG, ("accept_function: newpcb->tate: %s\n", tcp_debug_state_str(newpcb->state)));

  /* We have to set the callback here even though
   * the new socket is unknown. newconn->socket is marked as -1. */
  newconn = netconn_alloc(conn->type, conn->callback);
  if (newconn == NULL) {
    /* outof netconns: pass on this error to the application */
    if (sys_mbox_trypost(&conn->acceptmbox, &netconn_memory_err) == ERR_OK) {
      /* Register event with callback */
      API_EVENT(conn, NETCONN_EVT_RCVPLUS, 0);
    }

    /* connection will be aborted here */
    tcp_err(newpcb, NULL);
    tcp_arg(newpcb, NULL);
    return ERR_MEM;
  }
  newconn->pcb.tcp = newpcb;
  setup_tcp(newconn);
  (void)atomic_set(&newconn->tcp_connected, 1);
  /* no protection: when creating the pcb, the netconn is not yet known
     to the application thread */
  newconn->last_err = err;

  ip_addr_copy(newconn->remote_ip, newpcb->remote_ip);
  newconn->remote_port = newpcb->remote_port;

  /* handle backlog counter */
  tcp_backlog_delayed(newpcb);

  if (sys_mbox_trypost(&conn->acceptmbox, newconn) != ERR_OK) {
    /* When returning != ERR_OK, the pcb is aborted in tcp_process(),
       so do nothing here! */
    /* remove all references to this netconn from the pcb */
    struct tcp_pcb *pcb = newconn->pcb.tcp;
    tcp_arg(pcb, NULL);
    tcp_recv(pcb, NULL);
    tcp_sent(pcb, NULL);
    tcp_poll(pcb, NULL, 0);
    tcp_err(pcb, NULL);
    /* remove reference from to the pcb from this netconn */
    newconn->pcb.tcp = NULL;

    NETCONN_SET_SAFE_MBOX_STATE(newconn, NETCONN_MBOX_DELETING);
    netconn_free(newconn);
    return ERR_MEM;
  } else {
#if LWIP_SO_RCVBUF
    SYS_ARCH_INC(conn->recv_avail, 1);
#endif /* LWIP_SO_RCVBUF */

    /* Register event with callback */
    API_EVENT(conn, NETCONN_EVT_RCVPLUS, 0);
  }

  return ERR_OK;
}
#endif /* LWIP_TCP */

/**
 * Create a new pcb of a specific type.
 * Called from lwip_netconn_do_newconn().
 *
 * @param msg the api_msg_msg describing the connection type
 */
static void
pcb_new(struct api_msg *msg)
{
  enum lwip_ip_addr_type iptype = IPADDR_TYPE_V4;

  LWIP_ASSERT("pcb_new: pcb already allocated", msg->conn->pcb.tcp == NULL);

#if LWIP_IPV6 && LWIP_IPV4
  /* IPv6: Dual-stack by default, unless netconn_set_ipv6only() is called */
  if (NETCONNTYPE_ISIPV6(NETCONN_TYPE(msg->conn))) {
    iptype = IPADDR_TYPE_ANY;
  }
#endif

  /* Allocate a PCB for this connection */
  switch (NETCONNTYPE_GROUP(msg->conn->type)) {
#if LWIP_RAW
    case NETCONN_RAW:
      msg->conn->pcb.raw = raw_new_ip_type(iptype, (u8_t)msg->msg.n.proto);
      if (msg->conn->pcb.raw != NULL) {
#if LWIP_IPV6
#if LWIP_SOCK_OPT_ICMP6_FILTER
        /* rfc3542.  Section: 3.2.  ICMPv6 Type Filtering */
        u32_t i;
        for (i = 0; i < ICMP_FILTER_LENGTH; i++) {
          msg->conn->pcb.raw->icmp6_filter.icmp6_filt[i] = 0x00;
        }

#endif
        /* ICMPv6 packets should always have checksum calculated by the stack as per RFC 3542 chapter 3.1 */
        if (NETCONNTYPE_ISIPV6(msg->conn->type) &&
            (msg->conn->pcb.raw->raw_proto == IP6_NEXTH_ICMP6)) {
          msg->conn->pcb.raw->chksum_reqd = 1;
          msg->conn->pcb.raw->chksum_offset = 2;
        }
#endif /* LWIP_IPV6 */

#if LWIP_IPV4
        /* IP_HDRINCL is enabled by default */
        if (!(NETCONNTYPE_ISIPV6(msg->conn->type)) && (msg->msg.n.proto == IPPROTO_RAW)) {
          msg->conn->pcb.raw->hdrincl = 1;
        }
#endif

#if LWIP_IPV6 && LWIP_MAC_SECURITY
        msg->conn->pcb.raw->macsec_reqd = 1;
#endif /* LWIP_MAC_SECURITY */

#if LWIP_SO_PRIORITY
        msg->conn->pcb.raw->priority = LWIP_PKT_PRIORITY_DEFAULT;
#endif /* LWIP_SO_PRIORITY */

        raw_recv(msg->conn->pcb.raw, recv_raw, msg->conn);
      }
      break;

#if PF_PKT_SUPPORT
    case NETCONN_PKT_RAW:
      msg->conn->pcb.pkt_raw = raw_pkt_new(msg->msg.n.proto);
      if (msg->conn->pcb.pkt_raw == NULL) {
        msg->err = ERR_MEM;
        break;
      }
      raw_recv(msg->conn->pcb.pkt_raw, recv_raw, msg->conn);
      break;
#endif

#endif /* LWIP_RAW */
#if LWIP_UDP
    case NETCONN_UDP:
      msg->conn->pcb.udp = udp_new_ip_type(iptype);
      if (msg->conn->pcb.udp != NULL) {
#if LWIP_UDPLITE
        if (NETCONNTYPE_ISUDPLITE(msg->conn->type)) {
          udp_setflags(msg->conn->pcb.udp, UDP_FLAGS_UDPLITE);
        }
#endif /* LWIP_UDPLITE */
        if (NETCONNTYPE_ISUDPNOCHKSUM(msg->conn->type)) {
          udp_setflags(msg->conn->pcb.udp, UDP_FLAGS_NOCHKSUM);
        }

#if LWIP_IPV6 && LWIP_MAC_SECURITY
        msg->conn->pcb.udp->macsec_reqd = 1;
#endif /* LWIP_MAC_SECURITY */

#if LWIP_SO_PRIORITY
        msg->conn->pcb.udp->priority = LWIP_PKT_PRIORITY_DEFAULT;
#endif /* LWIP_SO_PRIORITY */

        udp_recv(msg->conn->pcb.udp, recv_udp, msg->conn);
      }
      break;
#endif /* LWIP_UDP */
#if LWIP_TCP
    case NETCONN_TCP:
      msg->conn->pcb.tcp = tcp_new_ip_type(iptype);
      if (msg->conn->pcb.tcp != NULL) {
#if LWIP_SO_PRIORITY
        msg->conn->pcb.tcp->priority = LWIP_PKT_PRIORITY_DEFAULT;
#endif /* LWIP_SO_PRIORITY */

        setup_tcp(msg->conn);
      }
      break;
#endif /* LWIP_TCP */
    default:
      /* Unsupported netconn type, e.g. protocol disabled */
      msg->err = ERR_VAL;
      return;
  }
  if (msg->conn->pcb.ip == NULL) {
    msg->err = ERR_MEM;
  }
}

/**
 * Create a new pcb of a specific type inside a netconn.
 * Called from netconn_new_with_proto_and_callback.
 *
 * @param m the api_msg_msg describing the connection type
 */
void
lwip_netconn_do_newconn(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;

  msg->err = ERR_OK;
  if (msg->conn->pcb.tcp == NULL) {
    pcb_new(msg);
  }
  /* Else? This "new" connection already has a PCB allocated. */
  /* Is this an error condition? Should it be deleted? */
  /* We currently just are happy and return. */
  TCPIP_APIMSG_ACK(msg);
}

/**
 * Create a new netconn (of a specific type) that has a callback function.
 * The corresponding pcb is NOT created!
 *
 * @param t the type of 'connection' to create (@see enum netconn_type)
 * @param callback a function to call on status changes (RX available, TX'ed)
 * @return a newly allocated struct netconn or
 *         NULL on memory error
 */
struct netconn *
netconn_alloc(enum netconn_type t, netconn_callback callback)
{
  struct netconn *conn = NULL;
  int size;

  conn = (struct netconn *)memp_malloc(MEMP_NETCONN);
  if (conn == NULL) {
    return NULL;
  }

  (void)memset_s(conn, sizeof(struct netconn), 0, sizeof(struct netconn));
  conn->last_err = ERR_OK;
  conn->type = t;
  conn->pcb.tcp = NULL;

  /* If all sizes are the same, every compiler should optimize this switch to nothing */
  switch (NETCONNTYPE_GROUP(t)) {
#if LWIP_RAW
#if PF_PKT_SUPPORT
    case NETCONN_PKT_RAW:
#endif
    case NETCONN_RAW:
      size = DEFAULT_RAW_RECVMBOX_SIZE;
      break;
#endif /* LWIP_RAW */
#if LWIP_UDP
    case NETCONN_UDP:
      size = DEFAULT_UDP_RECVMBOX_SIZE;
      break;
#endif /* LWIP_UDP */
#if LWIP_TCP
    case NETCONN_TCP:
      size = DEFAULT_TCP_RECVMBOX_SIZE;
      break;
#endif /* LWIP_TCP */
    default:
      LWIP_ASSERT("netconn_alloc: undefined netconn_type", 0);
      goto free_and_return;
  }

  if (sys_mbox_new_auto_expand(&conn->recvmbox, size) != ERR_OK) {
    goto free_and_return;
  }

#if !LWIP_NETCONN_SEM_PER_THREAD
  if (sys_sem_new(&conn->op_completed, 0) != ERR_OK) {
    sys_mbox_free(&conn->recvmbox);
    goto free_and_return;
  }
#endif

#if LWIP_TCP
  sys_mbox_set_invalid(&conn->acceptmbox);
#endif

  (void)atomic_set(&conn->mbox_state, NETCONN_MBOX_ACTIVE);
  conn->state        = NETCONN_NONE;
#if LWIP_SOCKET
  /* initialize socket to -1 since 0 is a valid socket */
  conn->socket       = -1;
#endif /* LWIP_SOCKET */
  conn->callback     = callback;
#if LWIP_TCP
  conn->current_msg  = NULL;
  conn->write_offset = 0;
#endif /* LWIP_TCP */
#if LWIP_SO_SNDTIMEO
  conn->send_timeout = 0;
#endif /* LWIP_SO_SNDTIMEO */
#if LWIP_SO_RCVTIMEO
  conn->recv_timeout = 0;
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVBUF
  conn->recv_bufsize = RECV_BUFSIZE_DEFAULT;
  conn->recv_avail   = 0;
#endif /* LWIP_SO_RCVBUF */
#if LWIP_SO_LINGER
  conn->linger = -1;
#endif /* LWIP_SO_LINGER */
  conn->flags = 0;
  conn->shutdown = NON_SHUTDOWN;

#if LWIP_SOCK_FILTER
  conn->sk_filter.len = 0;
  conn->sk_filter.filter = NULL;
#endif
#if LWIP_TCP
  conn->pending_error = 0;
  conn->refused_data = NULL;
  ip_addr_set_zero(&conn->remote_ip);
  conn->remote_port = 0;
#endif

  return conn;
free_and_return:
  memp_free(MEMP_NETCONN, conn);
  return NULL;
}

/**
 * Delete a netconn and all its resources.
 * The pcb is NOT freed (since we might not be in the right thread context do this).
 *
 * @param conn the netconn to free
 */
void
netconn_free(struct netconn *conn)
{
  LWIP_ASSERT("PCB must be deallocated outside this function", conn->pcb.tcp == NULL);

  /* mbox should be in deleting state here */
  LWIP_ASSERT("acceptmbox must be inactive before calling this function",
              !netconn_mbox_is_active(conn));

#if LWIP_TCP
  if (conn->refused_data != NULL) {
    (void)pbuf_free(conn->refused_data);
    conn->refused_data = NULL;
  }
#endif

  if (sys_mbox_valid(&conn->recvmbox)) {
    sys_mbox_free(&conn->recvmbox);
    sys_mbox_set_invalid(&conn->recvmbox);
  }
#if LWIP_TCP
  if (sys_mbox_valid(&conn->acceptmbox)) {
    sys_mbox_free(&conn->acceptmbox);
    sys_mbox_set_invalid(&conn->acceptmbox);
  }
#endif /* LWIP_TCP */

#if !LWIP_NETCONN_SEM_PER_THREAD
  sys_sem_free(&conn->op_completed);
  sys_sem_set_invalid(&conn->op_completed);
#endif
#if LWIP_SOCK_FILTER
  if (conn->sk_filter.filter != NULL) {
    mem_free(conn->sk_filter.filter);
    conn->sk_filter.filter = NULL;
    conn->sk_filter.len = 0;
  }
#endif

  memp_free(MEMP_NETCONN, conn);
}

/**
 * Delete rcvmbox and acceptmbox of a netconn and free the left-over data in
 * these mboxes
 *
 * @param conn the netconn to free
 * @bytes_drained bytes drained from recvmbox
 * @accepts_drained pending connections drained from acceptmbox
 */
static void
netconn_drain(struct netconn *conn)
{
  void *mem = NULL;
#if LWIP_TCP
  struct pbuf *p = NULL;
#endif /* LWIP_TCP */

  /* This runs in tcpip_thread, so we don't need to lock against rx packets */
  /* Delete and drain the recvmbox. */
  if (sys_mbox_valid(&conn->recvmbox)) {
    while (sys_arch_mbox_fetch(&conn->recvmbox, &mem, 1) != SYS_MBOX_EMPTY) {
#if LWIP_TCP
      if (NETCONNTYPE_GROUP(conn->type) == NETCONN_TCP) {
        if ((mem != NULL) && (mem != &netconn_aborted) && (mem != &netconn_memory_err) && (mem != &netconn_reset)) {
          p = (struct pbuf *)mem;
          /* pcb might be set to NULL already by err_tcp() */
          /*
           * RFC 1122:  If such a host issues a
           * CLOSE call while received data is still pending in TCP, or
           * if new data is received after CLOSE is called, its TCP
           * SHOULD send a RST to show that data was lost.
           */
          /* It would be wrong to update recv_wnd if data is not received by stack */
          (void)pbuf_free(p);
        }
      } else
#endif /* LWIP_TCP */
      {
        netbuf_delete((struct netbuf *)mem);
      }
    }

    /* pass NULL-message to recvmbox to wake up pending recv */
    /* use trypost to prevent deadlock */
    (void)sys_mbox_trypost(&conn->recvmbox, NULL);
  }

  /* Delete and drain the acceptmbox. */
#if LWIP_TCP
  if (sys_mbox_valid(&conn->acceptmbox)) {
    while (sys_arch_mbox_fetch(&conn->acceptmbox, &mem, 1) != SYS_MBOX_EMPTY) {
      if ((mem != &netconn_aborted) && (mem != &netconn_memory_err)) {
        struct netconn *newconn = (struct netconn *)mem;
        /* Only tcp pcbs have an acceptmbox, so no need to check conn->type */
        /* pcb might be set to NULL already by err_tcp() */
        /* drain recvmbox, acceptmbox might contain NULL entry */
        if (newconn != NULL) {
          netconn_drain(newconn);
          if (newconn->pcb.tcp != NULL) {
            tcp_abort(newconn->pcb.tcp);
            newconn->pcb.tcp = NULL;
          }
          netconn_free(newconn);
        }
      }
    }

    /* pass NULL-message to acceptmbox to wake up pending accept */
    /* use trypost to preven deadlock */
    (void)sys_mbox_trypost(&conn->acceptmbox, NULL);
  }
#endif /* LWIP_TCP */

  NETCONN_SET_SAFE_MBOX_STATE(conn, NETCONN_MBOX_DELETING);
}

#if LWIP_TCP
/**
 * Internal helper function to close a TCP netconn: since this sometimes
 * doesn't work at the first attempt, this function is called from multiple
 * places.
 *
 * @param conn the TCP netconn to close
 */
static err_t
lwip_netconn_do_close_internal(struct netconn *conn  WRITE_DELAYED_PARAM)
{
  err_t err;
  u8_t shut, shut_rx, shut_tx, close;
  u8_t close_finished = 0;
  struct tcp_pcb *tpcb = NULL;
#if LWIP_SO_LINGER
  u8_t linger_wait_required = 0;
#endif /* LWIP_SO_LINGER */

  LWIP_ASSERT("invalid conn", (conn != NULL));
  LWIP_ASSERT("this is for tcp netconns only", (NETCONNTYPE_GROUP(conn->type) == NETCONN_TCP));
  LWIP_ASSERT("conn must be in state NETCONN_CLOSE", (conn->state == NETCONN_CLOSE));
  LWIP_ASSERT("pcb already closed", (conn->pcb.tcp != NULL));
  LWIP_ASSERT("conn->current_msg != NULL", conn->current_msg != NULL);

  tpcb = conn->pcb.tcp;
  shut = conn->current_msg->msg.sd.shut;
  shut_rx = shut & NETCONN_SHUT_RD;
  shut_tx = shut & NETCONN_SHUT_WR;
  /* shutting down both ends is the same as closing
     (also if RD or WR side was shut down before already) */
  if (shut == NETCONN_SHUT_RDWR) {
    close = 1;
  } else if (shut_rx &&
             ((tpcb->state == FIN_WAIT_1) ||
              (tpcb->state == FIN_WAIT_2) ||
              (tpcb->state == CLOSING))) {
    close = 1;
  } else if (shut_tx && ((tpcb->flags & TF_RXCLOSED) != 0)) {
    close = 1;
  } else {
    close = 0;
  }

  /* Set back some callback pointers */
  if (close) {
    tcp_arg(tpcb, NULL);
  }
  if (tpcb->state == LISTEN) {
    tcp_accept(tpcb, NULL);
  } else {
    /* some callbacks have to be reset if tcp_close is not successful */
    if (shut_rx) {
      conn->shutdown = RCV_SHUTDOWN;
      tcp_recv(tpcb, NULL);
      tcp_accept(tpcb, NULL);
    }
    if (shut_tx) {
      conn->shutdown = SND_SHUTDOWN;
      tcp_sent(tpcb, NULL);
    }
    if (close) {
      conn->shutdown = SHUTDOWN_MASK;
      tcp_poll(tpcb, NULL, 0);
      tcp_err(tpcb, NULL);
    }
  }
  /* Try to close the connection */
  if (close) {
#if LWIP_SO_LINGER
    /* check linger possibilites before calling tcp_close */
    err = ERR_OK;
    /* linger enabled/required at all? (i.e. is there untransmitted data left?) */
    if ((conn->linger >= 0) && (conn->pcb.tcp->unsent || conn->pcb.tcp->unacked)) {
      if ((conn->linger == 0)) {
        /* data left but linger prevents waiting */
        tcp_abort(tpcb);
        tpcb = NULL;
      } else if (conn->linger > 0) {
        /* data left and linger says we should wait */
        if (netconn_is_nonblocking(conn)) {
          /* data left on a nonblocking netconn -> cannot linger */
          err = ERR_WOULDBLOCK;
        } else if ((s32_t)(sys_now() - conn->current_msg->msg.sd.time_started) >=
                   (conn->linger * 1000)) {
          /* data left but linger timeout has expired (this happens on further
             calls to this function through poll_tcp */
          tcp_abort(tpcb);
          tpcb = NULL;
        } else {
          /* data left -> need to wait for ACK after successful close */
          linger_wait_required = 1;
        }
      }
    }
    if ((err == ERR_OK) && (tpcb != NULL))
#endif /* LWIP_SO_LINGER */
    {
      err = tcp_close(tpcb);
    }
  } else {
    err = tcp_shutdown(tpcb, shut_rx, shut_tx);
  }
  if (err == ERR_OK) {
    close_finished = 1;
#if LWIP_SO_LINGER
    if (linger_wait_required) {
      /* wait for ACK of all unsent/unacked data by just getting called again */
      close_finished = 0;
      err = ERR_INPROGRESS;
    }
#endif /* LWIP_SO_LINGER */
  } else {
    if (err == ERR_MEM) {
      /* Closing failed because of memory shortage, try again later. Even for
         nonblocking netconns, we have to wait since no standard socket application
         is prepared for close failing because of resource shortage.
         Check the timeout: this is kind of an lwip addition to the standard sockets:
         we wait for some time when failing to allocate a segment for the FIN */
#if LWIP_SO_SNDTIMEO || LWIP_SO_LINGER
      s32_t close_timeout = LWIP_TCP_CLOSE_TIMEOUT_MS_DEFAULT;
#if LWIP_SO_SNDTIMEO
      if (conn->send_timeout > 0) {
        close_timeout = conn->send_timeout;
      }
#endif /* LWIP_SO_SNDTIMEO */
#if LWIP_SO_LINGER
      if (conn->linger >= 0) {
        /* use linger timeout (seconds) */
        close_timeout = conn->linger * 1000;
      }
#endif
      if ((s32_t)(sys_now() - conn->current_msg->msg.sd.time_started) >= close_timeout) {
#else /* LWIP_SO_SNDTIMEO || LWIP_SO_LINGER */
      if (conn->current_msg->msg.sd.polls_left == 0) {
#endif /* LWIP_SO_SNDTIMEO || LWIP_SO_LINGER */
        close_finished = 1;
        if (close) {
          /* in this case, we want to RST the connection */
          tcp_abort(tpcb);
          err = ERR_OK;
        }
      }
    } else {
      /* Closing failed for a non-memory error: give up */
      close_finished = 1;
    }
  }
  if (close_finished) {
    /* Closing done (succeeded, non-memory error, nonblocking error or timeout) */
    sys_sem_t *op_completed_sem = LWIP_API_MSG_SEM(conn->current_msg);
    conn->current_msg->err = err;
    conn->current_msg = NULL;
    conn->state = NETCONN_NONE;
    if (err == ERR_OK) {
      if (close) {
        /* Set back some callback pointers as conn is going away */
        conn->pcb.tcp = NULL;
        /* Trigger select() in socket layer. Make sure everybody notices activity
         on the connection, error first! */
        API_EVENT(conn, NETCONN_EVT_ERROR, 0);
      }
      if (shut_rx) {
        API_EVENT(conn, NETCONN_EVT_RCVPLUS, 0);
      }
      if (shut_tx) {
        API_EVENT(conn, NETCONN_EVT_SENDPLUS, 0);
      }
    }
    NETCONN_SET_SAFE_ERR(conn, err);
#if LWIP_TCPIP_CORE_LOCKING
    if (delayed)
#endif
    {
      /* wake up the application task */
      sys_sem_signal(op_completed_sem);
    }
    return ERR_OK;
  }
  if (!close_finished) {
    /* Closing failed and we want to wait: restore some of the callbacks */
    /* Closing of listen pcb will never fail! */
    LWIP_ASSERT("Closing a listen pcb may not fail!", (tpcb->state != LISTEN));
    if (shut_tx) {
      conn->shutdown = RCV_SHUTDOWN;
      tcp_sent(tpcb, sent_tcp);
    }
    /* when waiting for close, set up poll interval to 500ms */
    tcp_poll(tpcb, poll_tcp, 1);
    tcp_err(tpcb, err_tcp);
    tcp_arg(tpcb, conn);
    /* don't restore recv callback: we don't want to receive any more data */
  }
  /* If closing didn't succeed, we get called again either
     from poll_tcp or from sent_tcp */
  LWIP_ASSERT("err != ERR_OK", err != ERR_OK);
  return err;
}
#endif /* LWIP_TCP */

/**
 * Delete the pcb inside a netconn.
 * Called from netconn_delete.
 *
 * @param m the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_delconn(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;

  enum netconn_state state = msg->conn->state;
  LWIP_ASSERT("netconn state error", /* this only happens for TCP netconns */
              (state == NETCONN_NONE) || (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP));
#if LWIP_NETCONN_FULLDUPLEX
  /* In full duplex mode, blocking write/connect is aborted with ERR_CLSD */
  if (state != NETCONN_NONE) {
    if ((state == NETCONN_WRITE) ||
        ((state == NETCONN_CONNECT) && !IN_NONBLOCKING_CONNECT(msg->conn))) {
      /* close requested, abort running write/connect */
      sys_sem_t *op_completed_sem = NULL;
      LWIP_ASSERT("msg->conn->current_msg != NULL", msg->conn->current_msg != NULL);
      op_completed_sem = LWIP_API_MSG_SEM(msg->conn->current_msg);
      msg->conn->current_msg->err = ERR_CLSD;
      msg->conn->current_msg = NULL;
      msg->conn->write_offset = 0;
      msg->conn->state = NETCONN_NONE;
      NETCONN_SET_SAFE_ERR(msg->conn, ERR_CLSD);
      sys_sem_signal(op_completed_sem);
    }
  }
#else /* LWIP_NETCONN_FULLDUPLEX */
  if (((state != NETCONN_NONE) &&
       (state != NETCONN_LISTEN) &&
       (state != NETCONN_CONNECT)) ||
      ((state == NETCONN_CONNECT) && !IN_NONBLOCKING_CONNECT(msg->conn))) {
    /* This means either a blocking write or blocking connect is running
       (nonblocking write returns and sets state to NONE) */
    msg->err = ERR_INPROGRESS;
  } else
#endif /* LWIP_NETCONN_FULLDUPLEX */
  {
    LWIP_ASSERT("blocking connect in progress",
                (state != NETCONN_CONNECT) || IN_NONBLOCKING_CONNECT(msg->conn));
    msg->err = ERR_OK;
    /* Drain and delete mboxes */
    netconn_drain(msg->conn);

    if (msg->conn->pcb.tcp != NULL) {
      switch (NETCONNTYPE_GROUP(msg->conn->type)) {
#if LWIP_RAW
        case NETCONN_RAW:
          raw_remove(msg->conn->pcb.raw);
          break;
#if PF_PKT_SUPPORT
        case NETCONN_PKT_RAW:
          raw_pkt_remove(msg->conn->pcb.pkt_raw);
          break;
#endif
#endif /* LWIP_RAW */
#if LWIP_UDP
        case NETCONN_UDP:
          msg->conn->pcb.udp->recv_arg = NULL;
          udp_remove(msg->conn->pcb.udp);
          break;
#endif /* LWIP_UDP */
#if LWIP_TCP
        case NETCONN_TCP:
          LWIP_ASSERT("already writing or closing", msg->conn->current_msg == NULL &&
                      msg->conn->write_offset == 0);
          msg->conn->state = NETCONN_CLOSE;
          msg->msg.sd.shut = NETCONN_SHUT_RDWR;
          msg->conn->current_msg = msg;
#if LWIP_TCPIP_CORE_LOCKING
          if (lwip_netconn_do_close_internal(msg->conn, 0) != ERR_OK) {
            LWIP_ASSERT("state!", msg->conn->state == NETCONN_CLOSE);
            UNLOCK_TCPIP_CORE();
            (void)sys_arch_sem_wait(LWIP_API_MSG_SEM(msg), 0);
            LOCK_TCPIP_CORE();
            LWIP_ASSERT("state!", msg->conn->state == NETCONN_NONE);
          }
#else /* LWIP_TCPIP_CORE_LOCKING */
          (void)lwip_netconn_do_close_internal(msg->conn);
#endif /* LWIP_TCPIP_CORE_LOCKING */
          /* API_EVENT is called inside lwip_netconn_do_close_internal, before releasing
             the application thread, so we can return at this point! */
          return;
#endif /* LWIP_TCP */
        default:
          break;
      }
      msg->conn->pcb.tcp = NULL;
    }
    /* tcp netconns don't come here! */
    /* @todo: this lets select make the socket readable and writable,
       which is wrong! errfd instead? */
    API_EVENT(msg->conn, NETCONN_EVT_RCVPLUS, 0);
    API_EVENT(msg->conn, NETCONN_EVT_SENDPLUS, 0);
  }
  if (sys_sem_valid(LWIP_API_MSG_SEM(msg))) {
    TCPIP_APIMSG_ACK(msg);
  }
}

/**
 * Bind a pcb contained in a netconn
 * Called from netconn_bind.
 *
 * @param m the api_msg_msg pointing to the connection and containing
 *          the IP address and port to bind to
 */
void
lwip_netconn_do_bind(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;
  struct netif *netif = NULL;
  netif = netif_find_by_ipaddr(msg->msg.bc.ipaddr);

  if (ERR_IS_FATAL(msg->conn->last_err)) {
    msg->err = (err_t)((msg->conn->state == NETCONN_CLOSED) ? ERR_VAL : msg->conn->last_err);
  } else if (!(ip_addr_isany(msg->msg.bc.ipaddr) ||
               ip_addr_ismulticast(msg->msg.bc.ipaddr) ||
               (netif_ipaddr_isbrdcast(msg->msg.bc.ipaddr) != 0) ||
               ((netif != NULL) && (netif_is_up(netif) != 0)))) {
    msg->err = ERR_NOADDR;
  } else {
    msg->err = ERR_VAL;
    if (msg->conn->pcb.tcp != NULL) {
      switch (NETCONNTYPE_GROUP(msg->conn->type)) {
#if LWIP_RAW
        case NETCONN_RAW:
          msg->err = raw_bind(msg->conn->pcb.raw, API_EXPR_REF(msg->msg.bc.ipaddr));
          break;
#if PF_PKT_SUPPORT
        case NETCONN_PKT_RAW:
          msg->err = raw_pkt_bind(msg->conn->pcb.pkt_raw, msg->msg.bc.netifindex, msg->msg.bc.port);
          break;
#endif
#endif /* LWIP_RAW */
#if LWIP_UDP
        case NETCONN_UDP:
          msg->err = udp_bind(msg->conn->pcb.udp, API_EXPR_REF(msg->msg.bc.ipaddr), msg->msg.bc.port);
          break;
#endif /* LWIP_UDP */
#if LWIP_TCP
        case NETCONN_TCP:
          if (msg->conn->pcb.tcp->local_port == 0) {
            msg->err = tcp_bind(msg->conn->pcb.tcp, API_EXPR_REF(msg->msg.bc.ipaddr), msg->msg.bc.port);
          } else {
            msg->err = ERR_VAL;
          }

          break;
#endif /* LWIP_TCP */
        default:
          msg->err = ERR_OPNOTSUPP;
          break;
      }
    }
  }
  TCPIP_APIMSG_ACK(msg);
}

#if LWIP_TCP
/**
 * TCP callback function if a connection (opened by tcp_connect/lwip_netconn_do_connect) has
 * been established (or reset by the remote host).
 *
 * @see tcp.h (struct tcp_pcb.connected) for parameters and return values
 */
static err_t
lwip_netconn_do_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{
  struct netconn *conn = NULL;
  int was_blocking;
  sys_sem_t *op_completed_sem = NULL;

  LWIP_UNUSED_ARG(pcb);

  conn = (struct netconn *)arg;

  if (conn == NULL) {
    return ERR_VAL;
  }

  LWIP_ASSERT("conn->state == NETCONN_CONNECT", conn->state == NETCONN_CONNECT);
  LWIP_ASSERT("(conn->current_msg != NULL) || conn->in_non_blocking_connect",
              (conn->current_msg != NULL) || IN_NONBLOCKING_CONNECT(conn));

  if (conn->current_msg != NULL) {
    conn->current_msg->err = err;
    op_completed_sem = LWIP_API_MSG_SEM(conn->current_msg);
  }
  if ((NETCONNTYPE_GROUP(conn->type) == NETCONN_TCP) && (err == ERR_OK)) {
    setup_tcp(conn);
    tcp_sndbuf_init(pcb);
    ip_addr_copy(conn->remote_ip, pcb->remote_ip);
    conn->remote_port = pcb->remote_port;
    (void)atomic_set(&conn->tcp_connected, 1);
  }
  was_blocking = !IN_NONBLOCKING_CONNECT(conn);
  CLEAR_NONBLOCKING_CONNECT(conn);
  LWIP_ASSERT("blocking connect state error",
              (was_blocking && op_completed_sem != NULL) ||
              (!was_blocking && op_completed_sem == NULL));
  conn->current_msg = NULL;
  conn->state = NETCONN_NONE;
  NETCONN_SET_SAFE_ERR_VAL(conn, err);

  API_EVENT(conn, NETCONN_EVT_SENDPLUS, 0);

  if (was_blocking && (op_completed_sem != NULL)) {
    sys_sem_signal(op_completed_sem);
  }
  return ERR_OK;
}
#endif /* LWIP_TCP */

/**
 * Connect a pcb contained inside a netconn
 * Called from netconn_connect.
 *
 * @param m the api_msg_msg pointing to the connection and containing
 *          the IP address and port to connect to
 */
void
lwip_netconn_do_connect(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;
  if (ERR_IS_FATAL(msg->conn->last_err)) {
    msg->err =  msg->conn->last_err;
    TCPIP_APIMSG_ACK(msg);
    return;
  }

  if (msg->conn->pcb.tcp == NULL) {
    /* This may happen when calling netconn_connect() a second time */
    msg->err = ERR_CLSD;
  } else {
    switch (NETCONNTYPE_GROUP(msg->conn->type)) {
#if LWIP_RAW
      case NETCONN_RAW:
        msg->err = raw_connect(msg->conn->pcb.raw, API_EXPR_REF(msg->msg.bc.ipaddr));
        break;
#endif /* LWIP_RAW */
#if LWIP_UDP
      case NETCONN_UDP:
        msg->err = udp_connect(msg->conn->pcb.udp, API_EXPR_REF(msg->msg.bc.ipaddr), msg->msg.bc.port);
        break;
#endif /* LWIP_UDP */
#if LWIP_TCP
      case NETCONN_TCP:
        /* Prevent connect while doing any other action. */
        if (msg->conn->state == NETCONN_CONNECT) {
          msg->err = ERR_ALREADY;
        } else if (msg->conn->state == NETCONN_LISTEN) {
          msg->err = ERR_OPNOTSUPP;
        } else if (msg->conn->state != NETCONN_NONE) {
          msg->err  = ERR_ISCONN;
        } else {
          setup_tcp(msg->conn);
          msg->err = tcp_connect(msg->conn->pcb.tcp, API_EXPR_REF(msg->msg.bc.ipaddr),
                                 msg->msg.bc.port, lwip_netconn_do_connected);
          if (msg->err == ERR_OK) {
            u8_t non_blocking = netconn_is_nonblocking(msg->conn);
            msg->conn->state = NETCONN_CONNECT;
            SET_NONBLOCKING_CONNECT(msg->conn, non_blocking);
            if (non_blocking) {
              msg->err = ERR_INPROGRESS;
            } else {
              msg->conn->current_msg = msg;
              /* sys_sem_signal() is called from lwip_netconn_do_connected (or err_tcp()),
                 when the connection is established! */
#if LWIP_TCPIP_CORE_LOCKING
              LWIP_ASSERT("state!", msg->conn->state == NETCONN_CONNECT);
              UNLOCK_TCPIP_CORE();
              (void)sys_arch_sem_wait(LWIP_API_MSG_SEM(msg), 0);
              LOCK_TCPIP_CORE();
              LWIP_ASSERT("state!", msg->conn->state != NETCONN_CONNECT);
#endif /* LWIP_TCPIP_CORE_LOCKING */
              return;
            }
          }
        }
        break;
#endif /* LWIP_TCP */
      default:
        LWIP_DEBUGF(API_MSG_DEBUG, ("Invalid netconn type\n"));
        msg->err = ERR_OPNOTSUPP;
        break;
    }
  }
  /* For all other protocols, netconn_connect() calls TCPIP_APIMSG(),
     so use TCPIP_APIMSG_ACK() here. */
  TCPIP_APIMSG_ACK(msg);
}

/**
 * Disconnect a pcb contained inside a netconn
 * Only used for UDP netconns.
 * Called from netconn_disconnect.
 *
 * @param m the api_msg_msg pointing to the connection to disconnect
 */
void
lwip_netconn_do_disconnect(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;

#if LWIP_UDP
  if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_UDP) {
    udp_disconnect(msg->conn->pcb.udp);
    msg->err = ERR_OK;
  } else
#endif /* LWIP_UDP */
  {
    msg->err = ERR_VAL;
  }
  TCPIP_APIMSG_ACK(msg);
}

#if LWIP_TCP
/**
 * Set a TCP pcb contained in a netconn into listen mode
 * Called from netconn_listen.
 *
 * @param m the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_listen(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;

  if (ERR_IS_FATAL(msg->conn->last_err)) {
    msg->err = msg->conn->last_err;
  } else {
    msg->err = ERR_VAL; // shutdown is already called, invalid state for socket
    if (msg->conn->pcb.tcp != NULL) {
      if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP) {
        if (msg->conn->pcb.tcp->local_port == 0) {
          msg->err = ERR_NODEST;
          goto SEND_ERROR;
        }
        if (msg->conn->state == NETCONN_NONE) {
          struct tcp_pcb *lpcb;
          if (msg->conn->pcb.tcp->state != CLOSED) {
            /* connection is not closed, cannot listen */
            msg->err = ERR_VAL;
          } else {
            err_t err;
            u32_t added;
            u8_t backlog;
#if TCP_LISTEN_BACKLOG
            backlog = msg->msg.lb.backlog;
#else /* TCP_LISTEN_BACKLOG */
            backlog = TCP_DEFAULT_LISTEN_BACKLOG;
#endif /* TCP_LISTEN_BACKLOG */

            msg->err = ERR_OK;
            added = 0;
            if (!sys_mbox_valid(&msg->conn->acceptmbox)) {
              msg->err = sys_mbox_new(&msg->conn->acceptmbox, DEFAULT_ACCEPTMBOX_SIZE);
              if (msg->err != ERR_OK) {
                msg->err = ERR_BUF;
                goto SEND_ERROR;
              }

              added = 1;
            }

            lpcb = tcp_listen_with_backlog_and_err(msg->conn->pcb.tcp, backlog, &err);
            if (lpcb == NULL) {
              /* in this case, the old pcb is still allocated */
              msg->err = err;

              /* delete the recvmbox and allocate the acceptmbox */
              if (added) {
                sys_mbox_free(&msg->conn->acceptmbox);
                sys_mbox_set_invalid(&msg->conn->acceptmbox);
              }
            } else {
              /* delete the recvmbox and allocate the acceptmbox */
              if (sys_mbox_valid(&msg->conn->recvmbox)) {
                /* @todo: should we drain the recvmbox here? */
                sys_mbox_free(&msg->conn->recvmbox);
                sys_mbox_set_invalid(&msg->conn->recvmbox);
              }

              msg->conn->state = NETCONN_LISTEN;
              msg->conn->pcb.tcp = lpcb;
              tcp_arg(msg->conn->pcb.tcp, msg->conn);
              tcp_accept(msg->conn->pcb.tcp, accept_function);
            }

#if LWIP_IPV4 && LWIP_IPV6
            /* "Socket API like" dual-stack support: If IP to listen to is IP6_ADDR_ANY,
             * and NETCONN_FLAG_IPV6_V6ONLY is NOT set, use IP_ANY_TYPE to listen
             */
            if (ip_addr_cmp(&msg->conn->pcb.ip->local_ip, IP6_ADDR_ANY) &&
                (netconn_get_ipv6only(msg->conn) == 0)) {
              /* change PCB type to IPADDR_TYPE_ANY */
              IP_SET_TYPE_VAL(msg->conn->pcb.tcp->local_ip,  IPADDR_TYPE_ANY);
              IP_SET_TYPE_VAL(msg->conn->pcb.tcp->remote_ip, IPADDR_TYPE_ANY);
            }
#endif /* LWIP_IPV4 && LWIP_IPV6 */
          }
        } else if (msg->conn->state == NETCONN_LISTEN) {
          /* already listening, allow updating of the backlog */
          msg->err = ERR_OK;
          tcp_backlog_set(msg->conn->pcb.tcp, msg->msg.lb.backlog);
        }
      } else {
        msg->err = ERR_OPNOTSUPP;
      }
    }
  }
SEND_ERROR:
  TCPIP_APIMSG_ACK(msg);
}
#endif /* LWIP_TCP */

/**
 * Send some data on a RAW or UDP pcb contained in a netconn
 * Called from netconn_send
 *
 * @param m the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_send(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;

  if (ERR_IS_FATAL(msg->conn->last_err)) {
    msg->err = msg->conn->last_err;
  } else {
    msg->err = ERR_CLSD;
    if (msg->conn->pcb.tcp != NULL) {
      switch (NETCONNTYPE_GROUP(msg->conn->type)) {
#if LWIP_RAW
        case NETCONN_RAW:
          if (ip_addr_isany(&msg->msg.b->addr) || IP_IS_ANY_TYPE_VAL(msg->msg.b->addr)) {
            struct ip_hdr *iphdr = NULL;
            ip_addr_t dest_addr;

            if (((msg->conn->pcb.raw->flags & RAW_FLAGS_CONNECTED) == 0) ||
                ip_addr_isany(&msg->conn->pcb.raw->remote_ip)) {
              if (msg->conn->pcb.raw->hdrincl) {
                if (msg->msg.b->p->len < IP_HLEN) {
                  msg->err = ERR_MSGSIZE;
                  break;
                }

                /* IP header already included in p */
                iphdr = (struct ip_hdr *)msg->msg.b->p->payload;
                ip_addr_copy_from_ip4(dest_addr, iphdr->dest);
                msg->err = raw_sendto(msg->conn->pcb.raw, msg->msg.b->p, &dest_addr);
                break;
              }

              msg->err = ERR_NODEST;
              break;
            }

            msg->err = raw_send(msg->conn->pcb.raw, msg->msg.b->p);
          } else {
            msg->err = raw_sendto(msg->conn->pcb.raw, msg->msg.b->p, &msg->msg.b->addr);
          }

          break;
#if PF_PKT_SUPPORT
        case NETCONN_PKT_RAW:
          /* Check if its for sending RAW packets for PF_PACKET family */
          if ((msg->msg.b->flags & NETBUF_FLAG_IFINDEX) != 0) {
            msg->err = raw_pkt_sendto(msg->conn->pcb.pkt_raw, msg->msg.b->p, msg->msg.b->netifindex);
          } else {
            msg->err = raw_pkt_sendto(msg->conn->pcb.pkt_raw, msg->msg.b->p, 0);
          }
          break;
#endif
#endif
#if LWIP_UDP
        case NETCONN_UDP:
#if LWIP_CHECKSUM_ON_COPY
          if (ip_addr_isany(&msg->msg.b->addr) || IP_IS_ANY_TYPE_VAL(msg->msg.b->addr)) {
            if ((msg->conn->pcb.udp->flags & UDP_FLAGS_PEER_ADDR_SET) == 0) {
              msg->err = ERR_NODEST;
              break;
            }

            msg->err = udp_send_chksum(msg->conn->pcb.udp, msg->msg.b->p,
                                       msg->msg.b->flags & NETBUF_FLAG_CHKSUM, msg->msg.b->toport_chksum);
          } else {
            msg->err = udp_sendto_chksum(msg->conn->pcb.udp, msg->msg.b->p,
                                         &msg->msg.b->addr, msg->msg.b->port,
                                         msg->msg.b->flags & NETBUF_FLAG_CHKSUM, msg->msg.b->toport_chksum);
          }
#else /* LWIP_CHECKSUM_ON_COPY */
          if (ip_addr_isany_val(msg->msg.b->addr) || IP_IS_ANY_TYPE_VAL(msg->msg.b->addr)) {
            if ((msg->conn->pcb.udp->flags & UDP_FLAGS_PEER_ADDR_SET) == 0) {
              msg->err = ERR_NODEST;
              break;
            }

            msg->err = udp_send(msg->conn->pcb.udp, msg->msg.b->p);
          } else {
            msg->err = udp_sendto(msg->conn->pcb.udp, msg->msg.b->p, &msg->msg.b->addr, msg->msg.b->port);
          }
#endif /* LWIP_CHECKSUM_ON_COPY */
          break;
#endif /* LWIP_UDP */
        default:
          break;
      }
    }
  }
  TCPIP_APIMSG_ACK(msg);
}

#if LWIP_TCP
/**
 * Indicate data has been received from a TCP pcb contained in a netconn
 * Called from netconn_recv
 *
 * @param m the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_recv(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;

  msg->err = ERR_OK;
  if (ERR_IS_FATAL(msg->conn->last_err)) {
    msg->err = msg->conn->last_err;
  } else if (msg->conn->pcb.tcp != NULL) {
    if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP) {
      u32_t remaining = msg->msg.r.len;
      do {
        u16_t recved = (u16_t)((remaining > 0xffff) ? 0xffff : (u16_t)remaining);
        tcp_recved(msg->conn->pcb.tcp, recved);
        remaining -= recved;
      } while (remaining != 0);
    }
  }
  TCPIP_APIMSG_ACK(msg);
}

#if TCP_LISTEN_BACKLOG
/** Indicate that a TCP pcb has been accepted
 * Called from netconn_accept
 *
 * @param m the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_accepted(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;

  msg->err = ERR_OK;
  if (msg->conn->pcb.tcp != NULL) {
    if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP) {
      tcp_backlog_accepted(msg->conn->pcb.tcp);
    }
  }
  TCPIP_APIMSG_ACK(msg);
}
#endif /* TCP_LISTEN_BACKLOG */

/**
 * See if more data needs to be written from a previous call to netconn_write.
 * Called initially from lwip_netconn_do_write. If the first call can't send all data
 * (because of low memory or empty send-buffer), this function is called again
 * from sent_tcp() or poll_tcp() to send more data. If all data is sent, the
 * blocking application thread (waiting in netconn_write) is released.
 *
 * @param conn netconn (that is currently in state NETCONN_WRITE) to process
 * @return ERR_OK
 *         ERR_MEM if LWIP_TCPIP_CORE_LOCKING=1 and sending hasn't yet finished
 */
static err_t
lwip_netconn_do_writemore(struct netconn *conn  WRITE_DELAYED_PARAM)
{
  err_t err;
  const void *dataptr = NULL;
  tcpwnd_size_t len, available;
  u8_t write_finished = 0;
  size_t diff;
  u8_t dontblock;
  u8_t dontblock_sock;

  u8_t apiflags;

  LWIP_ASSERT("conn != NULL", conn != NULL);
  LWIP_ASSERT("conn->state == NETCONN_WRITE", (conn->state == NETCONN_WRITE));
  LWIP_ASSERT("conn->current_msg != NULL", conn->current_msg != NULL);
  LWIP_ASSERT("conn->pcb.tcp != NULL", conn->pcb.tcp != NULL);
  LWIP_ASSERT("conn->write_offset < conn->current_msg->msg.w.len",
              conn->write_offset < conn->current_msg->msg.w.len);

  apiflags = conn->current_msg->msg.w.apiflags;

  dontblock_sock = netconn_is_nonblocking(conn);
  dontblock = dontblock_sock || (apiflags & NETCONN_DONTBLOCK);

#if DRIVER_STATUS_CHECK
  if ((dontblock != 0) && (conn->pcb.tcp->drv_status == DRV_NOT_READY)) {
    sys_sem_t *op_completed_sem = LWIP_API_MSG_SEM(conn->current_msg);

    if (dontblock_sock) {
      LWIP_DEBUGF(DRV_STS_DEBUG, ("Driver Not Ready. So sending SENDMINUS event\n"));
      API_EVENT(conn, NETCONN_EVT_SENDMINUS, 0);
    }

    /* check for state, and ensure correct errno is set while returning ... */
    if (atomic_read(&conn->tcp_connected) == 0) {
      err = ERR_CONN;
      LWIP_DEBUGF(DRV_STS_DEBUG, ("Driver Not Ready. But connection is also not established, So returning ERR_CONN\n"));
    } else {
      LWIP_DEBUGF(DRV_STS_DEBUG, ("Driver Not Ready. So returning ERR_WOULDBLOCK\n"));
      err = ERR_WOULDBLOCK;
    }

    conn->current_msg->err = err;
    conn->current_msg = NULL;
    conn->state = NETCONN_NONE;

    sys_sem_signal(op_completed_sem);
    return err;
  }
#else
  if (atomic_read(&conn->tcp_connected) == 0) {
    sys_sem_t *op_completed_sem = LWIP_API_MSG_SEM(conn->current_msg);
    /* check for state, and ensure correct errno is set while returning ... */
    err = ERR_CONN;
    LWIP_DEBUGF(DRV_STS_DEBUG, ("Connection is also not established, So returning ERR_CONN\n"));

    conn->current_msg->err = err;
    conn->current_msg = NULL;
    conn->state = NETCONN_NONE;

    sys_sem_signal(op_completed_sem);
    return err;
  }
#endif

#if LWIP_SO_SNDTIMEO
  if ((conn->send_timeout != 0) &&
      ((s32_t)(sys_now() - conn->current_msg->msg.w.time_started) >= conn->send_timeout)) {
    write_finished = 1;
    if (conn->write_offset == 0) {
      /* nothing has been written */
      err = ERR_WOULDBLOCK;
      conn->current_msg->msg.w.len = 0;
    } else {
      /* partial write */
      err = ERR_OK;
      conn->current_msg->msg.w.len = conn->write_offset;
      conn->write_offset = 0;
    }
  } else
#endif /* LWIP_SO_SNDTIMEO */
  {
    dataptr = (const u8_t *)conn->current_msg->msg.w.dataptr + conn->write_offset;
    diff = conn->current_msg->msg.w.len - conn->write_offset;
#if LWIP_WND_SCALE
    len = diff;
#else
    if (diff > 0xffffUL) { /* max_u16_t */
      len = 0xffff;
      apiflags |= TCP_WRITE_FLAG_MORE;
    } else {
      len = (u16_t)diff;
    }
#endif
    available = tcp_sndbuf(conn->pcb.tcp);
    if (available < len) {
      /* don't try to write more than sendbuf */
      len = available;
      if (dontblock) {
        if (!len) {
          err = ERR_WOULDBLOCK;
          goto err_mem;
        }
      } else {
        apiflags |= TCP_WRITE_FLAG_MORE;
      }
    }
    LWIP_ASSERT("lwip_netconn_do_writemore: invalid length!",
                ((conn->write_offset + len) <= conn->current_msg->msg.w.len));
    err = tcp_write(conn->pcb.tcp, dataptr, len, apiflags);
    /* if OK or memory error, check available space */
    if ((err == ERR_OK) || (err == ERR_MEM)) {
err_mem:
      if (dontblock && (len < conn->current_msg->msg.w.len)) {
        /* non-blocking write did not write everything: mark the pcb non-writable
           and let poll_tcp check writable space to mark the pcb writable again */
        API_EVENT(conn, NETCONN_EVT_SENDMINUS, len);
        conn->flags |= NETCONN_FLAG_CHECK_WRITESPACE;
      } else if ((tcp_sndbuf(conn->pcb.tcp) <= conn->pcb.tcp->snd_buf_lowat) ||
                 (tcp_sndqueuelen(conn->pcb.tcp) >= conn->pcb.tcp->snd_queuelen_lowat)) {
        /* The queued byte- or pbuf-count exceeds the configured low-water limit,
           let select mark this pcb as non-writable. */
        API_EVENT(conn, NETCONN_EVT_SENDMINUS, len);
      }
    }

    if (err == ERR_OK) {
      err_t out_err;
      conn->write_offset += len;
      if ((conn->write_offset == conn->current_msg->msg.w.len) || dontblock) {
        /* return sent length */
        conn->current_msg->msg.w.len = conn->write_offset;
        /* everything was written */
        write_finished = 1;
      }
      out_err = tcp_output(conn->pcb.tcp);
      if (ERR_IS_FATAL(out_err) || (out_err == ERR_RTE)) {
        /* If tcp_output fails with fatal error or no route is found,
           don't try writing any more but return the error
           to the application thread. */
        err = out_err;
        write_finished = 1;
        conn->current_msg->msg.w.len = 0;
      }
    } else if (err == ERR_MEM) {
      /* If ERR_MEM, we wait for sent_tcp or poll_tcp to be called.
         For blocking sockets, we do NOT return to the application
         thread, since ERR_MEM is only a temporary error! Non-blocking
         will remain non-writable until sent_tcp/poll_tcp is called */
      /* tcp_write returned ERR_MEM, try tcp_output anyway */
      err_t out_err = tcp_output(conn->pcb.tcp);
      if (ERR_IS_FATAL(out_err) || (out_err == ERR_RTE)) {
        /* If tcp_output fails with fatal error or no route is found,
           don't try writing any more but return the error
           to the application thread. */
        err = out_err;
        write_finished = 1;
        conn->current_msg->msg.w.len = 0;
      } else if (dontblock) {
        /* non-blocking write is done on ERR_MEM */
        err = ERR_WOULDBLOCK;
        write_finished = 1;
        conn->current_msg->msg.w.len = 0;
      }
    } else {
      /* On errors != ERR_MEM, we don't try writing any more but return
         the error to the application thread. */
      write_finished = 1;
      conn->current_msg->msg.w.len = 0;
    }
  }
  if (write_finished) {
    /* everything was written: set back connection state
       and back to application task */
    sys_sem_t *op_completed_sem = LWIP_API_MSG_SEM(conn->current_msg);
    conn->current_msg->err = err;
    conn->current_msg = NULL;
    conn->write_offset = 0;
    conn->state = NETCONN_NONE;
    NETCONN_SET_SAFE_ERR(conn, err);
#if LWIP_TCPIP_CORE_LOCKING
    if (delayed)
#endif
    {
      sys_sem_signal(op_completed_sem);
    }
  }
#if LWIP_TCPIP_CORE_LOCKING
  else {
    return ERR_MEM;
  }
#endif
  return ERR_OK;
}
#endif /* LWIP_TCP */

/**
 * Send some data on a TCP pcb contained in a netconn
 * Called from netconn_write
 *
 * @param m the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_write(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;
  if (ERR_IS_FATAL(msg->conn->last_err)) {
    msg->err = (err_t)((msg->conn->last_err == ERR_CLSD) ? ERR_PIPE : msg->conn->last_err);
  } else {
    if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP) {
#if LWIP_TCP
      if (msg->conn->state != NETCONN_NONE) {
        /* netconn is connecting, closing or in blocking write */
        msg->err = ((msg->conn->state == NETCONN_LISTEN) ? ERR_PIPE : ERR_INPROGRESS);
        msg->err = (err_t)((msg->conn->state == NETCONN_CONNECT) ? ERR_CONN : msg->err);
      } else if (msg->conn->pcb.tcp != NULL) {
        msg->conn->state = NETCONN_WRITE;
        /* set all the variables used by lwip_netconn_do_writemore */
        LWIP_ASSERT("already writing or closing", msg->conn->current_msg == NULL &&
                    msg->conn->write_offset == 0);
        LWIP_ASSERT("msg->msg.w.len != 0", msg->msg.w.len != 0);
        msg->conn->current_msg = msg;
        msg->conn->write_offset = 0;
#if LWIP_TCPIP_CORE_LOCKING
        if (lwip_netconn_do_writemore(msg->conn, 0) != ERR_OK) {
          LWIP_ASSERT("state!", msg->conn->state == NETCONN_WRITE);
          UNLOCK_TCPIP_CORE();
          (void)sys_arch_sem_wait(LWIP_API_MSG_SEM(msg), 0);
          LOCK_TCPIP_CORE();
          LWIP_ASSERT("state!", msg->conn->state != NETCONN_WRITE);
        }
#else /* LWIP_TCPIP_CORE_LOCKING */
        (void)lwip_netconn_do_writemore(msg->conn);
#endif /* LWIP_TCPIP_CORE_LOCKING */
        /* for both cases: if lwip_netconn_do_writemore was called, don't ACK the APIMSG
           since lwip_netconn_do_writemore ACKs it! */
        return;
      } else {
        msg->err = ERR_PIPE;
      }
#else /* LWIP_TCP */
      msg->err = ERR_VAL;
#endif /* LWIP_TCP */
#if (LWIP_UDP || LWIP_RAW)
    } else {
      msg->err = ERR_VAL;
#endif /* (LWIP_UDP || LWIP_RAW) */
    }
  }
  TCPIP_APIMSG_ACK(msg);
}

/**
 * Return a connection's local or remote address
 * Called from netconn_getaddr
 *
 * @param m the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_getaddr(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;

  if ((msg->conn == NULL) || (msg->conn->pcb.tcp == NULL)) {
    msg->err = ERR_CONN;
    TCPIP_APIMSG_ACK(msg);
    return;
  }
  if (msg->conn->pcb.ip != NULL) {
    msg->err = ERR_OK;
    switch (NETCONNTYPE_GROUP(msg->conn->type)) {
#if LWIP_RAW
      case NETCONN_RAW:
        if (msg->msg.ad.local) {
          ip_addr_copy(API_EXPR_DEREF(msg->msg.ad.ipaddr),
                       msg->conn->pcb.raw->local_ip);
          API_EXPR_DEREF(msg->msg.ad.port) = msg->conn->pcb.raw->raw_proto;
        } else {
          if ((msg->conn->pcb.raw->flags & RAW_FLAGS_CONNECTED) != 0) {
            ip_addr_copy(API_EXPR_DEREF(msg->msg.ad.ipaddr),
                         msg->conn->pcb.raw->remote_ip);
            API_EXPR_DEREF(msg->msg.ad.port) = msg->conn->pcb.raw->raw_proto;
          } else {
            /* return an error as connecting is only a helper for upper layers */
            msg->err = ERR_CONN;
          }
        }
        break;
#endif /* LWIP_RAW */
#if PF_PKT_SUPPORT
      case NETCONN_PKT_RAW: {
        ip_addr_t addr_info;
        /* Non standard way of do it, but doing it :( */
        if (msg->msg.ad.local) {
          ip_addr_set_ip4_u32_val(&addr_info, (u32_t)msg->conn->pcb.pkt_raw->netifindex);
          ip_addr_copy(API_EXPR_DEREF(msg->msg.ad.ipaddr), addr_info);
          API_EXPR_DEREF(msg->msg.ad.port) = msg->conn->pcb.pkt_raw->proto.eth_proto;
        } else {
          msg->err = ERR_OPNOTSUPP;
        }
        break;
      }
#endif
#if LWIP_UDP
      case NETCONN_UDP:
        if (msg->msg.ad.local) {
          ip_addr_copy(API_EXPR_DEREF(msg->msg.ad.ipaddr),
                       msg->conn->pcb.udp->local_ip);
          API_EXPR_DEREF(msg->msg.ad.port) = msg->conn->pcb.udp->local_port;
        } else {
          if ((msg->conn->pcb.udp->flags & UDP_FLAGS_CONNECTED) == 0) {
            msg->err = ERR_CONN;
          } else {
            ip_addr_copy(API_EXPR_DEREF(msg->msg.ad.ipaddr),
                         msg->conn->pcb.ip->remote_ip);
            API_EXPR_DEREF(msg->msg.ad.port) = msg->conn->pcb.udp->remote_port;
          }
        }
        break;
#endif /* LWIP_UDP */
#if LWIP_TCP
      case NETCONN_TCP:
        if (msg->msg.ad.local) {
          ip_addr_copy(API_EXPR_DEREF(msg->msg.ad.ipaddr),
                       msg->conn->pcb.ip->local_ip);
          API_EXPR_DEREF(msg->msg.ad.port) = msg->conn->pcb.tcp->local_port;
        } else {
          if ((msg->conn->pcb.tcp->state == CLOSED) || (msg->conn->pcb.tcp->state == LISTEN)) {
            /* pcb is not connected and remote name is requested */
            msg->err = ERR_CONN;
          } else {
            ip_addr_copy(API_EXPR_DEREF(msg->msg.ad.ipaddr),
                         msg->conn->pcb.ip->remote_ip);
            API_EXPR_DEREF(msg->msg.ad.port) = msg->conn->pcb.tcp->remote_port;
          }
        }

        break;
#endif /* LWIP_TCP */
      default:
        msg->err = ERR_OPNOTSUPP;
        LWIP_ASSERT("invalid netconn_type", 0);
        break;
    }
  } else {
    msg->err = ERR_CLSD;
  }
  TCPIP_APIMSG_ACK(msg);
}

/**
 * Close or half-shutdown a TCP pcb contained in a netconn
 * Called from netconn_close
 * In contrast to closing sockets, the netconn is not deallocated.
 *
 * @param m the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_close(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;

#if LWIP_TCP
  enum netconn_state state = msg->conn->state;
  /* First check if this is a TCP netconn and if it is in a correct state
      (LISTEN doesn't support half shutdown) */
  if ((msg->conn->pcb.tcp != NULL) &&
      (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP) &&
      ((msg->msg.sd.shut == NETCONN_SHUT_RDWR) || (state != NETCONN_LISTEN))) {
    LWIP_ERROR("lwip_netconn_do_close: shutdown can not be done when in CLOSED state",
               msg->conn->net_tcp_state != CLOSED, msg->err = ERR_CONN; TCPIP_APIMSG_ACK(msg); return);

    /* Check if we are in a connected state */
    if (state == NETCONN_CONNECT) {
      /* TCP connect in progress: cannot shutdown */
      msg->err = ERR_CONN;
    } else if (state == NETCONN_WRITE) {
#if LWIP_NETCONN_FULLDUPLEX
      if (msg->msg.sd.shut & NETCONN_SHUT_WR) {
        /* close requested, abort running write */
        sys_sem_t *write_completed_sem;
        LWIP_ASSERT("msg->conn->current_msg != NULL", msg->conn->current_msg != NULL);
        write_completed_sem = LWIP_API_MSG_SEM(msg->conn->current_msg);
        msg->conn->current_msg->err = ERR_CLSD;
        msg->conn->current_msg = NULL;
        msg->conn->write_offset = 0;
        msg->conn->state = NETCONN_NONE;
        state = NETCONN_NONE;
        NETCONN_SET_SAFE_ERR(msg->conn, ERR_CLSD);
        sys_sem_signal(write_completed_sem);
      } else {
        LWIP_ASSERT("msg->msg.sd.shut == NETCONN_SHUT_RD", msg->msg.sd.shut == NETCONN_SHUT_RD);
        /* In this case, let the write continue and do not interfere with
           conn->current_msg or conn->state! */
        msg->err = tcp_shutdown(msg->conn->pcb.tcp, 1, 0);
      }
    }
    if ((state == NETCONN_NONE) || (state == NETCONN_LISTEN)) {
#else /* LWIP_NETCONN_FULLDUPLEX */
      msg->err = ERR_INPROGRESS;
    } else {
#endif /* LWIP_NETCONN_FULLDUPLEX */
      if (msg->msg.sd.shut & NETCONN_SHUT_RD) {
        /* Drain and delete mboxes */
        msg->conn->shutdown = RCV_SHUTDOWN;
        netconn_drain(msg->conn);
      }
      LWIP_ASSERT("already writing or closing", msg->conn->current_msg == NULL &&
                  msg->conn->write_offset == 0);
      msg->conn->state = NETCONN_CLOSE;
      msg->conn->current_msg = msg;
#if LWIP_TCPIP_CORE_LOCKING
      if (lwip_netconn_do_close_internal(msg->conn, 0) != ERR_OK) {
        LWIP_ASSERT("state!", msg->conn->state == NETCONN_CLOSE);
        UNLOCK_TCPIP_CORE();
        (void)sys_arch_sem_wait(LWIP_API_MSG_SEM(msg), 0);
        LOCK_TCPIP_CORE();
        LWIP_ASSERT("state!", msg->conn->state == NETCONN_NONE);
      }
#else /* LWIP_TCPIP_CORE_LOCKING */
      (void)lwip_netconn_do_close_internal(msg->conn);
#endif /* LWIP_TCPIP_CORE_LOCKING */
      /* for tcp netconns, lwip_netconn_do_close_internal ACKs the message */
      return;
    }
  } else
#endif /* LWIP_TCP */
  {
    msg->err = ERR_CLSD;
    if ((msg->msg.sd.shut != NETCONN_SHUT_RDWR) && (msg->conn->state == NETCONN_LISTEN)) {
      /* LISTEN doesn't support half shutdown */
      msg->err = ERR_CONN;
    }
  }
  TCPIP_APIMSG_ACK(msg);
}

#if LWIP_IGMP || (LWIP_IPV6 && LWIP_IPV6_MLD)
/*
 * Leave multicast groups for UDP netconns.
 * Called from netconn_leave_group and netconn_leave_group_netif
 *
 * @param m the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_leave_group(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;

  msg->err = ERR_CONN;
  if (ERR_IS_FATAL(msg->conn->last_err)) {
    msg->err = msg->conn->last_err;
  } else {
    if (msg->conn->pcb.udp != NULL) {
      if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_UDP) {
#if LWIP_UDP
#if LWIP_IPV6 && LWIP_IPV6_MLD
        if (NETCONNTYPE_ISIPV6(msg->conn->type)) {
          struct netif *netif = NULL;

          netif = netif_get_by_index(msg->msg.jl.if_idx);
          if (netif == NULL) {
            msg->err = ERR_IF;
            TCPIP_APIMSG_ACK(msg);
            return;
          }

          if (msg->msg.jl.join_or_leave == NETCONN_LEAVE) {
            msg->err = mld6_leavegroup_netif(netif,
                                             ip_2_ip6(API_EXPR_REF(msg->msg.jl.multiaddr)));
          }
        } else
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
        {
#if LWIP_IGMP
          if (msg->msg.jl.join_or_leave == NETCONN_LEAVE) {
            msg->err = igmp_leavegroup(ip_2_ip4(API_EXPR_REF(msg->msg.jl.netif_addr)),
                                       ip_2_ip4(API_EXPR_REF(msg->msg.jl.multiaddr)));
          }
#endif /* LWIP_IGMP */
        }
#endif /* LWIP_UDP */
#if (LWIP_TCP || LWIP_RAW)
      } else {
        msg->err = ERR_VAL;
#endif /* (LWIP_TCP || LWIP_RAW) */
      }
    } else {
      msg->err = ERR_CONN;
    }
  }
  TCPIP_APIMSG_ACK(msg);
}
#endif /* LWIP_IGMP || (LWIP_IPV6 && LWIP_IPV6_MLD) */

#if LWIP_DNS
/**
 * Callback function that is called when DNS name is resolved
 * (or on timeout). A waiting application thread is waked up by
 * signaling the semaphore.
 */
static void
lwip_netconn_do_dns_found(const char *name, const ip_addr_t *ipaddr, u32_t count, void *arg)
{
  struct dns_api_msg *msg = (struct dns_api_msg *)arg;
  u32_t i;

  /* we trust the internal implementation to be correct :-) */
  LWIP_UNUSED_ARG(name);

  if (ipaddr == NULL) {
    /*
     * If ipaddr = NULL, then count will contain the reason of error
     * that will be eventually set to h_errno or h_errnop
     */
    if (count != 0) {
      /* Incase of known errors */
      API_EXPR_DEREF(msg->err) = (u8_t)(count);
    } else {
      /* Incase of unknown errors */
      API_EXPR_DEREF(msg->err) = EINVAL;
    }
  } else {
    /* address was resolved */
    API_EXPR_DEREF(msg->err) = ERR_OK;
    for (i = 0; i < count; i++) {
      msg->addr[i] = ipaddr[i];
    }
  }
  API_EXPR_DEREF(msg->count) = count;
  /* wake up the application task waiting in netconn_gethostbyname */
  sys_sem_signal(API_EXPR_REF_SEM(msg->sem));
}

/**
 * Execute a DNS query
 * Called from netconn_gethostbyname
 *
 * @param arg the dns_api_msg pointing to the query
 */
void
lwip_netconn_do_gethostbyname(void *arg)
{
  struct dns_api_msg *msg = (struct dns_api_msg *)arg;
  u8_t addrtype =
#if LWIP_IPV4 && LWIP_IPV6
    msg->dns_addrtype;
#else
    LWIP_DNS_ADDRTYPE_DEFAULT;
#endif

  API_EXPR_DEREF(msg->err) = dns_gethostbyname_addrtype(msg->name,
                                                        API_EXPR_REF(msg->addr),
                                                        msg->count, lwip_netconn_do_dns_found, msg, addrtype);
#if LWIP_TCPIP_CORE_LOCKING
  /* For core locking, only block if we need to wait for answer/timeout */
  if (API_EXPR_DEREF(msg->err) == ERR_INPROGRESS) {
    UNLOCK_TCPIP_CORE();
    sys_sem_wait(API_EXPR_REF_SEM(msg->sem));
    LOCK_TCPIP_CORE();
    LWIP_ASSERT("do_gethostbyname still in progress!!", API_EXPR_DEREF(msg->err) != ERR_INPROGRESS);
  }
#else /* LWIP_TCPIP_CORE_LOCKING */
  if (API_EXPR_DEREF(msg->err) != ERR_INPROGRESS) {
    /* on error or immediate success, wake up the application
     * task waiting in netconn_gethostbyname */
    sys_sem_signal(API_EXPR_REF_SEM(msg->sem));
  }
#endif /* LWIP_TCPIP_CORE_LOCKING */
}

#if LWIP_DNS_REVERSE
void
lwip_netconn_do_reverse_dns_found(const char *hostname, u32_t count, void *arg)
{
  struct reverse_dns_api_msg *msg = (struct reverse_dns_api_msg *)arg;

  if (hostname != NULL) {
    size_t namelen = strlen(hostname);
    if (namelen < NI_MAXHOST) {
      if (strncpy_s(msg->hostname, NI_MAXHOST, hostname, namelen) != EOK) {
        API_EXPR_DEREF(msg->err) = EINVAL;
      } else {
        API_EXPR_DEREF(msg->err) = ERR_OK;
      }
    } else {
      msg->hostname = NULL;
      API_EXPR_DEREF(msg->err) = EINVAL;
    }
  } else {
    /* If hostname is NULL, then count will contain the error code (because h_errno is not implemented yet) */
    if (count != 0) {
      /* Incase of known errors */
      API_EXPR_DEREF(msg->err) = (u8_t)(count);
    } else {
      /* Incase of unknown errors */
      API_EXPR_DEREF(msg->err) = EINVAL;
    }
    msg->hostname = NULL;
  }

  sys_sem_signal(API_EXPR_REF_SEM(msg->sem));
}

void
lwip_netconn_do_getnamebyhost(void *arg)
{
  struct reverse_dns_api_msg *msg = (struct reverse_dns_api_msg *)arg;

  API_EXPR_DEREF(msg->err) = reverse_dns_getnamebyhost(API_EXPR_REF(msg->addr), msg->hostname,
                                                       lwip_netconn_do_reverse_dns_found, msg);
#if LWIP_TCPIP_CORE_LOCKING
  /* For core locking, only block if we need to wait for answer/timeout */
  if (API_EXPR_DEREF(msg->err) == ERR_INPROGRESS) {
    UNLOCK_TCPIP_CORE();
    sys_sem_wait(API_EXPR_REF_SEM(msg->sem));
    LOCK_TCPIP_CORE();
    LWIP_ASSERT("do_getnamebyhost still in progress!!", API_EXPR_DEREF(msg->err) != ERR_INPROGRESS);
  }
#else /* LWIP_TCPIP_CORE_LOCKING */
  if (API_EXPR_DEREF(msg->err) != ERR_INPROGRESS) {
    /*
     * on error or immediate success, wake up the application
     * task waiting in netconn_gethostbyname
     */
    sys_sem_signal(API_EXPR_REF_SEM(msg->sem));
  }
#endif /* LWIP_TCPIP_CORE_LOCKING */
}
#endif /* LWIP_DNS_REVERSE */
#endif /* LWIP_DNS */

#ifdef LWIP_GET_CONN_INFO
void
do_getconninfo(void *m)
{
  struct api_msg *msg = (struct api_msg *)m;
  struct tcp_pcb *tcp = NULL;
  struct udp_pcb *udp = NULL;
  ip_addr_t *dst_addr = NULL;
  ip_addr_t *src_addr = NULL;

  const ip4_addr_t *ip_add = NULL;
  struct tcpip_conn *conn_info = NULL;
  struct eth_addr *tdst_mac = NULL;
  struct eth_addr invalid_mac = {{0, 0, 0, 0, 0, 0}};
  s8_t ret;

  conn_info = msg->msg.conn_info;
  tdst_mac = &conn_info->dst_mac;
  if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_UDP) {
    udp = msg->conn->pcb.udp;
    if (udp == NULL) {
      msg->err = ERR_CLSD;
      goto RETURN;
    }

    ip_addr_copy(conn_info->dst_ip, udp->remote_ip);
    ip_addr_copy(conn_info->src_ip, udp->local_ip);
    conn_info->srcport = udp->local_port;
    conn_info->dstport = udp->remote_port;
    conn_info->seqnum = 0; /* Last sent Sequence number */
    conn_info->acknum = 0; /* Last acknowledged number sent */
    conn_info->tcpwin = 0;
    conn_info->last_payload_len = udp->last_payload_len;
    conn_info->tsval = 0;
    conn_info->tsecr = 0;
    conn_info->ipid = ip4_last_ip_id();
    dst_addr = &udp->remote_ip;
    src_addr = &udp->local_ip;
  } else if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP) {
    tcp = msg->conn->pcb.tcp;
    if (tcp == NULL) {
      msg->err = ERR_CLSD;
      goto RETURN;
    }

    if (tcp->state == LISTEN) {
      msg->err = ERR_VAL;
      goto RETURN;
    }

    if ((msg->conn->pcb.tcp->state == CLOSED) ||
        (msg->conn->pcb.tcp->state == TIME_WAIT)) {
      msg->err = ERR_CONN;
      goto RETURN;
    }

    ip_addr_copy(conn_info->dst_ip, tcp->remote_ip);
    ip_addr_copy(conn_info->src_ip, tcp->local_ip);
    conn_info->srcport = tcp->local_port;
    conn_info->dstport = tcp->remote_port;
    if (tcp->state == SYN_SENT) {
      conn_info->seqnum = tcp->lastack; /* seqnum of last ACKED byte */
    } else {
      conn_info->seqnum = (u32_t)(tcp->lastack - 1);
    }
    conn_info->acknum = tcp->rcv_nxt; /* Last acknowledged number sent */
    conn_info->tcpwin = tcp->snd_wnd;
    conn_info->last_payload_len = tcp->last_payload_len;
    conn_info->tsval = 0;
    conn_info->tsecr = 0;
    conn_info->ipid = ip4_last_ip_id();
    dst_addr = &tcp->remote_ip;
    src_addr = &tcp->local_ip;
  } else {
    msg->err = ERR_VAL;
    goto RETURN;
  }

  if (ip_addr_isany(dst_addr)) {
    tdst_mac = &invalid_mac;
    (void)memcpy_s(&conn_info->dst_mac, sizeof(conn_info->dst_mac), tdst_mac, sizeof(conn_info->dst_mac));
    msg->err = ERR_OK;
    goto RETURN;
  }

  if (NETCONNTYPE_ISIPV6(NETCONN_TYPE(msg->conn))) {
    ret = netif_find_dst_ip6addr_mac_addr(src_addr, &dst_addr, &tdst_mac);
  } else {
    ret = netif_find_dst_ipaddr(src_addr, &dst_addr);
    if (ret == 0) {
      ret = etharp_find_addr(NULL, &dst_addr->u_addr.ip4, &tdst_mac, &ip_add);
    }
  }

  if (ret == -1) {
    tdst_mac = &invalid_mac;
  }

  (void)memcpy_s(&conn_info->dst_mac, sizeof(conn_info->dst_mac), tdst_mac, sizeof(conn_info->dst_mac));
  msg->err = ERR_OK;

RETURN:
  TCPIP_APIMSG_ACK(msg);
  return;
}
#endif
#endif /* LWIP_NETCONN */
