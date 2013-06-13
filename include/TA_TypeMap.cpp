#include "TA_TypeMap.h"
#include "RDITypeMap.h"
#include "RDILocksHeld.h"


TA_TypeMap::TA_TypeMap()
    :  _type_map_1(NULL),
       _type_map_2(NULL)
{
}


void TA_TypeMap::initialize( EventChannel_i* channel, RDI_TypeMap*& origin_type_map )
{
    delete origin_type_map;
    origin_type_map = new RDI_TypeMap(NULL, 256);

    _type_map_1 = origin_type_map;
    _type_map_2 = new RDI_TypeMap(channel, 256);
}


bool TA_TypeMap::update_location_proxy_mapping(RDI_LocksHeld& held, const CosN::EventTypeSeq& added, const CosN::EventTypeSeq& deled, RDIProxySupplier* proxy, Filter_i* filter)
{

#ifdef USE_LOCATION_PROXY_SUPPLIER_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
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

    bool has_start_star_region_filter = false;

    if ( added.length() )
    {
        if ( RDI_STR_EQ( added[0].domain_name, "*" ) && RDI_STR_EQ( added[0].type_name, "*" ) )
        {
            int location_key = get_localocation_key( filter );

            if ( location_key != -1 )
            {
                {
                    TW_SCOPE_LOCK(l2p_map_lock, m_location_key_2_proxy_list_map_lock, "l2p_map_lock", WHATFN);
                    m_location_key_2_proxy_list_map[location_key].insert( dynamic_cast<SequenceProxyPushSupplier_i*>(proxy) );
                }

#ifdef USE_LOCATION_PROXY_SUPPLIER_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
                add_proxy_strm
                    << "\n\t" << "added a proxy with filter {[(*::*)]( $Region == '" << location_key << "' )}"
                    << ", location_number=" << m_location_key_2_proxy_list_map.size()
                    << ", cur_loc_proxy_number=" << m_location_key_2_proxy_list_map[location_key].size()
                    << "\n";
#endif
                has_start_star_region_filter = true;
            }
        }
        else if ( RDI_STR_EQ( added[0].type_name, "*" ) )
        {
            int location_key = get_localocation_key( filter );

            if ( location_key != -1 )
            {
                {
                    TW_SCOPE_LOCK(l2p_map_lock, m_location_key_2_proxy_list_map_lock, "l2p_map_lock", WHATFN);
                    m_domain_2_location_key_2_proxy_list_map[added[0].domain_name.in()][location_key].insert( dynamic_cast<SequenceProxyPushSupplier_i*>(proxy) );
                }

#ifdef USE_LOCATION_PROXY_SUPPLIER_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
                add_proxy_strm
                    << "\n\t" << "added a proxy with filter {[(" << added[0].domain_name << "::*)]( $Region == '" << location_key << "' )}"
                    << ", location_number=" << m_domain_2_location_key_2_proxy_list_map[added[0].domain_name.in()].size()
                    << ", cur_loc_proxy_number=" << m_domain_2_location_key_2_proxy_list_map[added[0].domain_name.in()][location_key].size()
                    << ", domain_number=" << m_domain_2_location_key_2_proxy_list_map.size()
                    << "\n";
#endif
                has_start_star_region_filter = true;
            }
        }
    }

    if ( deled.length() )
    {
        if ( RDI_STR_EQ( deled[0].domain_name, "*" ) && RDI_STR_EQ( deled[0].type_name, "*" ) )
        {
            TW_SCOPE_LOCK(l2p_map_lock, m_location_key_2_proxy_list_map_lock, "l2p_map_lock", WHATFN);

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
                        has_start_star_region_filter = true;
                    }

#ifdef USE_LOCATION_PROXY_SUPPLIER_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
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
            TW_SCOPE_LOCK(l2p_map_lock, m_location_key_2_proxy_list_map_lock, "l2p_map_lock", WHATFN);

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
                            has_start_star_region_filter = true;
                        }

#ifdef USE_LOCATION_PROXY_SUPPLIER_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
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

#ifdef USE_LOCATION_PROXY_SUPPLIER_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
    if ( added.length() || deled.length() )
    {
        RDIDbgForceLog( "EventChannel_i::update_location_proxy_mapping - " << "[channel=" << this->MyID() << "], [proxy=" << proxy->_proxy_id() << "], [filter=" << filter->MyFID() << "]"
            << add_del_strm.str().c_str()
            << filter_strm.str().c_str()
            << add_proxy_strm.str().c_str()
            << remove_proxy_strm.str().c_str()
            << " \n" );
    }
#endif

    if ( false == has_start_star_region_filter )
    {
        _type_map_1->update(held, added, deled, proxy, filter);
    }

    return _type_map_2->update(held, added, deled, proxy, filter);;
}


