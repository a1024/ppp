#include		"generic.h"
#include		<io.h>
#include		<stdio.h>
#include		<fcntl.h>
#include		<iostream>
#include		<Windows.h>
static const char file[]=__FILE__;
#ifdef _DEBUG
int				loglevel=LL_PROGRESS;
#else
int				loglevel=LL_CRITICAL;
#endif
bool			consoleactive=false;
void			RedirectIOToConsole()//https://stackoverflow.com/questions/191842/how-do-i-get-console-output-in-c-with-a-windows-program
{
	if(!consoleactive)
	{
		consoleactive=true;
		int hConHandle;
		long lStdHandle;
		CONSOLE_SCREEN_BUFFER_INFO coninfo;
		FILE *fp;

		// allocate a console for this app
		AllocConsole();

		// set the screen buffer to be big enough to let us scroll text
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
		coninfo.dwSize.Y=1000;
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

		// redirect unbuffered STDOUT to the console
		lStdHandle=(long)GetStdHandle(STD_OUTPUT_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "w");
		*stdout=*fp;
		setvbuf(stdout, nullptr, _IONBF, 0);

		// redirect unbuffered STDIN to the console
		lStdHandle=(long)GetStdHandle(STD_INPUT_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "r");
		*stdin=*fp;
		setvbuf(stdin, nullptr, _IONBF, 0);

		// redirect unbuffered STDERR to the console
		lStdHandle=(long)GetStdHandle(STD_ERROR_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "w");
		*stderr=*fp;
		setvbuf(stderr, nullptr, _IONBF, 0);

		// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
		// point to console as well
		std::ios::sync_with_stdio();
	}
}
void			freeconsole()
{
	if(consoleactive)
	{
		FreeConsole();
		consoleactive=false;
	}
}
void			print_closewarning(){printf("\n\tWARNING: CLOSING THIS WINDOW WILL CLOSE THE PROGRAM\n\n");}
void			log_start(int priority)
{
	if(loglevel>=priority)
	{
		RedirectIOToConsole();
		print_closewarning();
	}
}
void			logi(int priority, const char *format, ...)
{
	if(loglevel>=priority)
		vprintf(format, (char*)(&format+1));
}
void			log_pause(int priority)
{
	if(loglevel>=priority)
		system("pause");
}