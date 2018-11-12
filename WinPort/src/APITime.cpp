#include <time.h>
#include "WinCompat.h"
#include "WinPort.h"
#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000
#define SECSPERDAY         86400
#define SECSPERHOUR        3600
#define SECSPERMIN         60
#define MINSPERHOUR        60
#define HOURSPERDAY        24
#define EPOCHWEEKDAY       1  /* Jan 1, 1601 was Monday */
#define DAYSPERWEEK        7
#define MONSPERYEAR        12
#define DAYSPERQUADRICENTENNIUM (365 * 400 + 97)
#define DAYSPERNORMALQUADRENNIUM (365 * 4 + 1)

/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKSPERSEC)
/* 1601 to 1980 is 379 years plus 91 leap days */
#define SECS_1601_TO_1980  ((379 * 365 + 91) * (ULONGLONG)SECSPERDAY)
#define TICKS_1601_TO_1980 (SECS_1601_TO_1980 * TICKSPERSEC)

static const int MonthLengths[2][MONSPERYEAR] =
{
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static inline BOOL IsLeapYear(int Year)
{
    return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0);
}


static void TM2Systemtime(LPSYSTEMTIME lpSystemTime, const struct tm *ptm)
{
	lpSystemTime->wSecond = ptm->tm_sec;
	lpSystemTime->wMinute = ptm->tm_min;
	lpSystemTime->wHour = ptm->tm_hour;
	lpSystemTime->wDay = ptm->tm_mday;
	lpSystemTime->wMonth = ptm->tm_mon + 1;
	lpSystemTime->wYear = ptm->tm_year + 1900;
	lpSystemTime->wDayOfWeek = ptm->tm_wday;
	lpSystemTime->wMilliseconds = 0;
}
		
static void Systemtime2TM(const SYSTEMTIME *lpSystemTime, struct tm *ptm)
{
	ptm->tm_sec = lpSystemTime->wSecond;
	ptm->tm_min = lpSystemTime->wMinute;
	ptm->tm_hour = lpSystemTime->wHour;
	ptm->tm_mday = lpSystemTime->wDay;
	ptm->tm_mon = lpSystemTime->wMonth - 1;
	ptm->tm_year = lpSystemTime->wYear - 1900;
	ptm->tm_wday = lpSystemTime->wDayOfWeek;
}


WINPORT_DECL(GetLocalTime, VOID, (LPSYSTEMTIME lpSystemTime))
{
	time_t now = time(NULL);
	TM2Systemtime(lpSystemTime, localtime(&now));
}

WINPORT_DECL(GetSystemTime, VOID, (LPSYSTEMTIME lpSystemTime))
{
	time_t now = time(NULL);
	TM2Systemtime(lpSystemTime, gmtime(&now));
}

WINPORT_DECL(FileTime_UnixToWin32, VOID, (struct timespec ts, FILETIME *lpFileTime))
{
	if (!lpFileTime) return;
	time_t tm = ts.tv_sec;
	time_t add_ns = ts.tv_nsec / 1000000000;
	if (add_ns) {
		tm+= add_ns;
		ts.tv_nsec-= add_ns * 1000000000;
	}
	
	SYSTEMTIME sys_time = {};
	TM2Systemtime(&sys_time, gmtime(&tm));
	sys_time.wMilliseconds+= ts.tv_nsec/1000000;
	WINPORT(SystemTimeToFileTime)(&sys_time, lpFileTime);
}

WINPORT_DECL(FileTime_Win32ToUnix, VOID, (const FILETIME *lpFileTime, struct timespec *ts))
{
	if (!lpFileTime || !ts) return;
	FILETIME ftm2 = {0};
	if (!WINPORT(FileTimeToLocalFileTime)(lpFileTime, &ftm2))
		return;

	SYSTEMTIME sys_time = {};
	WINPORT(FileTimeToSystemTime)(&ftm2, &sys_time);
	struct tm tm = {};	
	Systemtime2TM(&sys_time, &tm);
	ts->tv_sec = mktime(&tm);
	ts->tv_nsec = sys_time.wMilliseconds;
	ts->tv_nsec*= 1000000;
}

