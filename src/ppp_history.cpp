#include		"ppp.h"
bool			historyOn=true;//TODO
std::vector<HistoryFrame> history;
int				histpos=0;
//int			imbytesize()
//{
//	switch(imagetype)
//	{
//	case IM_INT8_RGBA:
//		return iw*ih*sizeof(int);
//	case IM_FLOAT32_MOSAIC:
//		return iw*ih*sizeof(float);
//	case IM_FLOAT32_RGBA:
//		return iw*ih*sizeof(float)<<2;
//	}
//}
int*			hist_start(int iw, int ih)//current buffer is in history[histpos]
{
	for(int k=0, nsteps=history.size();k<nsteps;++k)
	{
		auto &buf=history[k].buffer;
		free(buf), buf=nullptr;
	}
	history.clear();
	int image_size0=IMWORDSIZE();//local variables
	//int image_size0=iw*ih;
	int *image0=(int*)malloc(image_size0<<2);
	history.push_back(HistoryFrame(image0, iw, ih)), histpos=0;
	return image0;
}
void			hist_clear()
{
	auto frame=history[histpos];
	for(int k=0, nsteps=history.size();k<nsteps;++k)
	{
		if(k==histpos)
			continue;
		auto &buf=history[k].buffer;
		free(buf), buf=nullptr;
	}
	history.clear();
	history.push_back(frame);
	histpos=0;
}
void			hist_premodify(int *&buffer, int nw, int nh)//nw, nh: new dimensions
{
	mark_modified();
	//if(!historyOn)
	//	return;
	iw=nw, ih=nh, image_size=IMWORDSIZE(), buffer=(int*)malloc(image_size<<2);
	auto &oldhframe=history[histpos];
	int *&obuffer=oldhframe.buffer, &ow=oldhframe.iw, &oh=oldhframe.ih;
	if(nh>oh||nw>ow)
		memset(buffer, 0xFF, image_size<<2);
	int mw=minimum(ow, nw), mh=minimum(oh, nh);
	if(nw==ow)
		memcpy(buffer, obuffer, ow*mh<<2);
	else
	{
		for(int ky=0;ky<mh;++ky)
			memcpy(buffer+nw*ky, obuffer+ow*ky, mw<<2);
	}
	if(unsigned(histpos+1)<history.size())//remove redo history
	{
		for(int kh=histpos+1, khEnd=history.size();kh<khEnd;++kh)
			free(history[kh].buffer);
		history.resize(histpos+1);
	}
	history.push_back(HistoryFrame(buffer, iw, ih)), ++histpos;
}
void			hist_postmodify(int *buffer, int nw, int nh)
{
	mark_modified();
	iw=nw, ih=nh, image_size=IMWORDSIZE(), image=buffer;
	if(unsigned(histpos+1)<history.size())//remove redo history
	{
		for(int kh=histpos+1, khEnd=history.size();kh<khEnd;++kh)
			free(history[kh].buffer);
		history.resize(histpos+1);
	}
	history.push_back(HistoryFrame(buffer, iw, ih)), ++histpos;
}
void			hist_undo(int *&buffer)
{
	if(histpos>0)
	{
		--histpos;
		auto &chf=history[histpos];//current history frame
		buffer=chf.buffer, iw=chf.iw, ih=chf.ih, image_size=iw*ih;
	}
}
void			hist_redo(int *&buffer)
{
	if((unsigned)histpos+1<history.size())
	{
		++histpos;
		auto &chf=history[histpos];//current history frame
		buffer=chf.buffer, iw=chf.iw, ih=chf.ih, image_size=iw*ih;
	}
}