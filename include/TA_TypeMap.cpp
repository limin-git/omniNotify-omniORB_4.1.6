#ifndef TA_TYPE_MAP_CPP_INCLUDED
#define TA_TYPE_MAP_CPP_INCLUDED

#include "TA_TypeMap.h"
#include "RDITypeMap.h"
#include "RDILocksHeld.h"
#include <iostream>
#include <algorithm>


TA_TypeMap::TA_TypeMap()
    : m_channel(NULL),
      m_type_map_1(NULL),
      m_type_map_2(NULL),
      _prx_batch_push(RDI_ULongHash, RDI_ULongRank),
      _prx_batch_push_2(RDI_ULongHash, RDI_ULongRank),
      m_is_prx_batch_push_changed( false )
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


bool TA_TypeMap::ta_update( RDI_LocksHeld& held, const CosN::EventTypeSeq& added, const CosN::EventTypeSeq& deled, RDIProxySupplier* proxy, Filter_i* filter )
{
#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_DEBUG
    CosNA::ChannelID channel_id = m_channel->MyID();
    CosNA::ProxyID proxy_id = proxy->_proxy_id();
    unsigned long filter_id = filter->getID();

    std::stringstream add_del_strm;
    std::stringstream filter_strm;
    std::stringstream add_proxy_strm;
    std::stringstream remove_proxy_strm;
    std::stringstream type_map_strm;
    bool is_need_output_a_newline = true;

    add_del_strm<< "\n\t" << "added: ";
    get_event_type_list_str( added, add_del_strm );

    add_del_strm << "\n\t" << "deled: ";
    get_event_type_list_str( deled, add_del_strm );

    filter_strm << "\n\t" << "filter: ";
    get_filter_str( filter, filter_strm );
#endif

    bool is_TA_TypeMap_updated = false;

    m_proxy_id_map[proxy] = proxy->_proxy_id();
    m_proxy_id_map[ dynamic_cast<SequenceProxyPushSupplier_i*>(proxy) ] = proxy->_proxy_id();

    if ( added.length() )
    {
        if ( RDI_STR_EQ( added[0].domain_name, "*" ) && RDI_STR_EQ( added[0].type_name, "*" ) )
        {
            int location_key = get_location_key_from_filter( filter );

            if ( location_key != -1 )
            {
                {
                    TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", "");
                    m_location_key_2_proxy_list_map[location_key].insert( dynamic_cast<SequenceProxyPushSupplier_i*>(proxy) );
                    is_TA_TypeMap_updated = true;
                }

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_DEBUG
                is_need_output_a_newline = false;
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
            int location_key = get_location_key_from_filter( filter );

            if ( location_key != -1 )
            {
                {
                    TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", "");
                    m_domain_2_location_key_2_proxy_list_map[added[0].domain_name.in()][location_key].insert( dynamic_cast<SequenceProxyPushSupplier_i*>(proxy) );
                    is_TA_TypeMap_updated = true;
                }

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_DEBUG
                is_need_output_a_newline = false;
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
        if ( false == m_location_key_2_proxy_list_map.empty() && ( RDI_STR_EQ( deled[0].domain_name, "*" ) && RDI_STR_EQ( deled[0].type_name, "*" ) ) )
        {
            TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", "");

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
                    }

                    is_TA_TypeMap_updated = true;

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_DEBUG
                    is_need_output_a_newline = false;
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
        else if ( ( false == m_domain_2_location_key_2_proxy_list_map.empty() ) && RDI_STR_EQ( deled[0].type_name, "*" ) )
        {
            TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", "");

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
                        }

                        is_TA_TypeMap_updated = true;

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_DEBUG
                        is_need_output_a_newline = false;
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

    if ( false == is_TA_TypeMap_updated )
    {
#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_DEBUG
        is_need_output_a_newline = false;
        type_map_strm << "\n\t" << "TypeMap: ";
#endif

        m_type_map_1->update( held, added, deled, proxy, filter );

        // update proxy list for consumer admin
        if ( false == m_location_key_2_proxy_list_map.empty() || false == m_domain_2_location_key_2_proxy_list_map.empty() )
        {
            TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", "");

            _prx_batch_push.clear();
            m_is_prx_batch_push_changed = true;

            RDI_Hash<CosN::EventType, RDI_TypeMap::VNode_t>& _tmap = m_type_map_1->_tmap;

            for ( RDI_HashCursor<CosN::EventType, RDI_TypeMap::VNode_t> curs = _tmap.cursor(); curs.is_valid(); curs++ )
            {
                RDI_TypeMap::PNode_t* pnode = curs.val()._prxy;

                while ( pnode )
                {
                    if ( proxy->_myadmin->_prx_batch_push.exists( m_proxy_id_map[ pnode->_prxy ] ) )
                    {
                        SequenceProxyPushSupplier_i* seq_push_proxy_supplier = dynamic_cast<SequenceProxyPushSupplier_i*>( pnode->_prxy );

                        if ( seq_push_proxy_supplier != NULL )
                        {
                            _prx_batch_push.insert( seq_push_proxy_supplier->_proxy_id(), seq_push_proxy_supplier );
                        }
                    }
#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_DEBUG
                    else
                    {
                        RDIDbgForceLog( "[WARNING] a proxy in TypeMap but not in ConsumerAdmin: "
                            << "[channel=" << channel_id << "], [proxy=" << proxy_id << "] - "
                            << "[channel=" << channel_id << "], [proxy=" <<  m_proxy_id_map[ pnode->_prxy ] << "] \n" );
                    }

                    if ( false == _prx_batch_push.exists( m_proxy_id_map[ pnode->_prxy ] ) )
                    {
                        type_map_strm << proxy->_proxy_id() << ":" << proxy << ", ";
                    }
#endif
                    pnode = pnode->_next;
                }
            }
        }

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_DEBUG
        type_map_strm << "length=" << _prx_batch_push.length() << "\n";
#endif
    }

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_DEBUG
    RDIDbgForceLog( "TA_TypeMap::ta_update - " << "[channel=" << channel_id << "], [proxy=" << proxy_id << "], [filter=" << filter_id << "], [added=" << added.length() << ",deled=" << deled.length() << "]"
        << add_del_strm.str().c_str()
        << filter_strm.str().c_str()
        << add_proxy_strm.str().c_str()
        << remove_proxy_strm.str().c_str()
        << type_map_strm.str().c_str()
        << ( is_need_output_a_newline ? "\n" : "" )
        );
#endif

    bool result = m_type_map_2->update( held, added, deled, proxy, filter );

#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_DEBUG
    check();
#endif

    return result;
}


void TA_TypeMap::consumer_admin_dispatch_event(RDI_StructuredEvent* event, ConsumerAdmin_i* cadmin)
{
    if ( false == m_location_key_2_proxy_list_map.empty() || false == m_domain_2_location_key_2_proxy_list_map.empty() )
    {
        int location_key = get_location_key_from_event( event );

        if ( -1 == location_key )
        {
            return;
        }

        TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", "");

        if ( false == m_location_key_2_proxy_list_map.empty() )
        {
            LocationKey2ProxySupplierListMap::iterator find_location_it = m_location_key_2_proxy_list_map.find( location_key );

            if ( find_location_it != m_location_key_2_proxy_list_map.end() )
            {
                ProxySupplierList& proxy_list = find_location_it->second;

                for ( ProxySupplierList::iterator it = proxy_list.begin(); it != proxy_list.end(); ++it )
                {
                    SequenceProxyPushSupplier_i* proxy = *it;

                    if ( cadmin->_prx_batch_push.exists( m_proxy_id_map[proxy] ) )
                    {
                        proxy->add_event(event);
                    }
#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_DEBUG
                    else
                    {
                        RDIDbgForceLog( "[WARNING] a proxy in TA_TypeMap but not in ConsumerAdmin: [channel=" << m_channel->MyID() << "], [proxy=" <<  m_proxy_id_map[proxy] << "] \n" );
                    }
#endif
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

                        if ( cadmin->_prx_batch_push.exists( m_proxy_id_map[proxy] ) )
                        {
                            proxy->add_event(event);
                        }
#ifdef USE_TA_TYPE_MAPPING_IN_EVENT_CHANNEL_DEBUG
                        else
                        {
                            RDIDbgForceLog( "[WARNING] a proxy in TA_TypeMap but not in ConsumerAdmin: [channel=" << m_channel->MyID() << "], [proxy=" <<  m_proxy_id_map[proxy] << "] \n" );
                        }
#endif
                    }
                }
            }
        }
    }
}


RDIstrstream& TA_TypeMap::log_output(RDIstrstream& str)
{
    //Ref: RDI_TypeMap::log_output
    str << "----------\nTA_TypeMap\n----------\n";

    TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", "");

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


int TA_TypeMap::get_location_key_from_filter( Filter_i* filter ) // ( $Region == '123' )
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
                        return get_location_key_from_filter_constraint_expr( constraint_expression.constraint_expr.in() );;
                    }
                }
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


RDI_Hash<CosNA::ProxyID, SequenceProxyPushSupplier_i *>* TA_TypeMap::get_prx_batch_push()
{
    if ( false == m_location_key_2_proxy_list_map.empty() || false == m_domain_2_location_key_2_proxy_list_map.empty() )
    {
        if ( true == m_is_prx_batch_push_changed )
        {
            TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", "" );

            for ( RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i *> curs = _prx_batch_push.cursor(); curs.is_valid(); ++curs )
            {
                _prx_batch_push_2.insert( curs.key(), curs.val() );
            }

            m_is_prx_batch_push_changed = false;
        }

        return &_prx_batch_push_2;
    }

    return NULL;
}


void TA_TypeMap::check()
{
    if ( m_location_key_2_proxy_list_map.empty() && m_domain_2_location_key_2_proxy_list_map.empty() )
    {
        return;
    }

    TW_SCOPE_LOCK(ta_type_map_lock, m_lock, "", "");

    typedef std::set<RDIProxySupplier*> ProxySupplierList;

    ProxySupplierList list_1;
    ProxySupplierList list_2;
    ProxySupplierList list_3;
    ProxySupplierList list_123;
    ProxySupplierList list_4;

    for ( LocationKey2ProxySupplierListMap::iterator it = m_location_key_2_proxy_list_map.begin(); it != m_location_key_2_proxy_list_map.end(); ++it )
    {
        list_1.insert( it->second.begin(), it->second.end() );
        list_123.insert( it->second.begin(), it->second.end() );
    }

    for ( Domain2LocationKey2ProxySupplierListMap ::iterator it = m_domain_2_location_key_2_proxy_list_map.begin(); it != m_domain_2_location_key_2_proxy_list_map.end(); ++it )
    {
        LocationKey2ProxySupplierListMap& the_map = it->second;

        for ( LocationKey2ProxySupplierListMap::iterator it2 = the_map.begin(); it2 != the_map.end(); ++it2 )
        {
            list_2.insert( it2->second.begin(), it2->second.end() );
            list_123.insert( it2->second.begin(), it2->second.end() );
        }
    }

    {
        RDI_Hash<CosN::EventType, RDI_TypeMap::VNode_t>& _tmap = m_type_map_1->_tmap;

        for ( RDI_HashCursor<CosN::EventType, RDI_TypeMap::VNode_t> curs = _tmap.cursor(); curs.is_valid(); curs++ )
        {
            RDI_TypeMap::PNode_t* pnode = curs.val()._prxy;

            while ( pnode )
            {
                list_3.insert( pnode->_prxy );
                list_123.insert( pnode->_prxy );
                pnode = pnode->_next;
            }
        }
    }

    {
        RDI_Hash<CosN::EventType, RDI_TypeMap::VNode_t>& _tmap = m_type_map_2->_tmap;

        for ( RDI_HashCursor<CosN::EventType, RDI_TypeMap::VNode_t> curs = _tmap.cursor(); curs.is_valid(); curs++ )
        {
            RDI_TypeMap::PNode_t* pnode = curs.val()._prxy;

            while ( pnode )
            {
                list_4.insert( pnode->_prxy );
                pnode = pnode->_next;
            }
        }
    }

    RDI_Assert( std::includes( list_4.begin(), list_4.end(), list_1.begin(), list_1.end() ), "TA_TypeMap::check failed." );
    RDI_Assert( std::includes( list_4.begin(), list_4.end(), list_2.begin(), list_2.end() ), "TA_TypeMap::check failed." );
    RDI_Assert( std::includes( list_4.begin(), list_4.end(), list_3.begin(), list_3.end() ), "TA_TypeMap::check failed." );
    RDI_Assert( list_123 == list_4, "TA_TypeMap::check failed." );
    RDI_Assert( list_4.size() == list_1.size() + list_2.size() + list_3.size(), "TA_TypeMap::check failed." );
}


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
