#include		"ppp.h"
#include		"ppp_inline_check.h"
#include		"ppp_xmm_clamp.h"
#include		"generic.h"
#include		<queue>
#include		<stack>
#include		<algorithm>
#include		<assert.h>
struct			XFillInfo
{
	int y, x1, x2;
	XFillInfo(int y, int x1, int x2):y(y), x1(x1), x2(x2){}
};
#define INSIDE(X, Y) mask[bw*(Y)+(X)]
static void		fill_v2_explore(int bw, int ty, int x1, int x2, std::stack<XFillInfo> &s, char *mask)
{
	if(x1<x2)
	{
		int tx, xstart=-1;
		if(INSIDE(x1, ty))
			for(xstart=x1;xstart-1>=0&&INSIDE(xstart-1, ty);--xstart);
		for(tx=x1+1;tx<x2;++tx)
		{
			if(INSIDE(tx, ty))
			{
				if(xstart==-1)
					xstart=tx;
			}
			else if(xstart!=-1)
			{
				s.push(XFillInfo(ty, xstart, tx));
				xstart=-1;
			}
		}
		if(xstart!=-1)
		{
			for(tx=x2;tx<bw&&INSIDE(tx, ty);++tx);
			s.push(XFillInfo(ty, xstart, tx));
		}
	}
}
char*			fill_v2_init(int *buffer, int bw, int bh, int x0, int y0)
{
	int size=bw*bh;
	char *mask=(char*)malloc(size);
	int activeColor=buffer[bw*y0+x0];
	for(int k=0;k<size;++k)
		mask[k]=buffer[k]==activeColor;
	return mask;
}
void			fill_v2(int *buffer, int bw, int bh, int x0, int y0, XFillCallback callback, void *data, char *mask0)
{
	if(x0<0||x0>=bw||y0<0||y0>=bh)
		return;
	
	int size=bw*bh;
	char *mask;
	if(mask0)
	{
		mask=(char*)malloc(size);
		memcpy(mask, mask0, size);
	}
	else
		mask=fill_v2_init(buffer, bw, bh, x0, y0);

	std::stack<XFillInfo> s;
	int y, x1, x2;
	for(x1=x0;x1-1>=0&&INSIDE(x1-1, y0);--x1);
	for(x2=x0+1;x2<bw&&INSIDE(x2, y0);++x2);
	s.push(XFillInfo(y0, x1, x2));
	while(s.size())
	{
		auto &info=s.top();
		y=info.y, x1=info.x1, x2=info.x2;
		s.pop();

		callback(data, x1, x2, y);
		//for(int kx=x1;kx<x2;++kx)
		//	buffer[bw*y+kx]=0xFF000000|rand()<<15|rand();//
		memset(mask+bw*y+x1, 0, x2-x1);
		
		if(y-1>=0)
			fill_v2_explore(bw, y-1, x1, x2, s, mask);
		if(y+1<bh)
			fill_v2_explore(bw, y+1, x1, x2, s, mask);
	}

	free(mask);
}
#undef	INSIDE
void			fill(int *buffer, int bw, int bh, int x0, int y0, int color)
{
	if(icheck(x0, y0))
		return;
	typedef std::pair<int, int> Point;
	std::queue<Point> q;
	q.push(Point(x0, y0));
	int oldcolor=buffer[bw*y0+x0];
	if(color==oldcolor)
		return;
	buffer[bw*y0+x0]=color;
	while(q.size())
	{
		auto f=q.front();
		int x=f.first, y=f.second;
		if(icheck(x, y))
			return;
		q.pop();
		if(x+1<bw&&buffer[bw*y+x+1]==oldcolor)
			buffer[bw*y+x+1]=color, q.push(Point(x+1, y));
		if(x-1>=0&&buffer[bw*y+x-1]==oldcolor)
			buffer[bw*y+x-1]=color, q.push(Point(x-1, y));
		if(y+1<ih&&buffer[bw*(y+1)+x]==oldcolor)
			buffer[bw*(y+1)+x]=color, q.push(Point(x, y+1));
		if(y-1>=0&&buffer[bw*(y-1)+x]==oldcolor)
			buffer[bw*(y-1)+x]=color, q.push(Point(x, y-1));
	}
}
struct			XFillPlain
{
	int *buffer, bw, color;
};
void			xfill_plain(void *p, int x1, int x2, int y)
{
	auto data=(XFillPlain*)p;
	memfill(data->buffer+data->bw*y+x1, &data->color, (x2-x1)<<2, 1<<2);
}
void			fill_mouse(int *buffer, int mx0, int my0, int color)
{
	int x0, y0;
	screen2image(mx0, my0, x0, y0);

	XFillPlain data={buffer, iw, color};
	fill_v2(buffer, iw, ih, x0, y0, xfill_plain, &data, nullptr);

	//fill(buffer, iw, ih, x0, y0, color);
}

