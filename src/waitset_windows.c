/* Copyright (c) 2021 Connected Way, LLC. All rights reserved.
 * Use of this source code is governed by a Creative Commons 
 * Attribution-NoDerivatives 4.0 International license that can be
 * found in the LICENSE file.
 */
#include <windows.h>

#include "ofc/config.h"
#include "ofc/types.h"
#include "ofc/handle.h"
#include "ofc/waitq.h"
#include "ofc/timer.h"
#include "ofc/libc.h"
#include "ofc/queue.h"
#include "ofc/socket.h"
#include "ofc/event.h"
#include "ofc/waitset.h"
#include "ofc_windows/socket_windows.h"
#include "ofc_windows/event_windows.h"

#include "ofc/heap.h"

#include "ofc/fs.h"
#include "ofc/file.h"

#include "ofc_linux/fs_linux.h"

/**
 * \defgroup waitset_windows Windows Dependent Scheduler Handling
 */

/** \{ */

OFC_VOID ofc_waitset_create_impl(WAIT_SET *pWaitSet)
{
  HANDLE win32WakeHandle ;

  win32WakeHandle = CreateEvent (NULL, FALSE, FALSE, NULL) ;
  pWaitSet->impl = (OFC_VOID *) win32WakeHandle ;
}

OFC_VOID ofc_waitset_destroy_impl(WAIT_SET *pWaitSet)
{
  HANDLE win32WakeHandle ;
  win32WakeHandle = (HANDLE) pWaitSet->impl ;
  if (win32WakeHandle != NULL)
    {
      CloseHandle (win32WakeHandle) ;
      pWaitSet->impl = OFC_NULL ;
    }
}

OFC_VOID ofc_waitset_signal_impl(OFC_HANDLE handle, OFC_HANDLE hEvent)
{
  ofc_waitset_wake_impl (handle) ;
}

OFC_VOID ofc_waitset_wake_impl(OFC_HANDLE handle)
{
  WAIT_SET *pWaitSet ;
  HANDLE win32WakeHandle ;

  pWaitSet = ofc_handle_lock (handle) ;

  if (pWaitSet != OFC_NULL)
    {
      win32WakeHandle = (HANDLE) pWaitSet->impl ;
      if (win32WakeHandle != NULL)
	SetEvent (win32WakeHandle) ;
      ofc_handle_unlock (handle) ;
    }
}

