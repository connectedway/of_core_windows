/* Copyright (c) 2021 Connected Way, LLC. All rights reserved.
 * Use of this source code is governed by a Creative Commons 
 * Attribution-NoDerivatives 4.0 International license that can be
 * found in the LICENSE file.
 */
#include <windows.h>

#include "ofc/types.h"
#include "ofc/env.h"
#include "ofc/libc.h"

#include "ofc/heap.h"

static const char *env2str[OFC_ENV_NUM] =
static LPCTSTR env2str[OFC_ENV_NUM] =
  {
    TSTR("OPEN_FILES_HOME"),
    TSTR("OPEN_FILES_INSTALL"),
    TSTR("OPEN_FILES_ROOT")
  } ;

OFC_BOOL
ofc_env_get_impl(OFC_ENV_VALUE value, OFC_TCHAR *ptr, OFC_SIZET len) {
  OFC_BOOL ret ;
  DWORD status ;

  ret = OFC_FALSE ;
  if (ptr != NULL && value < OFC_ENV_NUM)
    {
      status = GetEnvironmentVariable (env2str[value], ptr, (DWORD) (len)) ;

      if (status > 0)
	{
	  ret = OFC_TRUE ;
	}
    }
  return (ret) ;
}
