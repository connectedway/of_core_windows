/* Copyright (c) 2021 Connected Way, LLC. All rights reserved.
 * Use of this source code is governed by a Creative Commons 
 * Attribution-NoDerivatives 4.0 International license that can be
 * found in the LICENSE file.
 */
#define __OFC_CORE_DLL__

#include <winsock2.h>
#include <ws2tcpip.h>

#include "ofc/types.h"
#include "ofc/handle.h"
#include "ofc/libc.h"
#include "ofc/socket.h"
#include "ofc/impl/socketimpl.h"
#include "ofc/net.h"
#include "ofc/net_internal.h"

#include "ofc/heap.h"
/*
 * PSP_Socket - Create a Network Socket.
 *
 * This routine expects the socket to be managed by the statesocket module.
 * It also expects the socket handle to have been allocated.
 *
 * Accepts:
 *    hSock - The socket handle
 *    family - The network family (must be IP)
 *    socktype - SOCK_TYPE_STREAM or SOCK_TYPE_DGRAM
 *    protocol - Unused, must be STATE_NONE
 *
 * Returns:
 *    Status (STATE_SUCCESS or STATE_FAIL)
 */

typedef struct
{
  SOCKET socket ;
  OFC_FAMILY_TYPE family ;
  HANDLE hEvent ;
  OFC_IPADDR ip ;
} OFC_SOCKET_IMPL ;

OFC_HANDLE ofc_socket_impl_create(OFC_FAMILY_TYPE family,
                                  OFC_SOCKET_TYPE socktype)
{
  OFC_HANDLE hSocket ;
  OFC_SOCKET_IMPL *sock ;

  int stype ;
  int fam ;
  int proto ;
  BOOL on ;
  
  hSocket = OFC_HANDLE_NULL ;
  sock = ofc_malloc (sizeof (OFC_SOCKET_IMPL)) ;

  if (sock != OFC_NULL)
    {
      sock->family = family ;
      if (sock->family == OFC_FAMILY_IP)
	{
	  sock->ip.ip_version = OFC_FAMILY_IP ;
	  sock->ip.u.ipv4.addr = OFC_INADDR_ANY ;
	  fam = AF_INET;
	}
      else
	{
	  sock->ip.ip_version = OFC_FAMILY_IPV6 ;
	  sock->ip.u.ipv6 = ofc_in6addr_any ;
	  fam = AF_INET6 ;
	}
      
      if (socktype == SOCKET_TYPE_STREAM)
	{
	  stype = SOCK_STREAM;
	  proto = IPPROTO_TCP;
	}
      else if (socktype == SOCKET_TYPE_ICMP)
	{
	  stype = SOCK_RAW ;
	  if (sock->ip.ip_version == OFC_FAMILY_IP)
	    proto = IPPROTO_ICMP ;
	  else
	    proto = IPPROTO_ICMPV6 ;
	}
      else
	{
	  stype = SOCK_DGRAM;
	  proto = IPPROTO_UDP;
	}

      sock->socket = socket (fam, stype, proto) ;

      if (sock->socket == INVALID_SOCKET)
	{
	  ofc_free (sock) ;
	}
      else
	{
	  if (socktype == SOCKET_TYPE_DGRAM)
	    {
	      on = TRUE ;
	      setsockopt (sock->socket, SOL_SOCKET, SO_BROADCAST, 
			  (char *) &on, sizeof(on)) ;
	    }
	  sock->hEvent = CreateEvent (NULL, FALSE, FALSE, NULL) ;
	  WSAEventSelect (sock->socket, sock->hEvent,
			  FD_ACCEPT | FD_READ | FD_WRITE | 
			  FD_CONNECT | FD_CLOSE) ;
	  hSocket = ofc_handle_create (OFC_HANDLE_SOCKET_IMPL, sock) ;
	}
    }
  return (hSocket) ;
}

