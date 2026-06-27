#include"generic.h"
#include<stdint.h>
#include<io.h>
#include<stdio.h>
#include<fcntl.h>
#include<iostream>
#include<Windows.h>
//static const char file[]=__FILE__;
enum
{
	G_BUF_SIZE2=8192,
};
int loglevel=
#ifdef _DEBUG
	LL_PROGRESS
#else
	LL_CRITICAL
#endif
;
bool consoleactive=false;
static char g_buf2[G_BUF_SIZE2]={0};
void console_log(int priority, const char *format, ...)
{
	if(priority<loglevel)
		return;
	if(format)
	{
		va_list args;
		DWORD printed;

		va_start(args, format);
		printed=vsprintf_s(g_buf2, G_BUF_SIZE2, format, args);
		va_end(args);
		WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), g_buf2, printed, &printed, 0);
	}
}
void console_logw(int priority, const wchar_t *format, ...)
{
	if(priority<loglevel)
		return;
	if(format)
	{
		static wchar_t wbuf2[G_BUF_SIZE2]={0};
		va_list args;
		DWORD printed;

		va_start(args, format);
		printed=vswprintf_s(wbuf2, G_BUF_SIZE2, format, args);
		va_end(args);
		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), wbuf2, printed, &printed, 0);
	}
}
void console_start(int priority)//https://stackoverflow.com/questions/191842/how-do-i-get-console-output-in-c-with-a-windows-program
{
	if(priority<loglevel)
		return;
	if(!consoleactive)
	{
		consoleactive=1;
		int hConHandle;
		size_t lStdHandle;
		//CONSOLE_SCREEN_BUFFER_INFO coninfo;
		FILE *fp;

		// allocate a console for this app
		AllocConsole();

		// set the screen buffer to be big enough to let us scroll text
		//GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
		//coninfo.dwSize.Y=1000;
		//SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

		// redirect unbuffered STDOUT to the console
		lStdHandle=(size_t)GetStdHandle(STD_OUTPUT_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "w");
		*stdout=*fp;
		setvbuf(stdout, 0, _IONBF, 0);

		// redirect unbuffered STDIN to the console
		lStdHandle=(size_t)GetStdHandle(STD_INPUT_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "r");
		*stdin=*fp;
		setvbuf(stdin, 0, _IONBF, 0);

		// redirect unbuffered STDERR to the console
		lStdHandle=(size_t)GetStdHandle(STD_ERROR_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "w");
		*stderr=*fp;
		setvbuf(stderr, 0, _IONBF, 0);

#ifdef __cplusplus
		// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
		// point to console as well
		std::ios::sync_with_stdio();
#endif

		console_log(LL_CRITICAL, "\n\tWARNING: CLOSING THIS WINDOW WILL CLOSE THE PROGRAM\n\n");
	}
}
void console_end(void)
{
	if(consoleactive)
	{
		FreeConsole();
		consoleactive=0;
	}
}
void console_buffer_size(int x, int y)
{
	COORD coord={(int16_t)x, (int16_t)y};
	SMALL_RECT Rect={0, 0, (int16_t)(x-1), (int16_t)(y-1)};
	HANDLE Handle=GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleScreenBufferSize(Handle, coord);
	SetConsoleWindowInfo(Handle, TRUE, &Rect);
}

int console_scan(char *buf, int len)
{
	DWORD ret_len=0;
	int success=ReadConsoleA(GetStdHandle(STD_INPUT_HANDLE), buf, len, &ret_len, 0);
	if(!success)
	{
		int error=GetLastError();
		console_log(LL_CRITICAL, "I/O ERROR %d\n", error);
	}
	return ret_len;
}
int console_scan_int(void)
{
	int read=console_scan(g_buf2, G_BUF_SIZE);
	(void)read;
	return atoi(g_buf2);
}
double console_scan_float(void)
{
	int read=console_scan(g_buf2, G_BUF_SIZE);
	(void)read;
	return atof(g_buf2);
}
void console_pause(int priority)
{
	if(priority<loglevel)
		return;
	console_log(LL_CRITICAL, "Enter 0 to continue... ");
	int k=console_scan(g_buf2, G_BUF_SIZE);
	(void)k;
}
#if 0
void RedirectIOToConsole()//https://stackoverflow.com/questions/191842/how-do-i-get-console-output-in-c-with-a-windows-program
{
	if(!consoleactive)
	{
		consoleactive=true;
		int hConHandle;
		size_t lStdHandle;
		CONSOLE_SCREEN_BUFFER_INFO coninfo;
		FILE *fp;

		// allocate a console for this app
		AllocConsole();

		// set the screen buffer to be big enough to let us scroll text
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
		coninfo.dwSize.Y=1000;
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

		// redirect unbuffered STDOUT to the console
		lStdHandle=(size_t)GetStdHandle(STD_OUTPUT_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "w");
		*stdout=*fp;
		setvbuf(stdout, nullptr, _IONBF, 0);

		// redirect unbuffered STDIN to the console
		lStdHandle=(size_t)GetStdHandle(STD_INPUT_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "r");
		*stdin=*fp;
		setvbuf(stdin, nullptr, _IONBF, 0);

		// redirect unbuffered STDERR to the console
		lStdHandle=(size_t)GetStdHandle(STD_ERROR_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "w");
		*stderr=*fp;
		setvbuf(stderr, nullptr, _IONBF, 0);

		// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
		// point to console as well
		std::ios::sync_with_stdio();
	}
}
void freeconsole()
{
	if(consoleactive)
	{
		FreeConsole();
		consoleactive=false;
	}
}
void print_closewarning(){printf("\n\tWARNING: CLOSING THIS WINDOW WILL CLOSE THE PROGRAM\n\n");}
void log_start(int priority)
{
	if(loglevel>=priority)
	{
		RedirectIOToConsole();
		print_closewarning();
	}
}
void logi(int priority, const char *format, ...)
{
	if(loglevel>=priority)
		vprintf(format, (char*)(&format+1));
}
void log_pause(int priority)
{
	if(loglevel>=priority)
		system("pause");
}
#endif
