/* Copyright (c) 2021 Connected Way, LLC. All rights reserved.
 * Use of this source code is governed by a Creative Commons 
 * Attribution-NoDerivatives 4.0 International license that can be
 * found in the LICENSE file.
 */
#define __OFC_CORE_DLL

#include <winsock2.h>
#include <ws2tcpip.h>
#if defined(_WINCE_)
#include <Iphlpapi.h>
#endif

#include "ofc/core.h"
#include "ofc/types.h"
#include "ofc/config.h"
#include "ofc/libc.h"
#include "ofc/heap.h"
#include "ofc/net.h"
#include "ofc/net_internal.h"
#include "ofc_windows/config.h"

/**
 * \defgroup net_windows Windows Network Implementation
 */

/** \{ */

OFC_VOID ofc_net_init_impl(OFC_VOID) {
  WORD wVersionRequested ;
  WSADATA wsaData ;

  wVersionRequested = MAKEWORD (2, 0) ;
  WSAStartup (wVersionRequested, &wsaData) ;
}

OFC_VOID ofc_net_register_config_impl(OFC_HANDLE hEvent) {
}

OFC_VOID ofc_net_unregister_config_impl(OFC_HANDLE hEvent) {
}

static OFC_BOOL match_families(INTERFACE_INFO *ifaddrp) {
  OFC_BOOL ret ;

  ret = OFC_FALSE ;
#if defined(OFC_DISCOVER_IPV4)
  if (ifaddrp->iiAddress.Address.sa_family == AF_INET)
    ret = OFC_TRUE ;
#endif
#if defined(OFC_DISCOVER_IPV6)
  if (ifaddrp->iiAddress.Address.sa_family == AF_INET6)
    {
      ret = OFC_TRUE ;
    }
#endif      
  return (ret) ;

}

OFC_INT ofc_net_interface_count_impl(OFC_VOID) {
  OFC_INT count ;
  OFC_INT max_count ;
  SOCKET dgramSocket ;
  INTERFACE_INFO localAddr[OFC_MAX_NETWORK_INTERFACES];  
  DWORD bytesReturned ;
  OFC_INT i ;

#if defined(OFC_DISCOVER_IPV6)
  ADDRINFOA *res ;
  ADDRINFOA *p ;
  ADDRINFOA hints ;
  int ret ;
#endif

  max_count = 0 ;
  /*
   * Fist get IPv4 info
   */
  dgramSocket = socket(AF_INET, SOCK_DGRAM, 0);

  WSAIoctl(dgramSocket,
	   SIO_GET_INTERFACE_LIST, 
	   NULL, 
	   0, 
	   &localAddr,
	   sizeof(localAddr),
	   &bytesReturned,
	   NULL,
	   NULL);

  closesocket(dgramSocket);

  count = bytesReturned / sizeof(INTERFACE_INFO) ;

  for (i = 0 ; i < count ; i++)
    {
      if ((localAddr[i].iiFlags & IFF_UP) &&
	  match_families (&localAddr[i]))
	  max_count++ ;
    }

#if defined(OFC_DISCOVER_IPV6)
  /*
   * Now the IPv6.  We do this different
   */
  ofc_memset ((OFC_VOID *)&hints, 0, sizeof (hints)) ;

  hints.ai_family = AF_INET6 ;
  hints.ai_socktype = 0 ;
  hints.ai_flags = AI_ADDRCONFIG ;
 
  res = NULL ;
  ret = GetAddrInfoA ("", NULL, &hints, &res) ;

  if (ret == 0)
    {
      for (p = res ; p != NULL ; p = p->ai_next)
	{
	  if (p->ai_family == AF_INET6)
	    {
	      max_count++ ;
	    }
	}
      freeaddrinfo (res) ;
    }
#endif
  
  return (max_count) ;
}