OFC_VOID ofc_socket_impl_destroy(OFC_HANDLE hSocket)
{
  OFC_SOCKET_IMPL *sock ;

  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      CloseHandle (sock->hEvent) ;
      ofc_free(sock) ;
      ofc_handle_destroy (hSocket) ;
      ofc_handle_unlock (hSocket) ;
    }
}

/*
 * PSP_Bind - Bind an IP Address and port to a socket
 *
 * Accepts:
 *    hSock - Handle of socket
 *    ip - 32 bit representation of ip address in host order
 *    port - 16 bit representation of the port in host order
 *
 * Returns:
 *    status (STATE_SUCCESS or STATE_FAIL)
 */
static OFC_VOID make_sockaddr(struct sockaddr **mysockaddr,
                              socklen_t *mysocklen,
                              const OFC_IPADDR *ip,
                              OFC_UINT16 port)
{
  struct sockaddr_in *mysockaddr_in ;
  struct sockaddr_in6 *mysockaddr_in6 ;
  OFC_INT i ;
  
  if (ip->ip_version == OFC_FAMILY_IP)
    {
      mysockaddr_in = ofc_malloc (sizeof (struct sockaddr_in)) ;
      ofc_memset (mysockaddr_in, '\0', sizeof (struct sockaddr_in)) ;

      mysockaddr_in->sin_family = AF_INET ;
      OFC_NET_STON (&mysockaddr_in->sin_port, 0, port) ;
      OFC_NET_LTON (&mysockaddr_in->sin_addr.s_addr, 0,
		     ip->u.ipv4.addr) ;
      *mysockaddr = (struct sockaddr *) mysockaddr_in ;
      *mysocklen = sizeof (struct sockaddr_in) ;
    }
  else
    {
      mysockaddr_in6 = ofc_malloc (sizeof (struct sockaddr_in6)) ;
      ofc_memset (mysockaddr_in6, '\0', sizeof (struct sockaddr_in6)) ;

      mysockaddr_in6->sin6_family = AF_INET6 ;
      mysockaddr_in6->sin6_scope_id = ip->u.ipv6.scope ;

      OFC_NET_STON (&mysockaddr_in6->sin6_port, 0, port) ;
      for (i = 0 ; i < 16 ; i++)
	mysockaddr_in6->sin6_addr.s6_addr[i] = 
	  ip->u.ipv6._s6_addr[i] ;
      *mysockaddr = (struct sockaddr *) mysockaddr_in6 ;
      *mysocklen = sizeof (struct sockaddr_in6) ;
    }
}

OFC_VOID unmake_sockaddr(struct sockaddr *mysockaddr,
                         OFC_IPADDR *ip,
                         OFC_UINT16 *port)
{
  struct sockaddr_in *mysockaddr_in ;
  struct sockaddr_in6 *mysockaddr_in6 ;
  OFC_INT i ;

  if (mysockaddr->sa_family == AF_INET)
    {
      mysockaddr_in = (struct sockaddr_in *) mysockaddr ;
      ip->ip_version = OFC_FAMILY_IP ;
      *port = OFC_NET_NTOS (&mysockaddr_in->sin_port, 0) ;
      ip->u.ipv4.addr = OFC_NET_NTOL (&mysockaddr_in->sin_addr.s_addr, 0) ;
    }
  else
    {
      mysockaddr_in6 = (struct sockaddr_in6 *) mysockaddr ;
      if (ip != OFC_NULL)
	{
	  ip->ip_version = OFC_FAMILY_IPV6 ;
	  for (i = 0 ; i < 16 ; i++)
	    ip->u.ipv6._s6_addr[i] = 
	      mysockaddr_in6->sin6_addr.s6_addr[i] ; 
	}
      if (port != OFC_NULL)
	*port = OFC_NET_NTOS (&mysockaddr_in6->sin6_port, 0) ;
    }
}

