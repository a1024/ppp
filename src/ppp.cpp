#include		<Windows.h>

#include		<io.h>//for console
#include		<fcntl.h>
#include		<conio.h>

#include		<Gdiplus.h>
#include		<Gdiplusimaging.h>
//#include		<wcsplugin.h>//windows color system
#include		<commctrl.h>
#include		<sys\stat.h>
#include		<string>
#include		<cwctype>
#include		<sstream>
#include		<fstream>
#include		<vector>
#include		<queue>
#pragma			comment(lib, "Comctl32.lib")//status bar
#pragma			comment(lib, "Gdiplus.lib")
#define			STBI_WINDOWS_UTF8
#include		"stb_image.h"
#include		"lodepng.h"
//#define		SDL_MAIN_HANDLED
//#include		<SDL.h>
//#include		<SDL_syswm.h>
//#pragma		comment(lib, "SDL2.lib")
//#pragma		comment(lib, "SDL2main.lib")

#include		<iostream>
#include		<codecvt>
#include		<assert.h>
#include		<exception>
#include		<functional>
#include		<time.h>
#include		<strsafe.h>
//#pragma		comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

	#define		RELEASE
#ifndef RELEASE
	#define		PROFILER
#endif
	#define		CAPTURE_MOUSE
//	#define		USE_SSE2

//	#define		MOUSE_POLLING_TEST

const double	_pi=acos(-1.), todeg=180/_pi, torad=_pi/180;
int				*rgb=nullptr, w=0, h=0, rgbn=0,
				mx=0, my=0,//current mouse position
				prev_mx=0, prev_my=0,//previous mouse position
				start_mx=0, start_my=0,//original mouse position
				*image=nullptr,			iw=0, ih=0, image_size=0, nchannels=0,//image==history[histpos].buffer always
				*temp_buffer=nullptr,	tw=0, th=0,	//temporary buffer, has alpha
				*sel_buffer=nullptr,	sw=0, sh=0,	//selection buffer, has alpha
				*sel_mask=nullptr,		//selection mask, used only in free-form selection, inside selection polygon if true, same dimensions as sel_buffer
				logzoom=0,//log2(pixel size)
				spx=0, spy=0;//position of top left corner of the window in image coordinates
double			zoom=1;//a power of 2
bool			use_temp_buffer=false,
				modified=false;
//std::vector<int> delays;//X
std::wstring	workspace,//the workspace containing the workfolder
				workfolder;//the folder containing opened temp files, it has the name 'date time'
enum			ImageFormat{IF_BMP, IF_TIFF, IF_PNG, IF_JPEG, IF_FLIF};
int				tempformat=IF_PNG;
const wchar_t	*settingsfilename=L"settings.txt", *imageformats[]={L"BMP", L"TIFF", L"PNG", L"JPEG", L"FLIF"};
std::vector<std::wstring> framenames;//file names of all frames, must append workfolder with possible doublequote
std::vector<int*> thumbnails;//same size as framenames
int				th_w=0, th_h=0,//thumbnail dimensions
				thbox_h=0,//thumbnail box internal height
				thbox_x1=0,//thumbnail box xstart
				thbox_posy=0;//thumbnail box scroll position (thbox top -> screen top)
bool			thbox_scrollbar=false;
int				thbox_vscroll_my_start=0,//starting mouse position
				thbox_vscroll_s0=0,//initial scrollbar slider start
				thbox_vscroll_start=0,//scrollbar slider start
				thbox_vscroll_size=0;//scrollbar slider size
int				current_frame=0, nframes=0,
				framerate_num=0, framerate_den=1;
bool			animated=false;
tagRECT			R;
HINSTANCE		ghInstance=0;
HWND			ghWnd=nullptr;
//HWND			hFontbar=nullptr;
HWND			hFontcombobox=nullptr, hFontsizecb=nullptr, hTextbox=nullptr,
				hRotatebox=nullptr,
				hStretchHbox=nullptr, hStretchVbox=nullptr, hSkewHbox=nullptr, hSkewVbox=nullptr;
//HWND			gToolbar=0, gColorBar=0, ghStatusbar=0, hWndPanel=0;
HDC				ghDC=0, ghMemDC=0;
HBITMAP			hBitmap;
std::wstring	programpath, FFmpeg_path;
bool			FFmpeg_path_has_quotes=false, workspace_has_quotes=false;
std::string		FFmpeg_version;
std::wstring	filename=L"Untitled", filepath;//opened media file name & path, utf16
const wchar_t	doublequote=L'\"';
const int		g_buf_size=2048;
char			g_buf[g_buf_size]={0};
wchar_t			g_wbuf[g_buf_size]={0};
long			GUITPrint(HDC__ *ghDC, int x, int y, const char *a, ...)//return value: 0xHHHHWWWW		width=(short&)ret, height=((short*)&ret)[1]
{
	int length=vsprintf_s(g_buf, g_buf_size, a, (char*)(&a+1));
	if(length>0)
		return TabbedTextOutA(ghDC, x, y, g_buf, length, 0, nullptr, 0);
	return 0;
}
void			GUIPrint(HDC__ *ghDC, int x, int y, const char *a, ...)
{
	int length=vsprintf_s(g_buf, g_buf_size, a, (char*)(&a+1)), success;
	if(length>0)
		success=TextOutA(ghDC, x, y, g_buf, length);
}
void			GUIPrint(HDC__ *ghDC, int x, int y, int value)
{
	int length=sprintf_s(g_buf, 1024, "%d", value), success;
	if(length>0)
		success=TextOutA(ghDC, x, y, g_buf, length);
}
inline int		minimum(int a, int b){return (a+b-abs(a-b))>>1;}
inline int		maximum(int a, int b){return (a+b+abs(a-b))>>1;}
inline double	minimum(double a, double b){return (a+b-abs(a-b))*0.5;}
inline double	maximum(double a, double b){return (a+b+abs(a-b))*0.5;}
inline double	maximum(double a, double b, double c, double d)
{
	double x1=a+b+abs(a-b), x2=c+d+abs(c-d);
	return (x1+x2+abs(x1-x2))*0.25;
}
inline double	minimum(double a, double b, double c, double d)
{
	double x1=a+b-abs(a-b), x2=c+d-abs(c-d);
	return (x1+x2-abs(x1-x2))*0.25;
}
int				clamp(int lo, int x, int hi)
{
	int lo2=lo<<1, min=x+hi-abs(x-hi);
	return (lo2+min+abs(lo2-min))>>2;
//	return maximum(lo, minimum(x, hi));
}
inline int		shift(int x, int sh){return sh>=0?x<<sh:x>>-sh;}//signed shift
enum			DragType{D_NONE, D_DRAW, D_HSCROLL, D_VSCROLL, D_SELECTION, D_THBOX_VSCROLL, D_ALPHA1, D_ALPHA2};
char			kb[256]={false},
				mb[2]={false},//mouse buttons {0: left, 1: right}
				drag=0, prev_drag=0,//mouse button was pressed inside image
				timer=false;
bool			hscrollbar=false, vscrollbar=false;
int				hscroll_mx_start=0, vscroll_my_start=0,//starting mouse position
				hscroll_s0=0, vscroll_s0=0,//initial scrollbar slider start
				hscroll_start=0, vscroll_start=0,//scrollbar slider start
				hscroll_size=0, vscroll_size=0;//scrollbar slider size

struct			Font
{
	int type;
	LOGFONTW lf;
	TEXTMETRICW tm;
	std::vector<int> sizes;//if empty, use truetypesizes
	Font():type(-1){}
	Font(int type, LOGFONTW const *lf, TEXTMETRICW const *tm):type(type)
	{
		memcpy(&this->lf, lf, sizeof LOGFONTW);
		memcpy(&this->tm, tm, sizeof TEXTMETRICW);

		static const wchar_t *bitmapfontnames[]=
		{
			L"Courier",
			L"Fixedsys",
			L"MS Sans Serif",
			L"MS Serif",
			L"Small Fonts",
			L"System",
		};
		enum BitmapFont{BF_COURIER, BF_FIXEDSYS, BF_MS_SANS_SERIF, BF_MS_SERIF, BF_SMALL_FONTS, BF_SYSTEM,		NBITMAPFONTS};
		static const int
			courier		[]={10, 12, 15},					courier_size	=sizeof courier>>2,
			fixedsys	[]={9},								fixedsys_size	=sizeof fixedsys>>2,
			mssansserif	[]={8, 10, 12, 14, 18, 24},			mssansserif_size=sizeof mssansserif>>2,
			msserif		[]={6, 7, 8, 10, 12, 14, 18, 24},	msserif_size	=sizeof msserif>>2,
			smallfonts	[]={2, 3, 4, 5, 6, 7, 8},			smallfonts_size	=sizeof smallfonts>>2,
			system		[]={10},							system_size		=sizeof system>>2;
		for(int k=0;k<NBITMAPFONTS;++k)
		{
			if(!wcscmp(lf->lfFaceName, bitmapfontnames[k]))//match
			{
				const int *sizes=nullptr;
				int size=0;
				switch(k)
				{
				case BF_COURIER:		sizes=courier,		size=courier_size;		break;
				case BF_FIXEDSYS:		sizes=fixedsys,		size=fixedsys_size;		break;
				case BF_MS_SANS_SERIF:	sizes=mssansserif,	size=mssansserif_size;	break;
				case BF_MS_SERIF:		sizes=msserif,		size=msserif_size;		break;
				case BF_SMALL_FONTS:	sizes=smallfonts,	size=smallfonts_size;	break;
				case BF_SYSTEM:			sizes=system,		size=system_size;		break;
				}
				this->sizes.assign(sizes, sizes+size);
				break;
			}
		}
	}
};
bool			operator<(Font const &a, Font const &b){return wcscmp(a.lf.lfFaceName, b.lf.lfFaceName)<0;}
const int		truetypesizes[]={8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72}, truetypesizes_size=sizeof(truetypesizes)>>2;
//const wchar_t	*ttypesizes_str[]={L"8", L"9", L"10", L"11", L"12", L"14", L"16", L"18", L"20", L"22", L"24", L"26", L"28", L"36", L"48", L"72"};
std::vector<Font> fonts;
int				font_idx=0, font_size=10;
bool			bold=false, italic=false, underline=false;
HFONT			hFont=nullptr;

unsigned long	broken=0;
int				broken_line=-1;
void			messagebox(HWND hWnd, const wchar_t *title, const wchar_t *format, ...)
{
	vswprintf_s(g_wbuf, g_buf_size, format, (char*)(&format+1));
	MessageBoxW(hWnd, g_wbuf, title, MB_OK);
}
void			formatted_error(const wchar_t *lpszFunction, int line)
{
	DWORD dw=GetLastError();
	wchar_t *lpMsgBuf=nullptr;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (wchar_t*)&lpMsgBuf, 0, nullptr);
	wchar_t *lpDisplayBuf=(wchar_t*)LocalAlloc(LMEM_ZEROINIT, (lstrlenW(lpMsgBuf)+lstrlenW(lpszFunction)+40)*sizeof(wchar_t));
	StringCchPrintfW(lpDisplayBuf, LocalSize(lpDisplayBuf)/sizeof(TCHAR), TEXT("%s (line %d) failed with error %d: %s"), lpszFunction, line, dw, lpMsgBuf);
//	MessageBoxW(nullptr, lpDisplayBuf, TEXT("Error"), MB_OK);
	MessageBoxW(nullptr, lpDisplayBuf, lpDisplayBuf, MB_OK);//
	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}
void			error_exit(const wchar_t *lpszFunction, int line)//Format a readable error message, display a message box, and exit from the application.
{
	formatted_error(lpszFunction, line);
	ExitProcess(1);
}
void			check(int line)
{
	unsigned long error=GetLastError();
	if(error&&!broken)
	{
		broken=error, broken_line=line;
		formatted_error(L"check", line);
	}
}
void			check_exit(const wchar_t *lpszFunction, int line)
{
	int err=GetLastError();
	err=err;
	//if(err)
	//	error_exit(lpszFunction, line);
}
void			unimpl(){messagebox(ghWnd, L"Error", L"Not implemented yet.");}
void			modify()
{
	if(!modified)
	{
		modified=true;
		int size=GetWindowTextW(ghWnd, g_wbuf+1, g_buf_size-1);
		if(g_wbuf[1]!=L'*')
		{
			g_wbuf[0]=L'*', g_wbuf[1+size]=L'\0';
			SetWindowTextW(ghWnd, g_wbuf);
		}
	}
}
void			read_binary(const wchar_t *filename, std::vector<byte> &binary_data)
{
	std::ifstream input(filename, std::ios::binary);
	binary_data.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
	input.close();
}
void			write_file(const wchar_t *filename, std::string const &output)
{
	std::ofstream file(filename, std::ios::out);
	file<<output;
	file.close();
}
void			write_binary(const wchar_t *filename, std::vector<byte> const &binary_data)
{
	std::string str(binary_data.begin(), binary_data.end());
	std::ofstream file(filename, std::ios::binary|std::ios::out);
	file<<str;
	file.close();
}
void			read_utf8_file(const wchar_t *filename, std::wstring &output)
{
	std::wifstream wif(filename);
	wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
	std::wstringstream wss;
	wss<<wif.rdbuf();
	output=std::move(wss.str());
}

enum			LogLevel
{
	LL_CRITICAL,	//talk only when unavoidable		//inspired by FFmpeg
	LL_OPERATIONS,	//talk only when doing dangerous stuff
	LL_PROGRESS,	//report progress
};
int				loglevel=LL_PROGRESS;
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
void			print_closewarning(){printf("\n\tWARNING: CLOSING THIS WINDOW WILL CLOSE PAINT++\n\n");}
void			log_start(int priority)
{
	if(loglevel>=priority)
	{
		RedirectIOToConsole();
		print_closewarning();
	}
}
void			log(int priority, const char *format, ...)
{
	if(loglevel>=priority)
		vprintf(format, (char*)(&format+1));
}
void			log_pause(int priority)
{
	if(loglevel>=priority)
		system("pause");
}
#define			log_end freeconsole

bool			check_FFmpeg_path(std::wstring &path)
{
	if(!path.size())
		return false;
	auto &end=*path.rbegin();
	if(end!=L'\\'&&end!=L'/')
		path.push_back(L'/');
	std::wstring FFmpeg_command=path+L"FFmpeg";

	SECURITY_ATTRIBUTES saAttr={sizeof(SECURITY_ATTRIBUTES), nullptr, 1};
	void *g_hChildStd_IN_Rd=nullptr, *g_hChildStd_IN_Wr=nullptr, *g_hChildStd_OUT_Rd=nullptr, *g_hChildStd_OUT_Wr=nullptr;
	if(!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))//Create a pipe for the child process's STDOUT.
		error_exit(TEXT("StdoutRd CreatePipe"), __LINE__);
	if(!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))//Ensure the read handle to the pipe for STDOUT is not inherited.
		error_exit(TEXT("Stdout SetHandleInformation"), __LINE__);
	if(!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))//Create a pipe for the child process's STDIN.
		error_exit(TEXT("Stdin CreatePipe"), __LINE__);
	if(!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))//Ensure the write handle to the pipe for STDIN is not inherited.
		error_exit(TEXT("Stdin SetHandleInformation"), __LINE__);
	
	//create child process
	PROCESS_INFORMATION piProcInfo={0};//Set up members of the PROCESS_INFORMATION structure.
	STARTUPINFOW siStartInfo=
	{//Set up members of the STARTUPINFO structure. This structure specifies the STDIN and STDOUT handles for redirection.
		sizeof(STARTUPINFO), nullptr, nullptr, nullptr,//resetved, desktop, title
		0, 0, 0, 0, 0, 0, 0,//x, y, xsize, ysize, xcountchars, ycountchars, fill attribute
		STARTF_USESTDHANDLES,//flags
		0, 0, nullptr,//show window, reserved2, reserved2
		g_hChildStd_IN_Rd, g_hChildStd_OUT_Wr, g_hChildStd_OUT_Wr,//stdin, stdout, stderr
	};
	int bSuccess=CreateProcessW(nullptr,//Create the child process.
		&FFmpeg_command[0],	//command line
		nullptr,			//process security attributes
		nullptr,			//primary thread security attributes
		TRUE,				//handles are inherited
		CREATE_NO_WINDOW,	//creation flags
		nullptr,			//use parent's environment
		nullptr,			//use parent's current directory
		&siStartInfo,		//STARTUPINFO pointer
		&piProcInfo);		//receives PROCESS_INFORMATION
	if(!bSuccess)//If an error occurs, exit the application.
		error_exit(TEXT("CreateProcess"), __LINE__);
	else
	{
		//Close handles to the child process and its primary thread.
		//Some applications might keep these handles to monitor the status of the child process, for example.
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
		
		//Close handles to the stdin and stdout pipes no longer needed by the child process.
		//If they are not explicitly closed, there is no way to recognize that the child process has ended.
		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(g_hChildStd_IN_Rd);
	}
	unsigned long read=0;
	std::string result;
	do
	{
		memset(g_buf, 0, g_buf_size);
		bSuccess=ReadFile(g_hChildStd_OUT_Rd, g_buf, g_buf_size, &read, nullptr);
		result+=g_buf;
	}while(bSuccess&&read);
	if(!result.size())
		return false;
	const char ex_start[]="ffmpeg version";
	int k=0, size=result.size();
	for(int kEnd=minimum(size, strlen(ex_start));k<kEnd;++k)
		if(result[k]!=ex_start[k])
			return false;
	for(;k<size&&result[k]==' ';++k);
	int versionstart=k;
	for(;k<size&&result[k]!=' ';++k);
	FFmpeg_version.assign(result.begin()+versionstart, result.begin()+k);
	return true;
}
bool			check_FFmpeg_path(std::string &path)
{
	if(!path.size())
		return false;
	{//remove doublequotes
		const char dquote='\"';
		bool opening_dquote=path[0]==dquote, closing_dquote=*path.rbegin()==dquote;
		if(closing_dquote)
			path.pop_back();
		if(opening_dquote&&path.size())
			path.erase(path.begin());
	}
	auto &end=*path.rbegin();
	if(end!='\\'&&end!='/')
		path.push_back('/');
	std::string FFmpeg_command=path+"FFmpeg";

	SECURITY_ATTRIBUTES saAttr={sizeof(SECURITY_ATTRIBUTES), nullptr, 1};
	void *g_hChildStd_IN_Rd=nullptr, *g_hChildStd_IN_Wr=nullptr, *g_hChildStd_OUT_Rd=nullptr, *g_hChildStd_OUT_Wr=nullptr;
	if(!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))//Create a pipe for the child process's STDOUT.
		error_exit(TEXT("StdoutRd CreatePipe"), __LINE__);
	if(!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))//Ensure the read handle to the pipe for STDOUT is not inherited.
		error_exit(TEXT("Stdout SetHandleInformation"), __LINE__);
	if(!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))//Create a pipe for the child process's STDIN.
		error_exit(TEXT("Stdin CreatePipe"), __LINE__);
	if(!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))//Ensure the write handle to the pipe for STDIN is not inherited.
		error_exit(TEXT("Stdin SetHandleInformation"), __LINE__);
	
	//create child process
	PROCESS_INFORMATION piProcInfo={0};//Set up members of the PROCESS_INFORMATION structure.
	STARTUPINFOA siStartInfo=
	{//Set up members of the STARTUPINFO structure. This structure specifies the STDIN and STDOUT handles for redirection.
		sizeof(STARTUPINFO), nullptr, nullptr, nullptr,//resetved, desktop, title
		0, 0, 0, 0, 0, 0, 0,//x, y, xsize, ysize, xcountchars, ycountchars, fill attribute
		STARTF_USESTDHANDLES,//flags
		0, 0, nullptr,//show window, reserved2, reserved2
		g_hChildStd_IN_Rd, g_hChildStd_OUT_Wr, g_hChildStd_OUT_Wr,//stdin, stdout, stderr
	};
	int bSuccess=CreateProcessA(nullptr,//Create the child process.
		&FFmpeg_command[0],	//command line
		nullptr,			//process security attributes
		nullptr,			//primary thread security attributes
		TRUE,				//handles are inherited
		CREATE_NO_WINDOW,	//creation flags
		nullptr,			//use parent's environment
		nullptr,			//use parent's current directory
		&siStartInfo,		//STARTUPINFO pointer
		&piProcInfo);		//receives PROCESS_INFORMATION
	if(!bSuccess)//If an error occurs, exit the application.
		return false;
	//	error_exit(TEXT("CreateProcess"), __LINE__);
	else
	{
		//Close handles to the child process and its primary thread.
		//Some applications might keep these handles to monitor the status of the child process, for example.
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
		
		//Close handles to the stdin and stdout pipes no longer needed by the child process.
		//If they are not explicitly closed, there is no way to recognize that the child process has ended.
		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(g_hChildStd_IN_Rd);
	}
	unsigned long read=0;
	std::string result;
	do
	{
		memset(g_buf, 0, g_buf_size);
		bSuccess=ReadFile(g_hChildStd_OUT_Rd, g_buf, g_buf_size, &read, nullptr);
		result+=g_buf;
	}while(bSuccess&&read);
	if(!result.size())
		return false;
	const char ex_start[]="ffmpeg version";
	int k=0, size=result.size();
	for(int kEnd=minimum(size, strlen(ex_start));k<kEnd;++k)
		if(result[k]!=ex_start[k])
			return false;
	for(;k<size&&result[k]==' ';++k);
	int versionstart=k;
	for(;k<size&&result[k]!=' ';++k);
	FFmpeg_version.assign(result.begin()+versionstart, result.begin()+k);
	return true;
}
void			exec_FFmpeg_command(const wchar_t *executable, std::wstring const &args, std::string &result)//executable is FFmpeg or FFprobe
{
	auto &end=*FFmpeg_path.rbegin();
	if(end!=L'\\'&&end!=L'/')
		FFmpeg_path.push_back(L'/');
	std::wstring command=FFmpeg_path+executable+(FFmpeg_path_has_quotes?L"\" ":L" ")+L"-hide_banner "+args;
//	std::wstring command=FFmpeg_path+executable+(FFmpeg_path_has_quotes?L"\" ":L" ")+L"-hide_banner -loglevel warning "+args;
	
	SECURITY_ATTRIBUTES saAttr={sizeof(SECURITY_ATTRIBUTES), nullptr, 1};
	void *g_hChildStd_IN_Rd=nullptr, *g_hChildStd_IN_Wr=nullptr, *g_hChildStd_OUT_Rd=nullptr, *g_hChildStd_OUT_Wr=nullptr;
	if(!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))//Create a pipe for the child process's STDOUT.
		error_exit(TEXT("StdoutRd CreatePipe"), __LINE__);
	if(!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))//Ensure the read handle to the pipe for STDOUT is not inherited.
		error_exit(TEXT("Stdout SetHandleInformation"), __LINE__);
	if(!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))//Create a pipe for the child process's STDIN.
		error_exit(TEXT("Stdin CreatePipe"), __LINE__);
	if(!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))//Ensure the write handle to the pipe for STDIN is not inherited.
		error_exit(TEXT("Stdin SetHandleInformation"), __LINE__);
	
	//create child process
	PROCESS_INFORMATION piProcInfo={0};//Set up members of the PROCESS_INFORMATION structure.
	STARTUPINFOW siStartInfo=
	{//Set up members of the STARTUPINFO structure. This structure specifies the STDIN and STDOUT handles for redirection.
		sizeof(STARTUPINFO), nullptr, nullptr, nullptr,//resetved, desktop, title
		0, 0, 0, 0, 0, 0, 0,//x, y, xsize, ysize, xcountchars, ycountchars, fill attribute
		STARTF_USESTDHANDLES,//flags
		0, 0, nullptr,//show window, reserved2, reserved2
		g_hChildStd_IN_Rd, g_hChildStd_OUT_Wr, g_hChildStd_OUT_Wr,//stdin, stdout, stderr
	};
	int bSuccess=CreateProcessW(nullptr,//Create the child process.
		&command[0],	//command line
		nullptr,			//process security attributes
		nullptr,			//primary thread security attributes
		TRUE,				//handles are inherited
		CREATE_NO_WINDOW,	//creation flags
		nullptr,			//use parent's environment
		nullptr,			//use parent's current directory
		&siStartInfo,		//STARTUPINFO pointer
		&piProcInfo);		//receives PROCESS_INFORMATION
	if(!bSuccess)//If an error occurs, exit the application.
		error_exit(TEXT("CreateProcess"), __LINE__);
	else
	{
		//Close handles to the child process and its primary thread.
		//Some applications might keep these handles to monitor the status of the child process, for example.
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
		
		//Close handles to the stdin and stdout pipes no longer needed by the child process.
		//If they are not explicitly closed, there is no way to recognize that the child process has ended.
		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(g_hChildStd_IN_Rd);
	}
	result.clear();
	unsigned long read=0;
	do
	{
		memset(g_buf, 0, g_buf_size);
		bSuccess=ReadFile(g_hChildStd_OUT_Rd, g_buf, g_buf_size, &read, nullptr);
		result+=g_buf;
	}while(bSuccess&&read);
}

void			skip_whitespace(std::wstring const &text, int &k)
{
	const int size=text.size();
	for(;k<size&&(text[k]==L' '||text[k]==L'\t'||text[k]==L'\r'||text[k]==L'\n');++k);//skip whitespace
}
void			skip_whitespace(std::string const &text, int &k)
{
	const int size=text.size();
	for(;k<size&&(text[k]==' '||text[k]=='\t'||text[k]=='\r'||text[k]=='\n');++k);//skip whitespace
}
bool			compare_string_caseinsensitive(std::wstring const &text, int &k, const wchar_t *label, int label_size)
{
	const int size=text.size();
	if(size-k<label_size)
		return false;
	int k2=k;
	for(int k3=0;k2<size&&k3<label_size;++k2, ++k3)
		if(std::towupper(text[k2])!=std::towupper(label[k3]))
			return false;
	k=k2;
	return true;
}
bool			compare_string_caseinsensitive(std::string const &text, int &k, const char *label, int label_size)
{
	const int size=text.size();
	if(size-k<label_size)
		return false;
	int k2=k;
	for(int k3=0;k2<size&&k3<label_size;++k2, ++k3)
		if(std::towupper(text[k2])!=std::towupper(label[k3]))
			return false;
	k=k2;
	return true;
}
int				seek_path(std::wstring const &text, int &k)//k should point at path start / opening quote, returns true if path is enclosed in double quotes
{
	const int size=text.size();
	char quotes=text[k]==doublequote;
	if(quotes)
	{
		++k;
		for(;k<size&&text[k]!=doublequote;++k);
		return true;//k at closing quote
	}
	for(;k<size&&text[k]!=L' '&&text[k]!=L'\t'&&text[k]!=L'\r'&&text[k]!=L'\n';++k);//skip till whitespace
	return false;
}
bool			seek_token(std::string const &text, int &k, const char *token)//skip till token is found
{
	const int size=text.size(), token_size=strlen(token);
	for(int k2=k;k2<size;++k2)
	{
		bool match=true;
		for(int k3=0;k2+k3<size&&k3<token_size;++k3)
		{
			if(text[k2+k3]!=token[k3])
			{
				match=false;
				break;
			}
		}
		if(match)
		{
			k=k2;
			return true;
		}
	}
	return false;
}
bool			parse_path_field(std::wstring const &text, int &k, const wchar_t *token, std::wstring &result)//returns true if has spaces & quotes
{
	const wchar_t path_token[]=L"path:";
	int path_size=lstrlenW(path_token), token_size=lstrlenW(token);
	skip_whitespace(text, k);
	char match=compare_string_caseinsensitive(text, k, token, token_size);
	skip_whitespace(text, k);
	match|=(char)compare_string_caseinsensitive(text, k, path_token, path_size);
	skip_whitespace(text, k);
	if(match)
	{
		int k0=k;
		int quotes=seek_path(text, k)!=0;
		result.assign(text.begin()+k0, text.begin()+k);
		k+=quotes;
		//for(int k2=0, size=result.size();k2<size;++k)
		//{
		//	if(result[k2]==' ')
		//	{
		//		if(result[0]!=doublequote)
		//			result.insert(result.begin(), doublequote);//unreachable
		//		quotes=true;
		//		break;
		//	}
		//}
		return quotes!=0;
	}
	return false;
}
unsigned		read_unsigned_int(std::string const &text, int &k)//k at first decimal digit
{
	const int size=text.size();
	int k0=k, p=1;
	for(;k<size&&text[k]>='0'&&text[k]<='9';++k);//skip till end of number
	unsigned value=0;
	for(int k2=k-1;k2>=k0;--k2, p*=10)
		value+=(text[k2]-'0')*p;
	return value;
}
double			read_unsigned_float(std::string const &text, int &k)//k at first decimal digit
{
	const int size=text.size();
	int k0=k, kpoint=-1;
	double p=1;
	for(;k<size&&(text[k]>='0'&&text[k]<='9'||text[k]=='.');++k)//skip till end of number
	{
		if(text[k]=='.'&&kpoint==-1)
			kpoint=k;
		else if(kpoint!=-1)
			p*=0.1;
	}
	double value=0;
	for(int k2=k-1;k2>=k0;--k2)
		if(text[k2]!='.')
			value+=(text[k2]-'0')*p, p*=10;
	return value;
}
void			read_settings()
{
	std::wstring text;
	read_utf8_file((programpath+settingsfilename).c_str(), text);
	int k=0;
	FFmpeg_path_has_quotes=parse_path_field(text, k, L"FFmpeg", FFmpeg_path);
	workspace_has_quotes=parse_path_field(text, k, L"Workspace", workspace);
	if(!workspace.size())
	{
		workspace=programpath;
		for(int k2=0, size=workspace.size();k2<size;++k2)
		{
			if(workspace[k2]==' ')
			{
				if(workspace[0]!=doublequote)
					workspace.insert(workspace.begin(), doublequote);
				auto end=*workspace.rbegin();
				if(end==doublequote)
					workspace.pop_back();
				workspace_has_quotes=true;
				break;
			}
		}
	}
}
void			ask_FFmpeg_path()
{
	log_start(LL_CRITICAL);
	log(LL_CRITICAL,
		"FFmpeg not found.\n"
		"Please download FFmpeg from:\n"
		"\n"
		"\tffmpeg.zeranoe.com/builds\n"
		"\n"
		"In the website, choose:\n"
		"\n"
		"\tVersion: <latest>\n"
		"\tArchitecture: 64bit\n"
		"\tLinking: static\n"
		"\n"
		"and specify the path to FFmpeg and FFprobe here, or in:\n"
		"\n"
		"\t%ls\n"
		"\n"
		"> ", (programpath+settingsfilename).c_str());
	for(;;)
	{
		std::string str;
	//	std::cin>>str;
		std::getline(std::cin, str);
	//	std::cin.getline(
		if(check_FFmpeg_path(str))//if valid path entered, write it in 'settings.txt'
		{
			FFmpeg_path_has_quotes=false;
			for(int k=0, kEnd=str.size();k<kEnd;++k)
			{
				if(str[k]==' ')
				{
					FFmpeg_path_has_quotes=true;
					break;
				}
			}
			if(FFmpeg_path_has_quotes)
			{
				if(str[0]!='\"')
					str.insert(str.begin(), '\"');
			}
			FFmpeg_path.assign(str.begin(), str.end());
			std::string str2="FFmpeg path: "+str;
			if(FFmpeg_path_has_quotes)
				str2+='\"';
			str2+="\nWorkspace path:";
			write_file((programpath+settingsfilename).c_str(), str2);
			break;
		}
		else//check 'settings.txt'
		{
			read_settings();
		//	read_utf8_file((programpath+settingsfilename).c_str(), FFmpeg_path);
			if(check_FFmpeg_path(FFmpeg_path))
				break;
		}
		log(LL_CRITICAL,
			"\nFFmpeg not found.\n"
			"> ");
	}
	log(LL_CRITICAL,
		"\n"
		"Found FFmpeg version %s\n"
		"Press any key to continue...\n", FFmpeg_version.c_str());
	_getch();
	freeconsole();
}

void			copy_to_clipboard(const char *a, int size)//size not including null terminator
{
	char *clipboard=(char*)LocalAlloc(LMEM_FIXED, (size+1)*sizeof(char));
	memcpy(clipboard, a, (size+1)*sizeof(char));
	clipboard[size]='\0';
	OpenClipboard(ghWnd);
	EmptyClipboard();
	SetClipboardData(CF_OEMTEXT, (void*)clipboard);
	CloseClipboard();
}
void			copy_to_clipboard(std::string const &str){copy_to_clipboard(str.c_str(), str.size());}
void			sound_to_clipboard(short *audio_buf, float *L, float *R, int nsamples)
{
	std::stringstream LOL_1;
	LOL_1<<"idx\tL\tR\r\n\r\n";
	for(int k=0;k<nsamples;++k)
		LOL_1<<k<<'\t'<<audio_buf[k<<1]<<'\t'<<L[k]<<'\t'<<audio_buf[(k<<1)+1]<<'\t'<<R[k]<<"\r\n";
	//	LOL_1<<k<<'\t'<<audio_buf[k<<1]<<' '<<L[k]<<'\t'<<audio_buf[(k<<1)+1]<<' '<<R[k]<<"\r\n";
	copy_to_clipboard(LOL_1.str());
}
void			buffer_to_clipboard(const void *buffer, int size_bytes)
{
	std::stringstream LOL_1;
	auto a=(const char*)buffer;
	for(int k=0;k<size_bytes;++k)
		LOL_1<<(a[k]&0xFF)<<"\r\n";
	//	LOL_1<<std::hex<<(int)a[k]<<"\r\n";
	copy_to_clipboard(LOL_1.str());
}
void			polygon_to_clipboard(const int *buffer, int npoints)
{
	std::stringstream LOL_1;
	for(int k=0, nnumbers=npoints<<1;k+1<nnumbers;k+=2)
		LOL_1<<(k>>1)<<":\t"<<buffer[k]<<'\t'<<buffer[k+1]<<"\r\n";
	copy_to_clipboard(LOL_1.str());
}

//profiler
typedef std::pair<std::string, double> ProfInfo;
std::vector<ProfInfo> prof;
int				prof_array_start_idx=0;
long long		prof_t1=0;
void			prof_start()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	prof_t1=li.QuadPart;