WINPORT_DECL(SystemTimeToFileTime, BOOL, (const SYSTEMTIME *lpSystemTime, LPFILETIME lpFileTime))
{
	int month, year, cleaps, day;

	/* FIXME: normalize the TIME_FIELDS structure here */
	/* No, native just returns 0 (error) if the fields are not */
	if(     lpSystemTime->wMilliseconds > 999 ||
		lpSystemTime->wSecond > 59 ||
		lpSystemTime->wMinute > 59 ||
		lpSystemTime->wHour > 23 ||
		lpSystemTime->wMonth < 1 || lpSystemTime->wMonth > 12 ||
		lpSystemTime->wDay < 1 ||
		lpSystemTime->wDay > MonthLengths [ lpSystemTime->wMonth ==2 || IsLeapYear(lpSystemTime->wYear)] [ lpSystemTime->wMonth - 1] ||
		lpSystemTime->wYear < 1601 ) return FALSE;

	/* now calculate a day count from the date
	* First start counting years from March. This way the leap days
	* are added at the end of the year, not somewhere in the middle.
	* Formula's become so much less complicate that way.
	* To convert: add 12 to the month numbers of Jan and Feb, and 
	* take 1 from the year */
	if(lpSystemTime->wMonth < 3) {
		month = lpSystemTime->wMonth + 13;
		year = lpSystemTime->wYear - 1;
	} else {
		month = lpSystemTime->wMonth + 1;
		year = lpSystemTime->wYear;
	}
	cleaps = (3 * (year / 100) + 3) / 4;   /* nr of "century leap years"*/
	day =  (36525 * year) / 100 - cleaps + /* year * dayperyr, corrected */
		(1959 * month) / 64 +         /* months * daypermonth */
		lpSystemTime->wDay -          /* day of the month */
		584817 ;                      /* zero that on 1601-01-01 */
	/* done */

	LARGE_INTEGER liTime;
	liTime.QuadPart = (((((LONGLONG) day * HOURSPERDAY + 
		lpSystemTime->wHour) * MINSPERHOUR + 
		lpSystemTime->wMinute) * SECSPERMIN +
		lpSystemTime->wSecond ) * 1000 +
		lpSystemTime->wMilliseconds ) * TICKSPERMSEC;

	lpFileTime->dwHighDateTime = liTime.HighPart;
	lpFileTime->dwLowDateTime = liTime.LowPart;
	return TRUE;
	/*
	struct tm tm = {0};
	tm.tm_sec = lpSystemTime->wSecond;
	tm.tm_min = lpSystemTime->wMinute;
	tm.tm_hour = lpSystemTime->wHour;
	tm.tm_mday = lpSystemTime->wDay;
	tm.tm_mon = lpSystemTime->wMonth - 1;
	tm.tm_year = lpSystemTime->wYear - 1900;
	//ignored tm.tm_wday
	//ignored tm.tm_yday
	time_t t = mktime(&tm);

	//adjust and add milliseconds:
	//time_t - number of seconds elapsed since 00:00 hours, Jan 1, 1970 UTC
	//LPFILETIME - number of 100-nanosecond intervals since January 1, 1601 (UTC).
	// - difference is 11,643,609,600 seconds

	ULARGE_INTEGER li;
	li.QuadPart = t;
	li.QuadPart+= SECS_1601_TO_1970;
	li.QuadPart*= 1000;
	li.QuadPart+= lpSystemTime->wMilliseconds;
	li.QuadPart*= 10000;
	lpFileTime->dwLowDateTime = li.LowPart;
	lpFileTime->dwHighDateTime = li.HighPart;

    return TRUE;*/
}


static void BiasFileTimeToFileTime(const FILETIME *lpSrc, LPFILETIME lpDst, bool from_local)
{
	time_t now = time(NULL);
	struct tm lt = { };
	localtime_r(&now, &lt);
	//local = utc + lt.tm_gmtoff
		
	ULARGE_INTEGER li;
	li.LowPart = lpSrc->dwLowDateTime;
	li.HighPart = lpSrc->dwHighDateTime;

	if (lt.tm_gmtoff > 0) {
		ULONGLONG diff = lt.tm_gmtoff;
		diff*= 10000000LL;
		if (from_local)
			li.QuadPart-= diff;
		else
			li.QuadPart+= diff;
	} else if (lt.tm_gmtoff < 0) {
		ULONGLONG diff = -lt.tm_gmtoff;
		diff*= 10000000LL;
		if (from_local)
			li.QuadPart+= diff;
		else
			li.QuadPart-= diff;
	}
	lpDst->dwLowDateTime = li.LowPart;
	lpDst->dwHighDateTime = li.HighPart;
}

