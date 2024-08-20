/*
 * SPDX-FileCopyrightText: 2024 Duncan Greatwood
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************
 * pist_timelog.h
 *
 * Utility to log start and end time of activities
 *
 * Do "#define PS_TIMINGS_DBG 1" before including file to activate timing debug
 *
 */

#ifndef INCLUDED_PS_TIMELOG_H
#define INCLUDED_PS_TIMELOG_H

#include <pistache/winornix.h>
#include <pistache/emosandlibevdefs.h> // For _IS_BSD

#include PIST_QUOTE(PST_CLOCK_GETTIME_HDR) // for clock_gettime and asctime
#include <stdio.h> // snprintf
#include <stdarg.h> // for vsnprintf

#include <string.h> // strlen

#include <map>
#include <mutex>


// ---------------------------------------------------------------------------

#include <pistache/pist_syslog.h>

#ifdef _IS_BSD
#define PS_STRLCPY(__dest, __src, __n) strlcpy(__dest, __src, __n)
#define PS_STRLCAT(__dest, __src, __size) strlcat(__dest, __src, __size)
#elif defined(__linux__)
#define PS_STRLCPY(__dest, __src, __n) strncpy(__dest, __src, __n)
#define PS_STRLCAT(__dest, __src, __size) strncat(__dest, __src, __size)
#elif defined(__APPLE__)
#define PS_STRLCPY(__dest, __src, __n) strlcpy(__dest, __src, __n)
#define PS_STRLCAT(__dest, __src, __size) strlcat(__dest, __src, __size)
#else
#define PS_STRLCPY(__dest, __src, __n) strcpy(__dest, __src)
#define PS_STRLCAT(__dest, __src, __size) strcat(__dest, __src)
#endif

#ifdef DEBUG
#define PS_TIMINGS_DBG 1
#endif

#ifdef PS_TIMINGS_DBG
// For C++ name demangling:
#ifdef _IS_WINDOWS
#pragma comment(lib, "dbghelp.lib")
extern char *__unDName(char*, const char*, int, void*, void*, int);
#else
#include <cxxabi.h> // for abi::__cxa_demangle
#endif


#endif

// ---------------------------------------------------------------------------

// Pointy delimiters (<...>) are default, others can be used
// Delimiters are repeated to indicate nesting (eg <<...>>)
// Note, in note DEBUG case, all these PS_TIMEDBG_START_xxx macros evaluate to
// nothing due to the non-DEBUG definition of PS_TIMEDBG_START_W_DELIMIT_CH
#define PS_TIMEDBG_START PS_TIMEDBG_START_POINTY
#define PS_TIMEDBG_START_POINTY PS_TIMEDBG_START_W_DELIMIT_CH('<')
#define PS_TIMEDBG_START_ROUND PS_TIMEDBG_START_W_DELIMIT_CH('(')
#define PS_TIMEDBG_START_SQUARE PS_TIMEDBG_START_W_DELIMIT_CH('[')
#define PS_TIMEDBG_START_CURLY PS_TIMEDBG_START_W_DELIMIT_CH('{')
#define PS_TIMEDBG_START_QUOTE PS_TIMEDBG_START_W_DELIMIT_CH('`')
#define PS_TIMEDBG_START_SLASH PS_TIMEDBG_START_W_DELIMIT_CH('\\')


#ifdef PS_TIMINGS_DBG

#define PS_TIMEDBG_START_W_DELIMIT_CH(PSDBG_DELIMIT_CH)                 \
    __PS_TIMEDBG __ps_timedbg(PSDBG_DELIMIT_CH,                         \
                                  __FILE__, __LINE__, __FUNCTION__, NULL);

#define PS_TIMEDBG_START_ARGS(__fmt, ...)                               \
    char ps_timedbg_buff[2048];                                         \
                                                                        \
    const char * ps_timedbg_inf =__PS_TIMEDBG::getInf(                  \
     &(ps_timedbg_buff[0]),sizeof(ps_timedbg_buff),__fmt, __VA_ARGS__); \
    __PS_TIMEDBG                                                        \
    __ps_timedbg('<', __FILE__, __LINE__, __FUNCTION__, ps_timedbg_inf)

