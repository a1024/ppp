#include		"ppp.h"
#include		<tmmintrin.h>
void			swap_rb(int *dstimage, const int *srcimage, int pxcount)
{
	const __m128i shuffle_rb=_mm_set_epi8
		(
			15, 12, 13, 14,
			11, 8, 9, 10,
			7, 4, 5, 6,
			3, 0, 1, 2
		);
	int xround=pxcount&~3, xrem=pxcount&3;
	__m128i pixels;
	for(int k=0;k<xround;k+=4)
	{
		pixels=_mm_loadu_si128((__m128i*)(srcimage+k));
		pixels=_mm_shuffle_epi8(pixels, shuffle_rb);//SSSE3
		_mm_storeu_si128((__m128i*)(dstimage+k), pixels);
	}
	if(xrem)
	{
		for(int k=0;k<xrem;++k)
			pixels.m128i_i32[k]=srcimage[xround+k];
		pixels=_mm_shuffle_epi8(pixels, shuffle_rb);
		for(int k=0;k<xrem;++k)
			dstimage[xround+k]=pixels.m128i_i32[k];
	}
}
void			invertcolor(int *buffer, int bw, int bh, bool selection)
{
	if(selection&&selection_free)
	{
		for(int ky=0;ky<bh;++ky)
		{
			for(int kx=0;kx<bw;++kx)
			{
				int idx=bw*ky+kx;
				if(sel_mask[idx])
				{
					auto &c=buffer[idx];
					c=c&0xFF000000|~c&0x00FFFFFF;
				}
			}
		}
	}
	else
	{
		const __m128i ones=_mm_set1_epi32(-1);
		const __m128i alphamask=_mm_set1_epi32(0xFF000000), colormask=_mm_set1_epi32(0x00FFFFFF);
		int wround=bw&~3;
		for(int ky=0;ky<bh;++ky)
		{
			int *row=buffer+bw*ky;
			int kx=0;
			for(;kx<wround;++kx)
			{
				__m128i px=_mm_loadu_si128((__m128i*)(row+kx));
				__m128i a=_mm_and_si128(px, alphamask);
				px=_mm_xor_si128(px, ones);
				__m128i c=_mm_and_si128(px, colormask);
				c=_mm_or_si128(c, a);
				_mm_storeu_si128((__m128i*)(row+kx), c);
			}
			for(;kx<bw;++kx)
			{
				auto &c=row[kx];
				c=c&0xFF000000|~c&0x00FFFFFF;
			}
		}
		_mm_empty();
		//for(int ky=0;ky<bh;++ky)
		//{
		//	for(int kx=0;kx<bw;++kx)
		//	{
		//		auto &c=buffer[bw*ky+kx];
		//		c=c&0xFF000000|~c&0x00FFFFFF;
		//	}
		//}
	}
}