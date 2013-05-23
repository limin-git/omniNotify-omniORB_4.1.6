#ifndef SWITCHES_H_INCLUDED
#define SWITCHES_H_INCLUDED


#define NO_TYPEMAP_LOG_OUTPUT                           //RDI_TypeMap::log_output
#define NO_SEQUENCE_PROXY_PUSH_SUPPLIER_PUSH_EVENT      //SequenceProxyPushSupplier_i::push_event
#define PERFORMANCE_TEST_LOG
#define DEBUG_THREAD_POOL_QUEUE_SIZE
#define PERFORMANCE_MONITOR
#define NO_GC_ON_EVENT_QUEUE_INSERT                     //RDI_EventQueue::insert
#define GC_WHOLE_EVENT_QUEUE                            //RDI_EventQueue::garbage_collect
#define BATCH_PROXY_DISPATCH                          //EventChannel_i::proxy_dispatch
#define PROXY_DISPATCH_THREAD_POOL                    //EventChannel_i::proxy_dispatch





#ifdef PERFORMANCE_TEST_LOG
    #include "stubs.h"
    #include "ThreadTimeStamp.h"
#endif


#ifdef PROXY_DISPATCH_THREAD_POOL
    #include "ProxyDispatcheEentProcessorPool.h"
#endif

#ifdef PERFORMANCE_MONITOR
    #include "CountPerformanceMonitor.h"
    #include "TimePerformanceMonitor.h"
#endif


#ifdef PROXY_DISPATCH_THREAD_POOL
    #ifndef BATCH_PROXY_DISPATCH
        #error 'proxy dispatch thread pool' need 'bat proxy dispatch'
    #endif
#endif

#ifdef BATCH_PROXY_DISPATCH
    #include <vector>
#endif


#endif
