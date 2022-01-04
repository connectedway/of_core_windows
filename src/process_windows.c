/* Copyright (c) 2021 Connected Way, LLC. All rights reserved.
 * Use of this source code is governed by a Creative Commons 
 * Attribution-NoDerivatives 4.0 International license that can be
 * found in the LICENSE file.
 */
#define __OFC_CORE_DLL

#include <windows.h>

#include "ofc/types.h"
#include "ofc/handle.h"
#include "ofc/process.h"
#include "ofc/libc.h"
#include "ofc/thread.h"

#include "ofc/impl/consoleimpl.h"

#include "ofc/heap.h"

OFC_PROCESS_ID ofc_process_get_impl(OFC_VOID) {
  DWORD pid ;

  pid = GetCurrentProcessId() ;
  return ((OFC_PROCESS_ID) pid) ;
}

OFC_VOID ofc_process_block_signal(OFC_INT signal) {
  /* this is a noop on win32 */
}

OFC_VOID ofc_process_unblock_signal(OFC_INT signal) {
  /* This is a noop on win32 */
}

OFC_VOID ofc_process_signal (OFC_PROCESS_ID process, OFC_INT signal,
			     OFC_INT value) {
  /* This also a noop */
}

OFC_BOOL ofc_process_term_trap_impl (OFC_PROCESS_TRAP_HANDLER trap) {
  /*
   * On win32, we cannot register termination callbacks for the process
   */
  return (OFC_TRUE) ;
}

OFC_HANDLE ofc_process_exec_impl (OFC_CTCHAR *name,
				 OFC_TCHAR *uname,
				 OFC_INT argc,
				  OFC_CHAR **argv)  {
  OFC_HANDLE hProcess ;
  OFC_TCHAR *tname ;
  OFC_INT len ;
  OFC_CHAR *cmdline ;
  OFC_TCHAR *tcmdline ;
  OFC_INT i ;
  OFC_DWORD dwLastError ;

  BOOL status ;
  STARTUPINFO startupinfo ;
  PROCESS_INFORMATION processinfo ;
  DWORD pid ;

  hProcess = OFC_HANDLE_NULL ;
  
  tname = ofc_malloc (OFC_MAX_PATH) ;
  ofc_tstrncpy (tname, name, OFC_MAX_PATH) ;

  len = ofc_tstrnlen (tname, OFC_MAX_PATH) ;
  if (len < OFC_MAX_PATH)
    {
      ofc_tstrncpy (tname + len, TSTR(".exe"), OFC_MAX_PATH) ;
      tname[OFC_MAX_PATH] = TCHAR_EOS ;
    }

  len = 0 ;
  for (i = 0 ; i < argc ; i++)
    {
      /* include a space */
      if (i > 0)
	len++ ;
      len += ofc_strlen (argv[i]) ;
    }
  /* include a null */
  len++ ;

  cmdline = ofc_malloc (len) ;

  len = 0 ;
  for (i = 0 ; i < argc ; i++)
    {
      /* include a space */
      if (i > 0)
	cmdline[len++] = ' ' ;
      ofc_strcpy (&cmdline[len], argv[i]) ;
      len += ofc_strlen (argv[i]) ;
    }
  cmdline[len++] = '\0' ;
  tcmdline = ofc_cstr2tstr(cmdline) ;

  /*
   * Create a process with the same credentials as us
   */
  ofc_memset (&startupinfo, '\0', sizeof (startupinfo)) ;
  startupinfo.cb = sizeof (startupinfo) ;
  ofc_memset (&processinfo, '\0', sizeof (processinfo)) ;
  status = CreateProcess (tname, tcmdline, NULL, NULL, FALSE, 
			  0, NULL, NULL, 
			  &startupinfo, &processinfo) ;
  if (status == OFC_TRUE)
    {
      pid = processinfo.dwProcessId ;
      /*
       * We don't need the handles
       */
      CloseHandle (processinfo.hProcess) ;
      CloseHandle (processinfo.hThread) ;
      hProcess = 
	ofc_handle_create (OFC_HANDLE_PROCESS, (OFC_VOID *) pid) ;
    }
  else
    {
      dwLastError = GetLastError () ;
      ofc_printf ("Unable to create process %d\n", dwLastError) ;
    }
			      
  ofc_free (tcmdline) ;
  ofc_free (cmdline) ;
  ofc_free (tname) ;

  return (hProcess) ;
}

OFC_PROCESS_ID ofc_process_get_id_impl (OFC_HANDLE hProcess)
{
  DWORD pid ;

  pid = (DWORD) ofc_handle_lock (hProcess) ;
  if (pid != (DWORD) 0)
    ofc_handle_unlock (hProcess) ;

  return ((OFC_PROCESS_ID) pid) ;
}
  
OFC_VOID ofc_process_term_impl(OFC_HANDLE hProcess) 
{
  DWORD pid ;
  HANDLE handle ;
  DWORD dwLastError ;

  pid = (DWORD) ofc_handle_lock (hProcess) ;

  if (pid != (DWORD) 0)
    {
      handle = OpenProcess (PROCESS_TERMINATE, FALSE, pid) ;
      if (handle == NULL)
	{
	  dwLastError = GetLastError () ;
	  ofc_printf ("Cannot delete process %d\n", dwLastError) ;
	}
      else
	{
	  TerminateProcess (handle, 0) ;
	  CloseHandle (handle) ;
	}
      ofc_handle_destroy (hProcess) ;
      ofc_handle_unlock (hProcess) ;
    }
}

OFC_VOID ofc_process_kill_impl(OFC_PROCESS_ID pid) {
  HANDLE handle ;

  handle = OpenProcess (PROCESS_TERMINATE, FALSE, pid) ;
  if (handle == NULL)
    ofc_printf ("Cannot delete process\n") ;
  else
    {
      if (TerminateProcess (handle, 0) == 0)
	ofc_printf ("Terminate Process Failed with %d\n", 
		     GetLastError());
      else
	CloseHandle (handle) ;
    }
}

OFC_VOID
ofc_process_crash_impl(OFC_CCHAR *obuf) {
  ofc_write_console_impl(obuf) ;
  ExitProcess(1);
}  
