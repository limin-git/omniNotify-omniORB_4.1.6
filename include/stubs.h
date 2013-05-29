#ifndef STUB_H_INCLUDED
#define STUB_H_INCLUDED

#include "thread_wrappers.h"
#include "corba_wrappers.h"
#include "CosNotifyShorthands.h"
#include "CosNotifyFilter_i.h"
#include "CosNotification_i.h"
#include "RDIstrstream.h"
#include "RDIOplocks.h"
#include "RDIEvent.h"
#include "RDIEventQueue.h"
#include "RDIChannelUtil.h"
#include "RDINotifQueue.h"
#include "RDITimeWrappers.h"
#include "RDIList.h"
#include "RDILocksHeld.h"
#include "RDISeqDecls.h"

class RDI_TypeMap;
class EventChannelFactory_i;


#include "static_check.h"



class EventChannel_i_stub : WRAPPED_SKELETON_SUPER(AttNotification, EventChannel) 
{
    friend class EventChannelFactory_i;
public:
    EventChannel_i_stub(EventChannelFactory_i*, FilterFactory_i*, const CosN::QoSProperties&, const CosN::AdminProperties&, RDI_ServerQoS*, const CosNA::ChannelID&);
    char* do_command(const char*, CORBA::Boolean&, CORBA::Boolean&, AttN_Interactive_outarg WRAPPED_DECLARG);
    AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
    AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
    AttN::IactSeq* children(CORBA::Boolean WRAPPED_DECLARG );
    CORBA::Boolean safe_cleanup( WRAPPED_DECLARG_VOID );

    // Methods from CosEvCA::EventChannel Interface
    CosEvCA::ConsumerAdmin_ptr for_consumers( WRAPPED_DECLARG_VOID );
    CosEvCA::SupplierAdmin_ptr for_suppliers( WRAPPED_DECLARG_VOID );
    void destroy( WRAPPED_DECLARG_VOID );

    // Methods from CosNA::EventChannel Interface
    CosNA::EventChannelFactory_ptr MyFactory( WRAPPED_DECLARG_VOID );
#if 0 // LIMIN TODO: remove none necessary functions
    CosNA::ConsumerAdmin_ptr default_consumer_admin( WRAPPED_DECLARG_VOID );
    CosNA::SupplierAdmin_ptr default_supplier_admin( WRAPPED_DECLARG_VOID );
    CosNF::FilterFactory_ptr default_filter_factory( WRAPPED_DECLARG_VOID );
    CosNA::ConsumerAdmin_ptr new_for_consumers( CosNA::InterFilterGroupOperator, CosNA::AdminID& WRAPPED_DECLARG );
    CosNA::SupplierAdmin_ptr new_for_suppliers( CosNA::InterFilterGroupOperator, CosNA::AdminID& id WRAPPED_DECLARG );
    CosNA::ConsumerAdmin_ptr get_consumeradmin(CosNA::AdminID id  WRAPPED_DECLARG );
    CosNA::SupplierAdmin_ptr get_supplieradmin(CosNA::AdminID id  WRAPPED_DECLARG );
    CosNA::AdminIDSeq *      get_all_consumeradmins( WRAPPED_DECLARG_VOID );
    CosNA::AdminIDSeq *      get_all_supplieradmins( WRAPPED_DECLARG_VOID );
#endif
    // Methods from CosN::AdminPropertiesAdmin Interface
    CosN::AdminProperties* get_admin( WRAPPED_DECLARG_VOID );
    void set_admin(const CosN::AdminProperties& WRAPPED_DECLARG );

    // Methods from CosN::QoSAdmin Interface
    CosN::QoSProperties* get_qos( WRAPPED_DECLARG_VOID );
    void set_qos(const CosN::QoSProperties& WRAPPED_DECLARG );
    void validate_qos(const CosN::QoSProperties&, CosN_NamedPropertyRangeSeq_outarg WRAPPED_DECLARG );

    // Additional methods from AttNotification::EventChannel
    CosN::EventTypeSeq* obtain_offered_types( WRAPPED_DECLARG_VOID );
    CosN::EventTypeSeq* obtain_subscription_types( WRAPPED_DECLARG_VOID );
    AttN::ChannelStats  obtain_stats( WRAPPED_DECLARG_VOID );

    // ----------------------------------------------------------------------------------
    // (Local only -- not available via RPC)
#if 0 // LIMIN TODO: remove none necessary functions

