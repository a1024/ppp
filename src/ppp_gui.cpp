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
	for(int ky=y1;ky<y2;++ky)
		rgb[w*ky+x]=color;
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
	movewindow_c(hStretchHbox, x1, y1, dx, dy, repaint);
	movewindow_c(hStretchVbox, x1, y2, dx, dy, repaint);
	movewindow_c(hSkewHbox, x2, y1, dx, dy, repaint);
	movewindow_c(hSkewVbox, x2, y2, dx, dy, repaint);
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
	int xrange=x2-x1, xround=xrange&~3, xrem=xrange&3, xoffset=x1-x0;
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
			__m128i ky_bit=checkboard_sse2_y(logcb, ky-cby0);
			for(int kx=0;kx<xround;kx+=4)
			{
				src=_mm_set_epi32(by[(kx+3+xoffset)>>logzoom], by[(kx+2+xoffset)>>logzoom], by[(kx+1+xoffset)>>logzoom], by[(kx+xoffset)>>logzoom]);
				//dst=_mm_loadu_si128((__m128i*)pixels);

				//if(src.m128i_i32[0]!=src.m128i_i32[1]||src.m128i_i32[2]!=src.m128i_i32[3])//bouncing ball: hits but shouldn't
				//	int LOL_1=0;//
				dst=checkboard_sse2(logcb, kx-cbx0, ky_bit, c_even, c_odd);
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
				
				dst=checkboard_sse2(logcb, xround-cbx0, ky_bit, c_even, c_odd);
				dst=blend_ssse3(src, dst);
				
				for(int k=0;k<xrem;++k)
					pixels[xround+k]=src.m128i_i32[k];
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
			__m128i ky_bit=checkboard_sse2_y(logcb, ky);
			for(int kx=0;kx<xround;kx+=4)
			{
				src=_mm_set_epi32(by[(kx+3+xoffset)<<-logzoom], by[(kx+2+xoffset)<<-logzoom], by[(kx+1+xoffset)<<-logzoom], by[(kx+xoffset)<<-logzoom]);
				//dst=_mm_loadu_si128((__m128i*)pixels);
				
				dst=checkboard_sse2(logcb, kx-cbx0, ky_bit, c_even, c_odd);
				dst=blend_ssse3(src, dst);

				_mm_storeu_si128((__m128i*)(pixels+kx), dst);
			}
			if(xround<xrange)
			{
				for(int k=0;k<xrem;++k)
					src.m128i_i32[k]=by[(xround+k+xoffset)<<-logzoom];
				//for(int k=0;k<xrem;++k)
				//	dst.m128i_i32[k]=pixels[xround+k];
				
				dst=checkboard_sse2(logcb, xround-cbx0, ky_bit, c_even, c_odd);
				dst=blend_ssse3(src, dst);
				
				for(int k=0;k<xrem;++k)
					pixels[xround+k]=src.m128i_i32[k];
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
	int start=w*ya+x, end=w*yb+x, step=w<<1;
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
	const __m128i mask=_mm_set_epi32(0, 0x00FFFFFF, 0, 0x00FFFFFF);
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