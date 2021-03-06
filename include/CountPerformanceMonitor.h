#ifndef COUNT_PERFORMANCE_MONITOR_H_INCLUDED
#define COUNT_PERFORMANCE_MONITOR_H_INCLUDED


#include "Thread.h"
#include "ThreadGuard.h"
#include "RDI.h"
#include <map>
#include <sstream>


struct CountPerformanceMonitor : public Thread
{
public:

    static CountPerformanceMonitor& instance()
    {
        static CountPerformanceMonitor _instance;
        return _instance;
    }

    void add_count( const std::string& name, unsigned long number )
    {
        THREAD_GUARD( m_lock );
        m_count_map[name] += number;
    }

    void report()
    {
        std::stringstream strm;

        {
            THREAD_GUARD( m_lock );

            for ( std::map<std::string, unsigned long>::iterator it = m_count_map.begin(); it != m_count_map.end(); ++it )
            {
                strm << "\t" << it->first << ":\t" << it->second / (double)m_interval_in_seconds << std::endl;
                it->second = 0;
            }
        }

        if ( false == m_count_map.empty() )
        {
            RDIDbgForceLog( "\n----------- Performance Report - Count Per Second (Report Interval=" << m_interval_in_seconds << "s) ----------" << " \n" << strm.str().c_str() );
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

    CountPerformanceMonitor( int interval_in_seconds = 10 )
        : m_interval_in_seconds(interval_in_seconds)
    {
        start();
    }

private:

    unsigned long m_interval_in_seconds;
    omni_mutex m_lock;
    std::map<std::string, unsigned long> m_count_map;
};







#endif