//	prof_t1=__rdtsc();
}
void			prof_add(const char *label, int divisor=1)
{
#ifdef PROFILER
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	long long t2=li.QuadPart;
	QueryPerformanceFrequency(&li);
	prof.push_back(ProfInfo(label, 1000.*double(t2-prof_t1)/(li.QuadPart*divisor)));
	QueryPerformanceCounter(&li);
	prof_t1=li.QuadPart;

//	long long t2=__rdtsc();
//	prof.push_back(ProfInfo(std::string(label), double(t2-prof_t1)/divisor));
//	prof_t1=__rdtsc();
#endif
}
void			prof_loop_start(const char **labels, int n)
{
#ifdef PROFILER
	prof_array_start_idx=prof.size();
	for(int k=0;k<n;++k)
		prof.push_back(ProfInfo(labels[k], 0));
#endif
}
void			prof_add_loop(int idx)
{
#ifdef PROFILER
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	long long t2=li.QuadPart;
	QueryPerformanceFrequency(&li);
	prof[prof_array_start_idx+idx].second+=1000.*double(t2-prof_t1)/(li.QuadPart);
	QueryPerformanceCounter(&li);
	prof_t1=li.QuadPart;
#endif
}
void			prof_print()
{
#ifdef PROFILER
	int xpos=w-400, xpos2=w-200;
	for(int k=0, kEnd=prof.size();k<kEnd;++k)
	{
		auto &p=prof[k];
		int ypos=k*18;
		GUIPrint(ghMemDC, xpos, ypos, p.first.c_str());
		GUIPrint(ghMemDC, xpos2, ypos, "%lf", p.second);
	//	GUIPrint(ghMemDC, xpos2, ypos, "%g", p.second);
	}

	//copy to clipboard
	int len=0;
	for(int k=0, kEnd=prof.size();k<kEnd;++k)
		len+=sprintf_s(g_buf+len, g_buf_size-len, "%s\t\t%lf\r\n", prof[k].first.c_str(), prof[k].second);
//	copy_to_clipboard(g_buf, len);

	std::stringstream LOL_1;
	for(int k=0, kEnd=prof.size();k<kEnd;++k)
		LOL_1<<prof[k].first<<"\t\t"<<prof[k].second<<"\r\n";
//	copy_to_clipboard(LOL_1.str());

//	prof.clear();
#endif
}
//end profiler

void			writeworkfolder2wbuf()
{
	time_t open_time=time(0);
	tm open_date;
	localtime_s(&open_date, &open_time);
	swprintf_s(g_wbuf, L"%ls%04d%02d%02d-%02d%02d%02d\\", workspace.c_str(), 1900+open_date.tm_year, open_date.tm_mon+1, open_date.tm_mday, open_date.tm_hour, open_date.tm_min, open_date.tm_sec);
}
void			create_workfolder_from_wbuf()
{
	workfolder.assign(g_wbuf);
	int success=CreateDirectoryW(workfolder.c_str(), nullptr);
	if(!success)
		messagebox(ghWnd, L"Error: CreateDirectory", L"GetLastError() returned %d", GetLastError());
}
inline bool		file_exists(std::wstring const &name)
{
	FILE *file;
	_wfopen_s(&file, name.c_str(), L"r");
	if(file)
	{
		fclose(file);
		return true;
	}
	return false;
}
void			remove_directory(const wchar_t *dir)//Fully qualified name of the directory being deleted, without trailing backslash
{//https://stackoverflow.com/questions/213392/what-is-the-win32-api-function-to-use-to-delete-a-folder
	_SHFILEOPSTRUCTW file_op=
	{
		nullptr,
		FO_DELETE,
		dir,
		L"",

		0,
	//	FOF_NOCONFIRMATION|FOF_NOERRORUI|FOF_SILENT,

		false,
		0,
		L""
	};
	SHFileOperationW(&file_op);
}
void			delete_workfolder()
{
	if(workfolder.size())
	{
	//	MessageBoxW(0, L"About to delete work directory.", L"Information", MB_OK);//does not show
		auto &last=*workfolder.rbegin();
		if(last==L'\\'||last==L'/')
			workfolder.pop_back();
		remove_directory(workfolder.c_str());
		//int success=RemoveDirectoryW(workfolder.c_str());
		//if(!success)
		//{
		//	int errorcode=GetLastError();//ERROR_ALREADY_EXISTS;
		//	messagebox(nullptr, L"Error", L"GetLastError() returned %d", errorcode);
		//}
	}
}
enum			Commands
{
	IDM_FONT_COMBOBOX=1, IDM_FONTSIZE_COMBOBOX, IDM_TEXT_EDIT,

	//menu bar
	IDM_FILE_NEW, IDM_FILE_OPEN, IDM_FILE_SAVE, IDM_FILE_SAVE_AS, IDM_FILE_QUIT,
	IDM_EDIT_UNDO, IDM_EDIT_REPEAT, IDM_EDIT_CUT, IDM_EDIT_COPY, IDM_EDIT_PASTE, IDM_EDIT_CLEAR_SELECTION, IDM_EDIT_SELECT_ALL, IDM_EDIT_COPY_TO, IDM_EDIT_PASTE_FROM,
	IDM_VIEW_TOOLBOX, IDM_VIEW_COLORBOX, IDM_VIEW_STATUSBAR, IDM_VIEW_TEXT_TOOLBAR, IDM_VIEW_ZOOM, IDM_VIEW_VIEWBITMAP,
		IDSM_ZOOM_IN, IDSM_ZOOM_OUT, IDSM_ZOOM_CUSTOM, IDSM_ZOOM_SHOW_GRID, IDSM_ZOOM_SHOW_THUMBNAIL,
	IDM_IMAGE_FLIP_ROTATE, IDM_IMAGE_SKETCH_SKEW, IDM_IMAGE_INVERT_COLORS, IDM_IMAGE_ATTRIBUTES, IDM_IMAGE_CLEAR_IMAGE, IDM_IMAGE_DRAW_OPAQUE,
	IDM_COLORS_EDITCOLORS,
	IDM_HELP_TOPICS, IDM_HELP_ABOUT,
	IDM_TEST_RADIO1, IDM_TEST_RADIO2, IDM_TEST_RADIO3, IDM_TEST_RADIO4, IDM_TEST_RADIO5,

	////toolbar
	//IDM_FREE_SELECTION, IDM_RECT_SELECTION,
	//IDM_ERASER, IDM_FILL,
	//IDM_PICK_COLOR, IDM_MAGNIFIER,
	//IDM_PENCIL, IDM_BRUSH,
	//IDM_AIRBRUSH, IDM_TEXT,
	//IDM_LINE, IDM_CURVE,
	//IDM_RECTANGLE, IDM_POLYGON,
	//IDM_ELLIPSE, IDM_ROUNDED_RECT,

	////color bar
	//IDM_COLOR00, IDM_COLOR01, IDM_COLOR02, IDM_COLOR03, IDM_COLOR04, IDM_COLOR05, IDM_COLOR06, IDM_COLOR07, IDM_COLOR08, IDM_COLOR09, IDM_COLOR10, IDM_COLOR11, IDM_COLOR12, IDM_COLOR13,
	//IDM_COLOR14, IDM_COLOR15, IDM_COLOR16, IDM_COLOR17, IDM_COLOR18, IDM_COLOR19, IDM_COLOR20, IDM_COLOR21, IDM_COLOR22, IDM_COLOR23, IDM_COLOR24, IDM_COLOR25, IDM_COLOR26, IDM_COLOR27
};
enum			MousePosition{MP_NONCLIENT, MP_THUMBNAILBOX, MP_COLORBAR, MP_TOOLBAR, MP_VSCROLL, MP_HSCROLL, MP_IMAGEWINDOW, MP_IMAGE};
int				mousepos=-1;
enum			Focus{F_MAIN, F_TEXTBOX};
int				focus=F_MAIN;
namespace		resources
{
	const unsigned char free_select[]={0,4,17,0,15,18,0,6,17,0,7,17,0,6,18,0,10,17,0,2,17,0,9,17,0,3,17,0,3,17,0,7,17,0,7,17,0,22,17,0,8,17,0,5,17,0,10,17,0,3,17,0,12,17,0,1,18,0,1,18,0,4,18,0,1,20,0,4,17,0,3,17,0,27,17,0,1,17,0,13,18,0,14,17,0,11,};
	const unsigned char select[]={0,49,17,0,1,18,0,1,18,0,1,18,0,1,18,0,1,17,0,1,17,0,13,17,0,17,17,0,13,17,0,1,17,0,13,17,0,17,17,0,13,17,0,1,17,0,13,17,0,17,17,0,13,17,0,1,17,0,1,18,0,1,18,0,1,18,0,1,18,0,1,17,0,32,};
	const unsigned char eraser[]={0,39,22,0,9,17,33,49,33,49,33,49,17,0,7,17,33,49,33,49,33,49,18,0,6,17,33,49,33,49,33,49,17,65,17,0,5,17,33,49,33,49,33,49,17,66,17,0,4,17,33,49,33,49,33,49,17,66,17,0,4,17,33,49,33,49,33,49,17,66,17,0,4,17,86,17,66,17,0,5,17,86,17,65,17,0,6,17,86,18,0,8,23,0,56,};
	const unsigned char fill[]={0,24,18,0,13,17,33,17,0,8,19,0,1,17,35,18,0,5,19,81,17,35,17,34,17,0,3,19,81,17,102,19,0,2,18,81,17,102,21,0,1,18,81,18,97,17,98,21,97,20,0,2,17,98,21,98,20,0,2,17,97,21,98,17,0,1,19,0,3,21,98,17,0,2,19,0,4,19,98,17,0,4,18,0,5,17,98,17,0,5,18,0,6,18,0,7,17,0,29,};
	const unsigned char pick_color[]={0,28,19,0,12,17,81,19,0,10,22,0,8,24,0,9,22,0,9,17,33,20,0,9,17,34,113,18,0,9,17,34,113,17,0,1,17,0,8,17,34,113,17,0,10,17,34,97,17,0,10,17,34,97,17,0,10,17,34,97,17,0,10,97,17,98,17,99,0,7,98,19,101,0,7,103,0,8,};
	const unsigned char magnifier[]={0,4,20,0,10,18,84,18,0,7,17,113,35,83,113,17,0,6,17,34,86,17,0,5,17,81,33,88,17,0,4,17,81,33,88,17,0,4,17,90,17,0,4,17,90,17,0,5,17,87,33,17,0,6,17,113,85,33,113,17,113,0,6,18,83,33,113,131,0,8,20,129,81,131,0,12,129,81,131,0,12,129,81,131,0,12,129,81,130,0,13,130,0,2,};
	const unsigned char pencil[]={0,9,19,0,12,17,163,17,0,11,17,161,34,17,0,10,18,161,33,17,0,11,17,49,19,0,10,17,49,33,81,17,0,11,17,33,49,18,0,10,17,33,49,81,17,0,11,17,49,33,18,0,10,17,49,33,81,17,0,11,18,49,18,0,11,20,0,12,19,0,13,18,0,14,17,0,27,};
	const unsigned char brush[]={0,7,18,0,13,17,66,17,0,12,17,66,17,0,12,17,66,17,0,12,17,66,17,0,12,17,66,17,0,12,17,66,17,0,11,17,68,17,0,9,17,70,17,0,8,24,0,8,24,0,8,17,38,17,0,8,17,33,113,33,113,34,17,0,8,17,33,113,33,113,34,17,0,8,17,33,113,33,113,34,17,0,7,25,0,4,};
	const unsigned char airbrush[]={0,18,129,0,1,129,0,2,129,0,9,129,81,129,81,131,19,0,5,129,81,129,81,129,0,1,18,34,18,0,5,129,81,129,0,2,17,34,18,146,0,3,129,81,129,0,3,17,33,18,147,17,0,3,129,81,129,0,2,19,145,17,147,17,0,1,129,81,129,0,4,19,145,17,147,17,0,1,129,0,6,19,145,17,146,17,129,81,129,0,6,19,145,18,0,2,129,0,8,20,0,2,129,0,10,18,0,4,129,0,14,129,0,47,};
	const unsigned char text[]={0,22,17,0,14,19,0,13,19,0,13,20,0,11,17,0,1,19,0,11,17,0,1,20,0,9,17,0,3,19,0,9,17,0,3,20,0,8,24,0,7,17,0,5,20,0,6,17,0,6,19,0,4,20,0,3,23,0,50,};
	const unsigned char line[]={0,17,18,0,15,18,0,15,18,0,15,18,0,15,18,0,15,18,0,15,18,0,15,18,0,15,18,0,15,18,0,15,18,0,15,18,0,15,18,0,33,};
	const unsigned char curve[]={0,6,18,0,15,18,0,15,18,0,15,17,0,15,17,0,14,18,0,13,18,0,13,18,0,13,18,0,13,18,0,14,17,0,15,17,0,15,18,0,15,18,0,15,18,0,24,};
	const unsigned char rectangle[]={0,32,31,0,1,17,93,17,0,1,17,93,17,0,1,17,93,17,0,1,17,93,17,0,1,17,93,17,0,1,17,93,17,0,1,17,93,17,0,1,17,93,17,0,1,17,93,17,0,1,31,0,49,};
	const unsigned char polygon[]={0,20,25,0,7,17,86,17,0,8,17,86,17,0,7,17,86,17,0,8,17,86,17,0,7,17,86,17,0,8,17,86,17,0,7,17,86,17,0,8,17,85,23,0,3,17,91,17,0,2,17,92,17,0,2,17,92,17,0,2,30,0,34,};
	const unsigned char ellipse[]={0,52,23,0,7,18,87,18,0,4,17,91,17,0,2,17,93,17,0,1,17,93,17,0,1,17,93,17,0,1,17,93,17,0,2,17,91,17,0,4,18,87,18,0,7,23,0,53,};
	const unsigned char rounded_rectangle[]={0,34,27,0,4,17,91,17,0,2,17,93,17,0,1,17,93,17,0,1,17,93,17,0,1,17,93,17,0,1,17,93,17,0,1,17,93,17,0,1,17,93,17,0,2,17,91,17,0,4,27,0,51,};

	const char selection_transparency[]=
	{
		0,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,0,0,0,0,0,0,0,
		0,64,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,64,0,0,0,0,0,0,0,
		0,64,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,64,0,0,0,0,0,0,0,
		0,64,124,124,124,124,106,65,65,65,65,85,124,124,124,124,124,124,124,64,64,124,64,64,124,64,64,124,64,64,0,64,64,0,64,64,
		0,64,124,124,124,106,65,67,67,67,67,65,85,124,124,124,124,124,64,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,64,
		0,64,124,124,124,65,106,67,67,67,67,67,64,124,124,124,124,124,64,127,127,127,127,127,127,68,68,68,68,68,68,85,127,127,127,0,
		0,64,124,124,124,65,106,106,67,67,67,106,64,124,124,124,124,124,124,127,127,127,127,127,68,68,68,68,68,68,106,64,127,127,127,64,
		0,64,124,124,124,65,106,127,106,106,106,65,64,124,124,124,124,124,64,127,127,127,127,68,68,68,68,68,68,106,68,64,127,127,127,64,
		0,64,124,124,124,65,106,127,67,67,67,65,64,124,124,124,124,124,64,127,127,127,68,127,127,127,127,127,127,68,68,64,127,127,127,0,
		0,64,124,124,124,65,106,127,67,67,67,65,64,124,124,124,124,124,124,127,127,127,68,127,76,76,76,76,76,68,68,64,127,127,127,64,
		0,64,124,124,124,65,106,127,67,67,67,65,64,124,124,124,124,124,64,127,127,127,68,127,76,76,76,76,76,68,68,64,127,127,127,64,
		0,64,124,124,124,65,106,127,67,67,67,65,64,124,80,80,80,80,64,127,127,127,68,127,76,76,76,76,76,68,68,64,127,127,127,0,
		0,64,124,124,124,65,106,127,67,67,67,65,80,80,106,115,112,64,80,127,127,127,68,127,76,76,76,76,76,68,68,64,85,85,85,64,
		0,64,124,124,124,65,106,127,67,67,67,65,80,106,115,112,112,112,64,127,127,127,68,127,76,76,76,76,76,68,64,85,85,85,85,64,
		0,64,124,124,124,65,106,127,67,67,67,80,106,115,112,112,112,112,64,127,127,127,68,127,76,76,76,76,76,64,85,85,85,85,127,0,
		0,64,124,124,124,65,67,127,67,67,67,80,106,115,112,112,112,112,112,127,127,127,64,64,64,64,64,64,64,85,85,85,85,127,127,64,
		0,64,124,124,124,106,65,65,65,65,65,80,106,115,112,112,112,112,64,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,64,
		0,64,124,124,124,124,106,64,64,64,64,80,106,115,112,112,112,112,64,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,0,
		0,64,124,124,124,124,124,124,124,124,124,85,80,106,115,112,112,64,80,80,64,84,64,64,124,64,64,124,64,64,0,64,64,0,64,64,
		0,64,124,124,124,124,124,124,124,124,124,124,80,80,80,80,80,80,64,64,84,84,84,84,124,124,124,124,64,0,0,0,0,0,0,0,
		0,64,124,124,124,124,124,124,124,124,124,124,124,85,64,64,64,64,85,84,84,84,106,124,124,124,124,124,64,0,0,0,0,0,0,0,
		0,64,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,64,0,0,0,0,0,0,0,
		0,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,0,0,0,0,0,0,0,
		0,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,0,0,0,0,0,0,0,
		0,64,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,64,0,0,0,0,0,0,0,
		0,64,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,64,0,0,0,0,0,0,0,
		0,64,124,124,124,124,106,65,65,65,65,85,124,124,124,124,124,124,124,64,64,124,64,64,124,64,64,124,64,64,0,64,64,0,64,64,
		0,64,124,124,124,106,65,67,67,67,67,65,85,124,124,124,124,124,64,124,124,124,124,124,124,124,124,124,64,0,0,0,0,0,0,64,
		0,64,124,124,124,65,106,67,67,67,67,67,64,124,124,124,124,124,64,124,124,124,124,124,124,68,68,68,68,68,68,85,0,0,0,0,
		0,64,124,124,124,65,106,106,67,67,67,106,64,124,124,124,124,124,124,124,124,124,124,124,68,68,68,68,68,68,106,64,0,0,0,64,
		0,64,124,124,124,65,106,127,106,106,106,65,64,124,124,124,124,124,64,124,124,124,124,68,68,68,68,68,68,106,68,64,0,0,0,64,
		0,64,124,124,124,65,106,127,67,67,67,65,64,124,124,124,124,124,64,124,124,124,68,127,127,127,127,127,127,68,68,64,0,0,0,0,
		0,64,124,124,124,65,106,127,67,67,67,65,64,124,124,124,124,124,124,124,124,124,68,127,76,76,76,76,76,68,68,64,0,0,0,64,
		0,64,124,124,124,65,106,127,67,67,67,65,64,124,124,124,124,124,64,124,124,124,68,127,76,76,76,76,76,68,68,64,0,0,0,64,
		0,64,124,124,124,65,106,127,67,67,67,65,64,124,80,80,80,80,64,124,124,124,68,127,76,76,76,76,76,68,68,64,0,0,0,0,
		0,64,124,124,124,65,106,127,67,67,67,65,80,80,106,115,112,64,80,80,124,124,68,127,76,76,76,76,76,68,68,64,0,0,0,64,
		0,64,124,124,124,65,106,127,67,67,67,65,80,106,115,112,112,112,64,80,85,124,68,127,76,76,76,76,76,68,64,0,0,0,0,64,
		0,64,124,124,124,65,106,127,67,67,67,80,106,115,112,112,112,112,64,80,64,124,68,127,76,76,76,76,76,64,0,0,0,0,0,0,
		0,64,124,124,124,65,67,127,67,67,67,80,106,115,112,112,112,112,112,80,64,124,64,64,64,64,64,64,64,0,0,0,0,0,0,64,
		0,64,124,124,124,106,65,65,65,65,65,80,106,115,112,112,112,112,64,80,64,84,106,124,124,124,124,124,64,0,0,0,0,0,0,64,
		0,64,124,124,124,124,106,64,64,64,64,80,106,115,112,112,112,112,64,80,64,84,84,84,124,124,124,124,64,0,0,0,0,0,0,0,
		0,64,124,124,124,124,124,124,124,124,124,85,80,106,115,112,112,64,80,80,64,84,64,64,124,64,64,124,64,64,0,64,64,0,64,64,
		0,64,124,124,124,124,124,124,124,124,124,124,80,80,80,80,80,80,64,64,84,84,84,84,124,124,124,124,64,0,0,0,0,0,0,0,
		0,64,124,124,124,124,124,124,124,124,124,124,124,85,64,64,64,64,85,84,84,84,106,124,124,124,124,124,64,0,0,0,0,0,0,0,
		0,64,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,64,0,0,0,0,0,0,0,
		0,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,0,0,0,0,0,0,0,
	};

	//all brushes
	const int width1[]=
	{
		0, 0
	};
	const int width2[]=
	{
		-1, 0,
		-1, 0
	};
	const int width3[]=
	{
		0, 0,
		-1, 1,
		0, 0
	};
	const int width4[]=
	{
		-1, 0,
		-2, 1,
		-2, 1,
		-1, 0
	};
	const int width5[]=
	{
		-1, 1,
		-2, 2,
		-2, 2,
		-2, 2,
		-1, 1
	};
	const int eraser04[]=
	{
		-2, 1,
		-2, 1,
		-2, 1,
		-2, 1,
	};
	const int eraser06[]=
	{
		-3, 2,
		-3, 2,
		-3, 2,
		-3, 2,
		-3, 2,
		-3, 2,
	};
	const int eraser08[]=//large_square
	{
		-4, 3,
		-4, 3,
		-4, 3,
		-4, 3,
		-4, 3,
		-4, 3,
		-4, 3,
		-4, 3
	};
	const int eraser10[]=
	{
		-5, 4,
		-5, 4,
		-5, 4,
		-5, 4,
		-5, 4,
		-5, 4,
		-5, 4,
		-5, 4,
		-5, 4,
		-5, 4,
	};
	const int large_disk[]=
	{
		-1, 1,
		-2, 2,
		-3, 3,
		-3, 3,
		-3, 3,
		-2, 2,
		-1, 1
	};
	const int disk[]=
	{
		-1, 0,
		-2, 1,
		-2, 1,
		-1, 0
	};
	const int dot[]=
	{
		0, 0
	};
	const int *large_square=eraser08;
	const int square[]=
	{
		-2, 2,
		-2, 2,
		-2, 2,
		-2, 2,
		-2, 2
	};
	const int small_square[]=
	{
		-1, 0,
		-1, 0
	};
	const int large_right45[]=
	{
		3, 3,
		2, 2,
		1, 1,
		0, 0,
		-1, -1,
		-2, -2,
		-3, -3,
		-4, -4
	};
	const int right45[]=
	{
		2, 2,
		1, 1,
		0, 0,
		-1, -1,
		-2, -2
	};
	const int small_right45[]=
	{
		0, 0,
		-1, -1
	};
	const int large_left45[]=
	{
		-4, -4,
		-3, -3,
		-2, -2,
		-1, -1,
		0, 0,
		1, 1,
		2, 2,
		3, 3
	};
	const int left45[]=
	{
		-2, -2,
		-1, -1,
		0, 0,
		1, 1,
		2, 2
	};
	const int small_left45[]=
	{
		-1, -1,
		0, 0
	};
	struct		Brush
	{
		const int *bounds;
		int yoffset, ysize;
	};
	const Brush brushes[]=
	{
		{nullptr, 0, 0},
		{width1, 0, 1}, {width2, -1, 2}, {width3, -1, 3}, {width4, -2, 4}, {width5, -2, 5},
		{eraser04, -2, 4}, {eraser06, -3, 6}, {eraser08, -4, 8}, {eraser10, -5, 10},
		{large_disk, -3, 7}, {disk, -2, 4}, {dot, 0, 1},
		{large_square, -4, 8}, {square, -2, 5}, {small_square, -1, 2},
		{large_right45, -4, 8}, {right45, -2, 5}, {small_right45, -1, 2},
		{large_left45, -4, 8}, {left45, -2, 5}, {small_left45, -1, 2}
	};
}
void			unpack_icon(const unsigned char *rle, int csize, int *rgb2)
{
	enum Color{C_TRANSPARENT, C_BLACK, C_WHITE, C_YELLOW, C_MUSTARD, C_GREY, C_MARINE, C_DARK_GREY, C_DARK_BLUE, C_BLUE, C_DARK_RED};
	for(int kp=0, kc=0;kc<csize;++kc)
	{
		auto &latest=rle[kc];
		int color, count;
		if(latest)
			color=latest>>4, count=latest&15;
		else//zero is transparent, count is in the next cell
			++kc, color=0, count=rle[kc];
		switch(color)//0x??RRGGBB
		{
		case C_TRANSPARENT:	color=0x00FF00FF;break;//transparent is pink
	//	case C_TRANSPARENT:	color=0x00E3E3E3;break;
	//	case C_TRANSPARENT:	color=0x00F0F0F0;break;
		case C_BLACK:		color=0x00000000;break;
		case C_WHITE:		color=0x00FFFFFF;break;
		case C_YELLOW:		color=0x00FFFF00;break;
		case C_MUSTARD:		color=0x00BFBF00;break;
		case C_GREY:		color=0x00C0C0C0;break;
		case C_MARINE:		color=0x0000BFBF;break;
		case C_DARK_GREY:	color=0x00808080;break;
		case C_DARK_BLUE:	color=0x000000BF;break;
		case C_BLUE:		color=0x000000FF;break;
		case C_DARK_RED:	color=0x00BF0000;break;
		default:
			{
				int LOL_1=0;//unreachable
			}
			break;
		}
		int k2=0;
		for(;k2<count;++k2)
		{
			int idx=kp+k2;
			int ki=idx>>8, kx=idx&15, ky=idx>>4&15;
		//	if(color!=0xFFF0F0F0)//
				rgb2[ky<<4|kx]=color;
		}
		kp+=k2;
	}
}
HMENU			hMenubar=0,		hMenuFile=0, hMenuEdit=0, hMenuView=0, hMenuZoom=0, hMenuImage=0, hMenuColors=0, hMenuHelp=0, hMenuTest=0;

HCURSOR			hcursor_original=(HCURSOR)LoadCursorW(nullptr, IDC_ARROW),
				hcursor_cross=(HCURSOR)LoadCursorW(nullptr, IDC_CROSS),
				hcursor=hcursor_cross;
int				*icons=nullptr, nicons=0,//16x16 icons, buffer size = nicons*256
				*icons_seltype=nullptr;//2 icons of w=36, h=23
int				colorpalette[]=
{
	0xFF000000, 0xFFFFFFFF,//column major (transposed)
	0xFF808080, 0xFFC0C0C0,
	0xFF800000, 0xFFFF0000,
	0xFF808000, 0xFFFFFF00,
	0xFF008000, 0xFF00FF00,
	0xFF008080, 0xFF00FFFF,
	0xFF000080, 0xFF0000FF,
	0xFF800080, 0xFFFF00FF,
	0xFF808040, 0xFFFFFF80,
	0xFF004040, 0xFF00FF80,
	0xFF0080FF, 0xFF80FFFF,
	0xFF0040FF, 0xFF8080FF,
	0xFF8000FF, 0xFFFF0080,
	0xFF804000, 0xFFFF8040,
}, primary_idx=0;
int				ext_colorpalette[16];//extended color palette
int				primarycolor=colorpalette[0], secondarycolor=colorpalette[1],//0xAARRGGBB
				primary_alpha=255, secondary_alpha=255;
inline void		assign_alpha(int &color, int alpha){color=alpha<<24|color&0x00FFFFFF;}
bool			showgrid=false,
				showcolorbar=true, showtoolbox=true, showthumbnailbox=false;
			//	show_fliprotate=false, show_stretchskew=false;
enum			ColorbarShow{CS_NONE, CS_TEXTOPTIONS, CS_FLIPROTATE, CS_STRETCHSKEW};
int				colorbarcontents=CS_NONE;//additional options in color bar
enum			Mode
{
	M_FREE_SELECTION, M_RECT_SELECTION,
	M_ERASER, M_FILL,
	M_PICK_COLOR, M_MAGNIFIER,
	M_PENCIL, M_BRUSH,
	M_AIRBRUSH, M_TEXT,
	M_LINE, M_CURVE,
	M_RECTANGLE, M_POLYGON,
	M_ELLIPSE, M_ROUNDED_RECT,
};
enum			BrushType//see resources::brushes
{
	BRUSH_NONE,

	LINE1, LINE2, LINE3, LINE4, LINE5,

	ERASER04, ERASER06, ERASER08, ERASER10,

	BRUSH_LARGE_DISK, BRUSH_DISK, BRUSH_DOT,
	BRUSH_LARGE_SQUARE, BRUSH_SQUARE, BRUSH_SMALL_SQUARE,
	BRUSH_LARGE_RIGHT45, BRUSH_RIGHT45, BRUSH_SMALL_RIGHT45,
	BRUSH_LARGE_LEFT45, BRUSH_LEFT45, BRUSH_SMALL_LEFT45,
};
enum			AirbrushSize{S_SMALL, S_MEDIUM, S_LARGE};
enum			ShapeType
{
	ST_HOLLOW,	//outline
	ST_FULL,	//outline + fill
	ST_FILL,	//only fill
};
enum			InterpolationType{I_NEAREST, I_BILINEAR};
int				currentmode=M_PENCIL, prevmode=M_PENCIL,
				selection_free=false,//true: free-form selection, false: rectangular selection
				selection_transparency=TRANSPARENT,//opaque or transparent
				selection_persistent=false,//selection remains after switching to another tool
				erasertype=ERASER08,
				magnifier_level=0,//log pixel size {0:1, 1:2, 2:4, 3:8, 4:16, 5:32}
				brushtype=BRUSH_DISK,
				airbrush_size=S_SMALL, airbrush_cpt=18, airbrush_timer=10,
				linewidth=1,//[1, 5]
				rectangle_type=ST_HOLLOW, polygon_type=ST_HOLLOW, ellipse_type=ST_HOLLOW, roundrect_type=ST_HOLLOW,
				interpolation_type=I_BILINEAR;
struct			Point;
struct			Point2d//for bezier curve
{
	double x, y;
	Point2d():x(0), y(0){}
	Point2d(double x, double y):x(x), y(y){}
	Point2d(Point const &p);
	Point2d& operator=(Point const &p);
	void set(double x, double y){this->x=x, this->y=y;}
	void setzero(){x=y=0;}
	Point2d& operator+=(Point2d const &b){x+=b.x, y+=b.y;return *this;}
	void rotate(double cth, double sth)
	{
		double x2=x*cth+y*sth, y2=-x*sth+y*cth;
		set(x2, y2);
	}
	void transform(double A, double B, double C, double D)
	{
		double x2=x*A+y*B, y2=x*C+y*D;
		set(x2, y2);
	}
};
Point2d			operator+(Point2d const &a, Point2d const &b){return Point2d(a.x+b.x, a.y+b.y);}
Point2d			operator-(Point2d const &a, Point2d const &b){return Point2d(a.x-b.x, a.y-b.y);}
//bool			operator!=(Point2d const &a, Point2d const &b){return a.x!=b.x||a.y!=b.y;}
struct			Point
{
	int x, y;
	Point():x(0), y(0){}
	Point(int x, int y):x(x), y(y){}
	Point(Point2d const &p):x((int)round(p.x)), y((int)round(p.y)){}
	void set(int x, int y){this->x=x, this->y=y;}
	void setzero(){x=y=0;}
	Point& operator+=(Point const &b){x+=b.x, y+=b.y;return *this;}
};
Point2d::Point2d(Point const &p):x(p.x), y(p.y){}
Point2d&		Point2d::operator=(Point const &p){x=p.x, y=p.y;return *this;}
Point2d			operator*(Point const &p, double t){return Point2d(p.x*t, p.y*t);}
Point2d			operator*(double t, Point const &p){return Point2d(p.x*t, p.y*t);}
Point			operator-(Point const &a, Point const &b){return Point(a.x-b.x, a.y-b.y);}
bool			operator!=(Point const &a, Point const &b){return a.x!=b.x||a.y!=b.y;}
Point			sel_start, sel_end,//selection, image coordinates			//TODO: remove sel_start, sel_end
				sel_s1, sel_s2,//ordered screen coordinates for mouse test
				selpos;//position of top-right corner of selection in image coordinates
bool			selection_moved=false;

Point			text_start, text_end,//static point -> moving point, image coordinates
				text_s1, text_s2,//ordered screen coordinates
				textpos;//position of top-right corner of textbox in image coordinates

//struct			Pixel
//{
//	int x, y;
//	int color_before, color_after;
//};
//struct			Action
//{
//	//				type	data
//	//selection		's'		x, y, w, h, pixels...
//	char type;
////	std::vector<Pixel> buffer;
////	std::vector<int> forward, back;
//	std::vector<int> data;
//	Action(char type):type(type){}
//};
//std::vector<Action> history;
//int				histpos;
//void			resizeimage()
//{
//}
struct			HistoryFrame
{
	int *buffer;
	int iw, ih;
	HistoryFrame():buffer(nullptr), iw(0), ih(0){}
	HistoryFrame(int *buffer, int iw, int ih):buffer(buffer), iw(iw), ih(ih){}
};
std::vector<HistoryFrame> history;
int				histpos=0;
int*			hist_start(int iw, int ih)//current buffer is in history[histpos]
{
	for(int k=0, nsteps=history.size();k<nsteps;++k)
	{
		auto &buf=history[k].buffer;
		free(buf), buf=nullptr;
	}
	history.clear();
	int image_size=iw*ih;
	int *image=(int*)malloc(image_size<<2);
	history.push_back(HistoryFrame(image, iw, ih)), histpos=0;
	return image;
}
void			hist_premodify(int *&buffer, int nw, int nh)//nw, nh: new dimensions
{
	modify();
	iw=nw, ih=nh, image_size=iw*ih, buffer=(int*)malloc(image_size<<2);
	auto &oldhframe=history[histpos];
	int *&obuffer=oldhframe.buffer, &ow=oldhframe.iw, &oh=oldhframe.ih;
	if(nh>oh||nw>ow)
		memset(buffer, 0xFF, image_size<<2);
	int mw=minimum(ow, nw), mh=minimum(oh, nh);
	if(nw==ow)
		memcpy(buffer, obuffer, ow*mh<<2);
	else
	{
		for(int ky=0;ky<mh;++ky)
			memcpy(buffer+nw*ky, obuffer+ow*ky, mw<<2);
	}
	if(unsigned(histpos+1)<history.size())
	{
		for(int kh=histpos+1, khEnd=history.size();kh<khEnd;++kh)
			free(history[kh].buffer);
		history.resize(histpos+1);
	}
	history.push_back(HistoryFrame(buffer, iw, ih)), ++histpos;
}
void			hist_postmodify(int *buffer, int nw, int nh)
{
	modify();
	image=buffer, iw=nw, ih=nh;
	if(unsigned(histpos+1)<history.size())
	{
		for(int kh=histpos+1, khEnd=history.size();kh<khEnd;++kh)
			free(history[kh].buffer);
		history.resize(histpos+1);
	}
	history.push_back(HistoryFrame(buffer, iw, ih)), ++histpos;
}
void			hist_undo(int *&buffer)
{
	if(histpos>0)
	{
		--histpos;
		auto &chf=history[histpos];//current history frame
		buffer=chf.buffer, iw=chf.iw, ih=chf.ih;
	}
}
void			hist_redo(int *&buffer)
{
	if((unsigned)histpos+1<history.size())
	{
		++histpos;
		auto &chf=history[histpos];//current history frame
		buffer=chf.buffer, iw=chf.iw, ih=chf.ih;
	}
}

