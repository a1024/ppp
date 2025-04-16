#ifndef		PPP_XMM_GLOBALS_H
#define		PPP_XMM_GLOBALS_H
#include	<tmmintrin.h>
extern const __m128i m_onesmask;
extern const __m128i m_getalpha0, m_getalpha1, m_comp_alphas;
extern const __m128i m_one16, m_one32;
extern const __m128  m_oneps;
extern const __m128i m_shiftr8;
extern const __m128i m_pack8;

extern const __m128i m_channel_mask, m_alpha_mask;
#endif