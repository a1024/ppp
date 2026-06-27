#ifndef MINMAX_H
#define MINMAX_H
//#include<math.h>
#define MINIMUM(A, B) ((A)<(B)?(A):(B))
#define MAXIMUM(A, B) ((A)>(B)?(A):(B))
#define CLAMP(X, L, H) X=X<(L)?L:X, X=X>(H)?H:X
#define SHIFT_LEFT(X, S) ((S)>=0?(X)<<(S):(X)>>-(S))
#define NEGATE(X, F) (((X)^-(F))+(F))
#if 0
INLINE int minimum(int a, int b)
{
	if(a>b)
		a=b;
	return a;
//	return (a+b-abs(a-b))>>1;
}
INLINE int maximum(int a, int b)
{
	if(a<b)
		a=b;
	return a;
//	return (a+b+abs(a-b))>>1;
}
INLINE double minimum(double a, double b)
{
	if(a>b)
		a=b;
	return a;
//	return (a+b-abs(a-b))*0.5;
}
INLINE double maximum(double a, double b)
{
	if(a<b)
		a=b;
	return a;
//	return (a+b+abs(a-b))*0.5;
}
#endif
INLINE double maximum(double a, double b, double c, double d)
{
	if(a<b)
		a=b;
	if(a<c)
		a=c;
	if(a<d)
		a=d;
	return a;
	//double x1=a+b+abs(a-b), x2=c+d+abs(c-d);
	//return (x1+x2+abs(x1-x2))*0.25;
}
INLINE double minimum(double a, double b, double c, double d)
{
	if(a>b)
		a=b;
	if(a>c)
		a=c;
	if(a>d)
		a=d;
	return a;
//	double x1=a+b-abs(a-b), x2=c+d-abs(c-d);
//	return (x1+x2-abs(x1-x2))*0.25;
}
#if 0
INLINE int clamp(int lo, int x, int hi)
{
	if(lo>hi)
	{
		x=(lo+hi)>>1;
		return x;
	}
	if(x<lo)
		x=lo;
	if(x>hi)
		x=hi;
	return x;
	//int lo2=lo<<1, min=x+hi-abs(x-hi);
	//return (lo2+min+abs(lo2-min))>>2;

	//return maximum(lo, minimum(x, hi));
}
INLINE double clamp01(double x)
{
	if(x<0)
		x=0;
	if(x>1)
		x=1;
	return x;
	//double min=x+1-abs(x-1);//inaccurate
	//return (min+abs(min))*0.25;
}
INLINE int shift(int x, int sh){return sh>=0?x<<sh:x>>-sh;}//signed shift left
INLINE int conditional_negate(int x, int flag){return (x^-flag)+flag;}
#endif
#endif