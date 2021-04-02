//ppp.cpp - Paint++ main implementation file.
//UI design based on MSPaint for Windows XP by Microsoft.
//Icons from MSPaint for Windows XP.
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include		"ppp.h"
#include		"generic.h"
#include		"ppp_inline_blend.h"

#include		<Windows.h>

#include		<Gdiplus.h>
#include		<Gdiplusimaging.h>
//#include		<wcsplugin.h>//windows color system
#include		<commctrl.h>
#include		<sys\stat.h>
#include		<string>
#include		<cwctype>
#include		<sstream>
#include		<fstream>
#include		<algorithm>
#include		<vector>
//#include		<queue>
#pragma			comment(lib, "Comctl32.lib")//status bar
#pragma			comment(lib, "Gdiplus.lib")
//#define		SDL_MAIN_HANDLED
//#include		<SDL.h>
//#include		<SDL_syswm.h>
//#pragma		comment(lib, "SDL2.lib")
//#pragma		comment(lib, "SDL2main.lib")

#include		<iostream>
//#include		<codecvt>
#include		<assert.h>
#include		<exception>
#include		<functional>
#include		<time.h>
#include		<strsafe.h>
//#pragma		comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

static const char file[]=__FILE__;
int				cfocus=FOCUS_MAIN;
int				g_processed=0;//
#ifdef _DEBUG
char			debugmode=true;
#else
char			debugmode=false;
#endif
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
int				tempformat=IF_PNG;
const wchar_t	*settingsfilename=L"settings.txt", *imageformats[]={L"BMP", L"TIFF", L"PNG", L"JPEG", L"FLIF"};
std::vector<std::wstring> framenames;//file names of all frames, must append workfolder with possible doublequote
std::vector<int*> thumbnails;//same size as framenames
int				th_w=0, th_h=0,//thumbnail dimensions		TODO: th -> thumb
				thbox_h=0,//thumbnail box internal height
				thbox_x1=0,//thumbnail box xstart
				thbox_posy=0;//thumbnail box scroll position (thbox top -> screen top)

extern const int scrollbarwidth=17;
Scrollbar		vscroll={}, hscroll={},//image scrollbars
				thbox_vscroll={};
//bool			thbox_scrollbar=false;
//int			thbox_vscroll_my_start=0,//starting mouse position
//				thbox_vscroll_s0=0,//initial scrollbar slider start
//				thbox_vscroll_start=0,//scrollbar slider start
//				thbox_vscroll_size=0;//scrollbar slider size
int				current_frame=0, nframes=0,
				framerate_num=0, framerate_den=1;
bool			animated=false;
tagRECT			R;
HINSTANCE		ghInstance=0;
HWND			ghWnd=nullptr;
//HWND			hFontbar=nullptr;
HWND			hFontcombobox=nullptr, hFontsizecb=nullptr, hTextbox=nullptr,
				hRotatebox=nullptr;
//				hStretchHbox=nullptr, hStretchVbox=nullptr, hSkewHbox=nullptr, hSkewVbox=nullptr;
//HWND			gToolbar=0, gColorBar=0, ghStatusbar=0, hWndPanel=0;
HDC				ghDC=nullptr, ghMemDC=nullptr;
HBITMAP			hBitmap=nullptr;
std::wstring	programpath, FFmpeg_path;//all paths use forward slashes only, and are stripped of doublequotes and trailing slash
//bool			FFmpeg_path_has_quotes=false, workspace_has_quotes=false;
std::string		FFmpeg_version;
std::wstring	filename=L"Untitled", filepath;//opened media file name & path, utf16
const char		default_title[]="Untitled - Paint++";
extern const wchar_t doublequote=L'\"';
char			kb[256]={false},
//				mb[2]={false},//mouse buttons {0: left, 1: right}
				drag=0, prev_drag=0;//drag type
int				timer=0;
//bool			hscrollbar=false, vscrollbar=false;
//int			hscroll_mx_start=0, vscroll_my_start=0,//starting mouse position
//				hscroll_s0=0, vscroll_s0=0,//initial scrollbar slider start
//				hscroll_start=0, vscroll_start=0,//scrollbar slider start
//				hscroll_size=0, vscroll_size=0;//scrollbar slider size

bool			operator<(Font const &a, Font const &b){return wcscmp(a.lf.lfFaceName, b.lf.lfFaceName)<0;}
const int		truetypesizes[]={8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72};
extern const int truetypesizes_size=sizeof(truetypesizes)>>2;
std::vector<Font> fonts;
int				font_idx=0, font_size=10;
bool			bold=false, italic=false, underline=false;
HFONT			hFont=nullptr;

int				scrollbar_init_ms=500, scrollbar_tick_ms=35;
void			timer_set(int id)
{
	GEN_ASSERT(!timer);
	int elapse=10;
	switch(id)
	{
	case TIMER_AIRBRUSH:
		elapse=airbrush_timer;
		break;
	case TIMER_SCROLLBAR_INIT:
		elapse=scrollbar_init_ms;
		break;
	case TIMER_SCROLLBAR_CONT:
		elapse=scrollbar_tick_ms;
		break;
	}
	int ret=SetTimer(ghWnd, id, elapse, 0);
	SYS_ASSERT(ret);
	timer=id;
}
void			timer_kill()
{
	if(timer)
	{
		int ret=KillTimer(ghWnd, timer);
		SYS_ASSERT(ret);
		timer=0;
	}
	else
		LOG_ERROR("KillTimer(), but there is no timer.");//
}
void			unimpl(){messagebox(ghWnd, L"Error", L"Not implemented yet.");}
void			mark_modified()
{
	if(!modified)
	{
		modified=true;
		int size=GetWindowTextW(ghWnd, g_wbuf+1, g_buf_size-1);
		SYS_ASSERT(size);
		if(g_wbuf[1]!=L'*')
		{
			g_wbuf[0]=L'*', g_wbuf[1+size]=L'\0';
			size=SetWindowTextW(ghWnd, g_wbuf);
			SYS_ASSERT(size);
		}
	}
}

void			ask_FFmpeg_path()
{
	log_start(LL_CRITICAL);
	logi(LL_CRITICAL,
		"FFmpeg not found.\n"

		"Please download FFmpeg for Windows from:\n"
		"\n"
		"\tffmpeg.org/download.html\n"

		//"Please download FFmpeg from:\n"
		//"\n"
		//"\tffmpeg.zeranoe.com/builds\n"
		//"\n"
		//"In the website, choose:\n"
		//"\n"
		//"\tVersion: <latest>\n"
		//"\tArchitecture: 64bit\n"
		//"\tLinking: static\n"

		"\n"
		"and specify the path to FFmpeg and FFprobe here, or in:\n"
		"\n"
		"\t%ls\n"
		"\n"
		"> ", (programpath+L'/'+settingsfilename).c_str());
	for(;;)
	{
		std::string str;
		std::getline(std::cin, str);
	//	std::cin>>str;//useless
		if(check_FFmpeg_path(str))//if valid path entered, write it in 'settings.txt'
		{
			assign_path(str, 0, str.size(), FFmpeg_path);
			//FFmpeg_path_has_quotes=false;
			//for(int k=0, kEnd=str.size();k<kEnd;++k)
			//{
			//	if(str[k]==' ')
			//	{
			//		FFmpeg_path_has_quotes=true;
			//		break;
			//	}
			//}
			//if(FFmpeg_path_has_quotes)
			//{
			//	if(str[0]!='\"')
			//		str.insert(str.begin(), '\"');
			//}
			//FFmpeg_path.assign(str.begin(), str.end());
			//std::string str2="FFmpeg path: "+str;
			//if(FFmpeg_path_has_quotes)
			//	str2+='\"';
			//str2+="\nWorkspace path:";
			//write_file((programpath+settingsfilename).c_str(), str2);
			break;
		}
		else//check 'settings.txt'
		{
			read_settings();
		//	read_utf8_file((programpath+settingsfilename).c_str(), FFmpeg_path);
			if(check_FFmpeg_path(FFmpeg_path))
				break;
		}
		logi(LL_CRITICAL,
			"\nFFmpeg not found.\n"
			"> ");
	}
	logi(LL_CRITICAL,
		"\n"
		"Found FFmpeg version %s\n"
		"Press any key to continue...\n", FFmpeg_version.c_str());
	log_pause(LL_CRITICAL);
	log_end();
}


enum			MousePosition
{
	MP_NONCLIENT,
	MP_THUMBNAILBOX, MP_COLORBAR, MP_TOOLBAR,

	MP_VSCROLL, MP_HSCROLL, MP_IMAGEWINDOW,
	MP_RESIZE_TL, MP_RESIZE_TOP, MP_RESIZE_TR, MP_RESIZE_LEFT, MP_RESIZE_RIGHT, MP_RESIZE_BL, MP_RESIZE_BOTTOM, MP_RESIZE_BR,
	MP_IMAGE
};
int				mousepos=-1;
HMENU			hMenubar=0,		hMenuFile=0, hMenuEdit=0, hMenuView=0, hMenuZoom=0, hMenuImage=0, hMenuColors=0, hMenuHelp=0;
//HMENU			hMenuTest=0;

HCURSOR			hcursor_original=(HCURSOR)LoadCursorW(nullptr, IDC_ARROW),
				hcursor_sizewe=(HCURSOR)LoadCursorW(nullptr, IDC_SIZEWE),
				hcursor_sizens=(HCURSOR)LoadCursorW(nullptr, IDC_SIZENS),
				hcursor_sizenwse=(HCURSOR)LoadCursorW(nullptr, IDC_SIZENWSE),
				hcursor_sizenesw=(HCURSOR)LoadCursorW(nullptr, IDC_SIZENESW),
				hcursor_cross=(HCURSOR)LoadCursorW(nullptr, IDC_CROSS),//TODO: mspaint cursors
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

int				currentmode=M_PENCIL, prevmode=M_PENCIL,
				selection_free=false,//true: free-form selection, false: rectangular selection
				selection_transparency=TRANSPARENT,//2: opaque or 1: transparent
				selection_persistent=false,//selection remains after switching to another tool
				erasertype=ERASER08,
				magnifier_level=4,//log pixel size {0:1, 1:2, 2:4, 3:8, 4:16, 5:32}		default was 0
				brushtype=BRUSH_DISK,
				airbrush_size=S_SMALL, airbrush_cpt=18, airbrush_timer=10,
				linewidth=1,//[1, 5]
				rectangle_type=ST_HOLLOW, polygon_type=ST_HOLLOW, ellipse_type=ST_HOLLOW, roundrect_type=ST_HOLLOW,
				interpolation_type=I_BILINEAR;
char			magnification_levels[]=
{
	-1, 1,
	 2, 3,
	 4, 5
};

Rect			selection, sel0;//image coordinates, i: buffer start, f: buffer end (not in order)
//Point			sel_start, sel_end,//selection (start: fixed, end: moving), image coordinates			//TODO: use struct Rect
//				sel_s1, sel_s2,//ordered screen coordinates for mouse test
//				selpos,//position of top-left corner of selection in image coordinates
//				selid;//selection image dimensions
//bool			selection_moved=false;//deprecated

Rect			textrect;//image coordinates, sorted
//Point			text_start, text_end,//static point -> moving point, image coordinates
//				text_s1, text_s2,//ordered screen coordinates
//				textpos;//position of top-right corner of textbox in image coordinates

#include		"ppp_inline_check.h"
int				pick_color_mouse(int *buffer, int x0, int y0)
{
	int ix, iy;
	screen2image(mx, my, ix, iy);
	if(!icheck(ix, iy))
		return buffer[iw*iy+ix];
	return 0xFFFFFF;
}

extern const char anchors[]=//anchor: -1: resize from start, 0: no resize, 1: resize from end
{//	 x   y
	-1, -1,//TL
	 0, -1,//top
	 1, -1,//TR
	-1,  0,//left
	 1,  0,//right
	-1,  1,//BL
	 0,  1,//bottom
	 1,  1,//BR
};
void			imresize_setparams(char anchor, int delta, int oldsize, int &newsize, int &srcstart, int &dststart, int &copysize)
{
	if(anchor==-1)//resize from start
	{
		newsize=oldsize-conditional_negate(delta, kb[VK_SHIFT]);
		//newsize=oldsize-conditional_negate(imouse, kb[VK_SHIFT]);
		if(newsize<1)
			newsize=1;
		int dx=newsize-oldsize;
		srcstart=-dx&-(dx<0), dststart=dx&-(dx>0), copysize=oldsize-srcstart;
		//if(dx>0)
		//	srcstart=0, dststart=dx, copysize=oldsize;
		//else if(!dx)
		//	srcstart=0, dststart=0, copysize=oldsize;
		//else//dx<0
		//	srcstart=-dx, dststart=0, copysize=oldsize+dx;
	}
	else if(!anchor)//no resizing of dimension
	{
		newsize=oldsize;
		srcstart=0, dststart=0, copysize=oldsize;
	}
	else//resize from end
	{
		newsize=oldsize+delta;
		//newsize=imouse;
		if(newsize<1)
			newsize=1;
		srcstart=0, dststart=0, copysize=minimum(newsize, oldsize);
	}
}
void			imresize_setparams(Point &newsize, Point &srcstart, Point &dststart, Point &copysize)
{
	char xanchor=anchors[(drag-D_RESIZE_TL)<<1], yanchor=anchors[(drag-D_RESIZE_TL)<<1|1];//-1: resize from start, 0: original, 1: resize from end
	Point mouse(mx, my), prevmouse(start_mx, start_my);//mouse coordinates
	mouse.screen2image_rounded();
	prevmouse.screen2image_rounded();
	imresize_setparams(xanchor, mouse.x-prevmouse.x, iw, newsize.x, srcstart.x, dststart.x, copysize.x);
	imresize_setparams(yanchor, mouse.y-prevmouse.y, ih, newsize.y, srcstart.y, dststart.y, copysize.y);
}
void			imresize(int *im2, Point const &newsize, int const *im, Point const &oldsize, Point const &srcstart, Point const &dststart, Point const &copysize)
{
	Point dstend=dststart+copysize;
	if(newsize.x*dststart.y>2)
		memset(im2, 0xFF, newsize.x*dststart.y<<2);
	for(int ky=0;ky<copysize.y;++ky)
	{
		int *dst=im2+newsize.x*(dststart.y+ky);
		if(dststart.x>0)
			memset(dst, 0xFF, dststart.x<<2);
		if(copysize.x>0)
			memcpy(dst+dststart.x, im+oldsize.x*(srcstart.y+ky)+srcstart.x, copysize.x<<2);
		if(newsize.x-dstend.x>0)
			memset(dst+dstend.x, 0xFF, (newsize.x-dstend.x)<<2);
	}
	if(newsize.y-dstend.y>0)
		memset(im2+newsize.x*dstend.y, 0xFF, newsize.x*(newsize.y-dstend.y)<<2);
}
//void			sort_using_anchor(char anchor, int &start, int &end)
//{
//	if(anchor==-1)//resize from start
//	{
//		if(start<end)
//			std::swap(start, end);
//	}
//	else if(anchor==1)//resize from end
//	{
//		if(start>end)
//			std::swap(start, end);
//	}
//}
int				assign_using_anchor(char anchor, int &start, int &end, int mouse, int prevmouse)//returns true if flipped
{
#if 1
	int delta=mouse-prevmouse;
	if(anchor==-1)//resize from least significant side
	{
		if(start<end)
		{
			start+=delta;
			return start>end;
		}
		else
		{
			end+=delta;
			return start<end;
		}
	}
	else if(anchor==1)//resize from most significant side
	{
		int flipped=mouse>end;
		if(start<end)
		{
			end+=delta;
			return start>end;
		}
		else
		{
			start+=delta;
			return start<end;
		}
	}
	return false;
#endif
	//int delta=mouse-prevmouse;
	//if(anchor==-1)
	//{
	//	if(start<end)
	//		start+=delta;
	//	else
	//		end+=delta;
	//}
	//else if(anchor==1)
	//{
	//	if(start<end)
	//		end+=delta;
	//	else
	//		start+=delta;
	//}
#if 0
	if(anchor==1)
		std::swap(fixed, moving);
	if(anchor)
	{
		if((fixed<moving)!=(fixed<movingprev))//got flipped
		{
			if(size>=0)
				start=maximum(fixed, moving), size=-abs(fixed-moving);
			else
				start=minimum(fixed, moving), size=abs(fixed-moving);
		}
		else//didn't get flipped
		{
			if(size>=0)
				start=minimum(fixed, moving), size=abs(fixed-moving);
			else
				start=maximum(fixed, moving), size=-abs(fixed-moving);
		}
	}
#endif
	//if(anchor==-1)
	//	start=moving, size=fixed-moving;
	//else if(anchor==1)
	//	start=fixed, size=moving-fixed;
}
Point			newsize, srcstart, dststart, copysize;//globals for debugging
char			xanchor, yanchor;

