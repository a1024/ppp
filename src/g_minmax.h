#ifndef			MINMAX_H
#define			MINMAX_H
#include		<math.h>
inline int		minimum(int a, int b){return (a+b-abs(a-b))>>1;}
inline int		maximum(int a, int b){return (a+b+abs(a-b))>>1;}
inline double	minimum(double a, double b){return (a+b-abs(a-b))*0.5;}
inline double	maximum(double a, double b){return (a+b+abs(a-b))*0.5;}
inline double	maximum(double a, double b, double c, double d)
{
	double x1=a+b+abs(a-b), x2=c+d+abs(c-d);
	return (x1+x2+abs(x1-x2))*0.25;
}
inline double	minimum(double a, double b, double c, double d)
{
	double x1=a+b-abs(a-b), x2=c+d-abs(c-d);
	return (x1+x2-abs(x1-x2))*0.25;
}
inline int		clamp(int lo, int x, int hi)
{
	int lo2=lo<<1, min=x+hi-abs(x-hi);
	return (lo2+min+abs(lo2-min))>>2;
//	return maximum(lo, minimum(x, hi));
}
inline double	clamp01(double x)
{
	if(x<0)
		x=0;
	if(x>1)
		x=1;
	return x;
	//double min=x+1-abs(x-1);//inaccurate
	//return (min+abs(min))*0.25;
}
inline int		shift(int x, int sh){return sh>=0?x<<sh:x>>-sh;}//signed shift left
#endif