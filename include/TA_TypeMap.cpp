#ifndef TA_TYPE_MAP_CPP_INCLUDED
#define TA_TYPE_MAP_CPP_INCLUDED

#include "TA_TypeMap.h"
#include "RDITypeMap.h"
#include "RDILocksHeld.h"
#include <iostream>
#include <algorithm>


TA_TypeMap::TA_TypeMap()
    : m_type_map_1(NULL),
      m_type_map_2(NULL),
      m_prx_batch_push(RDI_ULongHash, RDI_ULongRank),
      m_prx_batch_push_2(RDI_ULongHash, RDI_ULongRank),
      m_is_prx_batch_push_changed( false )
{
}


TA_TypeMap::~TA_TypeMap()
{
    // DO NOT delete m_type_map_1
    delete m_type_map_2;
    m_type_map_2 = NULL;
}


void TA_TypeMap::initialize( EventChannel_i* channel, ConsumerAdmin_i* cadmin, RDI_TypeMap*& original_type_map )
{
    delete original_type_map;
    original_type_map = new RDI_TypeMap(NULL, 256); // DO NOT propagate subscription change

    m_type_map_1 = original_type_map;
    m_type_map_2 = new RDI_TypeMap(channel, 256); // DO propagate subscription change

    m_cadmin = cadmin;
}


CosN::EventTypeSeq* TA_TypeMap::obtain_subscription_types()
{
    return m_type_map_2->obtain_subscription_types();
}


CosN::EventTypeSeq* TA_TypeMap::pxy_obtain_subscription_types(RDIProxyConsumer* pxy, CosNA::ObtainInfoMode mode)
{
    return m_type_map_2->pxy_obtain_subscription_types( pxy, mode );
}


bool TA_TypeMap::ta_update( RDI_LocksHeld& held, const CosN::EventTypeSeq& added, const CosN::EventTypeSeq& deled, RDIProxySupplier* proxy, Filter_i* filter )
{
    bool is_changed = false;
    ProxyInfo prx_info( proxy, dynamic_cast<SequenceProxyPushSupplier_i*>(proxy), proxy->_proxy_id() );

    {
        TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", ""); // DO NOT cover 'RDI_TypeMap::update'

        m_prx_2_id_map[proxy] = proxy->_proxy_id();

        if ( added.length() && RDI_STR_EQ( added[0].type_name, "*" )  )
        {
            int location_key = get_location_key_from_filter( filter );

            if ( location_key != -1 )
            {
                if ( RDI_STR_EQ( added[0].domain_name, "*" ) )
                {
                    m_lk2prx_map[location_key].insert( prx_info );
                }
                else
                {
                    m_d2lk2prx_map[added[0].domain_name.in()][location_key].insert( prx_info );
                }

                is_changed = true;
            }
        }

        if ( deled.length() )
        {
            int location_key = -1;

            if ( false == m_lk2prx_map.empty() && ( RDI_STR_EQ( deled[0].domain_name, "*" ) && RDI_STR_EQ( deled[0].type_name, "*" ) ) )
            {
                location_key = remove_proxy( m_lk2prx_map, prx_info ) ;
            }
            else if ( ( false == m_d2lk2prx_map.empty() ) && RDI_STR_EQ( deled[0].type_name, "*" ) )
            {
                location_key = remove_proxy( m_d2lk2prx_map, prx_info, deled[0].domain_name.in() );
            }

            if ( location_key != -1 )
            {
                m_prx_2_id_map.erase( proxy );
                is_changed = true;
            }
        }
    }

    if ( false == is_changed )
    {
        m_type_map_1->update( held, added, deled, proxy, filter );
    }

    if ( false == is_changed )
    {
        TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", ""); // DO NOT cover 'RDI_TypeMap::update'

        if ( false == m_lk2prx_map.empty() || false == m_d2lk2prx_map.empty() )
        {
            update_prx_batch_push( prx_info );
        }
    }

    return m_type_map_2->update( held, added, deled, proxy, filter );
}