#if 0//cleartype test
void			ctt_clear(int x, int y, int dx, int dy, int color)
{
	for(int ky=0;ky<dy;++ky)
		for(int kx=0;kx<dx;++kx)
			rgb[w*(y+ky)+x+kx]=color;
}
void			ctt_copy(short x1, short y1, short dx, short dy, short x2, short y2)
{
	for(int ky=0;ky<dy;++ky)
		for(int kx=0;kx<dx;++kx)
			rgb[w*(y2+ky)+x2+kx]=rgb[w*(y1+ky)+x1+kx];
}
void			ctt_copy_mul(short x1, short y1, short dx, short dy, short x2, short y2)
{
	for(int ky=0;ky<dy;++ky)
	{
		for(int kx=0;kx<dx;++kx)
		{
			auto p=(unsigned char*)(rgb+w*(y1+ky)+x1+kx), p2=(unsigned char*)(rgb+w*(y2+ky)+x2+kx);
		//	p2[0]=p[0]*p2[0]>>8;
		//	p2[1]=p[1]*p2[1]>>8;
		//	p2[2]=p[2]*p2[2]>>8;
			p2[0]=p[0]*p2[0]/255;
			p2[1]=p[1]*p2[1]/255;
			p2[2]=p[2]*p2[2]/255;
		}
	}
}
void			ctt_absdiff(short x1, short y1, short x2, short y2, short dx, short dy, short xd, short yd)//pd = p1-p2
{
	for(int ky=0;ky<dy;++ky)
	{
		for(int kx=0;kx<dx;++kx)
		{
			auto p=(unsigned char*)(rgb+w*(y1+ky)+x1+kx),
				p2=(unsigned char*)(rgb+w*(y2+ky)+x2+kx),
				pd=(unsigned char*)(rgb+w*(yd+ky)+xd+kx);
			pd[0]=abs(p[0]-p2[0]);
			pd[1]=abs(p[1]-p2[1]);
			pd[2]=abs(p[2]-p2[2]);
		}
	}
}
void			ctt_print(short gx, short gy, short dx, short dy, short xd, short yd, int txtcolor)//X
{
	auto t=(unsigned char*)&txtcolor;
	float inv255=1.f/255;
	for(int ky=0;ky<dy;++ky)
	{
		for(int kx=0;kx<dx;++kx)
		{
			auto g=(unsigned char*)(rgb+w*(gy+ky)+gx+kx),//glyph
				pd=(unsigned char*)(rgb+w*(yd+ky)+xd+kx);//dst
			float gl_FragColor[]=//frag shader
			{
				t[0]*g[2]*inv255,//gl_FragColor=txtcolor*atlas;
				t[1]*g[1]*inv255,
				t[2]*g[0]*inv255,
				(g[0]+g[1]+g[2])*1.f/3*inv255,//pre-calculated
			};
			//alpha blend
		//	pd[0]=(1-gl_FragColor[3])*pd[0]+gl_FragColor[3]*gl_FragColor[0];
			pd[0]=(unsigned char)(pd[0]+gl_FragColor[3]*(gl_FragColor[0]-pd[0]));
			pd[1]=(unsigned char)(pd[1]+gl_FragColor[3]*(gl_FragColor[1]-pd[1]));
			pd[2]=(unsigned char)(pd[2]+gl_FragColor[3]*(gl_FragColor[2]-pd[2]));

			//if((unsigned char)(gl_FragColor[3]*(pd[0]-gl_FragColor[0]))>0)//
			//if(kx==0&&ky==8)
			//if(gx==17&&kx==2&&ky==7)
			//	int LOL_1=0;
			//if(gx==17&&kx==2&&ky==8)
			//	int LOL_2=0;
		}
	}
}
void			ctt_print15(short gx, short gy, short dx, short dy, short xd, short yd, int txtcolor)//transparent
{
	auto t=(unsigned char*)&txtcolor;
	float inv255=1.f/255;
	for(int ky=0;ky<dy;++ky)
	{
		for(int kx=0;kx<dx;++kx)
		{
			auto g=(unsigned char*)(rgb+w*(gy+ky)+gx+kx),//glyph
				pd=(unsigned char*)(rgb+w*(yd+ky)+xd+kx);//dst
			float gl_FragColor[]=//frag shader
			{
				t[0],//gl_FragColor=vec4(txtcolor.rgb, atlas.a);
				t[1],
				t[2],
				(g[0]+g[1]+g[2])*1.f/3*inv255,//pre-calculated
			};
			//alpha blend
		//	pd[0]=(1-gl_FragColor[3])*pd[0]+gl_FragColor[3]*gl_FragColor[0];
			pd[0]=(unsigned char)(pd[0]+gl_FragColor[3]*(gl_FragColor[0]-pd[0]));
			pd[1]=(unsigned char)(pd[1]+gl_FragColor[3]*(gl_FragColor[1]-pd[1]));
			pd[2]=(unsigned char)(pd[2]+gl_FragColor[3]*(gl_FragColor[2]-pd[2]));
		}
	}
}
void			ctt_print2(short gx, short gy, short dx, short dy, short xd, short yd, int txtcolor, int bkcolor)//opaque-only
{
	auto t=(unsigned char*)&txtcolor, bk=(unsigned char*)&bkcolor;
	float inv255=1.f/255;
	for(int ky=0;ky<dy;++ky)
	{
		for(int kx=0;kx<dx;++kx)
		{
			auto g=(unsigned char*)(rgb+w*(gy+ky)+gx+kx),//glyph
				pd=(unsigned char*)(rgb+w*(yd+ky)+xd+kx);//dst
			unsigned char gl_FragColor[]=//frag shader
			{
				(unsigned char)(bk[0]+g[0]*inv255*(t[0]-bk[0])),//gl_FragColor=mix(bk, txt, atlas);
				(unsigned char)(bk[1]+g[1]*inv255*(t[1]-bk[1])),
				(unsigned char)(bk[2]+g[2]*inv255*(t[2]-bk[2])),
				(unsigned char)(bk[3]+g[3]*inv255*(t[3]-bk[3])),//unused
			};
			//directional min blend
			pd[0]=t[0]<bk[0]?pd[0]<gl_FragColor[0]?pd[0]:gl_FragColor[0]:pd[0]>gl_FragColor[0]?pd[0]:gl_FragColor[0];
			pd[1]=t[1]<bk[1]?pd[1]<gl_FragColor[1]?pd[1]:gl_FragColor[1]:pd[1]>gl_FragColor[1]?pd[1]:gl_FragColor[1];
			pd[2]=t[2]<bk[2]?pd[2]<gl_FragColor[2]?pd[2]:gl_FragColor[2]:pd[2]>gl_FragColor[2]?pd[2]:gl_FragColor[2];
			//min blend
			//pd[0]=pd[0]<gl_FragColor[0]?pd[0]:gl_FragColor[0];
			//pd[1]=pd[1]<gl_FragColor[1]?pd[1]:gl_FragColor[1];
			//pd[2]=pd[2]<gl_FragColor[2]?pd[2]:gl_FragColor[2];
		}
	}
}
void			emulate_cleartype()
{
//	Rectangle(ghMemDC, -1, -1, w, h);
	HFONT hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);

	hFont=(HFONT)SelectObject(ghMemDC, hFont);//test 2
	int txtcolor=SetTextColor(ghMemDC, 0xFFFFFF), bkcolor=SetBkColor(ghMemDC, 0x000000);//white on black
	GUIPrint(ghMemDC, 0, 0, " a t l s");
	SetTextColor(ghMemDC, 0x4040C0), SetBkColor(ghMemDC, 0xC0C040);//red on cyan
//	SetTextColor(ghMemDC, 0xC0C0C0), SetBkColor(ghMemDC, 0x404040);//bright on dark
//	SetTextColor(ghMemDC, 0x404040), SetBkColor(ghMemDC, 0xC0C0C0);//dark on bright
	GUIPrint(ghMemDC, 2, 16, "atlas");//23x13
	GUIPrint(ghMemDC, w>>2, h>>1, "x");//
	GUIPrint(ghMemDC, w>>2, (h>>1)+16, "xxx");//
	int bkmode=SetBkMode(ghMemDC, TRANSPARENT);
	ctt_copy(0, 32, 23*4, 13*2, 32, 32);
//	ctt_copy(0, 32, 23*2, 13*2, 32, 32);
//	ctt_copy(0, 32, 23, 13, 32, 32);
	GUIPrint(ghMemDC, 2, 32, "atlas");
	SetTextColor(ghMemDC, txtcolor), SetBkColor(ghMemDC, bkcolor);
	SetBkMode(ghMemDC, bkmode);
	hFont=(HFONT)SelectObject(ghMemDC, hFont);
	int xpos=32+2, ypos=16, insidecolor=0xC04040, outsidecolor=0x40C0C0;
//	int ypos=16, insidecolor=0x404040, outsidecolor=0xC0C0C0;
	ctt_clear(xpos-1, 16, 23+1, 13, outsidecolor);
	ctt_print2(2, 0, 7, 13, xpos-1, ypos, insidecolor, outsidecolor);//a//opaque-only
	ctt_print2(10, 0, 5, 13, xpos+4, ypos, insidecolor, outsidecolor);//t
	ctt_print2(17, 0, 3, 13, xpos+8, ypos, insidecolor, outsidecolor);//l
	ctt_print2(2, 0, 7, 13, xpos+10, ypos, insidecolor, outsidecolor);//a
	ctt_print2(22, 0, 6, 13, xpos+16, ypos, insidecolor, outsidecolor);//s
	//ctt_print(2, 0, 7, 13, xpos-1, ypos, insidecolor);//a//1px padding
	//ctt_print(10, 0, 5, 13, xpos+4, ypos, insidecolor);//t
	//ctt_print(17, 0, 3, 13, xpos+8, ypos, insidecolor);//l
	//ctt_print(2, 0, 7, 13, xpos+10, ypos, insidecolor);//a
	//ctt_print(22, 0, 6, 13, xpos+16, ypos, insidecolor);//s
	ypos=32;
	ctt_print15(2, 0, 7, 13, xpos-1, ypos, insidecolor);//a//transparent
	ctt_print15(10, 0, 5, 13, xpos+4, ypos, insidecolor);//t
	ctt_print15(17, 0, 3, 13, xpos+8, ypos, insidecolor);//l
	ctt_print15(2, 0, 7, 13, xpos+10, ypos, insidecolor);//a
	ctt_print15(22, 0, 6, 13, xpos+16, ypos, insidecolor);//s

	ctt_absdiff(0, 16, 32, 16, 23+1, 13, xpos<<1, 16);
	ctt_absdiff(0, 32, 32, 32, 23+1, 13, xpos<<1, 32);

/*	//test 1
	hFont=(HFONT)SelectObject(ghMemDC, hFont);
	GUIPrint(ghMemDC, 0, 0, "0 / 0 <-atlas");//24x13	tiny atlas
	GUIPrint(ghMemDC, 0, 16, "0/0");//18x13		desired result
	int bkmode=SetBkMode(ghMemDC, TRANSPARENT);
	GUIPrint(ghMemDC, 0, 32, "0/0");//18x13		desired result
	SetBkMode(ghMemDC, bkmode);
	hFont=(HFONT)SelectObject(ghMemDC, hFont);

	ctt_clear(32, 16, 18, 13, 0xFFFFFF);
	ctt_copy_mul(0, 0, 6, 13, 32, 16);
	ctt_copy_mul(8, 0, 7, 13, 32+5, 16);
	ctt_copy_mul(17, 0, 6, 13, 32+11, 16);
	ctt_absdiff(0, 16, 32, 16, 18, 13, 64, 16);

	ctt_copy_mul(0, 0, 6, 13, 32, 32);
	ctt_copy_mul(8, 0, 7, 13, 32+5, 32);
	ctt_copy_mul(17, 0, 6, 13, 32+11, 32);
	ctt_absdiff(0, 32, 32, 32, 18, 13, 64, 32);

	//int px=32, py=0, dx=18, dy=13;
	//ctt_clear(32, 0, 18, 13, 0xFFFFFF);
	//ctt_copy_mul(0, 0, 6, 13, 32, 0);
	//ctt_copy_mul(8, 0, 7, 13, 32+5, 0);
	//ctt_copy_mul(17, 0, 6, 13, 32+11, 0);//*/
}
#endif

union			RedrawType
{
	int all;
	struct{char image, thumbbox, colorbar, toolbar;};
};
void			redrawbounds_dragbrush(Point const &p1, Point const &p2, int halfthickness, Rect &r)
{
	int t=shift(halfthickness, logzoom);
	if(p1.x<=p2.x)
		r.i.x=p1.x-t, r.f.x=p2.x+t;
	else
		r.i.x=p2.x-t, r.f.x=p1.x+t;
	if(p1.y<=p2.y)
		r.i.y=p1.y-t, r.f.y=p2.y+t;
	else
		r.i.y=p2.y-t, r.f.y=p1.y+t;
}
void			redrawbounds_dragbox(Point const &boxstart, Point const &boxend, Point const &delta, Rect &r)
{
	const int rmark_margin=11;
	if(delta.x>=0)
		r.i.x=boxstart.x-delta.x, r.f.x=boxend.x;
	else//delta.x<0
		r.i.x=boxstart.x, r.f.x=boxend.x-delta.x;

	if(delta.y>=0)
		r.i.y=boxstart.y-delta.y, r.f.y=boxend.y;
	else
		r.i.y=boxstart.y, r.f.y=boxend.y-delta.y;

	r.i.x-=rmark_margin, r.i.y-=rmark_margin;
	r.f.x+=rmark_margin, r.f.y+=rmark_margin;

	r.i.constraint_TL_zero();
	r.f.constraint_BR(Point(w, h));
}

Rect			imagewindow;//image window dimensions on screen, excluding scrollbars
Point			imageBRcorner,//position of bottom-right corner of visible part of image
				imagemarks[6];//{m1start, m1end, center-10, center+10, m2start, m2end}

