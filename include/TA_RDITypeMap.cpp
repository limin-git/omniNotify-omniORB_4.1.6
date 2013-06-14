#ifndef TA_RDI_TYPE_MAP_CPP_INCLUDED
#define TA_RDI_TYPE_MAP_CPP_INCLUDED

#include "TA_RDITypeMap.h"
#include "RDITypeMap.h"
#include "RDILocksHeld.h"


TA_RDITypeMap::TA_RDITypeMap()
    : m_channel(NULL),
      m_type_map_1(NULL),
      m_type_map_2(NULL),
      m_location_key_2_proxy_list_map(RDI_StrHash, RDI_StrRank),
      m_domain_2_location_key_2_proxy_list_map(RDI_StrHash, RDI_StrRank)
{
}


TA_RDITypeMap::~TA_RDITypeMap()
{
    // DO NOT delete m_type_map_1
    delete m_type_map_2;
    m_type_map_2 = NULL;
}


void TA_RDITypeMap::initialize( EventChannel_i* channel, RDI_TypeMap*& original_type_map )
{
    delete original_type_map;
    original_type_map = new RDI_TypeMap(NULL, 256); // DO NOT propagate subscription change

    m_channel = channel;
    m_type_map_1 = original_type_map;
    m_type_map_2 = new RDI_TypeMap(channel, 256); // DO propagate subscription change
}