WINPORT_DECL(LocalFileTimeToFileTime, BOOL, (const FILETIME *lpLocalFileTime, LPFILETIME lpFileTime))
{
	BiasFileTimeToFileTime(lpLocalFileTime, lpFileTime, true);
	return TRUE;
}

WINPORT_DECL(FileTimeToLocalFileTime, BOOL, (const FILETIME *lpFileTime, LPFILETIME lpLocalFileTime))
{
	BiasFileTimeToFileTime(lpFileTime, lpLocalFileTime, false);
	return TRUE;
}

WINPORT_DECL(CompareFileTime, LONG, (const FILETIME *lpFileTime1, const FILETIME *lpFileTime2))
{
    if (lpFileTime1 && !lpFileTime2)
		return 1;
    if (!lpFileTime1 && lpFileTime2)
		return -1;
    if (!lpFileTime1 && !lpFileTime2)
		return 0;
    if (lpFileTime1->dwHighDateTime > lpFileTime2->dwHighDateTime)
        return 1;
    if (lpFileTime1->dwHighDateTime < lpFileTime2->dwHighDateTime)
        return -1;
    if (lpFileTime1->dwLowDateTime > lpFileTime2->dwLowDateTime)
        return 1;
    if (lpFileTime1->dwLowDateTime < lpFileTime2->dwLowDateTime)
        return -1;
    return 0;
}

WINPORT_DECL(GetSystemTimeAsFileTime, VOID, (FILETIME *lpFileTime))
{
	SYSTEMTIME st;
	WINPORT(GetSystemTime)(&st);
	WINPORT(SystemTimeToFileTime)(&st, lpFileTime);
}

WINPORT_DECL(FileTimeToSystemTime, BOOL, (const FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime))
{
	ULARGE_INTEGER liTime;
	liTime.LowPart = lpFileTime->dwLowDateTime;
	liTime.HighPart = lpFileTime->dwHighDateTime;

	int SecondsInDay;
	long int cleaps, years, yearday, months;
	long int Days;
	LONGLONG Time;

	/* Extract millisecond from time and convert time into seconds */
	lpSystemTime->wMilliseconds =
		(WORD) (( liTime.QuadPart % TICKSPERSEC) / TICKSPERMSEC);
	Time = liTime.QuadPart / TICKSPERSEC;

	/* The native version of RtlTimeToTimeFields does not take leap seconds
	* into account */

	/* Split the time into days and seconds within the day */
	Days = Time / SECSPERDAY;
	SecondsInDay = Time % SECSPERDAY;

	/* compute time of day */
	lpSystemTime->wHour = (WORD) (SecondsInDay / SECSPERHOUR);
	SecondsInDay = SecondsInDay % SECSPERHOUR;
	lpSystemTime->wMinute = (WORD) (SecondsInDay / SECSPERMIN);
	lpSystemTime->wSecond = (WORD) (SecondsInDay % SECSPERMIN);

	/* compute day of week */
	lpSystemTime->wDayOfWeek = (WORD) ((EPOCHWEEKDAY + Days) % DAYSPERWEEK);

	/* compute year, month and day of month. */
	cleaps=( 3 * ((4 * Days + 1227) / DAYSPERQUADRICENTENNIUM) + 3 ) / 4;
	Days += 28188 + cleaps;
	years = (20 * Days - 2442) / (5 * DAYSPERNORMALQUADRENNIUM);
	yearday = Days - (years * DAYSPERNORMALQUADRENNIUM)/4;
	months = (64 * yearday) / 1959;
	/* the result is based on a year starting on March.
	* To convert take 12 from Januari and Februari and
	* increase the year by one. */
	if( months < 14 ) {
		lpSystemTime->wMonth = months - 1;
		lpSystemTime->wYear = years + 1524;
	} else {
		lpSystemTime->wMonth = months - 13;
		lpSystemTime->wYear = years + 1525;
	}
	/* calculation of day of month is based on the wonderful
	* sequence of INT( n * 30.6): it reproduces the 
	* 31-30-31-30-31-31 month lengths exactly for small n's */
	lpSystemTime->wDay = yearday - (1959 * months) / 64 ;
	return TRUE;
}


