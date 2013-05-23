#ifndef LOCK_HELPER_H_INCLUDED
#define LOCK_HELPER_H_INCLUDED

#include "RDI.h"
#include <omnithread.h>
#include <string>
#include <map>


struct ThreadTimeStamp
{
    struct TimeStamp
    {
        unsigned long last_time_s, last_time_n;
        unsigned long current_time_s, current_time_n;

        unsigned long get_elapse_in_millisecond()
        {
            return get_difference_in_nanosecond( last_time_s, last_time_n, current_time_s, current_time_n ) / 1000000;
        }

        unsigned long get_elapse_in_nanosecond()
        {
            return get_difference_in_nanosecond( last_time_s, last_time_n, current_time_s, current_time_n );
        }

        unsigned long get_difference_in_nanosecond( unsigned long little_s, unsigned long little_n, unsigned long big_s, unsigned long big_n )
        {
            return ( ( 1000000000 * big_s + big_n ) - ( 1000000000 * little_s + little_n ) );
        }
    };


    static ThreadTimeStamp& instance()
    {
        static ThreadTimeStamp _instance;
        return _instance;
    }

    void set_curtime(const std::string& id = "")
    {
        TimeStamp& t = m_time_stamp_map[TW_ID()][id];
        TW_GET_TIME( &t.last_time_s, &t.last_time_n );
    }

    unsigned long get_elapse_in_millisecond(const std::string& id = "")
    {
        return get_elapse_in_nanosecond(id) / 1000000;
    }

    unsigned long get_elapse_and_set_cur_in_millisecond(const std::string& id = "")
    {
        return get_elapse_and_set_cur_in_nanosecond(id) / 1000000;
    }

    unsigned long get_elapse_in_nanosecond(const std::string& id = "")
    {
        TimeStamp& t = m_time_stamp_map[TW_ID()][id];
        TW_GET_TIME( &t.current_time_s, &t.current_time_n );
        return t.get_elapse_in_nanosecond();
    }

    unsigned long get_elapse_and_set_cur_in_nanosecond(const std::string& id = "")
    {
        TimeStamp& t = m_time_stamp_map[TW_ID()][id];
        TW_GET_TIME( &t.current_time_s, &t.current_time_n );
        unsigned long elapse = t.get_elapse_in_nanosecond();
        TW_GET_TIME( &t.last_time_s, &t.last_time_n );
        return elapse;
    }

private:

    ThreadTimeStamp()
    {
        const size_t MAX_THREAD_NUMBER = 5000;

        for ( size_t i = 0; i < MAX_THREAD_NUMBER; ++i )
        {
            TimeStamp& t = m_time_stamp_map[i][""];

            TW_GET_TIME( &t.last_time_s, &t.last_time_n );
            TW_GET_TIME( &t.current_time_s, &t.current_time_n );
        }
    }

    std::map< unsigned long, std::map<std::string, TimeStamp> > m_time_stamp_map;
};





#endif
