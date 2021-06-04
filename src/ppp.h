#ifndef PPP_H
#define PPP_H
#include		<vector>
#include		<string>
#include		<Windows.h>

//	#define		RELEASE//disables hpos=... and SHOW_WINMESSAGES

//#ifndef RELEASE
//	#define		PROFILER//unused
//#endif
	#define		CAPTURE_MOUSE
	#define		SSSE3_BLEND//must be enabled
//	#define		SHOW_WINMESSAGES

//	#define		MOUSE_POLLING_TEST//must be disabled

extern char		debugmode;

//error handling
extern const int e_msg_size;
extern char		first_error_msg[], latest_error_msg[];
void			print_errors(HDC hDC);
bool 			log_error(const char *file, int line, const char *format, ...);
#define			LOG_ERROR(format, ...)	log_error(file, __LINE__, format, __VA_ARGS__)
int				sys_check(const char *file, int line);
#define			SYS_CHECK()		sys_check(file, __LINE__)
#define			SYS_ASSERT(SUCCESS)		((void)((SUCCESS)!=0||sys_check(file, __LINE__)))
#define			GEN_ASSERT(SUCCESS)		((void)((SUCCESS)!=0||log_error(file, __LINE__, #SUCCESS)))

//bitmaps
struct			Bitmap//pass Bitmap with dimensions instead of pointer
{
	int w, h;
	int *rgb;//not rgb[1] because of CreateDIBSection
};
enum			RawMode
{
	RM_INT,//'image' is int 0xAARRGGBB, 256-level channels		size=iw*ih px
	RM_FLOAT_MOSAIC,//'image' is RGGB float Bayer mosaic, [0~1] channels		size=iw*ih px
	RM_FLOAT,//'image' is float {r, g, b, a}, [0~1] channels		size=(iw/4)*ih px (iw is quadrupled for 16 bytes per pixel)
};
extern RawMode	rawMode;
enum			HistogramMode
{
	H_OFF,
	H_HISTOGRAM,
	H_CROSS_SECTION,
	H_COUNT,
};
extern HistogramMode histogramMode;
extern int		*rgb, w, h, rgbn,
				mx, my,//current mouse position
				prev_mx, prev_my,//previous mouse position
				start_mx, start_my,//original mouse position
				*image,			iw, ih, image_size, nchannels,//image==history[histpos].buffer always
				*temp_buffer,	tw, th,	//temporary buffer, has alpha
				*sel_buffer,	sw, sh,	//selection buffer, has alpha
				*sel_mask,		//selection mask, used only in free-form selection, inside selection polygon if true, same dimensions as sel_buffer
				logzoom,//log2(pixel size)
				spx, spy;//position of top left corner of the window in image coordinates
extern double	zoom;//a power of 2
extern bool		use_temp_buffer,
				modified;

extern int		current_frame, nframes,
				framerate_num, framerate_den;
extern bool		animated;

//strings		all paths use forward slashes only, and are stripped of doublequotes and trailing slash
extern std::wstring	workspace,//the workspace containing the workfolder
				workfolder;//the folder containing opened temp files, it has the name 'date time'
enum			ImageFormat{IF_BMP, IF_TIFF, IF_PNG, IF_JPEG, IF_FLIF};
extern int		tempformat;
extern const wchar_t *settingsfilename, *imageformats[];
extern std::vector<std::wstring> framenames;//file names of all frames, must append workfolder with possible doublequote
extern std::vector<int*> thumbnails;//same size as framenames
extern std::wstring	programpath, FFmpeg_path;
//extern bool	FFmpeg_path_has_quotes, workspace_has_quotes;
extern std::string FFmpeg_version;
extern std::wstring	filename, filepath;//opened media file name & path, utf16
extern const wchar_t doublequote;

//GUI
enum			Commands
{
	IDM_FONT_COMBOBOX=1, IDM_FONTSIZE_COMBOBOX, IDM_TEXT_EDIT,
	IDM_ROTATE_BOX,
	IDM_STRETCH_H_BOX, IDM_STRETCH_V_BOX, IDM_SKEW_H_BOX, IDM_SKEW_V_BOX,