OFC_BOOL ofc_socket_impl_bind(OFC_HANDLE hSocket, const OFC_IPADDR *ip,
                              OFC_UINT16 port)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_BOOL ret ;

  int status ;
  struct sockaddr *mysockaddr;
  socklen_t mysocklen ;

  ret = OFC_FALSE ;

  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      make_sockaddr (&mysockaddr, &mysocklen, ip, port) ;

      status = bind(sock->socket, mysockaddr, mysocklen) ;

      if (status != SOCKET_ERROR)
	ret = OFC_TRUE ;

      ofc_free (mysockaddr) ;

      ofc_handle_unlock(hSocket) ;
    }
  return (ret) ;
}

/*
 * PSP_close - Close a socket
 *
 * Accepts:
 *    hSock - Socket handle to close
 *
 * Returns:
 *    status (STATE_SUCCESS or STATE_FAIL)
 */
OFC_BOOL ofc_socket_impl_close(OFC_HANDLE hSocket)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_BOOL ret ;

  int status;

  ret = OFC_FALSE ;
  sock = ofc_handle_lock(hSocket) ;
  if (sock != OFC_NULL)
    {
      status = closesocket (sock->socket);
      if (status != SOCKET_ERROR)
	ret = OFC_TRUE ;
      ofc_handle_unlock(hSocket) ;
    }

  return(ret);
}

/*
 * PSP_Connect - Connect to a remote ip and port
 *
 * Accepts:
 *    hSock - Handle of socket to connect
 *    ip - 32 bit ip in host order
 *    port - 16 bit port in host order
 *
 * Returns:
 *    status (STATE_SUCCESS or STATE_FAIL)
 */
OFC_BOOL ofc_socket_impl_connect(OFC_HANDLE hSocket,
                                 const OFC_IPADDR *ip, OFC_UINT16 port)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_BOOL ret ;

  int status ;
  struct sockaddr *mysockaddr;
  socklen_t mysocklen ;

  ret = OFC_FALSE ;
  sock = ofc_handle_lock(hSocket) ;
  if (sock != OFC_NULL)
    {
      make_sockaddr (&mysockaddr, &mysocklen, ip, port) ;

      status = connect(sock->socket, mysockaddr, mysocklen) ;

      if (((status == SOCKET_ERROR) && 
	   (WSAGetLastError() == WSAEWOULDBLOCK)) ||
	  (status != SOCKET_ERROR))
	ret = OFC_TRUE ;
      ofc_free(mysockaddr) ;

      ofc_handle_unlock(hSocket) ;
    }

  return (ret) ;
}

/*
 * PSP_Listen - Listen for a connection from remote
 *
 * Accepts:
 *    hSock - Handle of Socket to set to listen
 *    backlog - Number of simultaneous open connections to accept
 *
 * Returns:
 *    status (STATE_SUCCESS or STATE_FAIL)
 */
OFC_BOOL ofc_socket_impl_listen(OFC_HANDLE hSocket, OFC_INT backlog)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_BOOL ret ;
  int status ;

  ret = OFC_FALSE ;
  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      status = listen(sock->socket, (int) backlog) ;
      if (status != SOCKET_ERROR)
	ret = OFC_TRUE ;

      ofc_handle_unlock (hSocket) ;
    }
  return(ret);
}

/*
 * PSP_Accept - Accept a connection on socket
 * 
 * Accepts:
 *    sock - Pointer to socket structure of listening socket
 *    newsock - Pointer to new socket structure
 *
 * Returns:
 *    status (STATE_SUCCESS or STATE_FAIL)
 */
