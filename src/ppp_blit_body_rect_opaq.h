#ifdef PPP_GUI_CPP
						bx=mullo_epi32(bx, m_rx);
						bx=_mm_srli_epi32(bx, 16);
						bx=clamp0h(bx, m_bwm1);

						rgb[kdst  ]=buffer[bx.m128i_i32[0]];
						rgb[kdst+1]=buffer[bx.m128i_i32[1]];
						rgb[kdst+2]=buffer[bx.m128i_i32[2]];
						rgb[kdst+3]=buffer[bx.m128i_i32[3]];
						
						m_ksrc=_mm_add_epi32(m_ksrc, m_xstep);
						kdst+=xstep4;
#endif