void TA_TypeMap::consumer_admin_dispatch_event(RDI_StructuredEvent*  event)
{
    if ( false == m_location_key_2_proxy_list_map.empty() || false == m_domain_2_location_key_2_proxy_list_map.empty() )
    {
        const RDI_RTVal * val = event->lookup_fdata_rtval( "Region" );

        if ( val != NULL )
        {
            char* delim = 0;
            int location_key = RDI_STRTOL(val->_v_string_ptr, &delim);
            if ((delim == 0) || (delim == val->_v_string_ptr) || (*delim != '\0'))
            {
                // TODO: log an error.
                return;
            }

            TW_SCOPE_LOCK(l2p_map_lock, m_location_key_2_proxy_list_map_lock, "l2p_map_lock", WHATFN);

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

#ifdef USE_LOCATION_PROXY_SUPPLIER_MAPPING_IN_EVENT_CHANNEL_LOG_DISPATCH_EVENT
                            RDIDbgForceLog( "\EventChannel_i::consumer_admin_dispatch_event - using location proxy mapping - add an event to proxy " << proxy->_proxy_id() << ". \n" );
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

#ifdef USE_LOCATION_PROXY_SUPPLIER_MAPPING_IN_EVENT_CHANNEL_LOG_DISPATCH_EVENT
                                RDIDbgForceLog( "\EventChannel_i::consumer_admin_dispatch_event - using location proxy mapping - add an event to proxy " << proxy->_proxy_id() << ". \n" );
#endif
                            }
                        }
                    }
                }
            }
        }
    }
}


int TA_TypeMap::get_localocation_key( Filter_i* filter ) // ( $Region == '123' )
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
                    if ( /*RDI_STR_EQ( event_types[j].domain_name, "*" ) &&*/ RDI_STR_EQ( event_types[j].type_name, "*" ) )
                    {
                        return get_location_key_from_constraint_expr( constraint_expression.constraint_expr.in() );;
                    }
                }
            }
        }
    }

    return -1;
}


int TA_TypeMap::get_location_key_from_constraint_expr( const char* constraint_expr )
{
    static const char*  REGION_EXPRESSION = "$Region == '";   // Note: TA_CosUtility.cpp:gGenerateConstraintExpression
    static const char*  AND_OPERATION = " ) and ( ";
    static const size_t REGION_EXPRESSION_LENGTH = ::strlen(REGION_EXPRESSION);
    static const size_t MAX_NUMBER_LENGTH = 10;

    char* exp_beg = std::strstr( const_cast<char*>(constraint_expr), REGION_EXPRESSION );

    if ( exp_beg != NULL )
    {
        if ( std::strstr( const_cast<char*>(constraint_expr), AND_OPERATION ) != NULL ) // Note: cannot deal with complex expression
        {
            return -1;
        }

        char* number_beg = exp_beg + REGION_EXPRESSION_LENGTH;
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


#ifdef USE_LOCATION_PROXY_SUPPLIER_MAPPING_IN_EVENT_CHANNEL_LOG_UPDATE_MAPPING
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
#endif
