#ifndef TA_RDI_TYPE_MAP_H_INCLUDED
#define TA_RDI_TYPE_MAP_H_INCLUDED

#include "CosNotifyShorthands.h"
#include "RDILocksHeld.h"
#include "RDIList.h"
#include "RDIHash.h"
#include <sstream>
//#include <map>
//#include <set>

class SequenceProxyPushSupplier_i;
class RDI_StructuredEvent;
class RDI_TypeMap;
class RDIProxySupplier;
class Filter_i;
class EventChannel_i;
class RDIstrstream;


class TA_RDITypeMap
{
public:

    TA_RDITypeMap();
    ~TA_RDITypeMap();
    void initialize( EventChannel_i* channel, RDI_TypeMap*& original_type_map );
    bool ta_update( RDI_LocksHeld& held, const CosN::EventTypeSeq& added, const CosN::EventTypeSeq& deled, RDIProxySupplier* proxy, Filter_i* filter );
    void consumer_admin_dispatch_event(RDI_StructuredEvent*  event);
    RDIstrstream& log_output(RDIstrstream& str);

private:

    static CORBA::String_member extract_location_key_from_filter( Filter_i* filter );
    static CORBA::String_member extract_location_key_from_filter_constraint_expr( const char* constraint_expr );  // ( $Region == '123' )
#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
    static void get_filter_str( Filter_i* filter, std::ostream& strm );
    static void get_event_type_str( const CosN::EventType& event_type, std::ostream& strm );
    static void get_event_type_list_str( const CosN::EventTypeSeq& event_type_list, std::ostream& strm );
#endif

public:

    typedef RDI_List<SequenceProxyPushSupplier_i*> ProxySupplierList;
    typedef RDI_Hash<CORBA::String_member, ProxySupplierList*> LocationKey2ProxySupplierListMap;
    typedef RDI_Hash<CORBA::String_member, LocationKey2ProxySupplierListMap*> Domain2LocationKey2ProxySupplierListMap;

    EventChannel_i*                         m_channel;
    RDI_TypeMap*                            m_type_map_1; // original type map, DO NOT propagate subscription change
    RDI_TypeMap*                            m_type_map_2; // DO propagate subscription change
    TW_Mutex                                m_lock;
    LocationKey2ProxySupplierListMap        m_location_key_2_proxy_list_map;
    Domain2LocationKey2ProxySupplierListMap m_domain_2_location_key_2_proxy_list_map;
};

inline RDIstrstream& operator<< (RDIstrstream& str, TA_RDITypeMap& map) { return map.log_output(str); }

#endif
