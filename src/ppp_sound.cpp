#include		"ppp.h"
#include		"generic.h"//log
#include		"g_api_loader.h"
#include		<SDL.h>
const char		file[]=__FILE__;

#if _MSC_VER<1800
bool			isfinite(float x)
{
	auto &x2=(int&)x;
	return (x2&0x7F100000)!=0x7F100000;
}
#endif

//enum			APIState
//{
//	API_NOT_LOADED, API_LOAD_ERROR, API_LOADED,
//};
//static int		API_state=API_NOT_LOADED;
//static HMODULE	handle=nullptr;
//#define	DECL_FN(FUNCNAME)		decltype(&FUNCNAME) p_##FUNCNAME=nullptr
DECL_FN(SDL_Init);
DECL_FN(SDL_GetError);
DECL_FN(SDL_OpenAudioDevice);
DECL_FN(SDL_PauseAudioDevice);
DECL_FN(SDL_memcpy);
DECL_FN(SDL_Delay);
DECL_FN(SDL_CloseAudioDevice);
//int			(__cdecl *p_SDL_Init)(unsigned flags)=nullptr;
//void*			(__cdecl *p_SDL_memcpy)(void *dst, const void *src, size_t len)=nullptr;
//SDL_AudioDeviceID (__cdecl *p_SDL_OpenAudioDevice)(void *dst, const void *src, size_t len)=nullptr;
int				load_SDL()
{
	if(API_state!=API_NOT_LOADED)
		return 0;
	handle=LoadLibraryA("SDL2.dll");		SYS_ASSERT(handle);
	int error=0;
	LOAD_FN(SDL_Init);
	LOAD_FN(SDL_GetError);
	LOAD_FN(SDL_OpenAudioDevice);
	LOAD_FN(SDL_PauseAudioDevice);
	LOAD_FN(SDL_memcpy);
	LOAD_FN(SDL_Delay);
	LOAD_FN(SDL_CloseAudioDevice);
	if(!error)
	{
		API_state=API_LOADED;
		p_SDL_Init(SDL_INIT_AUDIO);//
	}
	else
		API_state=API_LOAD_ERROR;
	return error;
}

typedef long long i64;
void			init_sound()
{
	p_SDL_Init(SDL_INIT_AUDIO);
}

//https://www.youtube.com/watch?v=6IX6873J1Y8
struct			AudioData
{
	Uint8 *pos;
	int length;
};
void			callback1(void *userdata, Uint8 *stream, int streamlength)
{
	auto audio=(AudioData*)userdata;
	if(!audio->length)
		return;
	unsigned length=audio->length<streamlength?audio->length:streamlength;
	p_SDL_memcpy(stream, audio->pos, length);//
	audio->pos+=length;
	audio->length-=length;
}
void			play_sound(float *sound, i64 nsamples, i64 samplerate, int nchannels)
{
	if(load_SDL())
		return;
	//SDL_AudioSpec audiospec;
	//Uint8 *start;
	//unsigned length;
	//SDL_LoadWAV_RW(

	AudioData audio={(Uint8*)sound, (int)(nchannels*nsamples*sizeof(float))};

	int buffersamples=128;
	SDL_AudioSpec audiospec=
	{
		(int)samplerate,
		1<<15|1<<8|32,//signed 32bit float
		nchannels,
		0,//silence value
		buffersamples,//samples in buffer
		0,//padding
		buffersamples*4,//buffer byte size
		callback1,
		&audio,//userdata
	};
	SDL_AudioSpec obtainedaudiospec;
	SDL_AudioDeviceID device=p_SDL_OpenAudioDevice(nullptr, 0, &audiospec, &obtainedaudiospec, SDL_AUDIO_ALLOW_ANY_CHANGE);
	if(!device)
	{
		LOG_ERROR("Error: %s\n", p_SDL_GetError());
		return;
	}
	p_SDL_PauseAudioDevice(device, 0);

	while(audio.length>0)
	{
		p_SDL_Delay(100);
	}

	p_SDL_CloseAudioDevice(device);
}

void			print_fbuf(float *buffer, int size)
{
	for(int k=0;k<size;++k)
		printf("%f\n", buffer[k]);
}