	//menu bar
	IDM_FILE_NEW, IDM_FILE_OPEN, IDM_FILE_SAVE, IDM_FILE_SAVE_AS, IDM_FILE_QUIT,
	IDM_EDIT_UNDO, IDM_EDIT_REPEAT, IDM_EDIT_CUT, IDM_EDIT_COPY, IDM_EDIT_PASTE, IDM_EDIT_CLEAR_SELECTION, IDM_EDIT_SELECT_ALL, IDM_EDIT_RESET_SEL_SCALE, IDM_EDIT_COPY_TO, IDM_EDIT_PASTE_FROM,
	IDM_VIEW_TOOLBOX, IDM_VIEW_COLORBOX, IDM_VIEW_STATUSBAR, IDM_VIEW_TEXT_TOOLBAR, IDM_VIEW_ZOOM, IDM_VIEW_VIEWBITMAP,
		IDSM_ZOOM_IN, IDSM_ZOOM_OUT, IDSM_ZOOM_CUSTOM, IDSM_ZOOM_SHOW_GRID, IDSM_ZOOM_SHOW_THUMBNAIL,
	IDM_IMAGE_FLIP_ROTATE, IDM_IMAGE_SKETCH_SKEW, IDM_IMAGE_INVERT_COLORS, IDM_IMAGE_ATTRIBUTES, IDM_IMAGE_CLEAR_IMAGE, IDM_IMAGE_CLEAR_ALPHA, IDM_IMAGE_SET_CHANNEL, IDM_IMAGE_ASSIGN_CHANNEL, IDM_IMAGE_EXPORT_CHANNEL, IDM_RAW_TO_FLOAT, IDM_IMAGE_DRAW_OPAQUE, IDM_IMAGE_LINEAR,
	IDM_COLORS_EDITCOLORS, IDM_COLORS_EDITMASK,
	IDM_HELP_TOPICS, IDM_HELP_ABOUT,
};
extern int		th_w, th_h,//thumbnail dimensions
				thbox_h,//thumbnail box internal height
				thbox_x1,//thumbnail box xstart
				thbox_posy;//thumbnail box scroll position (thbox top -> screen top)
extern const int scrollbarwidth;
struct			Scrollbar
{
	short
		dwidth,	//dynamic width: 0 means hidden, 'scrollbarwidth' means exists
		m_start,//starting mouse position
		s0,		//initial scrollbar slider start
		start,	//scrollbar slider start
		size;	//scrollbar slider size
	void decide(char condition){dwidth=scrollbarwidth&-condition;}//decide if the scrollbar will be drawn
	void decide_orwith(char condition){dwidth|=scrollbarwidth&-condition;}
	void leftbuttondown(int m){m_start=m, s0=start;}
};
extern Scrollbar vscroll, hscroll,//image scrollbars
				thbox_vscroll;
extern tagRECT	R;
extern HINSTANCE ghInstance;
extern HWND		ghWnd;
extern HWND		hFontcombobox, hFontsizecb, hTextbox,
				hRotatebox;
//				hStretchHbox, hStretchVbox, hSkewHbox, hSkewVbox;
extern HDC		ghDC, ghMemDC;
extern HBITMAP	hBitmap;

//paint++
enum			DragType
{
	D_NONE,
	D_DRAW,
	D_SELECTION,//drag selection
	D_HSCROLL, D_VSCROLL, D_THBOX_VSCROLL,//scrollbars

		D_ARROWS_START,
	D_HSCROLL_BACK, D_HSCROLL_FORWARD,
	D_VSCROLL_BACK, D_VSCROLL_FORWARD,
	D_THBOX_VSCROLL_BACK, D_THBOX_VSCROLL_FORWARD,
		D_ARROWS_END,//end of scrollbars

	D_ALPHA1, D_ALPHA2,

		D_RESIZE_START,

	D_RESIZE_TL, D_RESIZE_TOP, D_RESIZE_TR,
	D_RESIZE_LEFT, D_RESIZE_RIGHT,
	D_RESIZE_BL, D_RESIZE_BOTTOM, D_RESIZE_BR,