void			airbrush(int *buffer, int x0, int y0, int color)
{
	for(int k=0;k<airbrush_cpt;++k)
	{
		int x=0, y=0;
		switch(airbrush_size)
		{
		case S_SMALL:
			do
				x=rand()%9-4, y=rand()%9-4;
			while(x*x+y*y>16);
			break;
		case S_MEDIUM:
			do
				x=rand()%17-8, y=rand()%17-8;
			while(x*x+y*y>64);
			break;
		case S_LARGE:
			do
				x=rand()%25-12, y=rand()%25-12;
			while(x*x+y*y>144);
			break;
		}
		x+=x0, y+=y0;
		if(!icheck(x, y))
			buffer[iw*y+x]=color;
	}
}
void			airbrush_mouse(int *buffer, int mx0, int my0, int color)
{
	double x0, y0;
	screen2image(mx0, my0, x0, y0);
	airbrush(buffer, (int)x0, (int)y0, color);
}

struct			LineAlgorithm
{
	int dx, dy;
	int xa, xstart, xend;
	int invden, negative, num, num2, num2step;

	void init(int xa, int ya, int xb, int yb)
	{
		this->xa=xa;
		dx=xb-xa, dy=yb-ya;
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
		invden=(1<<16)/dy;//dy < 2^16
		negative=-(dx<0);
		num=dx+halfden;
		num2=abs(num)*invden;
		num2step=abs(dx)*invden;
	}
	int iterate()
	{
		//int ret=xa+num/dy-(num<0);
		int ret=xa+(negative^num2>>16)-negative-(num<0);
		num+=dx;
		num2+=num2step;
		return ret;
	}
};
#if 1
void			draw_line_v3(int *buffer, int bw, int bh, int x1, int y1, int x2, int y2, int color)//from ppp_lines.cpp
{//137~149ms	naive int		raw: 42~60ms
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
}
#endif
void			fill_horizontal(int *buffer, int bw, int bh, Point const *p, int nv, XFillCallback callback, void *params)
{
	int fillstart=p[0].x, fillend=p[0].x, ky=p[0].y;
	for(int k=1;k<nv;++k)
	{
		int x=p[k].x;
		if(fillstart>x)
			fillstart=x;
		if(fillend<x)
			fillend=x;
	}
	++fillend;
	fillstart+=(0-fillstart)&-(fillstart<0);
	fillend+=(bw-fillend)&-(fillend>bw);

	if(p[0].y>=0&&p[0].y<bh&&fillstart<fillend)
		callback(params, fillstart, fillend, ky);
	//{
	//	int xround=fillend-fillstart, xrem=xround&3;
	//	xround&=~3;
	//	const __m128i four=_mm_set1_epi32(4);
	//	__m128i mky=_mm_set1_epi32(ky);
	//	__m128i mkx=_mm_set_epi32(fillstart+3, fillstart+2, fillstart+1, fillstart), res;
	//	int idx=bw*ky+fillstart, *row=buffer+idx;
	//	for(int kx=0;kx<xround;kx+=4, idx+=4)
	//	{
	//		res=callback(mkx, mky, params, idx, 4);
	//		_mm_storeu_si128((__m128i*)(row+kx), res);
	//		mkx=_mm_add_epi32(mkx, four);
	//	}
	//	if(xrem)
	//	{
	//		res=callback(mkx, mky, params, idx, xrem);
	//		for(int kx=0;kx<xrem;++kx)
	//			row[xround+kx]=res.m128i_i32[kx];
	//	}
	//}
}
void			fill_convex_POT(int *buffer, int bw, int bh, Point const *p, int nv, int nvmask, XFillCallback callback, void *params)//number of vertices = POT
{
	int vtop=0;//min y
	char all_horizontal=true;
	for(int k=1;k<nv;++k)
	{
		if(p[vtop].y>p[k].y)
			vtop=k;
		all_horizontal&=p[0].y==p[k].y;
	}
	//if(all_horizontal&&!(p[0].y==p[1].y&&p[0].y==p[2].y&&p[0].y==p[3].y))
	//	int LOL_1=0;
	if(all_horizontal)
	{
		fill_horizontal(buffer, bw, bh, p, nv, callback, params);
		return;
	}
	//if(p[0].y==p[1].y&&p[0].y==p[2].y&&p[0].y==p[3].y)
	//	int LOL_1=0;

	int flipped=1-((p[vtop].x<p[vtop-1&nvmask].x||p[vtop].x>p[vtop+1&nvmask].x)<<1);
	int L2=vtop-flipped&nvmask, L1=vtop, R1=vtop, R2=vtop+flipped&nvmask;
	//if(p[L2].y==p[L1].y)//left edge is horizontal
	//{
	//	L1=L2;
	//	L2=L2-flipped&nvmask;//[L2=vtop-2]<-[L1=vtop-1] horizontal [R1=vtop]->[R2=vtop+1]
	//}
	//else if(p[R1].y==p[R2].y)//right edge is horizontal
	//{
	//	R1=R2;
	//	R2=R2+flipped&nvmask;//[L2=vtop-1]<-[L1=vtop] horizontal [R1=vtop+1]->[R2=vtop+2]
	//}
	int Lxa=p[L1].x, Lya=p[L1].y, Lxb=p[L2].x, Lyb=p[L2].y;
	int Rxa=p[R1].x, Rya=p[R1].y, Rxb=p[R2].x, Ryb=p[R2].y;
	while(Lya==Lyb)//left edge is horizontal
	{
		L1=L2;
		L2=L2-flipped&nvmask;//[L2=vtop-2]<-[L1=vtop-1] horizontal [R1=vtop]->[R2=vtop+1]
		Lxa=p[L1].x, Lya=p[L1].y, Lxb=p[L2].x, Lyb=p[L2].y;
	}
	while(Rya==Ryb)//right edge is horizontal
	{
		R1=R2;
		R2=R2+flipped&nvmask;//[L2=vtop-1]<-[L1=vtop] horizontal [R1=vtop+1]->[R2=vtop+2]
		Rxa=p[R1].x, Rya=p[R1].y, Rxb=p[R2].x, Ryb=p[R2].y;
	}
	int Lnext=L2-flipped&nvmask,
		Rnext=R2+flipped&nvmask;
#ifdef _DEBUG
	assert(p[L1].y==p[R1].y);
#endif
	LineAlgorithm left, right;
	left.init(Lxa, Lya, Lxb, Lyb);
	right.init(Rxa, Rya, Rxb, Ryb);
	__m128i mky=_mm_set1_epi32(Lya);
	for(int ky=Lya, Lkx=Lxa, Lkx2, Rkx=Rxa, Rkx2;;++ky)
	{
		if(ky==Lyb&&p[L2].y<p[Lnext].y||ky>Lyb)
		{
			L1=L2;
			L2=Lnext;
			Lnext=Lnext-flipped&nvmask;
			Lxa=p[L1].x, Lya=p[L1].y, Lxb=p[L2].x, Lyb=p[L2].y;
			if(Lya>=Lyb)//horizontal or upside-down
				break;
			left.init(Lxa, Lya, Lxb, Lyb);
			if(Lkx>Lxa)
				Lkx=Lxa;
		}
		if(ky==Ryb&&p[R2].y<p[Rnext].y||ky>Ryb)
		{
			R1=R2;
			R2=Rnext;
			Rnext=Rnext+flipped&nvmask;
			Rxa=p[R1].x, Rya=p[R1].y, Rxb=p[R2].x, Ryb=p[R2].y;
			if(Rya>=Ryb)
				break;
			right.init(Rxa, Rya, Rxb, Ryb);
			if(Rkx<Rxa)
				Rkx=Rxa;
		}
		Lkx2=left.iterate();
		Rkx2=right.iterate();
		if(Lkx2<left.xstart||Lkx2>left.xend)
		{
			Lkx2=clamp(left.xstart, Lkx2, left.xend);
			L1=L2;
			L2=Lnext;
			Lnext=Lnext-flipped&nvmask;
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
			R2=Rnext;
			Rnext=Rnext+flipped&nvmask;
			Rxa=p[R1].x, Rya=p[R1].y, Rxb=p[R2].x, Ryb=p[R2].y;
			if(Rya>=Ryb)
				break;
			right.init(Rxa, Rya, Rxb, Ryb);
			right.iterate();
		}
		if(ky>=0&&ky<bh)
		{
			//int fillstart=minimum(Lkx, Lkx2)+(Lkx2<Lkx);//X can be flipped
			//int fillend=minimum(Rkx, Rkx2)+(Rkx2<Rkx)+abs(Rkx2-Rkx)+(Rkx==Rkx2);

			int Lstart=minimum(Lkx, Lkx2)+(Lkx2<Lkx), Lrange=abs(Lkx2-Lkx)+(Lkx==Lkx2);
			int Rstart=minimum(Rkx, Rkx2)+(Rkx2<Rkx), Rrange=abs(Rkx2-Rkx)+(Rkx==Rkx2);
			int fillstart=minimum(Lstart, Rstart);
			int fillend=maximum(Lstart+Lrange, Rstart+Rrange);

			//int fs0=fillstart, fe0=fillend;//
			fillstart+=(0-fillstart)&-(fillstart<0);
			fillend+=(bw-fillend)&-(fillend>bw);

			if(fillstart<fillend)
				callback(params, fillstart, fillend, ky);
			//{
			//	int xround=fillend-fillstart, xrem=xround&3;
			//	xround&=~3;
			//	const __m128i four=_mm_set1_epi32(4);
			//	__m128i mkx=_mm_set_epi32(fillstart+3, fillstart+2, fillstart+1, fillstart), res;
			//	int idx=bw*ky+fillstart, *row=buffer+idx;
			//	for(int kx=0;kx<xround;kx+=4, idx+=4)
			//	{
			//		res=callback(mkx, mky, params, idx, 4);
			//		_mm_storeu_si128((__m128i*)(row+kx), res);
			//		mkx=_mm_add_epi32(mkx, four);
			//	}
			//	if(xrem)
			//	{
			//		res=callback(mkx, mky, params, idx, xrem);
			//		for(int kx=0;kx<xrem;++kx)
			//			row[xround+kx]=res.m128i_i32[kx];
			//	}
			//}
		}
		Lkx=Lkx2;
		Rkx=Rkx2;
		mky=_mm_add_epi32(mky, m_one32);
	}
}
struct			PolarPoint
{
	int x, y;
	double r, th;
	void set(Point const &p, Point const &v)//v: vector from bottom-left point
	{
		x=p.x, y=p.y;
		//r=sqrt((double)v.x*v.x+v.y*v.y);
		r=(double)v.x*v.x+v.y*v.y;
		if(r)
		{
			th=atan2((double)v.y, (double)v.x);//0<=th<pi (cannot reach pi)
#ifdef _DEBUG
			assert(th>=0&&th<=_pi);//
#endif
			//if(th<0)//unreachable
			//	th+=2*_pi;
		}
		else
			th=-1;
		//th=acos((double)v.x/r);
	}
	bool operator<(PolarPoint const &b)const
	{
		if(th==b.th)
			return r<b.r;
		return th<b.th;
	}
	Point get_point()const{return Point(x, y);}
};
void			print_points(PolarPoint const *points, int np)
{
	for(int k2=0;k2<np;++k2)
	{
		auto &point=points[k2];
		printf("(%d, %d), r=%g,\tth=%g\n", point.x, point.y, point.r, point.th*todeg);
	}
	printf("\n");
	log_pause(LL_PROGRESS);//
}
void			print_points(Point const *points, int np)
{
	for(int k2=0;k2<np;++k2)
	{
		auto &point=points[k2];
		printf("(%d, %d),  ", point.x, point.y);
	}
	printf("\n");
	log_pause(LL_PROGRESS);//
}
inline int		counterclockwise(Point const &a, Point const &b, Point const &c)
{
	int x1=b.x-a.x, y1=b.y-a.y;
	int x2=c.x-b.x, y2=c.y-b.y;
	return x1*y2-y1*x2;
}
void			convex_hull(Point const *p, int nv, std::vector<Point> &hull)
{//Graham's scan
	int bottomleft=0;
	for(int k=1;k<nv;++k)
	{
		if(p[bottomleft].y>p[k].y||p[bottomleft].y==p[k].y&&p[bottomleft].x>p[k].x)
			bottomleft=k;
	}
	static std::vector<PolarPoint> points;
	points.resize(nv);
	for(int k=0;k<nv;++k)
		points[k].set(p[k], p[k]-p[bottomleft]);
	std::sort(points.begin(), points.end());//smallest theta first, smallest r first
	for(int k=nv-2;k>=0;--k)
	{
		int k_next=k+1;
		//k_next-=nv&-(k_next>=nv);
		auto &p1=points[k], &p2=points[k_next];
		if(p1.th==p2.th)
			points.erase(points.begin()+k);
	}

	hull.clear();
	int nv2=points.size();

	//log_start(LL_PROGRESS);//
#if 0
	printf("\nFull set:\n");//
	print_points(p, nv);//
	printf("\nGraham scan input:\n");//
	print_points(points.data(), points.size());//
#endif
	for(int k=0;k<nv2;++k)
	{
		auto current=points[k].get_point();
		int size;
		while((size=hull.size())>1&&counterclockwise(hull[size-2], hull[size-1], current)<=0)
			hull.pop_back();
		hull.push_back(current);
#if 0
		printf("\nGraham scan %d:\n", k);//
		print_points(hull.data(), hull.size());//
#endif
	}
#if 0
		printf("\nGraham scan:\n");//
		print_points(hull.data(), hull.size());//
#endif
	//log_end();//
}
void			fill_convex(int *buffer, int bw, int bh, Point const *points, int nv, XFillCallback callback, void *params)
{
	static std::vector<Point> p;
	convex_hull(points, nv, p);
	nv=p.size();

	int vtop=0;//min y
	char all_horizontal=true;
	for(int k=1;k<nv;++k)
	{
		if(p[vtop].y>p[k].y)
			vtop=k;
		all_horizontal&=p[0].y==p[k].y;
	}
	if(all_horizontal)
	{
		fill_horizontal(buffer, bw, bh, p.data(), nv, callback, params);
		return;
	}

	int flipped=1-((p[vtop].x<p[mod(vtop-1, nv)].x||p[vtop].x>p[mod(vtop+1, nv)].x)<<1);
	int L2=mod(vtop-flipped, nv), L1=vtop, R1=vtop, R2=mod(vtop+flipped, nv);
	int Lxa=p[L1].x, Lya=p[L1].y, Lxb=p[L2].x, Lyb=p[L2].y;
	int Rxa=p[R1].x, Rya=p[R1].y, Rxb=p[R2].x, Ryb=p[R2].y;
	while(Lya==Lyb)//left edge is horizontal
	{
		L1=L2;
		L2=mod(L2-flipped, nv);//[L2=vtop-2]<-[L1=vtop-1] horizontal [R1=vtop]->[R2=vtop+1]
		Lxa=p[L1].x, Lya=p[L1].y, Lxb=p[L2].x, Lyb=p[L2].y;
	}
	while(Rya==Ryb)//right edge is horizontal
	{
		R1=R2;
		R2=mod(R2+flipped, nv);//[L2=vtop-1]<-[L1=vtop] horizontal [R1=vtop+1]->[R2=vtop+2]
		Rxa=p[R1].x, Rya=p[R1].y, Rxb=p[R2].x, Ryb=p[R2].y;
	}
	int Lnext=mod(L2-flipped, nv),
		Rnext=mod(R2+flipped, nv);
#ifdef _DEBUG
	assert(p[L1].y==p[R1].y);
#endif
	LineAlgorithm left, right;
	left.init(Lxa, Lya, Lxb, Lyb);
	right.init(Rxa, Rya, Rxb, Ryb);
	__m128i mky=_mm_set1_epi32(Lya);
	for(int ky=Lya, Lkx=Lxa, Lkx2, Rkx=Rxa, Rkx2;;++ky)
	{
		if(ky==Lyb&&p[L2].y<p[Lnext].y||ky>Lyb)
		{
			L1=L2;
			L2=Lnext;
			Lnext=mod(Lnext-flipped, nv);
			Lxa=p[L1].x, Lya=p[L1].y, Lxb=p[L2].x, Lyb=p[L2].y;
			if(Lya>=Lyb)//horizontal or upside-down
				break;
			left.init(Lxa, Lya, Lxb, Lyb);
			if(Lkx>Lxa)
				Lkx=Lxa;
		}
		if(ky==Ryb&&p[R2].y<p[Rnext].y||ky>Ryb)
		{
			R1=R2;
			R2=Rnext;
			Rnext=mod(Rnext+flipped, nv);
			Rxa=p[R1].x, Rya=p[R1].y, Rxb=p[R2].x, Ryb=p[R2].y;
			if(Rya>=Ryb)
				break;
			right.init(Rxa, Rya, Rxb, Ryb);
			if(Rkx<Rxa)
				Rkx=Rxa;
		}
		Lkx2=left.iterate();
		Rkx2=right.iterate();
		if(Lkx2<left.xstart||Lkx2>left.xend)
		{
			Lkx2=clamp(left.xstart, Lkx2, left.xend);
			L1=L2;
			L2=Lnext;
			Lnext=mod(Lnext-flipped, nv);
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
			R2=Rnext;
			Rnext=mod(Rnext+flipped, nv);
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

			fillstart+=(0-fillstart)&-(fillstart<0);
			fillend+=(bw-fillend)&-(fillend>bw);

			if(fillstart<fillend)
				callback(params, fillstart, fillend, ky);
			//{
			//	int xround=fillend-fillstart, xrem=xround&3;
			//	xround&=~3;
			//	const __m128i four=_mm_set1_epi32(4);
			//	__m128i mkx=_mm_set_epi32(fillstart+3, fillstart+2, fillstart+1, fillstart), res;
			//	int idx=bw*ky+fillstart, *row=buffer+idx;
			//	for(int kx=0;kx<xround;kx+=4, idx+=4)
			//	{
			//		res=callback(mkx, mky, params, idx, 4);//TODO: call callback once per row
			//		_mm_storeu_si128((__m128i*)(row+kx), res);
			//		mkx=_mm_add_epi32(mkx, four);
			//	}
			//	if(xrem)
			//	{
			//		res=callback(mkx, mky, params, idx, xrem);
			//		for(int kx=0;kx<xrem;++kx)
			//			row[xround+kx]=res.m128i_i32[kx];
			//	}
			//}
		}
		Lkx=Lkx2;
		Rkx=Rkx2;
		mky=_mm_add_epi32(mky, m_one32);
	}
//#ifdef _DEBUG
//	for(int k=0;k<(int)p.size();++k)//
//	{
//		auto &point=p[k];
//		if(point.is_inside(bw, bh))
//		{
//			auto &c=buffer[bw*point.y+point.x];
//			c+=0x808080;
//			c|=0xFF000000;
//		}
//	}
//#endif
}