OFC_VOID ofc_net_interface_addr_impl(OFC_INT index,
                                     OFC_IPADDR *pinaddr,
                                     OFC_IPADDR *pbcast,
                                     OFC_IPADDR *pmask) {
  OFC_INT count ;
  OFC_INT max_count ;
  OFC_INT i ;
  SOCKET dgramSocket ;
  INTERFACE_INFO localAddr[OFC_MAX_NETWORK_INTERFACES];  
  DWORD bytesReturned ;
  SOCKADDR_IN *pAddrInet ;
  SOCKADDR_IN *pBCastInet ;

#if defined(OFC_DISCOVER_IPV6)
  SOCKADDR_IN6 *pAddrInet6 ;
  ADDRINFOA *res ;
  ADDRINFOA *p ;
  ADDRINFOA hints ;
  int ret ;
#endif

  if (pinaddr != OFC_NULL)
    {
      pinaddr->ip_version = OFC_FAMILY_IP ;
      pinaddr->u.ipv4.addr = OFC_INADDR_NONE ;
    }
  if (pbcast != OFC_NULL)
    {
      pbcast->ip_version = OFC_FAMILY_IP ;
      pbcast->u.ipv4.addr = OFC_INADDR_NONE ;
    }
  if (pmask != OFC_NULL)
    {
      pmask->ip_version = OFC_FAMILY_IP ;
      pmask->u.ipv4.addr = OFC_INADDR_NONE ;
    }
  /*
   * See if it's an IPv4 
   */
  dgramSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0) ;

  WSAIoctl(dgramSocket,
	   SIO_GET_INTERFACE_LIST, 
	   NULL, 
	   0, 
	   &localAddr,
	   sizeof(localAddr),
	   &bytesReturned,
	   NULL,
	   NULL);

  closesocket(dgramSocket);

  count = bytesReturned / sizeof(INTERFACE_INFO) ;

  max_count = 0 ;

  for (i = 0 ; i < count && max_count < index ; i++)
    {
      if ((localAddr[i].iiFlags & IFF_UP) &&
	  match_families(&localAddr[i]))
	max_count++ ;
    }

  if ((max_count < count) && (max_count == index))
    {
      /*
       * Found it as an IPv4 addess
       */
      if (localAddr[index].iiAddress.Address.sa_family == AF_INET)
	{
	  if (pinaddr != OFC_NULL)
	    {
	      pAddrInet = (SOCKADDR_IN *) &localAddr[index].iiAddress ;
	      pinaddr->ip_version = OFC_FAMILY_IP ;
	      pinaddr->u.ipv4.addr = 
		OFC_NET_NTOL (&pAddrInet->sin_addr.s_addr, 0) ;
	    }
	  if (pbcast != OFC_NULL)
	    {
	      pBCastInet =
		(SOCKADDR_IN *) &localAddr[index].iiBroadcastAddress ;
	      pAddrInet = (SOCKADDR_IN *) &localAddr[index].iiNetmask ;
	      pBCastInet->sin_addr.s_addr &= ~pAddrInet->sin_addr.s_addr ;
	      pAddrInet = (SOCKADDR_IN *) &localAddr[index].iiAddress ;
	      pBCastInet->sin_addr.s_addr |= pAddrInet->sin_addr.s_addr ;
	      pbcast->ip_version = OFC_FAMILY_IP ;
	      pbcast->u.ipv4.addr = 
		OFC_NET_NTOL (&pBCastInet->sin_addr.s_addr, 0) ;
	    }
	  if (pmask != OFC_NULL)
	    {
	      pAddrInet = (SOCKADDR_IN *) &localAddr[index].iiNetmask  ;
	      pmask->ip_version = OFC_FAMILY_IP ;
	      pmask->u.ipv4.addr =
		OFCE_NET_NTOL (&pAddrInet->sin_addr.s_addr, 0) ;
	    }
	}
    }