void TA_TypeMap::consumer_admin_dispatch_event(RDI_StructuredEvent* event)
{
    TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", "");

    if ( true == m_lk2prx_map.empty() && true == m_d2lk2prx_map.empty() )
    {
        return;
    }

    int location_key = get_location_key_from_event( event );

    if ( -1 == location_key )
    {
        return;
    }

    ProxySupplierList inconsistent_prx_list_1;
    ProxySupplierList inconsistent_prx_list_2;

    if ( false == m_lk2prx_map.empty() )
    {
        LocationKey2ProxyListMap::iterator find_location_it = m_lk2prx_map.find( location_key );

        if ( find_location_it != m_lk2prx_map.end() )
        {
            ProxySupplierList& prx_list = find_location_it->second;

            for ( ProxySupplierList::iterator it = prx_list.begin(); it != prx_list.end(); ++it )
            {
                if ( m_cadmin->_prx_batch_push.exists( it->prx_id ) )
                {
                    it->seq_prx->add_event(event);
                }
                else
                {
                    inconsistent_prx_list_1.insert( *it );
                }
            }
        }
    }

    if ( false == m_d2lk2prx_map.empty() )
    {
        Domain2LocationKey2ProxyListMap::iterator find_domain_it = m_d2lk2prx_map.find( event->get_domain_name() );

        if ( find_domain_it != m_d2lk2prx_map.end() )
        {
            LocationKey2ProxyListMap& lk2prx_map = find_domain_it->second;
            LocationKey2ProxyListMap::iterator find_location_it = lk2prx_map.find( location_key );

            if ( find_location_it != lk2prx_map.end() )
            {
                ProxySupplierList& prx_list = find_location_it->second;

                for ( ProxySupplierList::iterator it = prx_list.begin(); it != prx_list.end(); ++it )
                {
                    if ( m_cadmin->_prx_batch_push.exists( it->prx_id ) )
                    {
                        it->seq_prx->add_event(event);
                    }
                    else
                    {
                        inconsistent_prx_list_2.insert( *it );
                    }
                }
            }
        }
    }

    if ( false == inconsistent_prx_list_1.empty() )
    {
        remove_proxy( m_lk2prx_map, inconsistent_prx_list_1 );
    }

    if ( false == inconsistent_prx_list_2.empty() )
    {
        remove_proxy( m_d2lk2prx_map, inconsistent_prx_list_2, event->get_domain_name() );
    }
}


void TA_TypeMap::update_prx_batch_push( const ProxyInfo& prx_info )
{
    m_prx_batch_push.clear();
    m_is_prx_batch_push_changed = true;

    for ( RDI_HashCursor<CosN::EventType, RDI_TypeMap::VNode_t> curs = m_type_map_1->_tmap.cursor(); curs.is_valid(); curs++ )
    {
        for ( RDI_TypeMap::PNode_t* pnode = curs.val()._prxy; pnode; pnode = pnode->_next )
        {
            if ( m_cadmin->_prx_batch_push.exists( m_prx_2_id_map[ pnode->_prxy ] ) )
            {
                SequenceProxyPushSupplier_i* seq_push_proxy_supplier = dynamic_cast<SequenceProxyPushSupplier_i*>( pnode->_prxy );

                if ( seq_push_proxy_supplier != NULL )
                {
                    m_prx_batch_push.insert( seq_push_proxy_supplier->_proxy_id(), seq_push_proxy_supplier );
                }
            }
        }
    }
}


RDI_Hash<CosNA::ProxyID, SequenceProxyPushSupplier_i*>* TA_TypeMap::get_prx_batch_push()
{
    TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", "" );

    if ( true == m_lk2prx_map.empty() && true == m_d2lk2prx_map.empty() )
    {
        return NULL;
    }

    if ( true == m_is_prx_batch_push_changed )
    {
        m_prx_batch_push_2.clear();

        for ( RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i*> curs = m_prx_batch_push.cursor(); curs.is_valid(); ++curs )
        {
            m_prx_batch_push_2.insert( curs.key(), curs.val() );
        }

        m_is_prx_batch_push_changed = false;
    }

    {
        std::vector<CosNA::ProxyID> invalid_ids;

        for ( RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i*> curs = m_prx_batch_push_2.cursor(); curs.is_valid(); ++curs )
        {
            if ( ! m_cadmin->_prx_batch_push.exists( curs.key() ) )
            {
                invalid_ids.push_back( curs.key() );
            }
        }

        for ( size_t i = 0; i < invalid_ids.size(); ++i )
        {
            m_prx_batch_push_2.remove( invalid_ids[i] );
        }
    }

    return &m_prx_batch_push_2;
}


int TA_TypeMap::remove_proxy( LocationKey2ProxyListMap& lk2prx_map, const ProxyInfo& prx_info )
{
    for ( LocationKey2ProxyListMap::iterator it = lk2prx_map.begin(); it != lk2prx_map.end(); ++it )
    {
        unsigned long location_key = it->first;
        ProxySupplierList& prx_list = it->second;
        ProxySupplierList::iterator findIt = prx_list.find( prx_info );

        if ( findIt != prx_list.end() )
        {
            prx_list.erase( findIt );

            if ( true == prx_list.empty() )
            {
                lk2prx_map.erase( it );
            }

            return location_key; // the proxy has just one filter
        }
    }

    return -1;
}


int TA_TypeMap::remove_proxy( Domain2LocationKey2ProxyListMap& d2lk2prx_map, const ProxyInfo& prx_info, const char* domain_name )
{
    Domain2LocationKey2ProxyListMap::iterator find_domain_it = d2lk2prx_map.find( domain_name );

    if ( find_domain_it != d2lk2prx_map.end() )
    {
        LocationKey2ProxyListMap& lk2prx_map = find_domain_it->second;

        for ( LocationKey2ProxyListMap::iterator it = lk2prx_map.begin(); it != lk2prx_map.end(); ++it )
        {
            unsigned long location_key = it->first;
            ProxySupplierList& prx_list = it->second;
            ProxySupplierList::iterator find_proxy_it = prx_list.find( prx_info );

            if ( find_proxy_it != prx_list.end() )
            {
                prx_list.erase( find_proxy_it );

                if ( true == prx_list.empty() )
                {
                    lk2prx_map.erase( it );

                    if ( true == lk2prx_map.empty() )
                    {
                        d2lk2prx_map.erase( find_domain_it );
                    }
                }

                return location_key; // the proxy has just one filter
            }
        }
    }

    return -1;
}


