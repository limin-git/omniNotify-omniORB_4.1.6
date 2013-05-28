#ifndef PROXY_DISPATCHE_EENT_PROCESSOR_POOL_H_INCLUDED
#define PROXY_DISPATCHE_EENT_PROCESSOR_POOL_H_INCLUDED

#include "RDITypeMap.h"
#include "QueueProcessorPool.h"
#include "ThreadGuard.h"
#include "SingleThreadBarrier.h"
#include <sstream>


#include "Switchecs.h"
#define PROXY_DISPATCH_THREAD_NUMBER 301



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
#ifdef DEBUG_THREAD_POOL_QUEUE_SIZE
    ,public Thread
#endif
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

        struct BarrierGuard
        {
            BarrierGuard( SingleThreadBarrier* barrier ) : m_barrier( barrier ) {;}

            ~BarrierGuard()
            {
                if ( NULL == m_barrier ) { RDI_Fatal("null barrier"); }
                m_barrier->post();
            }

            SingleThreadBarrier* m_barrier;
        };

        BarrierGuard barrier_buard( get_barrier( admin) );

        if ( astat == OrMatch ) {
            bpush->add_event(event);
            return;
        }
        if ( ! bpush->has_filters() ) {
            if ( (astat == NoFilters) || (astat == AndMatch) ) {
                bpush->add_event(event);
            }
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

#ifdef PERFORMANCE_DEBUG_LOG
        RDIDbgCosCPxyLog("Thrd=" << TW_ID() << ", Channel=" << _channel->MyID() << ", queueProcessorPoolCallback - debug"
            << ", initial count=" << barrier_buard.m_barrier->m_initial
            << ", count =" << barrier_buard.m_barrier->m_count
            << "\n");
#endif
    }

    void initialize_barrier( ConsumerAdmin_i* admin, size_t batch_size )
    {
        if ( admin != NULL )
        {
            size_t count = admin->NumProxies() * batch_size;

            if ( count )
            {
                THREAD_GUARD( m_lock );

                m_adimin_barrier_map[admin] = new SingleThreadBarrier( admin->NumProxies() * batch_size );
            }
        }
    }

    void wait_barrier( ConsumerAdmin_i* admin )
    {
        SingleThreadBarrier* barrier = get_barrier( admin );

        if ( barrier != NULL )
        {
            barrier->wait();
        }

        remove_barrier( admin );
    }


    void remove_barrier( ConsumerAdmin_i* admin )
    {
        if ( admin != NULL )
        {
            THREAD_GUARD( m_lock );

            std::map<ConsumerAdmin_i*, SingleThreadBarrier*>::iterator findIt = m_adimin_barrier_map.find( admin );

            if ( findIt != m_adimin_barrier_map.end() )
            {
                delete findIt->second;
                findIt->second = NULL;

                m_adimin_barrier_map.erase( findIt );
            }
        }
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

#ifdef DEBUG_THREAD_POOL_BARRIER
        RDIDbgForceLog( "ProxyDispatcheEentProcessorPool::queueItem - item id " << admin->_admin_id() * PROXY_DISPATCH_THREAD_NUMBER + proxy->_proxy_id() << "\n" );
#endif
        m_processor_poll.queueItem( admin->_admin_id() * PROXY_DISPATCH_THREAD_NUMBER + proxy->_proxy_id(), item );
    }

    SingleThreadBarrier* get_barrier( ConsumerAdmin_i* admin )
    {
        SingleThreadBarrier* barrier = NULL;

        if ( admin != NULL )
        {
            THREAD_GUARD( m_lock );

            std::map<ConsumerAdmin_i*, SingleThreadBarrier*>::iterator findIt = m_adimin_barrier_map.find( admin );

            if ( findIt != m_adimin_barrier_map.end() )
            {
                barrier = findIt->second;
            }
        }

        return barrier;
    }

private:

    ProxyDispatcheEentProcessorPool()
        : m_processor_poll(PROXY_DISPATCH_THREAD_NUMBER, *this, true)
    {
#ifdef DEBUG_THREAD_POOL_QUEUE_SIZE
        start();
#endif
    }

#ifdef DEBUG_THREAD_POOL_QUEUE_SIZE
    virtual void run()
    {
        while ( true )
        {
            std::vector<unsigned long> queue_size = m_processor_poll.getQueueSizes();

            unsigned long activate_thread_number = 0;
            unsigned long total_queue_size = 0;
            unsigned long last_value = queue_size[0];
            unsigned long same_value_number = 1;

            std::stringstream queue_sizes_strm;
            queue_sizes_strm << queue_size[0];

            for ( size_t i = 1; i < queue_size.size(); ++i )
            {
                total_queue_size += queue_size[i];

                if ( queue_size[i] != last_value  )
                {
                    if ( 1 < same_value_number )
                    {
                        queue_sizes_strm << "[" << same_value_number << "]";
                    }

                    queue_sizes_strm << ", " << queue_size[i];

                    last_value = queue_size[i];
                    same_value_number = 1;
                }
                else
                {
                    same_value_number++;
                }

                if ( 0 < queue_size[i] )
                {
                    activate_thread_number++;
                }
            }

            queue_sizes_strm << "[" << same_value_number << "]";

            std::stringstream queue_sizes_output_strm;
            queue_sizes_output_strm
                << ", activate_thread_number=" << activate_thread_number
                << ", queue_sizes=" << total_queue_size
                << "{" << queue_sizes_strm.rdbuf() << "}";

            std::stringstream admin_strm;

            {
                THREAD_GUARD( m_lock );

                for ( std::map<ConsumerAdmin_i*, SingleThreadBarrier*>::iterator it = m_adimin_barrier_map.begin(); it != m_adimin_barrier_map.end(); ++it )
                {
                    ConsumerAdmin_i* admin = it->first;

                    if ( admin != NULL )
                    {
                        admin_strm
                            << ", admin[" << admin << "]{"
                            << "proxy_number=" << admin->NumProxies()
                            << ", event_queue=" << reinterpret_cast<EventChannel_i_stub*>( reinterpret_cast<ConsumerAdmin_i_stub*>(admin)->_channel )->_events->length()
                            << ", proxy_queue=" << reinterpret_cast<EventChannel_i_stub*>( reinterpret_cast<ConsumerAdmin_i_stub*>(admin)->_channel )->_proxy_events.length()
                            << "}";
                    }
                }
            }

            RDIDbgForceLog( "ProxyDispatcheEentProcessorPool - "
                << admin_strm.str().c_str()
                << queue_sizes_output_strm.str().c_str()
                << " \n" );

            Thread::sleep( 1000 );
        }
    }

    virtual void terminate() {;}
#endif

private:

    omni_mutex m_lock; 
    std::map<ConsumerAdmin_i*, SingleThreadBarrier*> m_adimin_barrier_map;
    QueueProcessorPool<DispatchData, QueueProcessorWorker<DispatchData> > m_processor_poll;
};


#endif
