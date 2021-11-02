#include		"ppp.h"
#include		"g_api_loader.h"
#include		"fftw3.h"
#include		<Windows.h>
#include		<conio.h>
//#pragma		comment(lib, "libfftw3-3.lib")
const char		file[]=__FILE__;

//enum			APIState
//{
//	API_NOT_LOADED, API_LOAD_ERROR, API_LOADED,
//};
//static int		API_state=API_NOT_LOADED;
//static HMODULE	handle=nullptr;
DECL_FN(fftwf_malloc);
DECL_FN(fftwf_free);
DECL_FN(fftwf_plan_dft_1d);
DECL_FN(fftwf_plan_dft_r2c_1d);
DECL_FN(fftwf_plan_dft_c2r_1d);
DECL_FN(fftwf_plan_r2r_2d);
DECL_FN(fftwf_execute);
DECL_FN(fftwf_destroy_plan);
DECL_FN(fftwf_cleanup);
//void			*(*p_fftwf_malloc)(size_t n)=nullptr;
//void			(*p_fftwf_free)(void *p)=nullptr;
//fftwf_plan	(*p_fftwf_plan_r2r_2d)(int n0, int n1, float *in, float *out, fftwf_r2r_kind kind0, fftwf_r2r_kind kind1, unsigned flags)=nullptr;
//void			(*p_fftwf_execute)(fftwf_plan p)=nullptr;
//void			(*p_fftwf_destroy_plan)(fftwf_plan p)=nullptr;
//void			(*p_fftwf_cleanup)()=nullptr;
//inline int	check_p(void (*p)(), const char *funcname)
//{
//	if(!p)
//	{
//		LOG_ERROR("Failed to load %s()", funcname);
//		return 1;
//	}
//	return 0;
//}
int				load_FFTW()//returns error
{
	if(API_state!=API_NOT_LOADED)
		return 0;
	handle=LoadLibraryA("libfftw3f-3.dll");		SYS_ASSERT(handle);
	int error=0;
//#define LOAD_FN(FUNCNAME)	p_##FUNCNAME=(decltype(&FUNCNAME))GetProcAddress(handle, #FUNCNAME), error|=check_p((void(*)())p_##FUNCNAME, #FUNCNAME)
	LOAD_FN(fftwf_malloc);
	LOAD_FN(fftwf_free);
	LOAD_FN(fftwf_plan_dft_1d);
	LOAD_FN(fftwf_plan_dft_r2c_1d);
	LOAD_FN(fftwf_plan_dft_c2r_1d);
	LOAD_FN(fftwf_plan_r2r_2d);
	LOAD_FN(fftwf_execute);
	LOAD_FN(fftwf_destroy_plan);
	LOAD_FN(fftwf_cleanup);
	if(!error)
		API_state=API_LOADED;
	else
		API_state=API_LOAD_ERROR;
	return error;
}