OFC_HANDLE ofc_socket_impl_accept(OFC_HANDLE hSocket,
                                  OFC_IPADDR *ip, OFC_UINT16 *port)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_SOCKET_IMPL *newsock ;
  OFC_HANDLE hNewSock ;

  socklen_t addrlen;
  struct sockaddr *mysockaddr;	

  hNewSock = OFC_HANDLE_NULL ;
  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      newsock = ofc_malloc (sizeof (OFC_SOCKET_IMPL)) ;

      addrlen = (OFC_MAX (sizeof (struct sockaddr_in6),
			  sizeof (struct sockaddr_in))) ;

      mysockaddr = ofc_malloc (addrlen) ;

      newsock->socket = accept(sock->socket, mysockaddr, &addrlen);

      if (newsock->socket != INVALID_SOCKET)
	{
	  newsock->hEvent = CreateEvent (NULL, FALSE, FALSE, NULL) ;
	  WSAEventSelect (newsock->socket, newsock->hEvent,
			  FD_ACCEPT | FD_READ | FD_WRITE | 
			  FD_CONNECT | FD_CLOSE) ;

	  unmake_sockaddr (mysockaddr, ip, port) ;

	  hNewSock = ofc_handle_create (OFC_HANDLE_SOCKET_IMPL, newsock) ;
	}
      else
	{
	  ofc_free (newsock) ;
	}
      ofc_free(mysockaddr) ;

      ofc_handle_unlock(hSocket) ;
    }
  return (hNewSock) ;
}

/*
 * PSP_Reuseaddr - Clean up socket so we use it again
 * 
 * Accepts:
 *    hSock - Socket to set the port reuseable on
 *    onoff - TRUE for on, FALSE for off
 */
OFC_BOOL ofc_socket_impl_reuse_addr(OFC_HANDLE hSocket, OFC_BOOL onoff)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_BOOL ret ;

  int status ;
  BOOL on;

  ret = OFC_FALSE ;
  sock = ofc_handle_lock(hSocket) ;
  if (sock != OFC_NULL)
    {
      on = (onoff == OFC_TRUE ? TRUE : FALSE) ;
      status = setsockopt(sock->socket, SOL_SOCKET, SO_REUSEADDR, 
			  (const char *) &on, sizeof(on));
      if (status != SOCKET_ERROR)
	ret = OFC_TRUE ;
      ofc_handle_unlock (hSocket) ;
    }
  return (ret) ;
}

/*
 * PSP_Is_Connected - Test if the socket is connected or not
 *
 * Accepts:
 *     hSock - Socket to test for
 *
 * Returns:
 *   True if connected, false otherwise
 */
OFC_BOOL ofc_socket_impl_connected(OFC_HANDLE hSocket)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_BOOL ret ;

  int status ;
  int *namelenp;
  int len ;
  struct sockaddr sa;

  ret = OFC_FALSE ;
  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      len = sizeof(sa);
      namelenp = &len ;
      status = getpeername(sock->socket, &sa, namelenp);
      if (status != SOCKET_ERROR)
	ret = OFC_TRUE ;
      ofc_handle_unlock (hSocket) ;
    }

  return (ret) ;
}

/*
 * PSP_NoBlock - Set socket block mode
 *
 * Accepts:
 *    hSock - Socket to set the function on 
 *    onoff - Whether setting blocking on or off
 *    option - The option for the command
 *
 * Returns:
 *    status - Success of failure
 */
OFC_BOOL ofc_socket_impl_no_block(OFC_HANDLE hSocket, OFC_BOOL onoff)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_BOOL ret ;

  int status ;
  u_long cmd;

  ret = OFC_FALSE ;
  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      cmd = (onoff == OFC_TRUE ? 1 : 0) ;
      status = ioctlsocket (sock->socket, FIONBIO, &cmd) ;
      if (status != SOCKET_ERROR)
	ret = OFC_TRUE ;
      ofc_handle_unlock (hSocket) ;
    }
  return (ret) ;
}

/*
 * PSP_Send - Send data on a socket
 *
 * Accepts:
 *    hSock - Socket to send data on
 *    buf - Pointer to buffer to write
 *    len - Number of bytes to write
 *
 * Returns:
 *    Number of bytes written
 */