OFC_HANDLE ofc_waitset_wait_impl(OFC_HANDLE handle)
{
  WAIT_SET *pWaitSet ;

  OFC_HANDLE hEventHandle ;
  OFC_HANDLE triggered_event ;
  OFC_HANDLE timer_event ;
  OFC_HANDLE winHandle ;
  OFC_HANDLE fsHandle ;
  HANDLE *win32_handle_list ;
  OFC_HANDLE *ofc_handle_list ;

  DWORD wait_count ;
  DWORD leastWait ;
  DWORD wait_index ;
  OFC_MSTIME wait_time ;
  OFC_FS_TYPE fsType ;
  OFC_HANDLE hEvent ;
  OFC_HANDLE hMsgQ ;

  triggered_event = OFC_HANDLE_NULL ;
  pWaitSet = ofc_handle_lock (handle) ;

  if (pWaitSet != OFC_NULL)
    {
      leastWait = OFC_MAX_SCHED_WAIT ;
      timer_event = OFC_HANDLE_NULL ;

      win32_handle_list = ofc_malloc (sizeof (HANDLE)) ;
      ofc_handle_list = ofc_malloc (sizeof (OFC_HANDLE)) ;
      win32_handle_list[0] = (HANDLE) pWaitSet->impl ;
      ofc_handle_list[0] = OFC_HANDLE_NULL ;
      wait_count = 1 ;

      for (hEventHandle = 
	     (OFC_HANDLE) ofc_queue_first (pWaitSet->hHandleQueue) ;
	   hEventHandle != OFC_HANDLE_NULL && 
	     triggered_event == OFC_HANDLE_NULL ;
	   hEventHandle = 
	     (OFC_HANDLE) ofc_queue_next (pWaitSet->hHandleQueue, 
				      (OFC_VOID *) hEventHandle) ) 
	{
	  switch (ofc_handle_get_type(hEventHandle))
	    {
	    default:
	    case OFC_HANDLE_WAIT_SET:
	    case OFC_HANDLE_SCHED:
	    case OFC_HANDLE_APP:
	    case OFC_HANDLE_THREAD:
	    case OFC_HANDLE_PIPE:
	    case OFC_HANDLE_MAILSLOT:
	    case OFC_HANDLE_FSWIN32_FILE:
	    case OFC_HANDLE_FSDARWIN_FILE:
	    case OFC_HANDLE_QUEUE:
	    case OFC_HANDLE_FSDARWIN_OVERLAPPED:
	      /*
	       * These are not synchronizeable.  Simple ignore
	       */
	      break ;

	    case OFC_HANDLE_WAIT_QUEUE:
	      hEvent = ofc_waitq_get_event_handle (hEventHandle) ;
	      if (!ofc_waitq_empty (hEventHandle))
		{
		  triggered_event = hEventHandle ;
		}
	      else
		{
		  /*
		   * Wait on the associated event
		   */
		  win32_handle_list = 
		    ofc_realloc (win32_handle_list,
				     sizeof (HANDLE) * (wait_count+1)) ;
		  ofc_handle_list = 
		    ofc_realloc (ofc_handle_list,
				     sizeof (OFC_HANDLE) * (wait_count+1)) ;
		  
		  win32_handle_list[wait_count] = 
		    ofc_event_get_win32_handle (hEvent) ;
		  ofc_handle_list[wait_count] = hEventHandle ;
		  wait_count++ ;
		}
	      break ;

	    case OFC_HANDLE_FSWIN32_OVERLAPPED:
	      /*
	       * Wait on the event
	       */
	      win32_handle_list = 
		ofc_realloc (win32_handle_list,
				 sizeof (HANDLE) * (wait_count+1)) ;
	      ofc_handle_list = 
		ofc_realloc (ofc_handle_list,
				 sizeof (OFC_HANDLE) * (wait_count+1)) ;

	      win32_handle_list[wait_count] = 
		OfcFSWindowsGetOverlappedEvent (hEventHandle) ;
	      ofc_handle_list[wait_count] = hEventHandle ;

	      wait_count++ ;
	      break ;

	    case OFC_HANDLE_FSSMB_OVERLAPPED:
	      hMsgQ = OfcFileGetOverlappedWaitQ (hEventHandle) ;
	      hEvent = ofc_waitq_get_event_handle (hMsgQ) ;
	      if (!ofc_waitq_empty (hMsgQ))
		{
		  triggered_event = hEventHandle ;
		}
	      else
		{
		  /*
		   * Wait on the event
		   */
		  win32_handle_list = 
		    ofc_realloc (win32_handle_list,
				  sizeof (HANDLE) * (wait_count+1)) ;
		  ofc_handle_list = 
		    ofc_realloc (ofc_handle_list,
				  sizeof (OFC_HANDLE) * (wait_count+1)) ;

		  win32_handle_list[wait_count] = 
		    ofc_event_get_win32_handle (hEvent) ;
		  ofc_handle_list[wait_count] = hEventHandle ;
		  wait_count++ ;
		}
	      break ;

	    case OFC_HANDLE_FILE:
#if defined(OFC_FS_WIN32)
	      if (fsType == OFC_FST_WIN32)
		{
		  win32_handle_list = 
		    ofc_realloc (win32_handle_list,
				  sizeof (HANDLE) * (wait_count+1)) ;
		  ofc_handle_list = 
		    ofc_realloc (ofc_handle_list,
				  sizeof (OFC_HANDLE) * (wait_count+1)) ;

		  fsHandle = OfcFileGetFSHandle (hEventHandle) ;
		  win32_handle_list[wait_count] = 
		    OfcFSWindowsGetHandle (fsHandle) ;
		  ofc_handle_list[wait_count] = hEventHandle ;
		  wait_count++ ;
		}
#endif
	      break ;

	    case OFC_HANDLE_SOCKET:
	      /*
	       * Wait on event
	       */
	      win32_handle_list = 
		ofc_realloc (win32_handle_list,
			      sizeof (HANDLE) * (wait_count+1)) ;
	      ofc_handle_list = 
		ofc_realloc (ofc_handle_list,
			      sizeof (OFC_HANDLE) * (wait_count+1)) ;
	      
	      winHandle = ofc_socket_get_impl (hEventHandle) ;
	      win32_handle_list[wait_count] = 
		ofc_socket_get_win32_handle (winHandle) ;
	      ofc_handle_list[wait_count] = hEventHandle ;
	      wait_count++ ;
	      break ;

	    case OFC_HANDLE_EVENT:
	      if (ofc_event_test (hEventHandle))
		{
		  triggered_event = hEventHandle ;
		  if (ofc_event_get_type (hEventHandle) == OFC_EVENT_AUTO)
		    ofc_event_reset (hEventHandle) ;
		}
	      else
		{
		  /*
		   * Wait on the event
		   */
		  win32_handle_list = 
		    ofc_realloc (win32_handle_list,
				  sizeof (HANDLE) * (wait_count+1)) ;
		  ofc_handle_list = 
		    ofc_realloc (ofc_handle_list,
				  sizeof (OFC_HANDLE) * (wait_count+1)) ;

		  win32_handle_list[wait_count] = 
		    ofc_event_get_win32_handle (hEventHandle) ;
		  ofc_handle_list[wait_count] = hEventHandle ;

		  wait_count++ ;
		}
	      break ;

	    case OFC_HANDLE_TIMER:
	      wait_time = ofc_timer_get_wait_time (hEventHandle) ;
	      if (wait_time == 0)
		triggered_event = hEventHandle ;
	      else
		{
		  if (wait_time < leastWait)
		    {
		      leastWait = wait_time ;
		      timer_event = hEventHandle ;
		    }
		}
	      break ;

	    }
	}

      if (triggered_event == OFC_HANDLE_NULL)
	{
	  if (wait_count == 0)
	    {
	      Sleep (leastWait) ;
	      if (timer_event != OFC_HANDLE_NULL)
		triggered_event = timer_event ;
	    }
	  else
	    {
	      wait_index = WaitForMultipleObjects (wait_count,
						   win32_handle_list,
						   FALSE,
						   leastWait) ;
	      if (wait_index == WAIT_TIMEOUT && 
		  timer_event != OFC_HANDLE_NULL)
		triggered_event = timer_event ;
	      else if (wait_index >= WAIT_OBJECT_0 &&
		       wait_index < (WAIT_OBJECT_0 + wait_count))
		triggered_event = 
		  ofc_handle_list[wait_index - WAIT_OBJECT_0] ;
	    }
	}
      ofc_free (win32_handle_list) ;
      ofc_free (ofc_handle_list) ;

      ofc_handle_unlock (handle) ;
    }
  return (triggered_event) ;
}

OFC_VOID ofc_waitset_set_assoc_impl(OFC_HANDLE hEvent,
                                    OFC_HANDLE hApp, OFC_HANDLE hSet)
{
}

OFC_VOID ofc_waitset_add_impl(OFC_HANDLE hSet, OFC_HANDLE hApp,
                              OFC_HANDLE hEvent)
{
}

/** \} */
