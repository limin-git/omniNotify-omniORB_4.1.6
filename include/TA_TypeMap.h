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

    struct ProxyInfo
    {
        ProxyInfo( RDIProxySupplier* proxy_, SequenceProxyPushSupplier_i* seq_prx_ = NULL, CosNA::ProxyID prx_id_ = 0 )
            : proxy( proxy_ ),
              seq_prx( seq_prx_ ),
              prx_id( prx_id_ )
        {
        }

        bool operator <( const ProxyInfo& rhs ) const
        {
            return ( proxy < rhs.proxy );
        }

        CosNA::ProxyID prx_id;
        RDIProxySupplier* proxy;
        SequenceProxyPushSupplier_i* seq_prx;
    };

    typedef std::set<ProxyInfo> ProxySupplierList;
    typedef std::map<unsigned long, ProxySupplierList> LocationKey2ProxyListMap;
    typedef std::map<std::string, LocationKey2ProxyListMap> Domain2LocationKey2ProxyListMap;

public:

    bool ta_update( RDI_LocksHeld& held, const CosN::EventTypeSeq& added, const CosN::EventTypeSeq& deled, RDIProxySupplier* proxy, Filter_i* filter );
    RDIstrstream& log_output(RDIstrstream& str);

public:

    TA_TypeMap();
    ~TA_TypeMap();
    void initialize( EventChannel_i* channel, ConsumerAdmin_i* cadmin, RDI_TypeMap*& original_type_map );
    void consumer_admin_dispatch_event(RDI_StructuredEvent*  event);
    RDI_Hash<CosNA::ProxyID, SequenceProxyPushSupplier_i *>* get_prx_batch_push();

private:

    void update_prx_batch_push( const ProxyInfo& prx_info );

    static int  remove_proxy( LocationKey2ProxyListMap& lk2prx_map, const ProxyInfo& prx_info );
    static int  remove_proxy( Domain2LocationKey2ProxyListMap& d2lk2prx_map, const ProxyInfo& prx_info, const char* domain_name );
    static void remove_proxy( LocationKey2ProxyListMap& lk2prx_map, const ProxySupplierList& prx_list );
    static void remove_proxy( Domain2LocationKey2ProxyListMap& d2lk2prx_map, const ProxySupplierList& prx_list, const char* domain_name );

    static int get_location_key_from_filter( Filter_i* filter );
    static int get_location_key_from_filter_constraint_expr( const char* constraint_expr );  // ( $Region == '123' )
    static int get_location_key_from_event( RDI_StructuredEvent* event );

public:

    TW_Mutex                                                    m_lock;
    ConsumerAdmin_i*                                            m_cadmin;
    RDI_TypeMap*                                                m_type_map_1; // original type map, DO NOT propagate subscription change
    RDI_TypeMap*                                                m_type_map_2; // DO propagate subscription change
    LocationKey2ProxyListMap                                    m_lk2prx_map;
    Domain2LocationKey2ProxyListMap                             m_d2lk2prx_map;
    RDI_Hash<CosNA::ProxyID, SequenceProxyPushSupplier_i*>      m_prx_batch_push;
    RDI_Hash<CosNA::ProxyID, SequenceProxyPushSupplier_i*>      m_prx_batch_push_2;
    volatile bool                                               m_is_prx_batch_push_changed;
    std::map<RDIProxySupplier*, CosNA::ProxyID>                 m_prx_2_id_map;
};

inline RDIstrstream& operator<< (RDIstrstream& str, TA_TypeMap& map) { return map.log_output(str); }

#endif
