/*
Microsoft Azure IoT Device Libraries
Copyright (c) Microsoft Corporation
All rights reserved. 
MIT License
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
documentation files (the Software), to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
IN THE SOFTWARE.
*/

#ifndef MICROMOCKTESTMUTEX_H
#define MICROMOCKTESTMUTEX_H

#ifdef WIN32
#include "windows.h"

typedef HANDLE MICROMOCK_MUTEX_HANDLE;
typedef HANDLE MICROMOCK_GLOBAL_SEMAPHORE_HANDLE;

extern "C" HANDLE MicroMockCreateMutex(void);
extern "C" int MicroMockAcquireMutex(HANDLE Mutex);
extern "C" int MicroMockReleaseMutex(HANDLE Mutex);
extern "C" void MicroMockDestroyMutex(HANDLE Mutex);

extern "C" HANDLE MicroMockCreateGlobalSemaphore(const char* name, unsigned int highWaterCount);
extern "C" int MicroMockAcquireGlobalSemaphore(HANDLE semaphore);
extern "C" int MicroMockReleaseGlobalSemaphore(HANDLE semaphore);
extern "C" void MicroMockDestroyGlobalSemaphore(HANDLE semaphore);

#else

typedef void* MICROMOCK_MUTEX_HANDLE;
typedef void* MICROMOCK_GLOBAL_SEMAPHORE_HANDLE;

#define MicroMockCreateMutex()          ((MICROMOCK_MUTEX_HANDLE)1)
#define MicroMockAcquireMutex(Mutex)    (1)
#define MicroMockReleaseMutex(Mutex)    (1)
#define MicroMockDestroyMutex(Mutex)

#define MicroMockCreateGlobalSemaphore(a, b) ((MICROMOCK_GLOBAL_SEMAPHORE_HANDLE)(1))
#define MicroMockAcquireGlobalSemaphore(semaphore) (1)
#define MicroMockReleaseGlobalSemaphore(semaphore) (1)
#define MicroMockDestroyGlobalSemaphore(semaphore)
#endif

#endif /*MICROMOCKTESTMUTEX_H*/