OFC_SIZET ofc_socket_impl_send(OFC_HANDLE hSocket, const OFC_VOID *buf,
                               OFC_SIZET len)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_SIZET ret ;

  int status ;

  ret = -1 ;
  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      status = send(sock->socket, (const char *) buf, (int) len, 0) ;
      if ((status == SOCKET_ERROR) && (WSAGetLastError() == WSAEWOULDBLOCK))
	ret = 0 ;
      else if (status != SOCKET_ERROR)
	ret = status ;

      ofc_handle_unlock (hSocket) ;
    }
  return (ret) ;
}

/*
 * PSP_Send_To - Send Data on a datagram socket
 *
 * Accepts:
 *    hSock - socket to write data on
 *    buf - buffer to write
 *    len - number of bytes to write
 *    port - port number to write to (host order)
 *    ip - ip to write to (host order)
 *
 * Returns:
 *    Number of bytes written
 */
OFC_SIZET ofc_socket_impl_sendto(OFC_HANDLE hSocket, const OFC_VOID *buf,
                                 OFC_SIZET len,
                                 const OFC_IPADDR *ip,
                                 OFC_UINT16 port)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_SIZET ret ;
  int error ;

  int status ;
  struct sockaddr *mysockaddr;
  socklen_t mysocklen ;

  ret = -1 ;
  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      make_sockaddr (&mysockaddr, &mysocklen, ip, port) ;

      status = sendto(sock->socket, (const char * ) buf, (int) len, 0,
		      mysockaddr, mysocklen) ;

      error = WSAGetLastError () ;
      

      if ((status == SOCKET_ERROR) && (WSAGetLastError() == WSAEWOULDBLOCK))
	ret = 0 ;
      else if (status != SOCKET_ERROR)
	ret = status ;

      ofc_free (mysockaddr) ;
      ofc_handle_unlock (hSocket) ;
    }

  return (ret) ;
}

/*
 * PSP_Recv - Receive bytes from a socket
 *
 * Accepts:
 *    hSock - Socket to read from
 *    buf - pointer to buffer to store data
 *    len - size of buffer
 *
 * Returns:
 *    number of bytes read
 */
OFC_SIZET ofc_socket_impl_recv(OFC_HANDLE hSocket,
                               OFC_VOID *buf,
                               OFC_SIZET len)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_SIZET ret ;

  int status ;

  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      status = recv (sock->socket, (char *) buf, (int) len, 0);

      if ((status == SOCKET_ERROR) && (WSAGetLastError() == WSAEWOULDBLOCK))
	ret = 0 ;
      else if (status != SOCKET_ERROR)
	ret = status ;
      else
	ret = WSAGetLastError() ;

      ofc_handle_unlock (hSocket) ;
    }

  return(ret);
}

OFC_BOOL ofc_socket_impl_peek (OFC_HANDLE hSocket)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_BOOL ret ;
  OFC_CHAR buf[100] ;

  int status ;

  ret = OFC_FALSE ;
  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      status = recv (sock->socket, (char *) buf, 100, MSG_PEEK);

      if (status > 0)
	ret = OFC_TRUE ;

      ofc_handle_unlock (hSocket) ;
    }

  return(ret);
}

/*
 * PSP_Recv_From - Receive bytes from socket, return ip address
 *
 * Accepts:
 *    hSock - socket to read from
 *    buf - where to read data to
 *    len - size of buffer
 *    port - pointer to where to store the port (stored in host order)
 *    ip - Pointer to where to store the ip (stored in host order)
 *
 * Returns:
 *    number of bytes read
 */
