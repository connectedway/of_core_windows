/* Copyright (c) 2021 Connected Way, LLC. All rights reserved.
 * Use of this source code is governed by a Creative Commons 
 * Attribution-NoDerivatives 4.0 International license that can be
 * found in the LICENSE file.
 */
#if !defined(__OFC_EVENT_WINDOWS_H__)
#define __OFC_EVENT_WINDOWS_H__

#if defined(__cplusplus)
extern "C"
{
#endif
  HANDLE ofc_event_get_win32_handle (OFC_HANDLE hEvent);
#if defined(__cplusplus)
}
#endif

#endif