#undef WHATFN
#define WHATFN "TA_RDITypeMap::ta_update"
bool TA_RDITypeMap::ta_update( RDI_LocksHeld& held, const CosN::EventTypeSeq& added, const CosN::EventTypeSeq& deled, RDIProxySupplier* proxy, Filter_i* filter )
{
#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
    std::stringstream add_del_strm;
    std::stringstream filter_strm;
    std::stringstream add_proxy_strm;
    std::stringstream remove_proxy_strm;

    add_del_strm<< "\n\t" << "added: ";
    get_event_type_list_str( added, add_del_strm );

    add_del_strm << "\n\t" << "deled: ";
    get_event_type_list_str( deled, add_del_strm );

    filter_strm << "\n\t" << "filter: ";
    get_filter_str( filter, filter_strm );
#endif

    bool ta_typemap_updated = false;

    if ( added.length() )
    {
        if ( RDI_STR_EQ( added[0].domain_name, "*" ) && RDI_STR_EQ( added[0].type_name, "*" ) )
        {
            CORBA::String_member location_key = extract_location_key_from_filter( filter );

            if ( RDI_STR_NEQ( location_key, "" ) )
            {
                {
                    TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "ta_type_map_lock", WHATFN);

                    SequenceProxyPushSupplier_i* sproxy = dynamic_cast<SequenceProxyPushSupplier_i*>(proxy);
                    ProxySupplierList* proxy_list = NULL;

                    if ( m_location_key_2_proxy_list_map.lookup( location_key, proxy_list ) != 0 )
                    {
                        RDI_ListCursor<SequenceProxyPushSupplier_i*> lc;
                        unsigned int indx = 0;

                        for ( lc = proxy_list->cursor(); indx < proxy_list->length(); ++indx, ++lc )
                        {
                            if ( sproxy == *lc )
                            {
                                break;
                            }
                        }

                        if ( indx == proxy_list->length() )
                        {
                            proxy_list->insert_tail( sproxy );
                        }
                    }
                    else
                    {
                        proxy_list = new ProxySupplierList;
                        proxy_list->insert_tail( sproxy );
                        m_location_key_2_proxy_list_map.insert( location_key, proxy_list );
                    }

                    ta_typemap_updated = true;

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
                    add_proxy_strm
                        << "\n\t" << "added a proxy with filter {[(*::*)]( $Region == '" << location_key << "' )}"
                        << ", location_number=" << m_location_key_2_proxy_list_map.length()
                        << ", cur_loc_proxy_number=" << proxy_list->length()
                        << "\n";
#endif
                }
            }
        }
        else if ( RDI_STR_EQ( added[0].type_name, "*" ) )
        {
            CORBA::String_member location_key = extract_location_key_from_filter( filter );

            if ( RDI_STR_NEQ( location_key, "" ) )
            {
                {
                    TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "ta_type_map_lock", WHATFN);

                    LocationKey2ProxySupplierListMap* location_key_2_proxy_list_map = NULL;
                    if ( m_domain_2_location_key_2_proxy_list_map.lookup( added[0].domain_name, location_key_2_proxy_list_map ) != 0 )
                    {
                        SequenceProxyPushSupplier_i* sproxy = dynamic_cast<SequenceProxyPushSupplier_i*>(proxy);
                        ProxySupplierList* proxy_list = NULL;

                        if ( location_key_2_proxy_list_map->lookup( location_key, proxy_list ) != 0 )
                        {
                            RDI_ListCursor<SequenceProxyPushSupplier_i*> lc;
                            unsigned int indx = 0;

                            for ( lc = proxy_list->cursor(); indx < proxy_list->length(); ++indx, ++lc )
                            {
                                if ( sproxy == *lc )
                                {
                                    break;
                                }
                            }

                            if ( indx == proxy_list->length() )
                            {
                                proxy_list->insert_tail( sproxy );
                            }
                        }
                        else
                        {
                            proxy_list = new ProxySupplierList;
                            proxy_list->insert_tail( sproxy );
                            location_key_2_proxy_list_map->insert( location_key, proxy_list );
                        }
                    }
                    else
                    {
                        SequenceProxyPushSupplier_i* sproxy = dynamic_cast<SequenceProxyPushSupplier_i*>(proxy);
                        ProxySupplierList* proxy_list = new ProxySupplierList;
                        proxy_list->insert_tail( sproxy );

                        location_key_2_proxy_list_map = new LocationKey2ProxySupplierListMap( RDI_StrHash, RDI_StrRank );
                        location_key_2_proxy_list_map->insert( location_key, proxy_list );

                        m_domain_2_location_key_2_proxy_list_map.insert( added[0].domain_name, location_key_2_proxy_list_map );                        
                    }

                    ta_typemap_updated = true;
                }

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
                add_proxy_strm
                    << "\n\t" << "added a proxy with filter {[(" << added[0].domain_name << "::*)]( $Region == '" << location_key << "' )}"
                    << ", location_number=" << m_domain_2_location_key_2_proxy_list_map[added[0].domain_name.in()].size()
                    << ", cur_loc_proxy_number=" << m_domain_2_location_key_2_proxy_list_map[added[0].domain_name.in()][location_key].size()
                    << ", domain_number=" << m_domain_2_location_key_2_proxy_list_map.size()
                    << "\n";
#endif
            }
        }
    }

    if ( deled.length() )
    {
        if ( RDI_STR_EQ( deled[0].domain_name, "*" ) && RDI_STR_EQ( deled[0].type_name, "*" ) )
        {
            TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "ta_type_map_lock", WHATFN);

            RDI_HashCursor<CORBA::String_member, ProxySupplierList*> l2pcur;

            for ( l2pcur = m_location_key_2_proxy_list_map.cursor(); l2pcur.is_valid(); ++l2pcur )
            {
                const CORBA::String_member& location_key = l2pcur.key();
                ProxySupplierList* proxy_list = l2pcur.val();

                SequenceProxyPushSupplier_i* sproxy = dynamic_cast<SequenceProxyPushSupplier_i*>(proxy);

                RDI_ListCursor<SequenceProxyPushSupplier_i*> lc;
                unsigned int indx = 0;

                for ( lc = proxy_list->cursor(); indx < proxy_list->length(); ++indx, ++lc )
                {
                    if ( sproxy == *lc )
                    {
                        proxy_list->remove( lc );

                        if ( 0 == proxy_list->length() )
                        {
                            delete proxy_list;
                            proxy_list = NULL;
                            m_location_key_2_proxy_list_map.remove( location_key );
                        }

                        ta_typemap_updated = true;

                        break;
                    }
                }
            }
        }
        else if ( RDI_STR_EQ( deled[0].type_name, "*" ) )
        {
            TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "ta_type_map_lock", WHATFN);

            RDI_HashCursor<CORBA::String_member, LocationKey2ProxySupplierListMap*> d2l2pcur;

            for ( d2l2pcur = m_domain_2_location_key_2_proxy_list_map.cursor(); d2l2pcur.is_valid(); ++d2l2pcur )
            {
                const CORBA::String_member& domain_name = d2l2pcur.key();

                if ( domain_name != deled[0].domain_name )
                {
                    continue;
                }

                LocationKey2ProxySupplierListMap* location_key_2_proxy_list_map = d2l2pcur.val();

                RDI_HashCursor<CORBA::String_member, ProxySupplierList*> l2pcur;

                for ( l2pcur = location_key_2_proxy_list_map->cursor(); l2pcur.is_valid(); ++l2pcur )
                {
                    const CORBA::String_member& location_key = l2pcur.key();
                    ProxySupplierList* proxy_list = l2pcur.val();

                    SequenceProxyPushSupplier_i* sproxy = dynamic_cast<SequenceProxyPushSupplier_i*>(proxy);

                    RDI_ListCursor<SequenceProxyPushSupplier_i*> lc;
                    unsigned int indx = 0;

                    for ( lc = proxy_list->cursor(); indx < proxy_list->length(); ++indx, ++lc )
                    {
                        if ( sproxy == *lc )
                        {
                            proxy_list->remove( lc );

                            if ( 0 == proxy_list->length() )
                            {
                                delete proxy_list;
                                proxy_list = NULL;

                                location_key_2_proxy_list_map->remove( location_key );

                                if ( 0 == location_key_2_proxy_list_map->length() )
                                {
                                    delete location_key_2_proxy_list_map;
                                    location_key_2_proxy_list_map = NULL;
                                    m_domain_2_location_key_2_proxy_list_map.remove( domain_name );
                                }
                            }

                            ta_typemap_updated = true;

                            break;
                        }
                    }
                }

                break;
            }
        }
    }

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
    if ( added.length() || deled.length() )
    {
        RDIDbgForceLog( "TA_RDITypeMap::ta_update - " << "[channel=" << m_channel->MyID() << "], [proxy=" << proxy->_proxy_id() << "], [filter=" << filter->MyFID() << "]"
            << add_del_strm.str().c_str()
            << filter_strm.str().c_str()
            << add_proxy_strm.str().c_str()
            << remove_proxy_strm.str().c_str()
            << " \n" );
    }