#ifdef _IS_WINDOWS
// Note on __unDName from Wine:
//   Demangle a C++ identifier.
//
//   PARAMS
//    buffer   [O] If not NULL, the place to put the demangled string
//    mangled  [I] Mangled name of the function
//    buflen   [I] Length of buffer
//    memget   [I] Function to allocate memory with
//    memfree  [I] Function to free memory with
//    unknown  [?] Unknown, possibly a call back
//    flags    [I] Flags determining demangled format
//
//   RETURNS
//    Success: A string pointing to the unmangled name, allocated with memget.
//    (Pistache note - memget used solely if buffer null or buflen zero)
//    Failure: NULL.

#define GET__PTST_DEMANGLED                                             \
    char ptst_undecorated_name[2048+16];                                \
    ptst_undecorated_name[0] = 0;                                       \
    char * __ptst_demangled = __unDName(&(ptst_undecorated_name[0]),    \
                   typeid(*this).name()+1, 2048, malloc, free, 0x2800); 
#else
#define GET__PTST_DEMANGLED                                             \
    char * __ptst_demangled = abi::__cxa_demangle(                      \
        typeid(*this).name(), NULL, NULL, &__ptst_dem_status);
#endif // of ifdef _IS_WINDOWS ... else ...

// Same as PS_TIMEDBG_START but logs class name and "this" value
#define PS_TIMEDBG_START_THIS                                           \
    int __ptst_dem_status = 0;                                          \
    GET__PTST_DEMANGLED;                                                \
    char ps_timedbg_this_buff[2048];                                    \
    if ((__ptst_demangled) && (__ptst_dem_status == 0))                 \
        snprintf(&(ps_timedbg_this_buff[0]),                            \
                 sizeof(ps_timedbg_this_buff),                          \
                 "%s (this) %p", __ptst_demangled, (void *) this);      \
    else                                                                \
        snprintf(&(ps_timedbg_this_buff[0]),                            \
                 sizeof(ps_timedbg_this_buff),                          \
                 "this %p",  (void *) this);                            \
    if ((__ptst_demangled) && (__ptst_dem_status == 0))                 \
        free(__ptst_demangled);                                         \
    PS_TIMEDBG_START_ARGS("%s", &(ps_timedbg_this_buff[0]));

    
#define PS_TIMEDBG_START_STR(__str)                                     \
    PS_TIMEDBG_START_ARGS("%s", __str)

#define PS_TIMEDBG_GET_CTR                                              \
    (__ps_timedbg.getCounter())
    
// ---------------------------------------------------------------------------

class __PS_TIMEDBG
{
private:
    const char mMarkerChar;
    
    const char * mFileName;
    int mLineNum;
    const char * mFnName;
    
    struct PST_TIMESPEC mPsTimedbg;
    struct PST_TIMESPEC mPsTimeCPUdbg;

    static unsigned mUniCounter; // universal (static) counter
    unsigned mCounter; // individual counter for this __PS_TIMEDBG

    static std::map<PST_THREAD_ID, unsigned> mThreadMap;
    static std::mutex mThreadMapMutex;

    unsigned getThreadNextDepth(); // returns depth value after increment
    unsigned decrementThreadDepth(); // returns depth value before decrement
    
    void setMarkerChars(char * marker_chars, char the_marker,
                        unsigned call_depth)
        {
            if (call_depth > 20)
            {
                for(unsigned int i=0; i<10; i++)
                    marker_chars[i] = the_marker;
                PS_STRLCPY(&(marker_chars[10]), "...", 4);

                marker_chars += (10 + 3);
                for(unsigned int i=0; i<10; i++)
                    marker_chars[i] = the_marker;
                marker_chars[10 + 3 + 10] = 0;
            }
            else
            {
                for(unsigned int i=0; i<call_depth; i++)
                    marker_chars[i] = the_marker;
                marker_chars[call_depth] = 0;
            }
        }