BOOLEAN WINAPI RtlTimeToSecondsSince1970( const LARGE_INTEGER *Time, LPDWORD Seconds )
{
    ULONGLONG tmp = Time->QuadPart / TICKSPERSEC - SECS_1601_TO_1970;
    if (tmp > 0xffffffff) return FALSE;
    *Seconds = tmp;
    return TRUE;
}

WINPORT_DECL(FileTimeToDosDateTime, BOOL, (const FILETIME *ft, LPWORD fatdate, LPWORD fattime))
{
    LARGE_INTEGER       li;
    ULONG               t;
    time_t              unixtime;
    struct tm*          tm;

    if (!fatdate || !fattime)
    {
        WINPORT(SetLastError)(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    li.u.LowPart = ft->dwLowDateTime;
    li.u.HighPart = ft->dwHighDateTime;
    if (!RtlTimeToSecondsSince1970( &li, &t ))
    {
        WINPORT(SetLastError)(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    unixtime = t;
    tm = gmtime( &unixtime );
    if (fattime)
        *fattime = (tm->tm_hour << 11) + (tm->tm_min << 5) + (tm->tm_sec / 2);
    if (fatdate)
        *fatdate = ((tm->tm_year - 80) << 9) + ((tm->tm_mon + 1) << 5)
                   + tm->tm_mday;
    return TRUE;
}

void WINAPI RtlSecondsSince1970ToTime( DWORD Seconds, LARGE_INTEGER *Time ) 
{
    Time->QuadPart = Seconds * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;
}


WINPORT_DECL(DosDateTimeToFileTime, BOOL, ( WORD fatdate, WORD fattime, LPFILETIME ft))
{
    struct tm newtm;
#ifndef HAVE_TIMEGM
    struct tm *gtm;
    time_t time1, time2;
#endif
    newtm.tm_sec = (fattime & 0x1f) * 2;
    newtm.tm_min = (fattime >> 5) & 0x3f;
    newtm.tm_hour = (fattime >> 11);
    newtm.tm_mday = (fatdate & 0x1f);
    newtm.tm_mon = ((fatdate >> 5) & 0x0f) - 1;
    newtm.tm_year = (fatdate >> 9) + 80;
    newtm.tm_isdst = -1;
#ifdef HAVE_TIMEGM
    RtlSecondsSince1970ToTime( timegm(&newtm), (LARGE_INTEGER *)ft );
#else
    time1 = mktime(&newtm);
    gtm = gmtime(&time1);
    time2 = mktime(gtm);
    RtlSecondsSince1970ToTime( 2*time1-time2, (LARGE_INTEGER *)ft );
#endif
    return TRUE;
}



WINPORT_DECL(GetTickCount, DWORD, ())
{
#ifdef _WIN32
	return ::GetTickCount();
#elif defined(__APPLE__)
    static mach_timebase_info_data_t g_timebase_info;
    if (g_timebase_info.denom == 0)
        mach_timebase_info(&g_timebase_info);
    return mach_absolute_time()*g_timebase_info.numer/g_timebase_info.denom/1000000u;
#else
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
	DWORD rv = spec.tv_sec;
	rv*= 1000;
	rv+= (DWORD)(spec.tv_nsec / 1000000);
	return rv;
#endif
}

WINPORT_DECL(Sleep, VOID, (DWORD dwMilliseconds))
{
#ifdef _WIN32
	::Sleep(dwMilliseconds);
#else
	DWORD seconds  = dwMilliseconds / 1000;
	if (seconds) {
		sleep(seconds);
		dwMilliseconds-= seconds * 1000;
	}
	usleep(dwMilliseconds * 1000);
#endif
}

static clock_t g_process_start_stamp = WINPORT(GetTickCount)();
SHAREDSYMBOL clock_t GetProcessUptimeMSec()
{
	clock_t now = WINPORT(GetTickCount)();
	return  (now - g_process_start_stamp);
}