struct			XFillGradient
{
	int *buffer, bw;
	double A, B, C;
	__m128 *mc1, *dc, *pmA4;
};
__forceinline __m128i gen_gradient_sse(__m128 const &z, __m128 const *mc1, __m128 const *dc)
{
	__m128 alpha=clamp01(z);
	//alpha = clamp01(z)		eg: z={-1, 0, 1, 2}
	//__m128 cmp=_mm_cmpge_ps(z, _mm_setzero_ps());//cmp=-(z>=0) = {0, F, F, F}
	//__m128 alpha=_mm_and_ps(z, cmp);//alpha=z&cmp = {0, 0, 1, 2}
	//cmp=_mm_cmplt_ps(alpha, m_oneps);//cmp=-(alpha<1) = {F, F, 0, 0}
	//alpha=_mm_and_ps(alpha, cmp);//alpha&=cmp = {0, 0, 0, 0}
	//cmp=_mm_xor_ps(cmp, _mm_castsi128_ps(m_onesmask));//~cmp = {0, 0, F, F}
	//alpha=_mm_or_ps(alpha, _mm_and_ps(cmp, m_oneps));//alpha = {0, 0, 1, 1}

	//cx = mix(c1, c2, alpha)
	__m128 cx[]=
	{
		_mm_mul_ps(dc[0], alpha),
		_mm_mul_ps(dc[1], alpha),
		_mm_mul_ps(dc[2], alpha),
		_mm_mul_ps(dc[3], alpha),
	};
	cx[0]=_mm_add_ps(cx[0], mc1[0]);
	cx[1]=_mm_add_ps(cx[1], mc1[1]);
	cx[2]=_mm_add_ps(cx[2], mc1[2]);
	cx[3]=_mm_add_ps(cx[3], mc1[3]);

	//extract int color
	__m128i color[]=
	{
		_mm_cvtps_epi32(cx[0]),
		_mm_cvtps_epi32(cx[1]),
		_mm_cvtps_epi32(cx[2]),
		_mm_cvtps_epi32(cx[3]),
	};
	color[1]=_mm_slli_epi32(color[1],  8);
	color[2]=_mm_slli_epi32(color[2], 16);
	color[3]=_mm_slli_epi32(color[3], 24);
	color[0]=_mm_or_si128(color[0], color[1]);
	color[0]=_mm_or_si128(color[0], color[2]);
	color[0]=_mm_or_si128(color[0], color[3]);

	return color[0];
}
void			xfill_gradient(void *p, int x1, int x2, int y)
{
	auto data=(XFillGradient*)p;
	int xround=x2-x1, xrem=xround&3;
	xround&=~3;
	int *row=data->buffer+data->bw*y+x1;

	float z0[4]={float(data->A*x1+data->B*y+data->C)};
	z0[1]=float(z0[0]+data->A);
	z0[2]=float(z0[1]+data->A);
	z0[3]=float(z0[2]+data->A);
	__m128 z=_mm_loadu_ps(z0);
	int kx=0;
	__m128i colors;
	for(;kx<xround;kx+=4)
	{
		colors=gen_gradient_sse(z, data->mc1, data->dc);
		_mm_storeu_si128((__m128i*)(row+kx), colors);

		//next z
		z=_mm_add_ps(z, *data->pmA4);
	}
	if(xrem)
	{
		colors=gen_gradient_sse(z, data->mc1, data->dc);
		for(kx=0;kx<xrem;++kx)
			row[xround+kx]=colors.m128i_i32[kx];
	}
	//memfill(data->buffer+data->bw*y+x1, &data->color, (x2-x1)<<2, 1<<2);
}
void			draw_gradient(int *buffer, int bw, int bh, int c1, double x1, double y1, double x2, double y2, int c2, bool startOver)
{
	static char *mask=0;
	static int mw=0, mh=0;
	if(startOver)
	{
		if(mask)
			free(mask);
		mask=fill_v2_init(buffer, bw, bh, (int)x1, (int)y1);
		mw=bw, mh=bh;
	}

	//p1=(x1, y1, 0)
	//p2=(x2, y2, 1)
	//p3=(x1-dy, y1+dx, 0)
	//n = cross(p2-p1, p3-p1) = (-dx, -dy, dx^2+dy^2)
	//n.p=n.p1		-> z = (nx*x1+ny*y1 - nx*x - ny*y)/nz = Ax+By+C
	double dx=x2-x1, dy=y2-y1, d2=dx*dx+dy*dy;
	if(!d2)
		return;
	if(d2>100)//
		int LOL_1=0;//
	d2=1/d2;
	double A=dx*d2, B=dy*d2, C=-(dx*x1+dy*y1)*d2;
	auto p1=(unsigned char*)&c2, p2=(unsigned char*)&c1;
	
#if 1
	__m128 mA4=_mm_set1_ps((float)(A*4));//, mB=_mm_set1_ps((float)B), mC=_mm_set1_ps((float)C);
	__m128 mc1[]={_mm_set1_ps(p1[0]), _mm_set1_ps(p1[1]), _mm_set1_ps(p1[2]), _mm_set1_ps(p1[3])};
	__m128 mc2[]={_mm_set1_ps(p2[0]), _mm_set1_ps(p2[1]), _mm_set1_ps(p2[2]), _mm_set1_ps(p2[3])};
	__m128 dc[]={_mm_sub_ps(mc2[0], mc1[0]), _mm_sub_ps(mc2[1], mc1[1]), _mm_sub_ps(mc2[2], mc1[2]), _mm_sub_ps(mc2[3], mc1[3])};
	const __m128 half=_mm_set1_ps(0.4999f);
	mc1[0]=_mm_sub_ps(mc1[0], half);//cvtps_epi32(float) = round(float) = floor(float+0.5)
	mc1[1]=_mm_sub_ps(mc1[1], half);
	mc1[2]=_mm_sub_ps(mc1[2], half);
	mc1[3]=_mm_sub_ps(mc1[3], half);
	XFillGradient data={buffer, bw, A, B, C, mc1, dc, &mA4};
	fill_v2(buffer, bw, bh, (int)x1, (int)y1, xfill_gradient, &data, mask);
#endif
#if 0
	__m128 mA4=_mm_set1_ps((float)(A*4));//, mB=_mm_set1_ps((float)B), mC=_mm_set1_ps((float)C);
	__m128 mc1[]={_mm_set1_ps(p1[0]), _mm_set1_ps(p1[1]), _mm_set1_ps(p1[2]), _mm_set1_ps(p1[3])};
	__m128 mc2[]={_mm_set1_ps(p2[0]), _mm_set1_ps(p2[1]), _mm_set1_ps(p2[2]), _mm_set1_ps(p2[3])};
	__m128 dc[]={_mm_sub_ps(mc2[0], mc1[0]), _mm_sub_ps(mc2[1], mc1[1]), _mm_sub_ps(mc2[2], mc1[2]), _mm_sub_ps(mc2[3], mc1[3])};
	const __m128 half=_mm_set1_ps(0.4999f);
	mc1[0]=_mm_sub_ps(mc1[0], half);//cvtps_epi32(float) = round(float) = floor(float+0.5)
	mc1[1]=_mm_sub_ps(mc1[1], half);
	mc1[2]=_mm_sub_ps(mc1[2], half);
	mc1[3]=_mm_sub_ps(mc1[3], half);
	int xround=bw&~3, xrem=bw&3;
	for(int ky=0;ky<bh;++ky)
	{
		float z0[4]={float(B*ky+C)};
		z0[1]=float(z0[0]+A);
		z0[2]=float(z0[1]+A);
		z0[3]=float(z0[2]+A);
		__m128 z=_mm_loadu_ps(z0);
		int *row=buffer+bw*ky;
		int kx=0;
		__m128i colors;
		for(;kx<xround;kx+=4)
		{
			colors=gen_gradient_sse(z, mc1, dc);
			_mm_storeu_si128((__m128i*)(row+kx), colors);

			//next z
			z=_mm_add_ps(z, mA4);
		}
		if(xrem)
		{
			colors=gen_gradient_sse(z, mc1, dc);
			for(kx=0;kx<xrem;++kx)
				row[xround+kx]=colors.m128i_i32[kx];
		}
	}
#endif
#if 0
	for(int ky=0;ky<bh;++ky)
	{
		double z=B*ky+C;
		int *row=buffer+bw*ky;
		for(int kx=0;kx<bw;++kx)
		{
			double alpha=clamp01(z);
			auto p=(unsigned char*)(row+kx);
			p[0]=p2[0]+int(alpha*(p1[0]-p2[0]));
			p[1]=p2[1]+int(alpha*(p1[1]-p2[1]));
			p[2]=p2[2]+int(alpha*(p1[2]-p2[2]));
			p[3]=p2[3]+int(alpha*(p1[3]-p2[3]));
			z+=A;
		}
	}
#endif
}

