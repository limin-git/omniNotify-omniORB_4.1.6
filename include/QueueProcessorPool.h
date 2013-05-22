/**
  * The source code in this file is the property of
  * Ripple Systems and is not for redistribution
  * in any form.
  *
  * Source:   $File: //depot/3001_TIP_NEW/TA_BASE/transactive/core/threads/src/QueueProcessorPool.h $
  * @author:  Adam Radics
  * @version: $Revision: #2 $
  *
  * Last modification: $DateTime: 2012/02/06 16:15:14 $
  * Last modified by:  $Author: haijun.li $
  *
  * Kind of like a thread pool. Except items can be unique.
  * Items with the same ID or key are always assigned to the same thread
  * this preserves order of execution.
  * so its a thread pool, where the queue is maintained in each thread rather
  * than one queue for all threads.
  */

#if !defined(QUEUEPROCESSORPOOL_H)
#define QUEUEPROCESSORPOOL_H


#include <vector>
#include "QueueProcessor.h"


template<class ITEM> class QueueProcessorPoolCallback
{
public:

	virtual ~QueueProcessorPoolCallback() {};

	virtual void queueProcessorPoolCallback( boost::shared_ptr<ITEM> item ) = 0;
};


template<class ITEM, class PROCESSORWORKER> class QueueProcessorPool
{
	public:
		
        QueueProcessorPool(int numberOfWorkers, QueueProcessorPoolCallback<ITEM>& callback, bool startWorkers = true)
            : m_numWorkers(numberOfWorkers)
        {
            for (int i = 0; i < m_numWorkers; ++i)
            {
                m_workers.push_back(new PROCESSORWORKER(callback));
            }
		
            if ( true == startWorkers )
            {
                startProcessing();
            }
        }

		virtual ~QueueProcessorPool()
        {
            for ( typename std::vector<PROCESSORWORKER*>::iterator iter = m_workers.begin(); iter != m_workers.end(); ++iter )
            {
                (*iter)->terminate();
            }

            for ( typename std::vector<PROCESSORWORKER*>::iterator iter = m_workers.begin(); iter != m_workers.end(); ++iter )
            {
                (*iter)->terminateAndWait();
                delete (*iter);
            }

            m_workers.clear();
        }
    
        void startProcessing()
        {
			for ( typename std::vector< PROCESSORWORKER* >::iterator iter = m_workers.begin();
                  iter != m_workers.end(); ++iter )
            {
                (*iter)->start();
            }
        }
       
        void stopProcessing()
        {
            for ( typename std::vector<PROCESSORWORKER*>::iterator iter = m_workers.begin();
                  iter != m_workers.end(); ++iter )
            {
                (*iter)->terminateAndWait();
            }
        }
    
        void queueItem( unsigned long itemId, boost::shared_ptr<ITEM>& item )
        {
            if (NULL != item.get())
            {
                int worker = itemId % m_numWorkers;
                m_workers[worker]->insert(item);
            }
        }

		std::vector<unsigned long> getQueueSizes()
		{
			std::vector<unsigned long> sizes;
			typename std::vector<PROCESSORWORKER*>::iterator iter;
            for (iter = m_workers.begin(); iter != m_workers.end(); ++iter)
            {
                sizes.push_back((*iter)->getQueueSize());
            }

			return sizes;
		}

	private:

        int                                         m_numWorkers;
        std::vector< PROCESSORWORKER* >				m_workers;
};

template<class ITEM> class QueueProcessorWorker : public QueueProcessor<ITEM>
{
    public:
		QueueProcessorWorker(QueueProcessorPoolCallback<ITEM>& callback) 
            : m_callback(callback)
        {}

        virtual ~QueueProcessorWorker() {}

    protected:

		void processEvent( boost::shared_ptr<ITEM> newItem )
        {
            m_callback.queueProcessorPoolCallback(newItem);
        }

    private:

        QueueProcessorPoolCallback<ITEM>& m_callback;
};



#endif // !defined(QUEUEPROCESSORPOOL_H)
