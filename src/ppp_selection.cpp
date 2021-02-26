#include		"ppp.h"
#include		"generic.h"
#if 0
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
	screen2image_rounded(start_mx, start_my, sel_start.x, sel_start.y);
	screen2image_rounded(mx, my, sel_end.x, sel_end.y);
	//int half=shift(1, logzoom-1);
	//screen2image(start_mx+half, start_my+half, sel_start.x, sel_start.y);
	//screen2image(mx+half, my+half, sel_end.x, sel_end.y);

	Point start(0, 0), end(iw, ih);
	//start.image2screen();
	//end.image2screen();
	sel_start.x=clamp(start.x, sel_start.x, end.x);
	sel_start.y=clamp(start.y, sel_start.y, end.y);
	sel_end.x=clamp(start.x, sel_end.x, end.x);
	sel_end.y=clamp(start.y, sel_end.y, end.y);

	selection_sortNbound(sel_start, sel_end, p1, p2);
}
//void			selection_assign()
//{
//	Point p1, p2;
//	selection_sortNbound(sel_start, sel_end, p1, p2);
//	image2screen(p1.x, p1.y, sel_s1.x, sel_s1.y);
//	image2screen(p2.x, p2.y, sel_s2.x, sel_s2.y);
//	selpos=p1;
//	selid=p2-p1;
//}
#endif
void			selection_resize_mouse(Rect &sel, int mx, int my, int prev_mx, int prev_my, int *xflip, int *yflip)
{
	int ret;
	char xanchor=anchors[(drag-D_SEL_RESIZE_TL)<<1], yanchor=anchors[(drag-D_SEL_RESIZE_TL)<<1|1];
	Point mouse(mx, my), prevmouse(prev_mx, prev_my);
	mouse.screen2image_rounded();
	prevmouse.screen2image_rounded();
	ret=assign_using_anchor(xanchor, sel.i.x, sel.f.x, mouse.x, prevmouse.x);
	if(xflip)
		*xflip=ret;
	ret=assign_using_anchor(yanchor, sel.i.y, sel.f.y, mouse.y, prevmouse.y);
	if(yflip)
		*yflip=ret;
}
void			selection_remove()
{
	selection.set(0, 0, 0, 0);
	sw=0, sh=0, sel_buffer=(int*)realloc(sel_buffer, 0);
	//sel_start.set(0, 0), sel_end.set(0, 0);
	//selection_assign();
}
void			selection_select(Rect const &sel)
{
	int x1=minimum(sel.i.x, sel.f.x), dx=abs(sel.f.x-sel.i.x);
	int y1=minimum(sel.i.y, sel.f.y), dy=abs(sel.f.y-sel.i.y), y2=y1+dy;
	int ky;
	if(dx>0&&dy>0)
	{
		hist_premodify(image, iw, ih);
		sw=dx, sh=dy, sel_buffer=(int*)realloc(sel_buffer, sw*sh<<2);
		for(ky=0;ky<dy;++ky)
			memcpy(sel_buffer+sw*ky, image+iw*(y1+ky)+x1, dx<<2);
		for(ky=y1;ky<y2;++ky)
			memfill(image+iw*ky+x1, &secondarycolor, dx<<2, 1<<2);
	}
	selection_free=false;
}
void			selection_stamp(bool modify_hist)//sel_buffer -> image
{
	if(selection.iszero())
		return;
	if(modify_hist)
		hist_premodify(image, iw, ih);
#if 1
	//Rect sr=selection;
	//sr.i.clamp(0, iw, 0, ih);
	//sr.f.clamp(0, iw, 0, ih);
	int ix1, ix2, iy1, iy2;
	selection.get(ix1, ix2, iy1, iy2);
	int idx=ix2-ix1, idy=iy2-iy1;
	int iystart=clamp(0, iy1, ih-1), iyend=clamp(0, iy2, ih),
		ixstart=clamp(0, ix1, iw-1), ixend=clamp(0, ix2, iw);
	//int iystart=-iy1&-(iy1<0), iyend=minimum(ih-iy1, abs(idy)),
	//	ixstart=-ix1&-(ix1<0), ixend=minimum(iw-ix1, abs(idx));
	//iystart=clamp(0, iystart, ih), iyend=clamp(0, iyend, ih);
	//ixstart=clamp(0, ixstart, iw), ixend=clamp(0, ixend, iw);
	int ixstep=(ixstart<ixend)-(ixstart>ixend), ixcount=abs(ixend-ixstart);
	int iystep=(iystart<iyend)-(iystart>iyend), iycount=abs(iyend-iystart);
	if(interpolation_type==I_BILINEAR)//this loop is also in ppp_gui.cpp/stretch_blit(), but with screen2image coordinate conversion
	{
		int abs_idx=abs(idx), halfx=(abs_idx>>1)&-(abs_idx!=sw),
			abs_idy=abs(idy), halfy=(abs_idy>>1)&-(abs_idy!=sh);
		for(int ky=0, iy=iystart;ky<iycount;++ky, iy+=iystep)
		{
			int by=(iy-iy1)*sh-halfy;
			int by0=by;
			int byrem=by%idy;
			by/=idy;
			by-=by0<0;
			byrem+=abs_idy&-((byrem<0)|!byrem&(idy<0));
			int byrem_c=abs_idy-byrem;
			if(idy<0)
				std::swap(byrem, byrem_c);
			//by=clamp(0, by, sh-1);
			if(iy<0||iy>=ih)
				continue;
			int *srcrow0=sel_buffer+sw*by, *srcrow1=srcrow0+sw;
			const int *maskrow0=sel_mask+sw*by, *maskrow1=maskrow0+sw;
			//int *maskrow=(int*)((int)(sel_mask+sw*by)&-selection_free);
			int *dstrow=image+iw*iy;
			//int *dstrow=image+iw*(iy1+iy)+ix1;
			for(int kx=0, ix=ixstart;kx<ixcount;++kx, ix+=ixstep)
			{
				int bx=(ix-ix1)*sw-halfx;
				int bx0=bx;
				int bxrem=bx%idx;
				bx/=idx;
				bx-=bx0<0;
				bxrem+=abs_idx&-((bxrem<0)|!bxrem&(idx<0));
				int bxrem_c=abs_idx-bxrem;
				if(idx<0)
					std::swap(bxrem, bxrem_c);
				//bx=clamp(0, bx, sw-1);
				if(ix<0||ix>=iw)
					continue;
				
				byte *dst=(byte*)(dstrow+ix);
				const byte *src00, *src01, *src10, *src11;
				if(selection_free)
				{
					// yx		ENTIRE ROW: Y IS CONSTANT
					src00=((unsigned)by  <(unsigned)sh)&((unsigned)bx  <(unsigned)sw)?  maskrow0[bx  ]?(const byte*)(srcrow0+bx  ):dst  :(const byte*)&secondarycolor;
					src01=((unsigned)by  <(unsigned)sh)&((unsigned)bx+1<(unsigned)sw)?  maskrow0[bx+1]?(const byte*)(srcrow0+bx+1):dst  :(const byte*)&secondarycolor;
					src10=((unsigned)by+1<(unsigned)sh)&((unsigned)bx  <(unsigned)sw)?  maskrow1[bx  ]?(const byte*)(srcrow1+bx  ):dst  :(const byte*)&secondarycolor;
					src11=((unsigned)by+1<(unsigned)sh)&((unsigned)bx+1<(unsigned)sw)?  maskrow1[bx+1]?(const byte*)(srcrow1+bx+1):dst  :(const byte*)&secondarycolor;
				}
				else
				{
					src00=((unsigned)by  <(unsigned)sh)&((unsigned)bx  <(unsigned)sw)?(const byte*)(srcrow0+bx  ):(const byte*)&secondarycolor;
					src01=((unsigned)by  <(unsigned)sh)&((unsigned)bx+1<(unsigned)sw)?(const byte*)(srcrow0+bx+1):(const byte*)&secondarycolor;
					src10=((unsigned)by+1<(unsigned)sh)&((unsigned)bx  <(unsigned)sw)?(const byte*)(srcrow1+bx  ):(const byte*)&secondarycolor;
					src11=((unsigned)by+1<(unsigned)sh)&((unsigned)bx+1<(unsigned)sw)?(const byte*)(srcrow1+bx+1):(const byte*)&secondarycolor;
				}
				if(selection_transparency==TRANSPARENT)
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
						src0x=src00[kc]+(src01[kc]-src00[kc])*bxrem/abs_idx,
						src1x=src10[kc]+(src11[kc]-src10[kc])*bxrem/abs_idx;
					dst[kc]=src0x+(src1x-src0x)*byrem/abs_idy;
				}
			}
		}
		return;
	}
	for(int ky=0, iy=iystart;ky<iycount;++ky, iy+=iystep)
	{
		int by=(iy-iy1)*sh/idy;
		if(by<0||by>=sh)
			break;
		int *srcrow=sel_buffer+sw*by;
		int *maskrow=(int*)((int)(sel_mask+sw*by)&-selection_free);
		int *dstrow=image+iw*iy;
		//int *dstrow=image+iw*(iy1+iy)+ix1;
		for(int kx=0, ix=ixstart;kx<ixcount;++kx, ix+=ixstep)
		{
			int bx=(ix-ix1)*sw/idx;
			if(bx<0||bx>=sw)
				break;
			if(!selection_free||maskrow[bx])
			{
				int src=srcrow[bx];
				if(selection_transparency==OPAQUE||src!=secondarycolor)
					dstrow[ix]=src;
			}
	/*	if(selection_free)
		{
			if(maskrow[kx])
			{
				int src=srcrow[kx];
				if(selection_transparency==OPAQUE||src!=secondarycolor)
					dstrow[kx]=src;
			}
		}
		else
		{
			int src=srcrow[kx];
			if(selection_transparency==OPAQUE||src!=secondarycolor)
				dstrow[kx]=src;
		}//*/
		}
	}
