/**
 * @file
 * Stack-internal timers implementation.
 * This file includes timer callbacks for stack-internal timers as well as
 * functions to set up or stop timers and check for expired timers.
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
 *         Simon Goldschmidt
 *
 */

#include "lwip/opt.h"

#include "lwip/timeouts.h"
#include "lwip/priv/tcp_priv.h"

#include "lwip/def.h"
#include "lwip/memp.h"
#include "lwip/priv/tcpip_priv.h"

#include "lwip/ip4_frag.h"
#include "lwip/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/igmp.h"
#include "lwip/dns.h"
#include "lwip/nd6.h"
#include "lwip/ip6_frag.h"
#include "lwip/mld6.h"
#include "lwip/dhcp6.h"
#include "lwip/sys.h"
#include "lwip/pbuf.h"
#include "netif/lowpan6.h"
#include "lwip/nat64.h"

#if !LWIP_LOWPOWER

#if LWIP_DEBUG_TIMERNAMES
#define HANDLER(x) x, #x
#else /* LWIP_DEBUG_TIMERNAMES */
#define HANDLER(x) x
#endif /* LWIP_DEBUG_TIMERNAMES */

/** This array contains all stack-internal cyclic timers. To get the number of
 * timers, use LWIP_ARRAYSIZE() */
const struct lwip_cyclic_timer lwip_cyclic_timers[] = {
#if LWIP_TCP
  /* The TCP timer is a special case: it does not have to run always and
     is triggered to start from TCP using tcp_timer_needed() */
  {TCP_TMR_INTERVAL, HANDLER(tcp_tmr)},
#endif /* LWIP_TCP */
#if LWIP_IPV4
#if IP_REASSEMBLY
  {IP_TMR_INTERVAL, HANDLER(ip_reass_tmr)},
#endif /* IP_REASSEMBLY */
#if LWIP_ARP
  {ARP_TMR_INTERVAL, HANDLER(etharp_tmr)},
#endif /* LWIP_ARP */
#if LWIP_DHCP
  {DHCP_COARSE_TIMER_MSECS, HANDLER(dhcp_coarse_tmr)},
  {DHCP_FINE_TIMER_MSECS, HANDLER(dhcp_fine_tmr)},
#endif /* LWIP_DHCP */
#if LWIP_AUTOIP
  {AUTOIP_TMR_INTERVAL, HANDLER(autoip_tmr)},
#endif /* LWIP_AUTOIP */
#if LWIP_IGMP
  {IGMP_TMR_INTERVAL, HANDLER(igmp_tmr)},
#endif /* LWIP_IGMP */
#endif /* LWIP_IPV4 */
#if LWIP_DNS
  {DNS_TMR_INTERVAL, HANDLER(dns_tmr)},
#endif /* LWIP_DNS */
#if LWIP_NAT64
  {NAT64_TMR_INTERVAL, HANDLER(nat64_tmr)},
#endif
#if LWIP_IPV6
  {ND6_TMR_INTERVAL, HANDLER(nd6_tmr)},
#if LWIP_IPV6_REASS
  {IP6_REASS_TMR_INTERVAL, HANDLER(ip6_reass_tmr)},
#endif /* LWIP_IPV6_REASS */
#if LWIP_IPV6_MLD
  {MLD6_TMR_INTERVAL, HANDLER(mld6_tmr)},
#endif /* LWIP_IPV6_MLD */
#if LWIP_IPV6_DHCP6
  {DHCP6_TIMER_MSECS, HANDLER(dhcp6_tmr)},
#endif /* LWIP_IPV6_DHCP6 */
#if LWIP_6LOWPAN
  {LOWPAN6_TMR_INTERVAL, HANDLER(lowpan6_tmr)},
#endif
#endif /* LWIP_IPV6 */
};

#if LWIP_TIMERS && !LWIP_TIMERS_CUSTOM

/** The one and only timeout list */
static struct sys_timeo *next_timeout;
static u32_t timeouts_last_time;

#if LWIP_TESTMODE
struct sys_timeo**
lwip_sys_timers_get_next_timout(void)
{
  return &next_timeout;
}
#endif