void TA_TypeMap::remove_proxy( LocationKey2ProxyListMap& lk2prx_map, const ProxySupplierList& prx_list )
{
    for ( ProxySupplierList::const_iterator it = prx_list.begin(); it != prx_list.end(); ++it )
    {
        remove_proxy( lk2prx_map, *it );
    }
}


void TA_TypeMap::remove_proxy( Domain2LocationKey2ProxyListMap& d2lk2prx_map, const ProxySupplierList& prx_list, const char* domain_name )
{
    for ( ProxySupplierList::const_iterator it = prx_list.begin(); it != prx_list.end(); ++it )
    {
        remove_proxy( d2lk2prx_map, *it, domain_name );
    }
}


int TA_TypeMap::get_location_key_from_filter( Filter_i* filter ) // ( $Region == '123' )
{
    if ( NULL == filter )
    {
        return -1;
    }

    CosNF::ConstraintInfoSeq* all_constraints = filter->get_all_constraints();

    if ( NULL == all_constraints )
    {
        return -1;
    }

    for ( size_t i = 0; i < all_constraints->length(); ++i )
    {
        CosNF::ConstraintInfo& constraint_info = (*all_constraints)[i];
        CosNF::ConstraintExp& constraint_expression = constraint_info.constraint_expression;
        CosNotification::EventTypeSeq& event_types = constraint_expression.event_types;

        for ( size_t j = 0; j < event_types.length(); ++j )
        {
            if ( RDI_STR_EQ( event_types[j].type_name, "*" ) )
            {
                return get_location_key_from_filter_constraint_expr( constraint_expression.constraint_expr.in() );;
            }
        }
    }

    return -1;
}


int TA_TypeMap::get_location_key_from_filter_constraint_expr( const char* constraint_expr )
{
    static const char*  REGION_EXPRESSION = "$Region == '";   // Note: TA_CosUtility.cpp:gGenerateConstraintExpression
    static const char*  AND_OPERATION = " ) and ( ";
    static const size_t REGION_EXPRESSION_LENGTH = ::strlen(REGION_EXPRESSION);
    static const size_t MAX_NUMBER_LENGTH = 10;

    char* expr_beg = std::strstr( const_cast<char*>(constraint_expr), REGION_EXPRESSION );

    if ( NULL == expr_beg )
    {
        return -1;
    }

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

    return -1;
}


int TA_TypeMap::get_location_key_from_event( RDI_StructuredEvent* event )
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


RDIstrstream& TA_TypeMap::log_output(RDIstrstream& str)
{
    //Ref: RDI_TypeMap::log_output
    str << "----------\nTA_TypeMap\n----------\n";

    TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", "");

    if ( m_lk2prx_map.empty() && m_d2lk2prx_map.empty() )
    {
        str << "\t(no entries)\n";
        return str;
    }

    if ( false == m_lk2prx_map.empty() )
    {
        str << "*::* ( $Region == 'L' )";

        for ( LocationKey2ProxyListMap::iterator it = m_lk2prx_map.begin(); it != m_lk2prx_map.end(); ++it )
        {
            unsigned long location_key = it->first;
            ProxySupplierList& prx_list = it->second;

            str << "\n\tL ";
            str.setw(3); str << location_key;
            str << ": ";

            for ( ProxySupplierList::iterator proxy_it = prx_list.begin(); proxy_it != prx_list.end(); ++proxy_it )
            {
                str.setw(9); str << proxy_it->seq_prx;
            }
        }

        str << "\n";
    }

    for ( Domain2LocationKey2ProxyListMap::iterator it = m_d2lk2prx_map.begin(); it != m_d2lk2prx_map.end(); ++it )
    {
        const std::string& domain_name = it->first;
        LocationKey2ProxyListMap& lk2prx_map = it->second;

        str << domain_name.c_str() << "::* ( $Region == 'L' )";

        if ( lk2prx_map.empty() )
        {
            str << "\n\t(no entries)";
        }

        for ( LocationKey2ProxyListMap::iterator it = lk2prx_map.begin(); it != lk2prx_map.end(); ++it )
        {
            unsigned long location_key = it->first;
            ProxySupplierList& prx_list = it->second;

            str << "\n\tL ";
            str.setw(3); str << location_key;
            str << ": ";

            for ( ProxySupplierList::iterator proxy_it = prx_list.begin(); proxy_it != prx_list.end(); ++proxy_it )
            {
                str.setw(9); str << proxy_it->seq_prx;
            }
        }

        str << "\n";
    }

    return str;
}


#endif