#endif
#if 0//separate stamp loops
	int ystart=selpos.y>0?0:-selpos.y, yend=minimum(ih-selpos.y, sh),
		xstart=selpos.x>0?0:-selpos.x, xend=minimum(iw-selpos.x, sw);
	if(selection_free)
	{
		for(int ky=ystart;ky<yend;++ky)
		{
			int *srcrow=sel_buffer+sw*ky, *maskrow=sel_mask+sw*ky, *dstrow=image+iw*(selpos.y+ky)+selpos.x;
			for(int kx=xstart;kx<xend;++kx)
			{
				if(maskrow[kx])
				{
					int src=srcrow[kx];
					if(selection_transparency==OPAQUE||src!=secondarycolor)
						dstrow[kx]=src;
				}
			}
			//for(int kx=xstart;kx<xend;++kx)
			//{
			//	int sel_idx=sw*ky+kx;
			//	if(sel_mask[sel_idx])
			//	{
			//		auto &src=sel_buffer[sel_idx];
			//		if(selection_transparency==OPAQUE||src!=secondarycolor)
			//			image[iw*(selpos.y+ky)+selpos.x+kx]=src;
			//	}
			//}
		}
	}
	else//rectangular
	{
		if(selection_transparency==TRANSPARENT)
		{
			for(int ky=ystart;ky<yend;++ky)
			{
				int *srcrow=sel_buffer+sw*ky, *dstrow=image+iw*(selpos.y+ky)+selpos.x;
				for(int kx=xstart;kx<xend;++kx)
				{
					int src=srcrow[kx];
					if(src!=secondarycolor)
						dstrow[kx]=src;
				}
				//for(int kx=xstart;kx<xend;++kx)
				//{
				//	auto &src=sel_buffer[sw*ky+kx];
				//	if(src!=secondarycolor)
				//		image[iw*(selpos.y+ky)+selpos.x+kx]=src;
				//}
			}
		}
		else//opaque
		{
			for(int ky=ystart;ky<yend;++ky)
				memcpy(image+iw*(selpos.y+ky)+selpos.x, sel_buffer+sw*ky, (xend-xstart)<<2);
		}
	}
