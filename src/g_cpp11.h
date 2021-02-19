#ifndef			G_CPP11_H
#define			G_CPP11_H
#if _MSC_VER<1700
#include		<math.h>
namespace		std
{
	inline double round(double x){return floor(x+0.5);}
	inline bool signbit(double x){return (((char*)&x)[7]>>7)!=0;}
	inline bool isnan(double x){return x!=x;}
	inline bool isinf(double x){return abs(x)==_HUGE;}
}
#endif//_MSC_VER<1700
#endif