#if LWIP_TCP
/** global variable that shows if the tcp timer is currently scheduled or not */
static int tcpip_tcp_timer_active;

/**
 * Timer callback function that calls tcp_tmr() and reschedules itself.
 *
 * @param arg unused argument
 */
static void
tcpip_tcp_timer(void *arg)
{
  LWIP_UNUSED_ARG(arg);

  /* call TCP timer handler */
  tcp_tmr();
  /* timer still needed? */
  if (tcp_active_pcbs || tcp_tw_pcbs) {
    /* restart timer */
    sys_timeout(TCP_TMR_INTERVAL, tcpip_tcp_timer, NULL);
  } else {
    /* disable timer */
    tcpip_tcp_timer_active = 0;
  }
}

/**
 * Called from TCP_REG when registering a new PCB:
 * the reason is to have the TCP timer only running when
 * there are active (or time-wait) PCBs.
 */
void
tcp_timer_needed(void)
{
  LWIP_ASSERT_CORE_LOCKED();

  /* timer is off but needed again? */
  if (!tcpip_tcp_timer_active && (tcp_active_pcbs || tcp_tw_pcbs)) {
    /* enable and start timer */
    tcpip_tcp_timer_active = 1;
    sys_timeout(TCP_TMR_INTERVAL, tcpip_tcp_timer, NULL);
  }
}
#endif /* LWIP_TCP */

/**
 * Timer callback function that calls mld6_tmr() and reschedules itself.
 *
 * @param arg unused argument
 */
static void
cyclic_timer(void *arg)
{
  const struct lwip_cyclic_timer *cyclic = (const struct lwip_cyclic_timer *)arg;
#if LWIP_DEBUG_TIMERNAMES
  LWIP_DEBUGF(TIMERS_DEBUG, ("tcpip: %s()\n", cyclic->handler_name));
#endif
  cyclic->handler();
  sys_timeout(cyclic->interval_ms, cyclic_timer, arg);
}

/** Initialize this module */
void
sys_timeouts_init(void)
{
#if LWIP_TCP
  size_t i = 1;
#else
  size_t i = 0;
#endif

  /* tcp_tmr() at index 0 is started on demand */
  for (; i < LWIP_ARRAYSIZE(lwip_cyclic_timers); i++) {
    /* we have to cast via size_t to get rid of const warning
      (this is OK as cyclic_timer() casts back to const* */
    sys_timeout(lwip_cyclic_timers[i].interval_ms, cyclic_timer, LWIP_CONST_CAST(void *, &lwip_cyclic_timers[i]));
  }

  /* Initialise timestamp for sys_check_timeouts */
  timeouts_last_time = sys_now();
}

/**
 * Create a one-shot timer (aka timeout). Timeouts are processed in the
 * following cases:
 * - while waiting for a message using sys_timeouts_mbox_fetch()
 * - by calling sys_check_timeouts() (NO_SYS==1 only)
 *
 * @param msecs time in milliseconds after that the timer should expire
 * @param handler callback function to call when msecs have elapsed
 * @param arg argument to pass to the callback function
 */
#if LWIP_DEBUG_TIMERNAMES
err_t
sys_timeout_debug(u32_t msecs, sys_timeout_handler handler, void *arg, const char *handler_name)
#else /* LWIP_DEBUG_TIMERNAMES */
err_t
sys_timeout_ext(u32_t msecs, sys_timeout_handler handler, void *arg)
#endif /* LWIP_DEBUG_TIMERNAMES */
{
  struct sys_timeo *timeout = NULL;
  struct sys_timeo *t = NULL;
  u32_t now, diff;

  LWIP_ASSERT_CORE_LOCKED();

  timeout = (struct sys_timeo *)memp_malloc(MEMP_SYS_TIMEOUT);
  if (timeout == NULL) {
    LWIP_ASSERT("sys_timeout: timeout != NULL, pool MEMP_SYS_TIMEOUT is empty", timeout != NULL);
    return -1;
  }

  now = sys_now();
  if (next_timeout == NULL) {
    diff = 0;
    timeouts_last_time = now;
  } else {
    diff = now - timeouts_last_time;
  }

  timeout->next = NULL;
  timeout->h = handler;
  timeout->arg = arg;
  timeout->time = msecs + diff;
#if LWIP_DEBUG_TIMERNAMES
  timeout->handler_name = handler_name;
  LWIP_DEBUGF(TIMERS_DEBUG, ("sys_timeout: %p msecs=%"U32_F" handler=%s arg=%p\n",
                             (void *)timeout, msecs, handler_name, (void *)arg));
#endif /* LWIP_DEBUG_TIMERNAMES */

  if (next_timeout == NULL) {
    next_timeout = timeout;
    return 0;
  }

  if (next_timeout->time > timeout->time) {
    next_timeout->time -= timeout->time;
    timeout->next = next_timeout;
    next_timeout = timeout;
  } else {
    for (t = next_timeout; t != NULL; t = t->next) {
      timeout->time -= t->time;
      if (t->next == NULL || t->next->time > timeout->time) {
        if (t->next != NULL) {
          t->next->time -= timeout->time;
        }

        timeout->next = t->next;
        t->next = timeout;
        break;
      }
    }
  }

  return 0;
}

