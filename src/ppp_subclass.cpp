#include		"ppp.h"
#include		"generic.h"
#include		<CommCtrl.h>
const char		file[]=__FILE__;

/*int				getwindowint(HWND hWnd)
{
	int size=SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
	std::string str(size+1, 0);
	SendMessageA(hWnd, WM_GETTEXT, size, (LPARAM)&str[0]);
	int val=atoi(str.c_str());
	return val;
}
double			getwindowdouble(HWND hWnd)
{
	int size=SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
	std::string str(size+1, 0);
	SendMessageA(hWnd, WM_GETTEXT, size, (LPARAM)&str[0]);
	double val=atof(str.c_str());
	return val;
}//*/
int				identify_attr_wnd(HWND hWnd, HWND *hWndArray, int start, int count)
{
	int kWnd=start;
	for(;kWnd<count;++kWnd)
		if(hWnd==hWndArray[kWnd])
			break;
	return kWnd;
}

//FONT OPTIONS GUI
WNDPROC			FontComboboxProc=nullptr, FontSizeCbProc=nullptr;
long			__stdcall FontComboboxSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	switch(message)
	{
	case VK_TAB:
		break;
	}
	if(!FontComboboxProc)
		return 0;
	return CallWindowProcA(FontComboboxProc, hWnd, message, wParam, lParam);
}
long			__stdcall FontSizeCbSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	switch(message)
	{
	case VK_TAB:
		break;
	}
	if(!FontComboboxProc)
		return 0;
	return CallWindowProcA(FontSizeCbProc, hWnd, message, wParam, lParam);
}

//FLIPROTATE OPTIONS
WNDPROC			RotateBoxProc=nullptr;
long			__stdcall RotateBoxSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	switch(message)
	{
	case VK_TAB:
		break;
	}
	if(!FontComboboxProc)
		return 0;
	return CallWindowProcA(RotateBoxProc, hWnd, message, wParam, lParam);
}