OFC_SIZET ofc_socket_impl_recv_from(OFC_HANDLE hSocket,
                                    OFC_VOID *buf,
                                    OFC_SIZET len,
                                    OFC_IPADDR *ip,
                                    OFC_UINT16 *port)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_SIZET ret ;

  struct sockaddr *mysockaddr;
  socklen_t mysize ;
  int status ;

  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      mysize = (OFC_MAX (sizeof (struct sockaddr_in6),
			 sizeof (struct sockaddr_in))) ;

      mysockaddr = ofc_malloc(mysize) ;
      ofc_memset (mysockaddr, '\0', mysize) ;

      status = recvfrom(sock->socket, (char *) buf, (int) len, 0,
			mysockaddr, &mysize);

      if ((status == SOCKET_ERROR) && (WSAGetLastError() == WSAEWOULDBLOCK))
	ret = 0 ;
      else if (status != SOCKET_ERROR)
	{
	  unmake_sockaddr (mysockaddr, ip, port) ;
	  ret = status ;
	}
      else
	{
	  ret = -1 ;
	}

      ofc_free (mysockaddr) ;
      ofc_handle_unlock (hSocket) ;
    }
  return(ret) ;
}

HANDLE ofc_socket_get_win32_handle (OFC_HANDLE hSocket) 
{
  OFC_SOCKET_IMPL *pSocket ;
  HANDLE handle ;

  handle = INVALID_HANDLE_VALUE ;
  pSocket = ofc_handle_lock (hSocket) ;
  if (pSocket != OFC_NULL)
    {
      handle = pSocket->hEvent ;
      ofc_handle_unlock (hSocket) ;
    }
  return (handle) ;
}

OFC_SOCKET_EVENT_TYPE ofc_socket_impl_test(OFC_HANDLE hSocket)
{
  OFC_SOCKET_IMPL *pSocket ;
  WSANETWORKEVENTS NetworkEvents;
  OFC_SOCKET_EVENT_TYPE TestEvents ;

  TestEvents = 0 ;

  pSocket = ofc_handle_lock (hSocket) ;
  if (pSocket != OFC_NULL)
    {
      WSAEnumNetworkEvents(pSocket->socket,
			   pSocket->hEvent,
			   &NetworkEvents) ;

      if (NetworkEvents.lNetworkEvents & FD_CLOSE)
	TestEvents |= OFC_SOCKET_EVENT_CLOSE ;
      if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
	TestEvents |= OFC_SOCKET_EVENT_ACCEPT ;
      if (NetworkEvents.lNetworkEvents & FD_ADDRESS_LIST_CHANGE)
	TestEvents |= OFC_SOCKET_EVENT_ADDRESSCHANGE ;
      if (NetworkEvents.lNetworkEvents & FD_QOS)
	TestEvents |= OFC_SOCKET_EVENT_QOS ;
      if (NetworkEvents.lNetworkEvents & FD_OOB)
	TestEvents |= OFC_SOCKET_EVENT_QOB ;
      if (NetworkEvents.lNetworkEvents & FD_READ)
	TestEvents |= OFC_SOCKET_EVENT_READ ;
      if (NetworkEvents.lNetworkEvents & FD_WRITE)
	TestEvents |= OFC_SOCKET_EVENT_WRITE ;

      ofc_handle_unlock (hSocket) ;
    }
  
  return (TestEvents) ;
}

OFC_BOOL ofc_socket_impl_enable(OFC_HANDLE hSocket,
                                OFC_SOCKET_EVENT_TYPE type)
{
  OFC_SOCKET_IMPL *pSocket ;
  long NetworkEvents;
  OFC_BOOL ret ;

  ret = OFC_FALSE ;
  pSocket = ofc_handle_lock (hSocket) ;
  if (pSocket != OFC_NULL)
    {
      NetworkEvents = 0 ;
      ret = OFC_TRUE ;
      if (type & OFC_SOCKET_EVENT_CLOSE)
	NetworkEvents |= FD_CLOSE ;
      if (type & OFC_SOCKET_EVENT_ACCEPT)
	NetworkEvents |= FD_ACCEPT ;
      if (type & OFC_SOCKET_EVENT_ADDRESSCHANGE)
	NetworkEvents |= FD_ADDRESS_LIST_CHANGE ;
      if (type & OFC_SOCKET_EVENT_QOS)
	NetworkEvents |= FD_QOS ;
      if (type & OFC_SOCKET_EVENT_QOB)
	NetworkEvents |= FD_OOB ;
      if (type & OFC_SOCKET_EVENT_READ)
	NetworkEvents |= FD_READ ;
      if (type & OFC_SOCKET_EVENT_WRITE)
	NetworkEvents |= FD_WRITE ;

      WSAEventSelect (pSocket->socket, pSocket->hEvent, NetworkEvents) ;
      ofc_handle_unlock (hSocket) ;
    }
  
  return (ret) ;
}