    char reverseMarkerChar(char the_marker)
        {
            switch(the_marker)
            {
            case '<':
                return('>');

            case '(':
                return(')');

            case '[':
                return(']');

            case '{':
                return('}');

            case '`':
                return('\'');

            case '\\':
                return('/');

            default:
                return(the_marker);
            }
        }
    
                
    
    

public:
    static const char * getInf(char * _buff, unsigned _buffSize,
                               const char * _format, ...)
        {
            if (!_format)
                return(NULL);

            if ((!_buff) || (_buffSize < 6))
                return(NULL);

            unsigned int sizeof_buf = _buffSize - 6;
            char * buf_ptr = &(_buff[0]);
            
            va_list ap;
            va_start(ap, _format);
            
            // Temporarily disable -Wformat-nonliteral
            #ifdef __clang__
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wformat-nonliteral"
            #endif
            int ln = vsnprintf(buf_ptr, sizeof_buf, _format, ap);
            #ifdef __clang__
            #pragma clang diagnostic pop
            #endif
            
            va_end(ap);

            if (ln >= ((int) sizeof_buf))
                PS_STRLCAT(buf_ptr, "...", 5);

            return(buf_ptr);
        }
    
public:
    __PS_TIMEDBG(char marker_ch,
                 const char * f, int l, const char * m, const char * _inf) :
        mMarkerChar(marker_ch),
        mFileName(f), mLineNum(l), mFnName(m), mCounter(++mUniCounter)
        {
            const char * ps_time_str = "No-Time";
            char pschbuff[40];

            int res = PST_CLOCK_GETTIME(PST_CLOCK_PROCESS_CPUTIME_ID,
                                        &mPsTimeCPUdbg);
            if (res != 0)
                memset(&mPsTimeCPUdbg, 0, sizeof(mPsTimeCPUdbg));
            res = PST_CLOCK_GETTIME(PST_CLOCK_REALTIME, &mPsTimedbg);
            if (res == 0)
            {
                struct tm pstm;
                if (PST_GMTIME_R((const time_t *)(&mPsTimedbg.tv_sec),
                                 &pstm)!= NULL)
                {
                    if (PST_ASCTIME_R(&pstm, pschbuff) != NULL)
                    {
                        size_t pschbuff_len = strlen(pschbuff);
                        while((pschbuff_len > 0) &&
                              ((pschbuff[pschbuff_len-1] == '\n') ||
                               (pschbuff[pschbuff_len-1] == '\r')))
                            pschbuff_len--;
                        snprintf(&(pschbuff[pschbuff_len]),
                                 sizeof(pschbuff) - (pschbuff_len + 1),
                                 ",%03ldms", (mPsTimedbg.tv_nsec / 1000000));
                        ps_time_str = &(pschbuff[0]);
                    }
                }
            }

            char marker_chars[32];
            unsigned call_depth = getThreadNextDepth();
            setMarkerChars(&(marker_chars[0]), mMarkerChar, call_depth);
            
            if (_inf)
                PSLogFn(LOG_DEBUG, PS_LOG_AND_STDOUT,
                        mFileName, mLineNum, mFnName,
                        "%sCtr:%u %s [%s]", &(marker_chars[0]),
                        mCounter, ps_time_str, _inf);
            else
                PSLogFn(LOG_DEBUG, PS_LOG_AND_STDOUT,
                        mFileName, mLineNum, mFnName,
                        "%sCtr:%u %s", &(marker_chars[0]),
                        mCounter, ps_time_str);
        }

