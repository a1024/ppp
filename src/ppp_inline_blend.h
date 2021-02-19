#ifndef		PPP_INLINE_BLEND_H
#define		PPP_INLINE_BLEND_H
#include	"ppp_xmm_globals.h"
inline __m128i	checkboard_sse2_y(int logcb, int ky)//ky is thread idx
{
	__m128i cb=_mm_set1_epi32(ky);
	cb=_mm_srli_epi32(cb, logcb);
	cb=_mm_and_si128(cb, m_one32);
	return cb;
}
inline __m128i	checkboard_sse2(int logcb, int kx, __m128i const &ky_bit, __m128i const &c_even, __m128i const &c_odd)
{//checkboard(kx, ky) = (kx>>logcb^ky>>logcb)&1 ? oddcolor : evencolor
	__m128i cb=_mm_set_epi32(kx+3, kx+2, kx+1, kx);
	cb=_mm_srli_epi32(cb, logcb);
	cb=_mm_and_si128(cb, m_one32);
	cb=_mm_xor_si128(cb, ky_bit);
	cb=_mm_sub_epi32(_mm_setzero_si128(), cb);//negate
	__m128i cbc=_mm_xor_si128(cb, m_onesmask);
	cb=_mm_and_si128(cb, c_odd);
	cbc=_mm_and_si128(cbc, c_even);
	cb=_mm_or_si128(cb, cbc);
	//if(cb.m128i_i32[0]==0xFFFFFF||cb.m128i_i32[1]==0xFFFFFF||cb.m128i_i32[2]==0xFFFFFF||cb.m128i_i32[3]==0xFFFFFF)//
	//	int LOL_1=0;//
	return cb;
}
__forceinline __m128i	blend_ssse3(__m128i const &src, __m128i const &dst)//4 packed pixels
{
	//p = sa*s+(255-sa)*d
	//d = p/255 = ((p+1+(p>>8))>>8)

	//get alphas
	__m128i a0=_mm_shuffle_epi8(src, m_getalpha0), ac0=_mm_xor_si128(a0, m_comp_alphas);//SSSE3
	__m128i a1=_mm_shuffle_epi8(src, m_getalpha1), ac1=_mm_xor_si128(a1, m_comp_alphas);

	//alpha * src
	__m128i s0=_mm_unpacklo_epi8(src, _mm_setzero_si128());
	s0=_mm_mullo_epi16(s0, a0);
	__m128i s1=_mm_unpackhi_epi8(src, _mm_setzero_si128());
	s1=_mm_mullo_epi16(s1, a1);

	//alpha_c * dst
	__m128i d0=_mm_unpacklo_epi8(dst, _mm_setzero_si128());
	d0=_mm_mullo_epi16(d0, ac0);
	__m128i d1=_mm_unpackhi_epi8(dst, _mm_setzero_si128());
	d1=_mm_mullo_epi16(d1, ac1);

	//p = a*s+(255-a)*d
	__m128i p0=_mm_adds_epu16(s0, d0);
	__m128i p1=_mm_adds_epu16(s1, d1);

	//d = p/255 = (p+1+(p>>8))>>8
	s0=_mm_adds_epu16(p0, m_one16);
	p0=_mm_shuffle_epi8(p0, m_shiftr8);
	p0=_mm_adds_epu16(p0, s0);
	p0=_mm_shuffle_epi8(p0, m_shiftr8);

	s1=_mm_adds_epu16(p1, m_one16);
	p1=_mm_shuffle_epi8(p1, m_shiftr8);
	p1=_mm_adds_epu16(p1, s1);
	p1=_mm_shuffle_epi8(p1, m_shiftr8);

	//join low & high parts
	p0=_mm_shuffle_epi8(p0, m_pack8);//00AA00BB00GG00RR00AA00BB00GG00RR -> 0000000000000000AABBGGRRAABBGGRR
	p1=_mm_shuffle_epi8(p1, m_pack8);
	p0=_mm_unpacklo_epi8(p0, p1);//-> ...b9b1b8b0
	p0=_mm_shuffle_epi8(p0, m_pack8);//-> ...b3b2b1b0			//p/255 = {p0/255, p1/255}

	return p0;
}
#endif