    void server_qos_changed();
    RDI_NotifQoS*   qos_properties() 	{ return _qosprop; }
    CosNA::ChannelID MyID()		{ return _serial;  }

    // An event is sent to the channel by some connected supplier

    int new_structured_event(RDI_StructuredEvent* event);
    int new_any_event(const CORBA::Any& event);
    int new_structured_event(const CosN::StructuredEvent& event);

    // Increment/decrement the number of consumers and suppliers

    CORBA::Boolean incr_consumers();
    void           decr_consumers();
    CORBA::Boolean incr_suppliers();
    void decr_suppliers();

    // Retrieve administrative properties of the channel. We do not
    // acquire a lock since we assume that they do not change often
    // and, in addition, a snapshop is sufficient

    CORBA::Long   max_consumers() const	{ return _admin_qos.maxConsumers;    }
    CORBA::Long   max_suppliers() const	{ return _admin_qos.maxSuppliers;    }
    CORBA::UShort push_threads() const 	{ return _server_qos->numPushThreads;  }
    CORBA::UShort pull_threads() const 	{ return _server_qos->numPullThreads;  }
    CORBA::UShort pull_period() const 	{ return _server_qos->pullEventPeriod; }
    CORBA::UShort ochange_threads() const	{ return _server_qos->numOChangeThreads;  }
    CORBA::UShort schange_threads() const	{ return _server_qos->numSChangeThreads;  }

    void pull_period_s_n(unsigned long &s, unsigned long &n) const
    {
        unsigned long msec = (unsigned long)_server_qos->pullEventPeriod;
        s = msec / 1000;
        n = (msec % 1000) * 1000000;
    }
    // Unregister Admin objects

    void unregister(RDI_LocksHeld& held, SupplierAdmin_i* suplAdmin);
    void unregister(RDI_LocksHeld& held, ConsumerAdmin_i* consAdmin);

    CORBA::Boolean update_mapping(RDI_LocksHeld&, const CosN::EventTypeSeq&, const CosN::EventTypeSeq&, ConsumerAdmin_i*, Filter_i*);

    CORBA::Boolean update_mapping(RDI_LocksHeld&, const CosN::EventTypeSeq&, const CosN::EventTypeSeq&, RDIProxySupplier*, Filter_i*);

#endif
    struct ProxyDispatch_t
    {
        RDI_StructuredEvent* _event;
        ConsumerAdmin_i*     _admin;
        RDI_FilterState_t    _state;

        ProxyDispatch_t(RDI_StructuredEvent* e=0, ConsumerAdmin_i* a=0, RDI_FilterState_t s=NoFilters)
            :  _event(e), _admin(a), _state(s) {;}
        ProxyDispatch_t(const ProxyDispatch_t& p)
            : _event(p._event), _admin(p._admin), _state(p._state) {;}
        ProxyDispatch_t& operator= (const ProxyDispatch_t& p)
        {
            _event=p._event; _admin=p._admin; 
            _state=p._state; return *this;
        }
    };

#if 0 // LIMIN TODO: remove none necessary functions
    // Update the hash table that keeps information about the event  //
    // types supplied by suppliers.  When a new entry is created or  //
    // an existing entry is deleted, insert the delta into the       //
    // _ochange_pool, causing _offer_change msgs to be sent          //

    void propagate_ochange(RDI_LocksHeld&, const CosN::EventTypeSeq&, const CosN::EventTypeSeq&);

    // There have been some changes in the event types referenced in //
    // consumers filters. Insert the delta into the _schange_pool,   //
    // causing _subscription_change msgs to be sent                  //
    // [This method is invoked by the RDI_TypeMap module]            //

    void propagate_schange(RDI_LocksHeld&             held,
        const CosN::EventTypeSeq&  added,
        const CosN::EventTypeSeq&  deled);

    // These versions retrieve the sequence of event types supplied by suppliers and
    // the sequence of event types referenced in all consumer filters 
    CosN::EventTypeSeq* pxy_obtain_offered_types(RDIProxySupplier* pxy, CosNA::ObtainInfoMode mode);
    CosN::EventTypeSeq* pxy_obtain_subscription_types(RDIProxyConsumer* pxy, CosNA::ObtainInfoMode mode);

