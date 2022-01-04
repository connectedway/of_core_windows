/* Copyright (c) 2021 Connected Way, LLC. All rights reserved.
 * Use of this source code is governed by a Creative Commons 
 * Attribution-NoDerivatives 4.0 International license that can be
 * found in the LICENSE file.
 */
#include <windows.h>

#include "ofc/types.h"
#include "ofc/impl/consoleimpl.h"
#include "ofc/libc.h"

/**
 * \defgroup console_windows Windows Console Interface
 */

/** \{ */

#undef LOG_TO_FILE
#define LOG_FILE "openfiles.log"

HANDLE g_fd = INVALID_HANDLE_VALUE ;

static OFC_VOID open_log(OFC_VOID) {
#if defined(LOG_TO_FILE)
  g_fd = CreateFileA(LOG_FILE,
		     GENERIC_WRITE,
		     FILE_SHARE_DELETE | FILE_SHARE_WRITE | FILE_SHARE_READ,
		     NULL,
		     OPEN_ALWAYS,
		     FILE_ATTRIBUTE_NORMAL,
		     NULL);
#else
  g_fd = GetStdHandle (STD_OUTPUT_HANDLE);
#endif  
}

OFC_VOID ofc_write_stdout_impl(OFC_CCHAR *obuf, OFC_SIZET len) {
  DWORD dwLen;

  if (g_fd == INVALID_HANDLE_VALUE)
    open_log() ;
  WriteFile (g_fd, obuf, len, &dwLen, NULL);
}

OFC_VOID ofc_write_console_impl(OFC_CCHAR *obuf) {
  DWORD dwLen;
  
  if (g_fd == INVALID_HANDLE_VALUE)
    open_log() ;
  WriteFile (g_fd, obuf, ofc_strlen(obuf), &dwLen, NULL);
}

OFC_VOID ofc_read_stdin_impl(OFC_CHAR *inbuf, OFC_SIZET len) {
  BOOL status;
  DWORD dwLen;
  CHAR *p;

  status = ReadFile(GetStdHandle(STD_INPUT_HANDLE),
		    inbuf, len-1, &dwLen, NULL);
  if (status == TRUE)
    {
      inbuf[dwLen+1] = '\0';
      p = ofc_strtok(inbuf, "\n\r");
      *p = '\0';
    }
  else
    inbuf[0] = '\0';
}

OFC_VOID ofc_read_password_impl(OFC_CHAR *inbuf, OFC_SIZET len) {
  BOOL status ;
  DWORD dwLen ;
  CHAR *p ;
  DWORD mode ;

  /*
   * Make sure we leave room for the eos
   */
  GetConsoleMode (GetStdHandle (STD_INPUT_HANDLE), &mode) ;
  mode &= ~ENABLE_ECHO_INPUT ;
  SetConsoleMode (GetStdHandle (STD_INPUT_HANDLE), mode) ;

  status = ReadFile (GetStdHandle (STD_INPUT_HANDLE), 
		     inbuf, len-1, &dwLen, NULL) ;

  mode |= ENABLE_ECHO_INPUT ;
  SetConsoleMode (GetStdHandle (STD_INPUT_HANDLE), mode) ;

  if (status == OFC_TRUE)
    {
      /*
       * Make sure it's null terminated
       */
      inbuf[dwLen+1] = '\0' ;
      p = ofc_strtok (inbuf, "\n\r") ;
      *p = '\0' ;
    }
  else
    inbuf[0] = '\0' ;
}


/** \} */
