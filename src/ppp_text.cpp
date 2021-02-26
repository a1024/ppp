#include		"ppp.h"
#include		"generic.h"
#include		<algorithm>
void			move_textbox()
{
	Rect tr2=textrect;
	tr2.image2screen();
	movewindow_c(hTextbox, tr2.i.x+1, tr2.i.y+1, tr2.f.x-tr2.i.x, tr2.f.y-tr2.i.y, true);
	//movewindow(hTextbox, text_s1.x, text_s1.y, text_s2.x-text_s1.x, text_s2.y-text_s1.y, true);
//	movewindow(hTextbox, 8+text_s1.x, 50+text_s1.y, text_s2.x-text_s1.x, text_s2.y-text_s1.y, true);
//	movewindow(hTextbox, 9+text_s1.x, 51+text_s1.y, text_s2.x-text_s1.x, text_s2.y-text_s1.y, true);
}
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
	textrect.set(0, 0, 0, 0);
	//text_start.setzero(), text_end.setzero();
	//text_s1.setzero(), text_s2.setzero();
	//textpos.setzero();
	ShowWindow(hTextbox, SW_HIDE);
}
void			exit_textmode()
{
	remove_textbox();
	if(colorbarcontents==CS_TEXTOPTIONS)
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
		clear_textbox();

		BITMAPINFO bmpInfo={{sizeof(BITMAPINFOHEADER), iw, -ih, 1, 32, BI_RGB, 0, 0, 0, 0, 0}};
		int *rgb2=nullptr;
		hBitmap=CreateDIBSection(0, &bmpInfo, DIB_RGB_COLORS, (void**)&rgb2, 0, 0);
		hBitmap=(HBITMAP)SelectObject(ghMemDC, hBitmap);
		hFont=(HFONT)SelectObject(ghMemDC, hFont);
		const int alphamask=0xFF000000, colormask=0x00FFFFFF;
		memcpy(rgb2, image, image_size<<2);//copy image for proper anti-aliasing
		for(int k=0;k<image_size;++k)//set original alpha (reverse alpha)
			rgb2[k]|=alphamask;

		textrect.sort_coords();
		RECT r={textrect.i.x, textrect.i.y, textrect.f.x, textrect.f.y};//left top right bottom
		//Point i1, i2;
		//screen2image(text_s1.x, text_s1.y, i1.x, i1.y);
		//screen2image(text_s2.x, text_s2.y, i2.x, i2.y);
		//RECT r={i1.x, i1.y, i2.x, i2.y};
		//RECT r={i1.x+3, i1.y+3, i2.x+3, i2.y+3};//@Batang
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
	textrect.set(-128, -128, -128, -128);
	//text_start.setzero(), text_end.setzero();
	//text_s1.setzero(), text_s2.setzero();
	//textpos.setzero();
	move_textbox();
}
WNDPROC			oldEditProc=nullptr;
long			__stdcall EditProc(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	switch(message)
	{
	case WM_CHAR://ctrl A: select all
		render(REDRAW_IMAGE_PARTIAL, 0, w, 0, h);
		if(wParam==1)
		{
			SendMessageW(hWnd, EM_SETSEL, 0, -1);
			return 1;
		}
		break;
	}
	return CallWindowProcW(oldEditProc, hWnd, message, wParam, lParam);
}
int				__stdcall FontProc(LOGFONTW const *lf, TEXTMETRICW const *tm, unsigned long FontType, long lParam)
{
	fonts.push_back(Font(FontType, lf, tm));
	return true;//nonzero to continue enumeration
}