    // for stats
    inline void incr_num_rdi_match();
    inline void incr_num_rvm_eval();
    inline void incr_num_announcements();
    inline void incr_num_notifications(unsigned int qsize);
    void        dump_stats(RDI_LocksHeld& held, CORBA::Boolean force);
    void        _rpt_stats(RDIstrstream& str); // called by dump_stats

    // From time to time we may need to debug the event queue
    void out_queue_debug_info(RDIstrstream& str, CORBA::Boolean showEvents=0) { _events->out_debug_info(str, showEvents); }

    RDIstrstream& log_output(RDIstrstream& str, CORBA::Boolean show_events = 0);
    RDIstrstream& log_output_config(RDIstrstream& str);

    // Local-only functions
    CORBA::Boolean do_go_command(RDIstrstream& str, RDIParseCmd& p,
        CORBA::Boolean& target_changed,
        AttN_Interactive_outarg next_target);
    CORBA::Boolean do_set_command(RDIstrstream& str, RDIParseCmd& p);
    void out_commands(RDIstrstream& str);
    void out_heading(RDIstrstream& str);
    void out_stats(RDIstrstream& str);
    void out_debug_info(RDIstrstream& str, CORBA::Boolean show_events = 0);
    void out_config(RDIstrstream& str);
    void out_info_filters(RDIstrstream& str, CORBA::Boolean admins, CORBA::Boolean proxies);
    void cleanup(RDIstrstream& str, CORBA::Boolean admins, CORBA::Boolean proxies);

    const AttN::NameSeq& L_my_name() { return _my_name; }

    RDI_PullSupplier*      pull_supplier() { if (_shutmedown) return 0; return _pull_supplier; }
    RDI_NotifyConsumer*    push_consumer() { if (_shutmedown) return 0; return _push_consumer; }
    RDI_ChangePool*        ochange_pool()  { if (_shutmedown) return 0; return _ochange_pool; }
    RDI_ChangePool*        schange_pool()  { if (_shutmedown) return 0; return _schange_pool; }

    CORBA::Boolean shutting_down() { return _shutmedown; }

    // N.B. only the macros in RDIOplocksMacros.h should call this function
    RDIOplockEntry** macro_get_oplockptr_ptr() { return &_oplockptr; }
    RDI_TypeMap*     macro_get_typemap_ptr()   { return _type_map; }
#endif

public:
    RDIOplockEntry*        _oplockptr;
#ifndef NO_OBJ_GC
    RDI_TimeT	         _last_use;
    TW_Thread*             _gcollector;
    TW_Condition*          _gc_wait;
    TW_Condition*          _gc_exit;
    CORBA::Boolean         _gcisactive;
    CORBA::ULong           _objGCPeriod;
#endif
    AttN::EventChannel_var _my_oref;
    AttN::NameSeq          _my_name;
    EventChannelFactory_i* _my_channel_factory;
    ConsumerAdmin_i*       _def_consumer_admin;
    SupplierAdmin_i*       _def_supplier_admin;
    FilterFactory_i*       _def_filter_factory;
    RDI_NotifQoS*          _qosprop;
    RDI_TypeMap*           _type_map;
    RDI_AdminQoS           _admin_qos;
    RDI_ServerQoS*         _server_qos;
    CORBA::ULong           _serial;
    CORBA::ULong           _admin_serial;
    CORBA::ULong           _num_consadmin;
    CORBA::ULong           _num_suppadmin;
    CORBA::ULong           _num_consumers;
    CORBA::ULong           _num_suppliers;

    TW_Mutex               _proxy_lock;   // Proxy event queue mutex
    TW_Condition           _proxy_empty;  // Proxy event queue condition

    // Event filtering and dispatching thread pool
    EventChannelDispatch*  _rdi_dispatch;

    // Global thread responsible for channel maintainance and cleanup.
    // The conditional variable is used to signal channel destruction,
    // as well as implement perioding garbage collection.
    // When 'destroy' is called on the channel,  '_shutmedown' becomes
    // non zero. 
    // When the garbage collection thread exits, '_gcisactive' becomes
    // zero.

    CORBA::Boolean         _shutmedown;

    TW_Thread*             _reporter;
    TW_Condition*          _rep_wait;
    TW_Condition*          _rep_exit;
    CORBA::Boolean         _repisactive;
    CORBA::ULong           _repInterval;

    // Lists of announced events and entries for proxy dispatching
    RDI_EventQueue*        _events;

