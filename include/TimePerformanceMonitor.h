#ifndef TIME_PERFORMANCE_MONITOR_H_INCLUDED
#define TIME_PERFORMANCE_MONITOR_H_INCLUDED


#include "Thread.h"
#include "ThreadGuard.h"
#include <map>
#include <sstream>


struct TimePerformanceMonitor : public Thread
{
public:

    static TimePerformanceMonitor& instance()
    {
        static TimePerformanceMonitor _instance;
        return _instance;
    }

    void add_time( const std::string& name, unsigned long number )
    {
        THREAD_GUARD( m_lock );
        m_count_map[name] += number;
    }

    void report()
    {
        std::stringstream strm;

        {
            THREAD_GUARD( m_lock );

            for ( std::map<std::string, double>::iterator it = m_count_map.begin(); it != m_count_map.end(); ++it )
            {
                strm << it->first << ":" << ( it->second / 1000000 ) / m_interval_in_seconds << std::endl;
                it->second = 0;
            }
        }

        if ( false == m_count_map.empty() )
        {
            RDIDbgForceLog( "\n ----- TimePerformanceMonitor - time(in millisecond) used per second -----" << " \n" << strm.str().c_str() );
        }
    }

    virtual void run()
    {
        while ( true )
        {
            Thread::sleep( m_interval_in_seconds * 1000 );

            report();
        }
    }

    virtual void terminate() {;}

private:

    TimePerformanceMonitor( int interval_in_seconds = 10 )
        : m_interval_in_seconds(interval_in_seconds)
    {
        start();
    }

private:

    unsigned long m_interval_in_seconds;
    omni_mutex m_lock;
    std::map<std::string, double> m_count_map;
};


struct TimeGuard
{
    TimeGuard( const std::string& key )
        : m_key( key )
    {
        TW_GET_TIME( &begin_time_s, &begin_time_n );
    }

    ~TimeGuard()
    {
        unsigned long cur_time_s, cur_time_n;
        TW_GET_TIME( &cur_time_s, &cur_time_n );
        unsigned long time_elapsed = get_difference_in_nanosecond( begin_time_s, begin_time_n, cur_time_s, cur_time_n );
        TimePerformanceMonitor::instance().add_time( m_key, time_elapsed );
    }

private:

    unsigned long get_difference_in_nanosecond( unsigned long little_s, unsigned long little_n, unsigned long big_s, unsigned long big_n )
    {
        return ( ( 1000000000 * big_s + big_n ) - ( 1000000000 * little_s + little_n ) );
    }

private:

    std::string m_key;
    unsigned long begin_time_s, begin_time_n;
};


#define CAT(prefix, suffix) prefix ## suffix
#define TIME_GUARD( name, key ) TimeGuard CAT(time_guard, name) ( key )



#endif
