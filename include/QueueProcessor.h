/**
  * The source code in this file is the property of
  * Ripple Systems and is not for redistribution
  * in any form.
  *
  * Source:   $File: //depot/3001_TIP_NEW/TA_BASE/transactive/core/threads/src/QueueProcessor.h $
  * @author:  Ripple
  * @version: $Revision: #2 $
  *
  * Last modification: $DateTime: 2012/02/06 16:15:14 $
  * Last modified by:  $Author: haijun.li $
  *
  * The QueueProcessor thread is responsible for processing queue items
  * using a pure virtual method provided by the subclass
  *
  */

#ifndef QUEUEPROCESSOR_H
#define QUEUEPROCESSOR_H

#include "Thread.h"
#include "ThreadGuard.h"
#include "QueueItem.h"
#include "RDI.h"
#include <deque>
#include <boost/shared_ptr.hpp>


template<class ITEM> class QueueProcessor : public Thread
{

private:

	typedef typename std::deque< QueueItem<ITEM> > ItemQueue;

public:

	QueueProcessor<ITEM> (unsigned long maxQueueSize = MAX_QUEUEPROCESSOR_QUEUE_SIZE, bool canDeleteItem = true) 
	    : m_semaphore(0),
          m_maxQueueSize(maxQueueSize),
          m_keepRunning(true),
          m_logStats(false)
	{
	}

	~QueueProcessor<ITEM> () 
	{
	};

	void insert( boost::shared_ptr<ITEM>& newItem )
	{
#ifdef DEBUG_THREAD_POOL_BARRIER
        RDIDbgForceLog( "QueueProcessor::insert - 1 m_queue.size=" << m_queue.size() << "\n" );
#endif
		THREAD_GUARD( m_queueLock );

#ifdef DEBUG_THREAD_POOL_BARRIER
        RDIDbgForceLog( "QueueProcessor::insert - 2 m_queue.size=" << m_queue.size() << "\n" );
#endif

		unsigned long queueSize = m_queue.size();
		if (queueSize == m_maxQueueSize)
		{
			QueueItem<ITEM> oldestItem = m_queue.front();
			m_queue.pop_front();
		}
		QueueItem<ITEM> newPtr( newItem );
		m_queue.push_back( newPtr );

#ifdef DEBUG_THREAD_POOL_BARRIER
        RDIDbgForceLog( "QueueProcessor::insert - 3 m_queue.size=" << m_queue.size() << "\n" );
#endif

		m_semaphore.post();

#ifdef DEBUG_THREAD_POOL_BARRIER
        RDIDbgForceLog( "QueueProcessor::insert - 4 m_queue.size=" << m_queue.size() << "\n" );
#endif
	}

	void insertUnique( boost::shared_ptr<ITEM>& newItem )
	{
		bool itemExists = false;

		THREAD_GUARD( m_queueLock );

		if ( ! m_queue.empty() )
		{
			itemExists = ( std::find(m_queue.begin(), m_queue.end(), newItem) != m_queue.end() );
		}

		if (!itemExists)
		{
			QueueItem<ITEM> newPtr( newItem );
			m_queue.push_back( newPtr );

			m_semaphore.post();
		}
	}

	void insertReplace( boost::shared_ptr<ITEM>& newItem )
	{
		THREAD_GUARD( m_queueLock );

		if ( ! m_queue.empty() )
		{
			typename ItemQueue::iterator itr = m_queue.begin();
			
			while (itr != m_queue.end())
			{
				if (true == itr->isReplacedBy(newItem))
				{
					itr = m_queue.erase(itr);
				}
				else
				{
					++itr;
				}
			}
		}

		QueueItem<ITEM> newPtr( newItem );
		m_queue.push_back( newPtr );

		m_semaphore.post();
	}

	unsigned long getQueueSize()
	{
		return m_queue.size();
	}

public:

	virtual void run() 
	{
		while ( m_keepRunning )
		{
			boost::shared_ptr<ITEM> newEvent = blockForItem();

			if (newEvent.get() != NULL)
			{
				processEvent( newEvent );
			}
		}

        m_keepRunning = true;
	};

	virtual void terminate()
	{
		m_keepRunning = false;
		unBlockQueue();
	};

	virtual void processEvent( boost::shared_ptr<ITEM> newEvent ) = 0;
	
	boost::shared_ptr<ITEM> blockForItem() 
	{
		m_semaphore.wait();

		boost::shared_ptr<ITEM> nextItem ((ITEM*) NULL);

		THREAD_GUARD( m_queueLock );

		if ( m_queue.empty() )
		{
			return nextItem;
		}
		
		nextItem = m_queue.front().getItemPtr();
		m_queue.pop_front();
		return nextItem;
	}

	void unBlockQueue()
	{
		m_semaphore.post();
	}

private:

	ItemQueue										m_queue;
    omni_mutex                                      m_queueLock;
    omni_semaphore                                  m_semaphore;
	unsigned long									m_maxQueueSize;
	bool											m_keepRunning;
	bool											m_logStats;
};


#endif
