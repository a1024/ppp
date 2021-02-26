#include		"ppp.h"
std::vector<HistoryFrame> history;
int				histpos=0;
int*			hist_start(int iw, int ih)//current buffer is in history[histpos]
{
	for(int k=0, nsteps=history.size();k<nsteps;++k)
	{
		auto &buf=history[k].buffer;
		free(buf), buf=nullptr;
	}
	history.clear();
	int image_size=iw*ih;
	int *image=(int*)malloc(image_size<<2);
	history.push_back(HistoryFrame(image, iw, ih)), histpos=0;
	return image;
}
void			hist_premodify(int *&buffer, int nw, int nh)//nw, nh: new dimensions
{
	mark_modified();
	iw=nw, ih=nh, image_size=iw*ih, buffer=(int*)malloc(image_size<<2);
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
	iw=nw, ih=nh, image_size=iw*ih, image=buffer;
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