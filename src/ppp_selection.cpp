#include		"ppp.h"
#include		"generic.h"
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
	{
		selection_moved=false;
	//	mark_modified();
		hist_premodify(image, iw, ih);
	}
	if(sel_start!=sel_end)
	{
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
					byte *data=(byte*)bmi+size-bmi->bmiHeader.biSizeImage;
					int data_size=bmi->bmiHeader.biSizeImage;
					//int data_size=size-bmi->bmiHeader.biSize;
					
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