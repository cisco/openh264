/*!
 * \copy
 *     Copyright (c)  2009-2013, Cisco Systems
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * \file	WelsThreadLib.c
 *
 * \brief	Interfaces introduced in thread programming
 *
 * \date	11/17/2009 Created
 *
 *************************************************************************************
 */


#include "WelsThreadLib.h"
#include <stdio.h>

#ifdef  WIN32

void WelsSleep( uint32_t dwMilliseconds )
{
	Sleep( dwMilliseconds );
}

WELS_THREAD_ERROR_CODE    WelsMutexInit( WELS_MUTEX   * mutex )
{
	InitializeCriticalSection(mutex);

	return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE    WelsMutexLock( WELS_MUTEX   * mutex )
{
	EnterCriticalSection(mutex);

	return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE    WelsMutexUnlock( WELS_MUTEX * mutex )
{
	LeaveCriticalSection(mutex);

	return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE    WelsMutexDestroy( WELS_MUTEX * mutex )
{
    DeleteCriticalSection(mutex);

	return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE    WelsEventInit( WELS_EVENT  *  event )
{
    WELS_EVENT   h = CreateEvent(NULL, FALSE, FALSE, NULL);

	if( h == NULL ){
		return WELS_THREAD_ERROR_GENERIAL;
	}
	*event = h;
	return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE    WelsEventSignal( WELS_EVENT * event )
{
	if( SetEvent( *event ) ){
		return WELS_THREAD_ERROR_OK;
	}
	return WELS_THREAD_ERROR_GENERIAL;
}

WELS_THREAD_ERROR_CODE    WelsEventReset( WELS_EVENT * event )
{
	if ( ResetEvent( *event ) )
		return WELS_THREAD_ERROR_OK;
	return WELS_THREAD_ERROR_GENERIAL;
}

WELS_THREAD_ERROR_CODE    WelsEventWait( WELS_EVENT * event )
{
	return WaitForSingleObject(*event, INFINITE );
}

WELS_THREAD_ERROR_CODE    WelsEventWaitWithTimeOut( WELS_EVENT * event, uint32_t dwMilliseconds )
{
	return WaitForSingleObject(*event, dwMilliseconds );
}

WELS_THREAD_ERROR_CODE    WelsMultipleEventsWaitSingleBlocking(	uint32_t nCount,
																WELS_EVENT *event_list,
																uint32_t dwMilliseconds )
{
	return WaitForMultipleObjects( nCount, event_list, FALSE, dwMilliseconds );
}

WELS_THREAD_ERROR_CODE    WelsMultipleEventsWaitAllBlocking( uint32_t nCount, WELS_EVENT *event_list )
{
	return WaitForMultipleObjects( nCount, event_list, TRUE, (uint32_t)-1 );
}

WELS_THREAD_ERROR_CODE    WelsEventDestroy( WELS_EVENT * event )
{
	CloseHandle( *event );

	*event = NULL;
	return WELS_THREAD_ERROR_OK;
}


WELS_THREAD_ERROR_CODE    WelsThreadCreate( WELS_THREAD_HANDLE * thread,  LPWELS_THREAD_ROUTINE  routine, 
										   void * arg, WELS_THREAD_ATTR attr)
{
    WELS_THREAD_HANDLE   h = CreateThread(NULL, 0, routine, arg, 0, NULL);

	if( h == NULL ) {
		return WELS_THREAD_ERROR_GENERIAL;
	}
	* thread = h;

	return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE	  WelsSetThreadCancelable()
{
	// nil implementation for WIN32
	return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE    WelsThreadJoin( WELS_THREAD_HANDLE  thread )
{
    WaitForSingleObject(thread, INFINITE);

	return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE    WelsThreadCancel( WELS_THREAD_HANDLE  thread )
{
	return WELS_THREAD_ERROR_OK;
}


WELS_THREAD_ERROR_CODE    WelsThreadDestroy( WELS_THREAD_HANDLE *thread )
{
	if ( thread != NULL )
	{
		CloseHandle(*thread);
		*thread = NULL;
	}	
	return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_HANDLE        WelsThreadSelf()
{
	return GetCurrentThread();
}

WELS_THREAD_ERROR_CODE    WelsQueryLogicalProcessInfo(WelsLogicalProcessInfo * pInfo)
{
	SYSTEM_INFO  si;	
	
	GetSystemInfo(&si);

	pInfo->ProcessorCount = si.dwNumberOfProcessors;

	return WELS_THREAD_ERROR_OK;
}

#elif   defined(__GNUC__)

#ifdef MACOS
#include <CoreServices/CoreServices.h>
//#include <Gestalt.h>
#endif//MACOS

static int32_t  SystemCall(const str_t * pCmd, str_t * pRes, int32_t iSize)
{
    int32_t fd[2];
    int32_t iPid;
    int32_t iCount;
    int32_t left;
    str_t * p = NULL;
    int32_t iMaxLen = iSize - 1;
    memset(pRes, 0, iSize);

    if( pipe(fd) ){
        return -1;
    }

    if( (iPid = fork()) == 0 ){
        int32_t  fd2[2];
        if( pipe(fd2) ){
            return -1;
        }
        close(STDOUT_FILENO);
        dup2(fd2[1],STDOUT_FILENO);
        close(fd[0]);
        close(fd2[1]);
        system(pCmd);
        read(fd2[0], pRes, iMaxLen);
        write(fd[1], pRes, strlen(pRes));	// confirmed_safe_unsafe_usage
        close(fd2[0]);
		close(fd[1]);
        exit(0);
    }
    close(fd[1]);
    p = pRes;
    left = iMaxLen;
    while( (iCount = read(fd[0], p, left)) ){
        p += iCount;
        left -= iCount;
        if( left <=0 ) break;   
    }
    close(fd[0]);
    return 0;
}

void WelsSleep( uint32_t dwMilliseconds )
{
	usleep( dwMilliseconds * 1000 );	// microseconds
}

WELS_THREAD_ERROR_CODE    WelsThreadCreate( WELS_THREAD_HANDLE * thread,  LPWELS_THREAD_ROUTINE  routine, 
										   void * arg, WELS_THREAD_ATTR attr)
{
	WELS_THREAD_ERROR_CODE err = 0;

	pthread_attr_t at;
	err = pthread_attr_init(&at);
	if ( err )
		return err;
	err = pthread_attr_setscope(&at, PTHREAD_SCOPE_SYSTEM);
	if ( err )
		return err;
	err = pthread_attr_setschedpolicy(&at, SCHED_FIFO);
	if ( err )
		return err;
	err = pthread_create( thread, &at, routine, arg );

	pthread_attr_destroy(&at);

	return err;

//	return pthread_create(thread, NULL, routine, arg); 
}

WELS_THREAD_ERROR_CODE	  WelsSetThreadCancelable()
{
	WELS_THREAD_ERROR_CODE err = pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
	if ( 0 == err )
		err = pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, NULL );
	return err;
}

WELS_THREAD_ERROR_CODE    WelsThreadJoin( WELS_THREAD_HANDLE  thread )
{
    return pthread_join(thread, NULL);
}

WELS_THREAD_ERROR_CODE    WelsThreadCancel( WELS_THREAD_HANDLE  thread )
{
	return pthread_cancel( thread );
}

WELS_THREAD_ERROR_CODE    WelsThreadDestroy( WELS_THREAD_HANDLE *thread )
{	
	return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_HANDLE        WelsThreadSelf()
{
	return pthread_self();
}

WELS_THREAD_ERROR_CODE    WelsMutexInit( WELS_MUTEX   * mutex )
{
	return pthread_mutex_init(mutex, NULL);
}

WELS_THREAD_ERROR_CODE    WelsMutexLock( WELS_MUTEX   * mutex )
{
	return pthread_mutex_lock(mutex);
}

WELS_THREAD_ERROR_CODE    WelsMutexUnlock( WELS_MUTEX * mutex )
{
	return pthread_mutex_unlock(mutex);
}

WELS_THREAD_ERROR_CODE    WelsMutexDestroy( WELS_MUTEX * mutex )
{
    return pthread_mutex_destroy(mutex);
}

// unnamed semaphores can not work well for posix threading models under not root users

WELS_THREAD_ERROR_CODE    WelsEventInit( WELS_EVENT *event )
{
	return sem_init(event, 0, 0);
}

WELS_THREAD_ERROR_CODE   WelsEventDestroy( WELS_EVENT * event )
{
	return sem_destroy( event );	// match with sem_init	
}

WELS_THREAD_ERROR_CODE    WelsEventOpen( WELS_EVENT **p_event, str_t *event_name )
{
	if ( p_event == NULL || event_name == NULL )
		return WELS_THREAD_ERROR_GENERIAL;
	*p_event = sem_open(event_name, O_CREAT,  (S_IRUSR | S_IWUSR)/*0600*/, 0);
	if ( *p_event == (sem_t *)SEM_FAILED ) {
		sem_unlink( event_name );
		*p_event = NULL;
		return WELS_THREAD_ERROR_GENERIAL;
	} else {		
		return WELS_THREAD_ERROR_OK;
	}
}
WELS_THREAD_ERROR_CODE    WelsEventClose( WELS_EVENT *event, str_t *event_name )
{
	WELS_THREAD_ERROR_CODE err = sem_close( event );	// match with sem_open
	if ( event_name )
		sem_unlink( event_name );
	return err;
}

WELS_THREAD_ERROR_CODE   WelsEventSignal( WELS_EVENT * event )
{
	WELS_THREAD_ERROR_CODE err = 0;
//	int32_t val = 0;
//	sem_getvalue(event, &val);
//	fprintf( stderr, "before signal it, val= %d..\n",val );
	err = sem_post(event);
//	sem_getvalue(event, &val);
//	fprintf( stderr, "after signal it, val= %d..\n",val );
    return err;
}
WELS_THREAD_ERROR_CODE    WelsEventReset( WELS_EVENT * event )
{
	// FIXME for posix event reset, seems not be supported for pthread??
	sem_close(event);
	return sem_init(event, 0, 0);
}

WELS_THREAD_ERROR_CODE   WelsEventWait( WELS_EVENT * event )
{
	return sem_wait(event);	// blocking until signaled
}

WELS_THREAD_ERROR_CODE    WelsEventWaitWithTimeOut( WELS_EVENT * event, uint32_t dwMilliseconds )
{	
	if ( dwMilliseconds != (uint32_t)-1 )
	{
		return sem_wait(event);
	}
	else
	{
#if defined(MACOS)
		int32_t err = 0;
		int32_t wait_count = 0;
		do{
			err = sem_trywait(event);
			if ( WELS_THREAD_ERROR_OK == err)
				break;// WELS_THREAD_ERROR_OK;
			else if ( wait_count > 0 )
				break;
			usleep( dwMilliseconds * 1000 );
			++ wait_count;
		}while(1);
		return err;
#else
		struct timespec ts;
		struct timeval tv;

		gettimeofday(&tv,0);

		ts.tv_sec = tv.tv_sec + dwMilliseconds /1000;
		ts.tv_nsec = tv.tv_usec*1000 + (dwMilliseconds % 1000) * 1000000;

		return sem_timedwait(event, &ts);
#endif//MACOS
	}
}

WELS_THREAD_ERROR_CODE    WelsMultipleEventsWaitSingleBlocking(	uint32_t nCount,
																WELS_EVENT **event_list,
																uint32_t dwMilliseconds )
{
	// bWaitAll = FALSE && blocking
	uint32_t nIdx = 0;
	const uint32_t kuiAccessTime = 2;	// 2 us once
//	uint32_t uiSleepMs = 0;

	if ( nCount == 0 )
		return WELS_THREAD_ERROR_WAIT_FAILED;

	while (1)
	{
		nIdx = 0;	// access each event by order
		while ( nIdx < nCount )
		{
			int32_t err = 0;			
//#if defined(MACOS)	// clock_gettime(CLOCK_REALTIME) & sem_timedwait not supported on mac, so have below impl
			int32_t wait_count = 0;
//			struct timespec ts;
//			struct timeval tv;
//			
//			gettimeofday(&tv,0);
//			ts.tv_sec = tv.tv_sec/*+ kuiAccessTime / 1000*/;		// second
//			ts.tv_nsec = (tv.tv_usec + kuiAccessTime) * 1000;	// nano-second
			
			/*
			 * although such interface is not used in __GNUC__ like platform, to use 
			 * pthread_cond_timedwait() might be better choice if need
			 */
			do{
				err = sem_trywait( event_list[nIdx] );
				if ( WELS_THREAD_ERROR_OK == err )
					return WELS_THREAD_ERROR_WAIT_OBJECT_0 + nIdx;
				else if ( wait_count > 0 )
					break;
				usleep(kuiAccessTime);
				++ wait_count;
			}while( 1 );
//#else
//			struct timespec ts;
//			
//			if ( clock_gettime(CLOCK_REALTIME, &ts) == -1 )
//				return WELS_THREAD_ERROR_WAIT_FAILED;
//			ts.tv_nsec += kuiAccessTime/*(kuiAccessTime % 1000)*/ * 1000;
//			
////			fprintf( stderr, "sem_timedwait(): start to wait event %d..\n", nIdx );
//			err = sem_timedwait(event_list[nIdx], &ts);
////			if ( err == -1 )
////			{
////				sem_getvalue(&event_list[nIdx], &val);
////				fprintf( stderr, "sem_timedwait() errno(%d) semaphore %d..\n", errno, val);
////				return WELS_THREAD_ERROR_WAIT_FAILED;
////			}			
////			fprintf( stderr, "sem_timedwait(): wait event %d result %d errno %d..\n", nIdx, err, errno );
//			if ( WELS_THREAD_ERROR_OK == err ) // non-blocking mode
//			{	
////				int32_t val = 0;
////				sem_getvalue(&event_list[nIdx], &val);
////				fprintf( stderr, "after sem_timedwait(), event_list[%d] semaphore value= %d..\n", nIdx, val);
////				fprintf( stderr, "WelsMultipleEventsWaitSingleBlocking sleep %d us\n", uiSleepMs);
//				return WELS_THREAD_ERROR_WAIT_OBJECT_0 + nIdx;
//			}
//#endif					
			// we do need access next event next time
			++ nIdx;
//			uiSleepMs += kuiAccessTime;
		}
		usleep( 1 );	// switch to working threads
//		++ uiSleepMs;
	}	

	return WELS_THREAD_ERROR_WAIT_FAILED;
}

WELS_THREAD_ERROR_CODE    WelsMultipleEventsWaitAllBlocking( uint32_t nCount, WELS_EVENT **event_list )
{
	// bWaitAll = TRUE && blocking
	uint32_t nIdx = 0;
//	const uint32_t kuiAccessTime = (uint32_t)-1;// 1 ms once
	uint32_t uiCountSignals = 0;
	uint32_t uiSignalFlag	= 0;	// UGLY: suppose maximal event number up to 32
	
	if ( nCount == 0 || nCount > (sizeof(uint32_t)<<3) )
		return WELS_THREAD_ERROR_WAIT_FAILED;
	
	while (1)
	{
		nIdx = 0;	// access each event by order
		while (nIdx < nCount)
		{			
			const uint32_t kuiBitwiseFlag = (1<<nIdx);
			
			if ( (uiSignalFlag & kuiBitwiseFlag) != kuiBitwiseFlag ) // non-blocking mode
			{	
				int32_t err = 0;
//				fprintf( stderr, "sem_wait(): start to wait event %d..\n", nIdx );
				err = sem_wait(event_list[nIdx]);
//				fprintf( stderr, "sem_wait(): wait event %d result %d errno %d..\n", nIdx, err, errno );
				if ( WELS_THREAD_ERROR_OK == err )
				{
//					int32_t val = 0;
//					sem_getvalue(&event_list[nIdx], &val);
//					fprintf( stderr, "after sem_timedwait(), event_list[%d] semaphore value= %d..\n", nIdx, val);

					uiSignalFlag |= kuiBitwiseFlag;
					++ uiCountSignals;
					if ( uiCountSignals >= nCount )
					{						
						return WELS_THREAD_ERROR_OK;
					}
				}				
			}			
			// we do need access next event next time
			++ nIdx;
		}		
	}	
	
	return WELS_THREAD_ERROR_WAIT_FAILED;
}

WELS_THREAD_ERROR_CODE    WelsQueryLogicalProcessInfo(WelsLogicalProcessInfo * pInfo)
{
#ifdef LINUX

#define   CMD_RES_SIZE    2048
    str_t pBuf[CMD_RES_SIZE];
   
    SystemCall("cat /proc/cpuinfo | grep \"processor\" | wc -l", pBuf, CMD_RES_SIZE);

    pInfo->ProcessorCount = atoi(pBuf);

    if( pInfo->ProcessorCount == 0 ){
        pInfo->ProcessorCount = 1;
    }   
 
	return WELS_THREAD_ERROR_OK;
#undef   CMD_RES_SIZE

#else

	SInt32 cpunumber;
	Gestalt(gestaltCountOfCPUs,&cpunumber);

	pInfo->ProcessorCount	= cpunumber;

	return WELS_THREAD_ERROR_OK;

#endif//LINUX
}

#endif



