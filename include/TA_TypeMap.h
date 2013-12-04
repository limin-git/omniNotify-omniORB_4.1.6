#ifndef TA_TYPE_MAP_H_INCLUDED
#define TA_TYPE_MAP_H_INCLUDED

#include "CosNotifyShorthands.h"
#include "RDILocksHeld.h"
#include "RDIHash.h"
#include <sstream>
#include <map>
#include <set>

class SequenceProxyPushSupplier_i;
class RDI_StructuredEvent;
class RDI_TypeMap;
class RDIProxySupplier;
class Filter_i;
class EventChannel_i;
class RDIstrstream;
class ConsumerAdmin_i;


class TA_TypeMap
{
public:

    typedef std::set<RDIProxySupplier*> ProxySupplierList;
    typedef std::map<unsigned long, ProxySupplierList> LocationKey2ProxySupplierListMap;
    typedef std::map<std::string, LocationKey2ProxySupplierListMap> Domain2LocationKey2ProxySupplierListMap;

public:

    TA_TypeMap();
    ~TA_TypeMap();
    void initialize( EventChannel_i* channel, RDI_TypeMap*& original_type_map );
    bool ta_update( RDI_LocksHeld& held, const CosN::EventTypeSeq& added, const CosN::EventTypeSeq& deled, RDIProxySupplier* proxy, Filter_i* filter );
    void consumer_admin_dispatch_event(RDI_StructuredEvent*  event, ConsumerAdmin_i* cadmin);
    RDIstrstream& log_output(RDIstrstream& str);

    RDI_Hash<CosNA::ProxyID, SequenceProxyPushSupplier_i *>* get_prx_batch_push();

private:

    void update_prx_batch_push( RDIProxySupplier* proxy );

    static int remove_proxy( LocationKey2ProxySupplierListMap& location_key_2_proxy_list_map, RDIProxySupplier* proxy );
    static int remove_proxy( Domain2LocationKey2ProxySupplierListMap& domain_2_location_key_2_proxy_list_map, RDIProxySupplier* proxy, const char* domain_name );
    static void remove_proxy( LocationKey2ProxySupplierListMap& location_key_2_proxy_list_map, const ProxySupplierList& proxy_list );
    static void remove_proxy( Domain2LocationKey2ProxySupplierListMap& domain_2_location_key_2_proxy_list_map, const ProxySupplierList& proxy_list, const char* domain_name );

    static int get_location_key_from_filter( Filter_i* filter );
    static int get_location_key_from_filter_constraint_expr( const char* constraint_expr );  // ( $Region == '123' )
    static int get_location_key_from_event( RDI_StructuredEvent* event );
    static void get_filter_str( Filter_i* filter, std::ostream& strm );
    static void get_event_type_str( const CosN::EventType& event_type, std::ostream& strm );
    static void get_event_type_list_str( const CosN::EventTypeSeq& event_type_list, std::ostream& strm );

    void check();

public:

    EventChannel_i*                                             m_channel;
    RDI_TypeMap*                                                m_type_map_1; // original type map, DO NOT propagate subscription change
    RDI_TypeMap*                                                m_type_map_2; // DO propagate subscription change
    TW_Mutex                                                    m_lock;
    LocationKey2ProxySupplierListMap                            m_location_key_2_proxy_list_map;
    Domain2LocationKey2ProxySupplierListMap                     m_domain_2_location_key_2_proxy_list_map;
    RDI_Hash<CosNA::ProxyID, SequenceProxyPushSupplier_i *>     _prx_batch_push;
    RDI_Hash<CosNA::ProxyID, SequenceProxyPushSupplier_i *>     _prx_batch_push_2;
    bool m_is_prx_batch_push_changed;

    std::map<RDIProxySupplier*, CosNA::ProxyID> m_proxy_id_map;
};

inline RDIstrstream& operator<< (RDIstrstream& str, TA_TypeMap& map) { return map.log_output(str); }

#endif