//register line
#if 0
struct			Range
{
	int i, f;
	//Range():i(iw), f(0){}
	Range(int bw):i(bw), f(-1){}
	//Range(int bw):i(bw), f(0){}
	void assign(int x, int bw)
	{
		if(i>x)
		{
			if(x<0)
				i=0;
			else
				i=x;
		}
		if(f<x)
		{
			if(x>=bw)
				f=bw-1;
			else
				f=x;
		}
		//if(x>=0&&i>x)
		//	i=x;
		//if(x<iw&&f<x+1)
		//	f=x+1;
	}
};
std::vector<Range> bounds;
void			register_line(std::vector<Range> &bounds, int bw, int bh, int x1, int y1, int x2, int y2)
{
	clip_line_y_only(bw, bh, x1, y1, x2, y2);
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
			//if(kx<0||kx>=bw||ky<0||ky>=bh)//
			//	return;//
			
			if(ky>=0&&ky<bh)
				bounds[ky].assign(kx, bw);

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
			//if(kx<0||kx>=bw||ky<0||ky>=bh)//
			//	return;//
			
			if(ky>=0&&ky<bh)
				bounds[ky].assign(kx, bw);

			error+=dx;
			cmp=-(error>=dy);
			kx+=inc&cmp;
			error-=dy&cmp;
		}
	}