inline int		swap_rb(int color)
{
	auto p=(unsigned char*)&color;
	auto temp=p[0];
	p[0]=p[2], p[2]=temp;
	return color;
}
inline bool		checkx(int x){return x<0||x>=w;}
inline bool		checky(int y){return y<0||y>=h;}
inline bool		check(int x, int y){return x<0||x>=w||y<0||y>=h;}
inline bool		icheckx(int x){return x<0||x>=iw;}
inline bool		ichecky(int y){return y<0||y>=ih;}
inline bool		icheck(int x, int y){return x<0||x>=iw||y<0||y>=ih;}
void			rectangle(int x1, int x2, int y1, int y2, int color)
{
	if(check(x1, y1)||check(x2-1, y2-1))//
		return;//
	for(int ky=y1;ky<y2;++ky)
		for(int kx=x1;kx<x2;++kx)
			rgb[w*ky+kx]=color;
}
void			rect_checkboard(int x1, int x2, int y1, int y2, int color1, int color2)
{
	if(check(x1, y1)||check(x2-1, y2-1))//
		return;//
	int colors[]={color1, color2};
	for(int ky=y1;ky<y2;++ky)
		for(int kx=x1;kx<x2;++kx)
			rgb[w*ky+kx]=colors[(kx+ky)&1];
}
void			h_line(int x1, int x2, int y, int color)
{
	if(checkx(x1)||checkx(x2-1)||checky(y))//
		return;//
	for(int kx=x1;kx<x2;++kx)
		rgb[w*y+kx]=color;
}
void			v_line(int x, int y1, int y2, int color)
{
	if(checkx(x)||checky(y1)||checky(y2-1))//
		return;//
	for(int ky=y1;ky<y2;++ky)
		rgb[w*ky+x]=color;
}
void			h_line_add(int x1, int x2, int y, int ammount)
{
	if(checkx(x1)||checkx(x2-1)||checky(y))//
		return;//
	for(int kx=x1;kx<x2;++kx)
	{
		auto p=(unsigned char*)(rgb+w*y+kx);
		p[0]+=ammount;
		p[1]+=ammount;
		p[2]+=ammount;
	}
}
void			v_line_add(int x, int y1, int y2, int ammount)
{
	if(checkx(x)||checky(y1)||checky(y2-1))//
		return;//
	for(int ky=y1;ky<y2;++ky)
	{
		auto p=(unsigned char*)(rgb+w*ky+x);
		p[0]+=ammount;
		p[1]+=ammount;
		p[2]+=ammount;
	}
}
void			h_line_alt(int x1, int x2, int y, int *colors)
{
	if(checkx(x1)||checkx(x2-1)||checky(y))//
		return;//
	for(int kx=x1;kx<x2;++kx)
		rgb[w*y+kx]=colors[(kx-x1)&1];
}
void			v_line_alt(int x, int y1, int y2, int *colors)
{
	if(checkx(x)||checky(y1)||checky(y2-1))//
		return;//
	for(int ky=y1;ky<y2;++ky)
		rgb[w*ky+x]=colors[(ky-y1)&1];
}
void			rectangle_hollow(int x1, int x2, int y1, int y2, int color)//sorted coordinates
{
	h_line(x1, x2, y1, color);
	h_line(x1, x2, y2-1, color);
	v_line(x1, y1+1, y2-1, color);
	v_line(x2-1, y1+1, y2-1, color);
}
void			line45_forward(int x1, int y1, int n, int color)//top-left (x1, y2) -> bottom-right
{
	if(check(x1, y1)||check(x1+n, y1+n))
		return;
	for(int k=0;k<n;++k)
		rgb[w*(y1+k)+x1+k]=color;
}
void			line45_sideways(int x1, int y1, int n, int color)//bottom-right (x1, y1) -> top-left
{
	if(check(x1, y1)||check(x1+n, y1-n))
		return;
	for(int k=0;k<n;++k)
		rgb[w*(y1-k)+x1+k]=color;
}
void			_3d_hole(int x1, int x2, int y1, int y2)
{
	h_line(x1, x2-1, y1, 0xA0A0A0);//top
	h_line(x1+1, x2-2, y1+1, 0x000000);
	v_line(x1, y1+1, y2-1, 0xA0A0A0);//left
	v_line(x1+1, y1+2, y2-2, 0x000000);
	
	v_line(x2-1, y1, y2, 0xFFFFFF);//right
	v_line(x2-2, y1+1, y2-1, 0xF0F0F0);//
	h_line(x1, x2-1, y2-1, 0xFFFFFF);//bottom
	h_line(x1+1, x2-2, y2-2, 0xF0F0F0);//
}
void			_3d_border(int x1, int x2, int y1, int y2)
{
	h_line(x1, x2, y1, 0xFFFFFF);//top
	h_line(x1+1, x2-1, y1+1, 0xF0F0F0);
	v_line(x1, y1+1, y2, 0xFFFFFF);//left
	v_line(x1+1, y1+2, y2-1, 0xF0F0F0);

	v_line(x2-1, y1+1, y2-1, 0xA0A0A0);//right
	v_line(x2-2, y1+2, y2-2, 0xF0F0F0);
	h_line(x1+1, x2-1, y2-2, 0xF0F0F0);//bottom
	h_line(x1+1, x2, y2-1, 0xA0A0A0);
}
inline void		screen2image(int sx, int sy, int &ix, int &iy)
{
	ix=spx+shift(sx-(58*showtoolbox+3), -logzoom);
	iy=spy+shift(sy-5, -logzoom);
	//ix=spx+shift(sx-61, -logzoom);
	//iy=spy+shift(sy-5, -logzoom);
}
inline void		screen2image(int sx, int sy, double &ix, double &iy)
{
	ix=spx+(sx-(58*showtoolbox+3))/zoom;
	iy=spy+(sy-5)/zoom;
	//ix=spx+(sx-61)/zoom;
	//iy=spy+(sy-5)/zoom;
}
inline void		image2screen(int ix, int iy, int &sx, int &sy)
{
	//sx=int((ix-spx)*zoom+(58*showtoolbox+3));
	//sy=int((iy-spy)*zoom+5);
	sx=shift(ix-spx, logzoom)+(58*showtoolbox+3);
	sy=shift(iy-spy, logzoom)+5;
}
void			scrollbar_slider(int &winpos_ip, int imsize_ip, int winsize, int barsize, double zoom, int &sliderstart, int &slidersize)
{
	const int scrollbarsize=17;
	double winsize_ip=winsize/zoom;
	slidersize=int(barsize*winsize_ip/imsize_ip);
	if(slidersize<scrollbarsize)
	{
		slidersize=scrollbarsize;
		sliderstart=int((barsize-scrollbarsize)*winpos_ip/(imsize_ip-winsize_ip));
	}
	else
		sliderstart=barsize*winpos_ip/imsize_ip;
	if(sliderstart>barsize-slidersize)
	{
		sliderstart=barsize-slidersize;
		winpos_ip=int((imsize_ip+2-winsize_ip)*sliderstart/(barsize-slidersize));
	}
}
void			scrollbar_scroll(int &winpos_ip, int imsize_ip, int winsize, int barsize, int slider0, int mp_slider0, int mp_slider, double zoom, int &sliderstart, int &slidersize)//_ip: in image pixels, mp_slider: mouse position starting from start of slider
{
	const int scrollbarsize=17;
	double winsize_ip=winsize/zoom, r=winsize_ip/imsize_ip;
	slidersize=int(barsize*r);
	bool minsize=slidersize<scrollbarsize;
	if(minsize)
		slidersize=scrollbarsize;
	sliderstart=slider0+mp_slider-mp_slider0;
	if(sliderstart<0)
		sliderstart=0;
	if(sliderstart>barsize-slidersize)
		sliderstart=barsize-slidersize;
	winpos_ip=int((imsize_ip+2-winsize_ip)*sliderstart/(barsize-slidersize));
}

void			selection_sort(Point const &sel_start, Point const &sel_end, Point &p1, Point &p2)
{
	p1.set(minimum(sel_start.x, sel_end.x), minimum(sel_start.y, sel_end.y));
	p2.set(maximum(sel_start.x, sel_end.x), maximum(sel_start.y, sel_end.y));
}
void			selection_sortNbound(Point const &sel_start, Point const &sel_end, Point &p1, Point &p2)
{
	p1.set(minimum(sel_start.x, sel_end.x), minimum(sel_start.y, sel_end.y));
	p2.set(maximum(sel_start.x, sel_end.x), maximum(sel_start.y, sel_end.y));
	if(p1.x<0)
		p1.x=0;
	if(p2.x>iw)
		p2.x=iw;
	if(p1.y<0)
		p1.y=0;
	if(p2.y>ih)
		p2.y=ih;
}
void			selection_assign_mouse(int start_mx, int start_my, int mx, int my, Point &sel_start, Point &sel_end, Point &p1, Point &p2)
{
	screen2image(start_mx, start_my, sel_start.x, sel_start.y);
	screen2image(mx, my, sel_end.x, sel_end.y);
	selection_sortNbound(sel_start, sel_end, p1, p2);
}
void			selection_assign()
{
	Point p1, p2;
	selection_sortNbound(sel_start, sel_end, p1, p2);
	image2screen(p1.x, p1.y, sel_s1.x, sel_s1.y);
	image2screen(p2.x, p2.y, sel_s2.x, sel_s2.y);
	selpos=p1;
}
void			selection_remove()
{
	sw=0, sh=0, sel_buffer=(int*)realloc(sel_buffer, 0);
	sel_start.set(0, 0), sel_end.set(0, 0);
	selection_assign();
}
void			selection_select(Point const &p1, Point const &p2)//p1, p2: sorted-bounded image coordinates
{
	int dx=p2.x-p1.x, dy=p2.y-p1.y, kx, ky;
	if(dx>0&&dy>0)
	{
		selpos=p1;
		sw=dx, sh=dy, sel_buffer=(int*)realloc(sel_buffer, sw*sh<<2);
		for(ky=0;ky<dy;++ky)
			memcpy(sel_buffer+sw*ky, image+iw*(p1.y+ky)+p1.x, dx<<2);
	//	if(!kb[VK_CONTROL]||kb['A'])
		for(ky=p1.y;ky<p2.y;++ky)
			for(kx=p1.x;kx<p2.x;++kx)
				image[iw*ky+kx]=secondarycolor;
	}
	selection_free=false;
}
void			selection_deselect()//sel_buffer -> image
{
	if(selection_moved)
		modify();
	selection_moved=false;
	if(sel_start!=sel_end)
	{
		hist_premodify(image, iw, ih);
		//Point p1, p2;//sorted-bounded selection in image coordinates
		//selection_sortNbound(sel_start, sel_end, p1, p2);
		int ystart=selpos.y>0?0:-selpos.y, yend=minimum(ih-selpos.y, sh),
			xstart=selpos.x>0?0:-selpos.x, xend=minimum(iw-selpos.x, sw);
		if(selection_free)
		{
			for(int ky=ystart;ky<yend;++ky)
			{
				for(int kx=xstart;kx<xend;++kx)
				{
					int sel_idx=sw*ky+kx;
					if(sel_mask[sel_idx])
					{
						auto &src=sel_buffer[sel_idx];
						if(selection_transparency==OPAQUE||src!=secondarycolor)
							image[iw*(selpos.y+ky)+selpos.x+kx]=src;
					}
				}
			}
		}
		else
		{
			if(selection_transparency==TRANSPARENT)
			{
				for(int ky=ystart;ky<yend;++ky)
				{
					for(int kx=xstart;kx<xend;++kx)
					{
						//auto dst=(unsigned char*)(image+iw*(selpos.y+ky)+selpos.x+kx), src=(unsigned char*)(sel_buffer+sw*ky+kx);
						//auto &alpha=src[3];
						//dst[0]=dst[0]+((src[0]-dst[0])*alpha>>8);
						//dst[1]=dst[1]+((src[1]-dst[1])*alpha>>8);
						//dst[2]=dst[2]+((src[2]-dst[2])*alpha>>8);
						auto &src=sel_buffer[sw*ky+kx];
						if(src!=secondarycolor)
							image[iw*(selpos.y+ky)+selpos.x+kx]=src;
					}
				}
			}
			else
			{
				for(int ky=ystart;ky<yend;++ky)
					memcpy(image+iw*(selpos.y+ky)+selpos.x, sel_buffer+sw*ky, (xend-xstart)<<2);
				//	memcpy(image+iw*(selpos.y+ky), sel_buffer+sw*ky+xstart, (xend-xstart)<<2);
			}
		}
	}
}
void			selection_move_mouse(int prev_mx, int prev_my, int mx, int my)
{
	Point m, prev_m;
	screen2image(mx, my, m.x, m.y);
	screen2image(prev_mx, prev_my, prev_m.x, prev_m.y);
	Point d=m-prev_m;
	selection_moved|=d.x||d.y;
	selpos+=d;
	sel_start+=d, sel_end+=d, sel_s1+=d, sel_s2+=d;
}
void			selection_selectall()
{
	currentmode=M_RECT_SELECTION;
	selection_deselect();
	sel_start.set(0, 0), sel_end.set(iw, ih);
	selection_assign();
	Point p1, p2;
	selection_sortNbound(sel_start, sel_end, p1, p2);
	selection_select(p1, p2);
}