void			render(int redrawtype, int rx1, int rx2, int ry1, int ry2)
{
	prof_add("Entry");
	RedrawType redraw;
	redraw.all=redrawtype;
	Rect redrawrect={Point(rx1, ry1), Point(rx2, ry2)};//args by value, copy value
	if(drag==D_ALPHA1||drag==D_ALPHA2)
		redraw.all=REDRAW_COLORBAR;

	const unsigned char background_grey=0xAB;
	if(redraw.all==REDRAW_ALL)
		memset(rgb, background_grey, rgbn<<2);
	prof_add("memset");
	
	//cycle_image(image, iw, ih, rand()%iw, rand()%ih);
	//prof_add("cycle");
	//draw_line_brush(image, iw, ih, BRUSH_LARGE_DISK, 10, 10, 10, 10, 0xFF000000);
	//draw_line_brush(image, iw, ih, BRUSH_LARGE_SQUARE, 10, 10, 10, 11, 0xFF000000);
	//draw_line_brush(image, iw, ih, BRUSH_LARGE_SQUARE, 10, 10, 20, 20, 0xFF000000);
	//draw_line_brush(image, iw, ih, BRUSH_LARGE_RIGHT45, 10, 10, 20, 20, 0xFF000000);
	//for(int k=0;k<10000;++k)
	//{
	//	draw_line_v2(rgb, w, h, rand()%w, rand()%h, rand()%w, rand()%h, 0x000000);
	//
	//	//MoveToEx(ghMemDC, rand()%w, rand()%h, 0);
	//	//LineTo(ghMemDC,rand()%w, rand()%h);
	//
	//	//draw_line(rgb, w, h, rand()%w, rand()%h, rand()%w, rand()%h, 0x000000, false);
	//}
	//for(int k=0;k<10000;++k)
	//{
	//	//draw_line_aa_v2(rgb, w, h, rand()%w, rand()%h, rand()%w, rand()%h, 1+rand()*0.0001220703125, rand()<<15|rand());
	//	//draw_line_brush(rgb, w, h, BRUSH_LARGE_RIGHT45, rand()%w, rand()%h, rand()%w, rand()%h, rand()<<15|rand());
	//	draw_line_brush(rgb, w, h, BRUSH_LARGE_DISK, rand()%w, rand()%h, rand()%w, rand()%h, rand()<<15|rand());
	//}
	//prof_add("LINES");
	
//	const int scrollbarwidth=17;//19
	int x1=0, x2=w, y1=0, y2=h;
	//draw UI
#if 1
	const int thbox_w0=3+10+90+10;//separator + max thumbnail width with padding
	thbox_vscroll.decide(thbox_h>y2-y1);
//	thbox_scrollbar=thbox_h>y2-y1;
	showthumbnailbox=animated&&w>thbox_w0+thbox_vscroll.dwidth&&y2-y1>34;
	if(showthumbnailbox)//draw thumbnail box
	{
		int thbox_w=thbox_w0+thbox_vscroll.dwidth;
		if(redraw.thumbbox)
		{
			if(!thbox_vscroll.dwidth||thbox_posy<0)
				thbox_posy=0;
			thbox_x1=w-thbox_w;
			rectangle(thbox_x1, w, y1, y2, 0xF0F0F0);
			v_line(thbox_x1+1, y1, y2, 0xFFFFFF);
			v_line(thbox_x1+2, y1, y2, 0xA0A0A0);
			int th_x1=thbox_x1+13, th_x2=w-(10+thbox_vscroll.dwidth);//thumbnail space
			if(thbox_vscroll.dwidth)
				draw_vscrollbar(x2-scrollbarwidth, scrollbarwidth, y1, y2, thbox_posy, thbox_h, thbox_vscroll.s0, thbox_vscroll.m_start, 1, thbox_vscroll.start, thbox_vscroll.size, D_THBOX_VSCROLL);
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
				GUIPrint(ghMemDC, th_x1, ypos, "%d", kth+1);
				if(kth==current_frame)
					SetBkMode(ghMemDC, bkmode), SetBkColor(ghMemDC, bkcolor), SetTextColor(ghMemDC, textcolor);
				ypos+=16;
				int *thumbnail=thumbnails[kth];
				if(thumbnail)
				{
					for(int ky=maximum(0, y1-ypos), kyEnd=minimum(y2-ypos, th_h);ky<kyEnd;++ky)
					//for(int ky=maximum(0, y1-ypos), kyEnd=th_h+minimum(y2-(ypos+th_h), 0);ky<kyEnd;++ky)
					{
						int *srow=thumbnail+th_w*ky, *dstart=rgb+w*(ypos+ky)+xpos;
#if 1
						int xround=th_w&~3, xrem=th_w&3;
						__m128i src, dst;
						for(int kx=0;kx<xround;kx+=4)
						{
							src=_mm_loadu_si128((__m128i*)(srow+kx));
							dst=_mm_loadu_si128((__m128i*)(dstart+kx));
							dst=blend_ssse3(src, dst);
							//__m128i mask=_mm_set1_epi32(0xFF0000FF);//
							//dst=_mm_and_si128(dst, mask);//
							_mm_storeu_si128((__m128i*)(dstart+kx), dst);//DIB section: 0x00RRGGBB
						}
						if(xrem)
						{
							for(int k=0;k<xrem;++k)
							{
								src.m128i_i32[k]=srow[xround+k];
								dst.m128i_i32[k]=dstart[xround+k];
							}
							dst=blend_ssse3(src, dst);
							for(int k=0;k<xrem;++k)
								dstart[xround+k]=dst.m128i_i32[k];
						}
#else
						memcpy(dstart, srow, th_w<<2);//
#endif
					}
				}
				else
					rectangle(xpos, xpos+th_w, maximum(ypos, y1), minimum(ypos+th_h, y2), 0xFFFFFF);
				//for(int ky=0, ky2=ypos+ky;ky<th_h&&ky2>=y1&&ky2<y2;++ky, ++ky2)
				//{
				//	for(int kx=0;kx<th_w;++kx)
				//	{
				//		if(thumbnail)
				//		{
				//			auto dst=(byte*)(rgb+w*ky2+xpos+kx), src=(byte*)(thumbnail+th_w*ky+kx);
				//			auto &alpha=src[3];
				//			dst[0]=dst[0]+((src[0]-dst[0])*alpha>>8);
				//			dst[1]=dst[1]+((src[1]-dst[1])*alpha>>8);
				//			dst[2]=dst[2]+((src[2]-dst[2])*alpha>>8);
				//		}
				//		else
				//			rgb[w*ky2+xpos+kx]=0xFFFFFF;
				//	}
				//}
			}
			SetBkMode(ghMemDC, bkmode0);
		}//end if redraw.thumbbox
		x2-=thbox_w;
	}//end if showthumbnailbox
	//else
	//	thbox_scrollbar=false;
	prof_add("thumbnail box");
	showcolorbar=h>74&&x2-x1>=256;
	if(showcolorbar)//draw color palette
	{
		if(redraw.colorbar)
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
				line45_backslash(x-n, y-n, n, color_aslider);
				line45_forwardslash(x, y, n, color_aslider);
				h_line(x-n, x+n+1, y-n, color_aslider);
				x=sstart+secondary_alpha, y=h-47;
				line45_forwardslash(x-n, y+n, n, color_aslider);
				line45_backslash(x, y, n, color_aslider);
				h_line(x-n, x+n+1, y+n, color_aslider);
			}
		//	if(currentmode==M_TEXT&&x2-x1>850)//draw bold, italic, underline buttons
			switch(colorbarcontents)
			{
			case CS_TEXTOPTIONS:
				if(x2-x1>850)
				{
					int x0=650, y0=h-48;
					//int bkmode=SetBkMode(ghMemDC, TRANSPARENT);
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
					GUIPrint(ghMemDC, x0+120+3, y0+3, "I");
					GUIPrint(ghMemDC, x0+170, y0+3, "U");
					//SetBkMode(ghMemDC, bkmode);
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
					GUIPrint(ghMemDC, x0, y0+8+3, "Stretch:");
					GUIPrint(ghMemDC, x0+128, y0+8-10+3, "%%");
					GUIPrint(ghMemDC, x0+128, y0+8-10+22+3, "%%");
					GUIPrint(ghMemDC, x0+190, y0+8+3, "Skew:");
					GUIPrint(ghMemDC, x0+190+120, y0+8-9+3, "degrees");
					GUIPrint(ghMemDC, x0+190+120, y0+8-9+22+3, "degrees");
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
		}//end if redraw.colorbar
		y2-=74;
	}//end color palette
	prof_add("color palette");
	showtoolbox=h-74*showcolorbar>276;
	if(showtoolbox)//draw tool box
	{
		if(redraw.toolbar)
		{
			h_line(0, 56, 0, 0xA0A0A0);
			h_line(0, 56, 1, 0xFFFFFF);
			rectangle(0, 56, 2, h-74, 0xF0F0F0);
			v_line(56, 0, h-73, 0xA0A0A0);
			h_line(0, 57, h-74, 0xA0A0A0);
			static bool icons_packed=true;
			if(icons_packed)//extract icons		TODO: move to main
			{
				icons_packed=false;
				unpack_icons(icons, nicons);
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
		}
		x1+=58;
	}//end toolbox
	prof_add("toolbox");
#endif
	//draw tool type selection box below the toolbox
#if 1
	const int color_selection=0x3399FF;
	if(redraw.toolbar&&showtoolbox)
	{
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
					"0.5x", "2x",
				//	"1x", "2x",
					"4x", "8x",
					"16x", "32x",
				};
				enum CharIdx
				{
					IDX_0, IDX_1, IDX_2, IDX_3, IDX_4, IDX_5, IDX_6,
					IDX_8,
					IDX_PERIOD, IDX_X,
				};
				static const char chars[]=//active high, LSB is on left: LSB->MSB (rows are reversed)
				{
					14, 17, 19, 21, 25, 17, 17, 14,//0<<4:	0	new
						4,  7,  4,  4,  4,  4,  4,  4,//1<<3:	1
					14, 17, 16,  8,  4,  2,  1, 31,//2<<3:	2
					14, 17, 16, 12, 16, 16, 17, 14,//3<<3:	3
						8, 12, 10,  9,  9, 31,  8,  8,//4<<3:	4
					31,  1,  1, 15, 16, 16, 17, 14,//5<<4:	5	new
					14, 17,  1, 15, 17, 17, 17, 14,//6<<3:	6
					14, 17, 17, 14, 17, 17, 17, 14,//7<<3:	8
						0,  0,  0,  0,  0,  0,  3,  3,//8<<3:	.	new
						0,  0,  9,  9,  6,  6,  9,  9,//9<<3:	x
				};
				static const char widths[]={5, 4, 5, 5, 5, 5, 5, 5, 2, 4};
				for(int k=0;k<6;++k)
				{
					int x0=7+21*(k&1)+1, y0=210+22*(k>>1)+1;
					const char *zoomlevel=zoomlevels[k];
					int size=0, length=0;
					for(int kc=0;zoomlevel[kc];++kc)//calculate text length
					{
						int idx=0;
						switch(zoomlevel[kc])
						{
						case '0':idx=IDX_0;break;
						case '1':idx=IDX_1;break;
						case '2':idx=IDX_2;break;
						case '3':idx=IDX_3;break;
						case '4':idx=IDX_4;break;
						case '5':idx=IDX_5;break;
						case '6':idx=IDX_6;break;
						case '8':idx=IDX_8;break;
						case '.':idx=IDX_PERIOD;break;
						case 'x':idx=IDX_X;break;
						}
						size+=widths[idx];
						++length;
					}
					size+=length-1;
					int color_in;
					if(magnification_levels[k]==magnifier_level)//select color & fill highlight
					{
						for(int ky=0;ky<20;++ky)
							memfill(rgb+w*(y0+ky)+x0, &color_selection, 19<<2, 1<<2);
							//for(int kx=0;kx<19;++kx)
							//	rgb[w*(y0+ky)+x0+kx]=color_selection;
						color_in=0xFFFFFF;
					}
					else
						color_in=0;
					x0+=10-(size>>1)-!k, y0+=11-4;
				//	int offset=11-(size>>1);
					for(int kc=0;zoomlevel[kc];++kc)
					{
						int idx=0;
						switch(zoomlevel[kc])
						{
						case '0':idx=IDX_0;break;
						case '1':idx=IDX_1;break;
						case '2':idx=IDX_2;break;
						case '3':idx=IDX_3;break;
						case '4':idx=IDX_4;break;
						case '5':idx=IDX_5;break;
						case '6':idx=IDX_6;break;
						case '8':idx=IDX_8;break;
						case '.':idx=IDX_PERIOD;break;
						case 'x':idx=IDX_X;break;
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
		}//end switch
	}//end if
#endif
	int use_temp_buffer0=0;
	if(!image)
	{
		if(redraw.all!=REDRAW_ALL&&(redraw.image&REDRAW_IMAGE_SCROLL)==REDRAW_IMAGE_SCROLL)//fill imagewindow background
			for(int ky=y1;ky<y2;++ky)
				memset(rgb+w*ky+x1, background_grey, (x2-x1)<<2);
	}
	if(image&&redraw.image)
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
		hscroll.decide(iw*zoom>x2-x1);
		vscroll.decide(ih*zoom>y2-y1);
		//hscrollbar=iw*zoom>x2-x1;
		//vscrollbar=ih*zoom>y2-y1;
		if(spx<0)
			spx=0;
		if(spy<0)
			spy=0;
		int xend=x1_2+shift(iw-spx, logzoom), yend=y1_2+shift(ih-spy, logzoom);
		hscroll.decide_orwith(!hscroll.dwidth&&vscroll.dwidth&&xend>=w-scrollbarwidth);
		vscroll.decide_orwith(!vscroll.dwidth&&vscroll.dwidth&&yend>=h-74-scrollbarwidth);
		//hscrollbar|=!hscrollbar&&vscrollbar&&xend>=w-scrollbarwidth;
		//vscrollbar|=!vscrollbar&&hscrollbar&&yend>=h-74-scrollbarwidth;
		int color_barbk=0xF0F0F0, color_slider=0xCDCDCD, color_button=0xE5E5E5;//0xDDDDDD
		if(hscroll.dwidth&&vscroll.dwidth&&redraw.image>>1)
			rectangle(x2-scrollbarwidth, x2, y2-scrollbarwidth, y2, color_barbk);//corner square
		if(hscroll.dwidth)
		{
			if(redraw.image>>1)
			{
				int vsbw=vscroll.dwidth;
				rectangle(x1+scrollbarwidth, x2-scrollbarwidth-vsbw, y2-scrollbarwidth, y2, color_barbk);//horizontal
				rectangle(x1, x1+scrollbarwidth, y2-scrollbarwidth, y2, color_button);//left
				rectangle(x2-scrollbarwidth-vsbw, x2-vsbw, y2-scrollbarwidth, y2, color_button);//right
				if(drag==D_HSCROLL)
					scrollbar_scroll(spx, iw, x2-x1-vsbw, x2-x1-(scrollbarwidth<<1)-vsbw, hscroll.s0, hscroll.m_start, mx, zoom, hscroll.start, hscroll.size);
				else
					scrollbar_slider(spx, iw, x2-x1-vsbw, x2-x1-(scrollbarwidth<<1)-vsbw, zoom, hscroll.start, hscroll.size);
				rectangle(x1+scrollbarwidth+hscroll.start, x1+scrollbarwidth+hscroll.start+hscroll.size, y2-scrollbarwidth, y2, color_slider);//horizontal slider
			}
			y2-=scrollbarwidth;
		}
		else
			spx=0;
		if(vscroll.dwidth&&h-74*showcolorbar>scrollbarwidth*3)
		{
			if(redraw.image>>1)
				draw_vscrollbar(x2-scrollbarwidth, scrollbarwidth, y1, y2, spy, ih, vscroll.s0, vscroll.m_start, zoom, vscroll.start, vscroll.size, D_VSCROLL);
			x2-=scrollbarwidth;
		}
		else
			spy=0;
#endif

		x1=x1_2, y1=y1_2;
		if(tw!=iw||th!=ih)
			temp_buffer=(int*)realloc(temp_buffer, image_size<<2), tw=iw, th=ih;
		if(drag&&!prev_drag)
		//if(!prev_drag)
			start_mx=prev_mx=mx, start_my=prev_my=my;

		//drag actions
#if 1
		if(drag==D_DRAW||drag==D_SELECTION)
		{
			int color, color2;
			if(kb[VK_LBUTTON])
				color=primarycolor, color2=secondarycolor;
			else
				color=secondarycolor, color2=primarycolor;
			switch(currentmode)
			{
			case M_FREE_SELECTION:
				if(drag==D_DRAW)//select with mouse
				{
					freesel_add_mouse(mx, my);
					memcpy(temp_buffer, image, image_size<<2), use_temp_buffer=true;

					int *imask=(int*)malloc(image_size<<2);
					memset(imask, 0, image_size<<2);
					int nv=freesel.size();
					for(int k=1;k<nv;++k)
					{
						auto &p1=freesel[k-1], &p2=freesel[k];
						draw_line_brush(temp_buffer, iw, ih, LINE3, p1.x, p1.y, p2.x, p2.y, 0, true, imask);
					}
					//std::swap(temp_buffer, imask);//
					free(imask);
					//if(redraw.all==REDRAW_IMAGE_PARTIAL&&nv>1)//redraw: drag brush t/2=2
					//	redrawbounds_dragbrush(freesel[nv-2], freesel[nv-1], 2, redrawrect);
				}
				else if(drag==D_SELECTION)//drag selection
				{
					selection_move_mouse(prev_mx, prev_my, mx, my);
					if(redraw.all==REDRAW_IMAGE_PARTIAL)//redraw: drag-box
					{
						Rect sr=selection, delta={Point(prev_mx, prev_my), Point(mx, my)};
						sr.sort_coords();
						sr.image2screen();
						delta.screen2image();
						delta.image2screen();
						redrawbounds_dragbox(sr.i, sr.f, delta.f-delta.i, redrawrect);
					}
					if(kb[VK_SHIFT])
						selection_stamp(false);
				}
				break;
			case M_RECT_SELECTION:
				if(drag==D_DRAW)//select with mouse
				{
					//int half=shift(1, logzoom-1);
					//screen2image(mx+half, my+half, sel_end.x, sel_end.y);
					screen2image_rounded(mx, my, selection.f.x, selection.f.y);
					//screen2image_rounded(mx, my, sel_end.x, sel_end.y);
					selection.f.clamp(0, iw, 0, ih);
					//Point start(0, 0), end(iw, ih);
					//sel_end.x=clamp(start.x, sel_end.x, end.x);
					//sel_end.y=clamp(start.y, sel_end.y, end.y);
				}
				else if(drag==D_SELECTION)//drag selection
				{
					selection_move_mouse(prev_mx, prev_my, mx, my);
					if(redraw.all==REDRAW_IMAGE_PARTIAL)//redraw: drag-box
					{
						Rect sr=selection, delta={Point(prev_mx, prev_my), Point(mx, my)};
						sr.sort_coords();
						sr.image2screen();
						delta.screen2image();
						delta.image2screen();
						if(delta.f.x!=delta.i.x)
							int LOL_1=0;
						redrawbounds_dragbox(sr.i, sr.f, delta.f-delta.i, redrawrect);
					}
					if(kb[VK_SHIFT])
						selection_stamp(false);
				}
				break;
			case M_ERASER:
				if(!prev_drag)
					hist_premodify(image, iw, ih);
				draw_line_brush_mouse(image, erasertype, prev_mx, prev_my, mx, my, color2);
				if(redraw.all==REDRAW_IMAGE_PARTIAL)
					redrawbounds_dragbrush(Point(prev_mx, prev_my), Point(mx, my), 5, redrawrect);
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
				draw_line_mouse(image, prev_mx, prev_my, mx, my, color, color2, false);
				if(redraw.all==REDRAW_IMAGE_PARTIAL)
					redrawbounds_dragbrush(Point(prev_mx, prev_my), Point(mx, my), 2, redrawrect);
				break;
			case M_BRUSH:
				if(!prev_drag)
					hist_premodify(image, iw, ih);
				draw_line_brush_mouse(image, brushtype, prev_mx, prev_my, mx, my, color);
				if(redraw.all==REDRAW_IMAGE_PARTIAL)
					redrawbounds_dragbrush(Point(prev_mx, prev_my), Point(mx, my), 4, redrawrect);
				break;
			case M_AIRBRUSH:
				if(!prev_drag)
					hist_premodify(image, iw, ih);
				airbrush_mouse(image, mx, my, color);
				if(redraw.all==REDRAW_IMAGE_PARTIAL)
					redrawbounds_dragbrush(Point(mx, my), Point(mx, my), 13, redrawrect);
				break;
			case M_TEXT:
				if(drag==D_DRAW)//define textbox with mouse
				{
					textrect.f.x+=mx-prev_mx;//works because zoom=1 in text mode
					textrect.f.y+=my-prev_my;
					//screen2image(mx, my, text_end.x, text_end.y);
					if(redraw.all==REDRAW_IMAGE_PARTIAL)
						redrawrect.set(0, 0, w, h);//TODO: correct image coordinates
				}
				else if(drag==D_SELECTION)//drag textbox
				{
					Point m, prev_m;
					screen2image(mx, my, m.x, m.y);
					screen2image(prev_mx, prev_my, prev_m.x, prev_m.y);
					Point delta=m-prev_m;
					textrect.i+=delta;
					textrect.f+=delta;
					//textpos+=delta;
					//text_start+=delta, text_end+=delta, text_s1+=delta, text_s2+=delta;
					move_textbox();
					if(redraw.all==REDRAW_IMAGE_PARTIAL)
					{
						Rect sr=textrect, delta={Point(prev_mx, prev_my), Point(mx, my)};
						sr.sort_coords();
						sr.image2screen();
						delta.screen2image();
						delta.image2screen();
						redrawbounds_dragbox(sr.i, sr.f, delta.f-delta.i, redrawrect);

						//Rect tr2=textrect;
						//tr2.image2screen();
						//redrawbounds_dragbox(tr2.i, tr2.f, delta, redrawrect);
					}
				}
				break;
			case M_LINE:
				memcpy(temp_buffer, image, image_size<<2);
				use_temp_buffer=true;

				//{//
				//	int *imask=(int*)malloc(image_size<<2);//
				//	memset(imask, 0, image_size<<2);
				//	Point p1(start_mx, start_my), p2(mx, my);
				//	p1.screen2image();
				//	p2.screen2image();
				//	draw_line_brush(temp_buffer, iw, ih, LINE5, p1.x, p1.y, p2.x, p2.y, 0, true, imask);
				//	free(imask);
				//}
				draw_line_mouse(temp_buffer, start_mx, start_my, mx, my, (0xFF000000&~-kb['D'])|color, color2, true);
				if(redraw.all==REDRAW_IMAGE_PARTIAL)
					redrawrect.set(x1, y1, x2, y2);//whole image
				break;
			case M_CURVE:
				curve_update_mouse(mx, my);
				memcpy(temp_buffer, image, image_size<<2);
			//	memset(temp_buffer, 0, image_size<<2);
				use_temp_buffer=true;
				curve_draw(temp_buffer, color);
				if(redraw.all==REDRAW_IMAGE_PARTIAL)
					redrawrect.set(0, 0, w, h);//TODO: set redrawrect
				break;
			case M_RECTANGLE:
				memcpy(temp_buffer, image, image_size<<2);
			//	memset(temp_buffer, 0, image_size<<2);
				use_temp_buffer=true;
				draw_rectangle_mouse(temp_buffer, start_mx, mx, start_my, my, 0xFF000000|color, 0xFF000000|color2);
				if(redraw.all==REDRAW_IMAGE_PARTIAL)
					redrawrect.set(0, 0, w, h);//TODO: set redrawrect
				break;
			case M_POLYGON://draw temp outline
				{
					int c3, c4;
					if(polygon_leftbutton)
						c3=(0xFF000000&~-kb['D'])|primarycolor, c4=secondarycolor;
					else
						c3=(0xFF000000&~-kb['D'])|secondarycolor, c4=primarycolor;
					memcpy(temp_buffer, image, image_size<<2);
				//	memset(temp_buffer, 0, image_size<<2);
					use_temp_buffer=true;
					for(int k=0, npoints=polygon.size();k<npoints-1;++k)
					{
						auto &p1=polygon[k], &p2=polygon[(k+1)%npoints];
						draw_line_brush(temp_buffer, iw, ih, linewidth, p1.x, p1.y, p2.x, p2.y, c3);
					}
					if(polygon.size())
					{
						auto &p=*polygon.rbegin();
						Point i;
						screen2image(mx, my, i.x, i.y);
						draw_line_brush(temp_buffer, iw, ih, linewidth, p.x, p.y, i.x, i.y, c3);
						if(redraw.all==REDRAW_IMAGE_PARTIAL)
						{
							Point anchor;
							image2screen(p.x, p.y, anchor.x, anchor.y);
							redrawbounds_dragbrush(anchor, Point(mx, my), 2, redrawrect);
						}
					}
					else
					{
						draw_line_mouse(temp_buffer, start_mx, start_my, mx, my, c3, c4, false);
						if(redraw.all==REDRAW_IMAGE_PARTIAL)
							redrawbounds_dragbrush(Point(start_mx, start_my), Point(mx, my), 2, redrawrect);
					}
				}
				break;
			case M_ELLIPSE:
				memcpy(temp_buffer, image, image_size<<2);
			//	memset(temp_buffer, 0, image_size<<2);
				use_temp_buffer=true;
				draw_ellipse_mouse(temp_buffer, start_mx, mx, start_my, my, 0xFF000000|color, 0xFF000000|color2);
				if(redraw.all==REDRAW_IMAGE_PARTIAL)
					redrawbounds_dragbrush(Point(start_mx, start_my), Point(mx, my), 2, redrawrect);
				break;
			case M_ROUNDED_RECT:
				memcpy(temp_buffer, image, image_size<<2);
			//	memset(temp_buffer, 0, image_size<<2);
				use_temp_buffer=true;
				draw_roundrect_mouse(temp_buffer, start_mx, mx, start_my, my, 0xFF000000|color, 0xFF000000|color2);
				if(redraw.all==REDRAW_IMAGE_PARTIAL)
					redrawbounds_dragbrush(Point(start_mx, start_my), Point(mx, my), 2, redrawrect);
				break;
			}
		}
		else if(drag>D_RESIZE_SEL_START&&drag<D_RESIZE_END)
		{
			int xflip, yflip;
			selection=sel0;
			selection_resize_mouse(selection, mx, my, start_mx, start_my, &xflip, &yflip);
			//xanchor=anchors[(drag-D_SEL_RESIZE_TL)<<1], yanchor=anchors[(drag-D_SEL_RESIZE_TL)<<1|1];
			//Point mouse(mx, my), prevmouse(prev_mx, prev_my);
			//mouse.screen2image_rounded();
			//prevmouse.screen2image_rounded();
			//int xflip=assign_using_anchor(xanchor, sr.i.x, sr.f.x, mouse.x, prevmouse.x);
			//int yflip=assign_using_anchor(yanchor, sr.i.y, sr.f.y, mouse.y, prevmouse.y);
			char drag2=drag;
			if(xflip)
			{
				switch(drag2)
				{
				case D_SEL_RESIZE_TL:	drag2=D_SEL_RESIZE_TR;		break;
				case D_SEL_RESIZE_TOP:								break;
				case D_SEL_RESIZE_TR:	drag2=D_SEL_RESIZE_TL;		break;
				case D_SEL_RESIZE_LEFT:	drag2=D_SEL_RESIZE_RIGHT;	break;
				case D_SEL_RESIZE_RIGHT:drag2=D_SEL_RESIZE_LEFT;	break;
				case D_SEL_RESIZE_BL:	drag2=D_SEL_RESIZE_BR;		break;
				case D_SEL_RESIZE_BOTTOM:							break;
				case D_SEL_RESIZE_BR:	drag2=D_SEL_RESIZE_BL;		break;
				}
			}
			if(yflip)
			{
				switch(drag2)
				{
				case D_SEL_RESIZE_TL:		drag2=D_SEL_RESIZE_BL;		break;
				case D_SEL_RESIZE_TOP:		drag2=D_SEL_RESIZE_BOTTOM;	break;
				case D_SEL_RESIZE_TR:		drag2=D_SEL_RESIZE_BR;		break;
				case D_SEL_RESIZE_LEFT:									break;
				case D_SEL_RESIZE_RIGHT:								break;
				case D_SEL_RESIZE_BL:		drag2=D_SEL_RESIZE_TL;		break;
				case D_SEL_RESIZE_BOTTOM:	drag2=D_SEL_RESIZE_TOP;		break;
				case D_SEL_RESIZE_BR:		drag2=D_SEL_RESIZE_TR;		break;
				}
			}
			if(xflip||yflip)
			{
				switch(drag2)
				{
				case D_SEL_RESIZE_TL:		hcursor=hcursor_sizenwse;	break;
				case D_SEL_RESIZE_TOP:		hcursor=hcursor_sizens;		break;
				case D_SEL_RESIZE_TR:		hcursor=hcursor_sizenesw;	break;
				case D_SEL_RESIZE_LEFT:		hcursor=hcursor_sizewe;		break;
				case D_SEL_RESIZE_RIGHT:	hcursor=hcursor_sizewe;		break;
				case D_SEL_RESIZE_BL:		hcursor=hcursor_sizenesw;	break;
				case D_SEL_RESIZE_BOTTOM:	hcursor=hcursor_sizens;		break;
				case D_SEL_RESIZE_BR:		hcursor=hcursor_sizenwse;	break;
				}
				SetCursor(hcursor);
			}
			//Point delta=mouse-prevmouse;
			//if(xanchor==-1)
			//{
			//	if(sr.i.x<sr.f.x)
			//		sr.i.x+=delta.x;
			//	else
			//		sr.f.x+=delta.x;
			//}
			//else if(xanchor==1)
			//{
			//	if(sr.i.x<sr.f.x)
			//		sr.f.x+=delta.x;
			//	else
			//		sr.i.x+=delta.x;
			//}
		}
	/*	else if(drag>D_RESIZE_SEL_START&&drag<D_RESIZE_END)
		{
			xanchor=anchors[(drag-D_SEL_RESIZE_TL)<<1], yanchor=anchors[(drag-D_SEL_RESIZE_TL)<<1|1];
			Point mouse(mx, my), prevmouse(prev_mx, prev_my);
			mouse.screen2image_rounded();
			prevmouse.screen2image_rounded();
			
			//int half=shift(1, logzoom-1);
			//Point mouse(mx+half, my+half);
			//mouse.screen2image();

			//sel_end.set(mx, my);
			//sel_end.screen2image();
			//selection_assign();
			int xflip=assign_using_anchor(xanchor, selection.i.x, selection.f.x, mouse.x, prevmouse.x);
			int yflip=assign_using_anchor(yanchor, selection.i.y, selection.f.y, mouse.y, prevmouse.y);
			if(xflip)
			{
				switch(drag)
				{
				case D_SEL_RESIZE_TL:	drag=D_SEL_RESIZE_TR;	break;
				case D_SEL_RESIZE_TOP:							break;
				case D_SEL_RESIZE_TR:	drag=D_SEL_RESIZE_TL;	break;
				case D_SEL_RESIZE_LEFT:	drag=D_SEL_RESIZE_RIGHT;break;
				case D_SEL_RESIZE_RIGHT:drag=D_SEL_RESIZE_LEFT;	break;
				case D_SEL_RESIZE_BL:	drag=D_SEL_RESIZE_BR;	break;
				case D_SEL_RESIZE_BOTTOM:						break;
				case D_SEL_RESIZE_BR:	drag=D_SEL_RESIZE_BL;	break;
				}
			}
			if(yflip)
			{
				switch(drag)
				{
				case D_SEL_RESIZE_TL:		drag=D_SEL_RESIZE_BL;		break;
				case D_SEL_RESIZE_TOP:		drag=D_SEL_RESIZE_BOTTOM;	break;
				case D_SEL_RESIZE_TR:		drag=D_SEL_RESIZE_BR;		break;
				case D_SEL_RESIZE_LEFT:									break;
				case D_SEL_RESIZE_RIGHT:								break;
				case D_SEL_RESIZE_BL:		drag=D_SEL_RESIZE_TL;		break;
				case D_SEL_RESIZE_BOTTOM:	drag=D_SEL_RESIZE_TOP;		break;
				case D_SEL_RESIZE_BR:		drag=D_SEL_RESIZE_TR;		break;
				}
			}
			if(xflip||yflip)
			{
				switch(drag)
				{
				case D_SEL_RESIZE_TL:		hcursor=hcursor_sizenwse;	break;
				case D_SEL_RESIZE_TOP:		hcursor=hcursor_sizens;		break;
				case D_SEL_RESIZE_TR:		hcursor=hcursor_sizenesw;	break;
				case D_SEL_RESIZE_LEFT:		hcursor=hcursor_sizewe;		break;
				case D_SEL_RESIZE_RIGHT:	hcursor=hcursor_sizewe;		break;
				case D_SEL_RESIZE_BL:		hcursor=hcursor_sizenesw;	break;
				case D_SEL_RESIZE_BOTTOM:	hcursor=hcursor_sizens;		break;
				case D_SEL_RESIZE_BR:		hcursor=hcursor_sizenwse;	break;
				}
				SetCursor(hcursor);
			}
			//if(xanchor==-1)//resize from start of view
			//{
			//	if(selection.i.x<selection.f.x)
			//		selection.i.x=mouse.x;
			//	else
			//		selection.f.x=mouse.x;
			//}
			//else if(xanchor==1)//resize from end of view
			//{
			//	if(selection.i.x<selection.f.x)
			//		selection.f.x=mouse.x;
			//	else
			//		selection.i.x=mouse.x;
			//}
			//if(xanchor)
			//	sel_end.x=mouse.x;
			//if(yanchor)
			//	sel_end.y=mouse.y;
			//assign_using_anchor(xanchor, selpos.x, selid.x, sel_start.x, sel_end.x, prevmouse.x);
			//assign_using_anchor(yanchor, selpos.y, selid.y, sel_start.y, sel_end.y, prevmouse.y);

			//Point p1, p2;
			//selection_sortNbound(sel_start, sel_end, p1, p2);
			//image2screen(p1.x, p1.y, sel_s1.x, sel_s1.y);
			//image2screen(p2.x, p2.y, sel_s2.x, sel_s2.y);
			//selection_assign();

			//Point mouse(mx, my);
			//mouse.screen2image();
			//switch(drag)
			//{
			//case D_SEL_RESIZE_TL:
			//	break;
			//case D_SEL_RESIZE_TOP:
			//	break;
			//case D_SEL_RESIZE_TR:
			//	break;
			//case D_SEL_RESIZE_LEFT:
			//	break;
			//case D_SEL_RESIZE_RIGHT:
			//	break;
			//case D_SEL_RESIZE_BL:
			//	break;
			//case D_SEL_RESIZE_BOTTOM:
			//	break;
			//case D_SEL_RESIZE_BR:
			//	break;
			//}
		}//*/
#endif
		prof_add("action");

		int x2s=hscroll.dwidth?w-vscroll.dwidth:x1+shift(iw-spx, logzoom),
			y2s=vscroll.dwidth?h-74-hscroll.dwidth:y1+shift(ih-spy, logzoom);
		//int x2s=hscrollbar?w-scrollbarwidth*vscrollbar:x1+shift(iw-spx, logzoom),
		//	y2s=vscrollbar?h-74-scrollbarwidth*hscrollbar:y1+shift(ih-spy, logzoom);
		if(vscroll.dwidth&&x2s>w-scrollbarwidth)
			x2s=w-vscroll.dwidth;
		if(spx+(x2s-x1)/zoom>iw)
			x2s=x1+shift(iw-spx, logzoom);
		if(hscroll.dwidth&&y2s>h-74-scrollbarwidth)
			y2s=h-74-scrollbarwidth;
		if(spy+(y2s-y1)/zoom>ih)
			y2s=y1+shift(ih-spy, logzoom);

		if(redraw.all!=REDRAW_ALL&&(redraw.image&REDRAW_IMAGE_SCROLL)==REDRAW_IMAGE_SCROLL)//fill imagewindow background
		{
			for(int ky=0;ky<y2;++ky)//note: ky starts from 0, kx starts from x1-3
				memset(rgb+w*ky+x1-3, background_grey, (x2-(x1-3))<<2);
			//if(x2s<x2)
			//	for(int ky=0;ky<y2s;++ky)//note: ky starts from 0 not y1
			//		memset(rgb+w*ky+x2s, background_grey, (x2-x2s)<<2);
			//if(y2s<y2)
			//	for(int ky=y2s;ky<y2;++ky)
			//		memset(rgb+w*ky+x1, background_grey, (x2-x1)<<2);
			prof_add("bk");
		}
		if((currentmode==M_ERASER||currentmode==M_BRUSH)&&!use_temp_buffer)//draw brush preview at cursor
		{
			memcpy(temp_buffer, image, image_size<<2);
		//	memset(temp_buffer, 0, image_size<<2);
			use_temp_buffer=true;
			auto b=resources::brushes+(currentmode==M_ERASER?erasertype:brushtype);
			Point i;
			screen2image(mx, my, i.x, i.y);
			int c=currentmode==M_BRUSH!=kb[VK_RBUTTON]?primarycolor:secondarycolor;
			for(int k=0, ky=i.y+b->yoffset;k<b->ysize;++k, ++ky)
				for(int kx=b->bounds[k<<1], kxEnd=b->bounds[k<<1|1];kx<=kxEnd;++kx)
					if(!icheck(i.x+kx, ky))
						temp_buffer[iw*ky+i.x+kx]=c;
			if(redraw.all==REDRAW_IMAGE_PARTIAL&&drag==D_NONE)
			{
				int maxb=5;
				int px1=x1+maxb, px2=x2s-maxb, py1=y1+maxb, py2=y2s-maxb;
				if(mx>=px1&&mx<px2&&my>=py1&&my<py2 && prev_mx>=px1&&prev_mx<px2&&prev_my>=py1&&prev_my<py2)
					redrawbounds_dragbrush(Point(prev_mx, prev_my), Point(mx, my), 5, redrawrect);
			}
		}
		int bx1, bx2, by1, by2;//blend rect
		if(redraw.all==REDRAW_IMAGE_PARTIAL)
		{
			bx1=redrawrect.i.x=maximum(x1, redrawrect.i.x);
			bx2=redrawrect.f.x=minimum(x2s, redrawrect.f.x);
			by1=redrawrect.i.y=maximum(y1, redrawrect.i.y);
			by2=redrawrect.f.y=minimum(y2s, redrawrect.f.y);
		}
		else//redraw rect can be bigger than blend rect
			bx1=x1, bx2=x2s, by1=y1, by2=y2s;
		use_temp_buffer0=use_temp_buffer;
		//if(*image==0xFF000000)
		//	int LOL_1=0;
		if(bx1<bx2&&by1<by2)
			blend_with_checkboard_zoomed(use_temp_buffer?temp_buffer:image, x1, y1, bx1, bx2, by1, by2, 0, 0);
		//{
		//	for(int ky=0, yend=minimum(y2s-y1, ih);ky<yend;++ky)
		//		for(int kx=0, xend=minimum(x2s-x1, iw);kx<xend;++kx)
		//			rgb[w*(y1+ky)+x1+kx]=image[iw*ky+kx];
		//}
			//blend_with_checkboard_zoomed(use_temp_buffer?temp_buffer:image, x1, y1, bx1, bx2, by1, by2, mx-x1, my-y1);//eye strain
		prof_add("blend");
		if(currentmode!=M_CURVE&&currentmode!=M_POLYGON)
			use_temp_buffer=false;

		if(showgrid&&logzoom>=2)//draw grid, at pixel size 4+
		{
			int gridcolors[]={0x808080, 0xC0C0C0}, pxsize=1<<logzoom;
			for(int ky=y1;ky<y2s;ky+=pxsize)//draw horizontal grid lines
				memfill(rgb+w*ky+x1, gridcolors, (x2s-x1)<<2, sizeof(gridcolors));
			for(int ky=y1;ky<y2s;++ky)//draw vertical grid lines
			{
				int color=gridcolors[(ky-y1)&1];
				for(int kx=x1;kx<x2s;kx+=pxsize)
					rgb[w*ky+kx]=color;
			}
			prof_add("pixel grid");
		}

		if(selection.nonzero()&&drag!=D_DRAW)//draw selection buffer, on top of grid
		{
			stretch_blit(sel_buffer, sw, sh, selection, x1, y1, bx1, bx2, by1, by2, selection_free?sel_mask:nullptr, selection_transparency==TRANSPARENT, secondarycolor);
			prof_add("selection buffer");
		}

		if(currentmode==M_TEXT)
			draw_selection_rectangle(textrect, x1, x2, y1, y2, 2);//X can't see selection edges
		draw_selection_rectangle(selection, x1, x2, y1, y2, 0);
		prof_add("selection box");

		//resize UI
#if 1
		if(redraw.image)//blue resize marks
		{
			int markcolor;
			Point marksize(10, 10),
				marksize_TL(4, 4);
			//	marksize_TL(3, 3);

			Point ip1, ip2;//start & end points in image coordinates
			int isfull=-(selection.iszero()|(drag==D_DRAW));//resize marks are on the full image rather than on selection
			if(isfull)//resize marks on full image
				ip1.setzero(), ip2.set(iw, ih), markcolor=0x0000FF;
			else//resize marks on selection
			{
				ip1=selection.i, ip2=selection.f;
				if(ip1.x>ip2.x)
					std::swap(ip1.x, ip2.x);
				if(ip1.y>ip2.y)
					std::swap(ip1.y, ip2.y);
				markcolor=0xFF8000;
			}

			Point delta=ip2-ip1, oddity(delta.x&1, delta.y&1);
			Point imstart=ip1;
			imstart.image2screen();
			if(isfull)
				imagemarks[0]=imstart-marksize_TL;
			else
				imagemarks[0]=imstart-marksize;
			imagemarks[1]=imstart;
			imagemarks[0].constraint_TL(Point(x1-marksize_TL.x, y1-marksize_TL.y)), imagemarks[0].constraint_BR(Point(x2, y2));
			imagemarks[1].constraint_TL(Point(x1, y1)), imagemarks[1].constraint_BR(Point(x2, y2));
			//imagemarks[0].set(x1-(marksize_TL.x&-!spx), y1-(marksize_TL.y&-!spy));
			//imagemarks[1].set(x1, y1);
			Point imcenter;
			image2screen(ip1.x+(delta.x>>1), ip1.y+(delta.y>>1), imcenter.x, imcenter.y);//image center
			if(logzoom>0)
			{
				int halfpx=1<<(logzoom-1);
				imcenter.x+=halfpx&-oddity.x, imcenter.y+=halfpx&-oddity.y;
			}
			imagemarks[2]=imcenter-marksize;
			imagemarks[3]=imcenter+marksize+oddity;
			imagemarks[2].constraint_TL(Point(x1, y1)), imagemarks[2].constraint_BR(Point(x2, y2));
			imagemarks[3].constraint_TL(Point(x1, y1)), imagemarks[3].constraint_BR(Point(x2, y2));
			Point spoint;
			image2screen(ip2.x, ip2.y, spoint.x, spoint.y);//bottom right corner
			imagemarks[4]=spoint;
			imagemarks[5]=spoint+marksize;
			imagemarks[4].constraint_TL(Point(x1, y1)), imagemarks[4].constraint_BR(Point(x2, y2));
			imagemarks[5].constraint_TL(Point(x1, y1)), imagemarks[5].constraint_BR(Point(x2, y2));

			//draw the marks
			const int *xmask, *ymask;
			char xmw, ymw, mh=resources::resizemark_h;
			if(oddity.x)
				xmask=resources::resizemark_odd, xmw=resources::resizemark_odd_w;
			else
				xmask=resources::resizemark_even, xmw=resources::resizemark_even_w;
			if(oddity.y)
				ymask=resources::resizemark_odd, ymw=resources::resizemark_odd_w;
			else
				ymask=resources::resizemark_even, ymw=resources::resizemark_even_w;

			//top left
			stamp_mask(nullptr, marksize.x, marksize.y, 0, 0, imagemarks[0].x, imagemarks[1].x, imagemarks[0].y, imagemarks[1].y, markcolor);

			//top
			stamp_mask(xmask, xmw, mh, 1, 0, imagemarks[2].x, imagemarks[3].x, imagemarks[0].y, imagemarks[1].y, markcolor);

			//top right
			stamp_mask(nullptr, marksize.x, marksize.y, 0, 0, imagemarks[4].x, imagemarks[5].x, imagemarks[0].y, imagemarks[1].y, markcolor);

			//left
			stamp_mask(ymask, ymw, mh, 1, 1, imagemarks[0].x, imagemarks[1].x, imagemarks[2].y, imagemarks[3].y, markcolor);
			
			//right
			stamp_mask(ymask, ymw, mh, 0, 1, imagemarks[4].x, imagemarks[5].x, imagemarks[2].y, imagemarks[3].y, markcolor);
			
			//bottom left
			stamp_mask(nullptr, marksize.x, marksize.y, 0, 0, imagemarks[0].x, imagemarks[1].x, imagemarks[4].y, imagemarks[5].y, markcolor);
			
			//bottom
			stamp_mask(xmask, xmw, mh, 0, 0, imagemarks[2].x, imagemarks[3].x, imagemarks[4].y, imagemarks[5].y, markcolor);
			
			//bottom right
			stamp_mask(nullptr, marksize.x, marksize.y, 0, 0, imagemarks[4].x, imagemarks[5].x, imagemarks[4].y, imagemarks[5].y, markcolor);
			prof_add("res. marks");
		}
		if(drag>D_RESIZE_START&&drag<D_RESIZE_SEL_START)//draw resize lines (only for image resize)
		//if(drag>D_RESIZE_START&&drag<D_RESIZE_END)
		{
			Point cTL(x1, y1), cBR(x2, y2);
			Point mouse(mx, my), prevmouse(start_mx, start_my), delta, point,
				sp1, sp2;
			if(logzoom>0)//snap to grid
			{
				//int halfnum=1<<(logzoom-1);//round instead of truncation
				//mouse.x+=halfnum, mouse.y+=halfnum;
				mouse.screen2image_rounded();//accounts for toolbox width & gui offsets
				mouse.image2screen();
				prevmouse.screen2image_rounded();
				prevmouse.image2screen();
			}
			delta=mouse-prevmouse;
			if(drag<D_RESIZE_SEL_START)
				sp1.set(x1, y1), sp2.set(x2s, y2s);
			else
			{
				Rect r=selection;
				r.sort_coords();
				sp1=r.i, sp2=r.f;
			}
			switch(drag)
			{
			case D_RESIZE_TL:
			case D_SEL_RESIZE_TL:
				point.set(0, 0);
				point.image2screen();
				point+=delta;
				v_line_comp_dots(point.x, point.y, sp2.y, cTL, cBR);
				h_line_comp_dots(point.x, sp2.x, point.y, cTL, cBR);
				break;
			case D_RESIZE_TOP:
			case D_SEL_RESIZE_TOP:
				point.set(0, 0);
				point.image2screen();
				point+=delta;
				h_line_comp_dots(sp1.x, sp2.x, point.y, cTL, cBR);
				break;
			case D_RESIZE_TR:
			case D_SEL_RESIZE_TR:
				point.set(iw, 0);
				point.image2screen();
				point+=delta;
				v_line_comp_dots(point.x, point.y, sp2.y, cTL, cBR);
				h_line_comp_dots(sp1.x, point.x, point.y, cTL, cBR);
				break;
			case D_RESIZE_LEFT:
			case D_SEL_RESIZE_LEFT:
				point.set(0, 0);
				point.image2screen();
				point+=delta;
				v_line_comp_dots(point.x, sp1.y, sp2.y, cTL, cBR);
				break;
			case D_RESIZE_RIGHT:
			case D_SEL_RESIZE_RIGHT:
				point.set(iw, 0);
				point.image2screen();
				point+=delta;
				v_line_comp_dots(point.x, sp1.y, sp2.y, cTL, cBR);
				break;
			case D_RESIZE_BL:
			case D_SEL_RESIZE_BL:
				point.set(0, ih);
				point.image2screen();
				point+=delta;
				v_line_comp_dots(point.x, sp1.y, point.y, cTL, cBR);
				h_line_comp_dots(point.x, sp2.x, point.y, cTL, cBR);
				break;
			case D_RESIZE_BOTTOM:
			case D_SEL_RESIZE_BOTTOM:
				point.set(0, ih);
				point.image2screen();
				point+=delta;
				h_line_comp_dots(sp1.x, sp2.x, point.y, cTL, cBR);
				break;
			case D_RESIZE_BR:
			case D_SEL_RESIZE_BR:
				point.set(iw, ih);
				point.image2screen();
				point+=delta;
				v_line_comp_dots(point.x, sp1.y, point.y, cTL, cBR);
				h_line_comp_dots(sp1.x, point.x, point.y, cTL, cBR);
				break;
			}
			prof_add("res. lines");
		}
#endif

		if(currentmode==M_MAGNIFIER)//draw magnifier highlight
		{
			if(mousepos==MP_IMAGE)
			{
				int wndx=x2-x1, wndy=y2-y1;
				//int wx2=x2-vscroll.dwidth, wndx=wx2-x1,
				//	wy2=y2-hscroll.dwidth, wndy=wy2-y1;
				int mag_action=magnifier_level&-(logzoom!=magnifier_level);//if mlevel==zoom: set zoom, otherwise reset zoom 1:1
				int rx=shift(wndx, logzoom-(mag_action+1)),
					ry=shift(wndy, logzoom-(mag_action+1));
				int roundmask=-1<<(logzoom&-(logzoom>0));
				//int roundmask=~(shift(1, logzoom)-1);
				//Point mouse(x1+((mx-x1)&roundmask), y1+((my-y1)&roundmask));
				//Point mouse(mx, my);
				//mouse.screen2image();//takes into account GUI window offsets
				//mouse.image2screen();
				int x0=x1+((mx-rx-x1)&roundmask)+rx,
					y0=y1+((my-ry-y1)&roundmask)+ry;
				x0=clamp(x1+rx, x0, x2-rx);
				y0=clamp(y1+ry, y0, y2-ry);
				int hx1=clamp(x1, x0-rx, x2-1),
					hx2=clamp(x1, x0+rx, x2-1),
					hy1=clamp(y1, y0-ry, y2-1),
					hy2=clamp(y1, y0+ry, y2-1);
				h_line_add(hx1, hx2+1, hy1, 128);
				h_line_add(hx1, hx2+1, hy2, 128);
				v_line_add(hx1, hy1+1, hy2, 128);
				v_line_add(hx2, hy1+1, hy2, 128);
			}
#if 0
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
#endif
			prof_add("magnification box");
		}
		imageBRcorner.set(x2s, y2s);
	}//end if image
	imagewindow.set(x1, y1, x2, y2);
#ifndef RELEASE
	if(debugmode)
	{
		HFONT hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
		hFont=(HFONT)SelectObject(ghMemDC, hFont);
		int fontH=13;
		int size=0;
		for(int k=0, nv=history.size();k<nv;++k)
			size+=history[k].iw*history[k].ih;
		GUIPrint(ghMemDC, (w>>1)+2, h>>1, "hpos=%d/%d,  %fMB", histpos, history.size(), float(size<<2)/1048576);//
		for(int k=0, kx=(w>>1)+1, ky=(h>>1)+fontH, kEnd=history.size();k<kEnd;++k)//
		{
			GUIPrint(ghMemDC, kx, ky, "0x%08X", (int)history[k].buffer);//
			ky+=fontH;
			if(ky>=h-fontH)
				ky=(h>>1)+fontH, kx+=70;

		}
		hFont=(HFONT)SelectObject(ghMemDC, hFont);
	}
#endif
//	draw_line_v3(rgb, w, h, 0, 0, 8, 1, 0x000000);
//	draw_line_v3(rgb, w, h, 8, 1+10, 1, 8+10, 0x000000);
//	draw_line_brush(rgb, w, h, BRUSH_LARGE_RIGHT45, 4, 4, 4+8, 4+1, 0x000000);
//	draw_line_brush(rgb, w, h, BRUSH_LARGE_SQUARE, 10, 10, 20, 20, 0x000000);
//	draw_line_brush(rgb, w, h, BRUSH_LARGE_RIGHT45, 10, 10, 20, 20, 0x000000);
//	draw_line_v2(rgb, w, h, 1, 0, 0, 2, 0x000000);
//	draw_line_v2(rgb, w, h, 3, 0, 0, 3, 0x000000);
//	draw_line_v2(rgb, w, h, 0, 0, 0, 1, 0x000000);
//	draw_line_v2(rgb, w, h, 1, 0, 0, 1, 0x000000);
//	draw_line_v2(rgb, w, h, 0, 0, 2, 4, 0x000000);
//	draw_line_v2(rgb, w, h, 0, 0, 1, 1, 0x000000);
//	draw_line_v2(rgb, w, h, 0, 0, 3, 1, 0x000000);
//	draw_line_v2(rgb, w, h, 3, 0, 0, 1, 0x000000);
//	draw_line_v2(rgb, w, h, 5, 0, 0, 4, 0x000000);//x
//	draw_line_v2(rgb, w, h, 0, 0, 5, 4, 0x000000);//v
//	draw_line_v2(rgb, w, h, 0, 0, 3, 1, 0x000000);
//	draw_line_v2(rgb, w, h, 0, 0, 1, 3, 0x000000);
//	draw_line_v2(rgb, w, h, 1, 0, 0, 3, 0x000000);
//	draw_line_v2(rgb, w, h, 20, 0, 0, 20, 0x000000);
//	draw_line_v2(rgb, w, h, 0, 0, 10, 5, 0x000000);
//	draw_line_brush(image, iw, ih, BRUSH_LARGE_RIGHT45, 410, 20, 390, 10, 0xFFFF00FF);
//	draw_line_brush(image, iw, ih, BRUSH_LARGE_RIGHT45, 100, 20, -50, 20, 0xFFFF00FF);
//	draw_line_v2(image, iw, ih, 204, 330, 313, 272, 0xFFFF00FF);
//	draw_line_aa_v2(rgb, w, h, w/2+1.5, h/2+1.5, w/2+51.5, h/2+12.5, 10, 0);
//	draw_line_aa_v2(rgb, w, h, 1.5, 1.5, 21.5, 12.5, 10, 0);
//	draw_line_aa(rgb, w, h, 0, 0, 10, 1, 0);
//	draw_line_v2(rgb, w, h, 0, 0, 43, 1, 0);//
//	draw_line_v2(rgb, w, h, 0, 0, 11, 10, 0);//
//	emulate_cleartype();

	//blit
#if 1
	if(debugmode)
	{
		prof_print(ghMemDC, w-400, w-200);
		//prof.clear();
		//prof_start();
		BitBlt(ghDC, 0, 0, w, h, ghMemDC, 0, 0, SRCCOPY);
		prof_add("BitBlt");
		
		static int frame_count=-1;
		++frame_count;
		//GUITPrint(ghDC, w>>1, (h>>1)-16*3, "m(%d, %d)prev", prev_mx, prev_my);//
		//GUITPrint(ghDC, w>>1, (h>>1)-16*2, "m(%d, %d)current", mx, my);//
		GUITPrint(ghDC, w>>1, (h>>1)-16, "DEBUG %08X(%d, %d), (%d, %d), temp=%d, %d\t\t", redrawtype, redrawrect.i.x, redrawrect.i.y, redrawrect.f.x, redrawrect.f.y, (int)use_temp_buffer0, frame_count);
		//MoveToEx(ghDC, prev_mx, prev_my, 0), LineTo(ghDC, mx, my);//hMemDC ruins rgb
		if(redrawrect.i.x<redrawrect.f.x&&redrawrect.i.y<redrawrect.f.y)
		{
			HBRUSH hBrush=(HBRUSH)GetStockObject(HOLLOW_BRUSH);//
			hBrush=(HBRUSH)SelectObject(ghDC, hBrush);
			Rectangle(ghDC, redrawrect.i.x, redrawrect.i.y, redrawrect.f.x, redrawrect.f.y);
			hBrush=(HBRUSH)SelectObject(ghDC, hBrush);
		}
	}
	else//normal
	{
		if(redrawrect.i.x<redrawrect.f.x&&redrawrect.i.y<redrawrect.f.y)
			BitBlt(ghDC, redrawrect.i.x, redrawrect.i.y, redrawrect.f.x-redrawrect.i.x, redrawrect.f.y-redrawrect.i.y, ghMemDC, redrawrect.i.x, redrawrect.i.y, SRCCOPY);
		prof_add("BitBlt");
		prof_print(ghDC, w-400, w-200);
		//GUITPrint(ghDC, w>>1, (h>>1)-16, "%08X(%d, %d), (%d, %d), temp=%d\t\t", redrawtype, redrawrect.i.x, redrawrect.i.y, redrawrect.f.x, redrawrect.f.y, (int)use_temp_buffer0);//
	}
#endif

	print_errors(ghDC);
	//if(broken)
	//{
	//	if(broken_line==-1)
	//		GUIPrint(ghDC, w>>1, (h>>1)-16, "ERROR: 0x%08X(%d)", broken, broken);
	//	else
	//		GUIPrint(ghDC, w>>1, (h>>1)-16, "ERROR: 0x%08X(%d), line %d", broken, broken, broken_line);
	//}
}
#ifdef MOUSE_POLLING_TEST
void			render2()//mouse polling test
{
/*	const int npositions=50;
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
	p_idx=(p_idx+1)%npositions;//increment idx//*/
	
	prof_add("entry");
	static int *rgb2=nullptr, w2=0, h2=0;
	if(!rgb2||w2!=w||h2!=h)
	{
		w2=w, h2=h, rgb2=(int*)realloc(rgb2, rgbn<<2);
		memcpy(rgb2, rgb, rgbn<<2);
	}
	else
		memcpy(rgb, rgb2, rgbn<<2);
	prof_add("memcpy");

	const char msg[]="Hello World!";
	HFONT hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);//
	hFont=(HFONT)SelectObject(ghMemDC, hFont);//

	//memcpy loop				//will crash with small window
	GUIPrint(ghMemDC, 0, 0, msg);//59x13
	const int dx=59, dy=13;
	int bmp[dy*dx];
	for(int ky=0;ky<dy;++ky)//prep
		memcpy(bmp+dx*ky, rgb+w*ky, dx<<2);
	prof_add("prep");
	for(int k=0;k<1000;++k)//test
	{
		int x=rand()%(w-dx), y=rand()%(h-dy);
		for(int ky=0;ky<dy;++ky)
			memcpy(rgb+w*(y+ky)+x, bmp+dx*ky, dx<<2);//0.9ms
			//for(int kx=0;kx<dx;++kx)//1.7ms
			//	rgb[w*(y+ky)+x+kx]=bmp[dx*ky+kx];
	}

	//hMemDC	22ms, 45fps
	//for(int k=0;k<1000;++k)
	//	GUIPrint(ghMemDC, rand()%w, rand()%(h+1), msg);//

	prof_add("test");
	hFont=(HFONT)SelectObject(ghMemDC, hFont);//
	
	prof_print();
	prof.clear();
	prof_start();
	BitBlt(ghDC, 0, 0, w, h, ghMemDC, 0, 0, SRCCOPY);
	prof_add("exit");
}
#endif
int				ask_to_save()
{
	swprintf_s(g_wbuf, L"Save changes to %s?", filename.c_str());
	int result=MessageBoxW(ghWnd, g_wbuf, L"Paint++", MB_YESNOCANCEL);
	return result;
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
void			displayhelp()
{
#if 0
	const char *help[]=
	{
		"Ctrl 0",		"Reset zoom",
		"Ctrl +/-",		"Zoom in/out",
		"Ctrl O",		"Open",
		"Ctrl S",		"Save",
		"Ctrl Shift S", "Save as",
		"Ctrl A",		"Select all",
		"Ctrl X/C/V",	"Cut/Copy/Paste",
		"Esc",			"Deselect",
		"Delete",		"Delete selection",
		"Ctrl G",		"Toggle grid",
		"Ctrl Z/Y",		"Undo/redo",
		"Ctrl I",		"Invert image/selection color",
		"Ctrl N",		"New Image",
		"F1",			"Show help",
		"F7",			"Toggle debug mode",
	};
	const int nparts=sizeof help>>2;
	int length=0;
	HFONT hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
	hFont=(HFONT)SelectObject(ghMemDC, hFont);
	SIZE s;
	GetTextExtentPoint32A(ghMemDC, " Ctrl Shift S ", 14, &s);
//	GetTextExtentPoint32A(ghMemDC, "              ", 14, &s);
	int spaces_px=s.cx;
	for(int k=0;k+1<nparts;k+=2)
	{
		int padding=14;
		int commandlen=strlen(help[k]);
		if(GetTextExtentPoint32A(ghMemDC, help[k], commandlen, &s))
			padding=(spaces_px-s.cx)/3+commandlen;
		length+=sprintf_s(g_buf+length, g_buf_size-length, "%*s:  %s\n", padding, help[k], help[k+1]);
	//	length+=sprintf_s(g_buf+length, g_buf_size-length, "%14s: %s\n", help[k], help[k+1]);
	}
	hFont=(HFONT)SelectObject(ghMemDC, hFont);
	MessageBoxA(ghWnd, g_buf, "Help", MB_OK);
#endif
	messagebox(ghWnd, L"Help",
		L"Ctrl 0:\t\tReset zoom\n"
		L"Ctrl +/-:\t\tZoom in/out\n"
		L"Ctrl O:\t\tOpen\n"
		L"Ctrl S:\t\tSave\n"
		L"Ctrl Shift S:\tSave as\n"
		L"Ctrl A:\t\tSelect all\n"
		L"Ctrl X/C/V:\tCut/Copy/Paste\n"
		L"Esc:\t\tDeselect\n"
		L"Delete:\t\tDelete selection\n"
		L"Ctrl G:\t\tToggle grid\n"
		L"Ctrl Z/Y:\t\tUndo/redo\n"
		L"Ctrl I:\t\tInvert image/selection color\n"
		L"Ctrl N:\t\tNew Image\n"
		L"F1:\t\tShow help\n"
		L"F4:\t\tToggle profiler\n"
		L"F7:\t\tToggle debug mode\n"
		L"Line tool:\n"
		L"Ctrl:\t\tdraw antialiased lines\n"
		L"D:\t\tdraw gradient\n"
		L"\n"
#ifdef _DEBUG
		L"Debug version. "
#else
		L"Release version. "
#endif
		L"Built on %S, %S",
		__DATE__, __TIME__);
}
Point			p1, p2;
long			__stdcall WndProc(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
//#ifndef RELEASE//at first message, error 126 the specified module could not be found
//	check(__LINE__);//
//#endif//
	bool handled=false;
	int ret=0;
#if !defined RELEASE //&& defined _DEBUG
	char *debugmsg=nullptr;
#define			DEBUG(format, ...)	sprintf_s(g_buf, g_buf_size, format, __VA_ARGS__), debugmsg=g_buf;
#else
#define			DEBUG(format, ...)
#endif
	switch(message)
	{
	case WM_CREATE:
//	case WM_SHOWWINDOW:
		{
		//	RegisterDragDrop(hWnd, (IDropTarget*)&mdt);

			//menu bar
			int success=1;
#define		CREATE_MENU(MENU)	MENU=CreateMenu(), SYS_ASSERT(MENU)
			CREATE_MENU(hMenubar);
			CREATE_MENU(hMenuFile);
			CREATE_MENU(hMenuEdit);
			CREATE_MENU(hMenuView);
			CREATE_MENU(hMenuZoom);
			CREATE_MENU(hMenuImage);
			CREATE_MENU(hMenuColors);
			CREATE_MENU(hMenuHelp);
			//CREATE_MENU(hMenuTest);
#undef		CREATE_MENU
			success=AppendMenuW(hMenuFile, MF_STRING, IDM_FILE_NEW, L"&New\tCtrl+N");					SYS_ASSERT(success);		//[F]ILE
			success=AppendMenuW(hMenuFile, MF_STRING, IDM_FILE_OPEN, L"&Open...\tCtrl+O");				SYS_ASSERT(success);
			success=AppendMenuW(hMenuFile, MF_STRING, IDM_FILE_SAVE, L"&Save\tCtrl+S");					SYS_ASSERT(success);
			success=AppendMenuW(hMenuFile, MF_STRING, IDM_FILE_SAVE_AS, L"Save &As...\tCtrl+Shift+S");	SYS_ASSERT(success);
			success=AppendMenuW(hMenuFile, MF_SEPARATOR, 0, 0);											SYS_ASSERT(success);
			success=AppendMenuW(hMenuFile, MF_STRING, IDM_FILE_QUIT, L"E&xit\tAlt+F4");					SYS_ASSERT(success);
			success=AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuFile, L"&File");						SYS_ASSERT(success);

			success=AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_UNDO, L"&Undo\tCrtl+Z");							SYS_ASSERT(success);		//[E]DIT
			success=AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_REPEAT, L"&Repeat\tCrtl+Y");						SYS_ASSERT(success);
			success=AppendMenuW(hMenuEdit, MF_SEPARATOR, 0, 0);													SYS_ASSERT(success);
			success=AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_CUT, L"Cu&t\tCrtl+X");							SYS_ASSERT(success);
			success=AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_COPY, L"&Copy\tCrtl+C");							SYS_ASSERT(success);
			success=AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_PASTE, L"&Paste\tCrtl+V");						SYS_ASSERT(success);
			success=AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_CLEAR_SELECTION, L"C&lear Selection\tDel");		SYS_ASSERT(success);
			success=AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_SELECT_ALL, L"Select &All\tCtrl+A");				SYS_ASSERT(success);
			success=AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_RESET_SEL_SCALE, L"Reset Selection Scale\tE");	SYS_ASSERT(success);
			success=AppendMenuW(hMenuEdit, MF_SEPARATOR, 0, 0);													SYS_ASSERT(success);
			success=AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_COPY_TO, L"C&opy To...");						SYS_ASSERT(success);//TODO
			success=AppendMenuW(hMenuEdit, MF_STRING, IDM_EDIT_PASTE_FROM, L"Paste &From...");					SYS_ASSERT(success);//TODO
			success=AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuEdit, L"&Edit");								SYS_ASSERT(success);

			success=AppendMenuW(hMenuView, MF_STRING, IDM_VIEW_TOOLBOX, L"&Tool Box\tCtrl+T");		SYS_ASSERT(success);//?	//[V]IEW
			success=AppendMenuW(hMenuView, MF_STRING, IDM_VIEW_COLORBOX, L"&Color Box\tCtrl+L");	SYS_ASSERT(success);//?
			success=AppendMenuW(hMenuView, MF_STRING, IDM_VIEW_STATUSBAR, L"&Status Bar");			SYS_ASSERT(success);//?
			success=CheckMenuItem(hMenuView, IDM_VIEW_STATUSBAR, MF_CHECKED);						SYS_ASSERT(success);
		//	success=AppendMenuW(hMenuView, MF_STRING, IDM_VIEW_TEXT_TOOLBAR, L"T&ext Toolbar");		SYS_ASSERT(success);//not needed
			success=AppendMenuW(hMenuView, MF_SEPARATOR, 0, 0);										SYS_ASSERT(success);
		//	success=AppendMenuW(hMenuView, MF_STRING, IDM_VIEW_ZOOM, L"&Zoom");						SYS_ASSERT(success);
			success=AppendMenuW(hMenuView, MF_STRING|MF_POPUP, (unsigned)hMenuZoom, L"&Zoom");		SYS_ASSERT(success);
			success=AppendMenuW(hMenuView, MF_STRING, IDM_VIEW_VIEWBITMAP, L"&View Bitmap\tCtrl+F");SYS_ASSERT(success);
			success=AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuView, L"&View");					SYS_ASSERT(success);

			success=AppendMenuW(hMenuZoom, MF_STRING, IDSM_ZOOM_IN, L"&Zoom Out\tCtrl +");			SYS_ASSERT(success);		//VIEW/[Z]OOM
			success=AppendMenuW(hMenuZoom, MF_STRING, IDSM_ZOOM_OUT, L"&Zoom In\tCtrl -");			SYS_ASSERT(success);
			success=AppendMenuW(hMenuZoom, MF_STRING, IDSM_ZOOM_CUSTOM, L"C&ustom...");				SYS_ASSERT(success);//?
			success=AppendMenuW(hMenuZoom, MF_SEPARATOR, 0, 0);										SYS_ASSERT(success);
			success=AppendMenuW(hMenuZoom, MF_STRING, IDSM_ZOOM_SHOW_GRID, L"Show &Grid\tCtrl+G");	SYS_ASSERT(success);
			success=AppendMenuW(hMenuZoom, MF_STRING, IDSM_ZOOM_SHOW_THUMBNAIL, L"Show T&humbnail");SYS_ASSERT(success);//TODO

			success=AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_FLIP_ROTATE, L"&Flip/Rotate...\tCtrl+R");		SYS_ASSERT(success);	//IMAGE
			success=AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_SKETCH_SKEW, L"&Sketch/Skew...\tCtrl+W");		SYS_ASSERT(success);
			success=AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_INVERT_COLORS, L"&Invert Colors\tCtrl+I");		SYS_ASSERT(success);
			success=AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_ATTRIBUTES, L"&Attributes...\tCtrl+E");		SYS_ASSERT(success);
			success=AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_CLEAR_IMAGE, L"&Clear Image\tCtrl+Shift+N");	SYS_ASSERT(success);
			success=AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_CLEAR_ALPHA, L"Clear Al&pha\tA");				SYS_ASSERT(success);
			success=AppendMenuW(hMenuImage, MF_SEPARATOR, 0, 0);												SYS_ASSERT(success);
			success=AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_SET_CHANNEL, L"Set Cha&nnel");					SYS_ASSERT(success);//new
			success=AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_ASSIGN_CHANNEL, L"Assi&gn Channel...\tC");		SYS_ASSERT(success);//new
			success=AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_EXPORT_CHANNEL, L"&Export Channel...");		SYS_ASSERT(success);//new
			success=AppendMenuW(hMenuImage, MF_SEPARATOR, 0, 0);												SYS_ASSERT(success);
			success=AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_DRAW_OPAQUE, L"&Draw Opaque");					SYS_ASSERT(success);
			success=AppendMenuW(hMenuImage, MF_STRING, IDM_IMAGE_LINEAR, L"&Linear Interpolation");				SYS_ASSERT(success);
			success=CheckMenuItem(hMenuImage, IDM_IMAGE_LINEAR, interpolation_type?MF_CHECKED:MF_UNCHECKED);	SYS_ASSERT(success);
			success=AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuImage, L"&Image");							SYS_ASSERT(success);

			success=AppendMenuW(hMenuColors, MF_STRING, IDM_COLORS_EDITCOLORS, L"&Edit Colors...");		SYS_ASSERT(success);	//COLORS
		//	success=AppendMenuW(hMenuColors, MF_STRING, IDM_COLORS_EDITMASK, L"Edit &Mask...\tCtrl+M");	SYS_ASSERT(success);//?
			success=AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuColors, L"&Colors");					SYS_ASSERT(success);

		//	success=AppendMenuW(hMenuTest, MF_STRING, IDM_TEST_RADIO1, L".&BMP");									SYS_CHECK(success);			//TEST
		//	success=AppendMenuW(hMenuTest, MF_STRING, IDM_TEST_RADIO2, L".&TIFF");									SYS_CHECK(success);
		//	success=AppendMenuW(hMenuTest, MF_STRING, IDM_TEST_RADIO3, L".&PNG");									SYS_CHECK(success);
		//	success=AppendMenuW(hMenuTest, MF_STRING, IDM_TEST_RADIO4, L".&JPEG");									SYS_CHECK(success);
		//	//success=AppendMenuW(hMenuTest, MF_STRING, IDM_TEST_RADIO5, L".&FLIF");								SYS_CHECK(success);
		//	success=CheckMenuRadioItem(hMenuTest, IDM_TEST_RADIO1, IDM_TEST_RADIO4, IDM_TEST_RADIO3, MF_BYCOMMAND);	SYS_CHECK(success);
		//	success=AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuTest, L"&Temp Format");							SYS_CHECK(success);

			success=AppendMenuW(hMenuHelp, MF_STRING, IDM_HELP_TOPICS, L"&Help\tF1");		SYS_ASSERT(success);		//HELP
		//	success=AppendMenuW(hMenuHelp, MF_STRING, IDM_HELP_TOPICS, L"&Help Topics");	SYS_ASSERT(success);//?
			success=AppendMenuW(hMenuHelp, MF_SEPARATOR, 0, 0);								SYS_ASSERT(success);
			success=AppendMenuW(hMenuHelp, MF_STRING, IDM_HELP_ABOUT, L"&About Paint++");	SYS_ASSERT(success);
			success=AppendMenuW(hMenubar, MF_POPUP, (unsigned)hMenuHelp, L"&Help");			SYS_ASSERT(success);

			success=SetMenu(hWnd, hMenubar);			SYS_ASSERT(success);
		//	check_exit(L"Entry", __LINE__);
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
	case WM_TIMER:
		{
			int redrawtype=0;
			switch(wParam)
			{
			case TIMER_AIRBRUSH:
				redrawtype=REDRAW_IMAGE_PARTIAL;
				break;
			case TIMER_SCROLLBAR_INIT:
				timer_kill();
				timer_set(TIMER_SCROLLBAR_CONT);
			case TIMER_SCROLLBAR_CONT:
				switch(drag)
				{
				case D_HSCROLL_BACK:
					--spx;
					redrawtype=REDRAW_IMAGEWINDOW;
					break;
				case D_HSCROLL_FORWARD:
					++spx;
					redrawtype=REDRAW_IMAGEWINDOW;
					break;

				case D_VSCROLL_BACK:
					--spy;
					redrawtype=REDRAW_IMAGEWINDOW;
					break;
				case D_VSCROLL_FORWARD:
					++spy;
					redrawtype=REDRAW_IMAGEWINDOW;
					break;

				case D_THBOX_VSCROLL_BACK:
					thbox_posy-=10+16+th_h;
					redrawtype=REDRAW_ALL;
					break;
				case D_THBOX_VSCROLL_FORWARD:
					thbox_posy+=10+16+th_h;
					redrawtype=REDRAW_ALL;
					break;
				}
				break;
			}
			render(redrawtype, 0, w, 0, h);
		}
		break;
	case WM_PAINT:
		render(REDRAW_ALL, 0, w, 0, h);
		break;
	case WM_GETICON:
		if(wParam==ICON_BIG)
		{
			//https://stackoverflow.com/questions/4285890/how-to-load-a-small-system-icon
			//100: IDI_APPLICATION, 101: IDI_WARNING, 102: IDI_QUESTION, 103: IDI_ERROR, 104: IDI_INFORMATION, 105:IDI_WINLOGO, 106: IDI_SHIELD
			HINSTANCE hUser32=GetModuleHandleW(L"user32");
			HICON hIcon=(HICON)LoadImageW(hUser32, (wchar_t*)100, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
			if(!hIcon)
				formatted_error(L"LoadImageW", __LINE__);//spams '0 the operation completed successfully'
			ret=(int)hIcon, handled=true;
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
			switch(wp_lo)
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
				
			case IDM_ROTATE_BOX://subclassed
			//	fliprotate();
#if 0
				{
					const int nn=10;//
					static int notifications[nn]={0}, current=0;
					notifications[current]=wp_hi;
					int length=0;
					for(int k=0;k<nn;++k)
					{
						const char *a="???";
						switch(notifications[k])
						{
						case EN_SETFOCUS :a="EN_SETFOCUS";break;
						case EN_KILLFOCUS:a="EN_KILLFOCUS";break;
						case EN_CHANGE   :a="EN_CHANGE";break;
						case EN_UPDATE   :a="EN_UPDATE";break;
						case EN_ERRSPACE :a="EN_ERRSPACE";break;
						case EN_MAXTEXT  :a="EN_MAXTEXT";break;
						case EN_HSCROLL  :a="EN_HSCROLL";break;
						case EN_VSCROLL  :a="EN_VSCROLL";break;
						}
						length+=sprintf_s(g_buf+length, g_buf_size-length, "0x%04X %s%s\t\n", notifications[k], a, k==current?" <":"");
					//	GUITPrint(ghDC, 0, k<<4, "0x%04X %s%s\t", notifications[k], a, k==current?" <":"");
					}
					current=(current+1)%nn;//
					debugmsg=g_buf;

					//const int nn=10;//
					//static int notifications[nn]={0}, current=0;
					//notifications[current]=wp_hi;
					//for(int k=0;k<nn;++k)
					//{
					//	const char *a="???";
					//	switch(notifications[k])
					//	{
					//	case EN_SETFOCUS :a="EN_SETFOCUS";break;
					//	case EN_KILLFOCUS:a="EN_KILLFOCUS";break;
					//	case EN_CHANGE   :a="EN_CHANGE";break;
					//	case EN_UPDATE   :a="EN_UPDATE";break;
					//	case EN_ERRSPACE :a="EN_ERRSPACE";break;
					//	case EN_MAXTEXT  :a="EN_MAXTEXT";break;
					//	case EN_HSCROLL  :a="EN_HSCROLL";break;
					//	case EN_VSCROLL  :a="EN_VSCROLL";break;
					//	}
					//	GUITPrint(ghDC, 0, k<<4, "0x%04X %s%s\t", notifications[k], a, k==current?" <":"");
					//}
					//current=(current+1)%nn;//

				//	DEBUG("%d\t", wp_hi);//
				//	int LOL_1=0;//
				}
#endif
				break;
			case IDM_STRETCH_H_BOX://subclassed
			case IDM_STRETCH_V_BOX:
			case IDM_SKEW_H_BOX:
			case IDM_SKEW_V_BOX:
				//stretchskew();
				//{
				//	DEBUG("%d\t", wp_hi);//
				//	//int LOL_1=0;//
				//}
				break;

				//file menu
			case IDM_FILE_NEW:
				image=hist_start(iw, ih);
				memset(image, 0xFF, image_size<<2);
			//	mark_modified();
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
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
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
				break;
			case IDM_EDIT_REPEAT:
				hist_redo(image);
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
				break;
			case IDM_EDIT_CUT:
				if(editcopy())
				{
					selection_remove();
					render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
				}
				break;
			case IDM_EDIT_COPY:
				editcopy();
				break;
			case IDM_EDIT_PASTE:
				editpaste();
				break;
			case IDM_EDIT_CLEAR_SELECTION:
				if(selection.nonzero())
				{
					memset(sel_buffer, 0xFF, sw*sh<<2);
					render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
				}
				break;
			case IDM_EDIT_SELECT_ALL:
				selection_selectall();
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
				break;
			case IDM_EDIT_RESET_SEL_SCALE:
				if(selection.nonzero())
				{
					selection_resetscale();
					render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
				}
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
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
				break;
			case IDSM_ZOOM_OUT:
				--logzoom, zoom*=0.5;
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
				break;
			case IDSM_ZOOM_CUSTOM:			unimpl();break;//a window with zoom options
			case IDSM_ZOOM_SHOW_GRID:
				showgrid=!showgrid;
				CheckMenuItem(hMenuZoom, IDSM_ZOOM_SHOW_GRID, showgrid?MF_CHECKED:MF_UNCHECKED);//deprecated
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
				break;
			case IDSM_ZOOM_SHOW_THUMBNAIL:	unimpl();break;//a small window with unzoomed visible part

				//image menu
			case IDM_IMAGE_FLIP_ROTATE:
				replace_colorbarcontents(CS_FLIPROTATE);
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
				break;
			case IDM_IMAGE_SKETCH_SKEW:
				replace_colorbarcontents(CS_STRETCHSKEW);
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
				break;
			case IDM_IMAGE_INVERT_COLORS:
				hist_premodify(image, iw, ih);
				invertcolor(image, iw, ih);
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
				break;
			case IDM_IMAGE_ATTRIBUTES:
				ShowWindow(hWndAttributes, SW_SHOWDEFAULT);
				break;
			case IDM_IMAGE_CLEAR_IMAGE:
				hist_premodify(image, iw, ih);
				memset(image, 0xFF, image_size<<2);
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
				break;
			case IDM_IMAGE_CLEAR_ALPHA:
				clear_alpha();
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
				break;
			case IDM_IMAGE_SET_CHANNEL:
				unimpl();
				break;
			case IDM_IMAGE_ASSIGN_CHANNEL:
				unimpl();
				break;
			case IDM_IMAGE_EXPORT_CHANNEL:
				unimpl();
				break;
			case IDM_IMAGE_DRAW_OPAQUE:
				selection_transparency=1<<int(selection_transparency==TRANSPARENT);
				//selection_transparency=selection_transparency==OPAQUE?TRANSPARENT:OPAQUE;
				CheckMenuItem(hMenuImage, IDM_IMAGE_DRAW_OPAQUE, selection_transparency==OPAQUE?MF_CHECKED:MF_UNCHECKED);//deprecated
				render(REDRAW_TOOLBAR, 0, w, 0, h);
				break;
			case IDM_IMAGE_LINEAR:
				interpolation_type=!interpolation_type;
				CheckMenuItem(hMenuImage, IDM_IMAGE_LINEAR, interpolation_type?MF_CHECKED:MF_UNCHECKED);//deprecated
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
				break;

				//colors menu
			case IDM_COLORS_EDITCOLORS:
				pickcolor(primarycolor, colorpalette[primary_idx], primary_alpha);
				render(REDRAW_COLORPALETTE, 0, w, 0, h);
				break;
			case IDM_COLORS_EDITMASK:
				ShowWindow(hWndMask, SW_SHOW);
				break;

				//temp format menu		TODO: move this option to a submenu
			//case IDM_TEST_RADIO1:
			//	CheckMenuRadioItem(hMenuTest, IDM_TEST_RADIO1, IDM_TEST_RADIO4, IDM_TEST_RADIO1, MF_BYCOMMAND);
			//	tempformat=IF_BMP;
			//	//MessageBeep(MB_ICONERROR);
			//	break;
			//case IDM_TEST_RADIO2:
			//	CheckMenuRadioItem(hMenuTest, IDM_TEST_RADIO1, IDM_TEST_RADIO4, IDM_TEST_RADIO2, MF_BYCOMMAND);
			//	tempformat=IF_TIFF;
			//	//MessageBeep(0xFFFFFFFF);
			//	break;
			//case IDM_TEST_RADIO3:
			//	CheckMenuRadioItem(hMenuTest, IDM_TEST_RADIO1, IDM_TEST_RADIO4, IDM_TEST_RADIO3, MF_BYCOMMAND);
			//	tempformat=IF_PNG;
			//	//MessageBeep(MB_ICONWARNING);
			//	break;
			//case IDM_TEST_RADIO4:
			//	CheckMenuRadioItem(hMenuTest, IDM_TEST_RADIO1, IDM_TEST_RADIO4, IDM_TEST_RADIO4, MF_BYCOMMAND);
			//	tempformat=IF_JPEG;
			//	//MessageBeep(MB_ICONINFORMATION);
			//	break;
			//case IDM_TEST_RADIO5:
			//	CheckMenuRadioItem(hMenuTest, IDM_TEST_RADIO1, IDM_TEST_RADIO4, IDM_TEST_RADIO5, MF_BYCOMMAND);
			//	tempformat=IF_FLIF;
			//	//MessageBeep(MB_ICONINFORMATION);
			//	break;

				//help menu
			case IDM_HELP_TOPICS:
				displayhelp();
				break;
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
#if 1
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
			else if(p2.x-=vscroll.dwidth, vscroll.dwidth&&mx>=p2.x)//image vertical scrollbar
				mousepos=MP_VSCROLL;
			else if(p2.y-=hscroll.dwidth, hscroll.dwidth&&my>p2.y)//image horizontal scrollbar
				mousepos=MP_HSCROLL;
			else//image field
			{
				int ypos=(my>imagemarks[0].y)+(my>imagemarks[1].y)+(my>imagemarks[2].y)+(my>imagemarks[3].y)+(my>imagemarks[4].y)+(my>imagemarks[5].y);
				int xpos=(mx>imagemarks[0].x)+(mx>imagemarks[1].x)+(mx>imagemarks[2].x)+(mx>imagemarks[3].x)+(mx>imagemarks[4].x)+(mx>imagemarks[5].x);
				switch(ypos)
				{
				case 0:
					mousepos=MP_IMAGEWINDOW;
					break;
				case 1:
					switch(xpos)
					{
					case 1:	mousepos=MP_RESIZE_TL;	break;
					case 3:	mousepos=MP_RESIZE_TOP;	break;
					case 5:	mousepos=MP_RESIZE_TR;	break;
					default:mousepos=MP_IMAGEWINDOW;break;
					}
					break;
				case 2:
				case 4:
					switch(xpos)
					{
					case 2:
					case 3:
					case 4:	mousepos=MP_IMAGE;		break;
					default:mousepos=MP_IMAGEWINDOW;break;
					}
					break;
				case 3:
					switch(xpos)
					{
					case 1:	mousepos=MP_RESIZE_LEFT;	break;
					case 2:
					case 3:
					case 4:	mousepos=MP_IMAGE;			break;
					case 5:	mousepos=MP_RESIZE_RIGHT;	break;
					default:mousepos=MP_IMAGEWINDOW;	break;
					}
					break;
				case 5:
					switch(xpos)
					{
					case 1:	mousepos=MP_RESIZE_BL;		break;
					case 3:	mousepos=MP_RESIZE_BOTTOM;	break;
					case 5:	mousepos=MP_RESIZE_BR;		break;
					default:mousepos=MP_IMAGEWINDOW;	break;
					}
					break;
				case 6:
					mousepos=MP_IMAGEWINDOW;
					break;
				}
				//Point s_end;
				//image2screen(iw, ih, s_end.x, s_end.y);
				//if(mx>=p1.x+4&&mx<s_end.x&&my>=p1.y+5&&my<s_end.y)
				//	mousepos=MP_IMAGE;
				//else
				//	mousepos=MP_IMAGEWINDOW;
			}
		}
#endif
		break;
	case WM_SETCURSOR://0x0020
		if(mousepos!=MP_NONCLIENT)
		{
			switch(mousepos)
			{
			case MP_RESIZE_TL:		hcursor=hcursor_sizenwse;	break;
			case MP_RESIZE_TOP:		hcursor=hcursor_sizens;		break;
			case MP_RESIZE_TR:		hcursor=hcursor_sizenesw;	break;
			case MP_RESIZE_LEFT:	hcursor=hcursor_sizewe;		break;
			case MP_RESIZE_RIGHT:	hcursor=hcursor_sizewe;		break;
			case MP_RESIZE_BL:		hcursor=hcursor_sizenesw;	break;
			case MP_RESIZE_BOTTOM:	hcursor=hcursor_sizens;		break;
			case MP_RESIZE_BR:		hcursor=hcursor_sizenwse;	break;
			case MP_IMAGE:			hcursor=hcursor_cross;		break;
			default:				hcursor=hcursor_original;	break;
			}
			SetCursor(hcursor);
			//SetCursor(mousepos==MP_IMAGE?hcursor:hcursor_original);//TODO: other tool cursors
			ret=true, handled=true;
		}
		break;
	//case WM_NCMOUSEMOVE://0x00A0: mouse on border
	//	break;
	//case WM_NCLBUTTONDOWN://0x00A1: click on border
	//	break;
	case WM_MOUSEMOVE://0x0200		when a button is down, only WM_MOUSEMOVE is received
		mx=(short&)lParam, my=((short*)&lParam)[1];//client coordinates
#ifdef MOUSE_POLLING_TEST
		if(true)//mouse polling test
			render2();//
		else
#endif
		{
			int redrawtype=0;
			switch(drag)
			{
			case D_NONE:
				if(currentmode==M_ERASER||currentmode==M_MAGNIFIER||currentmode==M_BRUSH)
					redrawtype=REDRAW_IMAGE_PARTIAL;
				else
					goto skip_render;
				break;
			case D_DRAW:
				redrawtype=REDRAW_IMAGE_PARTIAL;
				break;
			case D_HSCROLL:
			case D_VSCROLL:
				redrawtype=REDRAW_IMAGEWINDOW;
				break;
			case D_SELECTION:
				redrawtype=REDRAW_IMAGE_PARTIAL;
				break;
			case D_THBOX_VSCROLL:
				redrawtype=REDRAW_THUMBBOX;
				break;
			case D_ALPHA1:
			case D_ALPHA2:
				redrawtype=REDRAW_COLORBAR;
				break;
			case D_RESIZE_TL:
			case D_RESIZE_TOP:
			case D_RESIZE_TR:
			case D_RESIZE_LEFT:
			case D_RESIZE_RIGHT:
			case D_RESIZE_BL:
			case D_RESIZE_BOTTOM:
			case D_RESIZE_BR:
			case D_SEL_RESIZE_TL:
			case D_SEL_RESIZE_TOP:
			case D_SEL_RESIZE_TR:
			case D_SEL_RESIZE_LEFT:
			case D_SEL_RESIZE_RIGHT:
			case D_SEL_RESIZE_BL:
			case D_SEL_RESIZE_BOTTOM:
			case D_SEL_RESIZE_BR:
				redrawtype=REDRAW_IMAGEWINDOW;
				break;
			default:
				goto skip_render;
			}
			render(redrawtype, 0, w, 0, h);
			prev_mx=mx, prev_my=my;
skip_render:;
			//int windowsize=100>>1;
			//int x1=mx-windowsize, x2=mx+windowsize, y1=my-windowsize, y2=my+windowsize;
			//x1=clamp(0, x1, w), x2=clamp(0, x2, w);
			//y1=clamp(0, y1, h), y2=clamp(0, y2, h);
			//BitBlt(ghDC, x1, y1, x2-x1, y2-y1, ghMemDC, x1, y1, SRCCOPY);//
			//BitBlt(ghDC, 0, 0, w, h, ghMemDC, 0, 0, SRCCOPY);//
		}
		//if(drag||currentmode==M_ERASER||currentmode==M_MAGNIFIER||currentmode==M_BRUSH)
		//	render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
		break;
	case WM_LBUTTONDOWN://left click
	case WM_LBUTTONDBLCLK:
#if 1
		{
			kb[VK_LBUTTON]=true;
			const int scrollbarwidth=17;
			switch(mousepos)
			{
			case MP_THUMBNAILBOX:														//click on THUMBNAIL BOX
				if(thbox_vscroll.dwidth&&mx>=w-scrollbarwidth)//thbox vscrollbar
				{
					if(my>h-74*showcolorbar-scrollbarwidth)//down arrow
						thbox_posy+=10+16+th_h, drag=D_THBOX_VSCROLL_FORWARD, timer_set(TIMER_SCROLLBAR_INIT);
					else if(my>scrollbarwidth)//slider
						drag=D_THBOX_VSCROLL, thbox_vscroll.m_start=my, thbox_vscroll.s0=thbox_vscroll.start;
					else//up arrow
						thbox_posy-=10+16+th_h, drag=D_THBOX_VSCROLL_BACK, timer_set(TIMER_SCROLLBAR_INIT);
				}
				else//click on thumbnail
				{
					int frame_number=(my+thbox_posy-10)/(16+th_h+10);
					if(frame_number!=current_frame)
					{
						if(modified)
						{
							save_frame(current_frame);
							assign_thumbnail(thumbnails[current_frame], image);
							//load_thumbnails();//
						}
						load_frame(frame_number), current_frame=frame_number;
						modified=false;
					//	hist_start(image);
					}
				}
				break;
			case MP_COLORBAR:															//click on COLOR BAR
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
							if(idx<ROTATE_ARBITRARY)
								fliprotate(idx);
						}
						break;
					//case CS_STRETCHSKEW://no buttons
					//	break;
					}
				}
				break;
			case MP_TOOLBAR:															//click on TOOLBAR
				if(mx>=3&&mx<53&&my>5&&my<205)//tool box
				{
					int currentmode0=currentmode;
					int kx=(mx-3)/25, ky=(my-5)/25;
					currentmode=ky<<1|kx;//TODO: change cursor icon
					if(currentmode==M_TEXT&&logzoom)
						currentmode=prevmode;
					if(prevmode<=M_RECT_SELECTION&&currentmode>M_RECT_SELECTION&&!selection_persistent)//switch away from selection
					{
						selection_stamp(true);
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
						if(polygon.size()>=3)
							hist_premodify(image, iw, ih);
						polygon_draw(image, polygon_leftbutton?primarycolor:secondarycolor, polygon_type==ST_FILL==polygon_leftbutton?primarycolor:secondarycolor);
						//int c3, c4;
						//if(polygon_type==ST_FILL==polygon_leftbutton)
						//	c3=primarycolor, c4=secondarycolor;
						//else
						//	c3=secondarycolor, c4=primarycolor;
						//polygon_draw(image, c3, c4);
						use_temp_buffer=false;
					}
					//if(currentmode==M_MAGNIFIER&&prevmode!=M_MAGNIFIER&&logzoom>0)
					//	magnifier_level=clamp(0, logzoom, 5);
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
							selection_transparency=1<<int(selection_transparency==TRANSPARENT);
							//selection_transparency=selection_transparency==OPAQUE?TRANSPARENT:OPAQUE;
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
						magnifier_level=magnification_levels[(my-210)/22<<1|(mx-7)/21];
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
			case MP_VSCROLL://image vertical scrollbar									//click on SCROLLBARS
				//{
				//	int dy=shift(h/10, -logzoom);
				//	if(dy<=0)
				//		dy=1;
					if(my>=p2.y-scrollbarwidth-hscroll.dwidth)//down arrow
						++spy, drag=D_VSCROLL_FORWARD, timer_set(TIMER_SCROLLBAR_INIT);
					else if(my>=p1.y+scrollbarwidth)//vertical slider
						drag=D_VSCROLL, vscroll.m_start=my, vscroll.s0=vscroll.start;
					else//up arrow
						--spy, drag=D_VSCROLL_BACK, timer_set(TIMER_SCROLLBAR_INIT);
				//}
				break;
			case MP_HSCROLL://image horizontal scrollbar
				//{
				//	int dx=shift(w/10, -logzoom);
				//	if(dx<=0)
				//		dx=1;
					if(mx>=p2.x-scrollbarwidth-vscroll.dwidth)//right arrow
						++spx, drag=D_HSCROLL_FORWARD, timer_set(TIMER_SCROLLBAR_INIT);
					else if(mx>=p1.x+scrollbarwidth)//horizontal slider
						drag=D_HSCROLL, hscroll.m_start=mx, hscroll.s0=hscroll.start;
					else//left arrow
						--spx, drag=D_HSCROLL_BACK, timer_set(TIMER_SCROLLBAR_INIT);
				//}
				break;
			case MP_RESIZE_TL:
			case MP_RESIZE_TOP:
			case MP_RESIZE_TR:
			case MP_RESIZE_LEFT:
			case MP_RESIZE_RIGHT:
			case MP_RESIZE_BL:
			case MP_RESIZE_BOTTOM:
			case MP_RESIZE_BR:
				if(selection.nonzero())//resize selection
				{
					drag=mousepos-MP_RESIZE_TL+D_SEL_RESIZE_TL;
					sel0=selection;
					//xanchor=anchors[(drag-D_SEL_RESIZE_TL)<<1], yanchor=anchors[(drag-D_SEL_RESIZE_TL)<<1|1];
					//sort_using_anchor(xanchor, sel_start.x, sel_end.x);
					//sort_using_anchor(yanchor, sel_start.y, sel_end.y);
				}
				else//resize image
					drag=mousepos-MP_RESIZE_TL+D_RESIZE_TL;
				break;
			case MP_IMAGEWINDOW:														//click on IMAGE WINDOW
			case MP_IMAGE:
				switch(currentmode)
				{
				case M_FREE_SELECTION:
				case M_RECT_SELECTION:
					{
						Rect r=selection;
						r.image2screen();
						if(r.mousetest_orderless(mx, my))//drag selection
						//if(mx>=sel_s1.x&&mx<sel_s2.x&&my>=sel_s1.y&&my<sel_s2.y)
						{
							if(kb[VK_CONTROL]||kb[VK_SHIFT])
								selection_stamp(true);
							drag=D_SELECTION;
						}
						else//new selection
						{
							selection_stamp(true);

							selection.i.set(mx, my);
							selection.i.screen2image_rounded();
							selection.i.clamp(0, iw, 0, ih);
							selection.f=selection.i;
						//	screen2image_rounded(mx, my, sel_start.x, sel_start.y);
						//	//screen2image(mx, my, sel_start.x, sel_start.y);
						//	Point start(0, 0), end(iw, ih);
						//	sel_start.x=clamp(start.x, sel_start.x, end.x);
						//	sel_start.y=clamp(start.y, sel_start.y, end.y);
						//	sel_end=sel_start;
						//	selection_assign();

							drag=D_DRAW;
						}
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
						idx=shift(dx, -logzoom);
						idy=shift(dy, -logzoom);
						//screen2image(dx, dy, idx, idy);
						spx=imx-(idx>>1), spy=imy-(idy>>1);
					}
					else
						logzoom=0, zoom=1;
					currentmode=prevmode;
					break;
				case M_AIRBRUSH:
					drag=D_DRAW;
					timer_set(TIMER_AIRBRUSH);
					//if(!timer)
					//	SetTimer(ghWnd, 0, airbrush_timer, 0), timer=true;
					break;
				case M_TEXT:
					{
						int padding=2;
						Rect tr2=textrect;
						tr2.image2screen();
						if(tr2.mousetest(mx, my, padding))//drag textbox
					//	if(mx>=text_s1.x-padding&&mx<text_s2.x+padding&&my>=text_s1.y-padding&&my<text_s2.y+padding)
					//	if(mx>=text_s1.x&&mx<text_s2.x&&my>=text_s1.y&&my<text_s2.y)
							drag=D_SELECTION;
						else//define new textbox
						{
							print_text();
							textrect.i.set(mx, my);
							textrect.i.screen2image();
							textrect.f=textrect.i;
							//screen2image(mx, my, text_start.x, text_start.y);
							//text_end=text_start;
							drag=D_DRAW;
						}
					}
					break;
				case M_LINE:
					draw_line_frame1=true;
					drag=D_DRAW;
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
			render(REDRAW_ALL, 0, w, 0, h);
			prev_drag=drag;
		}
#endif
		break;
	case WM_LBUTTONUP:
#if 1
#ifdef CAPTURE_MOUSE
		if(drag)
			ReleaseCapture();
#endif
		switch(drag)
		{
		case D_DRAW:
			switch(currentmode)
			{
			case M_FREE_SELECTION:
				freesel_select();
				break;
			case M_RECT_SELECTION:
				{
					selection.f.set(mx, my);
					selection.f.screen2image_rounded();
					selection_select(selection);

					//Point p1, p2;//sorted-bounded image coordinates
					//selection_assign_mouse(start_mx, start_my, mx, my, sel_start, sel_end, p1, p2);
					//selection_select(p1, p2);
					//selid=p2-p1;
				}
				break;
			case M_PICK_COLOR:
				primarycolor=pick_color_mouse(image, mx, my);//with alpha
				primary_alpha=primarycolor>>24;
				currentmode=prevmode;
				break;
			case M_AIRBRUSH:
				if(timer)
					KillTimer(ghWnd, 0), timer=false;
				break;
			case M_TEXT:
				{
					//Point p1, p2;
					//image2screen(text_start.x, text_start.y, p1.x, p1.y);
					//image2screen(text_end.x, text_end.y, p2.x, p2.y);
					//selection_sort(p1, p2, text_s1, text_s2);

					move_textbox();
					int prevshow=ShowWindow(hTextbox, SW_SHOWNA);
				//	draw_line_mouse(image, start_mx, start_my, mx, my, rand()<<15|rand());
				}
				break;
			case M_LINE:
				hist_premodify(image, iw, ih);
				draw_line_mouse(image, start_mx, start_my, mx, my, primarycolor, secondarycolor, true);
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
			break;
		case D_HSCROLL_BACK:
		case D_HSCROLL_FORWARD:
		case D_VSCROLL_BACK:
		case D_VSCROLL_FORWARD:
		case D_THBOX_VSCROLL_BACK:
		case D_THBOX_VSCROLL_FORWARD:
			if(timer==TIMER_SCROLLBAR_INIT||timer==TIMER_SCROLLBAR_CONT)
				timer_kill();
			break;
		case D_RESIZE_TL:
		case D_RESIZE_TOP:
		case D_RESIZE_TR:
		case D_RESIZE_LEFT:
		case D_RESIZE_RIGHT:
		case D_RESIZE_BL:
		case D_RESIZE_BOTTOM:
		case D_RESIZE_BR:
			{
				imresize_setparams(newsize, srcstart, dststart, copysize);
				//xanchor=anchors[(drag-D_RESIZE_TL)<<1], yanchor=anchors[(drag-D_RESIZE_TL)<<1|1];//-1: resize from start, 0: original, 1: resize from end
				//newsize.set(mx, my);//mouse coordinates
				////if(logzoom>0)
				////{
				////	int halfnum=1<<(logzoom-1);//round instead of truncation
				////	newsize.x+=halfnum, newsize.y+=halfnum;
				////}
				//newsize.screen2image_rounded();
				//imresize_setparams(xanchor, newsize.x, iw, newsize.x, srcstart.x, dststart.x, copysize.x);
				//imresize_setparams(yanchor, newsize.y, ih, newsize.y, srcstart.y, dststart.y, copysize.y);
				if(newsize.x!=iw||newsize.y!=ih)
				{
					int count=newsize.x*newsize.y;
					int *image2=(int*)malloc(count<<2);
					imresize(image2, newsize, image, Point(iw, ih), srcstart, dststart, copysize);
					hist_postmodify(image2, newsize.x, newsize.y);
				}
			}
			break;
		case D_SEL_RESIZE_TL:
		case D_SEL_RESIZE_TOP:
		case D_SEL_RESIZE_TR:
		case D_SEL_RESIZE_LEFT:
		case D_SEL_RESIZE_RIGHT:
		case D_SEL_RESIZE_BL:
		case D_SEL_RESIZE_BOTTOM:
		case D_SEL_RESIZE_BR:
			selection=sel0;
			selection_resize_mouse(selection, mx, my, start_mx, start_my);
			break;
		}
		kb[VK_LBUTTON]=false, prev_drag=drag=D_NONE;
		render(REDRAW_ALL, 0, w, 0, h);
#endif
		break;
	case WM_RBUTTONDOWN://right click
	case WM_RBUTTONDBLCLK:
		{
			kb[VK_RBUTTON]=true;
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
				case M_TEXT:
					break;
				case M_FILL:
					hist_premodify(image, iw, ih);
					fill_mouse(image, mx, my, secondarycolor);
					break;
				case M_MAGNIFIER://reset zoom
					logzoom=0, zoom=1;
					currentmode=prevmode;
					break;
				case M_AIRBRUSH:
					drag=D_DRAW;
					if(!timer)
						SetTimer(ghWnd, 0, airbrush_timer, 0), timer=true;
					break;
				case M_LINE:
					draw_line_frame1=true;
					drag=D_DRAW;
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
			render(REDRAW_ALL, 0, w, 0, h);
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
				secondary_alpha=secondarycolor>>24;
				currentmode=prevmode;
				break;
			case M_AIRBRUSH:
				if(timer)
					KillTimer(ghWnd, 0), timer=false;
				break;
			case M_LINE:
				hist_premodify(image, iw, ih);
				draw_line_mouse(image, start_mx, start_my, mx, my, secondarycolor, primarycolor, true);
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
		kb[VK_RBUTTON]=false, prev_drag=drag=D_NONE;
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
		render(REDRAW_ALL, 0, w, 0, h);
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
					render(REDRAW_ALL, 0, w, 0, h);
				}
				else
					render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
			}
		//	else if(showthumbnailbox&&mousepos==MP_THUMBNAILBOX)
			else if(showthumbnailbox&&mx>thbox_x1)//scroll thumbnailbox
			{
				if(thbox_vscroll.dwidth)
					thbox_posy+=(!mw_forward-mw_forward)*(10+16+th_h);
				render(REDRAW_THUMBBOX, 0, w, 0, h);
			}
			else//scroll image
			{
				const int scroll_divisor=5;//scroll by 1/5th of screen
				Point delta(0, 0);
				if(vscroll.dwidth&&!kb[VK_SHIFT])
				{
					Point new_sp=imagewindow.i;
					new_sp.y+=conditional_negate(maximum(1, imagewindow.dy()/scroll_divisor), mw_forward);
					new_sp.screen2image();
					delta.y=new_sp.y-spy;
					spy=new_sp.y;
				}
				else if(hscroll.dwidth)
				{
					Point new_sp=imagewindow.i;
					new_sp.x+=conditional_negate(maximum(1, imagewindow.dx()/scroll_divisor), mw_forward);
					new_sp.screen2image();
					delta.x=new_sp.x-spx;
					spx=new_sp.x;
				}
				if(currentmode==M_TEXT)
				{
					textrect.i+=delta;
					textrect.f+=delta;
					move_textbox();
				}
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
			}
			//if(currentmode==M_TEXT)
			//	SendMessageW(hFontcombobox, message, wParam, lParam);
		//	render(REDRAW_ALL, 0, w, 0, h);
		//	GUIPrint(ghDC, w>>1, h-16, "zoom=%lf", zoom);
		}
		break;
	case WM_KEYDOWN:case WM_SYSKEYDOWN:
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
			render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
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
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
			}
			break;
		case VK_OEM_PLUS:case VK_ADD://=, numpad+
			if(kb[VK_CONTROL])//zoom in
			{
				++logzoom, zoom*=2;
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
			}
			break;
		case VK_OEM_MINUS:case VK_SUBTRACT://-, numpad-
			if(kb[VK_CONTROL])//zoom out
			{
				--logzoom, zoom*=0.5;
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
			}
			break;
		case VK_HOME:
			spx=0;
			if(kb[VK_CONTROL])//ctrl home
				spy=0;
			render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
			break;
		case VK_END:
			{
				Point delta(imagewindow.dx(), imagewindow.dy());
				spx=iw-shift(delta.x, -logzoom);
				if(kb[VK_CONTROL])//ctrl end
					spy=ih-shift(delta.y, -logzoom);
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
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
		case 'E':
			if(kb[VK_CONTROL])//attributes
			{
				int prevshow=ShowWindow(hWndAttributes, SW_SHOWDEFAULT);
			}
			else if(selection.nonzero())
			{
				selection_resetscale();
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
			}
			break;
		case 'A':
			if(kb[VK_CONTROL])//select all
			{
				selection_selectall();
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
			}
			else//clear alpha
			{
				clear_alpha();
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
			}
			break;
		case 'X':
			if(kb[VK_CONTROL])//cut
				if(editcopy())
				{
					selection_remove();
					render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
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
				render(REDRAW_ALL, 0, w, 0, h);//tool switch
			}
			break;
		case VK_ESCAPE:
			if(selection.nonzero())//deselect
			{
				selection_stamp(true);
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
				replace_colorbarcontents(CS_TEXTOPTIONS&-(currentmode==M_TEXT));
			render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
			break;
		case VK_DELETE:
			if(selection.nonzero())//delete selection
			{
				selection_remove();
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
			}
			break;
		case 'G':
			if(kb[VK_CONTROL])//toggle grid
			{
				showgrid=!showgrid;
				CheckMenuItem(hMenuZoom, IDSM_ZOOM_SHOW_GRID, showgrid?MF_CHECKED:MF_UNCHECKED);//deprecated
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
			}
			break;
		case 'Z':
			if(kb[VK_CONTROL])//undo
			{
				hist_undo(image);
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
			}
			break;
		case 'Y':
			if(kb[VK_CONTROL])//redo
			{
				hist_redo(image);
				render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
			}
			break;
		case 'I':
			if(kb[VK_CONTROL])//invert color, leave alpha
			{
				hist_premodify(image, iw, ih);
				if(selection.nonzero())
					invertcolor(sel_buffer, sw, sh, true);
				else
					invertcolor(image, iw, ih);
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
			}
			break;
		case 'N':
			if(kb[VK_CONTROL])//clear image
			{
				if(kb[VK_SHIFT])
				{
					hist_premodify(image, iw, ih);
					memset(image, 0xFF, image_size<<2);
					render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
				}
				else if(modified)//new image
				{
					int result=ask_to_save();
					switch(result)
					{
					case IDYES:
						if(!save_media_as())
							break;
					case IDNO:
						{
							int success=SetWindowTextA(ghWnd, default_title);
							SYS_ASSERT(success);

							image=hist_start(iw, ih);
							memset(image, 0xFF, image_size<<2);
							render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
						}
						break;
					case IDCANCEL:
						break;
					}
				}
			}
			else//fill with noise
			{
				hist_premodify(image, iw, ih);
				for(int k=0;k<image_size;++k)
				//	image[k]=0xFFFFFFFF;
					image[k]=0xFF000000|rand()<<15|rand();
				//	image[k]=rand()<<30|rand()<<15|rand();
				render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
			}
			break;
		case VK_F1:
			displayhelp();
			break;
		case VK_F4:
			if(!kb[VK_MENU])//profiler
			{
				prof_toggle();
				render(REDRAW_ALL, 0, w, 0, h);
			}
			break;
		case VK_F7://debug mode
			debugmode=!debugmode;
			if(debugmode!=prof_on)
				prof_toggle();
			render(REDRAW_ALL, 0, w, 0, h);
			break;
		}
		break;
	case WM_KEYUP:case WM_SYSKEYUP:
		kb[wParam]=false;
		break;
	case WM_CLOSE:
//	case WM_DESTROY:
		if(modified)
		{
			int result=ask_to_save();
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
	if(ghDC)//draw statusbar [& debug info]
	{
#ifdef SHOW_WINMESSAGES
//#if !defined RELEASE //&& defined _DEBUG
		if(debugmode)
		{
			if(debugmsg)
			{
				RECT rect={0, 0, w, h};
				int bkColor2=0x808080|0xFFFFFF&(rand()<<15|rand());
				//auto p=(byte*)&bkColor2;
				//p[0]=0x80|p[0]>>1;
				//p[1]=0x80|p[1]>>1;
				//p[2]=0x80|p[2]>>1;
				int bkColor=SetBkColor(ghDC, bkColor2);
			//	int bkColor=SetBkColor(ghDC, 0xFFFF00);
				int height=DrawTextA(ghDC, debugmsg, -1, &rect, DT_EXPANDTABS);
			//	TextOutA(ghDC, 0, 0, debugmsg, strlen(debugmsg));
				SetBkColor(ghDC, bkColor);
			}
			//	GUINPrint(ghDC, 0, 0, w, h, debugmsg);
			//	GUITPrint(ghDC, 0, 0, debugmsg);
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
				GUITPrint(ghDC, w>>1, k<<4, "0x%04X %s,\t(%d, %d)%s", msg.message, wm2str(msg.message), msg.mx, msg.my, k==m_idx?" <":"    ");
			//	GUIPrint(ghDC, w>>1, k<<4, "0x%04X, (%d, %d)%s", msg.message, msg.mx, msg.my, k==m_idx?" <":"    ");
			//	GUIPrint(ghDC, w>>1, k<<4, "0x%04X%s", msg.message, k==m_idx?" <":"    ");
			}
			m_idx=(m_idx+1)%nmessages;
			GUIPrint(ghDC, w>>2, 0, "drag=%d, dialog=%d", (int)drag, g_processed);//

		//	GUIPrint(ghDC, w>>1, 0, "message=0x%04X, drag=%d", message, (int)drag);//
		}
#endif
		Rectangle(ghDC, -1, h-17, w+1, h+1);
	//	Rectangle(ghDC, 0, h-16, w, h);
		int size=0;
		for(int k=0, nv=history.size();k<nv;++k)//determine history size in pixels
			size+=history[k].iw*history[k].ih;
	//	GUIPrint(ghDC, w-200, h-16, "x%g, %d/%d, %.2lfMB", zoom, histpos, history.size(), float(size<<2)/1048576);//
		GUIPrint(ghDC, w-200, h-16, "%d/%d, %.2lfMB", histpos+1, history.size(), float(size<<2)/1048576);//
		int iw2, ih2;
		if(drag>D_RESIZE_START&&drag<D_RESIZE_SEL_START)//resize image
		{
			Point newsize, srcstart, dststart, copysize;
			imresize_setparams(newsize, srcstart, dststart, copysize);
			iw2=newsize.x, ih2=newsize.y;
		}
		else
			iw2=iw, ih2=ih;
		if(selection.nonzero())
		{
			int selw=abs(selection.dx()), selh=abs(selection.dy());
			GUIPrint(ghDC, w>>1, h-16, "w=%d, h=%d, zoom: x%g, sel: %dx%d", iw2, ih2, zoom, selw, selh);
		}
		else
			GUIPrint(ghDC, w>>1, h-16, "w=%d, h=%d, zoom: x%g", iw2, ih2, zoom);
		if(mousepos==MP_IMAGE)
		{
			Point pm;
			screen2image(mx, my, pm.x, pm.y);
			if(!icheck(pm.x, pm.y))
			{
				auto p=(byte*)(image+iw*pm.y+pm.x);
				int color=swap_rb(*(int*)p);//DIB: 0xAARRGGBB -> 0xAABBGGRR
				int a=p[3], r=p[2], g=p[1], b=p[0];
				if(drag==D_DRAW)
				{
					Point sm;
					screen2image(start_mx, start_my, sm.x, sm.y);
					GUITPrint(ghDC, 0, h-16, "XY=(%d, %d), %dx%d,\tRGBA=(%d, %d, %d, %d)\t=0x%08X", pm.x, pm.y, pm.x-sm.x, pm.y-sm.y, r, g, b, a, color);
					//GUITPrint(ghDC, 0, h-16, "XY=(%d, %d), %dx%d,\tARGB=(%d, %d, %d, %d)\t=0x%02X%02X%02X%02X", pm.x, pm.y, pm.x-sm.x, pm.y-sm.y, a, r, g, b, a, r, g, b);
				}
				else
					GUITPrint(ghDC, 0, h-16, "XY=(%d, %d),\tRGBA=(%d, %d, %d, %d)\t=0x%08X", pm.x, pm.y, r, g, b, a, color);
					//GUITPrint(ghDC, 0, h-16, "XY=(%d, %d),\tARGB=(%d, %d, %d, %d)\t=0x%02X%02X%02X%02X", pm.x, pm.y, a, r, g, b, a, r, g, b);
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
	}
//#ifndef RELEASE
//	check(__LINE__);
//#endif
	if(handled)
		return ret;
	return DefWindowProcA(hWnd, message, wParam, lParam);
}
int				__stdcall WinMain(HINSTANCE__ *hInstance, HINSTANCE__ *hPrevInstance, char *lpCmdLine, int nCmdShow)
{
	ghInstance=hInstance;
	tagWNDCLASSEXA wndClassEx=
	{
		sizeof(tagWNDCLASSEXA),
		CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS,
		WndProc, 0, 0, hInstance,

		LoadIconA(nullptr, (char*)IDI_APPLICATION),

		LoadCursorA(nullptr, (char*)IDC_CROSS),
	//	LoadCursorA(nullptr, (char*)0x00007F00),//IDC_ARROW

		nullptr,
	//	(HBRUSH__*)(COLOR_WINDOW+1),
		0, "New format", 0
	};
	short success=RegisterClassExA(&wndClassEx);	SYS_ASSERT(success);
	ghWnd=CreateWindowExA(WS_EX_ACCEPTFILES, wndClassEx.lpszClassName, default_title, WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);	SYS_ASSERT(ghWnd);//2020-06-02
	
	HFONT hfDefault=(HFONT)GetStockObject(DEFAULT_GUI_FONT);	GEN_ASSERT(hfDefault);
	
	hFontcombobox	=CreateWindowExW(0, WC_COMBOBOXW, nullptr, WS_CHILD|WS_VISIBLE|WS_VSCROLL | CBS_DROPDOWN|CBS_HASSTRINGS, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, ghWnd, (HMENU)IDM_FONT_COMBOBOX, hInstance, nullptr);SYS_ASSERT(hFontcombobox);
	SendMessageW(hFontcombobox, WM_SETFONT, (WPARAM)hfDefault, 0);	SYS_CHECK();

	hFontsizecb		=CreateWindowExW(0, WC_COMBOBOXW, nullptr, WS_CHILD|WS_VISIBLE|WS_VSCROLL | CBS_DROPDOWN|CBS_HASSTRINGS, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, ghWnd, (HMENU)IDM_FONTSIZE_COMBOBOX, hInstance, nullptr);	SYS_ASSERT(hFontsizecb);
	SendMessageW(hFontsizecb, WM_SETFONT, (WPARAM)hfDefault, 0);	SYS_CHECK();
	FontComboboxProc=(WNDPROC)SetWindowLongPtrA(hFontcombobox	, GWLP_WNDPROC, (long)FontComboboxSubclass);	SYS_ASSERT(FontComboboxProc);
	FontSizeCbProc	=(WNDPROC)SetWindowLongPtrA(hFontsizecb		, GWLP_WNDPROC, (long)FontSizeCbSubclass);		SYS_ASSERT(FontSizeCbProc);

	hTextbox=CreateWindowExW(0, WC_EDITW, nullptr, WS_CHILD|WS_VISIBLE | ES_MULTILINE|ES_WANTRETURN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, ghWnd, (HMENU)IDM_TEXT_EDIT, hInstance, nullptr);	SYS_ASSERT(hTextbox);
	oldEditProc=(WNDPROC)SetWindowLongW(hTextbox, GWLP_WNDPROC, (long)EditProc);	SYS_ASSERT(oldEditProc);

	create_stretchskew();
	hRotatebox	=CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, nullptr, WS_CHILD|WS_VISIBLE | ES_LEFT|ES_WANTRETURN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, ghWnd, (HMENU)IDM_ROTATE_BOX,	hInstance, nullptr);	SYS_ASSERT(hRotatebox);
	
	SendMessageW(hRotatebox		, WM_SETFONT, (WPARAM)hfDefault, 0);	SYS_CHECK();

	RotateBoxProc	=(WNDPROC)SetWindowLongPtrA(hRotatebox	, GWLP_WNDPROC, (long)RotateBoxSubclass);	SYS_ASSERT(RotateBoxProc);


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
		assign_path(programpath, 0, programpath.size(), programpath);
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
		create_maskwindow();
		create_attributeswindow();

		int white_no_alpha=0xFFFFFF;
		memfill(ext_colorpalette, &white_no_alpha, 16<<2, 1<<2);
		//for(int k=0;k<16;++k)
		//	ext_colorpalette[k]=0xFFFFFF;

		if(!iw||!ih)
			iw=400, ih=300;
		image_size=iw*ih;
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
	//	for(int k=0;k<image_size;++k)
	//		image[k]=rand()<<30|rand()<<15|rand();
		//	image[k]=0xFF000000|rand()<<15|rand();
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
		if(font_idx>=(int)fonts.size())
			font_idx=fonts.size();
		SendMessageW(hFontcombobox, CB_SETCURSEL, font_idx, 0);
		//font_idx=SendMessageW(hFontcombobox, CB_SETCURSEL, 0, 0);
		//font_size=20;
		set_sizescombobox(font_idx);

	tagMSG msg;
	int ret=0;
	for(;ret=GetMessageA(&msg, 0, 0, 0);)
	{
		if(ret==-1)
		{
			LOG_ERROR("GetMessage returned -1 with: hwnd=%08X, msg=%s, wP=%d, lP=%d. \nQuitting.", msg.hwnd, wm2str(msg.message), msg.wParam, msg.lParam);
			break;
		}
	//	g_processed=IsDialogMessageA(hWndMask, &msg);
	//	g_processed=IsDialogMessageA(ghWnd, &msg);//blocks ESC, ENTER
	//	if(!g_processed)
	//	{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
	//	}
	}
//	for(;GetMessageA(&msg, 0, 0, 0);)TranslateMessage(&msg), DispatchMessageA(&msg);

	//	free_image(), free_frames();
		write_settings();//save state

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

	return msg.wParam;
}