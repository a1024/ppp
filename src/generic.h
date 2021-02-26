#ifndef GENERIC_H
#define GENERIC_H
#include		<Windows.h>
#ifdef __cplusplus


//[PROFILER SETTINGS]

//comment or uncomment
//	#define PROFILER_CYCLES

//select one or nothing
	#define	PROFILER_SCREEN
//	#define PROFILER_TITLE
//	#define PROFILER_CMD

//comment or uncomment
//	#define PROFILER_CLIPBOARD

//select one
	#define TIMING_USE_QueryPerformanceCounter
//	#define	TIMING_USE_rdtsc
//	#define TIMING_USE_GetProcessTimes	//~16ms resolution
//	#define TIMING_USE_GetTickCount		//~16ms resolution
//	#define TIMING_USE_timeGetTime		//~16ms resolution

//END OF [PROFILER SETTINGS]


//time-measuring functions
#ifdef __GNUC__
#define	__rdtsc	__builtin_ia32_rdtsc
#endif
inline double		time_sec()
{
#ifdef TIMING_USE_QueryPerformanceCounter
	static long long t=0;
	static LARGE_INTEGER li={};
	QueryPerformanceFrequency(&li);
	t=li.QuadPart;
	QueryPerformanceCounter(&li);
	return (double)li.QuadPart/t;
#elif defined TIMING_USE_rdtsc
	static LARGE_INTEGER li={};
	QueryPerformanceFrequency(&li);
	return (double)__rdtsc()*0.001/li.QuadPart;//pre-multiplied by 1000
#elif defined TIMING_USE_GetProcessTimes
	FILETIME create, exit, kernel, user;
	int success=GetProcessTimes(GetCurrentProcess(), &create, &exit, &kernel, &user);
	if(success)
//#ifdef PROFILER_CYCLES
	{
		const auto hns2sec=100e-9;
		return hns2ms*(unsigned long long&)user;
	//	return hns2ms*(unsigned long long&)kernel;
	}
//#else
//	{
//		SYSTEMTIME t;
//		success=FileTimeToSystemTime(&user, &t);
//		if(success)
//			return t.wHour*3600000.+t.wMinute*60000.+t.wSecond*1000.+t.wMilliseconds;
//		//	return t.wHour*3600.+t.wMinute*60.+t.wSecond+t.wMilliseconds*0.001;
//	}
//#endif
	SYS_CHECK();
	return -1;
#elif defined TIMING_USE_GetTickCount
	return (double)GetTickCount()*0.001;//the number of milliseconds that have elapsed since the system was started
#elif defined TIMING_USE_timeGetTime
	return (double)timeGetTime()*0.001;//system time, in milliseconds
#endif
}
inline double		time_ms()
{
#ifdef TIMING_USE_QueryPerformanceCounter
	static long long t=0;
	static LARGE_INTEGER li={};
	QueryPerformanceFrequency(&li);
	t=li.QuadPart;
	QueryPerformanceCounter(&li);
	return (double)li.QuadPart*1000./t;
#elif defined TIMING_USE_rdtsc
	static LARGE_INTEGER li={};
	QueryPerformanceFrequency(&li);
	return (double)__rdtsc()/li.QuadPart;//pre-multiplied by 1000
#elif defined TIMING_USE_GetProcessTimes
	FILETIME create, exit, kernel, user;
	int success=GetProcessTimes(GetCurrentProcess(), &create, &exit, &kernel, &user);
	if(success)
//#ifdef PROFILER_CYCLES
	{
		const auto hns2ms=100e-9*1000.;
		return hns2ms*(unsigned long long&)user;
	//	return hns2ms*(unsigned long long&)kernel;
	}
//#else
//	{
//		SYSTEMTIME t;
//		success=FileTimeToSystemTime(&user, &t);
//		if(success)
//			return t.wHour*3600000.+t.wMinute*60000.+t.wSecond*1000.+t.wMilliseconds;
//		//	return t.wHour*3600.+t.wMinute*60.+t.wSecond+t.wMilliseconds*0.001;
//	}
//#endif
	SYS_CHECK();
	return -1;
#elif defined TIMING_USE_GetTickCount
	return (double)GetTickCount();//the number of milliseconds that have elapsed since the system was started
#elif defined TIMING_USE_timeGetTime
	return (double)timeGetTime();//system time, in milliseconds
#endif
}
inline double		elapsed_ms(double &calltime)//since last call
{
	double t0=calltime;
	calltime=time_ms();
	return calltime-t0;

	//static double t1=0;
	//double t2=time_ms(), diff=t2-t1;
	//t1=t2;
	//return diff;
}
inline double		elapsed_cycles(long long &calltime)//since last call
{
	long long t0=calltime;
	calltime=__rdtsc();
	return double(calltime-t0);

	//static long long t1=0;
	//long long t2=__rdtsc();
	//double diff=double(t2-t1);
	//t1=t2;
	//return diff;
}

