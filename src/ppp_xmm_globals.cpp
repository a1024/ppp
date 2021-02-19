#include		<tmmintrin.h>
extern const __m128i m_getalpha0=_mm_set_epi8(-1, 7, -1, 7, -1, 7, -1, 7, -1, 3, -1, 3, -1, 3, -1, 3);
extern const __m128i m_getalpha1=_mm_set_epi8(-1, 15, -1, 15, -1, 15, -1, 15, -1, 11, -1, 11, -1, 11, -1, 11);
extern const __m128i m_comp_alphas=_mm_set1_epi32(0x00FF00FF), m_onesmask=_mm_set1_epi32(-1);
extern const __m128i m_one16=_mm_set1_epi16(1), m_one32=_mm_set1_epi32(1);
extern const __m128  m_oneps=_mm_set1_ps(1);
extern const __m128i m_shiftr8=_mm_set_epi8(-1, 15, -1, 13, -1, 11, -1, 9, -1, 7, -1, 5, -1, 3, -1, 1);
extern const __m128i m_pack8=_mm_set_epi8(15, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 0);

extern const __m128i m_channel_mask=_mm_set1_epi32(0xFF), m_alpha_mask=_mm_set1_epi32(0xFF000000);