void			invertcolor(int *buffer, int bw, int bh, bool selection=false)
{
	if(selection&&selection_free)
	{
		for(int ky=0;ky<bh;++ky)
		{
			for(int kx=0;kx<bw;++kx)
			{
				int idx=bw*ky+kx;
				if(sel_mask[idx])
				{
					auto &c=buffer[idx];
					c=c&0xFF000000|~c&0x00FFFFFF;
				}
			}
		}
	}
	else
	{
		for(int ky=0;ky<bh;++ky)
		{
			for(int kx=0;kx<bw;++kx)
			{
				auto &c=buffer[bw*ky+kx];
				c=c&0xFF000000|~c&0x00FFFFFF;
			}
		}
	}
}
//void			setpixel_uu(int *buffer, int x, int y, int color)//undo, unchecked
//{
//	buffer[iw*y+x]=color;
//}
//void			setpixel_ou(int *buffer, int x, int y, int color)//ordinary, unchecked
//{
//	buffer[iw*y+x]=color;
//}
void			draw_line(int *buffer, int x1, int y1, int x2, int y2, int color, bool invert_color, bool *imask=nullptr)
{
	if(x1<0&&x2<0||x1>=iw&&x2>=iw||y1<0&&y2<0||y1>=ih&&y2>=ih)
		return;
	double dx=x2-x1, dy=y2-y1, xa, ya, xb, yb;
	if(!dx&&!dy)
	{
		int ix1=(int)x1, iy1=(int)y1;
		if(ix1>=0&&ix1<iw&&iy1>=0&&iy1<ih)//TODO: history struct
		{
			int idx=iw*y1+ix1;
			auto &c=buffer[idx];
			if(invert_color)
			{
				auto &m=imask[idx];
				if(!m)
				{
					m=true;
					c=c&0xFF000000|~c&0x00FFFFFF;
				}
			}
			else
				c=color;
		}
	}
	else if(abs(dx)>abs(dy))//horizontal
	{
		if(x1<x2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		int xstart=(int)maximum(xa, 0.), xend=(int)minimum(xb, (double)iw-1);
		double r=(yb-ya)/(xb-xa);
		for(double x=xstart;x<=xend;++x)
		{
			double y=ya+(x-xa)*r;
			if(y>=0&&y<ih)
			{
				int ix=(int)round(x), iy=(int)round(y);
				if(icheck(ix, iy))//
					continue;//
				int idx=iw*iy+ix;
				auto &c=buffer[idx];
				if(invert_color)
				{
					auto &m=imask[idx];
					if(!m)
					{
						m=true;
						c=c&0xFF000000|~c&0x00FFFFFF;
					}
				}
				else
					c=color;
			}
		}
	}
	else//vertical
	{
		if(y1<y2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		int ystart=(int)maximum(ya, 0.), yend=(int)minimum(yb, (double)ih-1);
		double r=(xb-xa)/(yb-ya);
		for(double y=ystart;y<=yend;++y)
		{
			double x=xa+(y-ya)*r;
			if(x>=0&&x<iw)
			{
				int ix=(int)round(x), iy=(int)round(y);
				if(icheck(ix, iy))//
					continue;//
				int idx=iw*iy+ix;
				auto &c=buffer[idx];
				if(invert_color)
				{
					auto &m=imask[idx];
					if(!m)
					{
						m=true;
						c=c&0xFF000000|~c&0x00FFFFFF;
					}
				}
				else
					c=color;
			}
		}
	}
}
struct			Range
{
	int i, f;
	Range():i(iw), f(0){}
	void assign(int x)
	{
		if(x>=0&&i>x)
			i=x;
		if(x<iw&&f<x+1)
			f=x+1;
	}
};
void			register_line(std::vector<Range> &bounds, int x1, int y1, int x2, int y2)
{
	if(x1<0&&x2<0||x1>=iw&&x2>=iw||y1<0&&y2<0||y1>=ih&&y2>=ih)
		return;
	double dx=x2-x1, dy=y2-y1, xa, ya, xb, yb;
	if(!dx&&!dy)
	{
		int ix1=(int)x1, iy1=(int)y1;
		if(ix1>=0&&ix1<iw&&iy1>=0&&iy1<ih)
			bounds[y1].assign(x1);
	}
	else if(abs(dx)>abs(dy))//horizontal
	{
		if(x1<x2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		int xstart=(int)maximum(xa, 0.), xend=(int)minimum(xb, (double)iw-1);
		double r=(yb-ya)/(xb-xa);
		for(double x=xstart;x<=xend;++x)
		{
			double y=ya+(x-xa)*r;
			if(y>=0&&y<ih)
			{
				int ix=(int)round(x), iy=(int)round(y);
				if(icheck(ix, iy))//
					continue;//
				bounds[iy].assign(ix);
			}
		}
	}
	else//vertical
	{
		if(y1<y2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		int ystart=(int)maximum(ya, 0.), yend=(int)minimum(yb, (double)ih-1);
		double r=(xb-xa)/(yb-ya);
		for(double y=ystart;y<=yend;++y)
		{
			double x=xa+(y-ya)*r;
			if(x>=0&&x<iw)
			{
				int ix=(int)round(x), iy=(int)round(y);
				if(icheck(ix, iy))//
					continue;//
				bounds[iy].assign(ix);
			}
		}
	}
}
//typedef			void (*draw_line_fn)(int *buffer, double x1, double y1, double x2, double y2, int color);
void			draw_line_brush(int *buffer, int brush, int x1, int y1, int x2, int y2, int color, bool invert_color=false, bool *imask=nullptr)//invert_color ignores color argument & uses imask which has same dimensions as buffer
{
	const int nbrushes=sizeof resources::brushes/sizeof resources::Brush;
	const resources::Brush *b=resources::brushes+(brush>0&&brush<nbrushes)*brush;
	if(b->bounds)//robust
	{
		if(brush<BRUSH_LARGE_RIGHT45)
		{
			for(int ky=0;ky<b->ysize;++ky)
				for(int kx=b->bounds[ky<<1], kxEnd=b->bounds[ky<<1|1];kx<=kxEnd;++kx)
					draw_line(buffer, x1+kx, y1+b->yoffset+ky, x2+kx, y2+b->yoffset+ky, color, invert_color, imask);
		}
		else//'line' brushes
		{
			std::vector<Range> bounds(ih);
			Point
				p1L={x1+b->bounds[0], y1+b->yoffset},
				p1R={x1+b->bounds[(b->ysize-1)<<1], y1+b->yoffset+b->ysize-1},
				p2L={x2+b->bounds[0], y2+b->yoffset},
				p2R={x2+b->bounds[(b->ysize-1)<<1], y2+b->yoffset+b->ysize-1};
			register_line(bounds, p1L.x, p1L.y, p2L.x, p2L.y);
			register_line(bounds, p2L.x, p2L.y, p2R.x, p2R.y);
			register_line(bounds, p2R.x, p2R.y, p1R.x, p1R.y);
			register_line(bounds, p1R.x, p1R.y, p1L.x, p1L.y);
			if(invert_color)
			{
				for(int ky=0;ky<ih;++ky)
				{
					for(int kx=bounds[ky].i, kxEnd=bounds[ky].f;kx<kxEnd;++kx)
					{
						int idx=iw*ky+kx;
						bool &m=imask[idx];
						if(!m)
						{
							m=true;
							auto &c=buffer[idx];
							c=c&0xFF000000|~c&0x00FFFFFF;
						}
					}
				}
			}
			else
			{
				for(int ky=0;ky<ih;++ky)
					for(int kx=bounds[ky].i, kxEnd=bounds[ky].f;kx<kxEnd;++kx)
						buffer[iw*ky+kx]=color;
			}
		}
	}
}
//void			draw_line_brush_d(int *buffer, int x1, int y1, int x2, int y2, int color)
//{
//}
void			draw_line_mouse(int *buffer, int mx1, int my1, int mx2, int my2, int color)
{
	int x1, y1, x2, y2;
	screen2image(mx1, my1, x1, y1);
	screen2image(mx2, my2, x2, y2);
	int x2a, y2a;
	if(kb[VK_SHIFT])
	{
		int dx=x2-x1, dy=y2-y1;
		double angle=todeg*atan2(dy, dx);
		angle+=360&-(angle<0);
		GUIPrint(ghMemDC, w>>1, h>>1, "angle=%lf", angle);//
		if(angle<22.5||angle>337.5)//0 degrees
			x2a=x2, y2a=y1;
		else if(angle<67.5)//45 degrees
		{
			int av=(dx+dy)>>1;
			x2a=x1+av, y2a=y1+av;
		}
		else if(angle<112.5)//90 degrees
			x2a=x1, y2a=y2;
		else if(angle<157.5)//135 degrees
		{
			int av=(-dx+dy)>>1;
			x2a=x1-av, y2a=y1+av;
		}
		else if(angle<202.5)//180 degrees
			x2a=x2, y2a=y1;
		else if(angle<247.5)//225 degrees
		{
			int av=(dx+dy)>>1;
			x2a=x1+av, y2a=y1+av;
		}
		else if(angle<292.5)//270 degrees
			x2a=x1, y2a=y2;
		else//315 degrees
		{
			int av=(dx-dy)>>1;
			x2a=x1+av, y2a=y1-av;
		}
	}
	else
		x2a=x2, y2a=y2;
	draw_line_brush(buffer, linewidth, x1, y1, x2a, y2a, color);
}
void			draw_line_d(int *buffer, double x1, double y1, double x2, double y2, int color)
{
	if(x1<0&&x2<0||x1>=iw&&x2>=iw||y1<0&&y2<0||y1>=ih&&y2>=ih)
		return;
	double dx=x2-x1, dy=y2-y1, xa, ya, xb, yb;
	if(!dx&&!dy)
	{
		if(x1>=0&&x1<iw&&y1>=0&&y1<ih)
			buffer[iw*int(y1)+int(x1)]=color;//TODO: history struct
	}
	else if(abs(dx)>abs(dy))//horizontal
	{
		if(x1<x2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		int xstart=(int)maximum(xa, 0.), xend=(int)minimum(xb, (double)iw-1);
		double r=(yb-ya)/(xb-xa);
		for(double x=xstart;x<=xend;++x)
		{
			double y=ya+(x-xa)*r;
			if(y>=0&&y<ih)
			{
				//if(y<0||y>=ih||x<0||x>=iw)
				//	int LOL_1=0;
				buffer[iw*(int)y+(int)x]=color;//TODO: alpha blend
			}
		}
	}
	else//vertical
	{
		if(y1<y2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		int ystart=(int)maximum(ya, 0.), yend=(int)minimum(yb, (double)ih-1);
		double r=(xb-xa)/(yb-ya);
		for(double y=ystart;y<=yend;++y)
		{
			double x=xa+(y-ya)*r;
			if(x>=0&&x<iw)
			{
				//if(y<0||y>=ih||x<0||x>=iw)
				//	int LOL_1=0;
				buffer[iw*(int)y+(int)x]=color;
			}
		}
	}
}
void			draw_line_d_mouse(int *buffer, int mx1, int my1, int mx2, int my2, int color)
{
	double x1, y1, x2, y2;
	screen2image(mx1, my1, x1, y1);
	screen2image(mx2, my2, x2, y2);
	draw_line_d(buffer, x1, y1, x2, y2, color);
}

void			draw_line_brush_mouse(int *buffer, int brush, int mx1, int my1, int mx2, int my2, int color)
{
//	double x1, y1, x2, y2;
	int x1, y1, x2, y2;
	screen2image(mx1, my1, x1, y1);
	screen2image(mx2, my2, x2, y2);
	draw_line_brush(buffer, brush, x1, y1, x2, y2, color);
}

std::vector<Point> bezier;
void			curve_add_mouse(int mx, int my)
{
	Point i;
	screen2image(mx, my, i.x, i.y);
	switch(bezier.size())
	{
	case 0:
	case 1:
		bezier.push_back(i);
		break;
	case 2:
		bezier.insert(bezier.begin()+1, i);
		break;
	case 3:
		bezier.insert(bezier.begin()+2, i);
		break;
	}
}
void			curve_update_mouse(int mx, int my)
{
	Point i;
	screen2image(mx, my, i.x, i.y);
	const int access[]={-1, 0, 1, 1, 2};
	int idx=access[(bezier.size()<5)*bezier.size()];
	if(idx!=-1)
		bezier[idx].set(i.x, i.y);
}
double			distance(Point const &a, Point const &b){int dx=b.x-a.x, dy=b.y-a.y; return sqrt(dx*dx+dy*dy);}
void			curve_draw(int *buffer, int color)
{
	double outerpath=0;
	for(int k=1, nv=bezier.size();k<nv;++k)
		outerpath+=distance(bezier[k-1], bezier[k]);
	outerpath=ceil(outerpath);
	switch(bezier.size())
	{
	case 2:
		draw_line_brush(buffer, linewidth, bezier[0].x, bezier[0].y, bezier[1].x, bezier[1].y, color);
		break;
	case 3:
		{
			Point p1=bezier[0];
			for(int k=0;k<=outerpath;++k)
			{
				double t=k/outerpath, tc=1-t;
				Point p2=(tc*tc)*bezier[0]+(2*tc*t)*bezier[1]+(t*t)*bezier[2];
				draw_line_brush(buffer, linewidth, p1.x, p1.y, p2.x, p2.y, color);
				p1=p2;
			}
		}
		break;
	case 4:
		{
			Point p1=bezier[0];
			for(int k=0;k<=outerpath;++k)
			{
				double t=k/outerpath, tc=1-t;
				Point p2=(tc*tc*tc)*bezier[0]+(3*tc*tc*t)*bezier[1]+(3*tc*t*t)*bezier[2]+(t*t*t)*bezier[3];
				draw_line_brush(buffer, linewidth, p1.x, p1.y, p2.x, p2.y, color);
				p1=p2;
			}
		}
		break;
	}
//	bezier.clear();
}

void			fill(int *buffer, int x0, int y0, int color)
{
	if(check(x0, y0))
		return;
	typedef std::pair<int, int> Point;
	std::queue<Point> q;
	q.push(Point(x0, y0));
	int oldcolor=buffer[iw*y0+x0];
	if(color==oldcolor)
		return;
	buffer[iw*y0+x0]=color;
	while(q.size())
	{
		auto f=q.front();
		int x=f.first, y=f.second;
		if(icheck(x, y))
			return;
		q.pop();
		if(x+1<iw&&buffer[iw*y+x+1]==oldcolor)
			buffer[iw*y+x+1]=color, q.push(Point(x+1, y));
		if(x-1>=0&&buffer[iw*y+x-1]==oldcolor)
			buffer[iw*y+x-1]=color, q.push(Point(x-1, y));
		if(y+1<ih&&buffer[iw*(y+1)+x]==oldcolor)
			buffer[iw*(y+1)+x]=color, q.push(Point(x, y+1));
		if(y-1>=0&&buffer[iw*(y-1)+x]==oldcolor)
			buffer[iw*(y-1)+x]=color, q.push(Point(x, y-1));
	}
}
void			fill_mouse(int *buffer, int mx0, int my0, int color)
{
	double x0, y0;
	screen2image(mx0, my0, x0, y0);
	fill(buffer, (int)x0, (int)y0, color);
}

void			airbrush(int *buffer, int x0, int y0, int color)
{
	for(int k=0;k<airbrush_cpt;++k)
	{
		int x=0, y=0;
		switch(airbrush_size)
		{
		case S_SMALL:
			do
				x=rand()%9-4, y=rand()%9-4;
			while(x*x+y*y>16);
			break;
		case S_MEDIUM:
			do
				x=rand()%17-8, y=rand()%17-8;
			while(x*x+y*y>64);
			break;
		case S_LARGE:
			do
				x=rand()%25-12, y=rand()%25-12;
			while(x*x+y*y>144);
			break;
		}
		x+=x0, y+=y0;
		if(!icheck(x, y))
			buffer[iw*y+x]=color;
	}
}
void			airbrush_mouse(int *buffer, int mx0, int my0, int color)
{
	double x0, y0;
	screen2image(mx0, my0, x0, y0);
	airbrush(buffer, (int)x0, (int)y0, color);
}

int				pick_color_mouse(int *buffer, int x0, int y0)
{
	int ix, iy;
	screen2image(mx, my, ix, iy);
	if(!icheck(ix, iy))
		return buffer[iw*iy+ix];
	return 0xFFFFFF;
}
//void			blend(int *dst, int *src)
//{
//	for(int ky=0;ky<
//}
void			draw_h_line(int *buffer, int x1, int x2, int y, int color)//x2 exclusive
{
	if(!ichecky(y))
	{
		x1=maximum(0, x1), x2=minimum(x2, iw);
		for(int kx=x1;kx<x2;++kx)
			buffer[iw*y+kx]=color;
	}
}
void			draw_v_line(int *buffer, int x, int y1, int y2, int color)//y2 exclusive
{
	if(!icheckx(x))
	{
		y1=maximum(0, y1), y2=minimum(y2, ih);
		for(int ky=y1;ky<y2;++ky)
			buffer[iw*ky+x]=color;
	}
}
void			draw_rectangle(int *buffer, int x1, int x2, int y1, int y2, int color, int color2)
{
	if(x2<x1)
		std::swap(x1, x2);
	if(y2<y1)
		std::swap(y1, y2);
	if(rectangle_type==ST_FULL||rectangle_type==ST_FILL)//fill shape
	{
		int color3=rectangle_type==ST_FILL?color:color2;
		int x1b=maximum(x1+1, 0), x2b=minimum(x2-1, iw), y1b=maximum(y1+1, 0), y2b=minimum(y2-1, ih);
		for(int ky=y1b;ky<y2b;++ky)
			for(int kx=x1b;kx<x2b;++kx)
				buffer[iw*ky+kx]=color3;
	}
	if(rectangle_type==ST_HOLLOW||rectangle_type==ST_FULL)//draw shape outline
	{
		for(int t=0;t<linewidth;++t, ++x1, --x2, ++y1, --y2)
		{
			draw_h_line(buffer, x1, x2, y1, color);
			draw_h_line(buffer, x1, x2, y2-1, color);
			draw_v_line(buffer, x1, y1, y2, color);
			draw_v_line(buffer, x2-1, y1, y2, color);
		}
	}
}
void			draw_rectangle_mouse(int *buffer, int mx1, int mx2, int my1, int my2, int color, int color2)
{
	int x1, y1, x2, y2;
	screen2image(mx1, my1, x1, y1);
	screen2image(mx2, my2, x2, y2);
	int x2a, y2a;
	if(kb[VK_SHIFT])
	{
		int dx=x2-x1, dy=y2-y1, m=minimum(abs(dx), abs(dy));
		if(dx>0)
			x2a=x1+m;
		else
			x2a=x1-m;
		if(dy>0)
			y2a=y1+m;
		else
			y2a=y1-m;
	}
	else
		x2a=x2, y2a=y2;
	draw_rectangle(buffer, x1, x2a, y1, y2a, color, color2);
}

std::vector<Point> polygon;//image coordinates
bool			polygon_leftbutton=true;
void			polygon_add_mouse(int mx, int my)
{
	int ix, iy;
	screen2image(mx, my, ix, iy);
	polygon.push_back(Point(ix, iy));
}
void			polygon_add_mouse(int start_mx, int start_my, int mx, int my)
{
	if(!polygon.size())
		polygon_add_mouse(start_mx, start_my);
	polygon_add_mouse(mx, my);
}
char			lineXrow(Point const &p1, Point const &p2, int y, double &x)
{
	if(p1.y==p2.y)
	{
		if(y==p1.y)
		{
			x=0x80000000;
			return 'G';//grasing
		}
		return false;
	}
	if(y==p1.y)
	{
		x=p1.x;
		return '1';
	}
	if(y==p2.y)
	{
		x=p2.x;
		return '2';
	}
	x=p1.x+(y-p1.y)*double(p2.x-p1.x)/(p2.y-p1.y);//x1+(y-y1)*(x2-x1)/(y2-y1)
	return (x>=p1.x)!=(x>p2.x)&&(y>=p1.y)!=(y>p2.y);
	//x=p1.x+(y-p1.y)*double(p2.x-p1.x)/(p2.y-p1.y);//x1+(y-y1)*(x2-x1)/(y2-y1)
	//if(x==p1.x&&y==p1.y)
	//	return '1';//intersects at p1
	//if(x==p2.x&&y==p2.y)
	//	return '2';//intersects at p2
	//return (x>=p1.x)!=(x>p2.x);
}
int				rowXpolygon_vertexcase(Point const &p_1, Point const &p0, Point const &p1)
{
	return (p_1.y>p0.y)!=(p1.y>p0.y);
}
void			polygon_bounds(std::vector<Point> const &polygon, int coord_idx, int &start, int &end, int clipstart, int clipend)
{
	int npoints=polygon.size();
	if(!npoints)
	{
		start=end=0;
		return;
	}
	int v=(&polygon[0].x)[coord_idx];//find [x/y]start & [x/y]end
	start=v, end=v;
	for(int k=1;k<npoints;++k)
	{
		v=(&polygon[k].x)[coord_idx];
		if(start>v)
			start=v;
		else if(end<v)
			end=v;
	}
	++end;
	if(start<clipstart)
		start=clipstart;
	if(end>clipend)
		end=clipend;
}
void			polygon_rowbounds(std::vector<Point> const &polygon, int ky, std::vector<double> &bounds)
{
//polygon_draw_startover:
	int npoints=polygon.size();
	for(int ks=0;ks<npoints;++ks)//for each side
	{
		double x;
		auto &p1=polygon[ks], &p2=polygon[(ks+1)%npoints];
		char result=lineXrow(p1, p2, ky, x);
		switch(result)
		{
		case 1:
			bounds.push_back(x);
			break;
		case '1'://intersects at p1
			if(rowXpolygon_vertexcase(polygon[(ks+npoints-1)%npoints], p1, p2))
				bounds.push_back(x);
			break;
		case '2'://intersects at p2
			break;
		case 'G'://grasing
			{
				int x1=p1.x, x2=p2.x;
				if(x1>x2)
					std::swap(x1, x2);
				bounds.push_back(x1), bounds.push_back(x2);
			}
			break;
		}
	}
	//if(bounds.size()&1)//DEBUG
	//{
	//	bounds.clear();
	//	goto polygon_draw_startover;
	//}
	std::sort(bounds.begin(), bounds.end());
}
void			polygon_draw(int *buffer, int color_line, int color_fill)
{
	//{//DEBUG
	//	polygon.clear();
	//	static const int coords[]=
	//	{
	//		175,	83 ,
	//		 86,	83 ,
	//		 86,	188,
	//		235,	188,
	//		235,	43 ,
	//	};
	//	const int nval=sizeof coords>>2;
	//	for(int k=0;k+1<nval;k+=2)
	//		polygon.push_back(Point(coords[k], coords[k+1]));
	//}//DEBUG
	//polygon_to_clipboard((int*)&polygon[0], polygon.size());//
	int npoints=polygon.size();
	if(npoints>=3)
	{
		hist_premodify(image, iw, ih);
		if(polygon_type==ST_FULL||polygon_type==ST_FILL)//fill polygon
		{
			int color3=polygon_type==ST_FILL?color_line:color_fill;
			int ystart, yend;
			polygon_bounds(polygon, 1, ystart, yend, 0, ih);
			std::vector<double> bounds;
			for(int ky=ystart;ky<yend;++ky)//for each row in polygon
			{
				polygon_rowbounds(polygon, ky, bounds);
				for(int kb=0, nbounds=bounds.size();kb+1<nbounds;kb+=2)
				{
					int kx=(int)round(bounds[kb]), kxEnd=(int)round(bounds[kb+1]);
					if(kx<0)
						kx=0;
					if(kxEnd>iw)
						kxEnd=iw;
					for(;kx<kxEnd;++kx)
						buffer[iw*ky+kx]=color3;
				}
				bounds.clear();
			}
		}
		if(polygon_type==ST_HOLLOW||polygon_type==ST_FULL)//draw polygon outline
		{
			for(int ks=0;ks<npoints;++ks)//for each side
			{
				auto &p1=polygon[ks], &p2=polygon[(ks+1)%npoints];
				draw_line_brush(buffer, linewidth, p1.x, p1.y, p2.x, p2.y, color_line);
			}
		}
	}
	polygon.clear();
}

std::vector<Point> freesel;//free form selection vertices
void			freesel_add_mouse(int mx, int my)
{
	Point i;
	screen2image(mx, my, i.x, i.y);
	freesel.push_back(i);
}
void			freesel_select()
{
	if(freesel.size()>=3)
	{
		int xstart, xend, ystart, yend;
		polygon_bounds(freesel, 0, xstart, xend, 0, iw);//get selection bounds
		polygon_bounds(freesel, 1, ystart, yend, 0, ih);

		sel_start.set(xstart, ystart), sel_end.set(xend, yend);//set selection parameters
		image2screen(xstart, ystart, sel_s1.x, sel_s1.y);
		image2screen(xend, yend, sel_s2.x, sel_s2.y);
		selpos.set(xstart, ystart);

		sw=xend-xstart, sh=yend-ystart;
		int sb_size=sw*sh;
		sel_buffer=(int*)realloc(sel_buffer, sb_size<<2);//allocate & initialize buffers
		sel_mask=(int*)realloc(sel_mask, sb_size<<2);
		memset(sel_buffer, 0, sb_size<<2);
		memset(sel_mask, 0, sb_size<<2);

		std::vector<double> bounds;
		for(int ky=ystart;ky<yend;++ky)
		{
			polygon_rowbounds(freesel, ky, bounds);
			for(int kb=0, nbounds=bounds.size();kb+1<nbounds;kb+=2)
			{
				int kx=(int)round(bounds[kb]), kxEnd=(int)round(bounds[kb+1]);
				if(kx<0)
					kx=0;
				if(kxEnd>iw)
					kxEnd=iw;
				for(;kx<kxEnd;++kx)
				{
					auto &src=image[iw*ky+kx];
					int sel_idx=sw*(ky-ystart)+kx-xstart;
					sel_buffer[sel_idx]=src;
					sel_mask[sel_idx]=-1;
					src=secondarycolor;
				}
			}
			bounds.clear();
		}
		selection_free=true;
	}
	freesel.clear();
}

inline void		setpixel(int *buffer, int x, int y, int color)
{
	if(!icheck(x, y))
		buffer[iw*y+x]=color;
}
inline void		ellipsePlotPoints(int *buffer, int xCenter, int yCenter, int x, int y, int color)
{
    setpixel(buffer, xCenter+x, yCenter+y, color);
    setpixel(buffer, xCenter-x, yCenter+y, color);
    setpixel(buffer, xCenter+x, yCenter-y, color);
    setpixel(buffer, xCenter-x, yCenter-y, color);
}
void			draw_ellipse_hollow(int *buffer, int xCenter, int yCenter, int Rx, int Ry, int color)//midpoint ellipse algorithm https://www.programming-techniques.com/2012/01/drawing-ellipse-with-mid-point-ellipse.html
{
	int Rx2=Rx*Rx, Ry2=Ry*Ry;
	int twoRx2=Rx2<<1, twoRy2=Ry2<<1;
	int p;
	int x=0, y=Ry;
	int px=0, py=twoRx2*y;
	ellipsePlotPoints(buffer, xCenter, yCenter, x, y, color);
	//For Region 1
	p=(int)round(Ry2-Rx2*Ry+0.25*Rx2);
//	p=Ry2-Rx2*Ry+(Rx2>>2);
	while(px<py)
	{
		++x;
		px+=twoRy2;
		if(p<0)
			p+=Ry2+px;
		else
		{
			--y;
			py-=twoRx2;
			p+=Ry2+px-py;
		}
		ellipsePlotPoints(buffer, xCenter, yCenter, x, y, color);
	}
	//For Region 2
	p=(int)round(Ry2*(x+0.5)*(x+0.5)+Rx2*(y-1)*(y-1)-Rx2*Ry2);
	while(y>0)
	{
		--y;
		py-=twoRx2;
		if(p>0)
			p+=Rx2-py;
		else
		{
			x++;
			px+=twoRy2;
			p+=Rx2-py+px;
		}
		ellipsePlotPoints(buffer, xCenter, yCenter, x, y, color);
	}
}
inline int		ellipse_fn(int x0, int y0, int rx, int ry, int y)
{
	double temp=double(y-y0)/ry;
	return int(rx*sqrt(1-temp*temp));
}
inline int		ellipse_fn2(int rx, int ry, int y)
{
	double temp=double(y)/ry;
	return int(rx*sqrt(1-temp*temp));
}
void			draw_ellipse(int *buffer, int x1, int x2, int y1, int y2, int color, int color2)
{
	if(x2<x1)
		std::swap(x1, x2);
	if(y2<y1)
		std::swap(y1, y2);
	int x0=(x1+x2)>>1, y0=(y1+y2)>>1, rx=(x2-x1)>>1, ry=(y2-y1)>>1;
	if(ellipse_type==ST_FULL||ellipse_type==ST_FILL)//solid ellipse
	{
		int color3=ellipse_type==ST_FILL?color:color2;
		int y1b=maximum(y1+1, 0), y2b=minimum(y2-1, ih);
		for(int ky=y1b;ky<y2b;++ky)
		{
			int reach=ellipse_fn(x0, y0, rx, ry, ky);
			//double temp=double(ky-y0)/ry;
			//int reach=int(rx*sqrt(1-temp*temp));
			int x1b=maximum(x0-reach, 0), x2b=minimum(x0+reach, iw);
			for(int kx=x1b;kx<x2b;++kx)
				buffer[iw*ky+kx]=color3;
		}
	}
	if(ellipse_type==ST_HOLLOW||ellipse_type==ST_FULL)//hollow ellipse
	{
		if(linewidth==1)
			draw_ellipse_hollow(buffer, x0, y0, rx, ry, color);
		else// if(rx&&ry)
		{
			int ky=ry, ysw=ry-linewidth;
			for(;ky>=ysw;--ky)
			{
				int reach=ellipse_fn2(rx, ry, ky);
				for(int kx=0;kx<reach;++kx)
					ellipsePlotPoints(buffer, x0, y0, kx, ky, color);
			}
			for(;ky>=0;--ky)
			{
				int reach1=ellipse_fn2(rx-linewidth, ry-linewidth, ky), reach2=ellipse_fn2(rx, ry, ky);
				for(int kx=reach1;kx<reach2;++kx)
					ellipsePlotPoints(buffer, x0, y0, kx, ky, color);
			}
		}
		//for(int t=0;t<linewidth;++t, --rx, --ry)
		//	draw_ellipse_hollow(buffer, x0, y0, rx, ry, color);
	}
}
void			draw_ellipse_mouse(int *buffer, int mx1, int mx2, int my1, int my2, int color, int color2)
{
	int x1, y1, x2, y2;
	screen2image(mx1, my1, x1, y1);
	screen2image(mx2, my2, x2, y2);
	int x2a, y2a;
	if(kb[VK_SHIFT])
	{
		int dx=x2-x1, dy=y2-y1, m=minimum(abs(dx), abs(dy));
		if(dx>0)
			x2a=x1+m;
		else
			x2a=x1-m;
		if(dy>0)
			y2a=y1+m;
		else
			y2a=y1-m;
	}
	else
		x2a=x2, y2a=y2;
	draw_ellipse(buffer, x1, x2a, y1, y2a, color, color2);
}

void			draw_roundrect(int *buffer, int x1, int x2, int y1, int y2, int color, int color2)
{
//	x1=0, x2=50, y1=0, y2=50;//DEBUG
	if(x2<x1)
		std::swap(x1, x2);
	if(y2<y1)
		std::swap(y1, y2);

	const int maxdiameter=20;//17
	const int curves[]=
	{
		0, 0, 0, 0, 0, 0, 0, 0,	//0, 1, 2
		1, 0, 0, 0, 0, 0, 0, 0,	//3, 4, 5
		2, 1, 0, 0, 0, 0, 0, 0,	//6, 7
		3, 1, 1, 0, 0, 0, 0, 0,	//8, 9
		4, 2, 1, 1, 0, 0, 0, 0,	//10, 11
		5, 2, 1, 1, 1, 0, 0, 0,	//12, 13
		5, 3, 2, 1, 1, 0, 0, 0,	//14, 15
		5, 4, 2, 2, 1, 0, 0, 0,	//16	X?
		6, 4, 3, 2, 1, 1, 0, 0,	//17
		7, 4, 3, 2, 1, 1, 1, 0,	//18, 19
		8, 5, 4, 3, 2, 1, 1, 1,	//20
	};
	const int curve_sizes[]={0, 1, 2, 3, 4, 5, 5, 5, 6, 7, 8};
	const int rc_fn[]=//radius-curve function
	{
		0, 0, 0,//0, 1, 2
		1, 1, 1,//3, 4, 5
		2, 2,	//6, 7
		3, 3,	//8, 9
		4, 4,	//10, 11
		5, 5,	//12, 13
		6, 6,	//14, 15
		7,		//16
		8,		//17
		9, 9,	//18, 19
		10,		//20
	};
	int dx=x2-x1, dy=y2-y1,
		diameter=dx<maxdiameter||dy<maxdiameter?minimum(dx, dy):maxdiameter,//header+footer size & curve diameter
	//	radius=diameter>>1, body=dy-diameter;
		curve_idx=rc_fn[diameter];
	const int *curve=curves+(curve_idx<<3);
	int curve_size=curve_sizes[curve_idx];
	//	body=dy-(curve_size<<1);

	//const int ci_size=maxdiameter>>1;
	//int c_info[ci_size];//edge info
	//int dx=x2-x1, dy=y2-y1,
	//	radius=0,//header/footer size & curve radius
	//	body=0;
	//if(dx<maxdiameter||dy<maxdiameter)//small
	//	radius=minimum(dx, dy), body=dy-radius, radius>>=1;
	//else//largest
	//	radius=ci_size, body=dy-maxdiameter;
	//for(int k=0, x=-radius, r2=radius*radius;k<radius;++k, ++x)
	//	c_info[k]=(int)round(radius-sqrt(r2-x*x));
	//for(int k=radius;k<ci_size;++k)
	//	c_info[k]=0;
	//for(int k=0;k<ci_size;++k)//cut corners
	//	c_info[k]=ci_size-1-k;
	int ya=maximum(y1, 0), yb=minimum(y2, ih);
	if(roundrect_type==ST_FULL||roundrect_type==ST_FILL)//fill shape
	{
		int c3=roundrect_type==ST_FILL?color:color2;
		for(int ky=ya;ky<yb;++ky)
		{
			int xa, xb;
			if(ky<y1+curve_size)//[0, curve_size[
				xa=x1+curve[ky-y1], xb=x2-curve[ky-y1];
			else if(ky<y2-curve_size)//[curve_size, y2-curve_size[
				xa=x1, xb=x2;
			else//[y2-curve_size, y2[
				xa=x1+curve[y2-1-ky], xb=x2-curve[y2-1-ky];
			if(xa<0)
				xa=0;
			if(xb>iw)
				xb=iw;
			for(int kx=xa;kx<xb;++kx)
				buffer[iw*ky+kx]=c3;
		}
	}
	if(roundrect_type==ST_HOLLOW||roundrect_type==ST_FULL)//draw shape outline
	{
		//int curve_idx2=curve_idx-linewidth;
		//if(curve_idx2<0)
		//	curve_idx2=0;
		//const int *incurve=curves+(curve_idx2<<3);
		//int curve_size2=curve_sizes[curve_idx2];
		int diameter2=diameter-linewidth;
		if(diameter2<0)
			diameter2=0;
		const int *incurve=curves+(rc_fn[diameter2]<<3);
		int curve_size2=curve_sizes[rc_fn[diameter2]];
		//int bounds[]={y1, y1+linewidth, y1+curve_size, y2-curve_size, y2-linewidth, y2};
		//std::sort(bounds+1, bounds+5);
		if(curve_size>=linewidth)//large shape
		{
			for(int ky=ya;ky<yb;++ky)
			{
				int xa, xb, xc, xd;
				if(ky<y1+linewidth)					//header1: outer -> outer
					xa=x1+curve[ky-y1], xb=x2-curve[ky-y1], xc=iw, xd=iw;
				else if(ky<y1+curve_size)			//header2: outer -> inner, inner -> outer
					xa=x1+curve[ky-y1], xb=x1+linewidth+incurve[ky-y1-linewidth], xc=x2-(linewidth+incurve[ky-y1-linewidth]), xd=x2-curve[ky-y1];
				else if(ky<y1+linewidth+curve_size2)//header3: x1 -> inner, inner -> x2
					xa=x1, xb=x1+linewidth+incurve[ky-y1-linewidth], xc=x2-(linewidth+incurve[ky-y1-linewidth]), xd=x2;//
				else if(ky<y2-linewidth-curve_size2)//body: x1 -> x1+linewidth, x2-linewidth -> x2
					xa=x1, xb=x1+linewidth, xc=x2-linewidth, xd=x2;
				else if(ky<y2-curve_size)			//footer3: x1 -> inner, inner -> x2
					xa=x1, xb=x1+linewidth+incurve[y2-1-ky-linewidth], xc=x2-(linewidth+incurve[y2-1-ky-linewidth]), xd=x2;
				else if(ky<y2-linewidth)			//footer2: outer -> inner, inner -> outer
					xa=x1+curve[y2-1-ky], xb=x1+linewidth+incurve[y2-1-ky-linewidth], xc=x2-(linewidth+incurve[y2-1-ky-linewidth]), xd=x2-curve[y2-1-ky];
				else								//footer1: outer -> outer
					xa=x1+curve[y2-1-ky], xb=x2-curve[y2-1-ky], xc=iw, xd=iw;
				if(xb>xc)
					xb=xc=(xb+xc)>>1;
				if(xa<0)
					xa=0;
				if(xb>iw)
					xb=iw;
				if(xc<0)
					xc=0;
				if(xd>iw)
					xd=iw;
				for(int kx=xa;kx<xb;++kx)
					buffer[iw*ky+kx]=color;
				for(int kx=xc;kx<xd;++kx)
					buffer[iw*ky+kx]=color;
			}
		}
		else//outer curve size < linewidth, small/narrow shape
		{
			int bounds[]={y1+linewidth, y2-linewidth};
			if(bounds[0]>bounds[1])
				bounds[0]=bounds[1]=(bounds[0]+bounds[1])>>1;
			for(int ky=ya;ky<yb;++ky)
			{
				int xa, xb, xc, xd;
				if(ky<bounds[0])				//header1: outer -> outer
					xa=x1+curve[ky-y1], xb=x2-curve[ky-y1], xc=iw, xd=iw;
				else if(ky<bounds[1])			//body: x1 -> x1+linewidth, x2-linewidth -> x2
					xa=x1, xb=x1+linewidth, xc=x2-linewidth, xd=x2;
				else							//footer1: outer -> outer
					xa=x1+curve[y2-1-ky], xb=x2-curve[y2-1-ky], xc=iw, xd=iw;
				if(xb>xc)
					xb=xc=(xb+xc)>>1;
				if(xa<0)
					xa=0;
				if(xb>iw)
					xb=iw;
				if(xc<0)
					xc=0;
				if(xd>iw)
					xd=iw;
				for(int kx=xa;kx<xb;++kx)
					buffer[iw*ky+kx]=color;
				for(int kx=xc;kx<xd;++kx)
					buffer[iw*ky+kx]=color;
			}
		}
	}
}
void			draw_roundrect_mouse(int *buffer, int mx1, int mx2, int my1, int my2, int color, int color2)
{
	int x1, y1, x2, y2;
	screen2image(mx1, my1, x1, y1);
	screen2image(mx2, my2, x2, y2);
	int x2a, y2a;
	if(kb[VK_SHIFT])
	{
		int dx=x2-x1, dy=y2-y1, m=minimum(abs(dx), abs(dy));
		if(dx>0)
			x2a=x1+m;
		else
			x2a=x1-m;
		if(dy>0)
			y2a=y1+m;
		else
			y2a=y1-m;
	}
	else
		x2a=x2, y2a=y2;
	draw_roundrect(buffer, x1, x2a, y1, y2a, color, color2);
}

void			draw_icon(int *icon, int x, int y)
{
	for(int ky=0;ky<16;++ky)
	{
		for(int kx=0;kx<16;++kx)
		{
			int color=icon[ky<<4|kx];
			if(color!=0xFF00FF)
				rgb[w*(y+ky)+x+kx]=color;
		}
	}
}
void			draw_icon_monochrome(const int *ibuffer, int dx, int dy, int xpos, int ypos, int color)
{
	for(int ky=0;ky<dy;++ky)
		for(int kx=0;kx<dx;++kx)
			if(ibuffer[ky]>>(dx-1-kx)&1)
				rgb[w*(ypos+ky)+xpos+kx]=color;
}
void			draw_icon_monochrome_transposed(const int *ibuffer, int dx, int dy, int xpos, int ypos, int color)
{
	for(int kx=0;kx<dx;++kx)
		for(int ky=0;ky<dy;++ky)
			if(ibuffer[dy-1-ky]>>kx&1)
				rgb[w*(ypos+kx)+xpos+ky]=color;
}

void			draw_vscrollbar(int x, int sbwidth, int y1, int y2, int &winpos, int content_h, int vscroll_s0, int vscroll_my_start, double zoom, int &vscroll_start, int &vscroll_size, int dragtype)//vertical scrollbar
{
//	const int scrollbarwidth=17;
	const int color_barbk=0xF0F0F0, color_slider=0xCDCDCD, color_button=0xE5E5E5;
	rectangle(x, x+sbwidth, y1, y1+sbwidth, color_button);//up
	rectangle(x, x+sbwidth, y1+sbwidth, y2-sbwidth, color_barbk);//vertical
	rectangle(x, x+sbwidth, y2-sbwidth, y2, color_button);//down
	int win_size=y2-y1, scroll_size=win_size-(sbwidth<<1);
	if(drag==dragtype)
		scrollbar_scroll(winpos, content_h, win_size, scroll_size, vscroll_s0, vscroll_my_start, my, zoom, vscroll_start, vscroll_size);
	else
		scrollbar_slider(winpos, content_h, win_size, scroll_size, zoom, vscroll_start, vscroll_size);
	rectangle(x, x+sbwidth, y1+sbwidth+vscroll_start, y1+sbwidth+vscroll_start+vscroll_size, color_slider);//vertical slider
//	x2-=sbwidth;
}
void			draw_selection_rectangle(Point &start, Point &end, Point &s1, Point &s2, int x1, int x2, int y1, int y2, int padding)
{
//	if(abs(sel_start.x-sel_end.x)*abs(sel_start.y-sel_end.y))
	if(start!=end)//selection highlight rectangle
	{
		Point p1, p2;
		selection_sort(start, end, p1, p2);
		image2screen(p1.x, p1.y, s1.x, s1.y);
		image2screen(p2.x, p2.y, s2.x, s2.y);
		s1.x-=padding, s1.y-=padding;
		s2.x+=padding, s2.y+=padding;
		int xstart=maximum(x1, s1.x),		xend=minimum(s2.x+1, x2),
			xstart_w=maximum(x1, s1.x+1),	xend_w=minimum(s2.x, x2),
			ystart=maximum(y1, s1.y),		yend=minimum(s2.y+1, y2),
			ystart_w=maximum(y1, s1.y+1),	yend_w=minimum(s2.y, y2);
		const int white=0xFFFFFF, black=0x000000;
		if(s1.y>=y1)//top
			h_line(xstart_w, xend_w, s1.y+1, white);
		if(s2.y<y2)//bottom
			h_line(xstart_w, xend_w, s2.y-1, white);
		if(s1.x>=x1)//left
			v_line(s1.x+1, ystart_w, yend_w, white);
		if(s2.x<x2)//right
			v_line(s2.x-1, ystart_w, yend_w, white);

		if(s1.y>=y1)//top
			h_line(xstart, xend, s1.y, black);
		if(s2.y<y2)//bottom
			h_line(xstart, xend, s2.y, black);
		if(s1.x>=x1)//left
			v_line(s1.x, ystart, yend, black);
		if(s2.x<x2)//right
			v_line(s2.x, ystart, yend, black);
	}
}
void			movewindow_c(HWND hWnd, int x, int y, int w, int h, bool repaint)//MoveWindow takes client coordinates
{
	int success=MoveWindow(hWnd, x, y, w, h, repaint);
	if(!success)
		formatted_error(L"MoveWindow", __LINE__);
//	check(__LINE__);
}
void			movewindow(HWND hWnd, int x, int y, int w, int h, bool repaint)//MoveWindow takes screen coordinates
{
	POINT position={x, y};
	ClientToScreen(ghWnd, &position);
	movewindow_c(hWnd, position.x, position.y, w, h, repaint);
}
void			move_textbox()
{
	movewindow(hTextbox, text_s1.x+1, text_s1.y+1, text_s2.x-text_s1.x, text_s2.y-text_s1.y, true);
	//movewindow(hTextbox, text_s1.x, text_s1.y, text_s2.x-text_s1.x, text_s2.y-text_s1.y, true);
//	movewindow(hTextbox, 8+text_s1.x, 50+text_s1.y, text_s2.x-text_s1.x, text_s2.y-text_s1.y, true);
//	movewindow(hTextbox, 9+text_s1.x, 51+text_s1.y, text_s2.x-text_s1.x, text_s2.y-text_s1.y, true);
}
void			fliprotate_moveui(bool repaint)
{
	movewindow(hRotatebox, 924-17, h-49, 50, 21, repaint);
}
void			stretchskew_moveui(bool repaint)
{
	int x1=705, x2=x1+180,
		y1=h-69, y2=y1+22,
		dx=50, dy=21;
	movewindow(hStretchHbox, x1, y1, dx, dy, repaint);
	movewindow(hStretchVbox, x1, y2, dx, dy, repaint);
	movewindow(hSkewHbox, x2, y1, dx, dy, repaint);
	movewindow(hSkewVbox, x2, y2, dx, dy, repaint);
}
void			show_colorbarcontents(bool show)
{
	static const char zero[]="0", onehundred[]="100";
	int mask=-(int)show,
		ncmdshow=SW_SHOWNOACTIVATE&mask;//SH_HIDE is 0
	//	ncmdshow=SW_SHOWNA&mask;//SH_HIDE is 0
	switch(colorbarcontents)
	{
	case CS_TEXTOPTIONS:
		ShowWindow(hFontcombobox, ncmdshow);
		ShowWindow(hFontsizecb, ncmdshow);
		break;
	case CS_FLIPROTATE:
		if(show)
		{
			fliprotate_moveui(false);
			SetWindowTextA(hRotatebox, zero);
		}
		ShowWindow(hRotatebox, ncmdshow);
		break;
	case CS_STRETCHSKEW:
		if(show)
		{
			stretchskew_moveui(false);
			SetWindowTextA(hStretchHbox, onehundred);
			SetWindowTextA(hStretchVbox, onehundred);
			SetWindowTextA(hSkewHbox, zero);
			SetWindowTextA(hSkewVbox, zero);
		}
		ShowWindow(hStretchHbox, ncmdshow);
		ShowWindow(hStretchVbox, ncmdshow);
		ShowWindow(hSkewHbox, ncmdshow);
		ShowWindow(hSkewVbox, ncmdshow);
		break;
	}
}
void			replace_colorbarcontents(int contents)
{
	if(contents!=colorbarcontents)
	{
		show_colorbarcontents(false);
		colorbarcontents=contents;
		show_colorbarcontents(true);
	}
}
double			getwindowdouble(HWND hWnd)
{
	int length=GetWindowTextA(hWnd, g_buf, g_buf_size);
	return atof(g_buf);
}
enum			FlipRotate{FLIP_HORIZONTAL, FLIP_VERTICAL, ROTATE90, ROTATE180, ROTATE270, ROTATE_ARBITRARY};
void			fliprotate(int *buffer, int bw, int bh, int *&b2, int &w2, int &h2, int fr, int bk)
{
	switch(fr)
	{
	case FLIP_HORIZONTAL:
		w2=bw, h2=bh, b2=(int*)malloc(w2*h2<<2);
		for(int ky=0;ky<h2;++ky)
			for(int kx=0;kx<w2;++kx)
				b2[w2*ky+kx]=buffer[bw*ky+bw-1-kx];
		break;
	case FLIP_VERTICAL:
		w2=bw, h2=bh, b2=(int*)malloc(w2*h2<<2);
		for(int ky=0;ky<h2;++ky)
			for(int kx=0;kx<w2;++kx)
				b2[w2*ky+kx]=buffer[bw*(bh-1-ky)+kx];
		break;
	case ROTATE90:
		w2=bh, h2=bw, b2=(int*)malloc(w2*h2<<2);
		for(int ky=0;ky<h2;++ky)
			for(int kx=0;kx<w2;++kx)
				b2[w2*ky+kx]=buffer[bw*kx+bw-1-ky];
		break;
	case ROTATE180:
		w2=bw, h2=bh, b2=(int*)malloc(w2*h2<<2);
		for(int ky=0;ky<h2;++ky)
			for(int kx=0;kx<w2;++kx)
				b2[w2*ky+kx]=buffer[bw*(bh-1-ky)+bw-1-kx];
		break;
	case ROTATE270:
		w2=bh, h2=bw, b2=(int*)malloc(w2*h2<<2);
		for(int ky=0;ky<h2;++ky)
			for(int kx=0;kx<w2;++kx)
				b2[w2*ky+kx]=buffer[bw*(bh-1-kx)+ky];
		break;
	case ROTATE_ARBITRARY:
		{
			double radians=getwindowdouble(hRotatebox)*torad;
			if(radians)
			{
				double cth=cos(radians), sth=sin(radians);

				Point2d U(0, bh), UR(bw, bh), R(bw, 0);//determine new size
				U.rotate(cth, sth);
				UR.rotate(cth, sth);
				R.rotate(cth, sth);
				w2=(int)ceil(maximum(0, U.x, UR.x, R.x)-minimum(0, U.x, UR.x, R.x)), h2=(int)ceil(maximum(0, U.y, UR.y, R.y)-minimum(0, U.y, UR.y, R.y));

				int size2=w2*h2;
				b2=(int*)malloc(size2<<2);
				for(int k=0;k<size2;++k)
					b2[k]=bk;
				const double half=0.5;
				double bw_2=bw*half, bh_2=bh*half, w2_2=w2*half, h2_2=h2*half;
				if(interpolation_type==I_BILINEAR)
				{
					for(int ky=0;ky<h2;++ky)
					{
						for(int kx=0;kx<w2;++kx)
						{
							double x0=kx-w2_2, y0=ky-h2_2;
							double xd=bw_2+x0*cth+y0*sth, yd=bh_2-x0*sth+y0*cth;
							int x=(int)floor(xd), y=(int)floor(yd);
							double alpha_x=xd-x, alpha_y=yd-y;
							//byte *SW, *SE, *NE, *NW;
							//if((unsigned)x<(unsigned)bw)
							//{
							//	if((unsigned)y<(unsigned)bh)
							//	{
							//	}
							//	else
							//	{
							//	}
							//}
							byte *result=(byte*)(b2+w2*ky+kx),
								*NW=(unsigned)x  <(unsigned)bw&&(unsigned)y  <(unsigned)bh?(byte*)(buffer+bw* y   +x  ):(byte*)&bk,
								*SW=(unsigned)x  <(unsigned)bw&&(unsigned)y+1<(unsigned)bh?(byte*)(buffer+bw*(y+1)+x  ):(byte*)&bk,
								*SE=(unsigned)x+1<(unsigned)bw&&(unsigned)y+1<(unsigned)bh?(byte*)(buffer+bw*(y+1)+x+1):(byte*)&bk,
								*NE=(unsigned)x+1<(unsigned)bw&&(unsigned)y  <(unsigned)bh?(byte*)(buffer+bw* y   +x+1):(byte*)&bk;
							for(int kc=0;kc<4;++kc)
							{
								byte
									N=NW[kc]+int((NE[kc]-NW[kc])*alpha_x),
									S=SW[kc]+int((SE[kc]-SW[kc])*alpha_x);
								result[kc]=N+int((S-N)*alpha_y);
							}
						}
					}
				}
				else
				{
					for(int ky=0;ky<h2;++ky)
					{
						for(int kx=0;kx<w2;++kx)
						{
							double x0=kx-w2_2, y0=ky-h2_2;
							int x1=int(bw_2+x0*cth+y0*sth), y1=int(bh_2-x0*sth+y0*cth);
							//x1%=bw, x1+=bw&-(x1<0);
							//y1%=bh, y1+=bh&-(y1<0);
							if((unsigned)x1<(unsigned)bw&&(unsigned)y1<(unsigned)bh)
								b2[w2*ky+kx]=buffer[bw*y1+x1];
						}
					}
				}
			}
		}
		break;
	}
}
void			fliprotate(int fr=ROTATE_ARBITRARY)
{
	int *b2=nullptr, w2=0, h2=0;
	if(sel_start!=sel_end)
	{
		if(selection_free)
		{
			fliprotate(sel_buffer, sw, sh, b2, w2, h2, fr, secondarycolor);
			int *b3=nullptr, w3=0, h3=0;
			fliprotate(sel_mask, sw, sh, b3, w3, h3, fr, false);
			free(sel_mask);
			sel_mask=b3;
		}
		else
			fliprotate(sel_buffer, sw, sh, b2, w2, h2, fr, secondarycolor);
		free(sel_buffer);
		sel_buffer=b2, sw=w2, sh=h2;
		if(sel_start.x>sel_end.x)
			std::swap(sel_start.x, sel_end.x);
		if(sel_start.y>sel_end.y)
			std::swap(sel_start.y, sel_end.y);
		sel_end=sel_start+Point(sw, sh);
		selection_assign();
	}
	else
	{
		fliprotate(image, iw, ih, b2, w2, h2, fr, secondarycolor);
		//if(history[histpos].buffer!=image)//
		//	int LOL_1=0;//
		hist_postmodify(b2, w2, h2);
	}
}
void			stretchskew(int *buffer, int bw, int bh, int *&b2, int &w2, int &h2, double stretchH, double stretchV, double skewH, double skewV, int bk)
{
	double
		cH=cos(skewH), sH=sin(skewH), tH=sH/cH,
		cV=cos(skewV), sV=sin(skewV), tV=sV/cV;
	double
		A=1/stretchH, B=tH/stretchH,
		C=tV/stretchV, D=(1+tH*tV)/stretchV;
	//double
	//	A=1/stretchH, B=-tH/stretchH,
	//	C=-tV/stretchV, D=(1+tH*tV)/stretchV;
	//double
	//	A=1/stretchH, B=-sH/(cH*stretchH),
	//	C=0, D=1/(cH*stretchV);
	//double
	//	A=1/stretchH, B=-sH/cH/stretchH,
	//	C=0, D=1/cH;
	
	double
		A2=1*stretchH, B2=tH*stretchH,
		C2=tV*stretchV, D2=(1+tH*tV)*stretchV;
	Point2d U(0, bh), UR(bw, bh), R(bw, 0);//determine new size
	U.transform(A2, B2, C2, D2);
	UR.transform(A2, B2, C2, D2);
	R.transform(A2, B2, C2, D2);
	w2=(int)ceil(maximum(0, U.x, UR.x, R.x)-minimum(0, U.x, UR.x, R.x)), h2=(int)ceil(maximum(0, U.y, UR.y, R.y)-minimum(0, U.y, UR.y, R.y));
//	w2=bw, h2=bh;//

	int size2=w2*h2;
	b2=(int*)malloc(size2<<2);
	for(int k=0;k<size2;++k)
		b2[k]=bk;
	const double half=0.5;
	double bw_2=bw*half, bh_2=bh*half, w2_2=w2*half, h2_2=h2*half;
	if(interpolation_type==I_BILINEAR)
	{
		for(int ky=0;ky<h2;++ky)
		{
			for(int kx=0;kx<w2;++kx)
			{
				double x0=kx-w2_2, y0=ky-h2_2;
				double xd=bw_2+x0*A+y0*B, yd=bh_2+x0*C+y0*D;
				int x=(int)floor(xd), y=(int)floor(yd);
				double alpha_x=xd-x, alpha_y=yd-y;
				byte *result=(byte*)(b2+w2*ky+kx),
					*NW=(unsigned)x  <(unsigned)bw&&(unsigned)y  <(unsigned)bh?(byte*)(buffer+bw* y   +x  ):(byte*)&bk,
					*SW=(unsigned)x  <(unsigned)bw&&(unsigned)y+1<(unsigned)bh?(byte*)(buffer+bw*(y+1)+x  ):(byte*)&bk,
					*SE=(unsigned)x+1<(unsigned)bw&&(unsigned)y+1<(unsigned)bh?(byte*)(buffer+bw*(y+1)+x+1):(byte*)&bk,
					*NE=(unsigned)x+1<(unsigned)bw&&(unsigned)y  <(unsigned)bh?(byte*)(buffer+bw* y   +x+1):(byte*)&bk;
				for(int kc=0;kc<4;++kc)
				{
					byte
						N=NW[kc]+int((NE[kc]-NW[kc])*alpha_x),
						S=SW[kc]+int((SE[kc]-SW[kc])*alpha_x);
					result[kc]=N+int((S-N)*alpha_y);
				}
			}
		}
	}
	else
	{
		for(int ky=0;ky<h2;++ky)
		{
			for(int kx=0;kx<w2;++kx)
			{
				double x0=kx-w2_2, y0=ky-h2_2;
				int x1=int(bw_2+x0*A+y0*B), y1=int(bh_2+x0*C+y0*D);
				if((unsigned)x1<(unsigned)bw&&(unsigned)y1<(unsigned)bh)
					b2[w2*ky+kx]=buffer[bw*y1+x1];
			}
		}
	}
}
void			stretchskew()
{
	double
		stretchH=getwindowdouble(hStretchHbox)*0.01,
		stretchV=getwindowdouble(hStretchVbox)*0.01,
		skewH=getwindowdouble(hSkewHbox)*torad,
		skewV=getwindowdouble(hSkewVbox)*torad;
	if(stretchH!=1||stretchV!=1||skewH||skewV)
	{
		int *b2=nullptr, w2=0, h2=0;
		if(sel_start!=sel_end)
		{
			if(selection_free)
			{
				stretchskew(sel_buffer, sw, sh, b2, w2, h2, stretchH, stretchV, skewH, skewV, secondarycolor);
				int *b3=nullptr, w3=0, h3=0;
				stretchskew(sel_mask, sw, sh, b3, w3, h3, stretchH, stretchV, skewH, skewV, false);
				free(sel_mask);
				sel_mask=b3;
			}
			else
				stretchskew(sel_buffer, sw, sh, b2, w2, h2, stretchH, stretchV, skewH, skewV, secondarycolor);
			free(sel_buffer);
			sel_buffer=b2, sw=w2, sh=h2;
			if(sel_start.x>sel_end.x)
				std::swap(sel_start.x, sel_end.x);
			if(sel_start.y>sel_end.y)
				std::swap(sel_start.y, sel_end.y);
			sel_end=sel_start+Point(sw, sh);
			selection_assign();
		}
		else
		{
			stretchskew(image, iw, ih, b2, w2, h2, stretchH, stretchV, skewH, skewV, secondarycolor);
			hist_postmodify(b2, w2, h2);
		}
		//static const char zero[]="0", onehundred[]="100";
		//SetWindowTextA(hStretchHbox, onehundred);
		//SetWindowTextA(hStretchVbox, onehundred);
		//SetWindowTextA(hSkewHbox, zero);
		//SetWindowTextA(hSkewVbox, zero);
	}
}
void			blend_zoomed(const int *buffer, int x1, int x2, int y1, int y2)
{
	for(int ky=y1;ky<y2;++ky)
	{
		for(int kx=x1;kx<x2;++kx)
		{
			int ix=spx+shift(kx-x1, -logzoom), iy=spy+shift(ky-y1, -logzoom);
			if(icheck(ix, iy))//robust
				break;//
#ifdef USE_SSE2
			const __m128i zero=_mm_setzero_si128();
		//	const __m128 inv255=_mm_set1_ps(1.f/255);
			const float inv255=1.f/255;
			const __m128 half=_mm_set1_ps(0.5f);

			__m128i dst=zero, src=zero;
			auto &src2=buffer[iw*iy+ix];
			auto &dst2=rgb[w*ky+kx];
			dst.m128i_i32[0]=dst2, src.m128i_i32[0]=src2;
			__m128 alpha_255=_mm_set1_ps(((const byte*)&src2)[3]*inv255);
		//	__m128 alpha_255=_mm_set1_ps((src2>>24&0xFF)*inv255);
			dst=_mm_unpacklo_epi8(dst, zero);//1
			src=_mm_unpacklo_epi8(src, zero);//1
			dst=_mm_unpacklo_epi8(dst, zero);//1
			src=_mm_unpacklo_epi8(src, zero);//1

			__m128i diff=_mm_sub_epi32(src, dst);//1
			__m128 d2=_mm_cvtepi32_ps(diff);//4
			d2=_mm_mul_ps(d2, alpha_255);//5
			d2=_mm_sub_ps(d2, half);//3		(int)d2 == round(d2-0.5)
			diff=_mm_cvtps_epi32(d2);//4
			dst=_mm_add_epi32(dst, diff);//1
			dst=_mm_packs_epi32(dst, zero);//1
			dst=_mm_packs_epi16(dst, zero);//1
			dst2=dst.m128i_i32[0];


			//__m128i alpha=_mm_set1_epi32(src2>>24&0xFF);
			//__m128i diff=_mm_sub_epi32(src, dst);
			//
			//diff=_mm_mullo_epi16(diff, alpha);
			////diff=_mm_mul_epi32(diff, alpha);//X SSE4.1, two 64bit results
			//
			//__m128 d2=_mm_cvtepi32_ps(diff);//4
			//d2=_mm_mul_ps(d2, inv255);//5
			//diff=_mm_cvtps_epi32(d2);//4
			//dst=_mm_add_epi32(dst, diff);
			//dst=_mm_packs_epi32(dst, zero);
			//dst=_mm_packs_epi16(dst, zero);
			//dst2=dst.m128i_i32[0];
			////_mm_store1_ps((float*)(rgb+w*ky+kx), _mm_castsi128_ps(dst));//X aligned


			//dst=_mm_mul_epi32(dst, src);
			//__m128 d2=_mm_cvtepi32_ps(dst);
			//d2=_mm_mul_ps(d2, inv255);
			//dst=_mm_cvtps_epi32(d2);

			int LOL_1=0;//
#else
			auto dst=(unsigned char*)(rgb+w*ky+kx), src=(unsigned char*)(buffer+iw*iy+ix);
			auto &alpha=src[3];
			dst[0]=dst[0]+((src[0]-dst[0])*alpha/255);//...>>8 leaves artifacts
			dst[1]=dst[1]+((src[1]-dst[1])*alpha/255);
			dst[2]=dst[2]+((src[2]-dst[2])*alpha/255);
#endif
		}
	}
#ifdef SSE2
	_m_empty();
#endif
}
void			render()
{
	prof_start();
	memset(rgb, 0xAB, rgbn<<2);
	prof_add("memset");
	
	const int scrollbarwidth=17;//19
	int x1=0, x2=w, y1=0, y2=h;
	//draw UI
#if 1
	const int thbox_w0=3+10+90+10;//separator + max thumbnail width with padding
	if(showthumbnailbox=animated&&w>thbox_w0+scrollbarwidth*(thbox_scrollbar=thbox_h>y2-y1)&&y2-y1>34)//draw thumbnail box
	{
		if(!thbox_scrollbar)
			thbox_posy=0;
		if(thbox_posy<0)
			thbox_posy=0;
		int thbox_w=thbox_w0+scrollbarwidth*thbox_scrollbar;
		thbox_x1=w-thbox_w;
		rectangle(thbox_x1, w, y1, y2, 0xF0F0F0);
		v_line(thbox_x1+1, y1, y2, 0xFFFFFF);
		v_line(thbox_x1+2, y1, y2, 0xA0A0A0);
		int th_x1=thbox_x1+13, th_x2=w-(10+scrollbarwidth*thbox_scrollbar);//thumbnail space
		if(thbox_scrollbar)
			draw_vscrollbar(x2-scrollbarwidth, scrollbarwidth, y1, y2, thbox_posy, thbox_h, thbox_vscroll_s0, thbox_vscroll_my_start, 1, thbox_vscroll_start, thbox_vscroll_size, D_THBOX_VSCROLL);
		int bkmode0=SetBkMode(ghMemDC, TRANSPARENT);
		const int color_selection=0xA85E33;
		for(int kth=0;kth<nframes;++kth)
		{
			int xpos=((th_x2+th_x1)>>1)-(th_w>>1),//center of thumbnail space minus half thumbnail width
				ypos=10+(16+th_h+10)*kth-thbox_posy;
			if(kth==current_frame)
				rectangle(xpos-5, xpos+90+5, ypos-5, ypos+16+th_h+5, swap_rb(color_selection));
			int bkmode=0, bkcolor=0, textcolor=0;
			if(kth==current_frame)
				bkmode=SetBkMode(ghMemDC, OPAQUE), bkcolor=SetBkColor(ghMemDC, color_selection), textcolor=SetTextColor(ghMemDC, 0xFFFFFF);
			GUIPrint(ghMemDC, th_x1, ypos, kth+1);
			if(kth==current_frame)
				SetBkMode(ghMemDC, bkmode), SetBkColor(ghMemDC, bkcolor), SetTextColor(ghMemDC, textcolor);
			ypos+=16;
			int *thumbnail=thumbnails[kth];
			for(int ky=0, ky2=ypos+ky;ky<th_h&&ky2>=y1&&ky2<y2;++ky, ++ky2)
			{
				for(int kx=0;kx<th_w;++kx)
				{
					if(thumbnail)
					{
						auto dst=(byte*)(rgb+w*ky2+xpos+kx), src=(byte*)(thumbnail+th_w*ky+kx);
						auto &alpha=src[3];
						dst[0]=dst[0]+((src[0]-dst[0])*alpha>>8);
						dst[1]=dst[1]+((src[1]-dst[1])*alpha>>8);
						dst[2]=dst[2]+((src[2]-dst[2])*alpha>>8);
					}
					else
						rgb[w*ky2+xpos+kx]=0xFFFFFF;
				}
			}
		}
		SetBkMode(ghMemDC, bkmode0);
		x2-=thbox_w;
	}
	else
		thbox_scrollbar=false;
	prof_add("thumbnail box");
	if(showcolorbar=h>74&&x2-x1>=256)//draw color palette
	{
		rectangle(x1, x2, h-74, h-73, 0xE3E3E3);
		rectangle(x1, x2, h-73, h-72, 0xFFFFFF);
		rectangle(x1, x2, h-72, h-25, 0xF0F0F0);
		_3d_hole(x1, x1+31, h-65, h-33);
		rect_checkboard(2, 29, h-63, h-35, 0xF0F0F0, 0xFFFFFF);
		_3d_border(11, 26, h-53, h-38);
		rectangle(13, 24, h-51, h-40, secondarycolor);//have alpha
		_3d_border(4, 19, h-60, h-45);
		rectangle(6, 17, h-58, h-47, primarycolor);
		for(int ky=0;ky<2;++ky)
		{
			int y=h-65+(ky<<4);
			for(int kx=0;kx<14;++kx)
			{
				int x=31+(kx<<4);
				_3d_hole(x, x+16, y, y+16);
				rectangle(x+2, x+14, y+2, y+14, colorpalette[kx<<1|ky]);
			}
		}
		if(x2-x1>575)//draw alphas slider
		{
			const int sstart=320;//slider start
			if(drag==D_ALPHA1)
				primary_alpha=clamp(0, mx-sstart, 255), assign_alpha(primarycolor, primary_alpha);
			else if(drag==D_ALPHA2)
				secondary_alpha=clamp(0, mx-sstart, 255), assign_alpha(secondarycolor, secondary_alpha);
			const int color_aslider=0x808080;
			int c0=SetTextColor(ghMemDC, color_aslider), bk0=SetBkMode(ghMemDC, TRANSPARENT);
			GUIPrint(ghMemDC, 270, h-57, "Alpha:");
			GUIPrint(ghMemDC, 595, h-67, "%3d", primary_alpha);
			GUIPrint(ghMemDC, 595, h-51, "%3d", secondary_alpha);
		//	GUIPrint(ghMemDC, 279, h-51, g_alpha);
			SetTextColor(ghMemDC, c0), SetBkMode(ghMemDC, bk0);

			h_line(sstart, sstart+255, h-48, color_aslider);
			v_line(sstart, h-59, h-38, color_aslider);
			v_line(sstart+255, h-59, h-38, color_aslider);
			const int n=14;
			int x=sstart+primary_alpha, y=h-50;
			line45_forward(x-n, y-n, n, color_aslider);
			line45_sideways(x, y, n, color_aslider);
			h_line(x-n, x+n+1, y-n, color_aslider);
			x=sstart+secondary_alpha, y=h-47;
			line45_sideways(x-n, y+n, n, color_aslider);
			line45_forward(x, y, n, color_aslider);
			h_line(x-n, x+n+1, y+n, color_aslider);
		}
	//	if(currentmode==M_TEXT&&x2-x1>850)//draw bold, italic, underline buttons
		switch(colorbarcontents)
		{
		case CS_TEXTOPTIONS:
			if(x2-x1>850)
			{
				int x0=650, y0=h-48;
				int bkmode=SetBkMode(ghMemDC, TRANSPARENT);
				int color_sel=0xA0A0A0,//0xFF9933
					color_text=0x000000;//0xFFFFFF
				if(bold)
					rectangle(x0+50+1, x0+100-1, y0+1, y0+23-1, color_sel);
				//	rectangle(x0+50, x0+100, y0, y0+25, color_sel);
				else
					rectangle_hollow(x0+50+1, x0+100-1, y0+1, y0+23-1, color_sel);
				if(italic)
					rectangle(x0+100+1, x0+150-1, y0+1, y0+23-1, color_sel);
				else
					rectangle_hollow(x0+100+1, x0+150-1, y0+1, y0+23-1, color_sel);
				if(underline)
					rectangle(x0+150+1, x0+200-1, y0+1, y0+23-1, color_sel);
				else
					rectangle_hollow(x0+150+1, x0+200-1, y0+1, y0+23-1, color_sel);
				GUIPrint(ghMemDC, x0+70, y0+3, "B");
				GUIPrint(ghMemDC, x0+120, y0+3, "I");
				GUIPrint(ghMemDC, x0+170, y0+3, "U");
				SetBkMode(ghMemDC, bkmode);
			}
			break;
		case CS_FLIPROTATE:
			if(x2-x1>970)
			{
				int x0=650, y0=h-68;
				int bkmode=SetBkMode(ghMemDC, TRANSPARENT);
				HFONT hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
				hFont=(HFONT)SelectObject(ghMemDC, hFont);
				GUIPrint(ghMemDC, x0, y0, "Flip horizontal");
				GUIPrint(ghMemDC, x0, y0+20, "Flip vertical");
				GUIPrint(ghMemDC, x0+104, y0, "Rotate by 90");
				GUIPrint(ghMemDC, x0+104, y0+20, "Rotate by 180");
				GUIPrint(ghMemDC, x0+208, y0, "Rotate by 270");
				GUIPrint(ghMemDC, x0+208, y0+20, "Rotate by");
				GUIPrint(ghMemDC, x0+208+101, y0+20, "degrees");
				hFont=(HFONT)SelectObject(ghMemDC, hFont);
				SetBkMode(ghMemDC, bkmode);
			}
			break;
		case CS_STRETCHSKEW:
			if(x2-x1>940)
			{
				int x0=630, y0=h-68;
				int bkmode=SetBkMode(ghMemDC, TRANSPARENT);
				HFONT hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
				hFont=(HFONT)SelectObject(ghMemDC, hFont);
				GUIPrint(ghMemDC, x0, y0+8, "Stretch:");
				GUIPrint(ghMemDC, x0+128, y0+8-10, "%%");
				GUIPrint(ghMemDC, x0+128, y0+8-10+22, "%%");
				GUIPrint(ghMemDC, x0+190, y0+8, "Skew:");
				GUIPrint(ghMemDC, x0+190+120, y0+8-9, "degrees");
				GUIPrint(ghMemDC, x0+190+120, y0+8-9+22, "degrees");
				hFont=(HFONT)SelectObject(ghMemDC, hFont);
				SetBkMode(ghMemDC, bkmode);
				static const int icon_stretch[]=
				{
					0x10008,
					0x3000C,
					0x7000E,
					0xFFFFF,
					0x7000E,
					0x3000C,
					0x10008,
				};
				const int w_str=20, h_str=7;//20x7 icon
				static const int icon_skew[]=
				{
					0x0003,
					0x001C,
					0x00E8,
					0x071C,
					0x383E,
					0xC07F,
					0x0008,
					0x0008,
					0x0008,
					0x0008,
					0x0008,
					0xFFFF,
				};
				const int w_skew=16, h_skew=12;//16x12 icon
				draw_icon_monochrome			(icon_stretch, w_str, h_str, 680, h-64, 0xFF0000);
				draw_icon_monochrome_transposed	(icon_stretch, w_str, h_str, 680+5, h-64+16, 0xFF0000);
				draw_icon_monochrome_transposed	(icon_skew, w_skew, h_skew, 680+185, h-64-3, 0xFF0000);
				draw_icon_monochrome			(icon_skew, w_skew, h_skew, 680+185-1, h-64-3+24, 0xFF0000);
				//int ix=660, iy=h-64;//icon position
				//for(int ky=0;ky<h_res;++ky)
				//	for(int kx=0;kx<w_res;++kx)
				//		if(icon_resize[ky]>>kx&1)
				//			rgb[w*(iy+ky)+ix+kx]=0xFF0000;
			}
			break;
		}
		y2-=74;
	}//end color palette
	prof_add("color palette");
	if(showtoolbox=h-74*showcolorbar>276)//draw tool box
	{
		h_line(0, 56, 0, 0xA0A0A0);
		h_line(0, 56, 1, 0xFFFFFF);
		rectangle(0, 56, 2, h-74, 0xF0F0F0);
		v_line(56, 0, h-73, 0xA0A0A0);
		h_line(0, 57, h-74, 0xA0A0A0);
		static bool icons_packed=true;
		if(icons_packed)//extract icons
		{
			icons_packed=false;
			const unsigned char *rle_icons[]=
			{
				resources::free_select,	resources::select,		resources::eraser,	resources::fill,
				resources::pick_color,	resources::magnifier,	resources::pencil,	resources::brush,
				resources::airbrush,	resources::text,		resources::line,	resources::curve,
				resources::rectangle,	resources::polygon,		resources::ellipse,	resources::rounded_rectangle,
			};
			int lengths[]=
			{
				sizeof resources::free_select,	sizeof resources::select,		sizeof resources::eraser,	sizeof resources::fill,
				sizeof resources::pick_color,	sizeof resources::magnifier,	sizeof resources::pencil,	sizeof resources::brush,
				sizeof resources::airbrush,		sizeof resources::text,			sizeof resources::line,		sizeof resources::curve,
				sizeof resources::rectangle,	sizeof resources::polygon,		sizeof resources::ellipse,	sizeof resources::rounded_rectangle,
			};
			nicons=sizeof(lengths)>>2;
			icons=(int*)malloc(256*nicons<<2);
			for(int ki=0;ki<nicons;++ki)
				unpack_icon(rle_icons[ki], lengths[ki], icons+(ki<<8));
		}//end extract icons
		for(int ki=0;ki<nicons;++ki)//draw icons
		{
			int x=7+25*(ki&1), y=9+25*(ki>>1);
			int *icon=icons+(ki<<8);
			if(ki==currentmode)
			{
				for(int ky=0;ky<23;++ky)
					for(int kx=0;kx<25;++kx)
						rgb[w*(y-3+ky)+x-4+kx]=0xA0A0A0;
			}
			draw_icon(icon, x, y);
		}
		h_line(7, 48, 210, 0xA0A0A0);
		v_line(7, 211, 275, 0xA0A0A0);
		h_line(8, 49, 275, 0xFFFFFF);
		v_line(48, 211, 276, 0xFFFFFF);
		x1+=58;
	}//end toolbox
	prof_add("toolbox");
#endif
	if(image)
	{
		if(showtoolbox)
		{
			h_line(x1-1, x2, y1, 0xA0A0A0);
			h_line(x1-1, x2, y1+1, 0x696969);
			v_line(x1-1, y1+2, y2, 0x696969);
		}
		int x1_2=x1+3, y1_2=y1+5;

		//draw possible scroll bars
#if 1
		hscrollbar=iw*zoom>x2-x1, vscrollbar=ih*zoom>y2-y1;
		if(spx<0)
			spx=0;
		if(spy<0)
			spy=0;
		int xend=x1_2+shift(iw-spx, logzoom), yend=y1_2+shift(ih-spy, logzoom);
		hscrollbar|=!hscrollbar&&vscrollbar&&xend>=w-scrollbarwidth;
		vscrollbar|=!vscrollbar&&hscrollbar&&yend>=h-74-scrollbarwidth;
		int color_barbk=0xF0F0F0, color_slider=0xCDCDCD, color_button=0xE5E5E5;//0xDDDDDD
		if(hscrollbar&&vscrollbar)
			rectangle(x2-scrollbarwidth, x2, y2-scrollbarwidth, y2, color_barbk);//corner square
		if(hscrollbar)
		{
			int vsbw=scrollbarwidth*vscrollbar;
			rectangle(x1+scrollbarwidth, x2-scrollbarwidth-vsbw, y2-scrollbarwidth, y2, color_barbk);//horizontal
			rectangle(x1, x1+scrollbarwidth, y2-scrollbarwidth, y2, color_button);//left
			rectangle(x2-scrollbarwidth-vsbw, x2-vsbw, y2-scrollbarwidth, y2, color_button);//right
			if(drag==D_HSCROLL)
				scrollbar_scroll(spx, iw, x2-x1-vsbw, x2-x1-(scrollbarwidth<<1)-vsbw, hscroll_s0, hscroll_mx_start, mx, zoom, hscroll_start, hscroll_size);
			else
				scrollbar_slider(spx, iw, x2-x1-vsbw, x2-x1-(scrollbarwidth<<1)-vsbw, zoom, hscroll_start, hscroll_size);
			rectangle(x1+scrollbarwidth+hscroll_start, x1+scrollbarwidth+hscroll_start+hscroll_size, y2-scrollbarwidth, y2, color_slider);//horizontal slider
			y2-=scrollbarwidth;
		}
		else
			spx=0;
		if(vscrollbar&&h-74*showcolorbar>scrollbarwidth*3)
		{
			draw_vscrollbar(x2-scrollbarwidth, scrollbarwidth, y1, y2, spy, ih, vscroll_s0, vscroll_my_start, zoom, vscroll_start, vscroll_size, D_VSCROLL);
			x2-=scrollbarwidth;
		}
		else
			spy=0;
#endif

		x1=x1_2, y1=y1_2;
		if(tw!=iw||th!=ih)
			temp_buffer=(int*)realloc(temp_buffer, image_size<<2), tw=iw, th=ih;
		if(!prev_drag)
			start_mx=prev_mx=mx, start_my=prev_my=my;

		//draw tool type selection box below the toolbox
#if 1
		const int color_selection=0x3399FF;
		switch(currentmode)
		{
		case M_FREE_SELECTION:
		case M_RECT_SELECTION:
		case M_TEXT:
			{
				const int cw=36, ch=23, size=cw*ch<<1;
				static bool icons_packed=true;
				if(icons_packed)//extract 'selection type' icons
				{
					icons_packed=false;
					icons_seltype=(int*)malloc(size<<2);
					const unsigned char levels[]={0, 0x80, 0xC0, 0xFF};
					for(int k=0;k<size;++k)
					{
						auto &c=resources::selection_transparency[k];
						auto &c2=icons_seltype[k];
						unsigned char alpha=c>>6;
						if(alpha)
							c2=0xFF<<24|levels[c>>4&3]<<16|levels[c>>2&3]<<8|levels[c&3];
						else
							c2=0;
					}
				}
				int *icon=icons_seltype+(selection_transparency==TRANSPARENT)*(size>>1);
				for(int ky=0;ky<ch;++ky)
				{
					for(int kx=0;kx<cw;++kx)
					{
						auto &c=icon[cw*ky+kx];
						if(c&0xFF000000)
							rgb[w*(215+ky)+9+kx]=c&0x00FFFFFF;
					}
				}
				draw_icon(icons+(M_RECT_SELECTION<<8), 15+4, 246+3);
				if(selection_persistent)
					draw_icon(icons+(M_PENCIL<<8), 19+4, 242+3);
			}
			break;
		case M_ERASER:
			if(showtoolbox)
			{
				const int selsize=14;
				for(int k=0;k<4;++k)
				{
					auto b=resources::brushes+ERASER04+k;
					int xstart=21, ystart=212+(k<<4);
					int color_in;
					if(ERASER04+k==erasertype)
					{
						for(int ky=0;ky<selsize;++ky)
							for(int kx=0;kx<selsize;++kx)
								rgb[w*(ystart+ky)+xstart+kx]=color_selection;
						color_in=0xFFFFFF;
					}
					else
						color_in=0;
					xstart+=7, ystart+=7;
					for(int ky=0;ky<b->ysize;++ky)
						for(int kx=b->bounds[ky<<1], kxEnd=b->bounds[ky<<1|1];kx<=kxEnd;++kx)
							rgb[w*(ystart+b->yoffset+ky)+xstart+kx]=color_in;
				}
			}
			break;
		case M_MAGNIFIER:
			if(showtoolbox)
			{
				static const char *zoomlevels[]=
				{
					"1x", "2x",
					"4x", "8x",
					"16x", "32x",
				};
				static const char chars[]=
				{
					 4,  7,  4,  4,  4,  4,  4,  4,//0<<3:	1
					14, 17, 16,  8,  4,  2,  1, 31,//1<<3:	2
					14, 17, 16, 12, 16, 16, 17, 14,//2<<3:	3
					 8, 12, 10,  9,  9, 31,  8,  8,//3<<3:	4
					14, 17,  1, 15, 17, 17, 17, 14,//4<<3:	6
					14, 17, 17, 14, 17, 17, 17, 14,//5<<3:	8
					 0,  0,  9,  9,  6,  6,  9,  9,//6<<3:	x
				};
				static const char widths[]={4, 5, 5, 5, 5, 5, 4};
				for(int k=0;k<6;++k)
				{
					int x0=7+21*(k&1)+1, y0=210+22*(k>>1)+1;
					const char *zoomlevel=zoomlevels[k];
					int size=0, length=0;
					for(int kc=0;zoomlevel[kc];++kc)
					{
						int idx=0;
						switch(zoomlevel[kc])
						{
						case '1':idx=0;break;
						case '2':idx=1;break;
						case '3':idx=2;break;
						case '4':idx=3;break;
						case '6':idx=4;break;
						case '8':idx=5;break;
						case 'x':idx=6;break;
						}
						size+=widths[idx];
						++length;
					}
					size+=length-1;
					int color_in;
					if(k==magnifier_level)
					{
						for(int ky=0;ky<20;++ky)
							for(int kx=0;kx<19;++kx)
								rgb[w*(y0+ky)+x0+kx]=color_selection;
						color_in=0xFFFFFF;
					}
					else
						color_in=0;
					x0+=10-(size>>1), y0+=11-4;
				//	int offset=11-(size>>1);
					for(int kc=0;zoomlevel[kc];++kc)
					{
						int idx=0;
						switch(zoomlevel[kc])
						{
						case '1':idx=0;break;
						case '2':idx=1;break;
						case '3':idx=2;break;
						case '4':idx=3;break;
						case '6':idx=4;break;
						case '8':idx=5;break;
						case 'x':idx=6;break;
						}
						int width=widths[idx];
						for(int ky=0;ky<8;++ky)
						{
							char row=chars[idx<<3|ky];
							for(int kx=0;kx<width;++kx)
								if(row>>kx&1)
									rgb[w*(y0+ky)+x0+kx]=color_in;
						}
						x0+=width+1;
					}
				}
			}
			break;
		case M_BRUSH:
			if(showtoolbox)
			{
				const int
					xoffsets[]={2, 2, 2,  2, 2, 2,  2, 2, 2,  2, 2, 2},
					yoffsets[]={5, 6, 5,  6, 5, 6,  6, 5, 6,  6, 5, 6},
					seldx	[]={4, 5, 4,  5, 4, 5,  5, 4, 5,  5, 4, 5},//selection dx
					seldy	[]={11, 12, 11,  12, 11, 12,  12, 11, 12,  12, 11, 12};
				for(int k=0;k<12;++k)
				{
					auto b=resources::brushes+BRUSH_LARGE_DISK+k;
					int xstart=12+k%3*13, ystart=213+k/3*16;
					int color_in;
					if(BRUSH_LARGE_DISK+k==brushtype)
					{
						for(int ky=0;ky<seldy[k];++ky)
							for(int kx=0;kx<seldx[k];++kx)
								rgb[w*(ystart+ky)+xstart+kx]=color_selection;
						color_in=0xFFFFFF;
					}
					else
						color_in=0;
					for(int ky=0;ky<b->ysize;++ky)
						for(int kx=b->bounds[ky<<1], kxEnd=b->bounds[ky<<1|1];kx<=kxEnd;++kx)
							rgb[w*(ystart+yoffsets[k]+b->yoffset+ky)+xstart+xoffsets[k]+kx]=color_in;
				}
			}
			break;
		case M_AIRBRUSH:
			break;
		case M_LINE:case M_CURVE:
			if(showtoolbox)
			{
				const int yoffsets[]={4, 4, 3, 3, 2};
				for(int k=0;k<5;++k)
				{
					int xstart=10, ystart=214+12*k;
					int color_in;
					if(k+1==linewidth)
					{
						for(int ky=0;ky<10;++ky)
							for(int kx=0;kx<36;++kx)
								rgb[w*(ystart+ky)+xstart+kx]=color_selection;
						color_in=0xFFFFFF;
					}
					else
						color_in=0;
					for(int ky=0;ky<=k;++ky)
						for(int kx=0;kx<28;++kx)
							rgb[w*(ystart+yoffsets[k]+ky)+14+kx]=color_in;
				}
			}
			break;
		case M_RECTANGLE:case M_POLYGON:case M_ELLIPSE:case M_ROUNDED_RECT:
			if(showtoolbox)
			{
				//10,214 20 36x18
				int shape_type=0;
				switch(currentmode)
				{
				case M_RECTANGLE:	shape_type=rectangle_type;break;
				case M_POLYGON:		shape_type=polygon_type;break;
				case M_ELLIPSE:		shape_type=ellipse_type;break;
				case M_ROUNDED_RECT:shape_type=roundrect_type;break;
				}
				rectangle(10, 10+36, 214+20*shape_type, 214+18+20*shape_type, color_selection);
				int color_solid=0xA0A0A0;

				int color_line=shape_type==0?0xFFFFFF:0;//hollow option
				h_line(14, 42, 218, color_line);
				h_line(14, 42, 227, color_line);
				v_line(14, 219, 227, color_line);
				v_line(41, 219, 227, color_line);

				color_line=shape_type==1?0xFFFFFF:0;//full option
				rectangle(15, 41, 239, 247, color_solid);
				h_line(14, 42, 238, color_line);
				h_line(14, 42, 247, color_line);
				v_line(14, 239, 247, color_line);
				v_line(41, 239, 247, color_line);

				rectangle(14, 42, 258, 268, color_solid);//fill (solid) option
			}
			break;
		}
#endif

		if(drag==D_DRAW||drag==D_SELECTION)
		{
			int color, color2;
			if(mb[0])
				color=primarycolor, color2=secondarycolor;
			else
				color=secondarycolor, color2=primarycolor;
			switch(currentmode)
			{
			case M_FREE_SELECTION:
				if(drag==D_DRAW)
				{
					freesel_add_mouse(mx, my);
					memcpy(temp_buffer, image, image_size<<2), use_temp_buffer=true;
					bool *imask=(bool*)malloc(image_size);
					memset(imask, 0, image_size);
					for(int k=1, nv=freesel.size();k<nv;++k)
					{
						auto &p1=freesel[k-1], &p2=freesel[k];
						draw_line_brush(temp_buffer, LINE3, p1.x, p1.y, p2.x, p2.y, 0, true, imask);
					}
					free(imask);
				}
				else if(drag==D_SELECTION)
					selection_move_mouse(prev_mx, prev_my, mx, my);
				break;
			case M_RECT_SELECTION:
				if(drag==D_DRAW)//select with mouse
					screen2image(mx, my, sel_end.x, sel_end.y);
				else if(drag==D_SELECTION)//drag selection
					selection_move_mouse(prev_mx, prev_my, mx, my);
				break;
			case M_ERASER:
				if(!prev_drag)
					hist_premodify(image, iw, ih);
				draw_line_brush_mouse(image, erasertype, prev_mx, prev_my, mx, my, color2);
				break;
		//	case M_FILL:			break;//done
			case M_PICK_COLOR:
				if(showtoolbox)
					rectangle(8, 48, 211, 275, pick_color_mouse(image, mx, my));
				break;
		//	case M_MAGNIFIER:		break;//done
			case M_PENCIL:
				if(!prev_drag)
					hist_premodify(image, iw, ih);
				draw_line_mouse(image, prev_mx, prev_my, mx, my, color);
				break;
			case M_BRUSH:
				if(!prev_drag)
					hist_premodify(image, iw, ih);
				draw_line_brush_mouse(image, brushtype, prev_mx, prev_my, mx, my, color);
				break;
			case M_AIRBRUSH:
				if(!prev_drag)
					hist_premodify(image, iw, ih);
				airbrush_mouse(image, mx, my, color);
				break;
			case M_TEXT:
				if(drag==D_DRAW)//define textbox with mouse
					screen2image(mx, my, text_end.x, text_end.y);
				else if(drag==D_SELECTION)//drag textbox
				{
					Point m, prev_m;
					screen2image(mx, my, m.x, m.y);
					screen2image(prev_mx, prev_my, prev_m.x, prev_m.y);
					Point d=m-prev_m;
					textpos+=d;
					text_start+=d, text_end+=d, text_s1+=d, text_s2+=d;
					move_textbox();
				}
				break;
			case M_LINE:
				memset(temp_buffer, 0, image_size<<2), use_temp_buffer=true;
				draw_line_mouse(temp_buffer, start_mx, start_my, mx, my, 0xFF000000|color);
				break;
			case M_CURVE:
				curve_update_mouse(mx, my);
				memset(temp_buffer, 0, image_size<<2), use_temp_buffer=true;
				curve_draw(temp_buffer, color);
				break;
			case M_RECTANGLE:
				memset(temp_buffer, 0, image_size<<2), use_temp_buffer=true;
				draw_rectangle_mouse(temp_buffer, start_mx, mx, start_my, my, 0xFF000000|color, 0xFF000000|color2);
				break;
			case M_POLYGON://draw temp outline
				{
					int c3=0xFF000000|(polygon_leftbutton?primarycolor:secondarycolor);
					memset(temp_buffer, 0, image_size<<2), use_temp_buffer=true;
					for(int k=0, npoints=polygon.size();k<npoints-1;++k)
					{
						auto &p1=polygon[k], &p2=polygon[(k+1)%npoints];
						draw_line_brush(temp_buffer, linewidth, p1.x, p1.y, p2.x, p2.y, c3);
					}
					if(polygon.size())
					{
						auto &p=*polygon.rbegin();
						Point i;
						screen2image(mx, my, i.x, i.y);
						draw_line_brush(temp_buffer, linewidth, p.x, p.y, i.x, i.y, c3);
					}
					else
						draw_line_mouse(temp_buffer, start_mx, start_my, mx, my, c3);
				}
				break;
			case M_ELLIPSE:
				memset(temp_buffer, 0, image_size<<2), use_temp_buffer=true;
				draw_ellipse_mouse(temp_buffer, start_mx, mx, start_my, my, 0xFF000000|color, 0xFF000000|color2);
				break;
			case M_ROUNDED_RECT:
				memset(temp_buffer, 0, image_size<<2), use_temp_buffer=true;
				draw_roundrect_mouse(temp_buffer, start_mx, mx, start_my, my, 0xFF000000|color, 0xFF000000|color2);
				break;
			}
		}
		prof_add("draw");

		int x2s=hscrollbar?w-scrollbarwidth*vscrollbar:x1+shift(iw-spx, logzoom), y2s=vscrollbar?h-74-scrollbarwidth*hscrollbar:y1+shift(ih-spy, logzoom);
		if(vscrollbar&&x2s>w-scrollbarwidth)
			x2s=w-scrollbarwidth;
		if(spx+(x2s-x1)/zoom>iw)
			x2s=x1+shift(iw-spx, logzoom);
		if(hscrollbar&&y2s>h-74-scrollbarwidth)
			y2s=h-74-scrollbarwidth;
		if(spy+(y2s-y1)/zoom>ih)
			y2s=y1+shift(ih-spy, logzoom);

		int gsize=shift(8, logzoom);//16						//draw checkboard
		if(gsize<4)
			gsize=4;
		if(gsize>128)
			gsize=128;
		int gsize2=gsize<<1;
		const int cb_color=0xFFFFFF;
		HPEN hPen=CreatePen(PS_SOLID, 1, cb_color);
		HBRUSH hBrush=CreateSolidBrush(cb_color);
		hPen=(HPEN__*)SelectObject(ghMemDC, hPen), hBrush=(HBRUSH__*)SelectObject(ghMemDC, hBrush);
		HRGN hRgn=CreateRectRgn(x1, y1, x2s, y2s);
		SelectClipRgn(ghMemDC, hRgn);
		for(int ky=y1;ky<y2s;ky+=gsize2)
		{
			for(int kx=x1;kx<x2s;kx+=gsize2)
			{
				Rectangle(ghMemDC, kx, ky+gsize, kx+gsize, ky+gsize2);
				Rectangle(ghMemDC, kx+gsize, ky, kx+gsize2, ky+gsize);
			}
		}
		SelectClipRgn(ghMemDC, 0);
		DeleteObject(hRgn);
		hPen=(HPEN__*)SelectObject(ghMemDC, hPen), hBrush=(HBRUSH__*)SelectObject(ghMemDC, hBrush);
		DeleteObject(hPen), DeleteObject(hBrush);
		prof_add("grid");

		if(!use_temp_buffer||currentmode!=M_FREE_SELECTION)
			blend_zoomed(image, x1, x2s, y1, y2s);
		prof_add("blend image");
		if((currentmode==M_ERASER||currentmode==M_BRUSH)&&!use_temp_buffer)//draw brush preview at cursor
		{
			memset(temp_buffer, 0, image_size<<2), use_temp_buffer=true;
			auto b=resources::brushes+(currentmode==M_ERASER?erasertype:brushtype);
			Point i;
			screen2image(mx, my, i.x, i.y);
			int c=currentmode==M_BRUSH!=mb[1]?primarycolor:secondarycolor;
			for(int k=0, ky=i.y+b->yoffset;k<b->ysize;++k, ++ky)
				for(int kx=b->bounds[k<<1], kxEnd=b->bounds[k<<1|1];kx<=kxEnd;++kx)
					if(!icheck(i.x+kx, ky))
						temp_buffer[iw*ky+i.x+kx]=c;
		}
		if(use_temp_buffer)
		{
			if(currentmode==M_FREE_SELECTION)//no blend, assign
			{
				for(int ky=y1;ky<y2;++ky)
				{
					for(int kx=x1;kx<x2;++kx)
					{
						int ix=spx+shift(kx-x1, -logzoom), iy=spy+shift(ky-y1, -logzoom);
						if(icheck(ix, iy))//robust
							break;//
						rgb[w*ky+kx]=temp_buffer[iw*iy+ix];
					}
				}
			}
			else
				blend_zoomed(temp_buffer, x1, x2s, y1, y2s);
		}
		if(currentmode!=M_CURVE&&currentmode!=M_POLYGON)
			use_temp_buffer=false;
		prof_add("blend temp_buffer");

		if(sel_start!=sel_end&&drag!=D_DRAW)//draw selection buffer
		{
			int ix1=selpos.x, iy1=selpos.y, ix2=selpos.x+sw, iy2=selpos.y+sh;//image coordinates
			if(ix1<0)
				ix1=0;
			if(iy1<0)
				iy1=0;
			if(ix2>iw)
				ix2=iw;
			if(iy2>ih)
				iy2=ih;
			int sx1, sy1, sx2, sy2;//screen coordinates
			image2screen(ix1, iy1, sx1, sy1);
			image2screen(ix2, iy2, sx2, sy2);
			if(sy1<0)//avoid CRASH
				sy1=0;//
			if(sx1<0)//
				sx1=0;//
			if(sx2>x2)
				sx2=x2;
			if(sy2>y2)
				sy2=y2;
			if(selection_free)
			{
				bool opaque=selection_transparency!=TRANSPARENT;
				for(int ky=sy1;ky<sy2;++ky)
				{
					for(int kx=sx1;kx<sx2;++kx)
					{
						int ix=spx-selpos.x+shift(kx-x1, -logzoom), iy=spy-selpos.y+shift(ky-y1, -logzoom);
						int sel_idx=sw*iy+ix;
						auto &src=sel_buffer[sel_idx];
						if(sel_mask[sel_idx]&&(opaque||src!=secondarycolor))
							rgb[w*ky+kx]=src;
					}
				}
			}
			else
			{
				if(selection_transparency==TRANSPARENT)
				{
					for(int ky=sy1;ky<sy2;++ky)
					{
						for(int kx=sx1;kx<sx2;++kx)
						{
							int ix=spx-selpos.x+shift(kx-x1, -logzoom), iy=spy-selpos.y+shift(ky-y1, -logzoom);
							auto &src=sel_buffer[sw*iy+ix];
							if(src!=secondarycolor)
								rgb[w*ky+kx]=src;
						}
					}
				}
				else//opaque selection
				{
					for(int ky=sy1;ky<sy2;++ky)
					{
						for(int kx=sx1;kx<sx2;++kx)
						{
							int ix=spx-selpos.x+shift(kx-x1, -logzoom), iy=spy-selpos.y+shift(ky-y1, -logzoom);
							rgb[w*ky+kx]=sel_buffer[sw*iy+ix];
						}
					}
				}
			}
		}
		if(showgrid&&logzoom>=2)//show grid, at pixel size 4+
		{
			int gridcolors[]={0x808080, 0xC0C0C0};
			for(int ky=y1, pxsize=1<<logzoom;ky<y2s;ky+=pxsize)//draw vertical grid lines
				h_line_alt(x1, x2s, ky, gridcolors);
			for(int kx=x1, pxsize=1<<logzoom;kx<x2s;kx+=pxsize)//draw vertical grid lines
				v_line_alt(kx, y1, y2s, gridcolors);
		}
		if(currentmode==M_TEXT)
			draw_selection_rectangle(text_start, text_end, text_s1, text_s2, x1, x2, y1, y2, 2);
		draw_selection_rectangle(sel_start, sel_end, sel_s1, sel_s2, x1, x2, y1, y2, 0);
		if(currentmode==M_MAGNIFIER)//draw magnifier highlight
		{
			int xend=minimum(x1+iw, x2), yend=minimum(y1+ih, y2);
		//	if(mx>=x1&&mx<x2&&my>=y1&&my<y2)
			if(!logzoom&&magnifier_level&&mx>=x1&&mx<xend&&my>=y1&&my<yend)//TODO: magnifier rectangle for arbitrary zoom
			{
				int dx=x2-x1, dy=y2-y1;
				int rx=dx>>(magnifier_level+1), ry=dy>>(magnifier_level+1);//reach
				int x0=mx, y0=my;
				if(x0<x1+rx)
					x0=x1+rx;
				else if(x0>xend-rx)
					x0=xend-rx;
				if(y0<y1+ry)
					y0=y1+ry;
				else if(y0>yend-ry)
					y0=yend-ry;
				int hx1=x0-rx, hx2=x0+rx, hy1=y0-ry, hy2=y0+ry;//highlight
				if(hx1<x1)
					hx1=x1;
				if(hx2>xend-1)
					hx2=xend-1;
				if(hy1<y1)
					hy1=y1;
				if(hy2>yend-1)
					hy2=yend-1;
				h_line_add(hx1, hx2+1, hy1, 128);
				h_line_add(hx1, hx2+1, hy2, 128);
				v_line_add(hx1, hy1+1, hy2, 128);
				v_line_add(hx2, hy1+1, hy2, 128);
			}
		}
		prof_add("show image");
		prev_mx=mx, prev_my=my;
	}
	prof_print();
	prof.clear();
#ifndef RELEASE
	HFONT hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
	hFont=(HFONT)SelectObject(ghMemDC, hFont);
	int fontH=13;
	int size=0;
	for(int k=0, nv=history.size();k<nv;++k)
		size+=history[k].iw*history[k].ih;
	GUIPrint(ghMemDC, w>>1, h>>1, "hpos=%d/%d,  %fMB", histpos, history.size(), float(size<<2)/1048576);//
	for(int k=0, kx=w>>1, ky=(h>>1)+fontH, kEnd=history.size();k<kEnd;++k)//
	{
		GUIPrint(ghMemDC, kx, ky, "0x%08X", (int)history[k].buffer);//
		ky+=fontH;
		if(ky>=h-fontH)
			ky=(h>>1)+fontH, kx+=70;

	}
	hFont=(HFONT)SelectObject(ghMemDC, hFont);
#endif
	BitBlt(ghDC, 0, 0, w, h, ghMemDC, 0, 0, SRCCOPY);
	if(broken)
	{
		if(broken_line==-1)
			GUIPrint(ghDC, w>>1, (h>>1)-16, "ERROR: 0x%08X(%d)", broken, broken);
		else
			GUIPrint(ghDC, w>>1, (h>>1)-16, "ERROR: 0x%08X(%d), line %d", broken, broken, broken_line);
	}
}
#ifdef MOUSE_POLLING_TEST
void			render2()//mouse polling test
{
	const int npositions=50;
	static Point positions[npositions];
	static int p_idx=0;

	positions[p_idx].set(mx, my);//sample mouse position

	_LARGE_INTEGER li;//measure elapsed time
	QueryPerformanceFrequency(&li);
	long long freq=li.QuadPart;
	QueryPerformanceCounter(&li);
	static long long t1=0;
	double elapsed=double(li.QuadPart-t1)/freq;
	t1=li.QuadPart;

	memset(rgb, 0xFF, rgbn<<2);
	for(int k=0, kx=0, ky=0;k<npositions;++k)//write coordinates
	{
		auto &pk=positions[k];
		GUIPrint(ghMemDC, kx, ky<<4, "(%d, %d)%s", pk.x, pk.y, k==p_idx?" <":"");
		++ky;
		if(ky<<4>=h-32)
			ky=0, kx+=100;
	}
	GUIPrint(ghMemDC, 0, h-16, "fps=%lf, T=%lfms", 1/elapsed, 1000*elapsed);
	for(int k=0;k<npositions;++k)//draw trail
	{
		auto &pk=positions[k];
		if(!check(pk.x, pk.y))
			rgb[w*pk.y+pk.x]=0;
	}
	p_idx=(p_idx+1)%npositions;//increment idx
	BitBlt(ghDC, 0, 0, w, h, ghMemDC, 0, 0, SRCCOPY);
}
#endif
void			load_frame(int frame_number)
{
	std::wstring framepath=workfolder+framenames[frame_number];
	if(workspace_has_quotes)
		framepath+=L'\"';
	
	stbi_convert_wchar_to_utf8(g_buf, g_buf_size, framepath.c_str());
	unsigned char *original_image=stbi_load(g_buf, &iw, &ih, &nchannels, 0);
	image_size=iw*ih;
	image=hist_start(iw, ih);
//	image=(int*)realloc(image, image_size<<2);
	switch(nchannels)//set the 4 nchannels & swap red/blue channels
	{
	case 1:
		for(int k=0;k<image_size;++k)
		{
			auto p=(unsigned char*)(image+k);
			p[0]=original_image[k];
			p[1]=original_image[k];
			p[2]=original_image[k];
			p[3]=0xFF;
		}
		break;
	case 2:
		for(int k=0, k2=0;k<image_size;++k, k2+=2)
		{
			auto p=(unsigned char*)(image+k);
			p[0]=original_image[k2];
			p[1]=original_image[k2];
			p[2]=original_image[k2];
			p[3]=original_image[k2+1];
			//p[0]=0;
			//p[1]=original_image[k2+1];
			//p[2]=original_image[k2];
			//p[3]=0;
		}
		nchannels=4;//
		break;
	case 3:
		for(int k=0, k2=0;k<image_size;++k, k2+=3)
		{
			auto p=(unsigned char*)(image+k);
			p[0]=original_image[k2+2];
			p[1]=original_image[k2+1];
			p[2]=original_image[k2];
			p[3]=0xFF;
		}
		break;
	case 4:
		for(int k=0;k<image_size;++k)
		{
			auto p=(unsigned char*)(image+k), p2=(unsigned char*)((int*)original_image+k);
			p[0]=p2[2];
			p[1]=p2[1];
			p[2]=p2[0];
			p[3]=p2[3];
		}
	//	memcpy(image, original_image, image_size<<2);
		break;
	}
	stbi_image_free(original_image);
	original_image=nullptr;

	//Gdiplus::Bitmap bmp(framepath.c_str());
	//iw=bmp.GetWidth(), ih=bmp.GetHeight(), image_size=iw*ih;
	//image=(int*)realloc(image, image_size<<2);
	//Gdiplus::BitmapData data;
	//Gdiplus::Rect rect(0, 0, iw, ih);
	//bmp.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
	//memcpy(image, data.Scan0, image_size<<2);
	//bmp.UnlockBits(&data);
}
void			save_frame(int frame_number)
{
	std::wstring framepath=workfolder+framenames[frame_number];
	if(workspace_has_quotes)
		framepath+=L'\"';

	int *image2=(int*)malloc(image_size<<2);
	for(int k=0;k<image_size;++k)
		image2[k]=swap_rb(image[k]);
	std::vector<byte> png;
	unsigned error=lodepng::encode(png, (byte*)image2, iw, ih);
	write_binary(framepath.c_str(), png);//possibly overwrite frame
	free(image2);
}
void			assign_thumbnail(int *thumbnail)
{
	for(int ky=0;ky<th_h;++ky)//no averaging
	{
		int iy=ky*ih/th_h;
		for(int kx=0;kx<th_w;++kx)
		{
			int ix=kx*iw/th_w;
			thumbnail[th_w*ky+kx]=swap_rb(image[iw*iy+ix]);
		}
	}
}
void			load_thumbnails()
{
	std::vector<byte> png, bitmap;
	for(int kf=0;kf<nframes;++kf)
	{
		std::wstring framepath=workfolder+framenames[kf];
		if(workspace_has_quotes)
			framepath+=L'\"';

		unsigned iw, ih;
		read_binary(framepath.c_str(), png);
		bitmap.clear();
		lodepng::decode(bitmap, iw, ih, &png[0], png.size());//5th overload
		int *image=(int*)&bitmap[0];

		auto &thumbnail=thumbnails[kf];
		if(!thumbnail)
			thumbnail=(int*)malloc(th_w*th_h<<2);
		assign_thumbnail(thumbnail);
	}
}
void			get_name_from_path(std::wstring const &path, std::wstring &name)
{
	int start=-1;
	for(int k=path.size()-1;k>=0;--k)
	{
		if(filepath[k]==L'/'||filepath[k]==L'\\')
		{
			start=k+1;
			break;
		}
	}
	if(start==-1)
		start=0;//unreachable
	filename.assign(filepath.begin()+start, filepath.end());
}
void			open_media(std::wstring filepath_u16)//pass by copy
{
	if(filepath_u16.find(L' ')!=std::string::npos)
	{
		if(filepath_u16[0]!=doublequote)
			filepath_u16.insert(filepath_u16.begin(), doublequote);
		if(*filepath_u16.rbegin()!=doublequote)
			filepath_u16.push_back(doublequote);
	}
	std::string result;
	
	log_start(LL_OPERATIONS);
	writeworkfolder2wbuf();
	if(workfolder.size())
		log(LL_OPERATIONS,
			"About to delete the old folder:\n"
			"\t%ls\n"
			"and extract frame(s) to the new folder:\n"
			"\t%ls\n", workfolder.c_str(), g_wbuf);
	else
		log(LL_OPERATIONS,
			"About to extract frame(s) to the new folder:\n"
			"\t%ls\n", g_wbuf);
	log_pause(LL_OPERATIONS);

	delete_workfolder();			//delete old work folder if exists
	create_workfolder_from_wbuf();
	log(LL_PROGRESS, "Extracting frame(s)...");
	exec_FFmpeg_command(L"ffmpeg", L"-i "+filepath_u16+L" "+workfolder+L"%08d."+imageformats[tempformat]+(workspace_has_quotes?L"\"":L""), result);				//extract video frames
	log(LL_PROGRESS, "\tDone.\n");

	//list frame names	//https://stackoverflow.com/questions/2802188/file-count-in-a-directory-using-c
	framenames.clear();
	_WIN32_FIND_DATAW data;
	workfolder.push_back(L'*');
	if(workspace_has_quotes)
		workfolder.push_back(L'\"');
	void *hSearch=FindFirstFileW(workfolder.c_str(), &data);
	workfolder.pop_back();
	if(workspace_has_quotes)
		workfolder.pop_back();
	if(hSearch==INVALID_HANDLE_VALUE)
		formatted_error(L"FindFirstFileW", __LINE__);
	//	messagebox(ghWnd, L"Error", L"FindFirstFileW Returned invalid handle. GetLastError() returned %d. Line %d", GetLastError(), __LINE__);
	//	printf("Failed to find path: %ls", workfolder.c_str());
	else//Managed to locate and create a handle to that folder.
	{
		do
		{
			std::wstring name=data.cFileName;
			int k=maximum(0, name.size()-4);
			if(!(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&compare_string_caseinsensitive(name, k, L".PNG", 4))
				framenames.push_back(data.cFileName);
		}
		while(FindNextFileW(hSearch, &data));
		FindClose(hSearch);
	}
	nframes=framenames.size();
	for(int kt=0, size=thumbnails.size();kt<size;++kt)
		if(thumbnails[kt])
			free(thumbnails[kt]), thumbnails[kt]=nullptr;
	thumbnails.resize(nframes);
	animated=nframes>1;

	if(animated)//get framerate
	{
		log(LL_PROGRESS, "Querying framerate...");
		exec_FFmpeg_command(L"ffprobe", L"-v 0 -of csv=p=0 -select_streams v:0 -show_entries stream=r_frame_rate "+filepath_u16, result);//query framerate num & den https://askubuntu.com/questions/110264/how-to-find-frames-per-second-of-any-video-file
		int k=0, size=result.size();
		framerate_num=read_unsigned_int(result, k);
		if(k<size&&result[k]=='/')
		{
			++k;
			framerate_den=read_unsigned_int(result, k);
		}
		else
			framerate_den=1;
		log(LL_PROGRESS, "\tDone: %d/%d=%lf\n", framerate_num, framerate_den, double(framerate_num)/framerate_den);
	}
	
	log(LL_PROGRESS, "Opening first frame...");
	current_frame=0;//load first frame
	load_frame(current_frame);
	log(LL_PROGRESS, "\tDone.\n");

	const int thumbnail_size=90;//calculate thumbnails size
	int maxd=maximum(iw, ih);
	if(maxd>thumbnail_size)
		th_w=int(iw*(double)thumbnail_size/maxd), th_h=int(ih*(double)thumbnail_size/maxd);
	else
		th_w=iw, th_h=ih;
	thbox_h=(10+16+th_h)*nframes+10;
	log(LL_PROGRESS, "Loading thumbnails...");
	if(animated)
		load_thumbnails();
	log(LL_PROGRESS, "\tDone.\n");

	modified=false;
	log_pause(LL_PROGRESS);
	log_end();
}
void			open_media()
{
	wchar_t szFile[MAX_PATH]={'\0'};
	tagOFNW ofn={sizeof(ofn), ghWnd, 0, L"All files(*.*)\0*.*\0", 0, 0, 1, szFile, sizeof(szFile), 0, 0, 0, 0, OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST, 0, 0, 0, 0, 0, 0};
	if(GetOpenFileNameW(&ofn))
	{
		filepath=ofn.lpstrFile;
		//std::wstring filepath_u16=ofn.lpstrFile;
		//int length=stbi_convert_wchar_to_utf8(g_buf, g_buf_size, ofn.lpstrFile);
		//filepath=g_buf;//utf8
		int psize=filepath.size();
		get_name_from_path(filepath, filename);
		
		if(filepath.size())
		{
			open_media(filepath);

			SetWindowTextW(ghWnd, (filepath+L" - Paint++").c_str());
			render();
		}
	}
}
void			save_media(std::wstring const &outputpath_u16)
{
	std::wstring outputpath_cmd=outputpath_u16;
	bool has_space=false;
	int size=outputpath_cmd.size();
	for(int k=0;k<size;++k)
	{
		if(outputpath_cmd[k]==L' ')
		{
			has_space=true;
			break;
		}
	}
	if(has_space)
	{
		if(outputpath_cmd[0]!=L'\"')
			outputpath_cmd.insert(outputpath_cmd.begin(), L'\"');
		if(outputpath_cmd[size-1]!=L'\"')
			outputpath_cmd.push_back(L'\"');
	}
	if(file_exists(outputpath_u16))
		_wremove(outputpath_u16.c_str());

//	std::wstring framename;
	if((unsigned)current_frame>=framenames.size()||!workfolder.size())//workfolder wasn't created yet, save frame as 00000001.PNG
	{
		writeworkfolder2wbuf();
		create_workfolder_from_wbuf();
		framenames.push_back(L"00000001.PNG"), nframes=framenames.size(), current_frame=0;
	}

	//overwrite frame if modified, use FFmpeg
	std::wstring framepath=workfolder+framenames[current_frame]+(workspace_has_quotes?L"\"":L"");
	if(modified)
		save_frame(current_frame);
	std::string result;
	if(animated)
	{
		if(!framerate_num)
			framerate_num=25;
		if(!framerate_den)
			framerate_den=1;

		//swprintf_s(g_wbuf, g_buf_size, L"%%d");//
		swprintf_s(g_wbuf, g_buf_size, L" -framerate %lf -i %ls%%08d.PNG -plays 0 %ls", (double)framerate_num/framerate_den, workfolder.c_str(), outputpath_cmd.c_str());
	//	swprintf_s(g_wbuf, g_buf_size, L" -framerate %lf -i %ls%%08d.PNG %ls", (double)framerate_num/framerate_den, workfolder.c_str(), outputpath_cmd.c_str());//doesn't loop
	//	swprintf_s(g_wbuf, g_buf_size, L" -framerate %lf -i %ls%%08d.PNG \"D:\\Share Box\\!\\LOL.GIF\"", (double)framerate_num/framerate_den, workfolder.c_str());
		exec_FFmpeg_command(L"FFmpeg", g_wbuf, result);

		//exec_FFmpeg_command(L"FFmpeg",
		//	L" -i "+workfolder+L"%08d.PNG"+(workspace_has_quotes?L"\"":L"")
		//	+L" -vf palettegen "+workfolder+L"palette.png"+(workspace_has_quotes?L"\"":L""), result);
		//swprintf_s(g_wbuf, g_buf_size, L" -framerate %lf -i %ls\%08d.PNG -i %lspalette.PNG %ls", (double)framerate_num/framerate_den, workfolder.c_str(), workfolder.c_str(), outputpath_cmd.c_str());
		//exec_FFmpeg_command(L"FFmpeg", g_wbuf, result);

	//	exec_FFmpeg_command(L"FFmpeg", L" -i "+workfolder+L"%08d.PNG"+(workspace_has_quotes?L"\" ":L" ")+outputpath_cmd, result);
	}
	else
		exec_FFmpeg_command(L"FFmpeg", L" -i "+framepath+L' '+outputpath_cmd, result);
	const char failmsg[]="Conversion failed!\r\n";
	const int fm_size=sizeof(failmsg);
	bool conversion_failed=true;
	for(int k=fm_size-2, k2=result.size()-1;k>=0&&k2>=0;--k, --k2)//compare strings backwards
	{
		if(result[k2]!=failmsg[k])
		{
			conversion_failed=false;
			break;
		}
	}
	if(conversion_failed)
	{
		log_start(LL_OPERATIONS);
		log(LL_OPERATIONS, result.c_str());
		log(LL_OPERATIONS, "\n");
		log_pause(LL_OPERATIONS);
		log_end();
	}
	//int length=stbi_convert_wchar_to_utf8(g_buf, g_buf_size, ofn.lpstrFile);//not used
	//if(length)
	//{
	//	if(!animated&&ofn.nFilterIndex>0&&ofn.nFilterIndex<=5)//PNG, GIF, JPG, BMP, TIF: just Gdiplus
	//	{
	//		std::vector<byte> png;
	//		unsigned error=lodepng::encode(png, (byte*)image, iw, ih);
	//		//static const wchar_t *encodernames[]=
	//		//{
	//		//	L"image/png",
	//		//	L"image/gif",
	//		//	L"image/jpeg",
	//		//	L"image/bmp",
	//		//	L"image/tiff",//image/bmp, image/jpeg, image/gif, image/tiff, image/png
	//		//};
	//		//CLSID encoderClsid;//https://docs.microsoft.com/en-us/windows/win32/gdiplus/-gdiplus-converting-a-bmp-image-to-a-png-image-use
	//		//int success=GetEncoderClsid(encodernames[ofn.nFilterIndex], &encoderClsid);
	//		//if(success)
	//		//{
	//		//	Gdiplus::Bitmap bmp(iw, ih, iw<<2, PixelFormat32bppARGB, (unsigned char*)image);
	//		//	Gdiplus::Status status=bmp.Save(g_wbuf, &encoderClsid, nullptr);
	//		//	if(status)
	//		//	{
	//		//		swprintf_s(g_wbuf, g_buf_size, L"Bitmap::Save() -> %d", status);
	//		//		error_exit(g_wbuf, __LINE__);
	//		//	}
	//		//}
	//	}
	//	else//animated or other format: save frame if modified, then use FFmpeg
	//	{
	//	}
	//}
	SetWindowTextW(ghWnd, (outputpath_u16+L" - Paint++").c_str());
}
bool			save_media_as()
{
	wchar_t filetitle[]=L"Untitled.PNG";
	memcpy(g_wbuf, filetitle, sizeof filetitle);
	tagOFNW ofn=
	{
		sizeof(tagOFNW), ghWnd, 0,
		L"PNG (*.PNG)\0*.PNG\0"//lpstrFilter
		L"Animated PNG (*.APNG)\0*.APNG\0"
		L"GIF (*.GIF)\0*.GIF\0"
		L"JPEG (*.JPG;*.JPEG;*.JPE;*.JFIF)\0*.JPG\0"
		L"24-bit Bitmap (*.bmp;*.dib)\0*.bmp\0"
		L"TIFF (*.TIF;*.TIFF)\0*.TIF\0",
		0, 0,//custom filter
		1,//filter index: 0 is custom filter, 1 is first, ...
					
		g_wbuf, g_buf_size,//file (output)
					
		0, 0,//file title
		0, 0, OFN_ENABLESIZING|OFN_EXPLORER|OFN_NOTESTFILECREATE|OFN_PATHMUSTEXIST|OFN_EXTENSIONDIFFERENT|OFN_OVERWRITEPROMPT,//flags

		0,//nFileOffset
		8,//nFileExtension
		L"PNG",//default extension
					
		0, 0, 0
	};
	if(GetSaveFileNameW(&ofn))
	{
		filepath=ofn.lpstrFile;
		get_name_from_path(filepath, filename);
	//	std::wstring outputpath_u16=ofn.lpstrFile, outputpath_cmd=outputpath_u16;
		save_media(filepath);
		return true;
	}
	return false;
}
//struct			MyDropTarget:public IDropTarget
//{
//	long __stdcall QueryInterface(/*[in]*/ IID const &riid, /*[iid_is][out]*/ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject){return 0;}
//	unsigned long __stdcall AddRef(){return 0;}
//	unsigned long __stdcall Release(){return 0;}
//	long __stdcall DragEnter(/*[unique][in]*/ __RPC__in_opt IDataObject *pDataObj, /*[in]*/ DWORD grfKeyState, /*[in]*/ POINTL pt, /*[out][in]*/ __RPC__inout DWORD *pdwEffect){return 0;}
//        
//	long __stdcall DragOver(/*[in]*/ DWORD grfKeyState, /*[in]*/ POINTL pt, /*[out][in]*/ __RPC__inout DWORD *pdwEffect){return 0;}
//	long __stdcall DragLeave(void){return 0;}
//	long __stdcall Drop(/*[unique][in]*/ __RPC__in_opt IDataObject *pDataObj, /*[in]*/ DWORD grfKeyState, /*[in]*/ POINTL pt, /*[out][in]*/ __RPC__inout DWORD *pdwEffect){return 0;}
//} mdt;
void			resize_textgui()
{
	movewindow_c(hFontcombobox, 650, h-71, 200, 500, true);
//	movewindow_c(hFontcombobox, 650, h-72, 200, 500, true);
//	movewindow_c(hFontcombobox, 650, h-70, 200, 500, true);

	movewindow_c(hFontsizecb, 650, h-47, 50, 250, true);
//	movewindow_c(hFontsizecb, 650, h-49, 50, 250, true);
}
void			set_sizescombobox(int font_idx)
{
	static bool first_time=true;
	auto &font0=fonts[::font_idx], &font=fonts[font_idx];
	if(first_time||font0.sizes.size()||font.sizes.size())
	{
		first_time=false;
		const int *sizes=nullptr;
		int sizes_size=0;
		if(font.sizes.size())//bitmap font
			sizes=&font.sizes[0], sizes_size=font.sizes.size();
		else//true type font
			sizes=truetypesizes, sizes_size=truetypesizes_size;

		int nsizes=SendMessageW(hFontsizecb, CB_GETCOUNT, 0, 0);//clear sizes combobox
		for(int k=0;k<nsizes;++k)
			SendMessageW(hFontsizecb, CB_DELETESTRING, 0, 0);
		for(int k=0;k<sizes_size;++k)
		{
			swprintf_s(g_wbuf, g_buf_size, L"%d", sizes[k]);
			SendMessageW(hFontsizecb, CB_ADDSTRING, 0, (LPARAM)g_wbuf);
		}

		int result=std::find(sizes, sizes+sizes_size, font_size)-sizes;//find current fontsize
		if(result<sizes_size)//found
			SendMessageW(hFontsizecb, CB_SETCURSEL, result, 0);
		else if(sizes_size)//not found, find closest number in set
		{
			int min_dist=abs(font_size-sizes[0]);
			font_size=sizes[0];
			for(int k=1;k<sizes_size;++k)
			{
				int dist=abs(font_size-sizes[k]);
				if(min_dist>dist)
					font_size=sizes[k], min_dist=dist;
			}
		}
	}
//	::font_idx=font_idx;

//	swprintf_s(g_wbuf, g_buf_size, L"%d", font_size);
//	SendMessageW(hFontsizecb, CB_FINDSTRING, -1, (LPARAM)g_wbuf);

	//font_size=SendMessageW(hFontsizecb, CB_SETCURSEL, 0, 0);
	//font_size=sizes[font_size];
}
void			sample_font(int &font_idx, int &font_size)
{
	int idx2=SendMessageW(hFontcombobox, CB_GETCURSEL, 0, 0);
	if(idx2!=font_idx)
	{
		set_sizescombobox(idx2);
		font_idx=idx2;
	}
	idx2=SendMessageW(hFontsizecb, CB_GETCURSEL, 0, 0);
	SendMessageW(hFontsizecb, CB_GETLBTEXT, idx2, (LPARAM)g_wbuf);
	font_size=_wtoi(g_wbuf);
}
void			remove_textbox()
{
	text_start.setzero(), text_end.setzero();
	text_s1.setzero(), text_s2.setzero();
	textpos.setzero();
	ShowWindow(hTextbox, SW_HIDE);
}
void			exit_textmode()
{
	remove_textbox();
	replace_colorbarcontents(CS_NONE);
}
void			change_font(int font_idx, int font_size)
{
	auto &font=fonts[font_idx];
	auto &lf=font.lf;
	if(hFont)
		DeleteObject(hFont);
	int nHeight=-MulDiv(font_size, GetDeviceCaps(ghMemDC, LOGPIXELSY), 72);
	hFont=CreateFontW(nHeight, 0, lf.lfEscapement, lf.lfOrientation, lf.lfWeight, italic, underline, lf.lfStrikeOut, lf.lfCharSet, lf.lfOutPrecision, lf.lfClipPrecision, lf.lfQuality, lf.lfPitchAndFamily, lf.lfFaceName);
//	hFont=CreateFontW(lf.lfHeight, lf.lfWidth, lf.lfEscapement, lf.lfOrientation, lf.lfWeight, italic, underline, lf.lfStrikeOut, lf.lfCharSet, lf.lfOutPrecision, lf.lfClipPrecision, lf.lfQuality, lf.lfPitchAndFamily, lf.lfFaceName);

	SendMessageW(hTextbox, WM_SETFONT, (WPARAM)hFont, true);
}
void			clear_textbox(){SetWindowTextW(hTextbox, nullptr);}
void			print_text()
{
	int size=SendMessageW(hTextbox, WM_GETTEXTLENGTH, 0, 0);
	if(size>0)
	{
		std::wstring text(size+1, L'\0');
		SendMessageW(hTextbox, WM_GETTEXT, text.size(), (LPARAM)&text[0]);
	//	TabbedTextOutW(ghDC, 0, 0, text.c_str(), text.size()-1, 0, nullptr, 0);//
		clear_textbox();

		//auto &font=fonts[font_idx];
		//auto &lf=font.lf;
		//HFONT hFont=CreateFontW(lf.lfHeight, lf.lfWidth, lf.lfEscapement, lf.lfOrientation, lf.lfWeight, italic, underline, lf.lfStrikeOut, lf.lfCharSet, lf.lfOutPrecision, lf.lfClipPrecision, lf.lfQuality, lf.lfPitchAndFamily, lf.lfFaceName);
		BITMAPINFO bmpInfo={{sizeof(BITMAPINFOHEADER), iw, -ih, 1, 32, BI_RGB, 0, 0, 0, 0, 0}};
		int *rgb2=nullptr;
		hBitmap=CreateDIBSection(0, &bmpInfo, DIB_RGB_COLORS, (void**)&rgb2, 0, 0);
		hBitmap=(HBITMAP)SelectObject(ghMemDC, hBitmap);
		hFont=(HFONT)SelectObject(ghMemDC, hFont);
		const int alphamask=0xFF000000, colormask=0x00FFFFFF;
		memcpy(rgb2, image, image_size<<2);//copy image for proper anti-aliasing
		for(int k=0;k<image_size;++k)//set original alpha (reverse alpha)
			rgb2[k]|=alphamask;

		Point i1, i2;
		screen2image(text_s1.x, text_s1.y, i1.x, i1.y);
		screen2image(text_s2.x, text_s2.y, i2.x, i2.y);
		RECT r={i1.x, i1.y, i2.x, i2.y};//left top right bottom
	//	RECT r={0, 0, iw, ih};//
		int txtcolor=SetTextColor(ghMemDC, swap_rb(primarycolor&colormask)), bkmode=SetBkMode(ghMemDC, selection_transparency), bkcolor=SetBkColor(ghMemDC, swap_rb(secondarycolor&colormask));
		DrawTextW(ghMemDC, text.c_str(), size, &r, DT_NOCLIP);
		SetTextColor(ghMemDC, txtcolor), SetBkMode(ghMemDC, bkmode), SetBkColor(ghMemDC, bkcolor);

		hist_premodify(image, iw, ih);//copy printed text to image
		for(int k=0;k<image_size;++k)//reverse alpha algorithm
		{
			int color=rgb2[k];
			if(!(color&alphamask))//was modified by DrawTextW
			{
				if(color==(primarycolor&colormask))
					image[k]=primarycolor;
				else if(color==(secondarycolor&colormask))
					image[k]=secondarycolor;
				else//anti-aliasing colors
					image[k]=alphamask|color;
			}
		}
		
		hBitmap=(HBITMAP)SelectObject(ghMemDC, hBitmap);
		hFont=(HFONT)SelectObject(ghMemDC, hFont);
		DeleteObject(hBitmap);
	//	DeleteObject(hFont);
	}
	text_start.setzero(), text_end.setzero();
	text_s1.setzero(), text_s2.setzero();
	textpos.setzero();
	move_textbox();
}
WNDPROC			oldEditProc=nullptr;
long			__stdcall EditProc(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	switch(message)
	{
	case WM_CHAR:
		if(wParam==1)
		{
			SendMessageW(hWnd, EM_SETSEL, 0, -1);
			return 1;
		}
	//case WM_LBUTTONDOWN:
	//case WM_RBUTTONDOWN:
	//case WM_KEYDOWN:case WM_SYSKEYDOWN:
	//	int result=CallWindowProcW(oldEditProc, hWnd, message, wParam, lParam);
	//	focus=F_TEXTBOX;
	//	SetFocus(ghWnd);
	//	return result;
	}
	return CallWindowProcW(oldEditProc, hWnd, message, wParam, lParam);
}
bool			editcopy()
{
	if(sel_start!=sel_end)
	{
		int size=sizeof(BITMAPINFO)+(sw*sh<<2);
		char *clipboard=(char*)LocalAlloc(LMEM_FIXED, size);
		BITMAPINFO bmi={{sizeof(BITMAPINFOHEADER), sw, -sh, 1, 32, BI_RGB, 0, 0, 0, 0, 0}};
		memcpy(clipboard, &bmi, sizeof BITMAPINFO);
		memcpy(clipboard+sizeof BITMAPINFO, sel_buffer, sw*sh<<2);
		int success=OpenClipboard(ghWnd);
		if(!success)
		{
			formatted_error(L"OpenClipboard", __LINE__);
			return false;
		}
		EmptyClipboard();
		SetClipboardData(CF_DIB, (void*)clipboard);
		CloseClipboard();
		return true;
	}
	return false;
}
void			editpaste()
{
	unsigned clipboardformats[]={CF_DIB, CF_DIBV5, CF_BITMAP, CF_TIFF, CF_METAFILEPICT};
	int format=GetPriorityClipboardFormat(clipboardformats, sizeof clipboardformats>>2);
	if(format>0)
	{
		int success=OpenClipboard(ghWnd);
		if(!success)
		{
			formatted_error(L"OpenClipboard", __LINE__);
			return;
		}
		switch(format)
		{
		case CF_DIB:
			{
				void *hData=GetClipboardData(CF_DIB);
				int size=GlobalSize(hData);
				BITMAPINFO *bmi=(BITMAPINFO*)GlobalLock(hData);
				if(bmi->bmiHeader.biCompression==BI_RGB||bmi->bmiHeader.biCompression==BI_BITFIELDS)//uncompressed
				{
					int bw=bmi->bmiHeader.biWidth, bh=abs(bmi->bmiHeader.biHeight);
				//	int ncolors=bmi->bmiHeader.biClrUsed?bmi->bmiHeader.biClrUsed:1, bitdepth=bmi->bmiHeader.biBitCount;
					byte *data=(byte*)bmi+bmi->bmiHeader.biSize;
					int data_size=size-bmi->bmiHeader.biSize;
					
					sel_start.set(spx, spy), sel_end.set(spx+bw, spy+bh);
					image2screen(sel_start.x, sel_start.y, sel_s1.x, sel_s1.y);
					image2screen(sel_end.x, sel_end.y, sel_s2.x, sel_s2.y);
					selpos=sel_start;

					currentmode=M_RECT_SELECTION, selection_free=false;
					if(sel_start!=sel_end)
						selection_deselect();
					if(iw<spx+bw)
						iw=spx+bw;
					if(ih<spy+bh)
						ih=spy+bh;
					hist_premodify(image, iw, ih);
					sw=bw, sh=bh, sel_buffer=(int*)realloc(sel_buffer, sw*sh<<2);
					switch(bmi->bmiHeader.biBitCount)
					{
					case 24:
						for(int ky=0;ky<bh&&(bw*(ky+1)*3)<=data_size;++ky)
						{
							for(int kx=0;kx<bw;++kx)
							{
								auto dst=(byte*)(sel_buffer+sw*ky+kx), src=(byte*)(data+3*(bw*(bmi->bmiHeader.biHeight<0?ky:bmi->bmiHeader.biHeight-1-ky)+kx));
								dst[0]=src[0];
								dst[1]=src[1];
								dst[2]=src[2];
								dst[3]=0xFF;
							}
						}
						break;
					case 32:
						for(int ky=0;ky<bh&&(bw*(ky+1)<<2)<=data_size;++ky)
							for(int kx=0;kx<bw;++kx)
								sel_buffer[sw*ky+kx]=((int*)data)[bw*(bmi->bmiHeader.biHeight<0?ky:bmi->bmiHeader.biHeight-1-ky)+kx];
						break;
					}
				}
				GlobalUnlock(hData);
			}
			break;
		}
		CloseClipboard();
	}
}
void			pickcolor(int &color, int &pcolor, int alpha)
{
	CHOOSECOLOR cc={sizeof(CHOOSECOLOR), ghWnd, (HWND)ghInstance, swap_rb(color), (unsigned long*)ext_colorpalette, CC_FULLOPEN|CC_RGBINIT, 0, nullptr, nullptr};
	if(ChooseColorW(&cc))
		color=pcolor=swap_rb(cc.rgbResult), assign_alpha(color, alpha);
}
void			pickcolor_palette(int message, int &color, int &pcolor, int alpha)
{
	if(message==WM_LBUTTONDBLCLK)//show color picker
		pickcolor(color, pcolor, alpha);
	else
		color=pcolor, assign_alpha(color, alpha);
}
Point			p1, p2;
long			__stdcall WndProc(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	bool handled=false;
	int r=0;
#if !defined RELEASE && defined _DEBUG
	const char *debugmsg=nullptr;
#define			DEBUG(format, ...)	sprintf_s(g_buf, g_buf_size, format, __VA_ARGS__), debugmsg=g_buf;
#else
#define			DEBUG(format, ...)
#endif
	//if(focus==F_TEXTBOX&&message!=WM_CLOSE&&message!=WM_NCLBUTTONDOWN&&message!=WM_MOVE)
	//	SendMessageW(hTextbox, message, wParam, lParam);
	switch(message)
	{
	case WM_CREATE:
//	case WM_SHOWWINDOW:
		{
		//	RegisterDragDrop(hWnd, (IDropTarget*)&mdt);

			//menu bar
			hMenubar=CreateMenu();
			hMenuFile=CreateMenu(), hMenuEdit=CreateMenu(), hMenuView=CreateMenu(), hMenuZoom=CreateMenu(), hMenuImage=CreateMenu(), hMenuColors=CreateMenu(), hMenuHelp=CreateMenu();
			hMenuTest=CreateMenu();
			AppendMenuW(hMenuFile, MF_STRING, IDM_FILE_NEW, L"&New\tCtrl+N");			//[F]ILE
			AppendMenuW(hMenuFile, MF_STRING, IDM_FILE_OPEN, L"&Open...\tCtrl+O");
			AppendMenuW(hMenuFile, MF_STRING, IDM_FILE_SAVE, L"&Save\tCtrl+S");
			AppendMenuW(hMenuFile, MF_STRING, IDM_FILE_SAVE_AS, L"Save &As...\tCtrl+Shift+S");
			AppendMenuW(hMenuFile, MF_SEPARATOR, 0, 0);
			AppendMenuW(hMenuFile, MF_STRING, IDM_FILE_QUIT, L"E&xit\tAlt+F4");
			AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuFile, L"&File");

			AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_UNDO, L"&Undo\tCrtl+Z");			//[E]DIT
			AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_REPEAT, L"&Repeat\tCrtl+Y");
			AppendMenuW(hMenuEdit, MF_SEPARATOR, 0, 0);
			AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_CUT, L"Cu&t\tCrtl+X");
			AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_COPY, L"&Copy\tCrtl+C");
			AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_PASTE, L"&Paste\tCrtl+V");
			AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_CLEAR_SELECTION, L"C&lear Selection\tDel");
			AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_SELECT_ALL, L"Select &All\tCtrl+A");
			AppendMenuW(hMenuEdit, MF_SEPARATOR, 0, 0);
			AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_COPY_TO, L"C&opy To...");
			AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_PASTE_FROM, L"Paste &From...");
			AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuEdit, L"&Edit");

			AppendMenuW(hMenuView, MF_STRING, IDM_VIEW_TOOLBOX, L"&Tool Box\tCtrl+T");	//[V]IEW
			AppendMenuW(hMenuView, MF_STRING, IDM_VIEW_COLORBOX, L"&Color Box\tCtrl+L");
			AppendMenuW(hMenuView, MF_STRING, IDM_VIEW_STATUSBAR, L"&Status Bar");
			CheckMenuItem(hMenuView, IDM_VIEW_STATUSBAR, MF_CHECKED);//deprecated
			AppendMenuW(hMenuView, MF_STRING, IDM_VIEW_TEXT_TOOLBAR, L"T&ext Toolbar");
			AppendMenuW(hMenuView, MF_SEPARATOR, 0, 0);
		//	AppendMenuW(hMenuView, MF_STRING, IDM_VIEW_ZOOM, L"&Zoom");
			AppendMenuW(hMenuView, MF_STRING | MF_POPUP, (unsigned)hMenuZoom, L"&Zoom");
			AppendMenuW(hMenuView, MF_STRING, IDM_VIEW_VIEWBITMAP, L"&View Bitmap\tCtrl+F");
			AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuView, L"&View");

			AppendMenuW(hMenuZoom, MF_STRING, IDSM_ZOOM_IN, L"&Zoom Out\tCtrl+PgUp");		//VIEW/[Z]OOM
			AppendMenuW(hMenuZoom, MF_STRING, IDSM_ZOOM_OUT, L"&Zoom In\tCtrl+PgDn");
			AppendMenuW(hMenuZoom, MF_STRING, IDSM_ZOOM_CUSTOM, L"C&ustom...");
			AppendMenuW(hMenuZoom, MF_SEPARATOR, 0, 0);
			AppendMenuW(hMenuZoom, MF_STRING, IDSM_ZOOM_SHOW_GRID, L"Show &Grid\tCtrl+G");
			AppendMenuW(hMenuZoom, MF_STRING, IDSM_ZOOM_SHOW_THUMBNAIL, L"Show T&humbnail");

			AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_FLIP_ROTATE, L"&Flip/Rotate...\tCtrl+R");	//IMAGE
			AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_SKETCH_SKEW, L"&Sketch/Skew...\tCtrl+W");
			AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_INVERT_COLORS, L"&Invert Colors\tCtrl+I");
			AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_ATTRIBUTES, L"&Attributes...\tCtrl+E");
			AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_CLEAR_IMAGE, L"&Clear Image\tCtrl+Shift+N");
			AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_DRAW_OPAQUE, L"&Draw Opaque");
			AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuImage, L"&Image");

			AppendMenuW(hMenuColors, MF_STRING, IDM_COLORS_EDITCOLORS, L"&Edit Colors...");	//COLORS
			AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuColors, L"&Colors");

			AppendMenuW(hMenuTest, MF_STRING, IDM_TEST_RADIO1, L".&BMP");			//TEST
			AppendMenuW(hMenuTest, MF_STRING, IDM_TEST_RADIO2, L".&TIFF");
			AppendMenuW(hMenuTest, MF_STRING, IDM_TEST_RADIO3, L".&PNG");
			AppendMenuW(hMenuTest, MF_STRING, IDM_TEST_RADIO4, L".&JPEG");
		//	AppendMenuW(hMenuTest, MF_STRING, IDM_TEST_RADIO5, L".&FLIF");
			CheckMenuRadioItem(hMenuTest, IDM_TEST_RADIO1, IDM_TEST_RADIO4, IDM_TEST_RADIO3, MF_BYCOMMAND);
			AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuTest, L"&Temp Format");

			AppendMenuW(hMenuHelp, MF_STRING, IDM_HELP_TOPICS, L"&Help Topics");			//HELP
			AppendMenuW(hMenuHelp, MF_SEPARATOR, 0, 0);
			AppendMenuW(hMenuHelp, MF_STRING, IDM_HELP_ABOUT, L"&About Paint++");
			AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuHelp, L"&Help");

			SetMenu(hWnd, hMenubar);
			check_exit(L"Entry", __LINE__);

			//WNDCLASSEXW tb={sizeof WNDCLASSEXW, CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, };
			//RegisterClassExW(&tb);
		//	hFontbar=CreateWindowExW(0, TOOLBARCLASSNAMEW, L"Fonts", WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN, CW_USEDEFAULT, 0, 9+472+9, 30+31+8, hWnd, 0, 0, 0);//floating toolbar
			//hFontbar=CreateWindowExW(0, TOOLBARCLASSNAMEW, L"Fonts", WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, 0, 0, 0);//grey window
			//hFontbar=CreateWindowExW(WS_EX_TOOLWINDOW, TOOLBARCLASSNAMEW, L"Fonts", WS_CHILD, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, 0, 0, 0);//grey bar on top
		//	ShowWindow(hFontbar, SW_SHOWDEFAULT);
			HFONT hfDefault=(HFONT)GetStockObject(DEFAULT_GUI_FONT);

			hFontcombobox=CreateWindowExW(0, WC_COMBOBOXW, nullptr, CBS_DROPDOWN|CBS_HASSTRINGS|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_VSCROLL, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, (HMENU)IDM_FONT_COMBOBOX, nullptr, nullptr);
			SendMessageW(hFontcombobox, WM_SETFONT, (WPARAM)hfDefault, 0);

			hFontsizecb=CreateWindowExW(0, WC_COMBOBOXW, nullptr, CBS_DROPDOWN|CBS_HASSTRINGS|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_VSCROLL, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, (HMENU)IDM_FONTSIZE_COMBOBOX, nullptr, nullptr);
			SendMessageW(hFontsizecb, WM_SETFONT, (WPARAM)hfDefault, 0);

			hTextbox=CreateWindowExW(0, WC_EDITW, nullptr, WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_MULTILINE|ES_WANTRETURN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, nullptr, nullptr, nullptr);
		//	hTextbox=CreateWindowExW(0, WC_EDITW, nullptr, WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_MULTILINE|ES_WANTRETURN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, (HMENU)IDM_TEXT_EDIT, nullptr, nullptr);//returns nullptr
			if(!hTextbox)
				formatted_error(L"reateWindowExW(textbox)", __LINE__);
			//	messagebox(hWnd, L"Error", L"CreateWindowExW(textbox) at line %d", __LINE__);
			//	error_exit(L"CreateWindowExW(textbox)", __LINE__);
		//	hTextbox=CreateWindowExW(WS_EX_LAYERED, WC_EDITW, nullptr, WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_MULTILINE|ES_WANTRETURN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, nullptr, nullptr, nullptr);//invisible
			oldEditProc=(WNDPROC)SetWindowLongW(hTextbox, GWLP_WNDPROC, (long)EditProc);

			hRotatebox	=CreateWindowExW(0, WC_EDITW, nullptr, WS_BORDER|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_NUMBER, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, nullptr, ghInstance, nullptr);
			hStretchHbox=CreateWindowExW(0, WC_EDITW, nullptr, WS_BORDER|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_NUMBER, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, nullptr, ghInstance, nullptr);
			hStretchVbox=CreateWindowExW(0, WC_EDITW, nullptr, WS_BORDER|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_NUMBER, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, nullptr, ghInstance, nullptr);
			hSkewHbox	=CreateWindowExW(0, WC_EDITW, nullptr, WS_BORDER|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_NUMBER, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, nullptr, ghInstance, nullptr);
			hSkewVbox	=CreateWindowExW(0, WC_EDITW, nullptr, WS_BORDER|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_NUMBER, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, nullptr, ghInstance, nullptr);
			//hRotatebox	=CreateWindowExW(WS_EX_CLIENTEDGE|WS_EX_DLGMODALFRAME, WC_EDITW, nullptr, WS_BORDER|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_NUMBER, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, nullptr, nullptr, nullptr);//thick black rectangle
			//hStretchHbox	=CreateWindowExW(WS_EX_CLIENTEDGE|WS_EX_DLGMODALFRAME, WC_EDITW, nullptr, WS_BORDER|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_NUMBER, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, nullptr, nullptr, nullptr);
			//hStretchVbox	=CreateWindowExW(WS_EX_CLIENTEDGE|WS_EX_DLGMODALFRAME, WC_EDITW, nullptr, WS_BORDER|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_NUMBER, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, nullptr, nullptr, nullptr);
			//hSkewHbox		=CreateWindowExW(WS_EX_CLIENTEDGE|WS_EX_DLGMODALFRAME, WC_EDITW, nullptr, WS_BORDER|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_NUMBER, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, nullptr, nullptr, nullptr);
			//hSkewVbox		=CreateWindowExW(WS_EX_CLIENTEDGE|WS_EX_DLGMODALFRAME, WC_EDITW, nullptr, WS_BORDER|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE|WS_POPUP|ES_LEFT|ES_NUMBER, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, nullptr, nullptr, nullptr);
			if(!hRotatebox||!hStretchHbox||!hStretchVbox||!hSkewHbox||!hSkewVbox)
				formatted_error(L"CreateWindowExW(hRotatebox...hSkewVbox)", __LINE__);
			SendMessageW(hRotatebox		, WM_SETFONT, (WPARAM)hfDefault, 0);
			SendMessageW(hStretchHbox	, WM_SETFONT, (WPARAM)hfDefault, 0);
			SendMessageW(hStretchVbox	, WM_SETFONT, (WPARAM)hfDefault, 0);
			SendMessageW(hSkewHbox		, WM_SETFONT, (WPARAM)hfDefault, 0);
			SendMessageW(hSkewVbox		, WM_SETFONT, (WPARAM)hfDefault, 0);
		}
		break;
	case WM_SIZE:
		{
			GetClientRect(ghWnd, &R);
			int h2=R.bottom-R.top, w2=R.right-R.left;
			if(h!=h2||w!=w2)
			{
				if(!h2)
					h2=1;
				h=h2, w=w2, rgbn=w*h;
				if(hBitmap)
				{
					hBitmap=(HBITMAP)SelectObject(ghMemDC, hBitmap);
					DeleteObject(hBitmap);
				}
				BITMAPINFO bmpInfo={{sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0}};
				hBitmap=CreateDIBSection(0, &bmpInfo, DIB_RGB_COLORS, (void**)&rgb, 0, 0);
				hBitmap=(HBITMAP)SelectObject(ghMemDC, hBitmap);
				switch(colorbarcontents)
				{
				case CS_TEXTOPTIONS:
					resize_textgui();
					move_textbox();
					break;
				case CS_FLIPROTATE:
					fliprotate_moveui(true);
					break;
				case CS_STRETCHSKEW:
					stretchskew_moveui(true);
					break;
				}
				//if(currentmode==M_TEXT)
				//{
				//	resize_textgui();
				//	move_textbox();
				//}
			}
		//	SendMessageW(ghStatusbar, WM_SIZE, wParam, lParam);
			//SendMessageW(gToolbar, WM_SIZE, wParam, lParam);
			//SendMessageW(gColorBar, WM_SIZE, wParam, lParam);
		}
		break;
	case WM_MOVE:
		switch(colorbarcontents)
		{
		case CS_TEXTOPTIONS:
		//	resize_textgui();
			move_textbox();
			break;
		case CS_FLIPROTATE:
			fliprotate_moveui(true);
			break;
		case CS_STRETCHSKEW:
			stretchskew_moveui(true);
			break;
		}
		//if(currentmode==M_TEXT)
		//	move_textbox();
		break;
	case WM_PAINT:
	case WM_TIMER:
		render();
		break;
	case WM_GETICON:
		{
			//https://stackoverflow.com/questions/4285890/how-to-load-a-small-system-icon
			//100: IDI_APPLICATION, 101: IDI_WARNING, 102: IDI_QUESTION, 103: IDI_ERROR, 104: IDI_INFORMATION, 105:IDI_WINLOGO, 106: IDI_SHIELD
			HINSTANCE hModule=GetModuleHandleW(L"user32");
			HICON hIcon=(HICON)LoadImageW(hModule, (wchar_t*)100, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
			if(!hIcon)
				formatted_error(L"LoadImageW", __LINE__);
			return (long)hIcon;
		}
		break;
	case WM_CTLCOLOREDIT:
		if((HWND)lParam==hTextbox)//textbox about to be drawn
		{
		//	GUIPrint(ghDC, 0, 0, selection_transparency);//
			HDC hDC=(HDC)wParam;
			SetTextColor(hDC, swap_rb(primarycolor&0x00FFFFFF));
			SetBkMode(hDC, selection_transparency);
			SetBkColor(hDC, swap_rb(secondarycolor&0x00FFFFFF));
			return (long)GetStockObject(selection_transparency==TRANSPARENT?NULL_BRUSH:WHITE_BRUSH);
		//	return (long)GetStockObject(NULL_BRUSH);
		//	RedrawWindow(hTextbox, nullptr, nullptr, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE);//sends WM_CTLCOLOREDIT again
		//	return (long)GetStockObject(WHITE_BRUSH);

			//int brushes[]={WHITE_BRUSH, LTGRAY_BRUSH, GRAY_BRUSH, DKGRAY_BRUSH, NULL_BRUSH};
			//const int nbrushes=sizeof brushes>>2;
			//return (long)GetStockObject(brushes[rand()%nbrushes]);
		//	return (long)GetStockObject(rand()%6);
		//	return (long)GetStockObject(GRAY_BRUSH);
		}
		break;
	case WM_COMMAND:
#if 1
		{
			int wp_hi=((short*)&wParam)[1], wp_lo=(short&)wParam;
			if(wp_hi==EN_CHANGE&&(HWND)lParam==hTextbox)
			{
				//RedrawWindow(hTextbox, nullptr, nullptr, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE);
				//ShowWindow(hTextbox, SW_HIDE);
				//ShowWindow(hTextbox, SW_SHOWNORMAL);
				focus=F_TEXTBOX;
			//	SetFocus(hTextbox);
			}
			else switch(wp_lo)
			{
			case IDM_FONT_COMBOBOX://font selection combobox
			case IDM_FONTSIZE_COMBOBOX://font size combobox
				if(wp_hi==CBN_SELCHANGE||wp_hi==CBN_EDITCHANGE)
				{
					HWND handle=(HWND)lParam;
					int result=SendMessageW(handle, CB_GETCURSEL, 0, 0);
					//wchar_t listitem[256]={0};
					//SendMessageW((HWND)lParam, CB_GETLBTEXT, font_idx, (LPARAM)listitem);//
					//int LOL_1=0;
					bool changed=false;
					if(handle==hFontcombobox)
					{
						if(result!=-1)
							set_sizescombobox(result), font_idx=result, changed=true;
					}
					else if(handle==hFontsizecb)
					{
						changed=true;
						GetWindowTextW(hFontsizecb, g_wbuf, g_buf_size);
					//	SendMessageW(hFontsizecb, CB_GETLBTEXT, result, (LPARAM)g_wbuf);
						int fs2=_wtoi(g_wbuf);
						std::wstring static c0;//combobox contents
						std::wstring c=g_wbuf;
						if(!fs2)
						{
							if(c!=c0)
								messagebox(ghWnd, L"Paint++", L"Font size must be a numeric value.");
							fs2=10;
						}
						c0=c;
						font_size=fs2;
						//if(fs2!=font_size)
						//	SetFocus(ghWnd), font_size=fs2;
						static int ksizecb=0, wp0=0, lp0=0;
						++ksizecb;
						DEBUG("font size=%d, %d times, (0x%08X, h=0x%08X), (0x%08X, h=0x%08X)", font_size, ksizecb, wp0, lp0, wParam, lParam);
						wp0=wParam, lp0=lParam;
					//	DEBUG("font size=%d, %d times", font_size, ksizecb);
					}
					if(changed)
					{
						change_font(font_idx, font_size);
					//	SendMessageW(hTextbox, WM_SETFONT, (WPARAM)hFont, true);
					}
				}
				break;
			//case IDM_TEXT_EDIT://hTextbox
			//	if(wp_hi==EN_CHANGE)
			//		RedrawWindow(hTextbox, nullptr, nullptr, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE);
			//	break;

				//file menu
			case IDM_FILE_NEW:
				image=hist_start(iw, ih);
				memset(image, 0xFF, image_size<<2);
			//	modify();
				render();
				break;
			case IDM_FILE_OPEN:
				open_media();
				break;
			case IDM_FILE_SAVE:
				if(filepath.size())
					save_media(filepath);
				else
					save_media_as();
				break;
			case IDM_FILE_SAVE_AS:
				save_media_as();
				break;
			case IDM_FILE_QUIT:
				PostQuitMessage(0);
				break;

				//edit menu
			case IDM_EDIT_UNDO:
				hist_undo(image);
				render();
				break;
			case IDM_EDIT_REPEAT:
				hist_redo(image);
				render();
				break;
			case IDM_EDIT_CUT:
				if(editcopy())
				{
					selection_remove();
					render();
				}
				break;
			case IDM_EDIT_COPY:
				editcopy();
				break;
			case IDM_EDIT_PASTE:
				editpaste();
				break;
			case IDM_EDIT_CLEAR_SELECTION:
				if(sel_start!=sel_end)
				{
					memset(sel_buffer, 0xFF, sw*sh<<2);
					render();
				}
				break;
			case IDM_EDIT_SELECT_ALL:
				selection_selectall();
				render();
				break;
			case IDM_EDIT_COPY_TO:			unimpl();break;
			case IDM_EDIT_PASTE_FROM:		unimpl();break;

				//view menu
			case IDM_VIEW_TOOLBOX:			unimpl();break;//useless
			case IDM_VIEW_COLORBOX:			unimpl();break;
			case IDM_VIEW_STATUSBAR:
				unimpl();
				//if(GetMenuState(hMenuView, IDM_VIEW_STATUSBAR, MF_BYCOMMAND)==MF_CHECKED)
				//{
				//	ShowWindow(ghStatusbar, SW_HIDE);
				//	CheckMenuItem(hMenuView, IDM_VIEW_STATUSBAR, MF_UNCHECKED);
				//}
				//else
				//{
				//	ShowWindow(ghStatusbar, SW_SHOWNA);
				//	CheckMenuItem(hMenuView, IDM_VIEW_STATUSBAR, MF_CHECKED);
				//}
				break;
			case IDM_VIEW_TEXT_TOOLBAR:		unimpl();break;
			case IDM_VIEW_ZOOM:				unimpl();break;
			case IDM_VIEW_VIEWBITMAP:		unimpl();break;//fullscreen bitmap with black background until any key is pressed

				//view/zoom menu
			case IDSM_ZOOM_IN:
				++logzoom, zoom*=2;
				render();
				break;
			case IDSM_ZOOM_OUT:
				--logzoom, zoom*=0.5;
				render();
				break;
			case IDSM_ZOOM_CUSTOM:			unimpl();break;//a window with zoom options
			case IDSM_ZOOM_SHOW_GRID:
				showgrid=!showgrid;
				CheckMenuItem(hMenuZoom, IDSM_ZOOM_SHOW_GRID, showgrid?MF_CHECKED:MF_UNCHECKED);//deprecated
				render();
				break;
			case IDSM_ZOOM_SHOW_THUMBNAIL:	unimpl();break;//a small window with unzoomed visible part

				//image menu
			case IDM_IMAGE_FLIP_ROTATE:
				replace_colorbarcontents(CS_FLIPROTATE);
				render();
				break;
			case IDM_IMAGE_SKETCH_SKEW:
				replace_colorbarcontents(CS_STRETCHSKEW);
				render();
				break;
			case IDM_IMAGE_INVERT_COLORS:
				hist_premodify(image, iw, ih);
				invertcolor(image, iw, ih);
				render();
				break;
			case IDM_IMAGE_ATTRIBUTES:		unimpl();break;
			case IDM_IMAGE_CLEAR_IMAGE:
				hist_premodify(image, iw, ih);
				memset(image, 0xFF, image_size<<2);
				render();
				break;
			case IDM_IMAGE_DRAW_OPAQUE:
				selection_transparency=!selection_transparency;
				CheckMenuItem(hMenuImage, IDM_IMAGE_DRAW_OPAQUE, selection_transparency?MF_UNCHECKED:MF_CHECKED);//deprecated
				render();
				break;

				//colors menu
			case IDM_COLORS_EDITCOLORS:
				pickcolor(primarycolor, colorpalette[primary_idx], primary_alpha);
				render();
				break;

				//temp format menu		TODO: move this option to a submenu
			case IDM_TEST_RADIO1:
				CheckMenuRadioItem(hMenuTest, IDM_TEST_RADIO1, IDM_TEST_RADIO4, IDM_TEST_RADIO1, MF_BYCOMMAND);
				tempformat=IF_BMP;
			//	MessageBeep(MB_ICONERROR);
				break;
			case IDM_TEST_RADIO2:
				CheckMenuRadioItem(hMenuTest, IDM_TEST_RADIO1, IDM_TEST_RADIO4, IDM_TEST_RADIO2, MF_BYCOMMAND);
				tempformat=IF_TIFF;
			//	MessageBeep(0xFFFFFFFF);
				break;
			case IDM_TEST_RADIO3:
				CheckMenuRadioItem(hMenuTest, IDM_TEST_RADIO1, IDM_TEST_RADIO4, IDM_TEST_RADIO3, MF_BYCOMMAND);
				tempformat=IF_PNG;
			//	MessageBeep(MB_ICONWARNING);
				break;
			case IDM_TEST_RADIO4:
				CheckMenuRadioItem(hMenuTest, IDM_TEST_RADIO1, IDM_TEST_RADIO4, IDM_TEST_RADIO4, MF_BYCOMMAND);
				tempformat=IF_JPEG;
			//	MessageBeep(MB_ICONINFORMATION);
				break;
			case IDM_TEST_RADIO5:
				CheckMenuRadioItem(hMenuTest, IDM_TEST_RADIO1, IDM_TEST_RADIO4, IDM_TEST_RADIO5, MF_BYCOMMAND);
				tempformat=IF_FLIF;
			//	MessageBeep(MB_ICONINFORMATION);
				break;

				//help menu
			case IDM_HELP_TOPICS:			unimpl();break;
			case IDM_HELP_ABOUT:
				messagebox(ghWnd, L"About Paint++", L"%S version. Compiled on: %S, %S.",
#ifdef _DEBUG
					"Debug",
#else
					"Release",
#endif
					__DATE__, __TIME__);
				break;
			}
		}
#endif
		break;
	case WM_NCHITTEST://0x0084: non-client hit test - update mouse position
		{
			POINT p={(short&)lParam, ((short*)&lParam)[1]};//screen coordinates
			ScreenToClient(ghWnd, &p);//this can tell window position
			mx=p.x, my=p.y;
			const int scrollbarwidth=17;
			p1.setzero(), p2.set(w, h);
		//	x1=0, y1=0, x2=w, y2=h;
			if(mx<0||mx>=w||my<0||my>=h)
				mousepos=MP_NONCLIENT;
			else if(p2.x-=showthumbnailbox*(w-thbox_x1), showthumbnailbox&&mx>=thbox_x1)//thumbnail box
				mousepos=MP_THUMBNAILBOX;
			else if(p2.y-=74*showcolorbar, showcolorbar&&my>=h-74)//color bar
				mousepos=MP_COLORBAR;
			else if(p1.x+=57*showtoolbox, showtoolbox&&mx<57)//toolbar
				mousepos=MP_TOOLBAR;
			else if(p2.x-=scrollbarwidth*vscrollbar, vscrollbar&&mx>=p2.x)//image vertical scrollbar
				mousepos=MP_VSCROLL;
			else if(p2.y-=scrollbarwidth*hscrollbar, hscrollbar&&my>p2.y)//image horizontal scrollbar
				mousepos=MP_HSCROLL;
			else//image field
			{
				Point s_end;
				image2screen(iw, ih, s_end.x, s_end.y);
				if(mx>=p1.x+4&&mx<s_end.x&&my>=p1.y+5&&my<s_end.y)
					mousepos=MP_IMAGE;
				else
					mousepos=MP_IMAGEWINDOW;
			}
		}
		break;
	case WM_SETCURSOR://0x0020
		if(mousepos!=MP_NONCLIENT)
		{
			SetCursor(mousepos==MP_IMAGE?hcursor:hcursor_original);//TODO: other tool cursors
			r=true, handled=true;
		}
		break;
	//case WM_NCMOUSEMOVE://0x00A0: mouse on border
	//	break;
	//case WM_NCLBUTTONDOWN://0x00A1: click on border
	//	break;
	case WM_MOUSEMOVE://0x0200		when a button is down, only WM_MOUSEMOVE is received
		mx=(short&)lParam, my=((short*)&lParam)[1];//client coordinates
		if(drag||currentmode==M_ERASER||currentmode==M_MAGNIFIER||currentmode==M_BRUSH)
			render();
#ifdef MOUSE_POLLING_TEST
		else//mouse polling test
			render2();//
#endif
		break;
	case WM_LBUTTONDOWN://left click
	case WM_LBUTTONDBLCLK:
#if 1
		//if(focus==F_TEXTBOX)
		//	SendMessageW(hTextbox, message, wParam, lParam);
		//else
		{
			mb[0]=true;
			const int scrollbarwidth=17;
			switch(mousepos)
			{
			case MP_THUMBNAILBOX:
				if(thbox_scrollbar&&mx>=w-scrollbarwidth)//thbox vscrollbar
				{
					if(my>h-74*showcolorbar-scrollbarwidth)//down arrow
						thbox_posy+=10+16+th_h;
					else if(my>scrollbarwidth)//
						drag=D_THBOX_VSCROLL, thbox_vscroll_my_start=my, thbox_vscroll_s0=thbox_vscroll_start;
					else//up arrow
						thbox_posy-=10+16+th_h;
				}
				else//click on thumbnail
				{
					int frame_number=(my+thbox_posy-10)/(16+th_h+10);
					if(frame_number!=current_frame)
					{
						if(modified)
						{
							save_frame(current_frame);
							assign_thumbnail(thumbnails[current_frame]);
							//load_thumbnails();//
						}
						load_frame(frame_number), current_frame=frame_number;
						modified=false;
					//	hist_start(image);
					}
				}
				break;
			case MP_COLORBAR:
				if(mx>=31&&mx<255&&my>=h-65&&my<h-33)//color palette
				{
					int kx=(mx-31)>>4, ky=(my-(h-65))>>4;
					primary_idx=kx<<1|ky;
					pickcolor_palette(message, primarycolor, colorpalette[primary_idx], primary_alpha);
					//if(message==WM_LBUTTONDBLCLK)//show color picker			//FUNCTION
					//{
					//	CHOOSECOLOR cc={sizeof(CHOOSECOLOR), ghWnd, (HWND)ghInstance, swap_rb(primarycolor), (unsigned long*)ext_colorpalette, CC_FULLOPEN|CC_RGBINIT, 0, nullptr, nullptr};
					//	if(ChooseColorW(&cc))
					//		primarycolor=colorpalette[idx]=swap_rb(cc.rgbResult), assign_alpha(primarycolor, primary_alpha);
					//}
					//else
					//	primarycolor=colorpalette[idx], assign_alpha(primarycolor, primary_alpha);
					if(currentmode==M_TEXT)
						RedrawWindow(hTextbox, nullptr, nullptr, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE);
					//	RedrawWindow(hTextbox, nullptr, nullptr, 0);
				}
				else if(mx>=320-14&&mx<320+255+14)//alpha slider
				{
					if(my<h-47)
						drag=D_ALPHA1;
					else
						drag=D_ALPHA2;
				}
				else
				{
					switch(colorbarcontents)
					{
					case CS_TEXTOPTIONS:
						{
							int x0=700, y0=h-48;
							if(mx>=x0&&mx<x0+150&&my>=y0&&my<y0+23)//bold, italic, underline buttons
							{
								if(mx<x0+50)
									bold=!bold;
								else if(mx<x0+100)
									italic=!italic;
								else
									underline=!underline;
							}
						}
						break;
					case CS_FLIPROTATE:
						if(mx>=650&&mx<650+104*3&&my>=h-68&&my<h-68+40)
						{
							int kx=(mx-650)/104, ky=(my-(h-68))/20, idx=kx<<1|ky;
							fliprotate(idx);
						}
						break;
					//case CS_STRETCHSKEW://no buttons
					//	break;
					}
				}
				break;
			case MP_TOOLBAR:
				if(mx>=3&&mx<53&&my>5&&my<205)//tool box
				{
					int currentmode0=currentmode;
					int kx=(mx-3)/25, ky=(my-5)/25;
					currentmode=ky<<1|kx;//TODO: change cursor icon
					if(currentmode==M_TEXT&&logzoom)
						currentmode=prevmode;
					if(prevmode<=M_RECT_SELECTION&&currentmode>M_RECT_SELECTION&&!selection_persistent)//switch away from selection
					{
						selection_deselect();
						selection_remove();
					}
					if(currentmode0!=M_TEXT&&currentmode==M_TEXT)//switch to text mode
					{
						resize_textgui();
						replace_colorbarcontents(CS_TEXTOPTIONS);
						sample_font(font_idx, font_size);
						change_font(font_idx, font_size);
					//	SendMessageW(hTextbox, WM_SETFONT, (WPARAM)hFont, 0);
					}
					else if(currentmode0==M_TEXT&&currentmode!=M_TEXT)//switch away from text mode
					{
						print_text();
						exit_textmode();
					}
					if(prevmode==M_POLYGON&&currentmode!=M_POLYGON)//switch away from polygon
					{
						polygon_draw(image, polygon_leftbutton?primarycolor:secondarycolor, polygon_type==ST_FILL==polygon_leftbutton?primarycolor:secondarycolor);
						//int c3, c4;
						//if(polygon_type==ST_FILL==polygon_leftbutton)
						//	c3=primarycolor, c4=secondarycolor;
						//else
						//	c3=secondarycolor, c4=primarycolor;
						//polygon_draw(image, c3, c4);
						use_temp_buffer=false;
					}
					if(currentmode!=M_PICK_COLOR&&currentmode!=M_MAGNIFIER&&currentmode!=M_TEXT)
						prevmode=currentmode;
				}
				else if(mx>=7&&mx<49&&my>210&&my<276)//tool type
				{
					switch(currentmode)
					{
					case M_FREE_SELECTION:
					case M_RECT_SELECTION:
					case M_TEXT:
						if(my<243)
						{
							selection_transparency=selection_transparency==OPAQUE?TRANSPARENT:OPAQUE;
							if(currentmode==M_TEXT)
								RedrawWindow(hTextbox, nullptr, nullptr, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE);
						}
						else
							selection_persistent=!selection_persistent;
						break;
					case M_ERASER:
						erasertype=ERASER04+((my-211)>>4);
						if(erasertype<ERASER04)
							erasertype=ERASER04;
						else if(erasertype>ERASER10)
							erasertype=ERASER10;
						break;
					case M_MAGNIFIER:
						magnifier_level=(my-210)/22<<1|(mx-7)/21;
						break;
					case M_BRUSH:
						brushtype=BRUSH_LARGE_DISK+(my-210)/16*3+(mx-7)/14;
						break;
					//case M_AIRBRUSH:break;
					case M_LINE:
					case M_CURVE:
						linewidth=1+(my-210)/13;
						break;
					case M_RECTANGLE:
						rectangle_type=(my-210)/22;
						break;
					case M_POLYGON:
						polygon_type=(my-210)/22;
						break;
					case M_ELLIPSE:
						ellipse_type=(my-210)/22;
						break;
					case M_ROUNDED_RECT:
						roundrect_type=(my-210)/22;
						break;
					}
				}
				break;
			case MP_VSCROLL://image vertical scrollbar
				{
					int dy=shift(h/10, -logzoom);
					if(dy<=0)
						dy=1;
					if(my>=p2.y-scrollbarwidth-scrollbarwidth*hscrollbar)//down arrow
						spy+=dy;
					else if(my>=p1.y+scrollbarwidth)//vertical slider
						drag=D_VSCROLL, vscroll_my_start=my, vscroll_s0=vscroll_start;
					else//up arrow
						spy-=dy;
				}
				break;
			case MP_HSCROLL://image horizontal scrollbar
				{
					int dx=shift(w/10, -logzoom);
					if(dx<=0)
						dx=1;
					if(mx>=p2.x-scrollbarwidth-scrollbarwidth*vscrollbar)//right arrow
						spx+=dx;
					else if(mx>=p1.x+scrollbarwidth)//horizontal slider
						drag=D_HSCROLL, hscroll_mx_start=mx, hscroll_s0=hscroll_start;
					else//left arrow
						spx-=dx;
				}
				break;
			case MP_IMAGEWINDOW:
			case MP_IMAGE:
				switch(currentmode)
				{
				case M_FREE_SELECTION:
				case M_RECT_SELECTION:
					if(mx>=sel_s1.x&&mx<sel_s2.x&&my>=sel_s1.y&&my<sel_s2.y)//drag selection
						drag=D_SELECTION;
					else
					{
						selection_deselect();
						screen2image(mx, my, sel_start.x, sel_start.y);
						sel_end=sel_start;
						drag=D_DRAW;
					}
					break;
				case M_FILL:
					hist_premodify(image, iw, ih);
					fill_mouse(image, mx, my, primarycolor);
					break;
				case M_MAGNIFIER:
					if(logzoom!=magnifier_level)
					{
						int imx, imy;
						screen2image(mx, my, imx, imy);
						logzoom=magnifier_level, zoom=1<<magnifier_level;
						int dx=w-(61*showtoolbox+17), dy=h-(5+17+73*showcolorbar), idx, idy;
						screen2image(dx, dy, idx, idy);
						spx=imx-(idx>>1), spy=imy-(idy>>1);
					}
					else
						logzoom=0, zoom=1;
					currentmode=prevmode;
					break;
				case M_AIRBRUSH:
					drag=D_DRAW;
					if(!timer)
						SetTimer(ghWnd, 0, airbrush_timer, 0), timer=true;
					break;
				case M_TEXT:
					{
						int padding=2;
						if(mx>=text_s1.x-padding&&mx<text_s2.x+padding&&my>=text_s1.y-padding&&my<text_s2.y+padding)//drag textbox
					//	if(mx>=text_s1.x&&mx<text_s2.x&&my>=text_s1.y&&my<text_s2.y)
							drag=D_SELECTION;
						else//define new textbox
						{
							focus=F_MAIN;
						//	SetFocus(ghWnd);
							print_text();
							screen2image(mx, my, text_start.x, text_start.y);
							text_end=text_start;
							drag=D_DRAW;
						}
					}
					break;
				case M_CURVE:
					if(!bezier.size())
						curve_add_mouse(mx, my);
					curve_add_mouse(mx, my);
					drag=D_DRAW;
					break;
				case M_POLYGON:
					polygon_leftbutton=true;
					drag=D_DRAW;
					break;
				default:
					drag=D_DRAW;
					break;
				}
				break;
			}
#ifdef CAPTURE_MOUSE
			if(drag)
				SetCapture(ghWnd);
#endif
			render();
			prev_drag=drag;
		}
#endif
		break;
	case WM_LBUTTONUP:
#ifdef CAPTURE_MOUSE
		if(drag)
			ReleaseCapture();
#endif
		if(drag==D_DRAW)
		{
			switch(currentmode)
			{
			case M_FREE_SELECTION:
				freesel_select();
				break;
			case M_RECT_SELECTION:
				{
					Point p1, p2;//sorted-bounded image coordinates
					selection_assign_mouse(start_mx, start_my, mx, my, sel_start, sel_end, p1, p2);
					selection_select(p1, p2);
				}
				break;
			case M_PICK_COLOR:
				primarycolor=pick_color_mouse(image, mx, my);//with alpha
				currentmode=prevmode;
				break;
			case M_AIRBRUSH:
				if(timer)
					KillTimer(ghWnd, 0), timer=false;
				break;
			case M_TEXT:
				{
					Point p1, p2;
					image2screen(text_start.x, text_start.y, p1.x, p1.y);
					image2screen(text_end.x, text_end.y, p2.x, p2.y);
					selection_sort(p1, p2, text_s1, text_s2);

					move_textbox();
					int prevshow=ShowWindow(hTextbox, SW_SHOWNA);
				//	draw_line_mouse(image, start_mx, start_my, mx, my, rand()<<15|rand());
				}
				break;
			case M_LINE:
				hist_premodify(image, iw, ih);
				draw_line_mouse(image, start_mx, start_my, mx, my, primarycolor);
				use_temp_buffer=false;
				break;
			case M_CURVE:
				if(bezier.size()==4)
				{
					hist_premodify(image, iw, ih);
					curve_draw(image, primarycolor);
					bezier.clear();
					use_temp_buffer=false;
				}
				break;
			case M_RECTANGLE:
				hist_premodify(image, iw, ih);
				draw_rectangle_mouse(image, start_mx, mx, start_my, my, primarycolor, secondarycolor);
				use_temp_buffer=false;
				break;
			case M_POLYGON:
				polygon_add_mouse(start_mx, start_my, mx, my);
				break;
			case M_ELLIPSE:
				hist_premodify(image, iw, ih);
				draw_ellipse_mouse(image, start_mx, mx, start_my, my, primarycolor, secondarycolor);
				use_temp_buffer=false;
				break;
			case M_ROUNDED_RECT:
				hist_premodify(image, iw, ih);
				draw_roundrect_mouse(image, start_mx, mx, start_my, my, primarycolor, secondarycolor);
				use_temp_buffer=false;
				break;
			}
		}
		mb[0]=false, prev_drag=drag=D_NONE;
		render();
		break;
	case WM_RBUTTONDOWN://right click
	case WM_RBUTTONDBLCLK:
		//if(focus==F_TEXTBOX)
		//	SendMessageW(hTextbox, message, wParam, lParam);
		//else
		{
			mb[1]=true;
			const int scrollbarwidth=17;
			switch(mousepos)
			{
			//case MP_THUMBNAILBOX:
			//	break;
			case MP_COLORBAR:
				if(mx>=31&&mx<255&&my>=h-65&&my<h-33)//color palette
				{
					int kx=(mx-31)>>4, ky=(my-(h-65))>>4, idx=kx<<1|ky;
					pickcolor_palette(message, secondarycolor, colorpalette[idx], secondary_alpha);
					//if(message==WM_RBUTTONDBLCLK)//show color picker
					//{
					//	CHOOSECOLOR cc={sizeof(CHOOSECOLOR), ghWnd, (HWND)ghInstance, swap_rb(secondarycolor), (unsigned long*)ext_colorpalette, CC_FULLOPEN|CC_RGBINIT, 0, nullptr, nullptr};
					//	if(ChooseColorW(&cc))
					//		secondarycolor=colorpalette[idx]=swap_rb(cc.rgbResult), assign_alpha(secondarycolor, secondary_alpha);
					//}
					//else
					//	secondarycolor=colorpalette[idx], assign_alpha(secondarycolor, secondary_alpha);
					if(currentmode==M_TEXT)
						RedrawWindow(hTextbox, nullptr, nullptr, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE);
				}
				break;
			//case MP_TOOLBAR:
			//	break;
			//case MP_VSCROLL:
			//	break;
			//case MP_HSCROLL:
			//	break;
			case MP_IMAGEWINDOW:
			case MP_IMAGE:
				switch(currentmode)
				{
				case M_FREE_SELECTION:
				case M_RECT_SELECTION:
					break;
				case M_FILL:
					hist_premodify(image, iw, ih);
					fill_mouse(image, mx, my, secondarycolor);
					break;
				case M_AIRBRUSH:
					drag=D_DRAW;
					if(!timer)
						SetTimer(ghWnd, 0, airbrush_timer, 0), timer=true;
					break;
				case M_CURVE:
					if(!bezier.size())
						curve_add_mouse(mx, my);
					curve_add_mouse(mx, my);
					drag=D_DRAW;
					break;
				case M_POLYGON:
					polygon_leftbutton=false;
					drag=D_DRAW;
					break;
				default:
					drag=D_DRAW;
					break;
				}
				break;
			}
#ifdef CAPTURE_MOUSE
			if(drag)
				SetCapture(ghWnd);
#endif
			render();
			prev_drag=drag;
		}
		break;
	case WM_RBUTTONUP://context menu
#ifdef CAPTURE_MOUSE
		if(drag)
			ReleaseCapture();
#endif
		if(drag==D_DRAW)
		{
			switch(currentmode)
			{
			case M_PICK_COLOR:
				secondarycolor=pick_color_mouse(image, mx, my);//with alpha
				break;
			case M_AIRBRUSH:
				if(timer)
					KillTimer(ghWnd, 0), timer=false;
				break;
			case M_LINE:
				hist_premodify(image, iw, ih);
				draw_line_mouse(image, start_mx, start_my, mx, my, secondarycolor);
				use_temp_buffer=false;
				break;
			case M_CURVE:
				if(bezier.size()==4)
				{
					hist_premodify(image, iw, ih);
					curve_draw(image, secondarycolor);
					bezier.clear();
					use_temp_buffer=false;
				}
				break;
			case M_RECTANGLE:
				hist_premodify(image, iw, ih);
				draw_rectangle_mouse(image, start_mx, mx, start_my, my, secondarycolor, primarycolor);
				use_temp_buffer=false;
				break;
			case M_POLYGON:
				polygon_add_mouse(start_mx, start_my, mx, my);
				break;
			case M_ELLIPSE:
				hist_premodify(image, iw, ih);
				draw_ellipse_mouse(image, start_mx, mx, start_my, my, secondarycolor, primarycolor);
				use_temp_buffer=false;
				break;
			case M_ROUNDED_RECT:
				hist_premodify(image, iw, ih);
				draw_roundrect_mouse(image, start_mx, mx, start_my, my, secondarycolor, primarycolor);
				use_temp_buffer=false;
				break;
			}
		}
		mb[1]=false, prev_drag=drag=D_NONE;
		if(currentmode<=M_RECT_SELECTION&&(mousepos==MP_IMAGEWINDOW||mousepos==MP_IMAGE))//image region
	//	if(currentmode<=M_RECT_SELECTION&&mx>=59*showtoolbox&&mx<w&&my>=0&&my<h-74*showcolorbar)
		{
			POINT mouse={(short&)lParam, ((short*)&lParam)[1]};
			ClientToScreen(ghWnd, &mouse);
			HMENU hMenuContext=CreatePopupMenu();

			AppendMenuW(hMenuContext, MF_STRING, IDM_EDIT_CUT, L"Cu&t");
			AppendMenuW(hMenuContext, MF_STRING, IDM_EDIT_COPY, L"&Copy");
			AppendMenuW(hMenuContext, MF_STRING, IDM_EDIT_PASTE, L"&Paste");
			AppendMenuW(hMenuContext, MF_STRING, IDM_EDIT_CLEAR_SELECTION, L"&Clear Selection");
			AppendMenuW(hMenuContext, MF_STRING, IDM_EDIT_SELECT_ALL, L"Select &All");
			AppendMenuW(hMenuContext, MF_SEPARATOR, 0, 0);
			AppendMenuW(hMenuContext, MF_STRING, IDM_EDIT_COPY_TO, L"C&opy To...");
			AppendMenuW(hMenuContext, MF_STRING, IDM_EDIT_PASTE_FROM, L"Paste &From...");
			AppendMenuW(hMenuContext, MF_SEPARATOR, 0, 0);
			AppendMenuW(hMenuContext, MF_STRING, IDM_IMAGE_FLIP_ROTATE, L"&Flip/Rotate...");
			AppendMenuW(hMenuContext, MF_STRING, IDM_IMAGE_SKETCH_SKEW, L"&Sketch/Skew...");
			AppendMenuW(hMenuContext, MF_STRING, IDM_IMAGE_INVERT_COLORS, L"&Invert Colors");
			            
			TrackPopupMenu(hMenuContext, TPM_RIGHTBUTTON, mouse.x, mouse.y, 0, ghWnd, 0);
			DestroyMenu(hMenuContext);
		}
		render();
		break;
	case WM_MOUSEWHEEL:
		{
			unsigned short keys=(short&)wParam;
			bool mw_forward=((short*)&wParam)[1]>0;
			if(keys&0x0008)//ctrl wheel: zoom
			{
				if(mw_forward)
					++logzoom, zoom*=2;
				else
					--logzoom, zoom*=0.5;
				if(!logzoom)
					zoom=1;
				if(currentmode==M_TEXT&&logzoom)
				{
					exit_textmode();
					currentmode=prevmode;
				}
			}
		//	else if(showthumbnailbox&&mousepos==MP_THUMBNAILBOX)
			else if(showthumbnailbox&&mx>thbox_x1)//scroll thumbnailbox
			{
				if(thbox_scrollbar)
					thbox_posy+=(!mw_forward-mw_forward)*(10+16+th_h);
			}
			else//scroll image
			{
				if(vscrollbar)
					spy+=(!mw_forward-mw_forward)*(ih>>4);
				else if(hscrollbar)
					spx+=(!mw_forward-mw_forward)*(iw>>4);
			}
			//if(currentmode==M_TEXT)
			//	SendMessageW(hFontcombobox, message, wParam, lParam);
			render();
		//	GUIPrint(ghDC, w>>1, h-16, "zoom=%lf", zoom);
		}
		break;
	case WM_KEYDOWN:case WM_SYSKEYDOWN:
		//if(focus==F_TEXTBOX)
		//	SendMessageW(hTextbox, message, wParam, lParam);
		//else
		{
			kb[wParam]=true;
			switch(wParam)
			{
			case VK_RETURN://debug
				switch(colorbarcontents)
				{
				case CS_FLIPROTATE:
					fliprotate();
					break;
				case CS_STRETCHSKEW:
					stretchskew();
					break;
				}
				render();
				//if(currentmode==M_TEXT)
				//{
				//	SetWindowLongW(hTextbox, GWL_EXSTYLE, WS_EX_LAYERED);
				////	SetLayeredWindowAttributes(hTextbox, RGB(255, 0, 0), 128, LWA_COLORKEY|LWA_ALPHA);
				//	SetLayeredWindowAttributes(hTextbox, RGB(255, 255, 255), 128, LWA_ALPHA);
				//}
				break;//
			case '0':case VK_NUMPAD0:
				if(kb[VK_CONTROL])//reset zoom
				{
					logzoom=0, zoom=1;
					render();
				}
				break;
			case VK_OEM_PLUS:case VK_ADD://=, numpad+
				if(kb[VK_CONTROL])//zoom in
				{
					++logzoom, zoom*=2;
					render();
				}
				break;
			case VK_OEM_MINUS:case VK_SUBTRACT://-, numpad-
				if(kb[VK_CONTROL])//zoom out
				{
					--logzoom, zoom*=0.5;
					render();
				}
				break;
			case 'O':
				if(kb[VK_CONTROL])//open
					open_media();
				break;
			case 'S':
				if(kb[VK_CONTROL])
				{
					if(kb[VK_SHIFT])//save as
						save_media_as();
					else//save
					{
						if(filepath.size())
							save_media(filepath);
						else
							save_media_as();
					}
				}
				break;
			case 'X':
				if(kb[VK_CONTROL])//cut
					if(editcopy())
					{
						selection_remove();
						render();
					}
				break;
			case 'C':
				if(kb[VK_CONTROL])//copy
					editcopy();
				break;
			case 'V':
				if(kb[VK_CONTROL])//paste
				{
					editpaste();
					render();
				}
				break;
			case VK_ESCAPE:
				if(sel_start!=sel_end)//deselect
				{
					selection_deselect();
					selection_remove();
				}
				else if(currentmode==M_TEXT)
				{
					clear_textbox();
					remove_textbox();
				//	exit_textmode();
				}
				else if(currentmode==M_POLYGON)
					polygon.clear(), use_temp_buffer=false;
				if(colorbarcontents!=CS_TEXTOPTIONS)
					replace_colorbarcontents(CS_NONE);
				render();
				break;
			case VK_DELETE:
				if(sel_start!=sel_end)//delete selection
				{
					selection_remove();
					render();
				}
				break;
			case 'A':
				if(kb[VK_CONTROL])//select all
				{
					selection_selectall();
					render();
				}
				break;
			case 'G':
				if(kb[VK_CONTROL])//toggle grid
				{
					showgrid=!showgrid;
					CheckMenuItem(hMenuZoom, IDSM_ZOOM_SHOW_GRID, showgrid?MF_CHECKED:MF_UNCHECKED);//deprecated
					render();
				}
				break;
			case 'Z':
				if(kb[VK_CONTROL])//undo
				{
					hist_undo(image);
					render();
				}
				break;
			case 'Y':
				if(kb[VK_CONTROL])//redo
				{
					hist_redo(image);
					render();
				}
				break;
			case 'I':
				if(kb[VK_CONTROL])//invert color, leave alpha
				{
					hist_premodify(image, iw, ih);
					if(sel_start!=sel_end)
						invertcolor(sel_buffer, sw, sh, true);
					else
						invertcolor(image, iw, ih);
					render();
				}
				break;
			case 'N':
				if(kb[VK_CONTROL])//clear image
				{
					if(kb[VK_SHIFT])
					{
						hist_premodify(image, iw, ih);
						memset(image, 0xFF, image_size<<2);
						render();
					}
					else//new image
					{
						image=hist_start(iw, ih);//TODO: save question
						memset(image, 0xFF, image_size<<2);
						render();
					}
				}
				else//fill with noise
				{
					hist_premodify(image, iw, ih);
					for(int k=0;k<image_size;++k)
					//	image[k]=0xFFFFFFFF;
						image[k]=0xFF000000|rand()<<15|rand();
					//	image[k]=rand()<<30|rand()<<15|rand();
					render();
				}
				break;
			}
		}
		break;
	case WM_KEYUP:case WM_SYSKEYUP:
		kb[wParam]=false;
		break;
	case WM_CLOSE:
//	case WM_DESTROY:
		if(modified)
		{
			swprintf_s(g_wbuf, L"Save changes to %s?", filename.c_str());
			int result=MessageBoxW(hWnd, g_wbuf, L"Paint++", MB_YESNOCANCEL);
			switch(result)
			{
			case IDYES:
				if(filepath.size())
				{
					save_media(filepath);
					PostQuitMessage(0);
				}
				else if(save_media_as())
					PostQuitMessage(0);
				else
					return 0;
				break;
			case IDNO:
				PostQuitMessage(0);
				break;
			case IDCANCEL:
				return 0;
			}
		}
		else
			PostQuitMessage(0);
		break;
	}
