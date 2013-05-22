#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

/**
  * The source code in this file is the property of 
  * Ripple Systems and is not for redistribution
  * in any form.
  *
  * Source:   $File: //depot/3001_TIP_NEW/TA_BASE/transactive/core/threads/src/Thread.h $
  * @author:  B. Fountain
  * @version: $Revision: #2 $
  *
  * Last modification: $DateTime: 2012/02/06 16:15:14 $
  * Last modified by:  $Author: haijun.li $
  * 
  * Platform independent wrapper which allows us to manage a single thread. This
  * is an abstract class - extend it to specify run() and terminate() methods
  */

#include "omnithread.h"

#ifdef WIN32
#include <windows.h>
#elif defined ( SOLARIS ) || defined ( LINUX )
#include <pthread.h>
#else
#error Unsupported platform!
#endif // WIN32


class Thread
{
public:
    Thread()
        :  m_thread((omni_thread*)NULL),
          m_priority(omni_thread::PRIORITY_NORMAL)
    {
    }

    virtual ~Thread() {}

	void start()
    {
        if ( NULL == m_thread )
        {
            m_thread = new omni_thread(runThread, (void*) this, (omni_thread::priority_t)m_priority);
            m_thread->start();
        }
    }

    void terminateAndWait()
    {
        if (m_thread != NULL)
        {
            terminate();
            m_thread->join((void**)NULL);
            m_thread = (omni_thread*)NULL;
        }
    }

	void setPriority(int newPriority)
    {
        m_priority = newPriority;

        if (m_thread != NULL)
        {
            m_thread->set_priority((omni_thread::priority_t)m_priority);
        }
    }

	unsigned int getId() const
    {
        return (m_thread != NULL) ? m_thread->id() : static_cast< unsigned int >( -1 );
    }

    enum ThreadState_t
    {
        THREAD_STATE_NEW = omni_thread::STATE_NEW,
        THREAD_STATE_RUNNING = omni_thread::STATE_RUNNING,
        THREAD_STATE_TERMINATED = omni_thread::STATE_TERMINATED, 
        THREAD_STATE_UNKNOWN                                     
    };

    ThreadState_t getCurrentState() const
    {
        ThreadState_t lvCurrentState = THREAD_STATE_UNKNOWN;
        if( 0 != m_thread )
        {
            lvCurrentState = ( Thread::ThreadState_t )m_thread->state();
        }
        return lvCurrentState;
    }

	static void sleep(unsigned int milliseconds)
    {
        omni_thread::sleep(milliseconds / 1000, (milliseconds % 1000) * 1000000);
    }

    static void get_time(unsigned long* abs_sec, unsigned long* abs_nsec, unsigned long rel_sec = 0, unsigned long rel_nsec=0)
    {
        omni_thread::get_time(abs_sec, abs_nsec, rel_sec, rel_nsec );
    }

	static unsigned int getCurrentThreadId()
    {
#ifdef WIN32
        return ::GetCurrentThreadId();
#elif defined ( SOLARIS ) || defined ( LINUX )
        return pthread_self();
#else
#error Unsupported Platform!
#endif // WIN32
    }

    static void* runThread(void* ptr)
    {
        Thread* myThread = (Thread*) ptr;

        try
        {
            myThread->run();
        }
        catch (...)
        {
        }

        return NULL;
    }

    virtual void run() = 0;
    virtual void terminate() = 0;

private:

	omni_thread*    m_thread;
	unsigned int    m_priority;
};

#endif  // THREAD_H_INCLUDED