void			dft_1d_prepare(int size, DFT_1D *dft, int flags)
{
	if(load_FFTW())
		return;
	if(flags&DFT_ALLOC_BUFFERS)
	{
		//if(flags&DFT_REAL_TIME)
		//	dft->time=(float*)p_fftwf_malloc(size*sizeof(float));
		//else
			dft->time=(float*)p_fftwf_malloc(size*sizeof(fftwf_complex));
		dft->freq=(float*)p_fftwf_malloc(size*sizeof(fftwf_complex));
	}
	if(flags&DFT_FORWARD)
	{
		if(flags&DFT_REAL_TIME)
			dft->plan=p_fftwf_plan_dft_r2c_1d(size, dft->time, (fftwf_complex*)dft->freq, FFTW_ESTIMATE);
		else
			dft->plan=p_fftwf_plan_dft_1d(size, (fftwf_complex*)dft->time, (fftwf_complex*)dft->freq, FFTW_FORWARD, FFTW_ESTIMATE);
	}
	else if(flags&DFT_CLEAR_OTHER_PLANS)
		dft->plan=0;
	if(flags&DFT_INVERSE)
	{
		if(flags&DFT_REAL_TIME)
			dft->iplan=p_fftwf_plan_dft_c2r_1d(size, (fftwf_complex*)dft->freq, dft->time, FFTW_ESTIMATE);
		else
			dft->iplan=p_fftwf_plan_dft_1d(size, (fftwf_complex*)dft->freq, (fftwf_complex*)dft->time, FFTW_BACKWARD, FFTW_ESTIMATE);
	}
	else if(flags&DFT_CLEAR_OTHER_PLANS)
		dft->iplan=0;
}
void			dft_1d_destroy(DFT_1D *dft)
{
	if(load_FFTW())
		return;
	if(dft->plan)
		p_fftwf_destroy_plan((fftwf_plan)dft->plan), dft->plan=nullptr;
	if(dft->iplan)
		p_fftwf_destroy_plan((fftwf_plan)dft->iplan), dft->iplan=nullptr;
	if(dft->time)
		p_fftwf_free(dft->time), dft->time=nullptr;
	if(dft->freq)
		p_fftwf_free(dft->freq), dft->freq=nullptr;
}
void			dft_1d_apply(DFT_1D *dft, int flags)
{
	if(load_FFTW())
		return;
	if(flags&DFT_INVERSE)
		p_fftwf_execute((fftwf_plan)dft->iplan);
	else
		p_fftwf_execute((fftwf_plan)dft->plan);
	//scale_buffer(buffer, size, 1.f/sqrt((float)(size<<2)));//
}

static fftwf_plan p_dct=nullptr, p_idct=nullptr;
static float	*buf_in=nullptr, *buf_out=nullptr;
void			destroy_dct()
{
	if(load_FFTW())
		return;
	if(p_dct)
		p_fftwf_destroy_plan(p_dct), p_dct=nullptr;
	if(p_idct)
		p_fftwf_destroy_plan(p_idct), p_idct=nullptr;
	if(buf_in)
		p_fftwf_free(buf_in), buf_in=nullptr;
	if(buf_out)
		p_fftwf_free(buf_out), buf_out=nullptr;
}
void			prepare_dct(int bw, int bh)
{
	if(load_FFTW())
		return;
	int size=bw*bh;
	destroy_dct();//
	buf_in=(float*)p_fftwf_malloc(size*sizeof(float));
	buf_out=(float*)p_fftwf_malloc(size*sizeof(float));
	p_dct=p_fftwf_plan_r2r_2d(bh, bw, buf_in, buf_out,  FFTW_REDFT10,  FFTW_REDFT10,  FFTW_ESTIMATE);//DCT-II
	p_idct=p_fftwf_plan_r2r_2d(bh, bw, buf_in, buf_out,  FFTW_REDFT01,  FFTW_REDFT01,  FFTW_ESTIMATE);//DCT-III
}
static void		scale_buffer(float *buffer, int size, float factor)
{
	for(int k=0;k<size;++k)
		buffer[k]*=factor;
}
void			apply_dct(float *buffer, int bw, int bh)
{
	if(load_FFTW())
		return;
	int size=bw*bh;
	memcpy(buf_in, buffer, size*sizeof(float));
	p_fftwf_execute(p_dct);
	memcpy(buffer, buf_out, size*sizeof(float));
	scale_buffer(buffer, size, 1.f/sqrt((float)(size<<2)));//
}
void			apply_idct(float *buffer, int bw, int bh)
{
	if(load_FFTW())
		return;
	int size=bw*bh;
	memcpy(buf_in, buffer, size*sizeof(float));
	p_fftwf_execute(p_idct);
	memcpy(buffer, buf_out, size*sizeof(float));
	scale_buffer(buffer, size, 1.f/sqrt((float)(size<<2)));
}
void			cleanup_fftw()
{
	if(load_FFTW())
		return;
	destroy_dct();
	p_fftwf_cleanup();
}

void			signal_test()
{
	int width=4, height=4;
	float src[]=
	{
		1, 1, 2, 1,
		1, 1, 1, 1,
		1, 1, 1, 1,
		1, 1, 1, 1,
	};
	prepare_dct(width, height);
	apply_dct(src, width, height);
	apply_idct(src, width, height);
	cleanup_fftw();
}