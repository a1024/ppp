#include		"ppp.h"
#include		"generic.h"
#include		"ppp_inline_check.h"
#include		<ppl.h>
const char		file[]=__FILE__;
void			rectangle(int x1, int x2, int y1, int y2, int color)
{
	if(check(x1, y1)||check(x2-1, y2-1))//
		return;//
	for(int ky=y1;ky<y2;++ky)
		memfill(rgb+w*ky+x1, &color, (x2-x1)<<2, 1<<2);
	//	for(int kx=x1;kx<x2;++kx)
	//		rgb[w*ky+kx]=color;
}
void			rect_checkboard(int x1, int x2, int y1, int y2, int color1, int color2)
{
	if(check(x1, y1)||check(x2-1, y2-1))//
		return;//
	int colors[]={color1, color2, color1};
	for(int ky=y1;ky<y2;++ky)
		memfill(rgb+w*ky+x1, colors+(ky&1), (x2-x1)<<2, 2<<2);
	//	for(int kx=x1;kx<x2;++kx)
	//		rgb[w*ky+kx]=colors[(kx+ky)&1];
}
void			h_line(int x1, int x2, int y, int color)
{
	if(checkx(x1)||checkx(x2-1)||checky(y))//
		return;//
	memfill(rgb+w*y+x1, &color, (x2-x1)<<2, 1<<2);
	//for(int kx=x1;kx<x2;++kx)
	//	rgb[w*y+kx]=color;
}
void			v_line(int x, int y1, int y2, int color)
{
	if(checkx(x)||checky(y1)||checky(y2-1))//
		return;//
	int *row=rgb+w*y1+x;
	for(int ky=y1, y=0;ky<y2;++ky, y+=w)
		row[y]=color;
	//	rgb[w*ky+x]=color;
}
void			h_line_add(int x1, int x2, int y, int ammount)
{
	if(checkx(x1)||checkx(x2-1)||checky(y))//
		return;//
	int *row=rgb+w*y+x1, dx=x2-x1, xround=dx&~3;
	__m128i inc=_mm_set1_epi8(ammount);
	__m128i mask=_mm_set1_epi32(0x00FFFFFF);
	inc=_mm_and_si128(inc, mask);
	int kx=0;
	for(;kx+3<xround;kx+=4)
	{
		__m128i px=_mm_loadu_si128((__m128i*)(row+kx));
		px=_mm_add_epi8(px, inc);
		_mm_storeu_si128((__m128i*)(row+kx), px);
	}
	for(;kx<dx;++kx)
	{
		auto p=(unsigned char*)(row+kx);
		p[0]+=ammount;
		p[1]+=ammount;
		p[2]+=ammount;
	}
	_mm_empty();
	//for(int kx=x1;kx<x2;++kx)
	//{
	//	auto p=(unsigned char*)(rgb+w*y+kx);
	//	p[0]+=ammount;
	//	p[1]+=ammount;
	//	p[2]+=ammount;
	//}
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
	memfill(rgb+w*y+x1, colors, (x2-x1)<<2, 2<<2);
	//for(int kx=x1;kx<x2;++kx)
	//	rgb[w*y+kx]=colors[(kx-x1)&1];
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
void			line45_backslash(int x1, int y1, int n, int color)//top-left (x1, y2) -> bottom-right
{
	if(check(x1, y1)||check(x1+n, y1+n))
		return;
	int *start=rgb+w*y1+x1, limit=n*w, step=w+1;
	for(int k=0;k<limit;k+=step)
		start[k]=color;
	//for(int k=0;k<n;++k)
	//	rgb[w*(y1+k)+x1+k]=color;
}
void			line45_forwardslash(int x1, int y1, int n, int color)//bottom-right (x1, y1) -> top-left
{
	if(check(x1, y1)||check(x1+n, y1-n))
		return;
	int *start=rgb+w*(y1-n)+x1+n, limit=n*w, step=w-1;
	for(int k=0;k<limit;k+=step)
		start[k]=color;
	//for(int k=0;k<n;++k)
	//	rgb[w*(y1-k)+x1+k]=color;
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

void			scrollbar_slider(int &winpos_ip, int imsize_ip, int winsize, int barsize, double zoom, short &sliderstart, short &slidersize)
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
void			scrollbar_scroll(int &winpos_ip, int imsize_ip, int winsize, int barsize, int slider0, int mp_slider0, int mp_slider, double zoom, short &sliderstart, short &slidersize)//_ip: in image pixels, mp_slider: mouse position starting from start of slider
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

void			draw_icon(int *icon, int x, int y)
{
	const __m128i na_pink=_mm_set1_epi32(0x00FF00FF);
	const __m128i ones=_mm_set1_epi32(-1);
	for(int ky=0;ky<16;++ky)
	{
		int *srow=icon+(ky<<4), *dstart=rgb+w*(y+ky)+x;
		for(int kx=0;kx<16;kx+=4)
		{
			__m128i src=_mm_loadu_si128((__m128i*)(srow+kx));
			__m128i dst=_mm_loadu_si128((__m128i*)(dstart+kx));
			__m128i cmask=_mm_cmpeq_epi32(src, na_pink);
			dst=_mm_and_si128(dst, cmask);
			cmask=_mm_xor_si128(cmask, ones);
			src=_mm_and_si128(src, cmask);
			dst=_mm_or_si128(dst, src);
			_mm_storeu_si128((__m128i*)(dstart+kx), dst);
		}
	}
	//for(int ky=0;ky<16;++ky)
	//{
	//	for(int kx=0;kx<16;++kx)
	//	{
	//		int color=icon[ky<<4|kx];
	//		if(color!=0xFF00FF)
	//			rgb[w*(y+ky)+x+kx]=color;
	//	}
	//}
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

void			draw_vscrollbar(int x, int sbwidth, int y1, int y2, int &winpos, int content_h, int vscroll_s0, int vscroll_my_start, double zoom, short &vscroll_start, short &vscroll_size, int dragtype)//vertical scrollbar
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
void			h_line_bounded(int xa, int xb, int y, int color, Rect const &bounds)
{
	if(y>=bounds.i.y&&y<bounds.f.y)
		h_line(maximum(xa, bounds.i.x), minimum(xb, bounds.f.x), y, color);
}
void			v_line_bounded(int x, int ya, int yb, int color, Rect const &bounds)
{
	if(x>=bounds.i.x&&x<bounds.f.x)
		v_line(x, maximum(ya, bounds.i.y), minimum(yb, bounds.f.y), color);
}
void			draw_selection_rectangle(Rect irect, int x1, int x2, int y1, int y2, int padding)
{
	if(irect.nonzero())//selection highlight rectangle
	{
		const int white=0xFFFFFF, black=0x000000;
		irect.sort_coords();
		irect.image2screen();
		Point padpoint(padding, padding);
		irect.i-=padpoint;
		irect.f+=padpoint;
#if 1
		Rect bounds={Point(x1, y1), Point(x2, y2)};
		int xa, xb, ya, yb;
		irect.get(xa, xb, ya, yb);
		if(xb==xa)
			xa-=2, ++xb;
		else
			--xb;
		if(yb==ya)
			ya-=2, ++yb;
		else
			--yb;
		//--xb, --yb;
		//--xa, --ya;
		h_line_bounded(xa+1, xb-1, ya+1, white, bounds);
		h_line_bounded(xa+1, xb  , yb-1, white, bounds);
		v_line_bounded(xa+1, ya+1, yb-1, white, bounds);
		v_line_bounded(xb-1, ya+1, yb-1, white, bounds);
		h_line_bounded(xa, xb  , ya, black, bounds);
		h_line_bounded(xa, xb+1, yb, black, bounds);
		v_line_bounded(xa, ya  , yb, black, bounds);
		v_line_bounded(xb, ya  , yb, black, bounds);
#endif
#if 0
		HRGN hRgn=CreateRectRgn(x1, y1, x2, y2);			GEN_ASSERT(hRgn);
		int ret=(int)SelectObject(ghMemDC, hRgn);			GEN_ASSERT(ret==SIMPLEREGION);
		HBRUSH hBrush=(HBRUSH)GetStockObject(NULL_BRUSH);	GEN_ASSERT(hBrush);
		hBrush=(HBRUSH)SelectObject(ghMemDC, hBrush);		GEN_ASSERT(hBrush);
		Rectangle(ghMemDC, irect.i.x, irect.i.y, irect.f.x, irect.f.y);
		HPEN hPen=(HPEN)GetStockObject(WHITE_PEN);			GEN_ASSERT(hBrush);
		hPen=(HPEN)SelectObject(ghMemDC, hPen);				GEN_ASSERT(hPen);
		int success=Rectangle(ghMemDC, irect.i.x+1, irect.i.y+1, irect.f.x-1, irect.f.y-1);		GEN_ASSERT(success);
		hPen=(HPEN)SelectObject(ghMemDC, hPen);				GEN_ASSERT(hPen);
		hBrush=(HBRUSH)SelectObject(ghMemDC, hBrush);		GEN_ASSERT(hBrush);
		success=SetWindowRgn(ghWnd, nullptr, false);		GEN_ASSERT(success);
		success=DeleteObject(hRgn);							GEN_ASSERT(success);
#endif
#if 0
		//Point p1, p2;
		//selection_sort(istart, iend, p1, p2);
		//image2screen(p1.x, p1.y, s1.x, s1.y);
		//image2screen(p2.x, p2.y, s2.x, s2.y);
		//s1.x-=padding, s1.y-=padding;
		//s2.x+=padding, s2.y+=padding;
		int xstart	=maximum(x1, irect.i.x),	xend	=minimum(irect.f.x+1, x2),
			xstart_w=maximum(x1, irect.i.x+1),	xend_w	=minimum(irect.f.x, x2),
			ystart	=maximum(y1, irect.i.y),	yend	=minimum(irect.f.y+1, y2),
			ystart_w=maximum(y1, irect.i.y+1),	yend_w	=minimum(irect.f.y, y2);
		const int white=0xFFFFFF, black=0x000000;

		//inner white rectangle
		if(s1.y>=y1)//top
			h_line(xstart_w, xend_w, s1.y+1, white);
		if(s2.y<y2)//bottom
			h_line(xstart_w, xend_w, s2.y-1, white);
		if(s1.x>=x1)//left
			v_line(s1.x+1, ystart_w, yend_w, white);
		if(s2.x<x2)//right
			v_line(s2.x-1, ystart_w, yend_w, white);
		
		//outer black rectangle
		if(s1.y>=y1)//top
			h_line(xstart, xend, s1.y, black);
		if(s2.y<y2)//bottom
			h_line(xstart, xend, s2.y, black);
		if(s1.x>=x1)//left
			v_line(s1.x, ystart, yend, black);
		if(s2.x<x2)//right
			v_line(s2.x, ystart, yend, black);
#endif
	}
}

void			movewindow_c(HWND hWnd, int x, int y, int w, int h, bool repaint)//MoveWindow takes client coordinates, for CHILD windows
{
	int success=MoveWindow(hWnd, x, y, w, h, repaint);
	SYS_ASSERT(success);
	//if(!success)
	//	formatted_error(L"MoveWindow", __LINE__);
//	check(__LINE__);
}
//void			movewindow(HWND hWnd, int x, int y, int w, int h, bool repaint)//MoveWindow takes screen coordinates, for POPUP windows
//{
//	POINT position={x, y};
//	ClientToScreen(ghWnd, &position);
//	movewindow_c(hWnd, position.x, position.y, w, h, repaint);
//}

int				colorbarcontents=CS_NONE;//additional options in color bar
void			fliprotate_moveui(bool repaint)
{
	movewindow_c(hRotatebox, 924-17, h-49-3, 50, 21, repaint);
}
void			stretchskew_moveui(bool repaint)
{
	int x1=705, x2=x1+180,
		y1=h-69-1, y2=y1+22,
		dx=50, dy=21;
	movewindow_c(hStretchSkew[SS_STRETCH_H], x1, y1, dx, dy, repaint);
	movewindow_c(hStretchSkew[SS_STRETCH_V], x1, y2, dx, dy, repaint);
	movewindow_c(hStretchSkew[SS_SKEW_H], x2, y1, dx, dy, repaint);
	movewindow_c(hStretchSkew[SS_SKEW_V], x2, y2, dx, dy, repaint);
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
			fliprotate_moveui(true);
			SetWindowTextA(hRotatebox, zero);
		}
		ShowWindow(hRotatebox, ncmdshow);
		break;
	case CS_STRETCHSKEW:
		if(show)
		{
			stretchskew_moveui(true);
			SetWindowTextA(hStretchSkew[SS_STRETCH_H]	, onehundred);
			SetWindowTextA(hStretchSkew[SS_STRETCH_V]	, onehundred);
			SetWindowTextA(hStretchSkew[SS_SKEW_H]		, zero);
			SetWindowTextA(hStretchSkew[SS_SKEW_V]		, zero);
		}
		ShowWindow(hStretchSkew[SS_STRETCH_H]	, ncmdshow);
		ShowWindow(hStretchSkew[SS_STRETCH_V]	, ncmdshow);
		ShowWindow(hStretchSkew[SS_SKEW_H]		, ncmdshow);
		ShowWindow(hStretchSkew[SS_SKEW_V]		, ncmdshow);
		if(show)
		{
			SetFocus(hStretchSkew[SS_STRETCH_H]);
			SendMessageA(hStretchSkew[SS_STRETCH_H], EM_SETSEL, 0, -1);
		}
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

#include		"ppp_inline_blend.h"
void			blend_with_checkboard_zoomed(const int *buffer, int x0, int y0, int x1, int x2, int y1, int y2, short cbx0, short cby0)
{
#ifdef SSSE3_BLEND
	if(check(x1, y1)||check(x2-1, y2-1))
		return;
	int ix=0, iy=0;
	screen2image(x1, y1, ix, iy);
	if(icheck(ix, iy))
		return;
	screen2image(x2, y2, ix, iy);//
	if(ix>0&&iy>0&&icheck(ix-1, iy-1))//
		return;//
	int logcb=clamp(2, 3+logzoom, 7);
	__m128i c_even=_mm_set1_epi32(0xABABAB), c_odd=_mm_set1_epi32(0xFFFFFF);
	int xrange=x2-x1, xround=xrange&~3, xrem=xrange&3, xoffset=x1-x0, yoffset=y1-y0;
	if(logzoom>=0)//1:1 or zoomed in
	{
#ifdef _DEBUG
		for(int ky=y1;ky<y2;++ky)
#else
		Concurrency::parallel_for(y1, y2, [&](int ky)
#endif
		{
			iy=spy+((ky-y0)>>logzoom);
			const int *by=buffer+iw*iy+spx;
			__m128i src, dst;
			int *pixels=rgb+w*ky+x1;
			__m128i ky_bit=checkboard_sse2_y(logcb, ky-y0);
			for(int kx=0;kx<xround;kx+=4)
			{
				src=_mm_set_epi32(by[(kx+3+xoffset)>>logzoom], by[(kx+2+xoffset)>>logzoom], by[(kx+1+xoffset)>>logzoom], by[(kx+xoffset)>>logzoom]);
				//dst=_mm_loadu_si128((__m128i*)pixels);

				//if(src.m128i_i32[0]!=src.m128i_i32[1]||src.m128i_i32[2]!=src.m128i_i32[3])//bouncing ball: hits but shouldn't
				//	int LOL_1=0;//
				dst=checkboard_sse2(logcb, xoffset+kx, ky_bit, c_even, c_odd);
				//dst=_mm_set1_epi32(-1);

				//src=_mm_set_epi32(0xFF000000, 0xC0000000, 0x80000000, 0x40000000);//
				//dst=_mm_set_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);//
				dst=blend_ssse3(src, dst);
				//if(dst.m128i_i32[0]!=dst.m128i_i32[1]||dst.m128i_i32[2]!=dst.m128i_i32[3])//
				//	int LOL_1=0;//

				_mm_storeu_si128((__m128i*)(pixels+kx), dst);
			}
			if(xrem)
			{
				for(int k=0;k<xrem;++k)
					src.m128i_i32[k]=by[(xround+k+xoffset)>>logzoom];
				//for(int k=0;k<xrem;++k)
				//	dst.m128i_i32[k]=pixels[xround+k];
				
				dst=checkboard_sse2(logcb, xoffset+xround, ky_bit, c_even, c_odd);
				dst=blend_ssse3(src, dst);
				
				for(int k=0;k<xrem;++k)
					pixels[xround+k]=dst.m128i_i32[k];
			}
		}
#ifndef _DEBUG
		);
#endif
	}
	else//logzoom<0 (zoomed out)
	{
#ifdef _DEBUG
		for(int ky=y1;ky<y2;++ky)
#else
		Concurrency::parallel_for(y1, y2, [&](int ky)
#endif
		{
			iy=spy+((ky-y0)<<-logzoom);
			const int *by=buffer+iw*iy+spx;
			__m128i src, dst;
			int *pixels=rgb+w*ky+x1;
			__m128i ky_bit=checkboard_sse2_y(logcb, ky-y0);
			for(int kx=0;kx<xround;kx+=4)
			{
				src=_mm_set_epi32(by[(kx+3+xoffset)<<-logzoom], by[(kx+2+xoffset)<<-logzoom], by[(kx+1+xoffset)<<-logzoom], by[(kx+xoffset)<<-logzoom]);
				//dst=_mm_loadu_si128((__m128i*)pixels);
				
				dst=checkboard_sse2(logcb, xoffset+kx, ky_bit, c_even, c_odd);
				dst=blend_ssse3(src, dst);

				_mm_storeu_si128((__m128i*)(pixels+kx), dst);
			}
			if(xround<xrange)
			{
				for(int k=0;k<xrem;++k)
					src.m128i_i32[k]=by[(xround+k+xoffset)<<-logzoom];
				//for(int k=0;k<xrem;++k)
				//	dst.m128i_i32[k]=pixels[xround+k];
				
				dst=checkboard_sse2(logcb, xoffset+xround, ky_bit, c_even, c_odd);
				dst=blend_ssse3(src, dst);
				
				for(int k=0;k<xrem;++k)
					pixels[xround+k]=dst.m128i_i32[k];
			}
		}
#ifndef _DEBUG
		);
#endif
	}
	_m_empty();
#else
//	for(int ky=y1;ky<y2;++ky)
	Concurrency::parallel_for(y1, y2, [&](int ky)//blend 12ms, total 19ms, 51fps
	{
		for(int kx=x1;kx<x2;++kx)
		{
			int ix=spx+shift(kx-x1, -logzoom), iy=spy+shift(ky-y1, -logzoom);
			if(icheck(ix, iy))//robust
				break;//
			auto dst=(unsigned char*)(rgb+w*ky+kx), src=(unsigned char*)(buffer+iw*iy+ix);
			auto &alpha=src[3];
			dst[0]=dst[0]+((src[0]-dst[0])*alpha/255);//...>>8 leaves artifacts
			dst[1]=dst[1]+((src[1]-dst[1])*alpha/255);
			dst[2]=dst[2]+((src[2]-dst[2])*alpha/255);
		}
	}
	);
#endif
}

void			stamp_mask(const int *mask, char mw, char mh, char upsidedown, char transposed, short x1, short x2, short y1, short y2, int color)
{
//	std::vector<LOL_1> vec;
	//int ud=-upsidedown, tr=-transposed;
	int my=0, myc=y2-1-y1, mx=0, mxc=x2-1-x1;
	int *i_idx, *b_idx;
	if(upsidedown)
	{
		if(transposed)
			i_idx=&mxc, b_idx=&my;
		else
			i_idx=&myc, b_idx=&mx;
	}
	else
	{
		if(transposed)
			i_idx=&mx, b_idx=&my;
		else
			i_idx=&my, b_idx=&mx;
	}
	for(int ky=y1;ky<y2;++ky, ++my, --myc)
	{
		mx=0, mxc=x2-1-x1;
		for(int kx=x1;kx<x2;++kx, ++mx, --mxc)
		{
			int bit;
			if(mask)
				bit=-(mask[*i_idx]>>*b_idx&1);
			else
				bit=0xFFFFFFFF;
			auto &pixel=rgb[w*ky+kx];
			pixel=color&bit|pixel&~bit;

			//LOL_1 s={*i_idx, *b_idx};//
			//vec.push_back(s);//
		}
	}
//	vec.clear();
}
void			v_line_comp_dots(int x, int y1, int y2, Point const &cTL, Point const &cBR)
{
	if(x<cTL.x||x>=cBR.x)
		return;
	int ya, yb;
	if(y1<y2)
		ya=y1, yb=y2;
	else
		ya=y2, yb=y1;
	if(ya<cTL.y)
		ya=cTL.y;
	if(yb>=cBR.y)
		yb=cBR.y-1;
	++yb;
	int step=w<<1;
	int start=w*ya+x+w, end=w*yb+x;
	//int start=w*ya+x, end=w*yb+x;//X not seen with grid on
	for(int idx=start;idx<end;idx+=step)
	{
		auto &pixel=rgb[idx];
		pixel=0xFF000000&pixel|0x00FFFFFF&~pixel;
	}
}
void			h_line_comp_dots(int x1, int x2, int y, Point const &cTL, Point const &cBR)
{
	if(y<cTL.y||y>=cBR.y)
		return;
	int xa, xb;
	if(x1<x2)
		xa=x1, xb=x2;
	else
		xa=x2, xb=x1;
	if(xa<cTL.x)
		xa=cTL.x;
	if(xb>=cBR.x)
		xb=cBR.x-1;
	++xb;
	int xround=xb-xa, xrem=xround&3;
	xround&=~3;
	int *start=rgb+w*y+xa;
	const __m128i mask=_mm_set_epi32(0x00FFFFFF, 0, 0x00FFFFFF, 0);
	//const __m128i mask=_mm_set_epi32(0, 0x00FFFFFF, 0, 0x00FFFFFF);//X not seen with grid on
	__m128i pixels;
	for(int kx=0;kx<xround;kx+=4)
	{
		pixels=_mm_loadu_si128((__m128i*)(start+kx));
		pixels=_mm_xor_si128(pixels, mask);
		_mm_storeu_si128((__m128i*)(start+kx), pixels);
	}
	if(xrem)
	{
		for(int kx=0;kx<xrem;++kx)
			pixels.m128i_i32[kx]=start[xround+kx];
		pixels=_mm_xor_si128(pixels, mask);
		for(int kx=0;kx<xrem;++kx)
			start[xround+kx]=pixels.m128i_i32[kx];
	}
}

//const __m128i m_four=_mm_set1_epi32(4);
const __m128i m_lomask=_mm_set_epi32(0, -1, 0, -1);
//const __m128i m_himask=_mm_set_epi32(-1, 0, -1, 0);
__forceinline __m128i mullo_epi32(__m128i const &a, __m128i const &b)//<15 cycles
{
	__m128i t1=_mm_mul_epu32(a, b);//5 cycles
	__m128i a2=_mm_shuffle_epi32(a, _MM_SHUFFLE(2, 3, 0, 1));
	__m128i t2=_mm_shuffle_epi32(b, _MM_SHUFFLE(2, 3, 0, 1));
	t2=_mm_mul_epu32(a2, t2);
	t1=_mm_and_si128(t1, m_lomask);
	t2=_mm_and_si128(t2, m_lomask);
	t2=_mm_shuffle_epi32(t2, _MM_SHUFFLE(2, 3, 0, 1));
	t2=_mm_or_si128(t2, t1);
	return t2;
}
__forceinline __m128i clamp0h(__m128i const &x, __m128i const &hi)
{
	__m128i t1=_mm_cmpgt_epi32(x, _mm_setzero_si128());
	t1=_mm_and_si128(t1, x);
	__m128i t2=_mm_cmplt_epi32(t1, hi);
	__m128i x2=_mm_and_si128(t2, t1);
	t2=_mm_xor_si128(t2, m_onesmask);
	t2=_mm_and_si128(t2, hi);
	x2=_mm_or_si128(x2, t2);
	return x2;
}
#define			PPP_GUI_CPP
void			stretch_blit(const int *buffer, int bw, int bh,  Rect const &irect,  int sx0, int sy0,  int sx1, int sx2, int sy1, int sy2,  int *mask, int transparent, int bkcolor)//mask: pass nullptr for rectangular selection
{//i0: start on image, id: extent in image, s0: imagewindow start on screen, s1 & s2: draw start & end on screen
	int ix0=irect.i.x, idx=irect.dx(),
		iy0=irect.i.y, idy=irect.dy();
	if(!idx||!idy)
		return;
	//clamp screen coordinates
	int sxa, sya, sxb, syb;
	image2screen(ix0, iy0, sxa, sya);
	image2screen(ix0+idx, iy0+idy, sxb, syb);
	if(sxa>sxb)
		std::swap(sxa, sxb);
	if(sya>syb)
		std::swap(sya, syb);
	sx1=clamp(sxa, sx1, sxb);
	sx2=clamp(sxa, sx2, sxb);
	sy1=clamp(sya, sy1, syb);
	sy2=clamp(sya, sy2, syb);
	int xrange=abs(sx2-sx1), yrange=abs(sy2-sy1);
	int xstep=(sx1<sx2)-(sx1>sx2), ystep=(sy1<sy2)-(sy1>sy2);

	if(interpolation_type==I_BILINEAR)
	{
#if 1//grid
		int xnum=bw, xden=idx, abs_xden=abs(xden), xroundoffset=(abs_xden>>1)&-(abs_xden!=bw),
			ynum=bh, yden=idy, abs_yden=abs(yden), yroundoffset=(abs_yden>>1)&-(abs_yden!=bh);
		int gridcolors[]={0xFF8000, 0xFFC000};
		//int gridcolors[]={0x808080, 0xC0C0C0};
		int grid=-(showgrid&&abs_xden/bw>2&&abs_yden/bh>2);
		//int grid=-(logzoom>1&&abs_xden/bw>2&&abs_yden/bh>2);
		int axden2=abs_xden<<1, ayden2=abs_yden<<1;
		//int prevxrem=0, prevyrem=0;
		int gx=0, gx_prev=0,
			gy=0, gy_prev=0;
		for(int ky=sy1, ycount=0;ycount<yrange;++ycount, ky+=ystep)
		{
			gy=(spy-iy0+shift(ky-sy0, -logzoom))*ynum;
			int by=gy-yroundoffset;
			int by0=by;
			int byrem=by%yden;
			by/=yden;
			by-=by0<0;
			byrem+=abs_yden&-((byrem<0)|!byrem&(yden<0));
			//byrem+=abs_yden&-(byrem<0);
			int byrem_c=abs_yden-byrem;
			if(idy<0)
				std::swap(byrem, byrem_c);
			//by=clamp(-1, by, bh-1);

			gy/=yden;
			int ygrid=gy!=gy_prev;
			gy_prev=gy;
			//int ygrid=ycount*bh/abs_yden!=(ycount+1)*bh/abs_yden;
			//int ygrid=-(yerror>ayden2);
			//yerror-=ayden2&ygrid;

			const int *srcrow0=buffer+bw*by, *srcrow1=srcrow0+bw;
			const int *maskrow0=mask+bw*by, *maskrow1=maskrow0+bw;
			int *dstrow=rgb+w*ky;
			for(int kx=sx1, xcount=0;xcount<xrange;++xcount, kx+=xstep)
			{
				gx=(spx-ix0+shift(kx-sx0, -logzoom))*xnum;
				int bx=gx-xroundoffset;
				int bx0=bx;
				int bxrem=bx%xden;
				bx/=xden;
				bx-=bx0<0;
				bxrem+=abs_xden&-((bxrem<0)|!bxrem&(xden<0));
				//bxrem+=abs_xden&-(bxrem<0);
				int bxrem_c=abs_xden-bxrem;
				if(idx<0)
					std::swap(bxrem, bxrem_c);
				//bx=clamp(-1, bx, bw-1);
				
				gx/=xden;
				int xgrid=gx!=gx_prev;
				gx_prev=gx;
				//int xgrid=xcount*bw/abs_xden!=(xcount+1)*bw/abs_xden;
				//int xgrid=-(xerror>axden2);
				//xerror-=axden2&xgrid;
				if(grid&(ygrid|xgrid))
				//if(grid&(abs(byrem-prevyrem)>halfy||abs(bxrem-prevxrem)>halfx))//grid
				//if(grid&(!byrem|!bxrem))
				{
					dstrow[kx]=gridcolors[(kx^ky)&1];
					continue;
				}
				
				byte *dst=(byte*)(dstrow+kx);
				const byte *src00, *src01, *src10, *src11;
				if(selection_free)
				{
					// yx		ENTIRE ROW: Y IS CONSTANT
					src00=((unsigned)by  <(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?  maskrow0[bx  ]?(const byte*)(srcrow0+bx  ):dst  :(const byte*)&secondarycolor;
					src01=((unsigned)by  <(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?  maskrow0[bx+1]?(const byte*)(srcrow0+bx+1):dst  :(const byte*)&secondarycolor;
					src10=((unsigned)by+1<(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?  maskrow1[bx  ]?(const byte*)(srcrow1+bx  ):dst  :(const byte*)&secondarycolor;
					src11=((unsigned)by+1<(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?  maskrow1[bx+1]?(const byte*)(srcrow1+bx+1):dst  :(const byte*)&secondarycolor;
				}
				else
				{
					src00=((unsigned)by  <(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?(const byte*)(srcrow0+bx  ):(const byte*)&secondarycolor;
					src01=((unsigned)by  <(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?(const byte*)(srcrow0+bx+1):(const byte*)&secondarycolor;
					src10=((unsigned)by+1<(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?(const byte*)(srcrow1+bx  ):(const byte*)&secondarycolor;
					src11=((unsigned)by+1<(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?(const byte*)(srcrow1+bx+1):(const byte*)&secondarycolor;
				}
				if(transparent)
				{
					if(*(int*)src00==secondarycolor)
						src00=dst;
					if(*(int*)src01==secondarycolor)
						src01=dst;
					if(*(int*)src10==secondarycolor)
						src10=dst;
					if(*(int*)src11==secondarycolor)
						src11=dst;
				}
				for(int kc=0;kc<4;++kc)
				{
					int
						src0x=src00[kc]+(src01[kc]-src00[kc])*bxrem/abs_xden,
						src1x=src10[kc]+(src11[kc]-src10[kc])*bxrem/abs_xden;
					dst[kc]=src0x+(src1x-src0x)*byrem/abs_yden;
				}
			}
		}
#endif
#if 0//grid is on center of pixel
		int xnum=bw, xden=idx, abs_xden=abs(xden), xroundoffset=(abs_xden>>1)&-(abs_xden!=bw),
			ynum=bh, yden=idy, abs_yden=abs(yden), yroundoffset=(abs_yden>>1)&-(abs_yden!=bh);
		int gridcolors[]={0xFF8000, 0xFFC000};
		//int gridcolors[]={0x808080, 0xC0C0C0};
		int grid=-(showgrid&&abs_xden/bw>2&&abs_yden/bh>2);
		//int grid=-(logzoom>1&&abs_xden/bw>2&&abs_yden/bh>2);
		int axden2=abs_xden<<1, ayden2=abs_yden<<1;
		//int prevxrem=0, prevyrem=0;
		for(int ky=sy1, ycount=0, by_prev=0;ycount<yrange;++ycount, ky+=ystep)
		{
			int by=(spy-iy0+shift(ky-sy0, -logzoom))*ynum-yroundoffset;
			int by0=by;
			int byrem=by%yden;
			by/=yden;
			by-=by0<0;
			byrem+=abs_yden&-((byrem<0)|!byrem&(yden<0));
			//byrem+=abs_yden&-(byrem<0);
			int byrem_c=abs_yden-byrem;
			if(idy<0)
				std::swap(byrem, byrem_c);
			//by=clamp(-1, by, bh-1);

			int ygrid=by!=by_prev;
			//int ygrid=-(yerror>ayden2);
			//yerror-=ayden2&ygrid;
			by_prev=by;

			const int *srcrow0=buffer+bw*by, *srcrow1=srcrow0+bw;
			const int *maskrow0=mask+bw*by, *maskrow1=maskrow0+bw;
			int *dstrow=rgb+w*ky;
			for(int kx=sx1, xcount=0, bx_prev=0;xcount<xrange;++xcount, kx+=xstep)
			{
				int bx=(spx-ix0+shift(kx-sx0, -logzoom))*xnum-xroundoffset;
				int bx0=bx;
				int bxrem=bx%xden;
				bx/=xden;
				bx-=bx0<0;
				bxrem+=abs_xden&-((bxrem<0)|!bxrem&(xden<0));
				//bxrem+=abs_xden&-(bxrem<0);
				int bxrem_c=abs_xden-bxrem;
				if(idx<0)
					std::swap(bxrem, bxrem_c);
				//bx=clamp(-1, bx, bw-1);
				
				int xgrid=bx!=bx_prev;
				bx_prev=bx;
				//int xgrid=-(xerror>axden2);
				//xerror-=axden2&xgrid;
				if(grid&(ygrid|xgrid))
				//if(grid&(abs(byrem-prevyrem)>halfy||abs(bxrem-prevxrem)>halfx))//grid
				//if(grid&(!byrem|!bxrem))
				{
					dstrow[kx]=gridcolors[(kx^ky)&1];
					continue;
				}

				const byte *src00, *src01, *src10, *src11;
				byte *dst=(byte*)(dstrow+kx);
				if(selection_free)
				{
					// yx		ENTIRE ROW: Y IS CONSTANT
					src00=((unsigned)by  <(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?  maskrow0[bx  ]?(const byte*)(srcrow0+bx  ):dst  :(const byte*)&secondarycolor;
					src01=((unsigned)by  <(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?  maskrow0[bx+1]?(const byte*)(srcrow0+bx+1):dst  :(const byte*)&secondarycolor;
					src10=((unsigned)by+1<(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?  maskrow1[bx  ]?(const byte*)(srcrow1+bx  ):dst  :(const byte*)&secondarycolor;
					src11=((unsigned)by+1<(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?  maskrow1[bx+1]?(const byte*)(srcrow1+bx+1):dst  :(const byte*)&secondarycolor;
				}
				else
				{
					src00=((unsigned)by  <(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?(const byte*)(srcrow0+bx  ):(const byte*)&secondarycolor;
					src01=((unsigned)by  <(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?(const byte*)(srcrow0+bx+1):(const byte*)&secondarycolor;
					src10=((unsigned)by+1<(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?(const byte*)(srcrow1+bx  ):(const byte*)&secondarycolor;
					src11=((unsigned)by+1<(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?(const byte*)(srcrow1+bx+1):(const byte*)&secondarycolor;
				}
				if(transparent)
				{
					if(*(int*)src00==secondarycolor)
						src00=dst;
					if(*(int*)src01==secondarycolor)
						src01=dst;
					if(*(int*)src10==secondarycolor)
						src10=dst;
					if(*(int*)src11==secondarycolor)
						src11=dst;
				}
				for(int kc=0;kc<4;++kc)
				{
					int
						src0x=src00[kc]+(src01[kc]-src00[kc])*bxrem/abs_xden,
						src1x=src10[kc]+(src11[kc]-src10[kc])*bxrem/abs_xden;
					dst[kc]=src0x+(src1x-src0x)*byrem/abs_yden;
				}
			}
		}
#endif
#if 0//grid (broken) & mask
		int xnum=bw, xden=idx, abs_xden=abs(xden), xroundoffset=(abs_xden>>1)&-(abs_xden!=bw),
			ynum=bh, yden=idy, abs_yden=abs(yden), yroundoffset=(abs_yden>>1)&-(abs_yden!=bh);
		int gridcolors[]={0xFF8000, 0xFFC000};
		//int gridcolors[]={0x808080, 0xC0C0C0};
		int grid=-(showgrid&&abs_xden/bw>2&&abs_yden/bh>2);
		//int grid=-(logzoom>1&&abs_xden/bw>2&&abs_yden/bh>2);
		int axden2=abs_xden<<1, ayden2=abs_yden<<1;
		//int prevxrem=0, prevyrem=0;
		for(int ky=sy1, ycount=0, yerror=0;ycount<yrange;++ycount, ky+=ystep, yerror+=bh)
		{
			int by=(spy-iy0+shift(ky-sy0, -logzoom))*ynum-yroundoffset;
			int by0=by;
			int byrem=by%yden;
			by/=yden;
			by-=by0<0;
			byrem+=abs_yden&-(byrem<0);
			int byrem_c=abs_yden-byrem;
			if(idy<0)
				std::swap(byrem, byrem_c);
			//by=clamp(-1, by, bh-1);

			int ygrid=-(yerror>ayden2);
			yerror-=ayden2&ygrid;

			const int *srcrow0=buffer+bw*by, *srcrow1=srcrow0+bw;
			const int *maskrow0=mask+bw*by, *maskrow1=maskrow0+bw;
			int *dstrow=rgb+w*ky;
			for(int kx=sx1, xcount=0, xerror=0;xcount<xrange;++xcount, kx+=xstep, xerror+=bw)
			{
				int bx=(spx-ix0+shift(kx-sx0, -logzoom))*xnum-xroundoffset;
				int bx0=bx;
				int bxrem=bx%xden;
				bx/=xden;
				bx-=bx0<0;
				bxrem+=abs_xden&-(bxrem<0);
				int bxrem_c=abs_xden-bxrem;
				if(idx<0)
					std::swap(bxrem, bxrem_c);
				//bx=clamp(-1, bx, bw-1);

				int xgrid=-(xerror>axden2);
				xerror-=axden2&xgrid;
				if(grid&(ygrid|xgrid))
				//if(grid&(abs(byrem-prevyrem)>halfy||abs(bxrem-prevxrem)>halfx))//grid
				//if(grid&(!byrem|!bxrem))
				{
					dstrow[kx]=gridcolors[(kx^ky)&1];
					continue;
				}

				const byte *src00, *src01, *src10, *src11;
				byte *dst=(byte*)(dstrow+kx);
				if(selection_free)
				{
					// yx		ENTIRE ROW: Y IS CONSTANT
					src00=((unsigned)by  <(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?  maskrow0[bx  ]?(const byte*)(srcrow0+bx  ):dst  :(const byte*)&secondarycolor;
					src01=((unsigned)by  <(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?  maskrow0[bx+1]?(const byte*)(srcrow0+bx+1):dst  :(const byte*)&secondarycolor;
					src10=((unsigned)by+1<(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?  maskrow1[bx  ]?(const byte*)(srcrow1+bx  ):dst  :(const byte*)&secondarycolor;
					src11=((unsigned)by+1<(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?  maskrow1[bx+1]?(const byte*)(srcrow1+bx+1):dst  :(const byte*)&secondarycolor;
				}
				else
				{
					src00=((unsigned)by  <(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?(const byte*)(srcrow0+bx  ):(const byte*)&secondarycolor;
					src01=((unsigned)by  <(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?(const byte*)(srcrow0+bx+1):(const byte*)&secondarycolor;
					src10=((unsigned)by+1<(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?(const byte*)(srcrow1+bx  ):(const byte*)&secondarycolor;
					src11=((unsigned)by+1<(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?(const byte*)(srcrow1+bx+1):(const byte*)&secondarycolor;
				}
				if(transparent)
				{
					if(*(int*)src00==secondarycolor)
						src00=dst;
					if(*(int*)src01==secondarycolor)
						src01=dst;
					if(*(int*)src10==secondarycolor)
						src10=dst;
					if(*(int*)src11==secondarycolor)
						src11=dst;
				}
				for(int kc=0;kc<4;++kc)
				{
					int
						src0x=src00[kc]+(src01[kc]-src00[kc])*bxrem/abs_xden,
						src1x=src10[kc]+(src11[kc]-src10[kc])*bxrem/abs_xden;
					dst[kc]=src0x+(src1x-src0x)*byrem/abs_yden;
				}
			}
		}
#endif
#if 0//transparency
		int xnum=bw, xden=idx, abs_xden=abs(xden), halfx=abs_xden>>1,
			ynum=bh, yden=idy, abs_yden=abs(yden), halfy=abs_yden>>1;
		for(int ky=sy1, ycount=0;ycount<yrange;++ycount, ky+=ystep)
		{
			int by=(spy-iy0+shift(ky-sy0, -logzoom))*ynum-halfy;
			int byrem=by%yden;
			by/=yden;
			byrem+=abs_yden&-(byrem<0);
			int byrem_c=abs_yden-byrem;
			if(idy<0)
				std::swap(byrem, byrem_c);
			by=clamp(0, by, bh-1);
			const int *srcrow0=buffer+bw*by, *srcrow1=srcrow0+bw;
			int *dstrow=rgb+w*ky;
			for(int kx=sx1, xcount=0;xcount<xrange;++xcount, kx+=xstep)
			{
				int bx=(spx-ix0+shift(kx-sx0, -logzoom))*xnum-halfx;
				int bxrem=bx%xden;
				bx/=xden;
				bxrem+=abs_xden&-(bxrem<0);
				int bxrem_c=abs_xden-bxrem;
				if(idx<0)
					std::swap(bxrem, bxrem_c);
				bx=clamp(0, bx, bw-1);
				//const byte
				//	*src00=(const byte*)(srcrow0+bx),
				//	*src01=(const byte*)(srcrow0+bx2),
				//	*src10=(const byte*)(srcrow1+bx),
				//	*src11=(const byte*)(srcrow1+bx2);
				const byte//	yx		ENTIRE ROW: Y IS CONSTANT
					*src00=((unsigned)by  <(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?(const byte*)(srcrow0+bx  ):(const byte*)&secondarycolor,
					*src01=((unsigned)by  <(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?(const byte*)(srcrow0+bx+1):(const byte*)&secondarycolor,
					*src10=((unsigned)by+1<(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?(const byte*)(srcrow1+bx  ):(const byte*)&secondarycolor,
					*src11=((unsigned)by+1<(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?(const byte*)(srcrow1+bx+1):(const byte*)&secondarycolor;
				byte *dst=(byte*)(dstrow+kx);
				if(transparent)
				{
					if(*(int*)src00==secondarycolor)
						src00=dst;
					if(*(int*)src01==secondarycolor)
						src01=dst;
					if(*(int*)src10==secondarycolor)
						src10=dst;
					if(*(int*)src11==secondarycolor)
						src11=dst;
				}
				for(int kc=0;kc<4;++kc)
				{
					int
						src0x=src00[kc]+(src01[kc]-src00[kc])*bxrem/abs_xden,
						src1x=src10[kc]+(src11[kc]-src10[kc])*bxrem/abs_xden;
					dst[kc]=src0x+(src1x-src0x)*byrem/abs_yden;
				}
			}
		}
#endif
#if 0//doesn't work
		const int experimental_count=8;
		static int experimental=0;
		experimental=(experimental+1)%experimental_count;
	//	typedef long long INT;
		typedef int INT;
		//int abs_idx=abs(idx), abs_idy=abs(idy);
		//int xoffset=-(abs_idx>>1)&-(abs_idx!=bw), yoffset=-(abs_idy>>1)&-(abs_idy!=bh);
		int xoffset=0, yoffset=0;
		INT xnum=bw, xden=idx, abs_xden=abs(xden), halfx=abs_xden>>1,
			ynum=bh, yden=idy, abs_yden=abs(yden), halfy=abs_yden>>1;
		//INT xnum=bw*bw, xden=idx*(bw+1), abs_xden=abs(xden),
		//	ynum=bh*bh, yden=idy*(bh+1), abs_yden=abs(yden);
		//	ynum=bh, yden=idy;
		//int xden=idx+(idx>0)-(idx<0),
		//	yden=idy+(idy>0)-(idy<0);
		for(int ky=sy1, ycount=0;ycount<yrange;++ycount, ky+=ystep)
		{
			//int by=(spy-iy0+shift(ky-sy0, -logzoom))*(bh+1)+yoffset;
			INT by=(spy-iy0+shift(ky-sy0, -logzoom))*ynum-halfy;
			INT byrem=by%yden;
			by/=yden;
			//by/=idy;
			byrem+=abs_yden&-(byrem<0);
			INT byrem_c=abs_yden-byrem;
			if(idy<0)
				std::swap(byrem, byrem_c);
			int by2=clamp(0, by+1, bh-1);
			by=clamp(0, by, bh-1);
			const int *srcrow0=buffer+bw*by, *srcrow1=srcrow0+bw;
			int *dstrow=rgb+w*ky;
			for(int kx=sx1, xcount=0;xcount<xrange;++xcount, kx+=xstep)
			{
				//int bx=(spx-ix0+shift(kx-sx0, -logzoom))*(bw+1)+xoffset;
				INT bx=(spx-ix0+shift(kx-sx0, -logzoom))*xnum-halfx;
				INT bxrem=bx%xden;
				bx/=xden;
				//bx/=idx;
				bxrem+=abs_xden&-(bxrem<0);
				INT bxrem_c=abs_xden-bxrem;
				if(idx<0)
					std::swap(bxrem, bxrem_c);
				int bx2=clamp(0, bx+1, bw-1);
				bx=clamp(0, bx, bw-1);
				const byte
					*src00=(const byte*)(srcrow0+bx),
					*src01=(const byte*)(srcrow0+bx2),
					*src10=(const byte*)(srcrow1+bx),
					*src11=(const byte*)(srcrow1+bx2);
				//const byte//	yx		ENTIRE ROW: Y IS CONSTANT
				//	*src00=((unsigned)by  <(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?(const byte*)(srcrow0+bx  ):(const byte*)&secondarycolor,
				//	*src01=((unsigned)by  <(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?(const byte*)(srcrow0+bx+1):(const byte*)&secondarycolor,
				//	*src10=((unsigned)by+1<(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?(const byte*)(srcrow1+bx  ):(const byte*)&secondarycolor,
				//	*src11=((unsigned)by+1<(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?(const byte*)(srcrow1+bx+1):(const byte*)&secondarycolor;
				byte *dst=(byte*)(dstrow+kx);
				for(int kc=0;kc<4;++kc)
				{
					int
						src0x=src00[kc]+(src01[kc]-src00[kc])*bxrem/abs_xden,
						src1x=src10[kc]+(src11[kc]-src10[kc])*bxrem/abs_xden;
					dst[kc]=src0x+(src1x-src0x)*byrem/abs_yden;
				}
			}
		}
#endif
#if 0//initial reference
		int xnum=bw, xden=idx, abs_xden=abs(xden), halfx=abs_xden>>1,
			ynum=bh, yden=idy, abs_yden=abs(yden), halfy=abs_yden>>1;
		for(int ky=sy1, ycount=0;ycount<yrange;++ycount, ky+=ystep)
		{
			int by=(spy-iy0+shift(ky-sy0, -logzoom))*ynum;
			int byrem=by%yden;
			by/=yden;
			byrem+=abs_yden&-(byrem<0);
			int byrem_c=abs_yden-byrem;
			if(idy<0)
				std::swap(byrem, byrem_c);
			//int by2=clamp(0, by+1, bh-1);
			by=clamp(0, by, bh-1);
			const int *srcrow0=buffer+bw*by, *srcrow1=srcrow0+bw;
			int *dstrow=rgb+w*ky;
			for(int kx=sx1, xcount=0;xcount<xrange;++xcount, kx+=xstep)
			{
				int bx=(spx-ix0+shift(kx-sx0, -logzoom))*xnum;
				int bxrem=bx%xden;
				bx/=xden;
				//bx/=idx;
				bxrem+=abs_xden&-(bxrem<0);
				int bxrem_c=abs_xden-bxrem;
				if(idx<0)
					std::swap(bxrem, bxrem_c);
				//int bx2=clamp(0, bx+1, bw-1);
				bx=clamp(0, bx, bw-1);
				//const byte
				//	*src00=(const byte*)(srcrow0+bx),
				//	*src01=(const byte*)(srcrow0+bx2),
				//	*src10=(const byte*)(srcrow1+bx),
				//	*src11=(const byte*)(srcrow1+bx2);
				const byte//	yx		ENTIRE ROW: Y IS CONSTANT
					*src00=((unsigned)by  <(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?(const byte*)(srcrow0+bx  ):(const byte*)&secondarycolor,
					*src01=((unsigned)by  <(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?(const byte*)(srcrow0+bx+1):(const byte*)&secondarycolor,
					*src10=((unsigned)by+1<(unsigned)bh)&((unsigned)bx  <(unsigned)bw)?(const byte*)(srcrow1+bx  ):(const byte*)&secondarycolor,
					*src11=((unsigned)by+1<(unsigned)bh)&((unsigned)bx+1<(unsigned)bw)?(const byte*)(srcrow1+bx+1):(const byte*)&secondarycolor;
				byte *dst=(byte*)(dstrow+kx);
				for(int kc=0;kc<4;++kc)
				{
					int
						src0x=src00[kc]+(src01[kc]-src00[kc])*bxrem/abs_xden,
						src1x=src10[kc]+(src11[kc]-src10[kc])*bxrem/abs_xden;
					dst[kc]=src0x+(src1x-src0x)*byrem/abs_yden;
				}
				//if(src!=bkcolor)
				//	rgb[w*ky+kx]=src;
			}
		}
#endif
		return;
	}

	__m128i m_bkcolor=_mm_set1_epi32(bkcolor);
	__m128i m_sy0=_mm_set1_epi32(sy0), m_ydiff=_mm_set1_epi32(spy-iy0);
	__m128i m_sx0=_mm_set1_epi32(sx0), m_xdiff=_mm_set1_epi32(spx-ix0);
	int rx=bw*(1<<16)/idx;
	__m128i m_rx=_mm_set1_epi32(rx);
	__m128i m_bwm1=_mm_set1_epi32(bw-1);
	int sr=16+logzoom,
		zoommask=~(shift(1, logzoom)-1);//the 'logzoom' least significant bits are cleared
	__m128i m_zoommask=_mm_set1_epi32(zoommask);
	int xstep4=xstep<<2;
	__m128i m_xstep=_mm_set1_epi32(xstep4);
	int xround=xrange&~3, xrem=xrange&3;
	if(selection_free&&mask)//free selection
	{
		if(transparent)
		{
#if 1
			for(int ky=sy1;ky<sy2;++ky)
			{
				int by=(spy-iy0+shift(ky-sy0, -logzoom))*bh/idy;
				by=clamp(0, by, bh-1);
				const int *srow=buffer+bw*by, *mrow=mask+bw*by;
				int ksrc=sx1-sx0+shift(spx-ix0, logzoom);
				int kdst=w*ky+sx1;
				__m128i m_ksrc=_mm_set_epi32(ksrc+3*xstep, ksrc+2*xstep, ksrc+xstep, ksrc);
				__m128i bx, src, dst, eq, m_mask;
				if(logzoom>=0)
				{
					for(int kx=0;kx<xround;kx+=4)
					{
						bx=_mm_srli_epi32(m_ksrc, logzoom);//shift right
#include				"ppp_blit_body_free_trans.h"
					}
				}
				else
				{
					for(int kx=0;kx<xround;kx+=4)
					{
						bx=_mm_slli_epi32(m_ksrc, -logzoom);//shift left
#include				"ppp_blit_body_free_trans.h"
					}
				}
				if(xrem)
				{
					if(logzoom>=0)
						bx=_mm_srli_epi32(m_ksrc, logzoom);
					else
						bx=_mm_slli_epi32(m_ksrc, -logzoom);
					bx=mullo_epi32(bx, m_rx);
					bx=_mm_srli_epi32(bx, 16);
					bx=clamp0h(bx, m_bwm1);

					for(int k=0;k<xrem;++k)
					{
						src.m128i_i32[k]=srow[bx.m128i_i32[k]];
						m_mask.m128i_i32[k]=mask[bx.m128i_i32[k]];
						dst.m128i_i32[k]=rgb[kdst+k];
					}
					m_mask=_mm_cmpeq_epi32(m_mask, _mm_setzero_si128());//preparing the mask

					eq=_mm_cmpeq_epi32(src, m_bkcolor);
					eq=_mm_or_si128(eq, m_mask);
					dst=_mm_and_si128(dst, eq);

					eq=_mm_xor_si128(eq, m_onesmask);
					eq=_mm_and_si128(eq, src);

					dst=_mm_or_si128(dst, eq);
					for(int k=0;k<xrem;++k)
						rgb[kdst+k]=dst.m128i_i32[k];
				}
			}
#endif
		}
		else//free opaque
		{
#if 1
			for(int ky=sy1;ky<sy2;++ky)
			{
				int by=(spy-iy0+shift(ky-sy0, -logzoom))*bh/idy;
				by=clamp(0, by, bh-1);
				const int *srow=buffer+bw*by, *mrow=mask+bw*by;
				int ksrc=sx1-sx0+shift(spx-ix0, logzoom);
				int kdst=w*ky+sx1;
				__m128i m_ksrc=_mm_set_epi32(ksrc+3*xstep, ksrc+2*xstep, ksrc+xstep, ksrc);
				__m128i bx, src, dst, eq, m_mask;
				if(logzoom>=0)
				{
					for(int kx=0;kx<xround;kx+=4)
					{
						bx=_mm_srli_epi32(m_ksrc, logzoom);//shift right
#include				"ppp_blit_body_free_opaq.h"
					}
				}
				else
				{
					for(int kx=0;kx<xround;kx+=4)
					{
						bx=_mm_slli_epi32(m_ksrc, -logzoom);//shift left
#include				"ppp_blit_body_free_opaq.h"
					}
				}
				if(xrem)
				{
					if(logzoom>=0)
						bx=_mm_srli_epi32(m_ksrc, logzoom);
					else
						bx=_mm_slli_epi32(m_ksrc, -logzoom);
					bx=mullo_epi32(bx, m_rx);
					bx=_mm_srli_epi32(bx, 16);
					bx=clamp0h(bx, m_bwm1);

					for(int k=0;k<xrem;++k)
					{
						src.m128i_i32[k]=srow[bx.m128i_i32[k]];
						m_mask.m128i_i32[k]=mask[bx.m128i_i32[k]];
						dst.m128i_i32[k]=rgb[kdst+k];
					}
					m_mask=_mm_cmpeq_epi32(m_mask, _mm_setzero_si128());//preparing the mask

					eq=m_mask;
					dst=_mm_and_si128(dst, eq);

					eq=_mm_xor_si128(eq, m_onesmask);
					eq=_mm_and_si128(eq, src);

					dst=_mm_or_si128(dst, eq);
					for(int k=0;k<xrem;++k)
						rgb[kdst+k]=dst.m128i_i32[k];
				}
			}
#endif
		}
	}
	else//rectangular selection
	{
		if(transparent)
		{
#if 1
			for(int ky=sy1;ky<sy2;++ky)
			{
				int by=(spy-iy0+shift(ky-sy0, -logzoom))*bh/idy;
				by=clamp(0, by, bh-1);
				const int *srow=buffer+bw*by;
				int ksrc=sx1-sx0+shift(spx-ix0, logzoom);
				int kdst=w*ky+sx1;
				__m128i m_ksrc=_mm_set_epi32(ksrc+3*xstep, ksrc+2*xstep, ksrc+xstep, ksrc);
				__m128i bx, src, dst, eq;
				if(logzoom>=0)
				{
					for(int kx=0;kx<xround;kx+=4)
					{
						bx=_mm_srli_epi32(m_ksrc, logzoom);//shift right
#include				"ppp_blit_body_rect_trans.h"
					}
				}
				else
				{
					for(int kx=0;kx<xround;kx+=4)
					{
						bx=_mm_slli_epi32(m_ksrc, -logzoom);//shift left
#include				"ppp_blit_body_rect_trans.h"
					}
				}
				if(xrem)
				{
					if(logzoom>=0)
						bx=_mm_srli_epi32(m_ksrc, logzoom);
					else
						bx=_mm_slli_epi32(m_ksrc, -logzoom);
					bx=mullo_epi32(bx, m_rx);
					bx=_mm_srli_epi32(bx, 16);
					bx=clamp0h(bx, m_bwm1);

					for(int k=0;k<xrem;++k)
					{
						src.m128i_i32[k]=srow[bx.m128i_i32[k]];
						dst.m128i_i32[k]=rgb[kdst+k];
					}

					eq=_mm_cmpeq_epi32(src, m_bkcolor);
					dst=_mm_and_si128(dst, eq);

					eq=_mm_xor_si128(eq, m_onesmask);
					eq=_mm_and_si128(eq, src);

					dst=_mm_or_si128(dst, eq);
					for(int k=0;k<xrem;++k)
						rgb[kdst+k]=dst.m128i_i32[k];
				}
			}
#endif
#if 0//do not remove
			for(int ky=sy1, ycount=0;ycount<yrange;++ycount, ky+=ystep)
			{
				int by=(spy-iy0+shift(ky-sy0, -logzoom))*bh/idy;
				by=clamp(0, by, bh-1);
				int *srow=buffer+bw*by;
				for(int ksrc=sx1-sx0+shift(spx-ix0, logzoom), kdst=w*ky+sx1, xcount=0;xcount<xrange;++xcount, kdst+=xstep, ksrc+=xstep)
				{
					int bx=shift(ksrc, -logzoom)*rx>>16;
					bx=clamp(0, bx, bw-1);
					int src=srow[bx];
					if(src!=bkcolor)
						rgb[kdst]=src;
				}
			}
#endif
#if 0//do not remove
			for(int ky=sy1, ycount=0;ycount<yrange;++ycount, ky+=ystep)
			{
				int by=(spy-iy0+shift(ky-sy0, -logzoom))*bh/idy;//screen position(ky) - imagewindow screen start(sy0) + imagewindow image start(spy) - selection image start(iy0)
				//int by=(spy-iy0+shift(((ky-sy0)&zoommask), -logzoom))*bh/idy;
				//int by=spy-iy0+shift(((ky-sy0)&zoommask)*bh/idy, -logzoom);
				//int by=spy-iy0+shift((ky-sy0)*bh/idy, -logzoom);
				//int by=spy-iy0+shift((ky-sy0)*bh/idy+halfscreenpixel, -logzoom);
				//int by=spy-iy0+shift((ky-sy1)*bh/idy, -logzoom);
				//int by=spy-iy0+shift((ky-y0)*bh/dy, -logzoom);
				//int by=(spy-iy0)*bh+shift((ky-y0)*bh/dy, -logzoom);
				//int by=(spy-iy0+shift((ky-y0)/dy, -logzoom))*bh;//X
				//int by=(spy-iy0+shift(ky-y0, -logzoom))*bh/dy;
				//int by=shift(ky-y0, -logzoom)*bh/dy;
				//int by=(iy0+shift(ky-y0, -logzoom))*bh/dy;
				by=clamp(0, by, bh-1);//when zoomed out: nearest pixel to selection
				//if(by<0||by>=bh)
				//	return;
				for(int kx=sx1, xcount=0;xcount<xrange;++xcount, kx+=xstep)
				{
					int bx=(spx-ix0+shift(kx-sx0, -logzoom))*bw/idx;
					//int bx=(spx-ix0+shift(((kx-sx0)&zoommask), -logzoom))*bw/idx;
					//int bx=spx-ix0+shift(((kx-sx0)&zoommask)*bw/idx, -logzoom);
					//int bx=spx-ix0+shift((kx-sx0)*bw/idx, -logzoom);
					//int bx=spx-ix0+shift((kx-sx0)*bw/idx+halfscreenpixel, -logzoom);
					//int bx=spx-ix0+shift((kx-sx1)*bw/idx, -logzoom);
					//int bx=spx-ix0+shift((kx-x0)*bw/dx, -logzoom);
					//int bx=(spx-ix0)*bw+shift((kx-x0)*bw/dx, -logzoom);
					//int bx=(spx-ix0+shift((kx-x0)/dx, -logzoom))*bw;//X
					//int bx=(spx-ix0+shift(kx-x0, -logzoom))*bw/dx;
					//int bx=shift(kx-x0, -logzoom)*bw/dx;
					//int bx=(ix0+shift(kx-x0, -logzoom))*bw/dx;
					bx=clamp(0, bx, bw-1);
					//if(bx<0||bx>=bw)
					//	return;
					int src=buffer[bw*by+bx];
					if(src!=bkcolor)
						rgb[w*ky+kx]=src;
				}
			}
#endif
		}
		else//rectangular opaque
		{
#if 1
			for(int ky=sy1;ky<sy2;++ky)
			{
				int by=(spy-iy0+shift(ky-sy0, -logzoom))*bh/idy;
				by=clamp(0, by, bh-1);
				const int *srow=buffer+bw*by;
				int ksrc=sx1-sx0+shift(spx-ix0, logzoom);
				int kdst=w*ky+sx1;
				__m128i m_ksrc=_mm_set_epi32(ksrc+3*xstep, ksrc+2*xstep, ksrc+xstep, ksrc);
				__m128i bx;
				if(logzoom>=0)
				{
					for(int kx=0;kx<xround;kx+=4)
					{
						bx=_mm_srli_epi32(m_ksrc, logzoom);//shift right
#include				"ppp_blit_body_rect_opaq.h"
					}
				}
				else
				{
					for(int kx=0;kx<xround;kx+=4)
					{
						bx=_mm_slli_epi32(m_ksrc, -logzoom);//shift left
#include				"ppp_blit_body_rect_opaq.h"
					}
				}
				if(xrem)
				{
					if(logzoom>=0)
						bx=_mm_srli_epi32(m_ksrc, logzoom);
					else
						bx=_mm_slli_epi32(m_ksrc, -logzoom);
					bx=mullo_epi32(bx, m_rx);
					bx=_mm_srli_epi32(bx, 16);
					bx=clamp0h(bx, m_bwm1);
					
					for(int k=0;k<xrem;++k)
						rgb[kdst+k]=buffer[bx.m128i_i32[k]];
				}
			}
#endif
		}
	}//if mask else
}