		D_RESIZE_SEL_START,

	D_SEL_RESIZE_TL, D_SEL_RESIZE_TOP, D_SEL_RESIZE_TR,
	D_SEL_RESIZE_LEFT, D_SEL_RESIZE_RIGHT,
	D_SEL_RESIZE_BL, D_SEL_RESIZE_BOTTOM, D_SEL_RESIZE_BR,
	
		D_RESIZE_END,
};
extern char		kb[256],
				drag, prev_drag;//drag type
enum			TimerID
{
	TIMER_AIRBRUSH=1,
	TIMER_SCROLLBAR_INIT,
	TIMER_SCROLLBAR_CONT,
};
extern int		timer;//0: inactive, otherwise timer id

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
extern const int truetypesizes[], truetypesizes_size;
extern std::vector<Font> fonts;
extern int		font_idx, font_size;
extern bool		bold, italic, underline;
extern HFONT	hFont;

//clipboard
void			copy_to_clipboard(const char *a, int size);//size not including null terminator
inline void		copy_to_clipboard(std::string const &str){copy_to_clipboard(str.c_str(), str.size());}
void			sound_to_clipboard(short *audio_buf, float *L, float *R, int nsamples);
void			buffer_to_clipboard(const void *buffer, int size_bytes);
void			polygon_to_clipboard(const int *buffer, int npoints);
void			syscolors_to_clipboard();


//files
void			read_binary(const wchar_t *filename, std::vector<byte> &binary_data);
void			write_file(const wchar_t *filename, std::string const &output);
void			write_binary(const wchar_t *filename, std::vector<byte> const &binary_data);
void			read_utf8_file(const wchar_t *filename, std::wstring &output);
int				write_utf8_file(const wchar_t *filename, std::wstring &input);
void			remove_directory(const wchar_t *dir);//Fully qualified name of the directory being deleted, without trailing backslash

void			read_settings();
void			write_settings();
void			writeworkfolder2wbuf();
void			create_workfolder_from_wbuf();
void			delete_workfolder();

void			load_frame(int frame_number);
void			save_frame(int frame_number);
void			assign_thumbnail(int *thumbnail, int *image);
void			load_thumbnails();

bool			open_media(std::wstring filepath_u16);
void			open_media();
void			save_media(std::wstring const &outputpath_u16);
bool			save_media_as();

//process
void			exec_FFmpeg_command(const wchar_t *executable, std::wstring const &args, std::string &result);
bool			check_FFmpeg_path(std::wstring &path);
bool			check_FFmpeg_path(std::string &path);


//resources
namespace		resources
{
	extern const unsigned char
		free_select[],	select[],
		eraser[],		fill[],
		pick_color[],	magnifier[],
		pencil[],		brush[],
		airbrush[],		text[],
		line[],			curve[],
		rectangle[],	polygon[],
		ellipse[],		rounded_rectangle[];

	extern const char selection_transparency[];

	//all brushes
	extern const int
		width1[], width2[], width3[], width4[], width5[],
		eraser04[], eraser06[], eraser08[], eraser10[],
		large_disk[],		disk[],		dot[],
		*large_square,		square[],	small_square[],
		large_right45[],	right45[],	small_right45[],
		large_left45[],		left45[],	small_left45[];
	struct		Brush
	{
		const int *bounds;
		int yoffset, ysize;
	};
	extern const Brush brushes[];
	extern const int nbrushes;

	extern const int resizemark_even_w, resizemark_odd_w, resizemark_h,
			resizemark_even[], resizemark_odd[];
}
void			unpack_icon(const unsigned char *rle, int csize, int *rgb2);
void			unpack_icons(int *&icons, int &nicons);


//subclasses
enum			ChildWndFocus
{
	FOCUS_MAIN,
	FOCUS_FONTCB, FOCUS_FONTSIZECB, FOCUS_BOLD, FOCUS_UNDERLINE, FOCUS_ITALIC,
};
extern int		cfocus;
//font options GUI
extern WNDPROC	FontComboboxProc, FontSizeCbProc;
long			__stdcall FontComboboxSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam);
long			__stdcall FontSizeCbSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam);

