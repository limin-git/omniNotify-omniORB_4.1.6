#ifndef SWITCHES_H_INCLUDED
#define SWITCHES_H_INCLUDED


//#define NO_TYPEMAP_LOG_OUTPUT                                           //RDI_TypeMap::log_output
//#define NO_SEQUENCE_PROXY_PUSH_SUPPLIER_PUSH_EVENT                      //SequenceProxyPushSupplier_i::push_event
//#define NO_GC_ON_EVENT_QUEUE_INSERT                                     //RDI_EventQueue::insert
//#define NO_ADMIN_DISPATCH                                               //EventChannel_i::admin_dispatch

//#define PERFORMANCE_DEBUG_LOG
//#define PERFORMANCE_REPORT_LOG
//#define GC_WHOLE_EVENT_QUEUE                                            //RDI_EventQueue::garbage_collect
//#define BATCH_PROXY_DISPATCH                                            //EventChannel_i::proxy_dispatch
//#define PROXY_DISPATCH_THREAD_POOL                                      //EventChannel_i::proxy_dispatch
//#define DEBUG_THREAD_POOL_QUEUE_SIZE
//#define DEBUG_THREAD_POOL_BARRIER
//#define TEST_CONSUMERADMIN_DISPATCH_EVENT                               //only dispatch 6 - test dispatch performance

#define USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL                              //CosNotifyChannelAdmin_i.h:EventChannel_i
//#define USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING           //EventChannel_i::update_mapping
//#define USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_DISPATCH_EVENT           //ConsumerAdmin_i::dispatch_event
#define USETA_TYPE_MAPPING_IN_EVENT_CHANNEL_OUT_DEBUG_INFO                //EventChannel_i::out_debug_info



#ifdef PERFORMANCE_DEBUG_LOG
    #include "stubs.h"
    #include "ThreadTimeStamp.h"
#endif


#ifdef PROXY_DISPATCH_THREAD_POOL
    #include "ProxyDispatcheEentProcessorPool.h"
#endif

#ifdef PERFORMANCE_REPORT_LOG
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


#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL
    #include "TA_TypeMap.h"
#endif



#endif