#endif

    if ( false == ta_typemap_updated )
    {
        m_type_map_1->update(held, added, deled, proxy, filter);
    }

    return m_type_map_2->update(held, added, deled, proxy, filter);
}


#undef WHATFN
#define WHATFN "TA_RDITypeMap::consumer_admin_dispatch_event"
void TA_RDITypeMap::consumer_admin_dispatch_event(RDI_StructuredEvent*  event)
{
    if ( m_location_key_2_proxy_list_map.length() || m_domain_2_location_key_2_proxy_list_map.length() )
    {
        const RDI_RTVal * val = event->lookup_fdata_rtval( "Region" );

        if ( val != NULL )
        {
            TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "ta_type_map_lock", WHATFN);

            CORBA::String_member location_key = (const char*)val->_v_string_ptr;

            if ( m_location_key_2_proxy_list_map.length() )
            {
                ProxySupplierList* proxy_list = NULL;

                if ( m_location_key_2_proxy_list_map.lookup( location_key, proxy_list ) != 0 )
                {
                    RDI_ListCursor<SequenceProxyPushSupplier_i*> lc;
                    unsigned int indx = 0;

                    for ( lc = proxy_list->cursor(); indx < proxy_list->length(); ++indx, ++lc )
                    {
                        SequenceProxyPushSupplier_i* proxy = *lc;

                        if ( proxy != NULL )
                        {
                            proxy->add_event( event );

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_DISPATCH_EVENT
                            RDIDbgForceLog( "\TA_RDITypeMap::consumer_admin_dispatch_event - using ta type mapping - add an event to proxy " << proxy->_proxy_id() << ". \n" );
#endif
                        }
                    }
                }
            }

            if ( false == m_domain_2_location_key_2_proxy_list_map.length() )
            {
                LocationKey2ProxySupplierListMap* location_key_2_proxy_list_map = NULL;

                if ( m_domain_2_location_key_2_proxy_list_map.lookup( event->get_domain_name(), location_key_2_proxy_list_map ) != 0 )
                {
                    ProxySupplierList* proxy_list = NULL;

                    if ( location_key_2_proxy_list_map->lookup( location_key, proxy_list ) != 0 )
                    {
                        RDI_ListCursor<SequenceProxyPushSupplier_i*> lc;
                        unsigned int indx = 0;

                        for ( lc = proxy_list->cursor(); indx < proxy_list->length(); ++indx, ++lc )
                        {
                            SequenceProxyPushSupplier_i* proxy = *lc;

                            if ( proxy != NULL )
                            {
                                proxy->add_event( event );

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_DISPATCH_EVENT
                                RDIDbgForceLog( "\TA_RDITypeMap::consumer_admin_dispatch_event - using ta type mapping - add an event to proxy " << proxy->_proxy_id() << ". \n" );
#endif
                            }
                        }
                    }
                }
            }
        }
    }
}


