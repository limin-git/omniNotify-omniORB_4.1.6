#ifndef TA_TYPE_MAP_CPP_INCLUDED
#define TA_TYPE_MAP_CPP_INCLUDED

#include "TA_TypeMap.h"
#include "RDITypeMap.h"
#include "RDILocksHeld.h"


TA_TypeMap::TA_TypeMap()
    : m_channel(NULL),
      m_type_map_1(NULL),
      m_type_map_2(NULL),
      _prx_batch_push(RDI_ULongHash, RDI_ULongRank)
{
}


TA_TypeMap::~TA_TypeMap()
{
    // DO NOT delete m_type_map_1
    delete m_type_map_2;
    m_type_map_2 = NULL;
}


void TA_TypeMap::initialize( EventChannel_i* channel, RDI_TypeMap*& original_type_map )
{
    delete original_type_map;
    original_type_map = new RDI_TypeMap(NULL, 256); // DO NOT propagate subscription change

    m_channel = channel;
    m_type_map_1 = original_type_map;
    m_type_map_2 = new RDI_TypeMap(channel, 256); // DO propagate subscription change
}


#undef WHATFN
#define WHATFN "TA_TypeMap::ta_update"
bool TA_TypeMap::ta_update( RDI_LocksHeld& held, const CosN::EventTypeSeq& added, const CosN::EventTypeSeq& deled, RDIProxySupplier* proxy, Filter_i* filter )
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
            int location_key = extract_location_key_from_filter( filter );

            if ( location_key != -1 )
            {
                {
                    TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "ta_type_map_lock", WHATFN);
                    m_location_key_2_proxy_list_map[location_key].insert( dynamic_cast<SequenceProxyPushSupplier_i*>(proxy) );
                    ta_typemap_updated = true;
                }

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
                add_proxy_strm
                    << "\n\t" << "added a proxy with filter {[(*::*)]( $Region == '" << location_key << "' )}"
                    << ", location_number=" << m_location_key_2_proxy_list_map.size()
                    << ", cur_loc_proxy_number=" << m_location_key_2_proxy_list_map[location_key].size()
                    << "\n";
