add_executable(ppp WIN32
	fftw3.h
	g_api_loader.h
	g_buffer.c
	g_console.cpp
	g_cpp11.h
	g_file.c
	g_internal.h
	g_math.c
	g_memory.c
	g_minmax.h
#	g_parser.c
	g_performance.cpp
	g_string.cpp
	g_win32.c
	generic.h
	libfftw3-3.lib
	lodepng.cpp
	lodepng.h
	ppp.cpp
	ppp.h
	ppp_blit_body_free_opaq.h
	ppp_blit_body_free_trans.h
	ppp_blit_body_rect_opaq.h
	ppp_blit_body_rect_trans.h
	ppp_clipboard.cpp
	ppp_color.cpp
	ppp_coords.cpp
	ppp_error.cpp
	ppp_file.cpp
	ppp_fill.cpp
	ppp_gui.cpp
	ppp_history.cpp
	ppp_inline_blend.h
	ppp_inline_check.h
	ppp_lines.cpp
#	ppp_parser.cpp
	ppp_process.cpp
	ppp_processing.cpp
	ppp_resources.cpp
	ppp_selection.cpp
	ppp_shapes.cpp
	ppp_signal.cpp
	ppp_sound.cpp
	ppp_subclass.cpp
	ppp_text.cpp
	ppp_xmm_clamp.h
	ppp_xmm_globals.cpp
	ppp_xmm_globals.h
	stb_image.cpp
	stb_image.h
)

#if (CMAKE_VERSION VERSION_GREATER 3.12)
#  set_property(TARGET ppp PROPERTY CXX_STANDARD 20)
#endif()
