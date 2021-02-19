#include		"ppp.h"
#include		"ppp_inline_check.h"
#include		<algorithm>//sort
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
					int kx=(int)std::round(bounds[kb]), kxEnd=(int)std::round(bounds[kb+1]);
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
				draw_line_brush(buffer, iw, ih, linewidth, p1.x, p1.y, p2.x, p2.y, color_line);
			}
		}
	}
	polygon.clear();
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
	p=(int)std::round(Ry2-Rx2*Ry+0.25*Rx2);
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
	p=(int)std::round(Ry2*(x+0.5)*(x+0.5)+Rx2*(y-1)*(y-1)-Rx2*Ry2);
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