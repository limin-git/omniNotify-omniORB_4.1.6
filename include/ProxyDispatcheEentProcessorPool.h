#ifndef ADMINDISPATCHEVENTPROCESSORPOOL_H_INCLUDED
#define ADMINDISPATCHEVENTPROCESSORPOOL_H_INCLUDED


#include "QueueProcessorPool.h"

#define PERFORMANCE_TEST_LOG
#define PROXY_DISPATCH_THREAD_NUMBER 100


#ifdef PERFORMANCE_TEST_LOG
#include "stubs.h"
#include "ThreadTimeStamp.h"
#endif


struct DispatchData
{
    EventChannel_i* channel;
    ConsumerAdmin_i* admin;
    SequenceProxyPushSupplier_i * proxy;
    RDI_TypeMap* tmap;
    RDI_FilterState_t astat;
    RDI_StructuredEvent*  event;
    const char* dname;
    const char* tname;
};


struct ProxyDispatcheEentProcessorPool : public QueueProcessorPoolCallback<DispatchData>
{
public:

    static ProxyDispatcheEentProcessorPool& instance()
    {
        static ProxyDispatcheEentProcessorPool _instance;
        return _instance;
    }

    virtual void queueProcessorPoolCallback( boost::shared_ptr<DispatchData> item )
    {
        RDI_TypeMap::FNode_t* fnode=0;
        RDI_TypeMap::FList_t flist;

        EventChannel_i* _channel = item->channel;
        ConsumerAdmin_i* admin = item->admin;
        SequenceProxyPushSupplier_i * bpush = item->proxy;
        RDI_TypeMap* tmap = item->tmap;
        RDI_FilterState_t astat = item->astat;
        RDI_StructuredEvent*  event = item->event;
        const char* dname = item->dname;
        const char* tname = item->tname;

        AbstractSingleThreadBarrier* barrier = get_barrier( admin);

        if ( NULL == barrier )
        {
            RDI_Fatal("null barrier");
        }

        if ( astat == OrMatch ) {
            bpush->add_event(event);
            barrier->post();
            return;
        }
        if ( ! bpush->has_filters() ) {
            if ( (astat == NoFilters) || (astat == AndMatch) ) {
                bpush->add_event(event);
            }
            barrier->post();
            return;
        }

        tmap->lookupFilter(dname, tname, bpush, flist);
        if ( ! flist._star_star && ! flist._domn_star &&
            ! flist._star_type && ! flist._domn_type ) {
                if ( astat == OrMatch ) {
                    bpush->add_event(event);
                }
                return;
        }
        for ( fnode=flist._star_star; fnode; fnode=fnode->_next ) {
            if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
                bpush->add_event(event);
                continue;
            }
        }
        for ( fnode=flist._domn_star; fnode; fnode=fnode->_next ) {
            if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
                bpush->add_event(event);
                continue;
            }
        }
        for ( fnode=flist._star_type; fnode; fnode=fnode->_next ) {
            if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
                bpush->add_event(event);
                continue;
            }
        }
        for ( fnode=flist._domn_type; fnode; fnode=fnode->_next ) {
            if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
                bpush->add_event(event);
                continue;
            }
        }

        barrier->post();

#ifdef PERFORMANCE_TEST_LOG
#undef PERFORMANCE_TEST_LOG
        RDIDbgCosCPxyLog("Thrd=" << TW_ID() << ", Channel=" << _channel->MyID() << ", queueProcessorPoolCallback - debug"
            << ", initial count=" << barrier->m_initial
            << ", count =" << barrier->m_count
            << "\n");
#endif


    }

    AbstractSingleThreadBarrier* get_barrier( ConsumerAdmin_i* admin )
    {
        AbstractSingleThreadBarrier* barrier = NULL;

        if ( admin != NULL )
        {
            m_lock.acquire();

            std::map<ConsumerAdmin_i*, AbstractSingleThreadBarrier*>::iterator findIt = m_adimin_barrier_map.find( admin );

            if ( findIt != m_adimin_barrier_map.end() )
            {
                barrier = findIt->second;
            }

            m_lock.release();
        }

        return barrier;
    }

    void initialize_barrier( ConsumerAdmin_i* admin, size_t batch_size )
    {
        if ( admin != NULL )
        {
            m_lock.acquire();

            m_adimin_barrier_map[admin] = new AbstractSingleThreadBarrier( admin->NumProxies() * batch_size );

            m_lock.release();
        }
    }

    void remove_barrier( ConsumerAdmin_i* admin )
    {
        if ( admin != NULL )
        {
            m_lock.acquire();

            std::map<ConsumerAdmin_i*, AbstractSingleThreadBarrier*>::iterator findIt = m_adimin_barrier_map.find( admin );

            if ( findIt != m_adimin_barrier_map.end() )
            {
                delete findIt->second;
                findIt->second = NULL;

                m_adimin_barrier_map.erase( findIt );
            }

            m_lock.release();
        }
    }

    void wait_barrier( ConsumerAdmin_i* admin )
    {
        AbstractSingleThreadBarrier* barrier = get_barrier( admin );

        if ( barrier != NULL )
        {
            barrier->wait();
        }

        remove_barrier( admin );
    }

    void queueItem( EventChannel_i* channel, ConsumerAdmin_i* admin, SequenceProxyPushSupplier_i* proxy, RDI_TypeMap* tmap, RDI_FilterState_t astat, RDI_StructuredEvent*  event, const char* dname, const char* tname )
    {
        boost::shared_ptr<DispatchData> item( new DispatchData );

        item->channel = channel;
        item->admin = admin;
        item->proxy = proxy;
        item->tmap = tmap;
        item->astat = astat;
        item->event = event;
        item->dname = dname;
        item->tname = tname;

        m_processor_poll.queueItem( admin->MyID() * PROXY_DISPATCH_THREAD_NUMBER + proxy->MyID(), item );
    }

private:

    ProxyDispatcheEentProcessorPool()
        : m_processor_poll(PROXY_DISPATCH_THREAD_NUMBER, *this, true)
    {
    }

private:

    omni_mutex m_lock; 
    std::map<ConsumerAdmin_i*, AbstractSingleThreadBarrier*> m_adimin_barrier_map;
    QueueProcessorPool<DispatchData, QueueProcessorWorker<DispatchData> > m_processor_poll;
};


#endif
