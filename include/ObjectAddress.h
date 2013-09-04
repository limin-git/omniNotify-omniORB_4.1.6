#ifndef OBJECT_ADDRESS_H_INCLUDED
#define OBJECT_ADDRESS_H_INCLUDED

#include <omniORB4/CORBA.h>
#include "omniorb_poa_wrappers.h"
#include <sstream>


class ObjectAddress
{
public:

    static const std::string& get_object_address_str( CORBA::Object_ptr obj )
    {
        typedef std::map<CORBA::Object_ptr, std::string> CacheMap;
        static CacheMap cache;
        static CacheMap cache2;
        static omni_mutex cache_lock;

        cache_lock.acquire();

        CacheMap::iterator find_it = cache.find( obj );

        if ( find_it == cache.end() )
        {
            if ( 100000 <= cache.size() )
            {
                cache2.clear();
                cache.swap( cache2 );
            }

            std::string host;
            unsigned long port;
            std::stringstream strm;

            if ( true == get_object_address( obj, host, port ) )
            {
                strm << host << ":" << port;
                find_it = cache.insert( CacheMap::value_type( obj, strm.str() ) ).first;
            }
            else
            {
                cache_lock.release();

                static const std::string empty;
                return empty;
            }
        }

        cache_lock.release();
        return find_it->second;
    }

    static bool get_object_address( CORBA::Object_ptr obj, std::string& host, unsigned long& port )
    {
        CORBA::String_var str_ior =  WRAPPED_ORB_OA::orb()->object_to_string( obj );

        if ( strlen(str_ior) == 0 )
        {
            return false;
        }

        IOP::IOR ior;

        try
        {
            string_to_ior( str_ior, ior );

            if ( ior.profiles.length() == 0 && strlen(ior.type_id) == 0 )
            {
                return false;
            }

            for ( unsigned long count = 0; count < ior.profiles.length(); ++count )
            {
                if ( ior.profiles[count].tag == IOP::TAG_INTERNET_IOP)
                {
                    IIOP::ProfileBody pBody;
                    IIOP::unmarshalProfile( ior.profiles[count], pBody );

                    host = (const char*) pBody.address.host;
                    port = pBody.address.port;

                    return true;
                }
            }
        }
        catch ( CORBA::MARSHAL& )
        {
            NULL;
        }
        catch ( ... )
        {
            NULL;
        }

        return false;
    }

    static void string_to_ior( const char* iorstr, IOP::IOR& ior )
    {
        size_t s = ( iorstr ? strlen(iorstr) : 0 );

        if ( s < 4 )
        {
            throw CORBA::MARSHAL(0, CORBA::COMPLETED_NO);
        }

        const char *p = iorstr;

        if ( p[0] != 'I' || p[1] != 'O' || p[2] != 'R' || p[3] != ':')
        {
            throw CORBA::MARSHAL(0,CORBA::COMPLETED_NO);
        }

        s = (s - 4) / 2;  // how many octets are there in the string
        p += 4;

        cdrMemoryStream buf((CORBA::ULong)s, 0);

        for ( int i = 0; i < (int)s; ++i )
        {
            int j = i * 2;
            CORBA::Octet v;

            if ( p[j] >= '0' && p[j] <= '9')
            {
                v = ((p[j] - '0') << 4);
            }
            else if ( p[j] >= 'a' && p[j] <= 'f')
            {
                v = ((p[j] - 'a' + 10) << 4);
            }
            else if ( p[j] >= 'A' && p[j] <= 'F')
            {
                v = ((p[j] - 'A' + 10) << 4);
            }
            else
            {
                throw CORBA::MARSHAL(0,CORBA::COMPLETED_NO);
            }

            if ( p[j+1] >= '0' && p[j+1] <= '9')
            {
                v += (p[j+1] - '0');
            }
            else if ( p[j+1] >= 'a' && p[j+1] <= 'f')
            {
                v += (p[j+1] - 'a' + 10);
            }
            else if ( p[j+1] >= 'A' && p[j+1] <= 'F')
            {
                v += (p[j+1] - 'A' + 10);
            }
            else
            {
                throw CORBA::MARSHAL(0,CORBA::COMPLETED_NO);
            }

            buf.marshalOctet(v);
        }

        buf.rewindInputPtr();
        CORBA::Boolean b = buf.unmarshalBoolean();
        buf.setByteSwapFlag(b);

        ior.type_id = IOP::IOR::unmarshaltype_id(buf);
        ior.profiles <<= buf;
    }

};


#endif
