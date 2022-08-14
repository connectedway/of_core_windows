/* Copyright (c) 2021 Connected Way, LLC. All rights reserved.
 * Use of this source code is governed by a Creative Commons 
 * Attribution-NoDerivatives 4.0 International license that can be
 * found in the LICENSE file.
 */
#define __OFC_CORE_DLL__

#include <windows.h>

#include "ofc/core.h"
#include "ofc/types.h"
#include "ofc/handle.h"
#include "ofc/thread.h"
#include "ofc/sched.h"
#include "ofc/impl/threadimpl.h"
#include "ofc/libc.h"
#include "ofc/waitset.h"
#include "ofc/event.h"
#include "ofc/heap.h"

/** \{ */

typedef struct
{
  HANDLE thread ;
  OFC_DWORD (*scheduler)(OFC_HANDLE hThread, OFC_VOID *context)  ;
  OFC_VOID *context ;
  OFC_DWORD ret ;
  OFC_BOOL deleteMe ;
  OFC_HANDLE handle ;
  OFC_THREAD_DETACHSTATE detachstate ;
  OFC_HANDLE wait_set ;
  OFC_HANDLE hNotify ;
} WIN32_THREAD ;

static void *ofc_thread_launch(void *arg)
{
  WIN32_THREAD *win32Thread ;

  win32Thread = arg ;

  win32Thread->ret = (win32Thread->scheduler)(win32Thread->handle,
					      win32Thread->context) ;
  if (win32Thread->hNotify != OFC_HANDLE_NULL)
    ofc_event_set (win32Thread->hNotify) ;

  if (win32Thread->detachstate == OFC_THREAD_DETACH)
    {
      CloseHandle (win32Thread->thread) ;
      ofc_handle_destroy (win32Thread->handle) ;
      ofc_free (win32Thread) ;
    }
  return (OFC_NULL) ;
}

OFC_HANDLE ofc_thread_create_impl(OFC_DWORD(scheduler)(OFC_HANDLE hThread,
                                                       OFC_VOID *context),
                                  OFC_CCHAR *thread_name,
                                  OFC_INT thread_instance,
                                  OFC_VOID *context,
                                  OFC_THREAD_DETACHSTATE detachstate,
                                  OFC_HANDLE hNotify)
{
  WIN32_THREAD *win32Thread ;
  OFC_HANDLE ret ;

  ret = OFC_HANDLE_NULL ;
  win32Thread = ofc_malloc (sizeof (WIN32_THREAD)) ;
  if (win32Thread != OFC_NULL)
    {
      win32Thread->wait_set = OFC_HANDLE_NULL ;
      win32Thread->deleteMe = OFC_FALSE ;
      win32Thread->scheduler = scheduler ;
      win32Thread->context = context ;
      win32Thread->hNotify = hNotify ;
      win32Thread->handle = 
	ofc_handle_create (OFC_HANDLE_THREAD, win32Thread) ;
      win32Thread->detachstate = detachstate ;

      win32Thread->thread = 
	CreateThread (NULL, 0, 
		      (LPTHREAD_START_ROUTINE) ofc_thread_launch,
		      win32Thread, 0, NULL) ;

      if (win32Thread->thread == NULL)
	{
	  ofc_handle_destroy (win32Thread->handle) ;
	  ofc_free (win32Thread) ;
	}
      else
	{
	  if (win32Thread->detachstate == OFC_THREAD_DETACH)
	    {
	      CloseHandle (win32Thread->thread) ;
	      win32Thread->thread = NULL ;
	    }
	  ret = win32Thread->handle ;
	}
    }

  return (ret) ;
}

OFC_VOID
ofc_thread_set_waitset_impl(OFC_HANDLE hThread, OFC_HANDLE wait_set)
{
  WIN32_THREAD *win32Thread ;

  win32Thread = ofc_handle_lock (hThread) ;
  if (win32Thread != OFC_NULL)
    {
      win32Thread->wait_set = wait_set ;
      ofc_handle_unlock (hThread) ;
    }
}

OFC_VOID ofc_thread_delete_impl(OFC_HANDLE hThread)
{
  WIN32_THREAD *win32Thread ;

  win32Thread = ofc_handle_lock (hThread) ;
  if (win32Thread != OFC_NULL)
    {
      win32Thread->deleteMe = OFC_TRUE ;
      if (win32Thread->wait_set != OFC_HANDLE_NULL)
	ofc_waitset_wake (win32Thread->wait_set) ;
      ofc_handle_unlock (hThread) ;
    }
}

OFC_VOID ofc_thread_wait_impl(OFC_HANDLE hThread)
{
  WIN32_THREAD *win32Thread ;

  win32Thread = ofc_handle_lock (hThread) ;
  if (win32Thread != OFC_NULL)
    {
      if (win32Thread->detachstate == OFC_THREAD_JOIN)
	{
	  WaitForSingleObject (win32Thread->thread, INFINITE) ;
	  ofc_handle_destroy (win32Thread->handle) ;
	  ofc_free (win32Thread) ;
	}
      ofc_handle_unlock (hThread) ;
    }
}

OFC_BOOL ofc_thread_is_deleting_impl(OFC_HANDLE hThread)
{
  WIN32_THREAD *win32Thread ;
  OFC_BOOL ret ;

  ret = OFC_FALSE ;
  win32Thread = ofc_handle_lock (hThread) ;
  if (win32Thread != OFC_NULL)
    {
      if (win32Thread->deleteMe)
	ret = OFC_TRUE ;
      ofc_handle_unlock (hThread) ;
    }
  return (ret) ;
}

OFC_VOID ofc_sleep_impl(OFC_DWORD milliseconds)
{
  DWORD dwMilliseconds ;

  if (milliseconds == OFC_INFINITE)
    dwMilliseconds = INFINITE ;
  else
    dwMilliseconds = milliseconds ;
  Sleep (dwMilliseconds) ;
}

OFC_DWORD ofc_thread_create_variable_impl(OFC_VOID)
{
  return ((OFC_DWORD) TlsAlloc()) ;
}

OFC_VOID ofc_thread_destroy_variable_impl(OFC_DWORD dkey)
{
  TlsFree(dkey);
}

OFC_DWORD_PTR ofc_thread_get_variable_impl(OFC_DWORD var)
{
  return ((OFC_DWORD_PTR) TlsGetValue ((DWORD) var)) ;
}

OFC_VOID ofc_thread_set_variable_impl(OFC_DWORD var, OFC_DWORD_PTR val)
{
  TlsSetValue ((DWORD) var, (LPVOID) val) ;
}

/*
 * These routines are noops on platforms that support TLS
 */
OFC_CORE_LIB OFC_VOID
ofc_thread_create_local_storage_impl(OFC_VOID)
{
}

OFC_CORE_LIB OFC_VOID
ofc_thread_destroy_local_storage_impl(OFC_VOID)
{
}

OFC_CORE_LIB OFC_VOID
ofc_thread_init_impl(OFC_VOID)
{
}

OFC_CORE_LIB OFC_VOID
ofc_thread_destroy_impl(OFC_VOID)
{
}

OFC_CORE_LIB OFC_VOID
ofc_thread_detach_impl(OFC_HANDLE hThread)
{
  WIN32_THREAD *win32Thread ;

  win32ThreadThread = ofc_handle_lock (hThread) ;
  if (win32Thread != OFC_NULL)
    {
      win32Thread->detachstate = OFC_THREAD_DETACH;
      CloseHandle (win32Thread->thread) ;
      win32Thread->thread = NULL ;
      ofc_handle_unlock(hThread) ;
    }
}

/** \} */
