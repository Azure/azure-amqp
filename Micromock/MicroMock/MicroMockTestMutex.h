//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//

#ifndef MICROMOCKTESTMUTEX_H
#define MICROMOCKTESTMUTEX_H

#ifdef WIN32

typedef HANDLE MICROMOCK_MUTEX_HANDLE;

#if (defined _WIN32_WCE)

#define MicroMockCreateMutex()		CreateMutexW(NULL, FALSE, NULL)

#define MicroMockAcquireMutex(Mutex)	(WaitForSingleObject((Mutex), INFINITE) != WAIT_ABANDONED)

#else

#define MicroMockCreateMutex()		CreateMutexEx(NULL, FALSE, NULL,MUTEX_ALL_ACCESS )

#define MicroMockAcquireMutex(Mutex)	(WaitForSingleObjectEx((Mutex), INFINITE,FALSE) != WAIT_ABANDONED)

#endif

#define MicroMockDestroyMutex(Mutex)	(void)CloseHandle(Mutex)

#define MicroMockReleaseMutex(Mutex)	ReleaseMutex((Mutex))

#else

typedef void* MICROMOCK_MUTEX_HANDLE;

#define MicroMockCreateMutex()          ((MICROMOCK_MUTEX_HANDLE)1)
#define MicroMockDestroyMutex(Mutex)
#define MicroMockAcquireMutex(Mutex)    (1)
#define MicroMockReleaseMutex(Mutex)    (1)

#endif

#endif
