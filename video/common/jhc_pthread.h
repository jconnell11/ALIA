// jhc_pthread.h - Windows versions of simple POSIX thread functions
// 
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
// 
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023 Etaoin Systems
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
///////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __linux__

#include <process.h>

// type of a thread identifier
#define pthread_t HANDLE

// start a new thread running tfcn(arg)
#define pthread_create(pth, attr, tfcn, arg)  \
  *pth = (HANDLE) _beginthreadex(NULL, 0, tfcn, arg, 0, NULL)

// return value for a thread function (non-standard)
#define pthread_ret unsigned int __stdcall

// check if thread has terminated (non-standard)
#define pthread_busy(bar) (WaitForSingleObject(bar, 0) != WAIT_OBJECT_0)

// wait for thread to terminate
#define pthread_join(bar, ret) WaitForSingleObject(bar, INFINITE);

// type of a mutex identifier
#define pthread_mutex_t HANDLE

// grab control of a mutex
#define pthread_mutex_lock(foo)  \
  {if (foo == INVALID_HANDLE_VALUE) foo = CreateMutex(NULL, FALSE, NULL);  \
   WaitForSingleObject(foo, INFINITE);}

// release control of a mutex
#define pthread_mutex_unlock(foo) ReleaseMutex(foo);

// deallocate a mutex
#define pthread_mutex_destroy(p_foo)  \
  {if (*p_foo != INVALID_HANDLE_VALUE) CloseHandle(*p_foo);}

#else

#include <pthread.h>
#include <errno.h>

// return value for a thread function (non-standard)
#define pthread_ret void *

// check if thread has terminated (non-standard)
#define pthread_busy(bar) (pthread_tryjoin_np(bar, NULL) == EBUSY)

#endif