    ~__PS_TIMEDBG()
        {
            const char * ps_time_str = "No-Time";
            const char * ps_diff_time_str = "No-Time";
            char pschbuff[40];
            char ps_diff_chbuff[40];
            struct PST_TIMESPEC latest_ps_timedbg;
            struct PST_TIMESPEC latest_ps_time_cpu_dbg;
            int res = PST_CLOCK_GETTIME(PST_CLOCK_PROCESS_CPUTIME_ID,
                                        &latest_ps_time_cpu_dbg);
            if (res != 0)
                memset(&latest_ps_time_cpu_dbg, 0,
                       sizeof(latest_ps_time_cpu_dbg));
            res = PST_CLOCK_GETTIME(PST_CLOCK_REALTIME, &latest_ps_timedbg);

            if (res == 0)
            {
                struct tm pstm;
                if (PST_GMTIME_R((const time_t *)(&latest_ps_timedbg.tv_sec),
                                 &pstm) != NULL)
                {
                    if (PST_ASCTIME_R(&pstm, pschbuff) != NULL)
                    {
                        size_t pschbuff_len = strlen(pschbuff);
                        while((pschbuff_len > 0) &&
                              ((pschbuff[pschbuff_len-1] == '\n') ||
                               (pschbuff[pschbuff_len-1] == '\r')))
                            pschbuff_len--;
                        snprintf(&(pschbuff[pschbuff_len]),
                                 sizeof(pschbuff) - (pschbuff_len + 1),
                            ",%03ldms", (latest_ps_timedbg.tv_nsec / 1000000));
                        ps_time_str = &(pschbuff[0]);
                    }
                }

                int diff_sec =
                    (int)(latest_ps_timedbg.tv_sec - mPsTimedbg.tv_sec);
                if (diff_sec < 31536000)
                {
                    long diff_nsec =
                        latest_ps_timedbg.tv_nsec - mPsTimedbg.tv_nsec;

                    long diff_msec = (((long)diff_sec) *  1000) +
                                                         (diff_nsec / 1000000);

                    if ((diff_msec < 10) &&
                        ((mPsTimeCPUdbg.tv_nsec) || (mPsTimeCPUdbg.tv_sec)) &&
                        ((latest_ps_time_cpu_dbg.tv_nsec) ||
                                              (latest_ps_time_cpu_dbg.tv_sec)))
                    {
                        diff_sec = (int)(latest_ps_time_cpu_dbg.tv_sec -
                                                         mPsTimeCPUdbg.tv_sec);
                        diff_nsec = latest_ps_time_cpu_dbg.tv_nsec -
                                                         mPsTimeCPUdbg.tv_nsec;
                        long diff_usec = (((long)diff_sec) *  1000000) +
                                                            (diff_nsec / 1000);
                        diff_msec = diff_usec / 1000;
                        long diff_ms_thousandths = diff_usec % 1000;
                            
                        snprintf(&(ps_diff_chbuff[0]),
                             sizeof(ps_diff_chbuff) - 1,
                                 "%ld.%03ldms", diff_msec,diff_ms_thousandths);
                        
                    }
                    else
                    {
                        snprintf(&(ps_diff_chbuff[0]),
                                 sizeof(ps_diff_chbuff) - 1,
                                 "%ldms", diff_msec);
                    }
                    
                    ps_diff_time_str = &(ps_diff_chbuff[0]);
                }
            }

            char marker_chars[32];
            unsigned call_depth = decrementThreadDepth();
            setMarkerChars(&(marker_chars[0]), reverseMarkerChar(mMarkerChar),
                           call_depth);
            
            PSLogFn(LOG_DEBUG, PS_LOG_AND_STDOUT,
                mFileName, mLineNum, mFnName,
                "%s diff=%s ctr:%u%s", ps_time_str, ps_diff_time_str, mCounter,
                &(marker_chars[0]));
        }

    unsigned getCounter() {return(mCounter);}
};

// ---------------------------------------------------------------------------

#else // of ifdef PS_TIMINGS_DBG

#define PS_TIMEDBG_START_W_DELIMIT_CH(PSDBG_DELIMIT_CH)

#define PS_TIMEDBG_START_ARGS(__fmt, ...) 

#define PS_TIMEDBG_START_THIS

#endif

// ---------------------------------------------------------------------------

#endif // of ifndef INCLUDED_PS_TIMELOG_H

// ---------------------------------------------------------------------------

