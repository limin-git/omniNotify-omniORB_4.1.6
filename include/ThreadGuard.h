#ifndef THREAD_GUARD_H_INCLUDED
#define THREAD_GUARD_H_INCLUDED

#include <omnithread.h>


class ThreadGuard
{
public:

    ThreadGuard( TW_Mutex& lock )
        : m_lock( lock )
    {
        m_lock.acquire();
    }

    virtual ~ThreadGuard()
    {
        m_lock.release();
    }

    omni_mutex& m_lock;
};

#define THREAD_GUARD( lock ) ThreadGuard lock_guard ## __LINE__ ( lock )


#endif
