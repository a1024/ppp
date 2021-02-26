#ifndef			PPP_XMM_CLAMP_H
#define			PPP_XMM_CLAMP_H
#include		"ppp_xmm_globals.h"
__forceinline __m128 clamp01(__m128 const &x)
{
	__m128 t1=_mm_cmpgt_ps(x, _mm_setzero_ps());
	t1=_mm_and_ps(t1, x);

	__m128 t2=_mm_cmplt_ps(x, m_oneps);
	__m128 x2=_mm_and_ps(t2, t1);

	t2=_mm_xor_ps(t2, _mm_castsi128_ps(m_onesmask));
	t2=_mm_and_ps(t2, m_oneps);
	x2=_mm_or_ps(x2, t2);
	return x2;

	//eg: x={-1, 0, 1, 2}
	//__m128 cmp=_mm_cmpge_ps(x, _mm_setzero_ps());//cmp=-(x>=0) = {0, F, F, F}
	//__m128 alpha=_mm_and_ps(x, cmp);//alpha=x&cmp = {0, 0, 1, 2}
	//cmp=_mm_cmplt_ps(alpha, m_oneps);//cmp=-(alpha<1) = {F, F, 0, 0}
	//alpha=_mm_and_ps(alpha, cmp);//alpha&=cmp = {0, 0, 0, 0}
	//cmp=_mm_xor_ps(cmp, _mm_castsi128_ps(m_onesmask));//~cmp = {0, 0, F, F}
	//alpha=_mm_or_ps(alpha, _mm_and_ps(cmp, m_oneps));//alpha = {0, 0, 1, 1}
	//return alpha;
}
#endif