#if defined(OFC_DISCOVER_IPV6)
  else if (max_count <= index)
    {
      /* Go through IPv6 addresses */
      ofc_memset ((OFC_VOID *)&hints, 0, sizeof (hints)) ;

      hints.ai_family = AF_INET6 ;
      hints.ai_socktype = 0 ;
      hints.ai_flags = AI_ADDRCONFIG ;

      res = NULL ;
      ret = GetAddrInfoA ("", NULL, &hints, &res) ;

      if (ret == 0)
	{
	  for (p = res ; p != NULL && max_count < index; )
	    {
	      max_count++ ;
	      p = p->ai_next ;
	    }

	  if (max_count == index)
	    {
	      OFC_INT scope ;
	  
	      if (pinaddr != OFC_NULL)
		{
		  pAddrInet6 = (struct sockaddr_in6 *) p->ai_addr ;
		  scope = pAddrInet6->sin6_scope_id ;
		  pinaddr->ip_version = OFC_FAMILY_IPV6 ;
		  for (i = 0 ; i < 16 ; i++)
		    pinaddr->u.ipv6._s6_addr[i] =
		      pAddrInet6->sin6_addr.s6_addr[i] ;
		  pinaddr->u.ipv6.scope = scope ;
		}
	      if (pbcast != OFC_NULL)
		{
		  pbcast->ip_version = OFC_FAMILY_IPV6 ;
		  pbcast->u.ipv6 = ofc_in6addr_bcast ;
		  pbcast->u.ipv6.scope = scope ;
		}
	      if (pmask != OFC_NULL)
		{
		  pmask->ip_version = OFC_FAMILY_IPV6 ;
		  pmask->u.ipv6 = ofc_in6addr_none ;
		  pmask->u.ipv6.scope = scope ;
		}
	    }

	  freeaddrinfo (res) ;
	}
    }
#endif
}

OFC_CORE_LIB OFC_VOID
ofc_net_interface_wins_impl(OFC_INT index, OFC_INT *num_wins,
                            OFC_IPADDR **winslist) {
  /*
   * This is not provided by the platform
   */
  if (num_wins != OFC_NULL)
    *num_wins = 0 ;
  if (winslist != OFC_NULL)
    *winslist = OFC_NULL ;
}

OFC_VOID ofc_net_resolve_dns_name_impl(OFC_LPCSTR name,
                                       OFC_UINT16 *num_addrs,
                                       OFC_IPADDR *ip) {
  ADDRINFOA *res ;
  ADDRINFOA *p ;
  ADDRINFOA hints ;

  int ret ;

  OFC_INT i ;
  OFC_INT j ;
  OFC_IPADDR temp ;

  ofc_memset ((OFC_VOID *)&hints, 0, sizeof (hints)) ;

#if defined(OFC_PARAM_DISCOVER_IPV6)
#if defined(OFC_PARAM_DISCOVER_IPV4)
  hints.ai_family = AF_UNSPEC ;
#else
  hints.ai_family = AF_INET6 ;
#endif
#else
#if defined(OFC_PARAM_DISCOVER_IPV4)
  hints.ai_family = AF_INET ;
#else
#error "Neither IPv4 nor IPv6 Configured"
#endif
#endif
  hints.ai_socktype = 0 ;
  hints.ai_flags = AI_ADDRCONFIG ;

  if (ofc_pton (name, &temp) != 0)
    hints.ai_flags |= AI_NUMERICHOST ;

  res = NULL ;
  ret = GetAddrInfoA (name, NULL, &hints, &res) ;

  if (ret != 0)
    *num_addrs = 0 ;
  else
    {
      for (i = 0, p = res ; p != NULL && i < *num_addrs ; i++, p = p->ai_next)
	{
	  if (p->ai_family == AF_INET)
	    {
	      struct sockaddr_in *sa ;
	      sa = (struct sockaddr_in *) p->ai_addr ;

	      ip[i].ip_version = OFC_FAMILY_IP ;
	      ip[i].u.ipv4.addr = OFC_NET_NTOL (&sa->sin_addr.s_addr, 0) ;
	    }
	  else if (p->ai_family == AF_INET6)
	    {
	      struct sockaddr_in6 *sa6 ;
	      sa6 = (struct sockaddr_in6 *) p->ai_addr ;

	      ip[i].ip_version = OFC_FAMILY_IPV6 ;
	      for (j = 0 ; j < 16 ; j++)
		{
		  ip[i].u.ipv6._s6_addr[j] = 
		    sa6->sin6_addr.s6_addr[j] ; 
		}
	      ip[i].u.ipv6.scope = sa6->sin6_scope_id;
	    }
	}
      freeaddrinfo (res) ;
      *num_addrs = i ;
    }
}

/** \} */
