#ifdef PPP_GUI_CPP
						bx=mullo_epi32(bx, m_rx);
						bx=_mm_srli_epi32(bx, 16);
						bx=clamp0h(bx, m_bwm1);
						
						src=_mm_set_epi32(srow[bx.m128i_i32[3]], srow[bx.m128i_i32[2]], srow[bx.m128i_i32[1]], srow[bx.m128i_i32[0]]);
						m_mask=_mm_set_epi32(mrow[bx.m128i_i32[3]], mrow[bx.m128i_i32[2]], mrow[bx.m128i_i32[1]], mrow[bx.m128i_i32[0]]);
						dst=_mm_loadu_si128((__m128i*)(rgb+kdst));
						m_mask=_mm_cmpeq_epi32(m_mask, _mm_setzero_si128());//preparing the mask

						eq=m_mask;
						dst=_mm_and_si128(dst, eq);

						eq=_mm_xor_si128(eq, m_onesmask);
						eq=_mm_and_si128(eq, src);

						dst=_mm_or_si128(dst, eq);
						_mm_storeu_si128((__m128i*)(rgb+kdst), dst);
						
						m_ksrc=_mm_add_epi32(m_ksrc, m_xstep);
						kdst+=xstep4;
#endif