#if !defined RELEASE && defined _DEBUG
	if(debugmsg)
		GUIPrint(ghDC, 0, 0, debugmsg);
	static const int nmessages=10;
	struct WMessage
	{
		short message, mx, my;
		WMessage():message(), mx(), my(){}
		WMessage(short message, short mx, short my):message(message), mx(mx), my(my){}
		void set(short message, short mx, short my){this->message=message, this->mx=mx, this->my=my;}
	};
	static WMessage messages[nmessages];
	static int m_idx=0;
	messages[m_idx].set(message, mx, my);
	for(int k=0;k<nmessages;++k)
	{
		auto &msg=messages[k];
		GUIPrint(ghDC, w>>1, k<<4, "0x%04X, (%d, %d)%s", msg.message, msg.mx, msg.my, k==m_idx?" <":"    ");
	//	GUIPrint(ghDC, w>>1, k<<4, "0x%04X%s", msg.message, k==m_idx?" <":"    ");
	}
	m_idx=(m_idx+1)%nmessages;
	GUIPrint(ghDC, (w>>1)+200, 0, "drag=%d", (int)drag);//

//	GUIPrint(ghDC, w>>1, 0, "message=0x%04X, drag=%d", message, (int)drag);//
#endif
	Rectangle(ghDC, -1, h-17, w+1, h+1);