#endif
}
void			selection_move_mouse(int prev_mx, int prev_my, int mx, int my)
{
	Point m, prev_m;
	screen2image(mx, my, m.x, m.y);
	screen2image(prev_mx, prev_my, prev_m.x, prev_m.y);
	Point d=m-prev_m;
	selection.i+=d;
	selection.f+=d;
	//selection_moved|=d.x||d.y;
	//selpos+=d;
	//sel_start+=d, sel_end+=d, sel_s1+=d, sel_s2+=d;
}
void			selection_selectall()
{
	currentmode=M_RECT_SELECTION;
	selection_stamp(true);
	selection.set(0, 0, iw, ih);
	//sel_start.set(0, 0), sel_end.set(iw, ih);
	//selection_assign();
	//Point p1, p2;
	//selection_sortNbound(sel_start, sel_end, p1, p2);
	selection_select(selection);
}
void			selection_resetscale()
{
	selection.sort_coords();
	//selid.set(sw, sh);
	//if(sel_start.x>sel_end.x)
	//	std::swap(sel_start.x, sel_end.x);
	//if(sel_start.y>sel_end.y)
	//	std::swap(sel_start.y, sel_end.y);

	selection.i.clamp(0, iw-sw, 0, ih-sh);//bring selection inside image
	//sel_start.x=clamp(0, sel_start.x, iw-sw);
	//sel_start.y=clamp(0, sel_start.y, ih-sh);
	
	selection.f.set(selection.i.x+sw, selection.i.y+sh);
	//sel_end=sel_start+selid;
	//selection_assign();
}