    TW_Mutex               _qos_lock;
    TW_Mutex               _stats_lock;
    RDI_Watch              _performance_timer;
    RDI_ThStat*            _thread_stats;

    CORBA::ULong           _gq_acm;
    CORBA::ULong           _gq_ctr;
    CORBA::ULong           _pq_acm;
    CORBA::ULong           _pq_ctr;

    CORBA::ULong           _prev_num_rdi_match;
    CORBA::ULong           _prev_num_rvm_eval;
    CORBA::ULong           _prev_num_announcements;
    CORBA::ULong           _prev_num_notifications;

    CORBA::ULong           _stat_update_counter;
    CORBA::ULong           _stat_delta_target;

    CORBA::LongLong        _cum_msecs;
    CORBA::Boolean         _second_delta;
    // sleep this many nanosecs on each global event queue insert
    // (starts zero, adjusted according to average notif queue size) 
    unsigned long          _gq_sleep_nanosecs;
    double                 _prev_avg_nq_sz;

    RDI_List<ProxyDispatch_t> _proxy_events;

    // The following is the group manager for ConsumerAdmin_i objects
    // and it is used by the threads that perfrom filtering on behalf
    // of ConsumerAdmin_i objects

    CAdminGroupMgr*           _admin_group;

    // Hash tables for the various admin objects

    RDI_Hash<CosNA::AdminID, SupplierAdmin_i *> _supl_admin;
    RDI_Hash<CosNA::AdminID, ConsumerAdmin_i *> _cons_admin;

    // To support offer_change() we need a hash table of event types

    RDI_Hash<CosN::EventType, CORBA::ULong> _evtypes;

    // The thread pools used for pulling event from Pull suppliers
    // and pushing events to push consumers -- they may be NULL

    RDI_PullSupplier*      _pull_supplier;
    RDI_NotifyConsumer*    _push_consumer;

    // The thread pools used for sending offer_change and subscription_change
    // messages -- they may be NULL
    RDI_ChangePool*       _ochange_pool;
    RDI_ChangePool*       _schange_pool;

    // Do actual work of adding event to global event queue
    // returns -1 on failure, else 0
    int _new_structured_event(RDI_StructuredEvent* event);

    // Carry out event filtering and dispatching for ConsumerAdmin 
    // and ProxySupplier objects

    void  admin_dispatch();
    void  proxy_dispatch();

    // Does one of the filters, if any, of a given ConsumerAdmin_i
    // object match an announced event having the provided type?

    CORBA::Boolean match_event(ConsumerAdmin_i*      admin,
        RDI_StructuredEvent*  event,
        RDI_FilterState_t&    fstate);

#ifndef NO_OBJ_GC
    void  gcollect();
#endif
    void  periodic_report();
    void _children(AttN::IactSeq& ren, CORBA::Boolean only_cleanup_candidates);

    virtual ~EventChannel_i_stub();
};


//static void make_sure_same_size()
//{
//   LOKI_STATIC_CHECK( ( sizeof( EventChannel_i_stub ) == sizeof(EventChannel_i) ), same_size );
//}




