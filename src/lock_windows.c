/* Copyright (c) 2021 Connected Way, LLC. All rights reserved.
 * Use of this source code is governed by a Creative Commons 
 * Attribution-NoDerivatives 4.0 International license that can be
 * found in the LICENSE file.
 */
#include <windows.h>

#include "ofc/types.h"
#include "ofc/lock.h"
#include "ofc/heap.h"

typedef struct
{
  OFC_UINT32 thread ;
  HANDLE lock ;
} OFC_LOCK_IMPL ;

OFC_VOID ofc_lock_destroy_impl(OFC_LOCK_IMPL *lock)
{
  CloseHandle (lock->lock) ;
  ofc_free(lock);
}

OFC_VOID *ofc_lock_init_impl(OFC_VOID)
{
    OFC_LOCK_IMPL *lock;

    lock = ofc_malloc(sizeof(OFC_LOCK_IMPL));
    lock->lock = CreateMutex (NULL, FALSE, NULL) ;

    return (lock);
}

OFC_BOOL ofc_lock_try_impl(OFC_LOCK_IMPL *lock)
{
  OFC_BOOL ret ;
  DWORD status ;
  
  status = WaitForSingleObject (lock->lock, 0) ;

  if (status == WAIT_OBJECT_0)
    {
      ret = OFC_TRUE ;
    }
  else
    ret = OFC_FALSE ;
  return (ret) ;
}

OFC_VOID ofc_lock_impl(OFC_LOCK_IMPL *lock)
{
  WaitForSingleObject (lock->lock, INFINITE) ;
}

OFC_VOID ofc_unlock_impl(OFC_LOCK_IMPL *lock)
{
  ReleaseMutex (lock->lock) ;
}