extern int			prof_on;
void				prof_toggle();

//void				prof_start();
void				prof_add(const char *label, int divisor=1);
void				prof_sum(const char *label, int count);//add the sum of last 'count' steps
void				prof_loop_start(const char **labels, int n);//describe the loop body parts in 'labels'
void				prof_add_loop(int idx);//call on each part of loop body
#ifdef PROFILER_SCREEN
#define		PROF_PRINT_ARGS	HDC hDC, int xlabels, int xnumbers
#else
#define		PROF_PRINT_ARGS
#endif
void				prof_print(PROF_PRINT_ARGS);
void				prof_print_in_title();
//END OF [PROFILER]


#define		SIZEOF(STATIC_ARRAY)	(sizeof(STATIC_ARRAY)/sizeof(*(STATIC_ARRAY)))


//[CONSOLE]		cpp (name mangled) to enable cin & cout
enum			LogLevel
{
	LL_CRITICAL,	//talk only when unavoidable		//inspired by FFmpeg
	LL_OPERATIONS,	//talk only when doing dangerous stuff
	LL_PROGRESS,	//report progress
};
void			RedirectIOToConsole();
void			freeconsole();
void			print_closewarning();
void			log_start(int priority);
void			logi(int priority, const char *format, ...);
void			log_pause(int priority);
#define			log_end freeconsole


//string
#include		<string>
inline wchar_t	acme_tolower(wchar_t c)//ASCII-only
{
	return c+(('a'-'A')&-((c>='A')&(c<='Z')));
}
void			assign_path(std::wstring const &text, int start, int end, std::wstring &pathret);
void			assign_path(std::string const &text, int start, int end, std::wstring &pathret);
void			get_name_from_path(std::wstring const &path, std::wstring &name);

bool			skip_whitespace(std::wstring const &text, int &k);
bool			skip_whitespace(std::string const &text, int &k);
bool			compare_string_caseinsensitive(std::wstring const &text, int &k, const wchar_t *label, int label_size);
bool			compare_string_caseinsensitive(std::string const &text, int &k, const char *label, int label_size);
unsigned		read_unsigned_int(std::string const &text, int &k);
double			read_unsigned_float(std::string const &text, int &k);
int				acme_matchlabel_ci(const wchar_t *text, int size, int &k, const wchar_t *label, int &advance);
int				skip_till_newline_or_null(const wchar_t *text, int size, int &k, int &advance);

#include		"g_cpp11.h"

extern "C"
{
#else//!__cplusplus
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif//_WCHAR_T_DEFINED
#endif//__cplusplus

	//buffer
#define G_BUF_SIZE	2048
	extern const int g_buf_size;
	extern char		g_buf[G_BUF_SIZE];
	extern wchar_t	g_wbuf[G_BUF_SIZE];

	//math
	extern const double	_pi, todeg, torad;
	int				mod(int x, int n);
	int				floor_log2(unsigned long long n);
	int				floor_log10(double x);
	double			power(double x, int y);
	double			_10pow(int n);

	//memory
	void			memfill(void *dst, const void *src, int dstbytes, int srcbytes);//repeating pattern
	void			cycle_image(int *image, int iw, int ih, int dx, int dy);//lossless

	//string
	long long		acme_wtoll(const wchar_t *str, int *ret_advance);
	int				acme_wtoi(const wchar_t *str, int *ret_advance);
	int				acme_strcmp_ci(const wchar_t *s1, const wchar_t *s2);//returns zero on match, n+1 on mismatch at n

	//win32
	int				GUINPrint(HDC hDC, int x, int y, int w, int h, const char *a, ...);//DrawText with newlines
	long			GUITPrint(HDC hDC, int x, int y, const char *a, ...);//return value: 0xHHHHWWWW		width=(short&)ret, height=((short*)&ret)[1]
	void			GUIPrint(HDC hDC, int x, int y, const char *a, ...);
//	void			GUIPrint(HDC hDC, int x, int y, int value);
	const char*		wm2str(int message);
	void			messagebox(HWND hWnd, const wchar_t *title, const wchar_t *format, ...);//rename to messageboxw
	void			messageboxa(HWND hWnd, const char *title, const char *format, ...);
	void			formatted_error(const wchar_t *lpszFunction, int line);
	void			error_exit(const wchar_t *lpszFunction, int line);//Format a readable error message, display a message box, and exit from the application.

	double			getwindowdouble(HWND hWnd);
	int				getwindowint(HWND hWnd);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//GENEGIC_H