/**
 * Go through timeout list (for this task only) and remove the first matching
 * entry (subsequent entries remain untouched), even though the timeout has not
 * triggered yet.
 *
 * @param handler callback function that would be called by the timeout
 * @param arg callback argument that would be passed to handler
*/
void
sys_untimeout(sys_timeout_handler handler, void *arg)
{
  struct sys_timeo *prev_t = NULL;
  struct sys_timeo *t = NULL;

  LWIP_ASSERT_CORE_LOCKED();

  if (next_timeout == NULL) {
    return;
  }

  for (t = next_timeout, prev_t = NULL; t != NULL; prev_t = t, t = t->next) {
    if ((t->h == handler) && (t->arg == arg)) {
      /* We have a match */
      /* Unlink from previous in list */
      if (prev_t == NULL) {
        next_timeout = t->next;
      } else {
        prev_t->next = t->next;
      }
      /* If not the last one, add time of this one back to next */
      if (t->next != NULL) {
        if ((t->next->time + t->time) >= t->time) {
          t->next->time += t->time;
        } else {
          /*
           * assign max value if overflow occurs, only accuracy of system will be impacted,
           * but vulenrability can be ruled out
           */
          t->next->time = LWIP_MAX_VALUE;
        }
      }
      memp_free(MEMP_SYS_TIMEOUT, t);
      return;
    }
  }
  return;
}

/**
 * @ingroup lwip_nosys
 * Handle timeouts for NO_SYS==1 (i.e. without using
 * tcpip_thread/sys_timeouts_mbox_fetch(). Uses sys_now() to call timeout
 * handler functions when timeouts expire.
 *
 * Must be called periodically from your main loop.
 */
#if !LWIP_TESTMODE && !NO_SYS && !defined __DOXYGEN__
static
#endif /* !NO_SYS */
void
sys_check_timeouts(void)
{
  LWIP_ASSERT_CORE_LOCKED();

  if (next_timeout) {
    struct sys_timeo *tmptimeout = NULL;
    u32_t diff;
    sys_timeout_handler handler = NULL;
    void *arg = NULL;
    u32_t now;
    u8_t had_one;

    now = sys_now();
    /* this cares for wraparounds */
    diff = now - timeouts_last_time;
    do {
      PBUF_CHECK_FREE_OOSEQ();
      had_one = 0;
      tmptimeout = next_timeout;
      if (tmptimeout && (tmptimeout->time <= diff)) {
        /* timeout has expired */
        had_one = 1;
        timeouts_last_time += tmptimeout->time;
        diff -= tmptimeout->time;
        next_timeout = tmptimeout->next;
        handler = tmptimeout->h;
        arg = tmptimeout->arg;
#if LWIP_DEBUG_TIMERNAMES
        if (handler != NULL) {
          LWIP_DEBUGF(TIMERS_DEBUG, ("sct calling h=%s arg=%p\n",
                                     tmptimeout->handler_name, arg));
        }
#endif /* LWIP_DEBUG_TIMERNAMES */
        memp_free(MEMP_SYS_TIMEOUT, tmptimeout);
        if (handler != NULL) {
          handler(arg);
        }
        LWIP_TCPIP_THREAD_ALIVE();
      }
      /* repeat until all expired timers have been called */
    } while (had_one);
  }
}

