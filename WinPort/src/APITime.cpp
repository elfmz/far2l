#include <time.h>
#include <errno.h>
#include "WinCompat.h"
#include "WinPort.h"
#ifdef __APPLE__
#include <mach/mach_time.h>
#endif
#include <cctweaks.h>

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
#define DAYSPERQUADRICENTENNIUM  (365 * 400 + 97)
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

	SYSTEMTIME sys_time = {};
	WINPORT(FileTimeToSystemTime)(lpFileTime, &sys_time);
	struct tm tm = {};	
	Systemtime2TM(&sys_time, &tm);
	ts->tv_sec = timegm(&tm);
	ts->tv_nsec = sys_time.wMilliseconds;
	ts->tv_nsec*= 1000000;
}

WINPORT_DECL(SystemTimeToFileTime, BOOL, (const SYSTEMTIME *lpSystemTime, LPFILETIME lpFileTime))
{
	int month, year, cleaps, day;

	/* FIXME: normalize the TIME_FIELDS structure here */
	/* No, native just returns 0 (error) if the fields are not */
	if( lpSystemTime->wMilliseconds > 999 ||
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

static int LocalMinusUTC()
{
	time_t now = time(NULL);
	struct tm gt{}, lt{};
	gmtime_r(&now, &gt);
	localtime_r(&now, &lt);

	unsigned long long gt_secs = ((gt.tm_yday * 24 + gt.tm_hour) * 60 + gt.tm_min) * 60 + gt.tm_sec;
	unsigned long long lt_secs = ((lt.tm_yday * 24 + lt.tm_hour) * 60 + lt.tm_min) * 60 + lt.tm_sec;
	if (gt.tm_year > lt.tm_year) {
		gt_secs+= 366 * 24 * 60 * 60;

	} else if (gt.tm_year < lt.tm_year) {
		lt_secs+= 366 * 24 * 60 * 60;
	}

	int bias = (int)(long long)(lt_secs - gt_secs);

	if (gt.tm_year != lt.tm_year) {
		// there was workaround with adding leap year seconds count
		// so round bias by one minute to eliminate artifacts caused by it
		int abs_bias = abs(bias);
		int leap_fixup = abs_bias % 60;
		if (leap_fixup != 0) {
			if (leap_fixup <= 30) {
				abs_bias-= leap_fixup;
			} else {
				abs_bias+= 60 - leap_fixup;
			}
			bias = (bias < 0) ? -abs_bias : abs_bias;
		}
	}

	return bias;
}

static void BiasFileTimeToFileTime(const FILETIME *lpSrc, LPFILETIME lpDst, bool from_local)
{
	ULARGE_INTEGER li;
	li.LowPart = lpSrc->dwLowDateTime;
	li.HighPart = lpSrc->dwHighDateTime;

	int loc_minus_utc = LocalMinusUTC();

	if (loc_minus_utc > 0) {
		ULONGLONG diff = (unsigned int)loc_minus_utc;
		diff*= 10000000LL;
		if (from_local)
			li.QuadPart-= diff;
		else
			li.QuadPart+= diff;

	} else if (loc_minus_utc < 0) {
		ULONGLONG diff = (unsigned int)-loc_minus_utc;
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

WINPORT_DECL(FileTimeToDosDateTime, BOOL, (const FILETIME *ft, LPWORD fatdate, LPWORD fattime))
{
	if (UNLIKELY(!fatdate || !fattime))
	{
		WINPORT(SetLastError)(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	uint64_t xtm = ft->dwHighDateTime;
	xtm<<= 32;
	xtm|= ft->dwLowDateTime;
	time_t unixtime = xtm / TICKSPERSEC - SECS_1601_TO_1970;
	struct tm *tm = gmtime( &unixtime );
	if (fattime)
		*fattime = (tm->tm_hour << 11) + (tm->tm_min << 5) + (tm->tm_sec / 2);
	if (fatdate)
		*fatdate = ((tm->tm_year - 80) << 9) + ((tm->tm_mon + 1) << 5)
			+ tm->tm_mday;
	return TRUE;
}

WINPORT_DECL(DosDateTimeToFileTime, BOOL, ( WORD fatdate, WORD fattime, LPFILETIME ft))
{
	struct tm newtm;
	newtm.tm_sec = (fattime & 0x1f) * 2;
	newtm.tm_min = (fattime >> 5) & 0x3f;
	newtm.tm_hour = (fattime >> 11);
	newtm.tm_mday = (fatdate & 0x1f);
	newtm.tm_mon = ((fatdate >> 5) & 0x0f) - 1;
	newtm.tm_year = (fatdate >> 9) + 80;
	newtm.tm_isdst = -1;

	uint64_t xtm = timegm(&newtm);
	xtm*= TICKSPERSEC;
	xtm+= TICKS_1601_TO_1970;
	ft->dwLowDateTime = DWORD(xtm & 0xffffffff);
	ft->dwHighDateTime = DWORD(xtm >> 32);
	return TRUE;
}


static unsigned s_time_failmask = 0;

WINPORT_DECL(GetTickCount, DWORD, ())
{
	(void)s_time_failmask;
#ifdef _WIN32
	return ::GetTickCount();
#elif defined(__APPLE__)
	static mach_timebase_info_data_t g_timebase_info;
	if (g_timebase_info.denom == 0)
		mach_timebase_info(&g_timebase_info);
	return mach_absolute_time() * g_timebase_info.numer / g_timebase_info.denom / 1000000u;
#else

#if defined(CLOCK_MONOTONIC_COARSE) || defined(CLOCK_REALTIME_COARSE)
	if (LIKELY((s_time_failmask & 1) == 0)) {
		struct timespec spec{};
# ifdef CLOCK_MONOTONIC_COARSE
		if (LIKELY(clock_gettime(CLOCK_MONOTONIC_COARSE, &spec) == 0)) {
# else
		if (LIKELY(clock_gettime(CLOCK_REALTIME_COARSE, &spec) == 0)) {
# endif
			DWORD rv = spec.tv_sec;
			rv*= 1000;
			rv+= (DWORD)(spec.tv_nsec / 1000000);
			return rv;
		}
		fprintf(stderr, "%s: clock_gettime error %u\n", __FUNCTION__, errno);
		s_time_failmask|= 1;
	}
#endif
	if (LIKELY((s_time_failmask & 2) == 0)) {
		struct timeval tv{};
		if (LIKELY(gettimeofday(&tv, NULL) == 0)) {
			DWORD rv = tv.tv_sec;
			rv*= 1000;
			rv+= (DWORD)(tv.tv_usec / 1000);
			return rv;
		}
		fprintf(stderr, "%s: gettimeofday error %u\n", __FUNCTION__, errno);
		s_time_failmask|= 2;
	}

	return DWORD(time(NULL) * 1000);
#endif
}

WINPORT_DECL(Sleep, VOID, (DWORD dwMilliseconds))
{
#ifdef _WIN32
	::Sleep(dwMilliseconds);
#else
	DWORD seconds = dwMilliseconds / 1000;
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
	return (now - g_process_start_stamp);
}