#if 0
	//if(x1<0&&x2<0||x1>=iw&&x2>=iw||y1<0&&y2<0||y1>=ih&&y2>=ih)
	//	return;
	double dx=x2-x1, dy=y2-y1, xa, ya, xb, yb;
	if(!dx&&!dy)
	{
		int ix1=(int)x1, iy1=(int)y1;
	//	if(ix1>=0&&ix1<iw&&iy1>=0&&iy1<ih)
		if(iy1>=0&&iy1<ih)
			bounds[y1].assign(x1);
	}
	else if(abs(dx)>abs(dy))//horizontal
	{
		if(x1<x2)
			xa=x1, ya=y1, xb=x2, yb=y2;
		else
			xa=x2, ya=y2, xb=x1, yb=y1;
		int xstart=(int)xa, xend=(int)xb;//no clamping
		double r=(yb-ya)/(xb-xa);
		for(double x=xstart;x<=xend;++x)
		{
			double y=ya+(x-xa)*r;
			int ix=(int)std::round(x), iy=(int)std::round(y);
			if(iy>=0&&iy<ih)
			{
			//	if(icheck(ix, iy))//
			//		continue;//
				bounds[iy].assign(ix);
			}
		}
	}
	else//vertical & 45 degrees
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
			int ix=(int)std::round(x), iy=(int)std::round(y);
		//	if(ix>=0&&ix<iw)
		//	{
			//	if(icheck(ix, iy))//
			//		continue;//
				bounds[iy].assign(ix);
		//	}
		}
	}
#endif
}
#endif