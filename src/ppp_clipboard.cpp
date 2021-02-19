#include		"ppp.h"
#include		"generic.h"
#include		<sstream>
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
void			syscolors_to_clipboard()
{
	const int ncolors=31;
	int colors[ncolors]={0};
	const char *names[ncolors]={nullptr};
#define	GETNAMEDCOLOR(COLOR)	names[COLOR]=#COLOR, colors[COLOR]=GetSysColor(COLOR)
	GETNAMEDCOLOR(COLOR_SCROLLBAR);
	GETNAMEDCOLOR(COLOR_BACKGROUND);
	GETNAMEDCOLOR(COLOR_ACTIVECAPTION);
	GETNAMEDCOLOR(COLOR_INACTIVECAPTION);
	GETNAMEDCOLOR(COLOR_MENU);
	GETNAMEDCOLOR(COLOR_WINDOW);
	GETNAMEDCOLOR(COLOR_WINDOWFRAME);
	GETNAMEDCOLOR(COLOR_MENUTEXT);
	GETNAMEDCOLOR(COLOR_WINDOWTEXT);
	GETNAMEDCOLOR(COLOR_CAPTIONTEXT);
	GETNAMEDCOLOR(COLOR_ACTIVEBORDER);
	GETNAMEDCOLOR(COLOR_INACTIVEBORDER);
	GETNAMEDCOLOR(COLOR_APPWORKSPACE);
	GETNAMEDCOLOR(COLOR_HIGHLIGHT);
	GETNAMEDCOLOR(COLOR_HIGHLIGHTTEXT);
	GETNAMEDCOLOR(COLOR_BTNFACE);
	GETNAMEDCOLOR(COLOR_BTNSHADOW);
	GETNAMEDCOLOR(COLOR_GRAYTEXT);
	GETNAMEDCOLOR(COLOR_BTNTEXT);
	GETNAMEDCOLOR(COLOR_INACTIVECAPTIONTEXT);
	GETNAMEDCOLOR(COLOR_BTNHIGHLIGHT);

	GETNAMEDCOLOR(COLOR_3DDKSHADOW);
	GETNAMEDCOLOR(COLOR_3DLIGHT);
	GETNAMEDCOLOR(COLOR_INFOTEXT);
	GETNAMEDCOLOR(COLOR_INFOBK);

	GETNAMEDCOLOR(COLOR_HOTLIGHT);
	GETNAMEDCOLOR(COLOR_GRADIENTACTIVECAPTION);
	GETNAMEDCOLOR(COLOR_GRADIENTINACTIVECAPTION);

	GETNAMEDCOLOR(COLOR_MENUHILIGHT);
	GETNAMEDCOLOR(COLOR_MENUBAR);
#undef GETNAMEDCOLOR
	std::stringstream LOL_1;
	for(int k=0;k<ncolors;++k)
	{
		LOL_1<<k;
		if(names[k])
		{
			sprintf_s(g_buf, g_buf_size, "\t%s\t0x%08X", names[k], colors[k]);
			LOL_1<<g_buf;
		//	LOL_1<<'\t'<<names[k]<<'\t'<<colors[k];
		}
		LOL_1<<"\r\n";
	}
	copy_to_clipboard(LOL_1.str());
}