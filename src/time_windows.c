/* Copyright (c) 2021 Connected Way, LLC. All rights reserved.
 * Use of this source code is governed by a Creative Commons 
 * Attribution-NoDerivatives 4.0 International license that can be
 * found in the LICENSE file.
 */
#include <windows.h>

#include "ofc/types.h"
#include "ofc/time.h"
#include "ofc/impl/timeimpl.h"

#include "ofc/file.h"

/**
 * \defgroup time_windows Windows Timer Interface
 */

OFC_MSTIME ofc_time_get_now_impl(OFC_VOID) {
  DWORD now ;
#if defined(_WINCE_)
  now = GetTickCount() ;
#else
  now = GetTickCount() ;
#endif

  return ((OFC_MSTIME) now) ;
}

OFC_VOID ofc_time_get_file_time_impl(OFC_FILETIME *filetime) {
  SYSTEMTIME systemtime ;

  GetSystemTime (&systemtime) ;
  SystemTimeToFileTime (&systemtime, (FILETIME *) filetime) ;
}

OFC_UINT16 ofc_time_get_timezone_impl(OFC_VOID) {
  TIME_ZONE_INFORMATION timezone ;

  GetTimeZoneInformation (&timezone) ;
  return ((OFC_UINT16) timezone.Bias) ;
}

OFC_BOOL ofc_file_time_to_dos_date_time_impl(const OFC_FILETIME *lpFileTime,
                                             OFC_WORD *lpFatDate,
                                             OFC_WORD *lpFatTime) {
  return (FileTimeToDosDateTime ((const FILETIME *) lpFileTime,
				 (LPWORD) lpFatDate,
				 (LPWORD) lpFatTime)) ;
}

OFC_BOOL ofc_dos_date_time_to_file_time_impl(OFC_WORD FatDate,
                                             OFC_WORD FatTime,
                                             OFC_FILETIME *lpFileTime) {
  return (DosDateTimeToFileTime ((WORD) FatDate, (WORD) FatTime,
				 (FILETIME *) lpFileTime)) ;
}

OFC_MSTIME ofc_get_runtime_impl(OFC_VOID) {
  BOOL ret ;
  FILETIME lCreationTime ;
  FILETIME lExitTime ;
  FILETIME lKernelTime ;
  FILETIME lUserTime ;
  OFC_MSTIME runtime ;
  OFC_UINT64 ns ;

  runtime = 0 ;
  ret = GetThreadTimes (GetCurrentThread(), &lCreationTime, &lExitTime,
			&lKernelTime, &lUserTime) ;
  if (ret != 0)
    {
      ns = (OFC_UINT64) lKernelTime.dwHighDateTime << 32 | 
	lKernelTime.dwLowDateTime ;
      runtime += (OFC_MSTIME)(ns / (1000 * 10)) ;
      ns = (OFC_UINT64) lUserTime.dwHighDateTime << 32 | 
	lUserTime.dwLowDateTime ;
      runtime += (OFC_MSTIME)(ns / (1000 * 10)) ;
    }
  return (runtime) ;
}