#if 0

#undef WHATFN
#define WHATFN "TA_RDITypeMap::log_output"
RDIstrstream& TA_RDITypeMap::log_output(RDIstrstream& str)
{
    //Ref: RDI_TypeMap::log_output
    str << "----------\nTA_TypeMap\n----------\n";

    TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "ta_type_map_lock", WHATFN);

    if ( m_location_key_2_proxy_list_map.empty() && m_domain_2_location_key_2_proxy_list_map.empty() )
    {
        str << "\t(no entries)\n";
        return str;
    }

    if ( false == m_location_key_2_proxy_list_map.empty() )
    {
        str << "*::* ( $Region == 'L' )";

        for ( LocationKey2ProxySupplierListMap::iterator it = m_location_key_2_proxy_list_map.begin(); it != m_location_key_2_proxy_list_map.end(); ++it )
        {
            unsigned long location_key = it->first;
            ProxySupplierList& proxy_list = it->second;

            str << "\n\tL ";
            str.setw(3); str << location_key;
            str << ": ";

            for ( ProxySupplierList::iterator proxy_it = proxy_list.begin(); proxy_it != proxy_list.end(); ++proxy_it )
            {
                str.setw(9); str << *proxy_it;
            }
        }

        str << "\n";
    }

    for ( Domain2LocationKey2ProxySupplierListMap::iterator it = m_domain_2_location_key_2_proxy_list_map.begin(); it != m_domain_2_location_key_2_proxy_list_map.end(); ++it )
    {
        const std::string& domain_name = it->first;
        LocationKey2ProxySupplierListMap& location_key_2_proxy_list_map = it->second;

        str << domain_name.c_str() << "::* ( $Region == 'L' )";

        if ( location_key_2_proxy_list_map.empty() )
        {
            str << "\n\t(no entries)";
        }

        for ( LocationKey2ProxySupplierListMap::iterator it = location_key_2_proxy_list_map.begin(); it != location_key_2_proxy_list_map.end(); ++it )
        {
            unsigned long location_key = it->first;
            ProxySupplierList& proxy_list = it->second;

            str << "\n\tL ";
            str.setw(3); str << location_key;
            str << ": ";

            for ( ProxySupplierList::iterator proxy_it = proxy_list.begin(); proxy_it != proxy_list.end(); ++proxy_it )
            {
                str.setw(9); str << *proxy_it;
            }
        }

        str << "\n";
    }

    return str;
}