OFC_VOID ofc_socket_impl_set_send_size(OFC_HANDLE hSocket, OFC_INT size)
{
  OFC_SOCKET_IMPL *sock ;

  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      setsockopt(sock->socket, SOL_SOCKET, SO_SNDBUF,
		 (const char *) &size, sizeof(size));
      ofc_handle_unlock (hSocket) ;
    }
}
  
OFC_VOID ofc_socket_impl_set_recv_size(OFC_HANDLE hSocket, OFC_INT size)
{
  OFC_SOCKET_IMPL *sock ;

  sock = ofc_handle_lock (hSocket) ;
  if (sock != OFC_NULL)
    {
      setsockopt(sock->socket, SOL_SOCKET, SO_RCVBUF,
		 (const char *) &size, sizeof(size));
      ofc_handle_unlock (hSocket) ;
    }
}
  
OFC_BOOL ofc_socket_impl_get_addresses(OFC_HANDLE hSock,
                                       OFC_SOCKADDR *local,
                                       OFC_SOCKADDR *remote)
{
  OFC_SOCKET_IMPL *sock ;
  OFC_BOOL ret ;
  int win32_status ;
  struct sockaddr *local_sockaddr;
  struct sockaddr *remote_sockaddr;
  socklen_t local_sockaddr_size;
  socklen_t remote_sockaddr_size;

  ret = OFC_FALSE ;
  sock = ofc_handle_lock (hSock) ;
  if (sock != OFC_NULL)
    {
      local_sockaddr_size = (OFC_MAX (sizeof (struct sockaddr_in6),
				      sizeof (struct sockaddr_in))) ;
      local_sockaddr = ofc_malloc (local_sockaddr_size) ;

      win32_status = getsockname (sock->socket, local_sockaddr,
				  &local_sockaddr_size) ;
      if (win32_status == 0)
	{
	  if (local_sockaddr->sa_family == AF_INET)
	    local->sin_family = OFC_FAMILY_IP ;
	  else
	    local->sin_family = OFC_FAMILY_IPV6 ;
	  unmake_sockaddr (local_sockaddr, 
			   &local->sin_addr, &local->sin_port) ;

	  remote_sockaddr_size = (OFC_MAX (sizeof (struct sockaddr_in6),
					   sizeof (struct sockaddr_in))) ;
	  remote_sockaddr = ofc_malloc (remote_sockaddr_size) ;

	  win32_status = getpeername (sock->socket, 
				      remote_sockaddr,
				      &remote_sockaddr_size) ;
	  if (win32_status == 0)
	    {
	      if (remote_sockaddr->sa_family == AF_INET)
		remote->sin_family = OFC_FAMILY_IP ;
	      else
		remote->sin_family = OFC_FAMILY_IPV6 ;
	      unmake_sockaddr (remote_sockaddr, &remote->sin_addr, 
			       &remote->sin_port) ;
	      ret = OFC_TRUE ;
	    }
	  ofc_free (remote_sockaddr) ;
	}
      ofc_free (local_sockaddr) ;
      ofc_handle_unlock (hSock) ;
    }
  return (ret) ;
}

/** \} */