//	Rectangle(ghDC, 0, h-16, w, h);
	if(mousepos==MP_IMAGE)
	{
		Point pm;
		screen2image(mx, my, pm.x, pm.y);
		if(!icheck(pm.x, pm.y))
		{
			auto p=(byte*)(image+iw*pm.y+pm.x);
			int a=p[3], r=p[2], g=p[1], b=p[0];
			GUITPrint(ghDC, 0, h-16, "XY=(%d, %d),\tARGB=(%d, %d, %d, %d)\t=0x%02X%02X%02X%02X", pm.x, pm.y, a, r, g, b, a, r, g, b);
		}
	}
	else if(mousepos==MP_COLORBAR)
	{
		if(mx>=31&&mx<255&&my>=h-65&&my<h-33)
		{
			int kx=(mx-31)>>4, ky=(my-(h-65))>>4, idx=kx<<1|ky;
			auto p=(byte*)(colorpalette+idx);
			int r=p[2], g=p[1], b=p[0];
			GUITPrint(ghDC, 0, h-16, "RGB=(%d, %d, %d)\t=0x%02X%02X%02X", r, g, b, r, g, b);
		}
	}
	{
		int size=0;
		for(int k=0, nv=history.size();k<nv;++k)
			size+=history[k].iw*history[k].ih;
		GUIPrint(ghDC, w-200, h-16, "%d/%d, %.2lfMB", histpos, history.size(), float(size<<2)/1048576);//
	}
	if(handled)
		return r;
	return DefWindowProcA(hWnd, message, wParam, lParam);
}
int				__stdcall FontProc(LOGFONTW const *lf, TEXTMETRICW const *tm, unsigned long FontType, long lParam)
{
	fonts.push_back(Font(FontType, lf, tm));
	return true;//nonzero to continue enumeration
}
int				__stdcall WinMain(HINSTANCE__ *hInstance, HINSTANCE__ *hPrevInstance, char *lpCmdLine, int nCmdShow)
{
	ghInstance=hInstance;
	tagWNDCLASSEXA wndClassEx=
	{
		sizeof(tagWNDCLASSEXA),
		CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS,
		WndProc, 0, 0, hInstance,

		LoadIconA(nullptr, (char*)0x00007F00),//IDI_APPLICATION

	//	LoadCursorA(nullptr, IDC_CROSS),
		LoadCursorA(nullptr, (char*)0x00007F00),//IDC_ARROW

		(HBRUSH__*)(COLOR_WINDOW+1), 0, "New format", 0
	};
	RegisterClassExA(&wndClassEx);
	ghWnd=CreateWindowExA(WS_EX_ACCEPTFILES, wndClassEx.lpszClassName, "Untitled - Paint++", WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);//2020-06-02

	ShowWindow(ghWnd, nCmdShow);
	
	//	prof_start();
		programpath.resize(MAX_PATH);
		GetModuleFileNameW(0, &programpath[0], MAX_PATH);
		for(int k=programpath.size()-1;k>=0;--k)
		{
			if(programpath[k]==L'/'||programpath[k]==L'\\')
			{
				programpath.resize(k+1);
				break;
			}
		}
	//	prof_add("path");
		read_settings();
		if(!check_FFmpeg_path(FFmpeg_path))
			ask_FFmpeg_path();
	//	prof_add("read settings");

	//	wchar_t *cmdargs=GetCommandLineW();
		int nArgs;
		wchar_t **args=CommandLineToArgvW(GetCommandLineW(), &nArgs);
		if(nArgs>1)
		{
			std::wstring filepath_u16=args[1];
			open_media(filepath_u16);
		}

		//LARGE_INTEGER li;
		//QueryPerformanceCounter(&li);
		//t_start=li.QuadPart;

		ghDC=GetDC(ghWnd);
		ghMemDC=CreateCompatibleDC(ghDC);
		//e=(Engine*)&lgl;
		GetClientRect(ghWnd, &R);
		h=R.bottom-R.top, w=R.right-R.left;
		BITMAPINFO bmpInfo={{sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0}};
		hBitmap=CreateDIBSection(0, &bmpInfo, DIB_RGB_COLORS, (void**)&rgb, 0, 0);
		hBitmap=(HBITMAP)SelectObject(ghMemDC, hBitmap);

		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		unsigned long gdiplusToken;
		Gdiplus::Status state=Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
	//	centerP.x=X0=w/2, centerP.y=Y0=h/2;
	//	ClientToScreen(ghWnd, &centerP);
	//	e->initiate();
	//	prof_add("init");

		for(int k=0;k<16;++k)
			ext_colorpalette[k]=0xFFFFFF;

		iw=400, ih=300, image_size=iw*ih;
		image=hist_start(iw, ih);
	//	image=(int*)malloc(image_size<<2);
		//for(int ky=0;ky<ih;++ky)
		//{
		//	for(int kx=0;kx<iw;++kx)
		//	{
		//		unsigned char c=(kx<<4&0x7F)+(ky<<4&0x7F);
		//		image[iw*ky+kx]=c<<16|c<<8|c;
		//	}
		//}
		memset(image, 0xFF, image_size<<2);
		//for(int k=0;k<image_size;++k)
		//	image[k]=0xFF000000|rand()<<15|rand();
		//	image[k]=rand()<<30|rand()<<15|rand();
		//	image[k]=0xFFFFFFFF;

		//int color_outline=0xFF00FF;
		//draw_line(0, 0, 0, ih-1, color_outline);
		//draw_line(0, ih-1, iw-1, ih-1, color_outline);
		//draw_line(iw-1, ih-1, iw-1, 0, color_outline);
		//draw_line(iw-1, 0, 0, 0, color_outline);

	//	horiginalcursor=GetCursor();

		tagLOGFONTW lf=//load fonts
		{
			0, 0,//h, w
			100,//escapement (*0.1 degree)
			0,//orientation
			0,//weight
			0, 0, 0,//italic, underline, strikeout
			0,//charset
			0, 0, 0,//outprecision, clipprecision, quality
			0,//pitch and family
			{L'\0'},//facename
		};
		EnumFontFamiliesExW(ghMemDC, &lf, FontProc, 0, 0);
		std::sort(fonts.begin(), fonts.end());
		for(int k=0, nfonts=fonts.size();k<nfonts;++k)
			SendMessageW(hFontcombobox, CB_ADDSTRING, 0, (LPARAM)fonts[k].lf.lfFaceName);
		font_idx=SendMessageW(hFontcombobox, CB_SETCURSEL, 0, 0);
		font_size=20;
		set_sizescombobox(font_idx);

	tagMSG msg;
	for(;GetMessageA(&msg, 0, 0, 0);)TranslateMessage(&msg), DispatchMessageA(&msg);

	//	free_image(), free_frames();
		Gdiplus::GdiplusShutdown(gdiplusToken);
		if(hBitmap)
		{
			hBitmap=(HBITMAP)SelectObject(ghMemDC, hBitmap);
			DeleteObject(hBitmap);
		}
		if(hFont)
			DeleteObject(hFont);
		DeleteDC(ghMemDC);
		ReleaseDC(ghWnd, ghDC);

		delete_workfolder();

	return msg.wParam;//*/
}