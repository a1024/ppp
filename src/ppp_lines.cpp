#include		"ppp.h"
#include		"ppp_inline_check.h"
#include		"ppp_xmm_clamp.h"
#include		"generic.h"
#include		<algorithm>

//	#define			LINE_AA_SHOW_BOX

void			draw_h_line(int *buffer, int x1, int x2, int y, int color)//x2 exclusive
{
	if(!ichecky(y))
	{
		x1=maximum(0, x1), x2=minimum(x2, iw);
		memfill(buffer+iw*y+x1, &color, (x2-x1)<<2, 1<<2);
		//for(int kx=x1;kx<x2;++kx)
		//	buffer[iw*y+kx]=color;
	}
}
void			draw_v_line(int *buffer, int x, int y1, int y2, int color)//y2 exclusive
{
	if(!icheckx(x))
	{
		y1=maximum(0, y1), y2=minimum(y2, ih);
		for(int ky=y1;ky<y2;++ky)
			buffer[iw*ky+x]=color;
	}
}

inline bool		clip_line_boundary(int p, int q, double &t0, double &t1)
{
	if(!p)//parallel to boundary
	{
		if(q<0)
			return false;
	}
	else
	{
		double u=(double)q/p;
		if(p<0)//outside to inside
		{
			if(t1<u)
				return false;
			if(t0<u)
				t0=u;
		}
		else//p>0	inside to outside
		{
			if(t0>u)
				return false;
			if(t1>u)
				t1=u;
		}
	}
	return true;
}
bool			clip_line(int bw, int bh, int &x1, int &y1, int &x2, int &y2)
{//Liang-Barsky line clipping algorithm
	int dx=x2-x1, dy=y2-y1;
	double t0=0, t1=1;
	if(!clip_line_boundary(-dx, -(0-x1), t0, t1))//left
		return false;
	if(!clip_line_boundary( dx,  bw-1-x1 , t0, t1))//right
		return false;
	if(!clip_line_boundary(-dy, -(0-y1), t0, t1))//bottom
		return false;
	if(!clip_line_boundary( dy,  bh-1-y1 , t0, t1))//top
		return false;
	int xa=x1, ya=y1;
	x1=xa+(int)floor(t0*dx+0.5), y1=ya+(int)floor(t0*dy+0.5);
	x2=xa+(int)floor(t1*dx+0.5), y2=ya+(int)floor(t1*dy+0.5);
	return true;
}
bool			clip_line_y_only(int bw, int bh, int &x1, int &y1, int &x2, int &y2)
{//Liang-Barsky line clipping algorithm
	int dx=x2-x1, dy=y2-y1;
	double t0=0, t1=1;
	//if(!clip_line_boundary(-dx, -(0-x1), t0, t1))//left
	//	return false;
	//if(!clip_line_boundary( dx,  bw-1-x1 , t0, t1))//right
	//	return false;
	if(!clip_line_boundary(-dy, -(0-y1), t0, t1))//bottom
		return false;
	if(!clip_line_boundary( dy,  bh-1-y1 , t0, t1))//top
		return false;
	int xa=x1, ya=y1;
	x1=xa+(int)floor(t0*dx+0.5), y1=ya+(int)floor(t0*dy+0.5);
	x2=xa+(int)floor(t1*dx+0.5), y2=ya+(int)floor(t1*dy+0.5);
	return true;
}
void			draw_line_v2(int *buffer, int bw, int bh, int x1, int y1, int x2, int y2, int color)
{//Bresenham's line algorithm (pure integers)	https://web.archive.org/web/20160311235624/http://freespace.virgin.net/hugo.elias/graphics/x_lines.htm
	//int x1_0=x1, y1_0=y1, x2_0=x2, y2_0=y2;
	if(!clip_line(bw, bh, x1, y1, x2, y2))
		return;
#if 1//24~26ms
	int dx=abs(x2-x1), dy=abs(y2-y1), xa, ya, xb, yb;
	int error, inc, cmp;
	if(dx>=dy)//horizontal & 45 degrees
	{
		error=dx>>1;//initialize yerror fraction with half the denominator
		if(x1<x2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		inc=ya<=yb?1:-1;
		for(int kx=xa, ky=ya;kx<=xb;++kx)
		{
			//if(kx<0||kx>=bw||ky<0||ky>=bh)//
			//	return;//

			buffer[bw*ky+kx]=color;

			error+=dy;//add slope to fraction 'yerror'
			cmp=-(error>=dx);	//if fraction >1 then:
			ky+=inc&cmp;		//	...increment y
			error-=dx&cmp;		//	...and subtract 1 from yerror fraction
		}
	}
	else//vertical & 45 degrees (steep)
	{
		error=dy>>1;//initialize xerror fraction with half the denominator
		if(y1<y2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		inc=xa<=xb?1:-1;
		for(int ky=ya, kx=xa;ky<=yb;++ky)
		{
			//if(kx<0||kx>=bw||ky<0||ky>=bh)//
			//	return;//

			buffer[bw*ky+kx]=color;

			error+=dx;//add invslope to fraction 'xerror'
			cmp=-(error>=dy);	//if fraction >1 then:
			kx+=inc&cmp;		//	...increment x
			error-=dy&cmp;		//	...and subtract 1 from xerror fraction
		}
	}
#endif
#if 0//66~70ms		dual function Bresenham
	int dx=abs(x2-x1), dy=abs(y2-y1), xa, ya, xb, yb;
	int &yslopenum=dy, &yslopeden=dx, yerror=yslopeden>>1;
	int &xslopenum=dx, &xslopeden=dy, xerror=xslopeden>>1;
	if(x1<=x2)//sort by ascending x
		xa=x1, ya=y1, xb=x2, yb=y2;
	else
		xa=x2, ya=y2, xb=x1, yb=y1;
	int ymin, ymax, yinc, errinc=maximum(1, minimum(dx, dy));
	if(ya<yb)
		ymin=ya, ymax=yb, yinc=1;
	else if(yb<ya)
		ymin=yb, ymax=ya, yinc=-1;
	else
		ymin=ya, ymax=yb, yinc=0;
	for(int kx=xa, ky=ya;kx<=xb&&ky>=ymin&&ky<=ymax;)
	{
		if(kx<0||kx>=bw||ky<0||ky>=bh)//
			return;//

		buffer[bw*ky+kx]=color;

		yerror+=errinc;//add advance to fraction 'yerror'
		int cmp=-(yerror>=yslopeden);	//if fraction >1 then:
		ky+=yinc&cmp;					//	...increment y
		yerror-=yslopeden&cmp;			//	...and subtract 1 from yerror fraction

		xerror+=errinc;//add advance to fraction 'xerror'
		cmp=-(xerror>=xslopeden);		//if fraction >1 then:
		kx+=1&cmp;						//	...increment x
		xerror-=xslopeden&cmp;			//	...and subtract 1 from xerror fraction
	}
#endif
#if 0//66~81ms		Bresenham with pointer transpose
	int dx=abs(x2-x1), dy=abs(y2-y1), xa, ya, xb, yb;
	int error, inc, cmp, kx, ky;
	int *pkx, *pky, *pdx, *pdy,
		*px1, *px2, *py1, *py2;
	if(dx>=dy)//horizontal & 45 degrees
		pkx=&kx, pdx=&dx,	pky=&ky, pdy=&dy,	px1=&x1, px2=&x2,	py1=&y1, py2=&y2;
	else//vertical (steep) (transposed)
		pkx=&ky, pdx=&dy,	pky=&kx, pdy=&dx,	px1=&y1, px2=&y2,	py1=&x1, py2=&x2;
	error=*pdx>>1;//initialize error fraction with half the denominator
	if(*px1<*px2)
		xa=*px1, ya=*py1, xb=*px2, yb=*py2;
	else
		xa=*px2, ya=*py2, xb=*px1, yb=*py1;
	inc=ya<=yb?1:-1;
	for(*pkx=xa, *pky=ya;*pkx<=xb;++*pkx)
	{
		if(kx<0||kx>=bw||ky<0||ky>=bh)//
			return;//

		buffer[bw*ky+kx]=color;

		error+=*pdy;//add slope to fraction 'yerror'
		cmp=-(error>=*pdx);	//if fraction >1 then:
		*pky+=inc&cmp;		//	...increment y
		error-=*pdx&cmp;	//	...and subtract 1 from yerror fraction
	}
#endif
#if 0//137~149ms	naive int		raw: 42~60ms
	int xa, ya, xb, yb;
	if(y1<y2)
		xa=x1, ya=y1, xb=x2, yb=y2;
	else
		xa=x2, ya=y2, xb=x1, yb=y1;
	int dx=xb-xa, dy=yb-ya;//dy is always positive
	if(!dy)
	{
		memfill(buffer+bw*ya+minimum(xa, xb), &color, (abs(xb-xa)+1)<<2, 1<<2);
		return;
	}
	int xstart, xend;
	if(xa<xb)
		xstart=xa-1, xend=xb+1;
	//	xstart=xa, xend=xb;
	else if(xb<xa)
		xstart=xb-1, xend=xa+1;
	//	xstart=xb, xend=xa;
	else
		xstart=xa, xend=xb;
	int horizontal=abs(dx)>dy;
	dx+=((dx>0)-(dx<0))&-horizontal;//add sgn to deltas if <45 degrees
	dy+=horizontal;
	int halfden=dy;
	dx<<=1, dy<<=1;//double num & den
	int invden=(1<<16)/dy;//dy < 2^16
	int negative=-(dx<0);
	//int sdx=(dx>0)-(dx<0);
	for(int ky=ya, kx=xa, num=(ky+1-ya)*dx+halfden, num2=abs(num)*invden, num2step=abs(dx)*invden, kx2;ky<=yb;++ky, num+=dx, num2+=num2step)
	{
		//int num=(ky+1-ya)*dx+halfden;//next kx = line(next ky)

		//kx2=xa+(negative^((short*)&num2)[1])-negative-(num<0);//raw: 45~50ms		128~140ms
		kx2=xa+(negative^num2>>16)-negative-(num<0);			//raw: 42~60ms
		//kx2=xa+sdx*(num2>>16)-(num<0);//124~147ms
		//kx2=xa+(num*invden>>16)-(num<0);//125~135ms
		//kx2=xa+num/dy-(num<0);//round(num/dy) = floor((num+dy/2)/dy) = trunc((num+dy/2)/dy)-(num+dy/2<0)		raw: 37, 44~50ms
		if(kx2<xstart||kx2>xend)
		{
			if(kx2<xstart)
				kx2=xstart;
			if(kx2>xend)
				kx2=xend;
			memfill(buffer+bw*ky+minimum(kx, kx2)+(kx2<kx), &color, (abs(kx2-kx)+(kx==kx2))<<2, 1<<2);
			return;
		}
		memfill(buffer+bw*ky+minimum(kx, kx2)+(kx2<kx), &color, (abs(kx2-kx)+(kx==kx2))<<2, 1<<2);
		//buffer[bw*ky+kx]=color;//
		kx=kx2;
	}
#endif
#if 0//111~130ms	in-loop check		raw: 43~61ms
	int xa, ya, xb, yb;
	if(y1<y2)
		xa=x1, ya=y1, xb=x2, yb=y2;
	else
		xa=x2, ya=y2, xb=x1, yb=y1;
	int dx=abs(xb-xa), dy=yb-ya, xinc;
	if(!dy)//horizontal
	{
		memfill(buffer+bw*ya+minimum(xa, xb), &color, (abs(xb-xa)+1)<<2, 1<<2);
		return;
	}
	int xstart, xend;
	if(xa<xb)
		xstart=xa-1, xend=xb+1, xinc=1;
	else if(xb<xa)
		xstart=xb-1, xend=xa+1, xinc=-1;
	else
		xstart=xa, xend=xb, xinc=0;
	int error=maximum(dx, dy)>>1, diff;
	int inc=xa<=xb?1:-1, cmp;
	for(int ky=ya, kx=xa, kx2;ky<=yb;++ky)
	{
		if(dx>dy)//draws horizontal lines only
		{
			diff=dx-error;
			diff=diff/dy+(diff%dy!=0);
			kx2=kx+xinc*diff;
			if(kx2<xstart||kx2>xend)
			{
				if(kx2<xstart)
					kx2=xstart;
				if(kx2>xend)
					kx2=xend;
				memfill(buffer+bw*ky+minimum(kx, kx2)+(kx2<kx), &color, (abs(kx2-kx)+(kx==kx2))<<2, 1<<2);
				return;
			}
			//memfill(buffer+bw*ky+minimum(kx, kx2)+(kx2<kx), &color, (abs(kx2-kx)+(kx==kx2))<<2, 1<<2);
			buffer[bw*ky+kx]=color;//raw: 43~61ms
			error+=dy*diff-dx;
			kx=kx2;
		}
		else//vertical & 45 degrees (steep)
		{
			buffer[bw*ky+kx]=color;

			error+=dx;//add invslope to fraction 'xerror'
			cmp=-(error>=dy);	//if fraction >1 then:
			kx+=inc&cmp;		//	...increment x
			error-=dy&cmp;		//	...and subtract 1 from xerror fraction
		}
	}
#endif
#if 0//199~219ms	naive float
	int xa, ya, xb, yb;
	if(y1<y2)
		xa=x1, ya=y1, xb=x2, yb=y2;
	else
		xa=x2, ya=y2, xb=x1, yb=y1;
	int dx=xb-xa, dy=yb-ya;//dy is always positive
	if(!dy)
	{
		memfill(buffer+bw*ya+minimum(xa, xb), &color, (abs(xb-xa)+1)<<2, 1<<2);
		return;
	}
	int xstart, xend;
	if(xa<xb)
		xstart=xa-1, xend=xb+1;
	//	xstart=xa, xend=xb;
	else if(xb<xa)
		xstart=xb-1, xend=xa+1;
	//	xstart=xb, xend=xa;
	else
		xstart=xa, xend=xb;
	int horizontal=abs(dx)>dy;
	dx+=(dx>0)-(dx<0)&-horizontal;//add sgn to deltas if <45 degrees
	dy+=horizontal;
	//dx=(dx+(dx>0)-(dx<0))<<1;//slope=(dx+sgn(dx))/(dy+1) because end is inclusive
	//dy=(dy+1)<<1;//double num & den
	double slope=(double)dx/dy;
	double LOL_1=slope+0.5;
	for(int ky=ya, kx=xa, kx2;ky<=yb;++ky, LOL_1+=slope)
	{
		//double LOL_2=xa+(ky+1-ya)*slope;//
		kx2=xa+(int)floor(LOL_1);//raw: 136~156ms
		//kx2=xa+(int)floor((ky+1-ya)*slope+0.5);//raw: 159~170ms
		if(kx2<xstart||kx2>xend)
		{
			if(kx2<xstart)
				kx2=xstart;
			if(kx2>xend)
				kx2=xend;
			memfill(buffer+bw*ky+minimum(kx, kx2)+(kx2<kx), &color, (abs(kx2-kx)+(kx==kx2))<<2, 1<<2);
			return;
		}
		//memfill(buffer+bw*ky+minimum(kx, kx2)+(kx2<kx), &color, (abs(kx2-kx)+(kx==kx2))<<2, 1<<2);
		buffer[bw*ky+kx]=color;//
		kx=kx2;
	}
#endif
#if 0//naive int (complicated broken) X
	int xa, ya, xb, yb;
	if(y1<y2)
		xa=x1, ya=y1, xb=x2, yb=y2;
	else
		xa=x2, ya=y2, xb=x1, yb=y1;
	int dx=xb-xa, dy=yb-ya;//dy is always positive
	if(!dy)
	{
		memfill(buffer+bw*ya+minimum(xa, xb), &color, (abs(xb-xa)+1)<<2, 1<<2);
		return;
	}
	int xstart, xend;
	if(xa<xb)
		xstart=xa-1, xend=xb+1;
	//	xstart=xa, xend=xb;
	else if(xb<xa)
		xstart=xb-1, xend=xa+1;
	//	xstart=xb, xend=xa;
	else
		xstart=xa, xend=xb;
	int halfden=dy+1;
	//int halfden=dy;
	//int halfdx=dx;
	//dx<<=1, dy<<=1;//double num & den
	dx=(dx+(dx>0)-(dx<0))<<1;//slope=(dx+sgn(dx))/(dy+1) because end is inclusive
	dy=(dy+1)<<1;//double num & den
	for(int ky=ya, kx=xa, kx2, kx2_0=0x80000000;ky<=yb;++ky)
	//for(int ky=ya, kx=xa, kx2, count=0;ky<=yb;++ky, ++count)
	//for(int ky=ya, kx=xa, kxav=kx, kx2, kxav2;ky<=yb;++ky)
	{
		//int num=(ky-ya)*dx+halfdx+halfden;//kxav = line(ky+0.5)
		int num=(ky+1-ya)*dx+halfden;//next kx = line(next ky)
		kx2=xa+num/dy-(num<0);//round(num/dy) = floor((num+dy/2)/dy) = trunc((num+dy/2)/dy)-(num+dy/2<0)
		if(kx2_0==0x80000000)
			kx2_0=kx2;
		kx+=kx2-kx2_0, kx2_0=kx2;
		//kx+=(kx2>kx)-(kx2<kx);
		if(kx2<xstart||kx2>xend)
		{
			if(kx2<xstart)
				kx2=xstart;
			if(kx2>xend)
				kx2=xend;
			//memfill(buffer+bw*ky+minimum(kx, kx2)+((kx<kx2)|(kx2<kx)), &color, (abs(kx2-kx)+(kx==kx2))<<2, 1<<2);
			//memfill(buffer+bw*ky+minimum(kxav, kx2)+(kx2<kxav), &color, (abs(kx2-kxav)+(kxav==kx2))<<2, 1<<2);
			memfill(buffer+bw*ky+minimum(kx, kx2)+(kx2<kx), &color, (abs(kx2-kx)+(kx==kx2))<<2, 1<<2);
			return;
		}
		//kxav2=(kx+kx2)>>1;
		//memfill(buffer+bw*ky+minimum(kx, kx2)+((kx<kx2)|(kx2<kx)), &color, (abs(kx2-kx)+(kx==kx2))<<2, 1<<2);
		//memfill(buffer+bw*ky+minimum(kx, kx2)+(kx<kx2), &color, (abs(kx2-kx)+(kx==kx2))<<2, 1<<2);
		//memfill(buffer+bw*ky+minimum(kxav, kxav2)+(kxav2<kxav), &color, (abs(kxav2-kxav)+(kxav==kxav2))<<2, 1<<2);
		memfill(buffer+bw*ky+minimum(kx, kx2)+(kx2<kx), &color, (abs(kx2-kx)+(kx==kx2))<<2, 1<<2);
		//if(kx2_0!=kx2)
		//	kx+=kx2-kx2_0, kx2_0=kx2;
		//kx=kx2-(((kx2>kx)-(kx2<kx))&-(count==0));
		//kx=kx2-((kx2>kx)-(kx2<kx));
		//kx=kx2;
		//kxav=kxav2;
	}
#endif
#if 0//old ppp naive (steep-only) X
	int xa, ya, xb, yb;
	if(y1<y2)
		xa=x1, ya=y1, xb=x2, yb=y2;
	else
		xa=x2, ya=y2, xb=x1, yb=y1;
	int ystart=maximum(ya, 0), yend=minimum(yb, ih-1);
	double r=double(xb-xa)/(yb-ya);
	for(double y=ystart;y<=yend;++y)
	{
		double x=xa+(y-ya)*r;
		if(x>=0&&x<iw)
		{
			int ix=(int)std::round(x), iy=(int)std::round(y);
			if(icheck(ix, iy))//
				continue;//
			int idx=bw*iy+ix;
			buffer[idx]=color;
		}
	}
#endif
}
void			draw_line_v2_invert(int *buffer, int bw, int bh, int x1, int y1, int x2, int y2, int *imask)//imask makes sure pixel is negated once
{
	if(!clip_line(bw, bh, x1, y1, x2, y2))
		return;
	int dx=abs(x2-x1), dy=abs(y2-y1), xa, ya, xb, yb;
	int error=maximum(dx, dy)>>1, inc, cmp;
	if(dx>=dy)//horizontal
	{
		if(x1<x2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		inc=ya<=yb?1:-1;
		for(int kx=xa, ky=ya;kx<=xb;++kx)
		{
			if(kx<0||kx>=bw||ky<0||ky>=bh)//
				return;//

			int idx=bw*ky+kx;
			auto &m=imask[idx];
			if(!m)
			{
				m=-1;
				auto &c=buffer[idx];
				c=c&0xFF000000|~c&0x00FFFFFF;
			}

			error+=dy;
			cmp=-(error>=dx);
			ky+=inc&cmp;
			error-=dx&cmp;
		}
	}
	else//vertical & 45 degrees (steep)
	{
		if(y1<y2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		inc=xa<=xb?1:-1;
		for(int ky=ya, kx=xa;ky<=yb;++ky)
		{
			if(kx<0||kx>=bw||ky<0||ky>=bh)//
				return;//

			int idx=bw*ky+kx;
			auto &m=imask[idx];
			if(!m)
			{
				m=-1;
				auto &c=buffer[idx];
				c=c&0xFF000000|~c&0x00FFFFFF;
			}

			error+=dx;
			cmp=-(error>=dy);
			kx+=inc&cmp;
			error-=dy&cmp;
		}
	}
}

//draw_line_v1
#if 0
__forceinline void blendpixel(int *buffer, int bw, int x, int y, int color, double alpha)
{
	auto src=(unsigned char*)&color, dst=(unsigned char*)(buffer+bw*y+x);
	dst[0]=src[0]+(int)(alpha*(dst[0]-src[0]));
	dst[1]=src[1]+(int)(alpha*(dst[1]-src[1]));
	dst[2]=src[2]+(int)(alpha*(dst[2]-src[2]));
}
void			draw_line_aa(int *buffer, int bw, int bh, double x1, double y1, double x2, double y2, int color)
{//Xiaolin Wu's antialiased line algorithm		https://web.archive.org/web/20160408133525/http://freespace.virgin.net/hugo.elias/graphics/x_wuline.htm
	if(!clip_line(bw, bh, x1, y1, x2, y2))
		return;
	double dx=x2-x1, dy=y2-y1, xa, ya, xb, yb, slope;
	if(abs(dx)>=abs(dy))//horizontal & 45 degrees
	{
		if(x1<x2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1, dx=-dx, dy=-dy;
		slope=dy/dx;

		//end point a
		double xend=floor(xa+0.5), yend=ya+slope*(xend-xa);//nearest int x & corresponding y
		double xgap=xa+0.5;
		xgap=1-(xgap-floor(xgap));
		int ix1=(int)xend, iy1=(int)yend;
		double brightness1=(yend-floor(yend))*xgap;
		double brightness2=xgap-brightness1;
		//blendpixel(buffer, bw, ix1, iy1  , color, brightness1);
		//blendpixel(buffer, bw, ix1, iy1+1, color, brightness2);
		double yf=yend+slope;

		//end point b
		xend=floor(xb+0.5), yend=yb+slope*(xend-x2);
		xgap=x2-0.5;
		xgap=1-(xgap-floor(xgap));
		int ix2=(int)xend, iy2=(int)yend;
		brightness1=(yend-floor(yend))*xgap;
		brightness2=xgap-brightness1;
		//blendpixel(buffer, bw, ix2, iy2  , color, brightness1);
		//blendpixel(buffer, bw, ix2, iy2+1, color, brightness2);

		for(int kx=ix1;kx<=ix2;++kx)
	//	for(int kx=ix1+1;kx<ix2;++kx)
		{
			brightness1=yf-floor(yf);
			brightness2=1-brightness1;
			blendpixel(buffer, bw, kx, (int)yf  , color, brightness1);
			blendpixel(buffer, bw, kx, (int)yf+1, color, brightness2);
			yf+=slope;
		}
	}
	else//vertical: transposed
	{
		if(y1<y2)
			xa=y1, ya=x1, xb=y2, yb=x2;
		else
			xa=y2, ya=x2, xb=y1, yb=x1, dx=-dx, dy=-dy;
		slope=dx/dy;

		//end point a
		double xend=floor(xa+0.5), yend=ya+slope*(xend-xa);
		double xgap=xa+0.5;
		xgap=1-(xgap-floor(xgap));
		int ix1=(int)xend, iy1=(int)yend;
		double brightness1=(yend-floor(yend))*xgap;
		double brightness2=xgap-brightness1;
		//blendpixel(buffer, bw, iy1  , ix1, color, brightness1);
		//blendpixel(buffer, bw, iy1+1, ix1, color, brightness2);
		double yf=yend+slope;

		//end point b
		xend=floor(xb+0.5), yend=yb+slope*(xend-x2);
		xgap=x2-0.5;
		xgap=1-(xgap-floor(xgap));
		int ix2=(int)xend, iy2=(int)yend;
		brightness1=(yend-floor(yend))*xgap;
		brightness2=xgap-brightness1;
		//blendpixel(buffer, bw, iy2  , ix2, color, brightness1);
		//blendpixel(buffer, bw, iy2+1, ix2, color, brightness2);

		for(int kx=ix1;kx<=ix2;++kx)
	//	for(int kx=ix1+1;kx<ix2;++kx)
		{
			brightness1=yf-floor(yf);
			brightness2=1-brightness1;
			blendpixel(buffer, bw, (int)yf  , kx, color, brightness1);
			blendpixel(buffer, bw, (int)yf+1, kx, color, brightness2);
			yf+=slope;
		}
	}
/*	int dx=x2-x1, dy=y2-y1, xa, ya, xb, yb;
	int fracmask=(1<<fracbits)-1, half=1<<(fracbits-1);
#define	TRUNCATE(x)		((x)>>fracbits)
#define	FRAC(x)			((x)&fracmask)
#define	DIVIDE(x)		//???
	if(abs(dx)>=abs(dy))//horizontal & 45 degrees
	{
		if(x1<x2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1, dx=-dx, dy=-dy;
		//end point 1
		int xend=TRUNCATE(xa+half);
		for(int kx=xa+1;kx<xb;++kx)
		{
		}
	}
	else//vertical
	{
	}//*/
}
void			draw_line(int *buffer, int bw, int bh, int x1, int y1, int x2, int y2, int color, bool invert_color, bool *imask=nullptr)
{
	if(x1<0&&x2<0||x1>=bw&&x2>=bw||y1<0&&y2<0||y1>=bh&&y2>=bh)
		return;
	double dx=x2-x1, dy=y2-y1, xa, ya, xb, yb;
	if(!dx&&!dy)
	{
		int ix1=(int)x1, iy1=(int)y1;
		if(ix1>=0&&ix1<bw&&iy1>=0&&iy1<bh)//TODO: history struct
		{
			int idx=bw*y1+ix1;
			auto &c=buffer[idx];
			if(invert_color)
			{
				auto &m=imask[idx];
				if(!m)
				{
					m=-1;
					c=c&0xFF000000|~c&0x00FFFFFF;
				}
			}
			else
				c=color;
		}
	}
	else if(abs(dx)>abs(dy))//horizontal
	{
		if(x1<x2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		int xstart=(int)maximum(xa, 0.), xend=(int)minimum(xb, (double)(bw-1));
		double r=(yb-ya)/(xb-xa);
		for(double x=xstart;x<=xend;++x)
		{
			double y=ya+(x-xa)*r;
			if(y>=0&&y<bh)
			{
				int ix=(int)std::round(x), iy=(int)std::round(y);
				if(icheck(ix, iy))//
					continue;//
				int idx=bw*iy+ix;
				auto &c=buffer[idx];
				if(invert_color)
				{
					auto &m=imask[idx];
					if(!m)
					{
						m=-1;
						c=c&0xFF000000|~c&0x00FFFFFF;
					}
				}
				else
					c=color;
			}
		}
	}
	else//vertical
	{
		if(y1<y2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		int ystart=(int)maximum(ya, 0.), yend=(int)minimum(yb, (double)bh-1);
		double r=(xb-xa)/(yb-ya);
		for(double y=ystart;y<=yend;++y)
		{
			double x=xa+(y-ya)*r;
			if(x>=0&&x<bw)
			{
				int ix=(int)std::round(x), iy=(int)std::round(y);
				if(icheck(ix, iy))//
					continue;//
				int idx=bw*iy+ix;
				auto &c=buffer[idx];
				if(invert_color)
				{
					auto &m=imask[idx];
					if(!m)
					{
						m=-1;
						c=c&0xFF000000|~c&0x00FFFFFF;
					}
				}
				else
					c=color;
			}
		}
	}
}
#endif

__declspec(deprecated) __forceinline void blendpixel_ct(int *buffer, int bw, int x, int y, int color, double ar, double ag, double ab)//unused
{
	auto src=(unsigned char*)&color, dst=(unsigned char*)(buffer+bw*y+x);
	dst[0]=dst[0]+(int)(ab*(src[0]-dst[0]));
	dst[1]=dst[1]+(int)(ag*(src[1]-dst[1]));
	dst[2]=dst[2]+(int)(ar*(src[2]-dst[2]));
}
//__forceinline double brightness_from_position(double px, double py, double x1, double y1, double x2, double y2, double dx, double dy, double d2)
//{
//	double ar=clamp01(((px-x1)*(px-x2)+(py-y1)*(py-y2))*d2);//parameter = clamp((p-p1) dot (p-p2) / length(p1, p2))
//	px-=x1+ar*dx;//projection on segment
//	py-=y1+ar*dy;
//	ar=sqrt(px*px+py*py);//distance to segment
//	ar=clamp01(aa_offset-ar);//alpha
//}
struct AAInfo
{
	__m128
		x1, y1,
		dx, dy,
		invd2, aa_offset,
		third, twothirds,
		red, green, blue;
	__m128i alpha;
	int *buffer, bw;
};
inline void		sse2_split_channels(__m128i const &colors, __m128 &red, __m128 &green, __m128 &blue, __m128i &alpha)
{
	__m128i t=_mm_and_si128(colors, m_channel_mask);
	red=_mm_cvtepi32_ps(t);

	__m128i c=_mm_srli_epi32(colors, 8);
	t=_mm_and_si128(c, m_channel_mask);
	green=_mm_cvtepi32_ps(t);

	c=_mm_srli_epi32(c, 8);
	t=_mm_and_si128(c, m_channel_mask);
	blue=_mm_cvtepi32_ps(t);
	
	alpha=_mm_and_si128(colors, m_alpha_mask);
	//alpha=_mm_srli_epi32(c, 8);

	//c=_mm_srli_epi32(c, 8);
	//t=_mm_and_si128(c, m_channel_mask);//
	//alpha=_mm_cvtepi32_ps(t);
}
__forceinline __m128 aa_line_gen_alpha(__m128 const &px, __m128 const &py, AAInfo *aai)
{
	//t = clamp01((p-p1) dot (p2-p1) / length(p1, p2)^2) = clamp01(((px-x1)*dx+(py-y1)*dy)*invd2)
	__m128 t1=_mm_sub_ps(px, aai->x1);
	t1=_mm_mul_ps(t1, aai->dx);
	__m128 t2=_mm_sub_ps(py, aai->y1);
	t2=_mm_mul_ps(t2, aai->dy);
	t1=_mm_add_ps(t1, t2);
	t1=_mm_mul_ps(t1, aai->invd2);
	t1=clamp01(t1);

	//prx=px-(x1+ar*dx), pry=py-(y1+ar*dy)	projection on segment
	__m128 prx=_mm_mul_ps(t1, aai->dx);
	__m128 pry=_mm_mul_ps(t1, aai->dy);
	prx=_mm_add_ps(prx, aai->x1);
	prx=_mm_sub_ps(px, prx);
	pry=_mm_add_ps(pry, aai->y1);
	pry=_mm_sub_ps(py, pry);

	//distance = sqrt(px*px+py*py)		distance to segment
	prx=_mm_mul_ps(prx, prx);
	pry=_mm_mul_ps(pry, pry);
	prx=_mm_add_ps(prx, pry);
	prx=_mm_sqrt_ps(prx);

	//alpha=clamp01(aa_offset-distance)
	prx=_mm_sub_ps(aai->aa_offset, prx);
	__m128 alpha=clamp01(prx);
	return alpha;
}
__forceinline __m128 sse2_mixchannel(__m128 const &dst, __m128 const &src, __m128 const &alpha)//dst, src: 0~255; alpha: 0~1
{
	//dst+(src-dst)*alpha
	__m128 temp=_mm_sub_ps(src, dst);
	temp=_mm_mul_ps(temp, alpha);
	temp=_mm_add_ps(temp, dst);
	return temp;
}
__forceinline __m128i cb_aa_line_px(__m128i const &dst, __m128 const &mkx, __m128 const &mky, void *params, int idx, int count)
{
	auto aai=(AAInfo*)params;
	//__m128i dst=_mm_loadu_si128((__m128i*)(aai->buffer+idx));
	__m128 d_red, d_green, d_blue;
	__m128i d_alpha;
	sse2_split_channels(dst, d_red, d_green, d_blue, d_alpha);//dst alpha is discarded
	
	__m128 px=mkx, py=mky;//pixel
	//__m128 px=_mm_cvtepi32_ps(kx), py=_mm_cvtepi32_ps(ky);
	__m128 a_red=aa_line_gen_alpha(px, py, aai);
	px=_mm_add_ps(px, aai->third);
	__m128 a_green=aa_line_gen_alpha(px, py, aai);
	px=_mm_add_ps(px, aai->third);
	__m128 a_blue=aa_line_gen_alpha(px, py, aai);

	d_red	=sse2_mixchannel(d_red,		aai->red,	a_red);
	d_green	=sse2_mixchannel(d_green,	aai->green,	a_green);
	d_blue	=sse2_mixchannel(d_blue,	aai->blue,	a_blue);
	
	//combine channels
	__m128i b32=_mm_cvtps_epi32(d_blue);
	__m128i g32=_mm_cvtps_epi32(d_green);
	__m128i r32=_mm_cvtps_epi32(d_red);
	g32=_mm_slli_epi32(g32, 8);
	r32=_mm_slli_epi32(r32, 16);

	b32=_mm_or_si128(b32, g32);
	b32=_mm_or_si128(b32, r32);
	b32=_mm_or_si128(b32, aai->alpha);
	return b32;
}
void			cb_aa_line(void *p, int x1, int x2, int y)
{
	auto aai=(AAInfo*)p;
	int xround=x2-x1, xrem=xround&3;
	xround&=~3;
	int idx=aai->bw*y+x1, *row=aai->buffer+idx;

	const __m128 four=_mm_set1_ps(4);
	__m128 py=_mm_set1_ps((float)y);
	__m128 px=_mm_set_ps((float)(x1+3), (float)(x1+2), (float)(x1+1), (float)x1);
	__m128i dst, res;
	for(int kx=0;kx<xround;kx+=4, idx+=4)
	{
		dst=_mm_loadu_si128((__m128i*)(aai->buffer+idx));
		res=cb_aa_line_px(dst, px, py, p, idx, 4);
	/*	__m128i dst=_mm_loadu_si128((__m128i*)(aai->buffer+idx));
		__m128 d_red, d_green, d_blue;
		__m128i d_alpha;
		sse2_split_channels(dst, d_red, d_green, d_blue, d_alpha);//dst alpha is discarded

		//__m128 px=_mm_cvtepi32_ps(kx), py=_mm_cvtepi32_ps(ky);//pixel
		__m128 a_red=aa_line_gen_alpha(px, py, aai);
		px=_mm_add_ps(px, aai->third);
		__m128 a_green=aa_line_gen_alpha(px, py, aai);
		px=_mm_add_ps(px, aai->third);
		__m128 a_blue=aa_line_gen_alpha(px, py, aai);

		d_red	=sse2_mixchannel(d_red,		aai->red,	a_red);
		d_green	=sse2_mixchannel(d_green,	aai->green,	a_green);
		d_blue	=sse2_mixchannel(d_blue,	aai->blue,	a_blue);
	
		//combine channels
		__m128i b32=_mm_cvtps_epi32(d_blue);
		__m128i g32=_mm_cvtps_epi32(d_green);
		__m128i r32=_mm_cvtps_epi32(d_red);
		g32=_mm_slli_epi32(g32, 8);
		r32=_mm_slli_epi32(r32, 16);

		b32=_mm_or_si128(b32, g32);
		b32=_mm_or_si128(b32, r32);
		b32=_mm_or_si128(b32, aai->alpha);//*/
		
		_mm_storeu_si128((__m128i*)(row+kx), res);
		px=_mm_add_ps(px, four);
	}
	if(xrem)
	{
		for(int kx=0;kx<xrem;++kx)
			dst.m128i_i32[kx]=row[xround+kx];
		res=cb_aa_line_px(dst, px, py, p, idx, xrem);
		for(int kx=0;kx<xrem;++kx)
			row[xround+kx]=res.m128i_i32[kx];
	}
}
__forceinline __m128 aa_point_gen_alpha(__m128 const &px, __m128 const &py, AAInfo *aai)
{
	//t = length(p-p1) = sqrt((px-x1)^2+(py-y1)^2)
	__m128 prx=_mm_sub_ps(px, aai->x1);
	__m128 pry=_mm_sub_ps(py, aai->y1);

	//distance = sqrt(px*px+py*py)		distance to point
	prx=_mm_mul_ps(prx, prx);
	pry=_mm_mul_ps(pry, pry);
	prx=_mm_add_ps(prx, pry);
	prx=_mm_sqrt_ps(prx);

	//alpha=clamp01(aa_offset-distance)
	prx=_mm_sub_ps(aai->aa_offset, prx);
	__m128 alpha=clamp01(prx);
	return alpha;
}
__forceinline __m128i cb_aa_point_px(__m128i const &dst, __m128 const &kx, __m128 const &ky, void *params, int idx, int count)
{
	auto aai=(AAInfo*)params;
	//__m128i dst=_mm_loadu_si128((__m128i*)(aai->buffer+idx));
	__m128 d_red, d_green, d_blue;
	__m128i d_alpha;
	sse2_split_channels(dst, d_red, d_green, d_blue, d_alpha);//dst alpha is discarded

	__m128 px=kx, py=ky;//pixel
	//__m128 px=_mm_cvtepi32_ps(kx), py=_mm_cvtepi32_ps(ky);
	__m128 a_red=aa_point_gen_alpha(px, py, aai);
	px=_mm_add_ps(px, aai->third);
	__m128 a_green=aa_point_gen_alpha(px, py, aai);
	px=_mm_add_ps(px, aai->third);
	__m128 a_blue=aa_point_gen_alpha(px, py, aai);

	d_red	=sse2_mixchannel(d_red,		aai->red,	a_red);
	d_green	=sse2_mixchannel(d_green,	aai->green,	a_green);
	d_blue	=sse2_mixchannel(d_blue,	aai->blue,	a_blue);
	
	//combine channels
	__m128i b32=_mm_cvtps_epi32(d_blue);
	__m128i g32=_mm_cvtps_epi32(d_green);
	__m128i r32=_mm_cvtps_epi32(d_red);
	g32=_mm_slli_epi32(g32, 8);
	r32=_mm_slli_epi32(r32, 16);

	b32=_mm_or_si128(b32, g32);
	b32=_mm_or_si128(b32, r32);
	b32=_mm_or_si128(b32, aai->alpha);
	return b32;
}
void			cb_aa_point(void *p, int x1, int x2, int y)
{
	auto aai=(AAInfo*)p;
	int xround=x2-x1, xrem=xround&3;
	xround&=~3;
	int idx=aai->bw*y+x1, *row=aai->buffer+idx;

	const __m128 four=_mm_set1_ps(4);
	__m128 py=_mm_set1_ps((float)y);
	__m128 px=_mm_set_ps((float)(x1+3), (float)(x1+2), (float)(x1+1), (float)x1);
	__m128i dst, res;
	for(int kx=0;kx<xround;kx+=4, idx+=4)
	{
		dst=_mm_loadu_si128((__m128i*)(aai->buffer+idx));
		res=cb_aa_point_px(dst, px, py, p, idx, 4);
		_mm_storeu_si128((__m128i*)(row+kx), res);
		px=_mm_add_ps(px, four);
	}
	if(xrem)
	{
		for(int kx=0;kx<xrem;++kx)
			dst.m128i_i32[kx]=row[xround+kx];
		res=cb_aa_point_px(dst, px, py, p, idx, xrem);
		for(int kx=0;kx<xrem;++kx)
			row[xround+kx]=res.m128i_i32[kx];
	}
}
void			draw_line_aa_v2(int *buffer, int bw, int bh, double x1, double y1, double x2, double y2, double linewidth, int color)
{//kalbasa
	x1-=0.5, y1-=0.5, x2-=0.5, y2-=0.5;
#if 1
	if(linewidth<1)
		return;
	Point p[4];
	double dx=x2-x1, dy=y2-y1, d2=dx*dx+dy*dy;
	__m128i c=_mm_set1_epi32(color);
	AAInfo aai=
	{
		_mm_set1_ps((float)x1),
		_mm_set1_ps((float)y1),
		_mm_set1_ps((float)dx),
		_mm_set1_ps((float)dy),
		_mm_setzero_ps(),
		_mm_set1_ps((float)((linewidth+1)*0.5)),
		_mm_set1_ps(1.f/3),
		_mm_set1_ps(2.f/3),
	};
	if(linewidth<3)
		linewidth=3;
	sse2_split_channels(c, aai.red, aai.green, aai.blue, aai.alpha);
	aai.buffer=buffer, aai.bw=bw;
	if(d2)//line
	{
		double invd=linewidth/sqrt(d2), tx=dx*invd, ty=dy*invd;
		p[0].set(int(x1-tx-ty), int(y1-ty+tx));
		p[1].set(int(x1-tx+ty), int(y1-ty-tx));
		p[2].set(int(x2+tx+ty), int(y2+ty-tx));
		p[3].set(int(x2+tx-ty), int(y2+ty+tx));
#if defined _DEBUG && defined LINE_AA_SHOW_BOX
		draw_line_v2(buffer, bw, bh, p[0].x, p[0].y, p[1].x, p[1].y, color);//
		draw_line_v2(buffer, bw, bh, p[1].x, p[1].y, p[2].x, p[2].y, color);//
		draw_line_v2(buffer, bw, bh, p[2].x, p[2].y, p[3].x, p[3].y, color);//
		draw_line_v2(buffer, bw, bh, p[3].x, p[3].y, p[0].x, p[0].y, color);//
#endif
		aai.invd2=_mm_set1_ps(float(1/d2));
		fill_convex_POT(buffer, bw, bh, p, 4, 3, cb_aa_line, &aai);
	}
	else//point
	{
		p[0].set(int(x1-linewidth), int(y1-linewidth));
		p[1].set(int(x1-linewidth), int(y1+linewidth));
		p[2].set(int(x1+linewidth), int(y1+linewidth));
		p[3].set(int(x1+linewidth), int(y1-linewidth));
#if defined _DEBUG && defined LINE_AA_SHOW_BOX
		draw_line_v2(buffer, bw, bh, p[0].x, p[0].y, p[1].x, p[1].y, color);//
		draw_line_v2(buffer, bw, bh, p[1].x, p[1].y, p[2].x, p[2].y, color);//
		draw_line_v2(buffer, bw, bh, p[2].x, p[2].y, p[3].x, p[3].y, color);//
		draw_line_v2(buffer, bw, bh, p[3].x, p[3].y, p[0].x, p[0].y, color);//
#endif
		fill_convex_POT(buffer, bw, bh, p, 4, 3, cb_aa_point, &aai);
	}
#endif
#if 0
	double dx=x2-x1, dy=y2-y1, d2=dx*dx+dy*dy;
	Point p1, p2, p3, p4;
	if(d2)//line
	{
		double invd=linewidth/sqrt(d2), tx=dx*invd, ty=dy*invd;
		p1.set(int(x1-tx-ty), int(y1-ty+tx));
		p2.set(int(x1-tx+ty), int(y1-ty-tx));
		p3.set(int(x2+tx+ty), int(y2+ty-tx));
		p4.set(int(x2+tx-ty), int(y2+ty+tx));
	}
	else//point
	{
		p1.set(int(x1-linewidth), int(y1-linewidth));
		p2.set(int(x1-linewidth), int(y1+linewidth));
		p3.set(int(x1+linewidth), int(y1+linewidth));
		p4.set(int(x1+linewidth), int(y1-linewidth));
	}
	bounds.assign(bh, Range(bw));
	register_line(bounds, bw, bh, p1.x, p1.y, p2.x, p2.y);
	register_line(bounds, bw, bh, p2.x, p2.y, p3.x, p3.y);
	register_line(bounds, bw, bh, p3.x, p3.y, p4.x, p4.y);
	register_line(bounds, bw, bh, p4.x, p4.y, p1.x, p1.y);
	//draw_line_v2(buffer, bw, bh, p1.x, p1.y, p2.x, p2.y, color);//
	//draw_line_v2(buffer, bw, bh, p2.x, p2.y, p3.x, p3.y, color);//
	//draw_line_v2(buffer, bw, bh, p3.x, p3.y, p4.x, p4.y, color);//
	//draw_line_v2(buffer, bw, bh, p4.x, p4.y, p1.x, p1.y, color);//
	//for(int ky=0;ky<bh;++ky)//
	//{
	//	SetPixel(ghMemDC, bounds[ky].i, ky, 0x0000FF);//
	//	SetPixel(ghMemDC, bounds[ky].f, ky, 0xFF0000);//
	//}
	double aa_offset=(linewidth+1)*0.5;//average of line width & pixel size
	if(d2)
	{
		d2=1/d2;
		for(int ky=0;ky<bh;++ky)
		{
			int xstart=bounds[ky].i, xend=bounds[ky].f;
			for(int kx=xstart;kx<=xend;++kx)
			{
				double px=kx, py=ky;//pixel
				double ar=clamp01(((px-x1)*dx+(py-y1)*dy)*d2);//parameter = clamp((p-p1) dot (p2-p1) / length(p1, p2)^2)
				px-=x1+ar*dx;//projection on segment
				py-=y1+ar*dy;
				ar=sqrt(px*px+py*py);//distance to segment
				ar=clamp01(aa_offset-ar);//alpha

				px=kx+1./3, py=ky;
				double ag=clamp01(((px-x1)*dx+(py-y1)*dy)*d2);
				px-=x1+ag*dx;
				py-=y1+ag*dy;
				ag=sqrt(px*px+py*py);
				ag=clamp01(aa_offset-ag);

				px=kx+2./3, py=ky;
				double ab=clamp01(((px-x1)*dx+(py-y1)*dy)*d2);
				px-=x1+ab*dx;
				py-=y1+ab*dy;
				ab=sqrt(px*px+py*py);
				ab=clamp01(aa_offset-ab);

				blendpixel_ct(buffer, bw, kx, ky, color, ar, ag, ab);
			}
		}
	}
	else//single point
	{
		for(int ky=0;ky<bh;++ky)
		{
			int xstart=bounds[ky].i, xend=bounds[ky].f;//SIMD
			for(int kx=xstart;kx<=xend;++kx)
			{
				double px=kx, py=ky;//pixel
				px-=x1, py-=y1;
				double ar=sqrt(px*px+py*py);//distance from pixel to point
				ar=clamp01(aa_offset-ar);//alpha

				px=kx+1./3, py=ky;
				px-=x1, py-=y1;
				double ag=sqrt(px*px+py*py);
				ag=clamp01(aa_offset-ag);

				px=kx+2./3, py=ky;
				px-=x1, py-=y1;
				double ab=sqrt(px*px+py*py);
				ab=clamp01(aa_offset-ab);

				blendpixel_ct(buffer, bw, kx, ky, color, ar, ag, ab);
			}
		}
	}
#endif
}

struct			AssignColorInfo
{
	int *buffer, bw, color;
};
void			cb_assign_color(void *p, int x1, int x2, int y)
{
	auto aci=(AssignColorInfo*)p;
	memfill(aci->buffer+aci->bw*y+x1, &aci->color, (x2-x1)<<2, 1<<2);
	//auto color=(__m128i*)params;
	//return *color;
}
struct			InvertColorInfo
{
	int *buffer, *imask, bw;
	__m128i *mcomp;//{0, 1, 2, 3, 4} minus ones
};
__m128i			m_colormask=_mm_set1_epi32(0x00FFFFFF);
__forceinline __m128i cb_invert_color_px(__m128i const &dst, __m128i const &mask, __m128i const &kx, __m128i const &ky, void *params, int idx, int count)
{
	auto ici=(InvertColorInfo*)params;
	//if(mask.m128i_i32[0]!=mask.m128i_i32[1]||mask.m128i_i32[0]!=mask.m128i_i32[2]||mask.m128i_i32[0]!=mask.m128i_i32[3])//
	//	int LOL_1=0;//
	__m128i comp_pixels=_mm_xor_si128(mask, m_onesmask);
	comp_pixels=_mm_and_si128(comp_pixels, m_colormask);
	__m128i ret=_mm_xor_si128(dst, comp_pixels);
	//__m128i c1=_mm_and_si128(dst, mask);//dst&mask
	//__m128i c2=_mm_and_si128(dst, _mm_xor_si128(mask, m_onesmask));//dst&~mask
	//c2=_mm_xor_si128(c2, m_colormask);
	//dst=_mm_or_si128(c1, c2);//dst&mask|(colorcomp^dst&~mask)

	__m128i newmask=ici->mcomp[count];
	newmask=_mm_or_si128(newmask, mask);
	_mm_storeu_si128((__m128i*)(ici->imask+idx), newmask);
	//for(int k=0;k<count;++k)
	//	ici->imask[idx+k]=-1;
	return ret;
}
void			cb_invert_color(void *p, int x1, int x2, int y)
{
	auto ici=(InvertColorInfo*)p;

	int xround=x2-x1, xrem=xround&3;
	xround&=~3;
	const __m128i four=_mm_set1_epi32(4);
	__m128i mky=_mm_set1_epi32(y);
	__m128i mkx=_mm_set_epi32(x1+3, x1+2, x1+1, x1), res;
	int idx=ici->bw*y+x1, *row=ici->buffer+idx, *mrow=ici->imask+idx;
	__m128i dst, mask;
	for(int kx=0;kx<xround;kx+=4, idx+=4)
	{
		dst=_mm_loadu_si128((__m128i*)(row+kx));
		mask=_mm_loadu_si128((__m128i*)(mrow+kx));
		res=cb_invert_color_px(dst, mask, mkx, mky, p, idx, 4);
		_mm_storeu_si128((__m128i*)(row+kx), res);
		mkx=_mm_add_epi32(mkx, four);
	}
	if(xrem)
	{
		for(int kx=0;kx<xrem;++kx)
		{
			dst.m128i_i32[kx]=row[xround+kx];
			mask.m128i_i32[kx]=mrow[xround+kx];
		}
		res=cb_invert_color_px(dst, mask, mkx, mky, p, idx, xrem);
		for(int kx=0;kx<xrem;++kx)
			row[xround+kx]=res.m128i_i32[kx];
	}
}
void			draw_line_brush(int *buffer, int bw, int bh, int brush, int x1, int y1, int x2, int y2, int color, bool invert_color, int *imask)//invert_color ignores color argument & uses imask which has same dimensions as buffer
{
//	const int nbrushes=sizeof resources::brushes/sizeof resources::Brush;
	const resources::Brush *b=resources::brushes+(brush>0&&brush<resources::nbrushes)*brush;
	if(!b->bounds)//robust
		return;
	if(brush<BRUSH_LARGE_RIGHT45)
	{
#if 1
		if(brush==LINE1||brush==BRUSH_DOT)
		{
			if(invert_color)
				draw_line_v2_invert(buffer, bw, bh, x1, y1, x2, y2, imask);
			else
				draw_line_v2(buffer, bw, bh, x1, y1, x2, y2, color);
			return;
		}
		int dx=x2-x1, dy=y2-y1;
		int brushpoints=b->ysize<<1, delta=dx||dy;
		int nv=brushpoints<<delta;
		static std::vector<Point> points;
		points.resize(nv);
		for(int ky=0;ky<b->ysize;++ky)
			points[ky].set(b->bounds[ky<<1], b->yoffset+ky);
		for(int kv=b->ysize, ky=b->ysize-1;ky>=0;++kv, --ky)
			points[kv].set(b->bounds[ky<<1|1], b->yoffset+ky);
		if(delta)
		{
			for(int kv=0;kv<brushpoints;++kv)
			{
				auto &p=points[kv];
				points[kv+brushpoints].set(p.x+x2, p.y+y2);
			}
		}
		for(int kv=0;kv<brushpoints;++kv)
		{
			auto &p=points[kv];
			p.x+=x1, p.y+=y1;
		}
		//if(dx*dx+dy*dy>10*10)//
		//	int LOL_1=0;//
		if(invert_color)
		{
			__m128i mcomp[]=
			{
				_mm_setzero_si128(),
				_mm_set_epi32(0, 0, 0, -1),
				_mm_set_epi32(0, 0, -1, -1),
				_mm_set_epi32(0, -1, -1, -1),
				_mm_set1_epi32(-1),
			};
			InvertColorInfo ici={buffer, imask, bw, mcomp};
			fill_convex(buffer, bw, bh, points.data(), nv, cb_invert_color, &ici);
		}
		else
		{
			AssignColorInfo aci={buffer, bw, color};
			fill_convex(buffer, bw, bh, points.data(), nv, cb_assign_color, &aci);
			//__m128i m_color=_mm_set1_epi32(color);
			//auto LOL_1=&m_color;//
			//fill_convex(buffer, bw, bh, points.data(), nv, cb_assign_color, &m_color);
#if 0//def _DEBUG
			for(int k=0;k<(int)points.size();++k)//
			{
				auto &point=points[k];
				if(point.is_inside(bw, bh))
				{
					auto &c=buffer[bw*point.y+point.x];
					c+=0x800080;
					c|=0xFF000000;
				}
					//buffer[bw*point.y+point.x]=0xFFFF00FF;
			}
#endif
		}
#endif
#if 0
		if(brush==LINE1||brush==BRUSH_DOT)
		{
			if(invert_color)
				draw_line_v2_invert(buffer, bw, bh, x1, y1, x2, y2, imask);
			else
				draw_line_v2(buffer, bw, bh, x1, y1, x2, y2, color);
			return;
		}

		//'stamp & tail' convex brush algorithm
		int nv=b->ysize<<1;//NPOT
		std::vector<Point> vertices(nv);

		//load vertices CCW
		for(int ky=0;ky<b->ysize;++ky)
			vertices[ky].set(b->bounds[ky<<1], b->yoffset+ky);
		for(int kv=b->ysize, ky=b->ysize-1;ky>=0;++kv, --ky)
			vertices[kv].set(b->bounds[ky<<1|1], b->yoffset+ky);

		//classify edges on two sides of the line (x1, y1) -> (x2, y2)
		int s_start=-1, s_end=-1;
		int dx=x2-x1, dy=y2-y1;
		std::vector<int> cp(nv+1);
		if(dx||dy)
		{
			int c_prev;
			for(int ke=0;ke<=nv;++ke)//loop goes CCW
			{
				int kv=mod(ke, nv);
				auto &v1=vertices[kv], &v2=vertices[mod(ke+1, nv)];//edge points CCW
				int ex=v2.x-v1.x, ey=v2.y-v1.y;
				int cross=ex*dy-ey*dx;//edge x segment > 0 iff CCW
				cp[ke]=cross;
				//if(ke==7||ke==15)
				//	int LOL_1=0;
				if(ke)
				{
					if(c_prev<=0&&cross>0)
						s_start=kv;
					if(c_prev>=0&&cross<0)
						s_end=mod(kv+1, nv);
				}
				c_prev=cross;
			}
		}
		if(invert_color)
		{
			for(int ky=0;ky<b->ysize;++ky)
				draw_line_v2_invert(buffer, bw, bh, x1+b->bounds[ky<<1], y1+b->yoffset+ky, x1+b->bounds[ky<<1|1], y1+b->yoffset+ky, imask);
		/*	for(int ky=0;ky<b->ysize;++ky)
			{
				int idx2=bw*(y1+b->yoffset+ky)+x1;
				for(int kx=b->bounds[ky<<1], kxEnd=b->bounds[ky<<1|1];kx<=kxEnd;++kx)
				{
					int idx=idx2+kx;
					auto &m=imask[idx];
					if(!m)
					{
						m=-1;
						auto &c=buffer[idx];
						c=c&0xFF000000|~c&0x00FFFFFF;
					}
				}
			}//*/
			if(dx||dy)
			{
				__m128i mcomp[]=
				{
					_mm_setzero_si128(),
					_mm_set_epi32(0, 0, 0, -1),
					_mm_set_epi32(0, 0, -1, -1),
					_mm_set_epi32(0, -1, -1, -1),
					_mm_set1_epi32(-1),
				};
				InvertColorInfo ici={buffer, imask, mcomp};

				int kx_prev, ky_prev;
				for(int k=0, kv=s_start;k<nv&&kv!=s_end;++k, kv=mod(kv+1, nv))
				{
					int vx=kv>=b->ysize;
					int vy=kv+((((b->ysize-kv)<<1)-1)&-(kv>=b->ysize));
					int kx=b->bounds[vy<<1|vx], ky=b->yoffset+vy;
					if(k)
					{
						Point p[]=//overlaps: doesn't work with transparent brush
						{
							Point(x1+kx, y1+ky),
							Point(x1+kx_prev, y1+ky_prev),
							Point(x2+kx_prev, y2+ky_prev),
							Point(x2+kx, y2+ky),
						};
						//if(p[0].y==p[1].y&&p[0].y==p[2].y&&p[0].y==p[3].y)
						//	int LOL_1=0;
						fill_convex_POT(buffer, bw, bh, p, 4, 3, cb_invert_color, &ici);
					}
					kx_prev=kx, ky_prev=ky;
				}
			}
		}
		else//assign color
		{
			//stamp initial convex brush	CLIP
			for(int ky=0;ky<b->ysize;++ky)
				draw_line_v2(buffer, bw, bh, x1+b->bounds[ky<<1], y1+b->yoffset+ky, x1+b->bounds[ky<<1|1], y1+b->yoffset+ky, color);
				//	memfill(buffer+bw*(y1+b->yoffset+ky)+x1+b->bounds[ky<<1], &color, (b->bounds[ky<<1|1]-b->bounds[ky<<1])<<2, 1<<2);
				//{
				//	int *row=buffer+bw*(y1+b->yoffset+ky)+x1;
				//	for(int kx=b->bounds[ky<<1], kxEnd=b->bounds[ky<<1|1];kx<=kxEnd;++kx)
				//		row[kx]=color;
				//}
			if(dx||dy)//draw tail
			{
				__m128i m_color=_mm_set1_epi32(color);

				int kx_prev, ky_prev;
				for(int k=0, kv=s_start;k<nv&&kv!=s_end;++k, kv=mod(kv+1, nv))
				{
					int vx=kv>=b->ysize;
					int vy=kv+((((b->ysize-kv)<<1)-1)&-(kv>=b->ysize));
					int kx=b->bounds[vy<<1|vx], ky=b->yoffset+vy;
					if(k)
					{
						Point p[]=//overlaps: doesn't work with transparent brush
						{
							Point(x1+kx, y1+ky),
							Point(x1+kx_prev, y1+ky_prev),
							Point(x2+kx_prev, y2+ky_prev),
							Point(x2+kx, y2+ky),
						};
						//if(p[0].y==p[1].y&&p[0].y==p[2].y&&p[0].y==p[3].y)
						//	int LOL_1=0;
						fill_convex_POT(buffer, bw, bh, p, 4, 3, cb_assign_color, &m_color);
					}
					kx_prev=kx, ky_prev=ky;
				}
			/*	int kx_prev;
				for(int k=0, kv=s_start;k<nv&&kv!=s_end;++k, kv=mod(kv+1, nv))
				//for(int k=0;k<=b->ysize;++k)
				{
					//int kv=mod(k+s_start, nv);
					//int kv=k+s_start;
					int vx=kv>=b->ysize;
					int vy=kv+((((b->ysize-kv)<<1)-1)&-(kv>=b->ysize));
					//if(vy>=b->ysize)
					//	vy=(b->ysize<<1)-1-vy;
					//{
					//	vy-=b->ysize;
					//	vy=b->ysize-1-vy;
					//}
					int kx=b->bounds[vy<<1|vx], ky=b->yoffset+vy;
					if(k&&kx!=kx_prev)
					{
						int xstart=kx_prev+(kx_prev<kx)-(kx_prev>kx);
						for(int kx2=minimum(xstart, kx), kx2end=maximum(xstart, kx);kx2<=kx2end;++kx2)
							draw_line_v2(buffer, bw, bh, x1+kx2, y1+ky, x2+kx2, y2+ky, color);
					}
					else
						draw_line_v2(buffer, bw, bh, x1+kx, y1+ky, x2+kx, y2+ky, color);
					kx_prev=kx;
				}//*/
			}
		}
#endif
#if 0
		if(invert_color)
		{
			for(int ky=0;ky<b->ysize;++ky)
				for(int kx=b->bounds[ky<<1], kxEnd=b->bounds[ky<<1|1];kx<=kxEnd;++kx)
					draw_line_v2_invert(buffer, bw, bh, x1+kx, y1+b->yoffset+ky, x2+kx, y2+b->yoffset+ky, imask);
		}
		else
		{
			for(int ky=0;ky<b->ysize;++ky)
				for(int kx=b->bounds[ky<<1], kxEnd=b->bounds[ky<<1|1];kx<=kxEnd;++kx)
					draw_line_v2(buffer, bw, bh, x1+kx, y1+b->yoffset+ky, x2+kx, y2+b->yoffset+ky, color);
		}
#endif
	}
	else//'line' brushes
	{
		int by1=b->yoffset,
			bx1=b->bounds[0],
			by2=b->yoffset+b->ysize-1,
			bx2=b->bounds[(b->ysize-1)<<1];
		Point p[]=
		{
			Point(x1+bx1, y1+by1),//1a
			Point(x1+bx2, y1+by2),//1b
			Point(x2+bx2, y2+by2),//2b
			Point(x2+bx1, y2+by1),//2a
		};
		if(invert_color)
		{
			__m128i mcomp[]=
			{
				_mm_setzero_si128(),
				_mm_set_epi32(0, 0, 0, -1),
				_mm_set_epi32(0, 0, -1, -1),
				_mm_set_epi32(0, -1, -1, -1),
				_mm_set1_epi32(-1),
			};
			InvertColorInfo ici={buffer, imask, bw, mcomp};
			fill_convex_POT(buffer, bw, bh, p, 4, 3, cb_invert_color, &ici);
		}
		else
		{
			__m128i m_color=_mm_set1_epi32(color);
			fill_convex_POT(buffer, bw, bh, p, 4, 3, cb_assign_color, &m_color);
		}
#if 0
		int by1=b->yoffset,
			bx1=b->bounds[0],
			by2=b->yoffset+b->ysize-1,
			bx2=b->bounds[(b->ysize-1)<<1];
		Point p[]=
		{
			Point(x1+bx1, y1+by1),//1a
			Point(x1+bx2, y1+by2),//1b
			Point(x2+bx2, y2+by2),//2b
			Point(x2+bx1, y2+by1),//2a
		};
		const int
			nv=sizeof(p)/sizeof(Point),//number of vertices
			nvmask=nv-1;//nv=POT (fast mod reduction)

		int vtop=0;//min y
		for(int k=1;k<nv;++k)
			if(p[vtop].y>p[k].y)
				vtop=k;

		int flipped=1-((p[vtop].x<p[vtop-1&nvmask].x||p[vtop].x>p[vtop+1&nvmask].x)<<1);
		int L2=vtop-flipped&nvmask, L1=vtop, R1=vtop, R2=vtop+flipped&nvmask;
		if(p[L2].y==p[L1].y)//left edge is horizontal
		{
			L1=L2;
			L2=L2-flipped&nvmask;//[L2=vtop-2]<-[L1=vtop-1] horizontal [R1=vtop]->[R2=vtop+1]
		}
		else if(p[R1].y==p[R2].y)//right edge is horizontal
		{
			R1=R2;
			R2=R2+flipped&nvmask;//[L2=vtop-1]<-[L1=vtop] horizontal [R1=vtop+1]->[R2=vtop+2]
		}
		//assert(p[L1].y==p[R1].y);
		LineAlgorithm left, right;
		int Lxa=p[L1].x, Lya=p[L1].y, Lxb=p[L2].x, Lyb=p[L2].y;
		int Rxa=p[R1].x, Rya=p[R1].y, Rxb=p[R2].x, Ryb=p[R2].y;
		while(Lya==Lyb)
		{
			L1=L2;
			L2=L2-flipped&nvmask;
			Lxa=p[L1].x, Lya=p[L1].y, Lxb=p[L2].x, Lyb=p[L2].y;
		}
		while(Rya==Ryb)
		{
			R1=R2;
			R2=R2+flipped&nvmask;
			Rxa=p[R1].x, Rya=p[R1].y, Rxb=p[R2].x, Ryb=p[R2].y;
		}
		int Lnext=L2-flipped&nvmask,
			Rnext=R2+flipped&nvmask;
		left.init(Lxa, Lya, Lxb, Lyb);
		right.init(Rxa, Rya, Rxb, Ryb);
		for(int ky=Lya, Lkx=Lxa, Lkx2, Rkx=Rxa, Rkx2;;++ky)
		{
			if(ky==Lyb&&p[L2].y<p[Lnext].y||ky>Lyb)
			{
				L1=L2;
				L2=L2-flipped&nvmask;
				Lxa=p[L1].x, Lya=p[L1].y, Lxb=p[L2].x, Lyb=p[L2].y;
				if(Lya>=Lyb)//horizontal or upside-down
					break;
				left.init(Lxa, Lya, Lxb, Lyb);
			//	left.iterate();
			}
			if(ky==Ryb&&p[R2].y<p[Rnext].y||ky>Ryb)
			{
				R1=R2;
				R2=R2+flipped&nvmask;
				Rxa=p[R1].x, Rya=p[R1].y, Rxb=p[R2].x, Ryb=p[R2].y;
				if(Rya>=Ryb)
					break;
				right.init(Rxa, Rya, Rxb, Ryb);
			//	right.iterate();
			}
			Lkx2=left.iterate();
			Rkx2=right.iterate();
			if(Lkx2<left.xstart||Lkx2>left.xend)
			{
				Lkx2=clamp(left.xstart, Lkx2, left.xend);
				L1=L2;
				L2=L2-flipped&nvmask;
				Lxa=p[L1].x, Lya=p[L1].y, Lxb=p[L2].x, Lyb=p[L2].y;
				if(Lya>=Lyb)
					break;
				left.init(Lxa, Lya, Lxb, Lyb);
				left.iterate();
			}
			if(Rkx2<right.xstart||Rkx2>right.xend)
			{
				Rkx2=clamp(right.xstart, Rkx2, right.xend);
				R1=R2;
				R2=R2+flipped&nvmask;
				Rxa=p[R1].x, Rya=p[R1].y, Rxb=p[R2].x, Ryb=p[R2].y;
				if(Rya>=Ryb)
					break;
				right.init(Rxa, Rya, Rxb, Ryb);
				right.iterate();
			}
			if(ky>=0&&ky<bh)
			{
				int Lstart=minimum(Lkx, Lkx2)+(Lkx2<Lkx), Lrange=abs(Lkx2-Lkx)+(Lkx==Lkx2);
				int Rstart=minimum(Rkx, Rkx2)+(Rkx2<Rkx), Rrange=abs(Rkx2-Rkx)+(Rkx==Rkx2);
				int fillstart=minimum(Lstart, Rstart);
				int fillend=maximum(Lstart+Lrange, Rstart+Rrange);

				//int fs0=fillstart, fe0=fillend;//
				fillstart+=(0-fillstart)&-(fillstart<0);
				fillend+=(bw-1-fillstart)&-(fillstart>=bw);
				if(fillstart<fillend)
				{
					//if(fillstart<0||fillstart>=bw||fillend<0||fillend>=bw)
					//	int LOL_1=0;
					memfill(buffer+bw*ky+fillstart, &color, (fillend-fillstart)<<2, 1<<2);
				}
			}
			//int Lstart=minimum(Lkx, Lkx2)+(Lkx2<Lkx), Lrange=abs(Lkx2-Lkx)+(Lkx==Lkx2);
			//int Rstart=minimum(Rkx, Rkx2)+(Rkx2<Rkx), Rrange=abs(Rkx2-Rkx)+(Rkx==Rkx2);
			//int fillstart=minimum(Lstart, Rstart);
			//int fillrange=maximum(Lstart+Lrange, Rstart+Rrange)-fillstart;
			//if(ky<0||ky>=bh)//
			//	int LOL_1=0;//
			//if(fillstart<0||fillstart+fillrange>=bw||fillrange<0)//
			//	int LOL_2=0;//
			//memfill(buffer+bw*ky+fillstart, &color, fillrange<<2, 1<<2);
			//buffer[bw*ky+kx]=color;//
			Lkx=Lkx2;
			Rkx=Rkx2;
		}
#endif
#if 0
	//	std::vector<Range> bounds(ih);
		int by1=b->yoffset,
			bx1=b->bounds[0],
			by2=b->yoffset+b->ysize-1,
			bx2=b->bounds[(b->ysize-1)<<1];
		Point
			p1L(x1+bx1, y1+by1),
			p1R(x1+bx2, y1+by2),
			p2L(x2+bx1, y2+by1),
			p2R(x2+bx2, y2+by2);
		bounds.assign(bh, Range(bw));
		register_line(bounds, bw, bh, p1L.x, p1L.y, p2L.x, p2L.y);
		register_line(bounds, bw, bh, p2L.x, p2L.y, p2R.x, p2R.y);
		register_line(bounds, bw, bh, p2R.x, p2R.y, p1R.x, p1R.y);
		register_line(bounds, bw, bh, p1R.x, p1R.y, p1L.x, p1L.y);
		if(invert_color)
		{
			for(int ky=0;ky<bh;++ky)
			{
				for(int kx=bounds[ky].i, kxEnd=bounds[ky].f;kx<=kxEnd;++kx)
				{
					int idx=bw*ky+kx;
					bool &m=imask[idx];//SSE2
					if(!m)
					{
						m=-1;
						auto &c=buffer[idx];
						c=c&0xFF000000|~c&0x00FFFFFF;
					}
				}
			}
		}
		else
		{
			for(int ky=0;ky<bh;++ky)
				memfill(buffer+bw*ky+bounds[ky].i, &color, (bounds[ky].f+1-bounds[ky].i)<<2, 1<<2);
				//for(int kx=bounds[ky].i, kxEnd=bounds[ky].f;kx<=kxEnd;++kx)
				//	buffer[bw*ky+kx]=color;
		}
#endif
	}//if(brush<BRUSH_LARGE_RIGHT45)/else
}
bool			draw_line_frame1=false;
void			draw_line_mouse(int *buffer, int mx1, int my1, int mx2, int my2, int color, int c2grad, bool enable_gradient)
{
	double x1, y1, x2, y2;
	screen2image(mx1, my1, x1, y1);
	screen2image(mx2, my2, x2, y2);
	double x2a, y2a;
	if(kb[VK_SHIFT])
	{
		double dx=x2-x1, dy=y2-y1;
		double angle=todeg*atan2(dy, dx);
		angle+=360&-(angle<0);
		//GUIPrint(ghMemDC, w>>1, h>>1, "angle=%lf", angle);//
		if(angle<22.5||angle>337.5)//0 degrees
			x2a=x2, y2a=y1;
		else if(angle<67.5)//45 degrees
		{
			double av=(dx+dy)*0.5;
			x2a=x1+av, y2a=y1+av;
		}
		else if(angle<112.5)//90 degrees
			x2a=x1, y2a=y2;
		else if(angle<157.5)//135 degrees
		{
			double av=(-dx+dy)*0.5;
			x2a=x1-av, y2a=y1+av;
		}
		else if(angle<202.5)//180 degrees
			x2a=x2, y2a=y1;
		else if(angle<247.5)//225 degrees
		{
			double av=(dx+dy)*0.5;
			x2a=x1+av, y2a=y1+av;
		}
		else if(angle<292.5)//270 degrees
			x2a=x1, y2a=y2;
		else//315 degrees
		{
			double av=(dx-dy)*0.5;
			x2a=x1+av, y2a=y1-av;
		}
	}
	else
		x2a=x2, y2a=y2;
	if(kb['D']&&enable_gradient)
	{
		if(!kb[VK_CONTROL])
			x1=floor(x1), y1=floor(y1), x2a=floor(x2a), y2a=floor(y2a);
		draw_gradient(buffer, iw, ih, color, x1, y1, x2a, y2a, c2grad, draw_line_frame1);
		draw_line_frame1=false;
	}
	else if(kb[VK_CONTROL])
		draw_line_aa_v2(buffer, iw, ih, x1, y1, x2a, y2a, linewidth, color);
	else
		draw_line_brush(buffer, iw, ih, linewidth, (int)x1, (int)y1, (int)x2a, (int)y2a, color);
		//draw_line_brush(buffer, iw, ih, BRUSH_LARGE_RIGHT45, (int)x1, (int)y1, (int)x2a, (int)y2a, color);
}

void			draw_line_d(int *buffer, double x1, double y1, double x2, double y2, int color)
{
	if(x1<0&&x2<0||x1>=iw&&x2>=iw||y1<0&&y2<0||y1>=ih&&y2>=ih)
		return;
	double dx=x2-x1, dy=y2-y1, xa, ya, xb, yb;
	if(!dx&&!dy)
	{
		if(x1>=0&&x1<iw&&y1>=0&&y1<ih)
			buffer[iw*int(y1)+int(x1)]=color;//TODO: history struct
	}
	else if(abs(dx)>abs(dy))//horizontal
	{
		if(x1<x2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		int xstart=(int)maximum(xa, 0.), xend=(int)minimum(xb, (double)iw-1);
		double r=(yb-ya)/(xb-xa);
		for(double x=xstart;x<=xend;++x)
		{
			double y=ya+(x-xa)*r;
			if(y>=0&&y<ih)
			{
				//if(y<0||y>=ih||x<0||x>=iw)
				//	int LOL_1=0;
				buffer[iw*(int)y+(int)x]=color;//TODO: alpha blend
			}
		}
	}
	else//vertical
	{
		if(y1<y2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		int ystart=(int)maximum(ya, 0.), yend=(int)minimum(yb, (double)ih-1);
		double r=(xb-xa)/(yb-ya);
		for(double y=ystart;y<=yend;++y)
		{
			double x=xa+(y-ya)*r;
			if(x>=0&&x<iw)
			{
				//if(y<0||y>=ih||x<0||x>=iw)
				//	int LOL_1=0;
				buffer[iw*(int)y+(int)x]=color;
			}
		}
	}
}
void			draw_line_d_mouse(int *buffer, int mx1, int my1, int mx2, int my2, int color)
{
	double x1, y1, x2, y2;
	screen2image(mx1, my1, x1, y1);
	screen2image(mx2, my2, x2, y2);
	draw_line_d(buffer, x1, y1, x2, y2, color);
}

void			draw_line_brush_mouse(int *buffer, int brush, int mx1, int my1, int mx2, int my2, int color)
{
//	double x1, y1, x2, y2;
	int x1, y1, x2, y2;
	screen2image(mx1, my1, x1, y1);
	screen2image(mx2, my2, x2, y2);
	draw_line_brush(buffer, iw, ih, brush, x1, y1, x2, y2, color);
}

std::vector<Point> bezier;
void			curve_add_mouse(int mx, int my)
{
	Point i;
	screen2image(mx, my, i.x, i.y);
	switch(bezier.size())
	{
	case 0:
	case 1:
		bezier.push_back(i);
		break;
	case 2:
		bezier.insert(bezier.begin()+1, i);
		break;
	case 3:
		bezier.insert(bezier.begin()+2, i);
		break;
	}
}
void			curve_update_mouse(int mx, int my)
{
	Point i;
	screen2image(mx, my, i.x, i.y);
	const int access[]={-1, 0, 1, 1, 2};
	int idx=access[(bezier.size()<5)*bezier.size()];
	if(idx!=-1)
		bezier[idx].set(i.x, i.y);
}
double			distance(Point const &a, Point const &b)
{
	int dx=b.x-a.x, dy=b.y-a.y;
	return sqrt(double(dx*dx+dy*dy));
}
void			curve_draw(int *buffer, int color)
{
	double outerpath=0;
	for(int k=1, nv=bezier.size();k<nv;++k)
		outerpath+=distance(bezier[k-1], bezier[k]);
	outerpath=ceil(outerpath);
	switch(bezier.size())
	{
	case 2:
		draw_line_brush(buffer, iw, ih, linewidth, bezier[0].x, bezier[0].y, bezier[1].x, bezier[1].y, color);
		break;
	case 3:
		{
			Point p1=bezier[0];
			for(int k=0;k<=outerpath;++k)
			{
				double t=k/outerpath, tc=1-t;
				Point p2=(tc*tc)*bezier[0]+(2*tc*t)*bezier[1]+(t*t)*bezier[2];
				draw_line_brush(buffer, iw, ih, linewidth, p1.x, p1.y, p2.x, p2.y, color);
				p1=p2;
			}
		}
		break;
	case 4:
		{
			Point p1=bezier[0];
			for(int k=0;k<=outerpath;++k)
			{
				double t=k/outerpath, tc=1-t;
				Point p2=(tc*tc*tc)*bezier[0]+(3*tc*tc*t)*bezier[1]+(3*tc*t*t)*bezier[2]+(t*t*t)*bezier[3];
				draw_line_brush(buffer, iw, ih, linewidth, p1.x, p1.y, p2.x, p2.y, color);
				p1=p2;
			}
		}
		break;
	}
//	bezier.clear();
}