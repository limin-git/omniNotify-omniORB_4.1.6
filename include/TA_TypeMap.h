#ifndef TA_TYPE_MAP_H_INCLUDED
#define TA_TYPE_MAP_H_INCLUDED

#include "CosNotifyShorthands.h"
#include "RDILocksHeld.h "
#include <sstream>
#include <map>
#include <set>

class SequenceProxyPushSupplier_i;
class RDI_StructuredEvent;
class RDI_TypeMap;
class RDIProxySupplier;
class Filter_i;
class EventChannel_i;


class TA_TypeMap
{
public:

    TA_TypeMap();

    void initialize( EventChannel_i* channel, RDI_TypeMap*& origin_type_map );

    bool update_location_proxy_mapping(RDI_LocksHeld& held, const CosN::EventTypeSeq& added, const CosN::EventTypeSeq& deled, RDIProxySupplier* proxy, Filter_i* filter);
    void consumer_admin_dispatch_event(RDI_StructuredEvent*  event);

private:

    // filter related
    static int get_localocation_key( Filter_i* filter ); // ( $Region == '123' )
    static int get_location_key_from_constraint_expr( const char* constraint_expr );
    //
    static void get_filter_str( Filter_i* filter, std::ostream& strm );
    static void get_event_type_str( const CosN::EventType& event_type, std::ostream& strm );
    static void get_event_type_list_str( const CosN::EventTypeSeq& event_type_list, std::ostream& strm );

public:

    typedef std::set<SequenceProxyPushSupplier_i*> ProxySupplierList;
    typedef std::map<unsigned long, ProxySupplierList> LocationKey2ProxySupplierListMap;
    typedef std::map<std::string, LocationKey2ProxySupplierListMap> Domain2LocationKey2ProxySupplierListMap;

    RDI_TypeMap*                            _type_map_1;
    RDI_TypeMap*                            _type_map_2;
    TW_Mutex                                m_location_key_2_proxy_list_map_lock;
    LocationKey2ProxySupplierListMap        m_location_key_2_proxy_list_map;
    Domain2LocationKey2ProxySupplierListMap m_domain_2_location_key_2_proxy_list_map;
};



#endif
