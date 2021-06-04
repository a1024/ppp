#include		"ppp.h"
#include		"generic.h"
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
	//const __m128i ones=_mm_set1_epi32(-1);
	//const __m128i alphamask=_mm_set1_epi32(0xFF000000);
	const __m128i colormask=_mm_set1_epi32(0x00FFFFFF);
	int xround=bw&~3;
	if(selection&&selection_free)
	{
		for(int ky=0;ky<bh;++ky)
		{
			int *row=buffer+bw*ky, *mrow=sel_mask+bw*ky;
			int kx=0;
			for(;kx<xround;kx+=4)
			{
				__m128i px=_mm_loadu_si128((__m128i*)(row+kx));
				__m128i m_smask=_mm_loadu_si128((__m128i*)(mrow+kx));
				m_smask=_mm_and_si128(colormask, m_smask);
				px=_mm_xor_si128(px, m_smask);
				_mm_storeu_si128((__m128i*)(row+kx), px);
			}
			for(;kx<bw;++kx)
			{
				if(mrow[kx])
				{
					auto &c=row[kx];
					c=c&0xFF000000|~c&0x00FFFFFF;
				}
			}
		}
		//for(int ky=0;ky<bh;++ky)
		//{
		//	for(int kx=0;kx<bw;++kx)
		//	{
		//		int idx=bw*ky+kx;
		//		if(sel_mask[idx])
		//		{
		//			auto &c=buffer[idx];
		//			c=c&0xFF000000|~c&0x00FFFFFF;
		//		}
		//	}
		//}
	}
	else
	{
		for(int ky=0;ky<bh;++ky)
		{
			int *row=buffer+bw*ky;
			int kx=0;
			for(;kx<xround;kx+=4)
			{
				__m128i px=_mm_loadu_si128((__m128i*)(row+kx));

				px=_mm_xor_si128(px, colormask);
				_mm_storeu_si128((__m128i*)(row+kx), px);

				//__m128i a=_mm_and_si128(px, alphamask);
				//px=_mm_xor_si128(px, ones);
				//__m128i c=_mm_and_si128(px, colormask);
				//c=_mm_or_si128(c, a);
				//_mm_storeu_si128((__m128i*)(row+kx), c);
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
void			clear_alpha()
{
	hist_premodify(image, iw, ih);
	for(int k=0;k<image_size;++k)
		image[k]|=0xFF000000;
}

void			raw2float()
{
	if(rawMode!=RM_FLOAT_MOSAIC)
	{
		messageboxa(ghWnd, "Error", "This operation is only for raw images");
		return;
	}
	rawMode=RM_FLOAT;
	int iw0=iw>>1, ih0=ih>>1, iw2=iw0<<2, size=iw2*ih0;
	float *fimage=(float*)image, *fimage2=new float[size];
	int max_idx=0;
	for(int ky=0;ky<ih0;++ky)
	{
		for(int kx=0;kx<iw0;++kx)
		{
			int kx1=kx<<1, ky1=ky<<1;
			int kx2=kx<<2;
			fimage2[iw2*ky+kx2  ]=fimage[iw*(ky1+1)+kx1];//r
			fimage2[iw2*ky+kx2+1]=(fimage[iw*ky1+kx1]+fimage[iw*(ky1+1)+kx1+1])*0.5f;//g	little loss of green information
			fimage2[iw2*ky+kx2+2]=fimage[iw*ky1+kx1+1];//b
			fimage2[iw2*ky+kx2+3]=1;//a
			if(max_idx<iw2*ky+kx2+4)
				max_idx=iw2*ky+kx2+4;
		}
	}
	image=hist_start(iw2, ih0);
	iw=iw2>>2, ih=ih0, image_size=iw*ih;
	memcpy(image, fimage2, size<<2);
	//fimage2[(iw<<2)*(ih>>1)+(iw<<1)];
	delete[] fimage2;
}