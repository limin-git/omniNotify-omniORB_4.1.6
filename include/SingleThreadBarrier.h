#ifndef SINGLE_THREAD_BARRIER_H_INCLUDED
#define SINGLE_THREAD_BARRIER_H_INCLUDED

#include "RDI.h"
#include <omnithread.h>


class SingleThreadBarrier
{
public:
    SingleThreadBarrier(unsigned long initial = 1) 
        : m_semaphore(0),
        m_initial(initial)
    {
        if ( 0 < initial )
        {
            m_count = ( initial - 1 );
        }
        else
        {
            m_count = 0;
        }
    }

    virtual ~SingleThreadBarrier()
    {
        m_semaphore.post();
    }

    void wait()
    {
        m_semaphore.wait();
    }

    void post()
    {
        {
            THREAD_GUARD( m_countLock );

#ifdef DEBUG_THREAD_POOL_BARRIER
            RDIDbgForceLog( "SingleThreadBarrier::post - 1 - m_count=" << m_count << "\n" );
#endif
            if ( 0 < m_count )
            {
                --m_count;
                return;
            }
#ifdef DEBUG_THREAD_POOL_BARRIER
            RDIDbgForceLog( "SingleThreadBarrier::post - 2 - m_count=" << m_count << "\n" );
#endif
        }

        m_semaphore.post();
    }

protected:
    omni_semaphore m_semaphore;
public:

    omni_mutex m_countLock;
    unsigned long m_count;
    unsigned long m_initial;
};


#endif