#if LWIP_API_RICH
/* Set back the timestamp of the last call to sys_check_timeouts()
 * This is necessary if sys_check_timeouts() hasn't been called for a long
 * time (e.g. while saving energy) to prevent all timer functions of that
 * period being called.
 */
void
sys_restart_timeouts(void)
{
  LWIP_ASSERT_CORE_LOCKED();

  timeouts_last_time = sys_now();
}
#endif /* LWIP_API_RICH */

/* Return the time left before the next timeout is due. If no timeouts are
 * enqueued, returns 0xffffffff
 */
#if !LWIP_TESTMODE && !NO_SYS
static inline
#endif /* !NO_SYS */
u32_t
sys_timeouts_sleeptime(void)
{
  u32_t diff;

  LWIP_ASSERT_CORE_LOCKED();

  if (next_timeout == NULL) {
    return 0xffffffff;
  }
  diff = sys_now() - timeouts_last_time;
  if (diff > next_timeout->time) {
    return 0;
  } else {
    return next_timeout->time - diff;
  }
}

#if !NO_SYS

/**
 * Wait (forever) for a message to arrive in an mbox.
 * While waiting, timeouts are processed.
 *
 * @param mbox the mbox to fetch the message from
 * @param msg the place to store the message
 */
void
sys_timeouts_mbox_fetch(sys_mbox_t *mbox, void **msg)
{
  u32_t sleeptime;
  u32_t res;

again:
  if (next_timeout == NULL) {
    UNLOCK_TCPIP_CORE();
    (void)sys_arch_mbox_fetch_ext(mbox, msg, 0, 0);
    LOCK_TCPIP_CORE();
    return;
  }

  sleeptime = sys_timeouts_sleeptime();
  if (sleeptime == 0) {
    sys_check_timeouts();
    /* We try again to fetch a message from the mbox. */
    goto again;
  }

  UNLOCK_TCPIP_CORE();
  res = sys_arch_mbox_fetch_ext(mbox, msg, sleeptime, 0);
  LOCK_TCPIP_CORE();
  if (res == SYS_ARCH_TIMEOUT) {
    /*
     * If a SYS_ARCH_TIMEOUT value is returned, a timeout occurred
     * before a message could be fetched.
     */
    sys_check_timeouts();
    /* We try again to fetch a message from the mbox. */
    goto again;
  }
}

#ifdef DUAL_MBOX
/*
 * Wait (forever) for a message to arrive in an mbox.
 * While waiting, timeouts are processed.
 *
 * @param mbox the mbox to fetch the message from
 * @param msg the place to store the message
 */
void
sys_timeouts_dual_mbox_fetch(sys_dual_mbox_t *mbox, void **msg)
{
  u32_t sleeptime;
  u32_t res;

again:
  if (!next_timeout) {
    UNLOCK_TCPIP_CORE();
    (void)sys_arch_dual_mbox_fetch_ext(mbox, msg, 0, 0);
    LOCK_TCPIP_CORE();
    return;
  }

  sleeptime = sys_timeouts_sleeptime();
  if (sleeptime == 0) {
    sys_check_timeouts();
    /* We try again to fetch a message from the mbox. */
    goto again;
  }

  UNLOCK_TCPIP_CORE();
  res = sys_arch_dual_mbox_fetch_ext(mbox, msg, sleeptime, 0);
  LOCK_TCPIP_CORE();
  if (res == SYS_ARCH_TIMEOUT) {
    /* If a SYS_ARCH_TIMEOUT value is returned, a timeout occurred
       before a message could be fetched. */
    sys_check_timeouts();
    /* We try again to fetch a message from the mbox. */
    goto again;
  }
}
#endif /* DUAL_MBOX */

#endif /* NO_SYS */

#else /* LWIP_TIMERS && !LWIP_TIMERS_CUSTOM */
/* Satisfy the TCP code which calls this function */
void
tcp_timer_needed(void)
{
}
#endif /* LWIP_TIMERS && !LWIP_TIMERS_CUSTOM */
#endif