//STRETCHSKEW OPTIONS
HWND			hStretchSkew[4]={nullptr};
WNDPROC			stretchskewproc[4]={nullptr};
long			__stdcall StretchSkewSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	int kWnd=identify_attr_wnd(hWnd, hStretchSkew, 0, 4);
	if(kWnd>=4)
		return 0;
	switch(message)
	{
	case WM_CHAR:
		switch(wParam)
		{
		case 1:
			SendMessageA(hWnd, EM_SETSEL, 0, -1);
			return 1;
		case VK_TAB:
		case VK_RETURN:
		case VK_ESCAPE:
			return 0;
		}
		break;
	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_TAB:
			{
				HWND nextWnd=hStretchSkew[mod(kWnd+1-(kb[VK_SHIFT]<<1), 4)];
				SendMessageA(nextWnd, EM_SETSEL, 0, -1);
				HWND prevWnd=SetFocus(nextWnd);
				SYS_ASSERT(prevWnd);
			}
			break;
		case VK_RETURN:
			stretchskew();
		case VK_ESCAPE:
			replace_colorbarcontents(CS_NONE);
			break;
		}
		kb[wParam]=true;
		break;
	case WM_KEYUP:
		kb[wParam]=false;
		break;
	}
	return CallWindowProcA(stretchskewproc[kWnd], hWnd, message, wParam, lParam);
}
void			create_stretchskew()
{
	HFONT hfDefault=(HFONT)GetStockObject(DEFAULT_GUI_FONT);	GEN_ASSERT(hfDefault);
	for(int k=0;k<4;++k)
	{
		hStretchSkew[k]=CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, nullptr, WS_CHILD|WS_VISIBLE | ES_LEFT|ES_WANTRETURN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, ghWnd, (HMENU)(IDM_STRETCH_H_BOX+k), ghInstance, nullptr);
		SYS_ASSERT(hStretchSkew[k]);

		SendMessageW(hStretchSkew[k], WM_SETFONT, (WPARAM)hfDefault, 0);
		SYS_CHECK();

		stretchskewproc[k]=(WNDPROC)SetWindowLongPtrA(hStretchSkew[k], GWLP_WNDPROC, (long)StretchSkewSubclass);
		SYS_ASSERT(stretchskewproc[k]);
	}
	//hStretchSkew[0]=CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, nullptr, WS_CHILD|WS_VISIBLE | ES_LEFT|ES_WANTRETURN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, ghWnd, (HMENU)IDM_STRETCH_H_BOX,	ghInstance, nullptr);	SYS_ASSERT(hStretchSkew[0]);
	//hStretchSkew[1]=CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, nullptr, WS_CHILD|WS_VISIBLE | ES_LEFT|ES_WANTRETURN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, ghWnd, (HMENU)IDM_STRETCH_V_BOX,	ghInstance, nullptr);	SYS_ASSERT(hStretchSkew[1]);
	//hStretchSkew[2]=CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, nullptr, WS_CHILD|WS_VISIBLE | ES_LEFT|ES_WANTRETURN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, ghWnd, (HMENU)IDM_SKEW_H_BOX,		ghInstance, nullptr);	SYS_ASSERT(hStretchSkew[2]);
	//hStretchSkew[3]=CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, nullptr, WS_CHILD|WS_VISIBLE | ES_LEFT|ES_WANTRETURN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, ghWnd, (HMENU)IDM_SKEW_V_BOX,		ghInstance, nullptr);	SYS_ASSERT(hStretchSkew[3]);
	//SendMessageW(hStretchSkew[0], WM_SETFONT, (WPARAM)hfDefault, 0);	SYS_CHECK();
	//SendMessageW(hStretchSkew[1], WM_SETFONT, (WPARAM)hfDefault, 0);	SYS_CHECK();
	//SendMessageW(hStretchSkew[2], WM_SETFONT, (WPARAM)hfDefault, 0);	SYS_CHECK();
	//SendMessageW(hStretchSkew[3], WM_SETFONT, (WPARAM)hfDefault, 0);	SYS_CHECK();
	//stretchskewproc[0]=(WNDPROC)SetWindowLongPtrA(hStretchSkew[0], GWLP_WNDPROC, (long)StretchSkewSubclass);	SYS_ASSERT(stretchskewproc[0]);
	//stretchskewproc[1]=(WNDPROC)SetWindowLongPtrA(hStretchSkew[1], GWLP_WNDPROC, (long)StretchSkewSubclass);	SYS_ASSERT(stretchskewproc[1]);
	//stretchskewproc[2]=(WNDPROC)SetWindowLongPtrA(hStretchSkew[2], GWLP_WNDPROC, (long)StretchSkewSubclass);	SYS_ASSERT(stretchskewproc[2]);
	//stretchskewproc[3]=(WNDPROC)SetWindowLongPtrA(hStretchSkew[3], GWLP_WNDPROC, (long)StretchSkewSubclass);	SYS_ASSERT(stretchskewproc[3]);
}
#if 0
WNDPROC			StretchHBoxProc=nullptr, StretchVBoxProc=nullptr, SkewHBoxProc=nullptr, SkewVBoxProc=nullptr;
long			__stdcall StretchHBoxSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	switch(message)
	{
	case VK_TAB:
		break;
	}
	if(!FontComboboxProc)
		return 0;
	return CallWindowProcA(StretchHBoxProc, hWnd, message, wParam, lParam);
}
long			__stdcall StretchVBoxSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	switch(message)
	{
	case VK_TAB:
		break;
	}
	if(!FontComboboxProc)
		return 0;
	return CallWindowProcA(StretchVBoxProc, hWnd, message, wParam, lParam);
}
long			__stdcall SkewHBoxSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	switch(message)
	{
	case VK_TAB:
		break;
	}
	if(!FontComboboxProc)
		return 0;
	return CallWindowProcA(SkewHBoxProc, hWnd, message, wParam, lParam);
}
long			__stdcall SkewVBoxSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	switch(message)
	{
	case VK_TAB:
		break;
	}
	if(!FontComboboxProc)
		return 0;
	return CallWindowProcA(SkewVBoxProc, hWnd, message, wParam, lParam);
}
#endif