std::vector<Point> freesel;//free form selection vertices, image coordinates
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
		hist_premodify(image, iw, ih);
		int xstart, xend, ystart, yend;
		polygon_bounds(freesel, 0, xstart, xend, 0, iw);//get selection bounds
		polygon_bounds(freesel, 1, ystart, yend, 0, ih);

		selection.set(xstart, ystart, xend, yend);//set selection parameters
		//sel_start.set(xstart, ystart), sel_end.set(xend, yend);
		//image2screen(xstart, ystart, sel_s1.x, sel_s1.y);
		//image2screen(xend, yend, sel_s2.x, sel_s2.y);
		//selpos.set(xstart, ystart);
		//selid.set(xend-xstart, yend-ystart);

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
				int kx=(int)std::round(bounds[kb]), kxEnd=(int)std::round(bounds[kb+1]);
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

bool			editcopy()
{
	if(selection.nonzero())
	{
		int size=sizeof(BITMAPINFO)+(sw*sh<<2);
		char *clipboard=(char*)LocalAlloc(LMEM_FIXED, size);
		BITMAPINFO bmi={{sizeof(BITMAPINFOHEADER), sw, -sh, 1, 32, BI_RGB, sw*sh<<2, 0, 0, 0, 0}};
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
					byte *data=(byte*)bmi+size-bmi->bmiHeader.biSizeImage;
					int data_size=bmi->bmiHeader.biSizeImage?bmi->bmiHeader.biSizeImage:bw*bh<<2;
					//int data_size=size-bmi->bmiHeader.biSize;
					
					selection_stamp(true);
					selection.set(spx, spy, spx+bw, spy+bh);
					//sel_start.set(spx, spy), sel_end.set(spx+bw, spy+bh);
					//image2screen(sel_start.x, sel_start.y, sel_s1.x, sel_s1.y);
					//image2screen(sel_end.x, sel_end.y, sel_s2.x, sel_s2.y);
					//selpos=sel_start;
					//selid=sel_end-sel_start;

					currentmode=M_RECT_SELECTION, selection_free=false;
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
							int *dstrow=sel_buffer+sw*ky;
							byte *srcrow=data+3*bw*(bmi->bmiHeader.biHeight<0?ky:bmi->bmiHeader.biHeight-1-ky);
							for(int kx=0;kx<bw;++kx)
							{
								auto dst=(byte*)(dstrow+kx), src=(byte*)(srcrow+3*kx);
								dst[0]=src[0];
								dst[1]=src[1];
								dst[2]=src[2];
								dst[3]=0xFF;
							}
						}
						break;
					case 32:
						for(int ky=0;ky<bh&&(bw*(ky+1)<<2)<=data_size;++ky)
							memcpy(sel_buffer+sw*ky, (int*)data+bw*(bmi->bmiHeader.biHeight<0?ky:bmi->bmiHeader.biHeight-1-ky), bw<<2);
							//for(int kx=0;kx<bw;++kx)
							//	sel_buffer[sw*ky+kx]=((int*)data)[bw*(bmi->bmiHeader.biHeight<0?ky:bmi->bmiHeader.biHeight-1-ky)+kx];
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