const float inv255=1.f/255, inv3=1.f/3, inv765=1.f/765;
const float pi=acos(-1.f), pi3=pi/3, _2pi=pi+pi;
inline float	rgb2hue(float *rgb)//no need to normalize rgb components
{
	int min=0, max=0;
	if(rgb[min]>rgb[1])
		min=1;
	if(rgb[min]>rgb[2])
		min=2;
	if(rgb[max]<rgb[1])
		max=1;
	if(rgb[max]<rgb[2])
		max=2;
	float diff=rgb[max]-rgb[min];
	float hue=0;
	if(diff)
	{
		switch(max)
		{
		case 0:
			hue=(rgb[1]-rgb[2])/diff;
			break;
		case 1:
			hue=2+(rgb[2]-rgb[0])/diff;
			break;
		case 2:
			hue=4+(rgb[0]-rgb[1])/diff;
			break;
		}
		hue*=pi3;
		if(hue<0)
			hue+=_2pi;
	}
	return hue;
}
inline float	sincf(float x)
{
	return sin(x)/x;
}
inline float	sincf(float freq, float x)
{
	float arg=pi*freq*x;
	arg+=arg;
	return sin(arg)/arg;
}
inline float	hannf(float x)
{
	return 0.5f*((x>=-0.5f)-(x>0.5f))*(1+cos(_2pi*x));
}
void			hann_window(float *window, int size, float gain)
{
	float invsize=1.f/size;
	for(int k=0;k<size;++k)
		window[k]=gain*hannf(k*invsize-0.5f);
}
void			spectrogram2sound(void *buffer, int bw, int bh, ImageMode imagetype)
{
	//get luminance & hue (amplitude & phase), ignore alpha		saturation is also ignored
	int mosaic=imagetype==IM_FLOAT32_MOSAIC;
	bw>>=mosaic, bh>>=mosaic;//quarter size of mosaic
	int size=bw*bh;
	float *lum=new float[size], *hue=new float[size];

	//prepare spectrogram
#if 1
	float comp[3];//rgb
	switch(imagetype)
	{
	case IM_INT8_RGBA:
		{
			auto image=(int*)buffer;
			for(int k=0;k<size;++k)
			{
				auto p=(unsigned char*)(image+k);
				//float r=p[0]*inv255, g=p[1]*inv255, b=p[2]*inv255;
				lum[k]=(p[0]+p[1]+p[2])*inv765;

				comp[0]=p[0];
				comp[1]=p[1];
				comp[2]=p[2];
				hue[k]=rgb2hue(comp);
				//int min=0, max=0;
				//if(p[min]>p[1])
				//	min=1;
				//if(p[min]>p[2])
				//	min=2;
				//if(p[max]<p[1])
				//	max=1;
				//if(p[max]<p[2])
				//	max=2;
				//int diff=max-min;
				//if(diff)
				//{
				//	switch(max)
				//	{
				//	case 0:
				//		hue[k]=float(p[1]-p[2])/(max-min);
				//		break;
				//	case 1:
				//		hue[k]=2+float(p[2]-p[0])/(max-min);
				//		break;
				//	case 2:
				//		hue[k]=4+float(p[0]-p[1])/(max-min);
				//		break;
				//	}
				//	hue[k]*=pi3;
				//	if(hue[k]<0)
				//		hue[k]+=_2pi;
				//}
				//else
				//	hue[k]=0;
			}
		}
		break;
	case IM_FLOAT32_MOSAIC://assumes GRBG
		{
			auto image=(float*)buffer;
			int w2=bw<<1;
			for(int ky=0;ky<bh;++ky)
			{
				for(int kx=0;kx<bw;++kx)
				{
					int idx=bw*ky+kx, idx2=idx<<2;

					comp[0]=image[idx2+1];
					comp[1]=(image[idx2]+image[idx2+w2+1])*0.5f;
					comp[2]=image[idx2+w2];
					lum[idx]=(comp[0]+comp[1]+comp[2])*inv3;
					//lum[idx]=(image[idx2]+image[idx2+1]+image[idx2+w2]+image[idx2+w2+1])*0.25f;
					
					hue[idx]=rgb2hue(comp);
				}
			}
		}
		break;
	case IM_FLOAT32_RGBA:
		{
			auto image=(float*)buffer;
			int w2=bw<<1;
			for(int ky=0;ky<bh;++ky)
			{
				for(int kx=0;kx<bw;++kx)
				{
					int idx=bw*ky+kx, idx2=idx<<2;
					comp[0]=image[idx2];
					comp[1]=image[idx2+1];
					comp[2]=image[idx2+2];
					lum[idx]=(comp[0]+comp[1]+comp[2])*inv3;
					//lum[idx]=(image[idx2]+image[idx2+1]+image[idx2+2])*inv3;

					hue[idx]=rgb2hue(comp);
				}
			}
		}
		break;
	}

	float *x=lum, *y=hue;
	for(int k=0;k<size;++k)//
	{
		float l=lum[k], h=hue[k];
		x[k]=l*cos(h);
		y[k]=l*sin(h);
	}
#endif
	
#if 1//image goes from 0 to 20kHz (bottom to top)
	int windowsize=(int)ceil(bh*20020./20000)<<2, step=windowsize>>1;
	int nsamples=windowsize+step*bw;
	float samplerate=44100, Ts=1/samplerate;
	float *sound=new float[nsamples];
	memset(sound, 0, nsamples*sizeof(float));
	DFT_1D dft={};//must initialize pointers to zero to avoid freeing 0xCCCCCCCC
	dft_1d_prepare(windowsize, &dft, DFT_REAL_TIME|DFT_INVERSE|DFT_ALLOC_BUFFERS);//c2r
	float *window=new float[windowsize];
	hann_window(window, windowsize, 1.f/windowsize);
	//memset(dft.freq, 0, windowsize*sizeof(float)<<1);
	memset(dft.time, 0, windowsize*sizeof(float)<<1);//
	int compcount=windowsize<<1;
	for(int kx=0;kx<bw;++kx)
	{
		memset(dft.freq, 0, windowsize*sizeof(float)<<1);
		for(int ky=0;ky<bh;++ky)//fill the freq buffer
		{
			int idx=(bh-1-ky)<<1, idx3=bw*ky+kx;
			dft.freq[idx  ]= x[idx3];
			dft.freq[idx+1]= y[idx3];
			dft.freq[compcount-2-idx  ]= x[idx3];
			dft.freq[compcount-2-idx+1]=-y[idx3];
		}
		//if(kx==(bw>>1))
		//	complex_fbuf_to_clipboard(dft.freq, windowsize);//
		
		//memset(dft.time, 0, windowsize*sizeof(float)<<1);//
		dft_1d_apply(&dft, DFT_INVERSE);
		//if(kx==(bw>>1))
		//	sound_to_clipboard(dft.time, windowsize<<1);//

		int start=step*kx;
		for(int kt=0;kt<windowsize;++kt)
			sound[start+kt]+=dft.time[kt]*window[kt];
			//sound[start+kt]+=dft.time[((windowsize>>1)+kt)%windowsize]*window[kt];//origin in center: pulses

		//sound_to_clipboard(sound, start+windowsize);//
	}
	dft_1d_destroy(&dft);
#endif
#if 0//symmetric about image equator
	int windowsize=(int)ceil(bh*20020./20000)<<1, step=windowsize>>1;
	int nsamples=windowsize+step*bw;
	float samplerate=44100, Ts=1/samplerate;
	float *sound=new float[nsamples];
	memset(sound, 0, nsamples*sizeof(float));
	DFT_1D dft={};//must initialize pointers to zero to avoid freeing 0xCCCCCCCC
	dft_1d_prepare(windowsize, &dft, DFT_REAL_TIME|DFT_INVERSE|DFT_ALLOC_BUFFERS);//c2r
	float *window=new float[windowsize];
	hann_window(window, windowsize, 1.f/windowsize);
	memset(dft.freq, 0, windowsize*sizeof(float)<<1);//complex
	memset(dft.time, 0, windowsize*sizeof(float)<<1);//
	for(int kx=0;kx<bw;++kx)
	{
		for(int ky=0;ky<bh;++ky)//fill the freq buffer
		{
			int idx=ky<<1;
			//int idx=(bh-1-ky)<<1;
			int idx2=((windowsize-1)<<1)-idx, idx3=bw*ky+kx;
			dft.freq[idx]=x[idx3];
			dft.freq[idx+1]=y[idx3];
			dft.freq[idx2]=x[idx3];
			dft.freq[idx2+1]=-y[idx3];
		}
		//complex_fbuf_to_clipboard(dft.freq, windowsize);//
		
		//memset(dft.time, 0, windowsize*sizeof(float)<<1);//
		dft_1d_apply(&dft, DFT_INVERSE);
		//sound_to_clipboard(dft.time, windowsize<<1);//

		int start=step*kx;
		for(int kt=0;kt<windowsize;++kt)
			sound[start+kt]+=dft.time[((windowsize>>1)+kt)%windowsize]*window[kt];//origin in center
			//sound[start+kt]+=dft.time[kt]*window[kt];

		//sound_to_clipboard(sound, start+windowsize);//
	}
	dft_1d_destroy(&dft);
#endif
#if 0
	int windowsize=(int)ceil(bh*20020./20000)<<1, step=windowsize>>1;
	int nsamples=windowsize+step*bw;
	float samplerate=44100, Ts=1/samplerate;
	float *sound=new float[nsamples];
	memset(sound, 0, nsamples<<2);
	DFT_1D dft={};//must initialize pointers to zero to avoid freeing 0xCCCCCCCC
	dft_1d_prepare(windowsize, &dft, DFT_INVERSE|DFT_ALLOC_BUFFERS);
	float *window=new float[windowsize];
	hann_window(window, windowsize);
	memset(dft.freq, 0, windowsize*sizeof(float)<<1);
	for(int kx=0;kx<bw;++kx)
	{
		for(int ky=0;ky<bh;++ky)//fill the freq buffer
		{
			int idx=(bh-1-ky)<<1, idx2=((windowsize-1)<<1)-idx, idx3=bw*ky+kx;
			dft.freq[idx  ]=dft.freq[idx2  ]=x[idx3];
			dft.freq[idx+1]=y[idx3];
			dft.freq[idx2+1]=-y[idx3];
		}
		//memset(dft.freq+bh, 0, (windowsize-(bh<<1))*sizeof(float)<<1);
		//sound_to_clipboard(dft.freq, windowsize<<1);//

		dft_1d_apply(&dft, DFT_INVERSE);
		for(int kt=0;kt<windowsize;++kt)//
		{
			float re=dft.time[kt<<1], im=dft.time[(kt<<1)+1];
			dft.time[kt<<1]=sqrt(re*re+im*im);
			dft.time[(kt<<1)+1]=atan2(im, re);
		}
		complex_fbuf_to_clipboard(dft.time, windowsize);//

		int start=step*kx;
		for(int kt=0;kt<windowsize;++kt)
			sound[start+kt]+=dft.time[kt<<1]*window[kt];
	}
	dft_1d_destroy(&dft);
#endif
#if 0
	int window=1024,
		nsamples=(bw-1)*window;
	float samplerate=44100, Ts=1/samplerate;
	float *sound=new float[nsamples];
	float *temp=new float[window];
	memset(sound, 0, nsamples<<2);
	for(int ky=0;ky<bh;++ky)
	{
		float coeff=_2pi*(20.f*pow(20000.f/20.f, (float)(bh-ky)/bh))*Ts;
		int idx=bw*ky;
		for(int kx=0;kx<bw-1;++kx)//640x336:
		{
			float
				x1=x[idx+kx], xdiff=(x[idx+kx+1]-x1)/window,
				y1=y[idx+kx], ydiff=(y[idx+kx+1]-y1)/window;
			float kt3=coeff*window*kx;
			for(int kt=0, kt2=window*kx;kt<window;++kt, ++kt2, kt3+=coeff)
				sound[kt2]+=(x1+xdiff*kt)*cos(kt3)+(y1+ydiff*kt)*sin(kt3);
		}
	}

/*	int samplesperpx=1024,//steady-state spp
		overlap=512,
		window=samplesperpx+overlap*2,
		nsamples=bw*samplesperpx+overlap*2;
	float samplerate=44100, Ts=1/samplerate, invoverlap=1.f/overlap;
	float *sound=new float[nsamples];
	float *temp=new float[window];
	memset(sound, 0, nsamples<<2);
	for(int kx=0;kx<bw;++kx)
	{
		for(int ky=0;ky<bh;++ky)//640x336: release: 5s, debug: 1min
		{
			int idx=bw*ky+kx;
			float re=x[idx], im=y[idx];
			float coeff=_2pi*(20.f*pow(20000.f/20.f, (float)(bh-ky)/bh))*Ts;
			for(int kt=0;kt<window;++kt)
				temp[kt]=re*cos(coeff*(kx+kt))+im*sin(coeff*(kx+kt));
				//temp[kt]=re*cos(coeff*(kt))+im*sin(coeff*(kt));
			for(int kt=0;kt<overlap;++kt)
			{
				float fade=kt*invoverlap;
				temp[kt]*=fade;
				temp[window-1-kt]*=fade;
			}
			for(int kt=0;kt<window;++kt)
				sound[samplesperpx*kx+kt]+=temp[kt];
		}
	}//*/

	delete[] lum, hue, temp;
#endif
	//sound_to_clipboard(sound, nsamples);//

	//normalize
	float min=sound[0], max=sound[0];
	for(int k=1;k<nsamples;++k)
	{
		float &sample=sound[k];
		if(sample!=sample||!isfinite(sample))
			sample=0;
		if(min>sample)
			min=sample;
		if(max<sample)
			max=sample;
	}
	min=abs(min), max=abs(max);
	float normal=1/(max>min?max:min);
	for(int k=0;k<nsamples;++k)
		sound[k]*=normal;

	//sound_to_clipboard(sound, nsamples);//

	//log_start(LL_CRITICAL);
	//print_fbuf(sound, 1024);//
	//log_pause(LL_CRITICAL);
	//log_end();

	play_sound(sound, nsamples, (int)samplerate, 1);

	delete[] sound;
}
void			scalogram2sound(void *buffer, int bw, int bh, ImageMode imagetype)
{
}