//MASK WINDOW
const int		xpad=10, ypad=10,
				rowh=25, labelw=75, boxw=50, boxh=19, sliderw=28+510, buttonw=75;

int				brushmask=0xFFFFFFFF;
HWND			hWndMask=nullptr;
static HWND		hMaskEdit[5]={nullptr}, hMaskTrack[5]={nullptr}, hMaskOK;//{common, r, g, b, a}
enum			WndMaskCommands
{
	ID_BOX_ALL=1,	ID_TRACK_ALL,
	ID_BOX_R,		ID_TRACK_R,
	ID_BOX_G,		ID_TRACK_G,
	ID_BOX_B,		ID_TRACK_B,
	ID_BOX_A,		ID_TRACK_A,
	ID_OK,
};
long			__stdcall WndProcMask(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	switch(message)
	{
	case WM_CREATE:
		{
			int x=xpad+labelw+xpad, y=ypad+((rowh-boxh)>>1);
			HFONT hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
			for(int k=0;k<5;++k)
			{
				hMaskEdit[k]=CreateWindowExA(WS_EX_CLIENTEDGE, WC_EDITA, nullptr, WS_CHILD|WS_VISIBLE|ES_CENTER, x, y, boxw, boxh, hWnd, (HMENU)(ID_BOX_ALL+(k<<1)), ghInstance, nullptr);
				SYS_ASSERT(hMaskEdit[k]);
				SendMessageA(hMaskEdit[k], WM_SETFONT, (WPARAM)hFont, 0);	//SYS_CHECK();
				y+=rowh+(ypad<<int(k==0));
			}
			x+=boxw+xpad, y=ypad;
			for(int k=0;k<5;++k)
			{
				hMaskTrack[k]=CreateWindowExA(0, TRACKBAR_CLASSA, nullptr, WS_CHILD|WS_VISIBLE, x, y, sliderw, rowh, hWnd, (HMENU)(ID_TRACK_ALL+(k<<1)), ghInstance, nullptr);
				SYS_ASSERT(hMaskTrack[k]);
				y+=rowh+(ypad<<int(k==0));
			}
			y+=ypad;
			hMaskOK=CreateWindowExA(0, WC_BUTTONA, nullptr, WS_CHILD|WS_VISIBLE, x+sliderw-buttonw, y, buttonw, rowh, hWnd, (HMENU)ID_OK, ghInstance, nullptr);
			SendMessageA(hMaskOK, WM_SETFONT, (WPARAM)hFont, 0);	//SYS_CHECK();
			SetWindowTextA(hMaskOK, "OK");
		}
		break;
	case WM_COMMAND:
		{
			int wp_hi=((short*)&wParam)[1], wp_lo=(short&)wParam;
			switch(wp_lo)
			{
			case ID_BOX_ALL:
				if(wp_hi==CBN_SELCHANGE||wp_hi==CBN_EDITCHANGE)
				{
					double val=getwindowdouble(hMaskEdit[0]);
					//int size=SendMessageA(hMaskEdit[0], WM_GETTEXTLENGTH, 0, 0);
					//std::string str(size, 0);
					//SendMessageA(hTextbox, WM_GETTEXT, size, (LPARAM)&str[0]);
					//double val=atof(str.c_str());
				}
				break;
			case ID_TRACK_ALL:
				break;
			}
		}
		break;
	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_ESCAPE:
			ShowWindow(hWnd, SW_HIDE);
			break;
		case VK_RETURN:
			//apply mask
			ShowWindow(hWnd, SW_HIDE);
			break;
		}
		break;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return 0;
	}
	return DefWindowProcA(hWnd, message, wParam, lParam);
}
void			create_maskwindow()
{
	const int
		cl_w=xpad+labelw+xpad+boxw+xpad+sliderw+xpad,
		cl_h=ypad+(rowh+ypad)*6+(ypad<<1);
	int style=WS_POPUP|WS_CAPTION|WS_SYSMENU, exstyle=WS_EX_PALETTEWINDOW;
//	int style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME;
	RECT r={0, 0, cl_w, cl_h};
	int success=AdjustWindowRectEx(&r, style, false, exstyle);	SYS_ASSERT(success);
	int nc_w=r.right-r.left, nc_h=r.bottom-r.top;
	POINT center={w>>1, h>>1};
	ClientToScreen(ghWnd, &center);

	tagWNDCLASSEXA wndClassEx=
	{
		sizeof(tagWNDCLASSEXA),
		CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS,
		WndProcMask, 0, 0, ghInstance,
		LoadIconA(nullptr, (char*)IDI_APPLICATION),
		LoadCursorA(nullptr, (char*)IDC_ARROW),
		(HBRUSH__*)(COLOR_BTNFACE+1),
		0, "Mask Window", 0
	};
	success=RegisterClassExA(&wndClassEx);	SYS_ASSERT(success);
	hWndMask=CreateWindowExA
		(
			exstyle,
			wndClassEx.lpszClassName, "Mask",
			style,
			center.x-(nc_w>>1), center.y-(nc_h>>1), nc_w, nc_h,
			ghWnd, nullptr, ghInstance, nullptr
		);
	SYS_ASSERT(hWndMask);
}