#endif
            }
        }
        else if ( RDI_STR_EQ( added[0].type_name, "*" ) )
        {
            int location_key = extract_location_key_from_filter( filter );

            if ( location_key != -1 )
            {
                {
                    TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "ta_type_map_lock", WHATFN);
                    m_domain_2_location_key_2_proxy_list_map[added[0].domain_name.in()][location_key].insert( dynamic_cast<SequenceProxyPushSupplier_i*>(proxy) );
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

            for ( LocationKey2ProxySupplierListMap::iterator it = m_location_key_2_proxy_list_map.begin(); it != m_location_key_2_proxy_list_map.end(); ++it )
            {
                unsigned long location_key = it->first;
                ProxySupplierList& proxy_list = it->second;
                size_t cur_loc_proxy_number = proxy_list.size();

                ProxySupplierList::iterator findIt = proxy_list.find( dynamic_cast<SequenceProxyPushSupplier_i*>(proxy) );

                if ( findIt != proxy_list.end() )
                {
                    proxy_list.erase( findIt );

                    if ( true == proxy_list.empty() )
                    {
                        m_location_key_2_proxy_list_map.erase( it );
                        ta_typemap_updated = true;
                    }

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
                    remove_proxy_strm
                        << "\n\t" << "removed a proxy with filter {[(*::*)]( $Region == '" << location_key << "' )}"
                        << ", location_number=" << m_location_key_2_proxy_list_map.size()
                        << ", cur_loc_proxy_number=" << --cur_loc_proxy_number
                        << "\n";
#endif

                    break; // the proxy has just one filter
                }
            }
        }
        else if ( RDI_STR_EQ( deled[0].type_name, "*" ) )
        {
            TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "ta_type_map_lock", WHATFN);

            Domain2LocationKey2ProxySupplierListMap::iterator find_domain_it = m_domain_2_location_key_2_proxy_list_map.find( deled[0].domain_name.in() );

            if ( find_domain_it != m_domain_2_location_key_2_proxy_list_map.end() )
            {
                LocationKey2ProxySupplierListMap& location_key_2_proxy_list_map = find_domain_it->second;

                for ( LocationKey2ProxySupplierListMap::iterator it = location_key_2_proxy_list_map.begin(); it != location_key_2_proxy_list_map.end(); ++it )
                {
                    unsigned long location_key = it->first;
                    ProxySupplierList& proxy_list = it->second;
                    size_t cur_loc_proxy_number = proxy_list.size();

                    ProxySupplierList::iterator find_proxy_it = proxy_list.find( dynamic_cast<SequenceProxyPushSupplier_i*>(proxy) );

                    if ( find_proxy_it != proxy_list.end() )
                    {
                        proxy_list.erase( find_proxy_it );

                        if ( true == proxy_list.empty() )
                        {
                            location_key_2_proxy_list_map.erase( it );
                            ta_typemap_updated = true;
                        }

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
                        remove_proxy_strm
                            << "\n\t" << "removed a proxy with filter {[(" << deled[0].domain_name.in() << "::*)]( $Region == '"  << location_key << "' )}"
                            << ", location_number=" << m_location_key_2_proxy_list_map.size()
                            << ", cur_loc_proxy_number=" << --cur_loc_proxy_number
                            << ", domain_number=" << m_domain_2_location_key_2_proxy_list_map.size() - ( true == location_key_2_proxy_list_map.empty() )
                            << "\n";
#endif

                        break; // the proxy has just one filter
                    }
                }

                if ( true == location_key_2_proxy_list_map.empty() )
                {
                    m_domain_2_location_key_2_proxy_list_map.erase( find_domain_it );
                }
            }
        }
    }

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
    if ( added.length() || deled.length() )
    {
        RDIDbgForceLog( "TA_TypeMap::ta_update - " << "[channel=" << m_channel->MyID() << "], [proxy=" << proxy->_proxy_id() << "], [filter=" << filter->MyFID() << "]"
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

        _prx_batch_push.clear();

        RDI_Hash<CosN::EventType, RDI_TypeMap::VNode_t>& _tmap = m_type_map_1->_tmap;

        for ( RDI_HashCursor<CosN::EventType, RDI_TypeMap::VNode_t> curs = _tmap.cursor(); curs.is_valid(); curs++ )
        {
            RDI_TypeMap::PNode_t* pnode = curs.val()._prxy;
            SequenceProxyPushSupplier_i* proxy = dynamic_cast<SequenceProxyPushSupplier_i*>( pnode->_prxy );

            if ( proxy != NULL )
            {
                _prx_batch_push.insert( proxy->_proxy_id(), proxy );
            }
        }
    }

    return m_type_map_2->update(held, added, deled, proxy, filter);
}


#undef WHATFN
#define WHATFN "TA_TypeMap::consumer_admin_dispatch_event"
void TA_TypeMap::consumer_admin_dispatch_event(RDI_StructuredEvent* event)
{
    if ( false == m_location_key_2_proxy_list_map.empty() || false == m_domain_2_location_key_2_proxy_list_map.empty() )
    {
        int location_key = extract_location_key_from_event( event );

        if ( -1 == location_key )
        {
            return;
        }

        TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "ta_type_map_lock", WHATFN);

        if ( false == m_location_key_2_proxy_list_map.empty() )
        {
            LocationKey2ProxySupplierListMap::iterator find_location_it = m_location_key_2_proxy_list_map.find( location_key );

            if ( find_location_it != m_location_key_2_proxy_list_map.end() )
            {
                ProxySupplierList& proxy_list = find_location_it->second;

                for ( ProxySupplierList::iterator it = proxy_list.begin(); it != proxy_list.end(); ++it )
                {
                    SequenceProxyPushSupplier_i* proxy = *it;

                    if (  proxy != NULL )
                    {
                        proxy->add_event(event);

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_DISPATCH_EVENT
                        RDIDbgForceLog( "\TA_TypeMap::consumer_admin_dispatch_event - using ta type mapping - add an event to proxy " << proxy->_proxy_id() << ". \n" );
#endif
                    }
                }
            }
        }

        if ( false == m_domain_2_location_key_2_proxy_list_map.empty() )
        {
            Domain2LocationKey2ProxySupplierListMap::iterator find_domain_it = m_domain_2_location_key_2_proxy_list_map.find( event->get_domain_name() );

            if ( find_domain_it != m_domain_2_location_key_2_proxy_list_map.end() )
            {
                LocationKey2ProxySupplierListMap& location_key_2_proxy_list_map = find_domain_it->second;

                LocationKey2ProxySupplierListMap::iterator find_location_it = location_key_2_proxy_list_map.find( location_key );

                if ( find_location_it != location_key_2_proxy_list_map.end() )
                {
                    ProxySupplierList& proxy_list = find_location_it->second;

                    for ( ProxySupplierList::iterator it = proxy_list.begin(); it != proxy_list.end(); ++it )
                    {
                        SequenceProxyPushSupplier_i* proxy = *it;

                        if (  proxy != NULL )
                        {
                            proxy->add_event(event);

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_DISPATCH_EVENT
                            RDIDbgForceLog( "\TA_TypeMap::consumer_admin_dispatch_event - using ta type mapping - add an event to proxy " << proxy->_proxy_id() << ". \n" );
#endif
                        }
                    }
                }
            }
        }
    }
}


