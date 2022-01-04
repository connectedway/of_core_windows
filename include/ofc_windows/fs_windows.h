/* Copyright (c) 2021 Connected Way, LLC. All rights reserved.
 * Use of this source code is governed by a Creative Commons 
 * Attribution-NoDerivatives 4.0 International license that can be
 * found in the LICENSE file.
 */
#if !defined(__OFC_FSWINDOWS_H__)
#define __OFC_FSWINDOWS_H__

#include "ofc/types.h"
#include "ofc/file.h"

#define OFC_FS_WINDOWS_BLOCK_SIZE 512

/**
 * \defgroup fs_windows Windows File System Dependent Support
 * \ingroup fs
 */

/** \{ */

#if defined(__cplusplus)
extern "C"
{
#endif

OFC_VOID OfcFSWindowsDestroyOverlapped(OFC_HANDLE hOverlapped);

OFC_VOID
OfcFSWindowsSetOverlappedOffset(OFC_HANDLE hOverlapped, OFC_OFFT offset);

OFC_VOID OfcFSWindowsStartup(OFC_VOID);

OFC_VOID OfcFSWindowsShutdown(OFC_VOID);

HANDLE OfcFSWindowsGetHandle(OFC_HANDLE);

HANDLE OfcFSWindowsGetOverlappedEvent(OFC_HANDLE hOverlapped);

#if defined(__cplusplus)
}
#endif

#endif

/** \} */
