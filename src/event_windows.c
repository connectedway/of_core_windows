/* Copyright (c) 2021 Connected Way, LLC. All rights reserved.
 * Use of this source code is governed by a Creative Commons 
 * Attribution-NoDerivatives 4.0 International license that can be
 * found in the LICENSE file.
 */
#include <windows.h>

#include "ofc/types.h"
#include "ofc/handle.h"
#include "ofc/event.h"

#include "ofc/heap.h"
#include "ofc/impl/eventimpl.h"
#include "ofc/impl/waitsetimpl.h"

typedef struct
{
  HANDLE winevent ;
  OFC_EVENT_TYPE eventType ;
} WIN32_EVENT ;

OFC_HANDLE ofc_event_create_impl(OFC_EVENT_TYPE eventType) {
  WIN32_EVENT *win32_event ;
  OFC_HANDLE hWin32Event ;
  BOOL manual ;

  hWin32Event = OFC_HANDLE_NULL ;
  win32_event = ofc_malloc (sizeof (WIN32_EVENT)) ;
  if (win32_event != OFC_NULL)
    {
      win32_event->eventType = eventType ;
      manual = FALSE ;
      if (win32_event->eventType == OFC_EVENT_MANUAL)
	manual = TRUE ;
      win32_event->winevent = CreateEvent (NULL, manual, FALSE, NULL) ;
      if (win32_event->winevent == NULL)
	ofc_free (win32_event) ;
      else
	hWin32Event = ofc_handle_create (OFC_HANDLE_EVENT, win32_event) ;
    }
  return (hWin32Event) ;
}

OFC_VOID ofc_event_set_impl(OFC_HANDLE hEvent)
{
  WIN32_EVENT *win32Event ;
  BOOL ret ;

  win32Event = ofc_handle_lock (hEvent) ;
  if (win32Event != OFC_NULL)
    {
      ret = SetEvent (win32Event->winevent) ;
      ofc_handle_unlock (hEvent) ;
    }
}

OFC_VOID ofc_event_reset_impl(OFC_HANDLE hEvent)
{
  WIN32_EVENT *win32Event ;
  BOOL ret ;

  win32Event = ofc_handle_lock (hEvent) ;
  if (win32Event != OFC_NULL)
    {
      ret = ResetEvent (win32Event->winevent) ;
      ofc_handle_unlock (hEvent) ;
    }
}

OFC_EVENT_TYPE ofc_event_get_type_impl(OFC_HANDLE hEvent) 
{
  WIN32_EVENT *win32Event ;
  OFC_EVENT_TYPE eventType ;

  eventType = OFC_EVENT_AUTO ;
  win32Event = ofc_handle_lock (hEvent) ;
  if (win32Event != OFC_NULL)
    {
      eventType = win32Event->eventType ;
      ofc_handle_unlock(hEvent) ;
    }
  return (eventType) ;
}

OFC_VOID ofc_event_destroy_impl(OFC_HANDLE hEvent) 
{
  WIN32_EVENT *win32Event ;

  win32Event = ofc_handle_lock (hEvent) ;
  if (win32Event != OFC_NULL)
    {
      CloseHandle (win32Event->winevent) ;
      ofc_free (win32Event) ;
      ofc_handle_destroy (hEvent) ;
      ofc_handle_unlock (hEvent) ;
    }
}

OFC_VOID ofc_event_wait_impl(OFC_HANDLE hEvent) 
{
  WIN32_EVENT *win32_event ;
  HANDLE win_handle ;

  win32_event = ofc_handle_lock (hEvent) ;
  if (win32_event != OFC_NULL)
    {
      win_handle = win32_event->winevent ;
      ofc_handle_unlock(hEvent) ;
      WaitForSingleObject (win_handle, INFINITE) ;
    }
}

OFC_BOOL ofc_event_test_impl(OFC_HANDLE hEvent)
{
  WIN32_EVENT *win32_event ;
  HANDLE win_handle ;
  OFC_BOOL ret ;

  ret = OFC_TRUE ;
  win32_event = ofc_handle_lock (hEvent) ;
  if (win32_event != OFC_NULL)
    {
      win_handle = win32_event->winevent ;
      ofc_handle_unlock(hEvent) ;
      if (WaitForSingleObject (win_handle, 0) == WAIT_TIMEOUT)
	ret = OFC_FALSE ;
    }
  return (ret) ;
}

HANDLE ofc_event_get_win32_handle (OFC_HANDLE hEvent)
{
  WIN32_EVENT *win32_event ;
  HANDLE ret_event ;

  ret_event = NULL ;
  win32_event = ofc_handle_lock (hEvent) ;
  if (win32_event != OFC_NULL)
    {
      ret_event = win32_event->winevent ;
      ofc_handle_unlock(hEvent) ;
    }

  return (ret_event) ;
}