//flip/rotate
extern WNDPROC	RotateBoxProc;
long			__stdcall RotateBoxSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam);

//stretch/skew
enum			StretchSkew
{
	SS_STRETCH_H, SS_STRETCH_V,
	SS_SKEW_H, SS_SKEW_V,
};
extern HWND		hStretchSkew[4];
void			create_stretchskew();
//extern WNDPROC	StretchHBoxProc, StretchVBoxProc, SkewHBoxProc, SkewVBoxProc;
//long			__stdcall StretchHBoxSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam);
//long			__stdcall StretchVBoxSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam);
//long			__stdcall SkewHBoxSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam);
//long			__stdcall SkewVBoxSubclass(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam);

//mask
extern int		brushmask;
extern HWND		hWndMask;
long			__stdcall WndProcMask(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam);
void			create_maskwindow();

extern HWND		hWndAttributes;
long			__stdcall WndProcAttributes(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam);
void			create_attributeswindow();


//paint++
#include		"g_cpp11.h"
#include		"g_minmax.h"
extern bool		showgrid,
				showcolorbar, showtoolbox, showthumbnailbox;
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
inline Point2d	operator+(Point2d const &a, Point2d const &b){return Point2d(a.x+b.x, a.y+b.y);}
inline Point2d	operator-(Point2d const &a, Point2d const &b){return Point2d(a.x-b.x, a.y-b.y);}
inline Point2d	operator*(double s, Point2d const &b){return Point2d(s*b.x, s*b.y);}
inline bool	operator!=(Point2d const &a, Point2d const &b){return a.x!=b.x||a.y!=b.y;}
inline double abs(Point2d const &a){return sqrt(a.x*a.x+a.y*a.y);}
inline double dot(Point2d const &a, Point2d const &b){return a.x*b.x+a.y*b.y;}
inline bool collinear(Point2d const &a, Point2d const &b, Point2d const &c)
{
	const double tolerance=1e-6;
	return abs((c.x-b.x)*(b.y-a.y)-(b.x-a.x)*(c.y-b.y))<tolerance;
}
inline void		screen2image(int sx, int sy, int &ix, int &iy)
{
	ix=spx+shift(sx-(58*showtoolbox+3), -logzoom);
	iy=spy+shift(sy-5, -logzoom);
	//ix=spx+shift(sx-61, -logzoom);
	//iy=spy+shift(sy-5, -logzoom);
}
inline void		screen2image_rounded(int sx, int sy, int &ix, int &iy)
{
	int half=shift(1, logzoom-1);
	screen2image(sx+half, sy+half, ix, iy);
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
struct			Point
{
	int x, y;
	Point():x(0), y(0){}
	Point(int x, int y):x(x), y(y){}
	Point(Point2d const &p):x((int)std::round(p.x)), y((int)std::round(p.y)){}
	void set(int x, int y){this->x=x, this->y=y;}
	void setzero(){x=y=0;}
	bool is_inside(int bw, int bh)const{return (unsigned)x<(unsigned)bw&&(unsigned)y<(unsigned)bh;}
	Point& operator+=(Point const &p){x+=p.x, y+=p.y;return *this;}
	Point& operator-=(Point const &p){x-=p.x, y-=p.y;return *this;}
	void clamp(int x1, int x2, int y1, int y2)
	{
		x=::clamp(x1, x, x2);
		y=::clamp(y1, y, y2);
	}
	void constraint_TL_zero()
	{
		if(x<0)
			x=0;
		if(y<0)
			y=0;
	}
	void constraint_TL(Point const &TL)
	{
		if(x<TL.x)
			x=TL.x;
		if(y<TL.y)
			y=TL.y;
	}
	void constraint_BR(Point const &BR)
	{
		if(x>BR.x)
			x=BR.x;
		if(y>BR.y)
			y=BR.y;
	}
	void image2screen(){::image2screen(x, y, x, y);}
	Point image2screen_copy()
	{
		Point p;
		::image2screen(x, y, p.x, p.y);
		return p;
	}
	void screen2image(){::screen2image(x, y, x, y);}
	void screen2image_rounded(){::screen2image_rounded(x, y, x, y);}
};
inline Point2d::Point2d(Point const &p):x(p.x), y(p.y){}
inline Point2d&	Point2d::operator=(Point const &p){x=p.x, y=p.y;return *this;}
inline Point2d	operator*(Point const &p, double t){return Point2d(p.x*t, p.y*t);}
inline Point2d	operator*(double t, Point const &p){return Point2d(p.x*t, p.y*t);}
inline Point	operator-(Point const &a, Point const &b){return Point(a.x-b.x, a.y-b.y);}
inline bool		operator!=(Point const &a, Point const &b){return a.x!=b.x||a.y!=b.y;}
inline bool		operator==(Point const &a, Point const &b){return a.x==b.x||a.y==b.y;}
struct			Rect
{
	Point i, f;
	bool iszero()const{return i==f;}
	bool nonzero()const{return i!=f;}
	int dx()const{return f.x-i.x;}
	int dy()const{return f.y-i.y;}
	char mousetest_orderless(int mx, int my)const
	{
		int xhit, yhit;
		if(i.x<f.x)
			xhit=mx>=i.x&&mx<f.x;
		else
			xhit=mx>=f.x&&mx<i.x;
		if(i.y<f.y)
			yhit=my>=i.y&&my<f.y;
		else
			yhit=my>=f.y&&my<i.y;
		return xhit&&yhit;
	}
	char mousetest(int mx, int my, int padding=0)const//all points in screen coords
	{
		if(mx>=i.x&&mx<f.x&&my>=i.y&&my<f.y)
			return true;
		return (mx>=i.x-padding&&mx<f.x+padding&&my>=i.y-padding&&my<f.y+padding)<<1;
	}
	void set(int x1, int y1, int x2, int y2){i.set(x1, y1), f.set(x2, y2);}
	void get(int &x1, int &x2, int &y1, int &y2)const{x1=i.x, x2=f.x, y1=i.y, y2=f.y;}
	void set_mouse(int mx, int my)//start selecting with mouse, converted to image coords		TODO: check for usage/removal of this method
	{
		i.set(mx, my);
		i.screen2image();
		f=i;
	}
	void sort_coords()
	{
		if(i.x>f.x)
			std::swap(i.x, f.x);
		if(i.y>f.y)
			std::swap(i.y, f.y);
	}
	void sort_and_crop(Point &dimentions)
	{
		sort_coords();
		i.constraint_TL_zero();
		f.constraint_BR(dimentions);
	}
	void set_BR(Point const &dimentions){f=i+dimentions;}//prerequisite: coordinates must be sorted
	void image2screen()
	{
		i.image2screen();
		f.image2screen();
	}
	void screen2image()
	{
		i.screen2image();
		f.screen2image();
	}
	void screen2image_rounded()
	{
		i.screen2image_rounded();
		f.screen2image_rounded();
	}
	void win32_move_params(Point &pos, Point &size)const//TODO: check for usage/removal of this method
	{
		pos=i+Point(1, 1);
		size=f-i;
	}
};
/*struct			Selection
{
	Rect image, screen;//selection dimensions in image & screen coordinates
	Point ipos;//position of top-right corner of selection in image coordinates
	bool exists()const{return image.nonzero();}
	void assign()//fills screen coords & ipos; using image coords		TODO: rename assign -> update_screen_and_ipos
	{
		screen=image;
		screen.sort_and_crop(Point(iw, ih));
		ipos=screen.i;
		screen.image2screen();
	}
	void setzero()
	{
		memset(this, 0, sizeof *this);//this struct is packed
	}
};//*/


//history
void			mark_modified();
struct			HistoryFrame
{
	int *buffer;
	int iw, ih;
	HistoryFrame():buffer(nullptr), iw(0), ih(0){}
	HistoryFrame(int *buffer, int iw, int ih):buffer(buffer), iw(iw), ih(ih){}
};
extern int		histpos;
extern std::vector<HistoryFrame> history;
int*			hist_start(int iw, int ih);
void			hist_premodify(int *&buffer, int nw, int nh);
void			hist_postmodify(int *buffer, int nw, int nh);
void			hist_undo(int *&buffer);
void			hist_redo(int *&buffer);

//color
inline int		swap_rb(int color)
{
	auto p=(unsigned char*)&color;
	auto temp=p[0];
	p[0]=p[2], p[2]=temp;
	return color;
}
void			swap_rb(int *dstimage, const int *srcimage, int pxcount);
void			invertcolor(int *buffer, int bw, int bh, bool selection=false);
void			clear_alpha();

void			raw2float();


//globals
extern int		primarycolor, secondarycolor,//0xAARRGGBB
				primary_alpha, secondary_alpha;
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
extern int		currentmode, prevmode,
				selection_free,//true: free-form selection, false: rectangular selection
				selection_transparency,//2: opaque or 1: transparent
				selection_persistent,//selection remains after switching to another tool
				erasertype,
				magnifier_level,//log pixel size {0:1, 1:2, 2:4, 3:8, 4:16, 5:32}
				brushtype,
				airbrush_size, airbrush_cpt, airbrush_timer,
				linewidth,//[1, 5]
				rectangle_type, polygon_type, ellipse_type, roundrect_type,
				interpolation_type;

//selection
extern const char anchors[];
extern Rect		selection,//image coordinates, i: buffer start, f: buffer end (not in order)			//TODO: rename to selrect
				sel0;//stores original dimensiond during resizing
//extern Point	sel_start, sel_end,//selection (start: fixed, end: moving), image coordinates
//				sel_s1, sel_s2,//ordered screen coordinates for mouse test
//				selpos,//position of top-right corner of selection in image coordinates
//				selid;//selection screen dimensions
//extern bool	selection_moved;

extern Rect		textrect;//image coordinates, sorted
//extern Point	text_start, text_end,//static point -> moving point, image coordinates
//				text_s1, text_s2,//ordered screen coordinates
//				textpos;//position of top-right corner of textbox in image coordinates

int				assign_using_anchor(char anchor, int &start, int &end, int mouse, int prevmouse);
void			selection_resize_mouse(Rect &sel, int mx, int my, int prev_mx, int prev_my, int *xflip=nullptr, int *yflip=nullptr);
void			selection_sort(Point const &sel_start, Point const &sel_end, Point &p1, Point &p2);
void			selection_sortNbound(Point const &sel_start, Point const &sel_end, Point &p1, Point &p2);
void			selection_assign_mouse(int start_mx, int start_my, int mx, int my, Point &sel_start, Point &sel_end, Point &p1, Point &p2);
//void			selection_assign();
void			selection_remove();
void			selection_select(Rect const &sel);
void			selection_stamp(bool modify_hist);
void			selection_move_mouse(int prev_mx, int prev_my, int mx, int my);
void			selection_selectall();
void			selection_resetscale();

extern std::vector<Point> freesel;//free form selection vertices
void			freesel_add_mouse(int mx, int my);
void			freesel_select();

bool			editcopy();
void			editpaste();

//transform
enum			FlipRotate{FLIP_HORIZONTAL, FLIP_VERTICAL, ROTATE90, ROTATE180, ROTATE270, ROTATE_ARBITRARY};
void			fliprotate(int *buffer, int bw, int bh, int *&b2, int &w2, int &h2, int fr, int bk);
void			fliprotate(int fr=ROTATE_ARBITRARY);
void			stretchskew(int *buffer, int bw, int bh, int *&b2, int &w2, int &h2, double stretchH, double stretchV, double skewH, double skewV, int bk);
void			stretchskew();

//fill
typedef void (*XFillCallback)(void *data, int x1, int x2, int y);
char*			fill_v2_init(int *buffer, int bw, int bh, int x0, int y0);//use free() to free the returned mask
void			fill_v2(int *buffer, int bw, int bh, int x0, int y0, XFillCallback cb, void *data, char *mask);
void			fill(int *buffer, int bw, int bh, int x0, int y0, int color);
void			fill_mouse(int *buffer, int mx0, int my0, int color);
void			airbrush(int *buffer, int x0, int y0, int color);
void			airbrush_mouse(int *buffer, int mx0, int my0, int color);

void			draw_line_v3		(int *buffer, int bw, int bh, int x1, int y1, int x2, int y2, int color);
//typedef __m128i (*FillCallback)(__m128i const &kx, __m128i const &ky, void *params, int idx, int count);
void			fill_convex_POT	(int *buffer, int bw, int bh, Point const *p, int nv, int nvmask, XFillCallback callback, void *params);//uses AND instead of MOD
void			fill_convex		(int *buffer, int bw, int bh, Point const *points, int nv, XFillCallback callback, void *params);

void			draw_gradient(int *buffer, int bw, int bh, int c1, double x1, double y1, double x2, double y2, int c2, bool startOver);

//lines
void			draw_line_v2		(int *buffer, int bw, int bh, int x1, int y1, int x2, int y2, int color);//int Bresenham
void			draw_line_v2_invert	(int *buffer, int bw, int bh, int x1, int y1, int x2, int y2, int *imask);
void			draw_line_aa_v2(int *buffer, int bw, int bh, double x1, double y1, double x2, double y2, double linewidth, int color);//uses a callback per 4 pixels
void			draw_h_line(int *buffer, int x1, int x2, int y, int color);
void			draw_v_line(int *buffer, int x, int y1, int y2, int color);
void			draw_line_brush(int *buffer, int bw, int bh, int brush, int x1, int y1, int x2, int y2, int color, bool invert_color=false, int *imask=nullptr);//invert_color ignores color argument & uses imask which has same dimensions as buffer
//enum			DrawLineMode
//{
//	DL_DISABLE_GRADIENT,
//	DL_ENABLE_GRADIENT_STARTOVER,
//	DL_ENABLE_GRADIENT_CONTINUE,
//};
extern bool		draw_line_frame1;
void			draw_line_mouse(int *buffer, int mx1, int my1, int mx2, int my2, int color, int c2grad, bool enable_gradient);
void			draw_line_brush_mouse(int *buffer, int brush, int mx1, int my1, int mx2, int my2, int color);

extern std::vector<Point> bezier;
void			curve_add_mouse(int mx, int my);
void			curve_update_mouse(int mx, int my);
void			curve_draw(int *buffer, int color);

//shapes
void			draw_rectangle(int *buffer, int x1, int x2, int y1, int y2, int color, int color2);
void			draw_rectangle_mouse(int *buffer, int mx1, int mx2, int my1, int my2, int color, int color2);

extern std::vector<Point> polygon;//image coordinates
extern bool		polygon_leftbutton;
void			polygon_add_mouse(int start_mx, int start_my, int mx, int my);
void			polygon_bounds(std::vector<Point> const &polygon, int coord_idx, int &start, int &end, int clipstart, int clipend);
void			polygon_rowbounds(std::vector<Point> const &polygon, int ky, std::vector<double> &bounds);
void			polygon_draw(int *buffer, int color_line, int color_fill);

void			draw_ellipse_mouse(int *buffer, int mx1, int mx2, int my1, int my2, int color, int color2);

void			draw_roundrect_mouse(int *buffer, int mx1, int mx2, int my1, int my2, int color, int color2);

//text
void			move_textbox();
void			resize_textgui();
void			set_sizescombobox(int font_idx);
void			sample_font(int &font_idx, int &font_size);
void			remove_textbox();
void			exit_textmode();
void			change_font(int font_idx, int font_size);
void			clear_textbox();
void			print_text();
extern WNDPROC	oldEditProc;
long			__stdcall EditProc(HWND__ *hWnd, unsigned message, unsigned wParam, long lParam);
int				__stdcall FontProc(LOGFONTW const *lf, TEXTMETRICW const *tm, unsigned long FontType, long lParam);

//image processing
void			center_bright_object();
void			simple_average_stacker();
void			stack_sky_images();


//paint++ GUI
enum			ColorbarShow{CS_NONE, CS_TEXTOPTIONS, CS_FLIPROTATE, CS_STRETCHSKEW};
extern int		colorbarcontents;//additional options in color bar
void			rectangle(int x1, int x2, int y1, int y2, int color);
void			rect_checkboard(int x1, int x2, int y1, int y2, int color1, int color2);
void			h_line(int x1, int x2, int y, int color);
void			v_line(int x, int y1, int y2, int color);
void			h_line_add(int x1, int x2, int y, int ammount);
void			v_line_add(int x, int y1, int y2, int ammount);
void			h_line_alt(int x1, int x2, int y, int *colors);
void			v_line_alt(int x, int y1, int y2, int *colors);
void			rectangle_hollow(int x1, int x2, int y1, int y2, int color);
void			line45_backslash(int x1, int y1, int n, int color);
void			line45_forwardslash(int x1, int y1, int n, int color);
void			_3d_hole(int x1, int x2, int y1, int y2);
void			_3d_border(int x1, int x2, int y1, int y2);
void			scrollbar_slider(int &winpos_ip, int imsize_ip, int winsize, int barsize, double zoom, short &sliderstart, short &slidersize);
void			scrollbar_scroll(int &winpos_ip, int imsize_ip, int winsize, int barsize, int slider0, int mp_slider0, int mp_slider, double zoom, short &sliderstart, short &slidersize);

void			draw_icon(int *icon, int x, int y);
void			draw_icon_monochrome(const int *ibuffer, int dx, int dy, int xpos, int ypos, int color);
void			draw_icon_monochrome_transposed(const int *ibuffer, int dx, int dy, int xpos, int ypos, int color);

void			draw_vscrollbar(int x, int sbwidth, int y1, int y2, int &winpos, int content_h, int vscroll_s0, int vscroll_my_start, double zoom, short &vscroll_start, short &vscroll_size, int dragtype);
void			draw_selection_rectangle(Rect irect, int x1, int x2, int y1, int y2, int padding);

void			movewindow_c(HWND hWnd, int x, int y, int w, int h, bool repaint);

void			fliprotate_moveui(bool repaint);//move to ppp_transform.cpp
void			stretchskew_moveui(bool repaint);
void			show_colorbarcontents(bool show);
void			replace_colorbarcontents(int contents);
void			blend_with_checkboard_zoomed(const int *buffer, int x0, int y0, int x1, int x2, int y1, int y2, short cbx0, short cby0);

void			stamp_mask(const int *mask, char mw, char mh, char upsidedown, char transposed, short x1, short x2, short y1, short y2, int color);
void			v_line_comp_dots(int x, int y1, int y2, Point const &cTL, Point const &cBR);
void			h_line_comp_dots(int x1, int x2, int y, Point const &cTL, Point const &cBR);

//i0: start on image, id: extent in image, s0: imagewindow start on screen, s1 & s2: draw start & end on screen
void			stretch_blit(const int *buffer, int bw, int bh,  Rect const &irect,  int sx0, int sy0,  int sx1, int sx2, int sy1, int sy2,  int *mask, int transparent, int bkcolor);//mask: pass nullptr for rectangular selection

void			display_raw(int *buffer, int bw, int bh, int x1, int x2, int y1, int y2);
//void			display_raw(int *buffer, int bw, int bh, int x1, int x2, int y1, int y2, int bitdepth);


//application
void			timer_set(int id);//TimerID
void			timer_kill();
enum			RedrawTypeEnum//use bitfields if c[4] is not enough
{
	REDRAW_ALL				=0xFFFFFFFF,
	REDRAW_IMAGEWINDOW		=0x00000007,//image + im_scrollbars
	REDRAW_IMAGE_SCROLL		=0x00000004,//just im_scrollbars (not for calling)
	REDRAW_IMAGE_NOSCROLL	=0x00000003,//image
	REDRAW_IMAGE_PARTIAL	=0x00000001,//part of image
	REDRAW_THUMBBOX			=0x00000100,
	REDRAW_COLORBAR			=0x00030000,//colorbar includes colorpalette
	REDRAW_COLORPALETTE		=0x00010000,
	REDRAW_TOOLBAR			=0x03000000,//toolbar includes toolbox
	REDRAW_TOOLBOX			=0x01000000,
};
int				ask_to_save();
void			render(int redrawtype, int rx1, int rx2, int ry1, int ry2);
#endif//PPP_H