#endif
CORBA::String_member TA_RDITypeMap::extract_location_key_from_filter( Filter_i* filter ) // ( $Region == '123' )
{
    if ( filter != NULL )
    {
        CosNF::ConstraintInfoSeq* all_constraints = filter->get_all_constraints();

        if ( all_constraints != NULL )
        {
            for ( size_t i = 0; i < all_constraints->length(); ++i )
            {
                CosNF::ConstraintInfo& constraint_info = (*all_constraints)[i];
                CosNF::ConstraintExp& constraint_expression = constraint_info.constraint_expression;
                CosNotification::EventTypeSeq& event_types = constraint_expression.event_types;

                for ( size_t j = 0; j < event_types.length(); ++j )
                {
                    if ( RDI_STR_EQ( event_types[j].type_name, "*" ) )
                    {
                        return extract_location_key_from_filter_constraint_expr( constraint_expression.constraint_expr.in() );;
                    }
                }
            }
        }
    }

    return CORBA::String_member();
}

CORBA::String_member TA_RDITypeMap::extract_location_key_from_filter_constraint_expr( const char* constraint_expr )
{
    static const char*  REGION_EXPRESSION = "$Region == '";   // Note: TA_CosUtility.cpp:gGenerateConstraintExpression
    static const char*  AND_OPERATION = " ) and ( ";
    static const size_t REGION_EXPRESSION_LENGTH = ::strlen(REGION_EXPRESSION);
    static const size_t MAX_NUMBER_LENGTH = 10;

    char* expr_beg = std::strstr( const_cast<char*>(constraint_expr), REGION_EXPRESSION );

    if ( expr_beg != NULL )
    {
        if ( std::strstr( const_cast<char*>(constraint_expr), AND_OPERATION ) != NULL ) // Note: cannot deal with complex expression
        {
            return CORBA::String_member();
        }

        char* number_beg = expr_beg + REGION_EXPRESSION_LENGTH;
        char* number_end = ::strchr( number_beg, '\'' );
        size_t number_len = number_end - number_beg;

        if ( number_end != NULL )
        {
            char location_key_buf[MAX_NUMBER_LENGTH] = { 0 };
            std::strncpy( location_key_buf, number_beg, number_len );
            return CORBA::String_member( (const char*)location_key_buf );
        }
    }

    return CORBA::String_member();
}

#if 0

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
void TA_RDITypeMap::get_filter_str( Filter_i* filter, std::ostream& strm )
{
    if ( filter != NULL )
    {
        CosNF::ConstraintInfoSeq* all_constraints = filter->get_all_constraints();

        if ( all_constraints != NULL )
        {
            for ( size_t i = 0; i < all_constraints->length(); ++i )
            {
                strm << i << ":{";

                CosNF::ConstraintInfo& constraint_info = (*all_constraints)[i];
                CosNF::ConstraintExp& constraint_expression = constraint_info.constraint_expression;
                CosNotification::EventTypeSeq& event_types = constraint_expression.event_types;

                get_event_type_list_str( event_types, strm );

                strm << constraint_expression.constraint_expr.in();
                strm << "} ";
            }
        }
    }
}


void TA_RDITypeMap::get_event_type_str( const CosN::EventType& event_type, std::ostream& strm )
{
    strm << "(" << event_type.domain_name.in() << "::" << event_type.type_name.in() << ")";
}


void TA_RDITypeMap::get_event_type_list_str( const CosN::EventTypeSeq& event_type_list, std::ostream& strm )
{
    strm << "[";

    for ( size_t i = 0; i < event_type_list.length(); ++i )
    {
        get_event_type_str( event_type_list[i], strm );
    }

    strm << "]";
}
#endif

#endif

#endif