class ConsumerAdmin_i_stub :	 
    public RDINotifySubscribe,
    WRAPPED_SKELETON_SUPER(AttNotification, ConsumerAdmin) 
{
    friend class EventChannel_i;
public:
    ConsumerAdmin_i_stub(EventChannel_i* channel, CosNA::InterFilterGroupOperator op, 
        const CosNA::AdminID& serial);

    // Methods from AttNotification::Interactive Interface
    char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
        AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
    AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
    AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
    AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates  WRAPPED_DECLARG );
    CORBA::Boolean safe_cleanup( WRAPPED_DECLARG_VOID );

    // Methods from CosEvCA::ConsumerAdmin Interface
    CosEvCA::ProxyPushSupplier_ptr obtain_push_supplier( WRAPPED_DECLARG_VOID );
    CosEvCA::ProxyPullSupplier_ptr obtain_pull_supplier( WRAPPED_DECLARG_VOID );

    // Methods from CosNC::ConsumerAdmin Interface
    CosNA::AdminID                  MyID( WRAPPED_DECLARG_VOID );
    CosNA::EventChannel_ptr         MyChannel( WRAPPED_DECLARG_VOID );	
    CosNA::InterFilterGroupOperator MyOperator( WRAPPED_DECLARG_VOID );
    CosNF::MappingFilter_ptr priority_filter( WRAPPED_DECLARG_VOID );
    void  priority_filter(CosNF::MappingFilter_ptr fltr WRAPPED_DECLARG );
    CosNF::MappingFilter_ptr lifetime_filter( WRAPPED_DECLARG_VOID );
    void  lifetime_filter(CosNF::MappingFilter_ptr fltr WRAPPED_DECLARG );
    CosNA::ProxyIDSeq*       pull_suppliers( WRAPPED_DECLARG_VOID );
    CosNA::ProxyIDSeq*       push_suppliers( WRAPPED_DECLARG_VOID );
    CosNA::ProxySupplier_ptr get_proxy_supplier(CosNA::ProxyID proxy_id 
        WRAPPED_DECLARG );
    CosNA::ProxySupplier_ptr obtain_notification_pull_supplier(
        CosNA::ClientType ctype, 
        CosNA::ProxyID&   proxy_id WRAPPED_DECLARG );
    CosNA::ProxySupplier_ptr obtain_notification_push_supplier(
        CosNA::ClientType ctype, 
        CosNA::ProxyID&   proxy_id WRAPPED_DECLARG );
    void destroy( WRAPPED_DECLARG_VOID );

    // Methods from CosN::QoSAdmin Interface
    CosN::QoSProperties* get_qos( WRAPPED_DECLARG_VOID );
    void set_qos(const CosN::QoSProperties& qos WRAPPED_DECLARG );
    void validate_qos(const CosN::QoSProperties& r_qos,
        CosN_NamedPropertyRangeSeq_outarg a_qos
        WRAPPED_DECLARG );

    // Methods from CosNF::FilterAdmin Interface
    CosNF::FilterID     add_filter(CosNF::Filter_ptr fltr WRAPPED_DECLARG );
    void               remove_filter(CosNF::FilterID fltrID WRAPPED_DECLARG );
    CosNF::Filter_ptr   get_filter(CosNF::FilterID fltrID WRAPPED_DECLARG );
    CosNF::FilterIDSeq* get_all_filters( WRAPPED_DECLARG_VOID );
    void               remove_all_filters( WRAPPED_DECLARG_VOID );

    // Methods from CosNC::NotifySubscribe Interface
    void subscription_change(const CosN::EventTypeSeq& added,
        const CosN::EventTypeSeq& deled WRAPPED_DECLARG );

    // (Local only -- not available via RPC)

    CORBA::Boolean has_filters() const { return (_rqstypes.length() ? 1 : _fa_helper.has_filters()); }
    //  CORBA::Boolean has_filters() const { return _fa_helper.has_filters(); }

    // RDINotifySubscribe methods:
    void filter_destroy_i(Filter_i* filter);
    void propagate_schange(RDI_LocksHeld&             held,
        const CosN::EventTypeSeq&  added,
        const CosN::EventTypeSeq&  deled,
        Filter_i*                  filter);

    // Proxy de-registration members
    void remove_proxy(RDI_LocksHeld& held, ProxyPushSupplier_i* prx);
    void remove_proxy(RDI_LocksHeld& held, ProxyPullSupplier_i* prx);
    void remove_proxy(RDI_LocksHeld& held, EventProxyPushSupplier_i* prx);
    void remove_proxy(RDI_LocksHeld& held, EventProxyPullSupplier_i* prx);
    void remove_proxy(RDI_LocksHeld& held, StructuredProxyPushSupplier_i* prx);
    void remove_proxy(RDI_LocksHeld& held, StructuredProxyPullSupplier_i* prx);
    void remove_proxy(RDI_LocksHeld& held, SequenceProxyPushSupplier_i* prx);
    void remove_proxy(RDI_LocksHeld& held, SequenceProxyPullSupplier_i* prx);

    CORBA::ULong         NumProxies() const	{ return _num_proxies; }
    RDI_NotifQoS*        qos_properties()		{ return _qosprop; }

    CORBA::Boolean       gc_able(CORBA::ULong DeadAdminInterval);

    // these methods are called when the public methods cannot be used
    // (the public methods obtain OPLOCK)
    CosNA::AdminID                  _admin_id()       { return _serial; }
    CosNA::InterFilterGroupOperator _admin_operator() { return _and_or_oper; }

    // Local-only functions

    RDIstrstream& log_output(RDIstrstream& str);
    CORBA::Boolean do_go_command(RDIstrstream& str, RDIParseCmd& p,
        CORBA::Boolean& target_changed,
        AttN_Interactive_outarg next_target);
    CORBA::Boolean do_set_command(RDIstrstream& str, RDIParseCmd& p);
    void out_config(RDIstrstream& str);
    void out_commands(RDIstrstream& str);
    void out_info_filters(RDIstrstream& str, CORBA::Boolean admin, CORBA::Boolean proxies);
    void cleanup(RDIstrstream& str, CORBA::Boolean admin, CORBA::Boolean proxies);