#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_OUT_DEBUG_INFO
#undef WHATFN
#define WHATFN "TA_TypeMap::log_output"
RDIstrstream& TA_TypeMap::log_output(RDIstrstream& str)
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
#endif // USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_OUT_DEBUG_INFO


int TA_TypeMap::extract_location_key_from_filter( Filter_i* filter ) // ( $Region == '123' )
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

    return -1;
}


int TA_TypeMap::extract_location_key_from_filter_constraint_expr( const char* constraint_expr )
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
            return -1;
        }

        char* number_beg = expr_beg + REGION_EXPRESSION_LENGTH;
        char* number_end = ::strchr( number_beg, '\'' );
        size_t number_len = number_end - number_beg;

        if ( number_end != NULL )
        {
            char location_key_buf[MAX_NUMBER_LENGTH] = { 0 };

            std::strncpy( location_key_buf, number_beg, number_len );

            char* delim = 0;
            int location_key = RDI_STRTOL(location_key_buf, &delim);
            if ((delim == 0) || (delim == location_key_buf) || (*delim != '\0'))
            {
                return -1;
            }

            return location_key;
        }
    }

    return -1;
}


int TA_TypeMap::extract_location_key_from_event( RDI_StructuredEvent* event )
{
    try
    {
        const CosN::StructuredEvent& cos_event = event->get_cos_event();
        _CORBA_ULong length = cos_event.filterable_data.length();

        for ( size_t i = 0; i < length; ++i )
        {
            const CosN::Property& property = cos_event.filterable_data[i];

            if ( RDI_STR_EQ( "Region", property.name.in() ) )
            {
                CORBA::TypeCode_var tmp_tcp = property.value.type();

                if ( CORBA::_tc_string->equivalent(tmp_tcp) )
                {
                    const char* str = 0;
                    property.value >>= str;

                    if ( str != NULL )
                    {
                        char* delim = 0;
                        int location_key = RDI_STRTOL(str, &delim);

                        if ( (delim == 0) || (delim == str) || (*delim != '\0') )
                        {
                            return -1;
                        }

                        return location_key;
                    }
                }
            }
        }
    }
    catch (...)
    {
    }

    return -1;
}



#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
void TA_TypeMap::get_filter_str( Filter_i* filter, std::ostream& strm )
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


void TA_TypeMap::get_event_type_str( const CosN::EventType& event_type, std::ostream& strm )
{
    strm << "(" << event_type.domain_name.in() << "::" << event_type.type_name.in() << ")";
}


void TA_TypeMap::get_event_type_list_str( const CosN::EventTypeSeq& event_type_list, std::ostream& strm )
{
    strm << "[";

    for ( size_t i = 0; i < event_type_list.length(); ++i )
    {
        get_event_type_str( event_type_list[i], strm );
    }

    strm << "]";
}
#endif // USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING

#endif