//ATTRIBUTES WINDOW
const int		attr_xpad=10, attr_ypad=10,
				attr_col_w=90, attr_row_h=28,
				attr_edit_w=48, attr_edit_h=20,
				attr_button_w=90, attr_button_h=23;
HWND			hWndAttributes=nullptr;
enum			AttirbutesMessages
{
	ATTR_WIDTH=1,
	ATTR_HEIGHT,
	ATTR_SAMEBUFFER,
	ATTR_OK,
	ATTR_CANCEL,
	ATTR_NCONTROLS//number of controls plus one
};
static HWND		hAttr[ATTR_NCONTROLS]={nullptr};
static WNDPROC	AttrProc[ATTR_NCONTROLS]={nullptr};
char			same_buffer=false;
void			apply_image_dimensions()
{
	int newwidth=getwindowint(hAttr[ATTR_WIDTH]), newheight=getwindowint(hAttr[ATTR_HEIGHT]);
	if(newwidth&&newheight)
	{
		if(same_buffer)
		{
			image_size=iw*ih;
			int height=image_size/newwidth+(image_size%newwidth!=0);//ceil(size/h)
			int size=newwidth*height;
			int *image2=(int*)malloc(size<<2);
			int minsize=minimum(size, image_size);
			memcpy(image2, image, minsize<<2);
			if(size>image_size)//fill remainder with white
				memset(image2+image_size, 0xFF, (size-image_size)<<2);
			hist_postmodify(image2, newwidth, height);
		}
		else
			hist_premodify(image, newwidth, newheight);//modify & resize
	}
}
long			__stdcall WndSubclassAttributes(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	int kWnd=identify_attr_wnd(hWnd, hAttr, 1, ATTR_NCONTROLS);
	if(kWnd>=ATTR_NCONTROLS)
		return 0;
	switch(message)
	{
	case WM_CHAR:
		switch(wParam)
		{
		case 1:
			SendMessageA(hWnd, EM_SETSEL, 0, -1);
			return 1;
		case VK_TAB:
		case VK_RETURN:
		case VK_ESCAPE:
			return 0;
		}
		break;
	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_TAB:
			{
				HWND nextWnd=hAttr[1+mod(kWnd-(kb[VK_SHIFT]<<1), ATTR_NCONTROLS-1)];
				SendMessageA(nextWnd, EM_SETSEL, 0, -1);
				HWND prevWnd=SetFocus(nextWnd);
				SYS_ASSERT(prevWnd);
			}
			break;
		case VK_ESCAPE:
			ShowWindow(hWndAttributes, SW_HIDE);
			break;
		case VK_RETURN:
			apply_image_dimensions();
			ShowWindow(hWndAttributes, SW_HIDE);
			break;
		}
		kb[wParam]=true;
		break;
	case WM_KEYUP:
		kb[wParam]=false;
		break;
	}
	return CallWindowProcA(AttrProc[kWnd], hWnd, message, wParam, lParam);
}
long			__stdcall WndProcAttributes(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam)
{
	switch(message)
	{
	case WM_CREATE:
		{
			HFONT hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
			int x=attr_col_w, y=attr_ypad+1, success;
			for(int k=0;k<2;++k)
			{
				hAttr[ATTR_WIDTH+k]	=CreateWindowExA(WS_EX_CLIENTEDGE, WC_EDITA, nullptr, WS_CHILD|WS_VISIBLE|ES_LEFT|ES_WANTRETURN, x, y, attr_edit_w, attr_edit_h, hWnd, (HMENU)(ATTR_WIDTH+k), ghInstance, nullptr);
				SYS_ASSERT(hAttr[ATTR_WIDTH+k]);
				SendMessageA(hAttr[ATTR_WIDTH+k], WM_SETFONT, (WPARAM)hFont, 0);
				SYS_CHECK();
				y+=attr_row_h;
			}
			const char *buttonlabels[]={"OK", "Cancel", "Same buffer"};
			hAttr[ATTR_SAMEBUFFER]	=CreateWindowExA(0, WC_BUTTONA, nullptr, WS_CHILD|WS_VISIBLE | BS_CHECKBOX, attr_xpad, y, attr_button_w, attr_button_h, hWnd, (HMENU)ATTR_SAMEBUFFER, ghInstance, nullptr);
			SYS_ASSERT(hAttr[ATTR_SAMEBUFFER]);
			SendMessageA(hAttr[ATTR_SAMEBUFFER], WM_SETFONT, (WPARAM)hFont, 0);	SYS_CHECK();
			success=SetWindowTextA(hAttr[ATTR_SAMEBUFFER], buttonlabels[2]);	SYS_ASSERT(success);

			x+=attr_col_w, y=attr_ypad;
			for(int k=0;k<2;++k)
			{
				hAttr[ATTR_OK+k]	=CreateWindowExA(0, WC_BUTTONA, nullptr, WS_CHILD|WS_VISIBLE, x, y, attr_button_w, attr_button_h, hWnd, (HMENU)(ATTR_OK+k), ghInstance, nullptr);
				SYS_ASSERT(hAttr[ATTR_OK+k]);
				SendMessageA(hAttr[ATTR_OK+k], WM_SETFONT, (WPARAM)hFont, 0);	SYS_CHECK();
				success=SetWindowTextA(hAttr[ATTR_OK+k], buttonlabels[k]);		SYS_ASSERT(success);
				y+=attr_row_h;
			}

			for(int k=1;k<ATTR_NCONTROLS;++k)//subclass
			{
				AttrProc[k]=(WNDPROC)SetWindowLongPtrA(hAttr[k], GWLP_WNDPROC, (long)WndSubclassAttributes);
				SYS_ASSERT(AttrProc[k]);
			}
		}
		break;
	case WM_SHOWWINDOW:
		{
			sprintf_s(g_buf, g_buf_size, "%d", iw);
			int success=SetWindowTextA(hAttr[ATTR_WIDTH], g_buf);	SYS_ASSERT(success);

			sprintf_s(g_buf, g_buf_size, "%d", ih);
			success=SetWindowTextA(hAttr[ATTR_HEIGHT], g_buf);		SYS_ASSERT(success);

			SendMessageA(hAttr[ATTR_SAMEBUFFER], BM_SETCHECK, same_buffer?BST_CHECKED:BST_UNCHECKED, 0);

			HWND prevWnd=SetFocus(hAttr[1]);	SYS_ASSERT(prevWnd);
			SendMessageA(hAttr[1], EM_SETSEL, 0, -1);
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC=BeginPaint(hWnd, &ps);
			HFONT hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
			hFont=(HFONT)SelectObject(hDC, hFont);
			int bkmode=SetBkMode(hDC, TRANSPARENT);

			int x=attr_xpad, y=attr_ypad+4;
			GUIPrint(hDC, x, y, "Width:");

			y+=attr_row_h;
			GUIPrint(hDC, x, y, "Height:");

			SetBkMode(hDC, bkmode);
			hFont=(HFONT)SelectObject(hDC, hFont);
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_COMMAND:
		{
			int wp_hi=((short*)&wParam)[1], wp_lo=(short&)wParam;
			switch(wp_lo)
			{
			//case ATTR_WIDTH:
			//case ATTR_HEIGHT:
			//	switch(wp_hi)
			//	{
			//	case EN_CHANGE:
			//	case EN_UPDATE:
			//		SetFocus(hAttr[1+(wp_hi)%(ATTR_NCONTROLS-1)]);
			//		break;
			//	}
			//	break;
			case ATTR_SAMEBUFFER:
				same_buffer=!same_buffer;
				SendMessageA(hAttr[ATTR_SAMEBUFFER], BM_SETCHECK, same_buffer?BST_CHECKED:BST_UNCHECKED, 0);
				//same_buffer=IsDlgButtonChecked(hWnd, ATTR_SAMEBUFFER)==BST_CHECKED;
				break;
			case ATTR_OK:
				apply_image_dimensions();
				ShowWindow(hWnd, SW_HIDE);
				break;
			case ATTR_CANCEL:
				ShowWindow(hWnd, SW_HIDE);
				break;
			}
		}
		break;
	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_ESCAPE:
			ShowWindow(hWnd, SW_HIDE);
			break;
		case VK_RETURN:
			apply_image_dimensions();
			ShowWindow(hWnd, SW_HIDE);
			break;
		}
		break;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return 0;
	}
	return DefWindowProcA(hWnd, message, wParam, lParam);
}
void			create_attributeswindow()
{
	const int
		cl_w=attr_xpad*2+attr_col_w*3,
		cl_h=attr_ypad*2+attr_row_h*3;
	int style=WS_POPUP|WS_CAPTION|WS_SYSMENU, exstyle=WS_EX_PALETTEWINDOW;
	RECT r={0, 0, cl_w, cl_h};
	int success=AdjustWindowRectEx(&r, style, false, exstyle);	SYS_ASSERT(success);
	int nc_w=r.right-r.left, nc_h=r.bottom-r.top;
	POINT center={w>>1, h>>1};
	ClientToScreen(ghWnd, &center);

	tagWNDCLASSEXA wndClassEx=
	{
		sizeof(tagWNDCLASSEXA),
		CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS,
		WndProcAttributes, 0, 0, ghInstance,
		LoadIconA(nullptr, (char*)IDI_APPLICATION),
		LoadCursorA(nullptr, (char*)IDC_ARROW),
		(HBRUSH__*)(COLOR_BTNFACE+1),
		0, "Attributes Window", 0
	};
	success=RegisterClassExA(&wndClassEx);	SYS_ASSERT(success);
	hWndAttributes=CreateWindowExA
		(
			exstyle,
			wndClassEx.lpszClassName, "Attributes",
			style,
			center.x-(nc_w>>1), center.y-(nc_h>>1), nc_w, nc_h,
			ghWnd, nullptr, ghInstance, nullptr
		);
	SYS_ASSERT(hWndAttributes);
	hAttr[0]=hWndAttributes;
}