#ifndef NO_OBJ_GC
    CORBA::Boolean obj_gc(RDI_TimeT curtime, CORBA::ULong deadAdmin, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy);
    CORBA::Boolean gc_able(RDI_TimeT curtime, CORBA::ULong deadAmin);
#endif

    const AttN::NameSeq& L_my_name() { return _my_name; }

    // N.B. only the macros in RDIOplocksMacros.h should call this function
    RDIOplockEntry** macro_get_oplockptr_ptr() { return &_oplockptr; }

public:
    RDIOplockEntry*                 _oplockptr;
#ifndef NO_OBJ_GC
    RDI_TimeT	                  _last_use;
#endif
    AttN::ConsumerAdmin_var         _my_oref;
    AttN::NameSeq                   _my_name;
    // _disposed is true once _disconnect_clients_and_dispose has been called
    CORBA::Boolean                  _disposed;
    FAdminHelper     	          _fa_helper;
    EventChannel_i*                 _channel;
    RDI_NotifQoS*                   _qosprop;
    CosNA::AdminID                  _serial;
    CosNA::InterFilterGroupOperator _and_or_oper;
    CosN::EventTypeSeq              _rqstypes;
    CosNF::MappingFilter_var        _prio_filter;	// Priority mapping filter
    CosNF::MappingFilter_var        _life_filter;	// Lifetime mapping filter
    CosNA::ProxyID                  _prx_serial;	// Factory for proxy IDs
    CORBA::ULong                    _num_proxies;	// Number of active proxies

    // Since the Event Service proxy objects do not have a unique ID
    // associated with them, we keep these proxies in a regular list

    RDI_List<EventProxyPushSupplier_i *> _cosevent_push;
    RDI_List<EventProxyPullSupplier_i *> _cosevent_pull;

    // All Notification Service proxy objects have unique IDs. Thus, 
    // we maintain them using hash tables for fast lookups 

    RDI_Hash<CosNA::ProxyID, ProxyPushSupplier_i *>           _prx_any_push;
    RDI_Hash<CosNA::ProxyID, ProxyPullSupplier_i *>           _prx_any_pull;
    RDI_Hash<CosNA::ProxyID, StructuredProxyPushSupplier_i *> _prx_struc_push;
    RDI_Hash<CosNA::ProxyID, StructuredProxyPullSupplier_i *> _prx_struc_pull;
    RDI_Hash<CosNA::ProxyID, SequenceProxyPushSupplier_i *>   _prx_batch_push;
    RDI_Hash<CosNA::ProxyID, SequenceProxyPullSupplier_i *>   _prx_batch_pull;

    // Dispatch an event to all consumers using the CORBA Event Service
    // interface -- no filtering is performed here


    void dispatch_event(RDI_StructuredEvent*  event);

    // Dispatch an event to all consumers using the CORBA Notification
    // Service interface -- filtering is performed here

    void dispatch_event(RDI_StructuredEvent*  event, 
        RDI_FilterState_t     fltstat,
        RDI_TypeMap*          typemap);

    void disconnect_clients_and_dispose(CORBA::Boolean fast_destroy);

    // The real implementation (caller must have bumped scope lock acquiring oplock):
    void _disconnect_clients_and_dispose(RDI_LocksHeld&            held,
        CORBA::Boolean            fast_destroy,
        CORBA::Boolean            update_channel,
        WRAPPED_DISPOSEINFO_PTR&  dispose_info);

    void _children(AttN::IactSeq& ren, CORBA::Boolean only_cleanup_candidates);
    void _qos_changed(RDI_LocksHeld& held);
    void _removed_push_proxy(RDIProxyPushSupplier* proxy);
    void _removed_pull_proxy();

    virtual ~ConsumerAdmin_i_stub();
};


#endif

