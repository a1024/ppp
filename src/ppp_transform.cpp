#include		"ppp.h"
#include		"generic.h"
void			fliprotate(int *buffer, int bw, int bh, int *&b2, int &w2, int &h2, int fr, int bk)
{
	switch(fr)
	{
	case FLIP_HORIZONTAL:
		w2=bw, h2=bh, b2=(int*)malloc(w2*h2<<2);
		for(int ky=0;ky<h2;++ky)
			for(int kx=0;kx<w2;++kx)
				b2[w2*ky+kx]=buffer[bw*ky+bw-1-kx];
		break;
	case FLIP_VERTICAL:
		w2=bw, h2=bh, b2=(int*)malloc(w2*h2<<2);
		for(int ky=0;ky<h2;++ky)
			for(int kx=0;kx<w2;++kx)
				b2[w2*ky+kx]=buffer[bw*(bh-1-ky)+kx];
		break;
	case ROTATE90:
		w2=bh, h2=bw, b2=(int*)malloc(w2*h2<<2);
		for(int ky=0;ky<h2;++ky)
			for(int kx=0;kx<w2;++kx)
				b2[w2*ky+kx]=buffer[bw*kx+bw-1-ky];
		break;
	case ROTATE180:
		w2=bw, h2=bh, b2=(int*)malloc(w2*h2<<2);
		for(int ky=0;ky<h2;++ky)
			for(int kx=0;kx<w2;++kx)
				b2[w2*ky+kx]=buffer[bw*(bh-1-ky)+bw-1-kx];
		break;
	case ROTATE270:
		w2=bh, h2=bw, b2=(int*)malloc(w2*h2<<2);
		for(int ky=0;ky<h2;++ky)
			for(int kx=0;kx<w2;++kx)
				b2[w2*ky+kx]=buffer[bw*(bh-1-kx)+ky];
		break;
	case ROTATE_ARBITRARY:
		{
			double radians=getwindowdouble(hRotatebox)*torad;
			if(radians)
			{
				double cth=cos(radians), sth=sin(radians);

				Point2d U(0, bh), UR(bw, bh), R(bw, 0);//determine new size
				U.rotate(cth, sth);
				UR.rotate(cth, sth);
				R.rotate(cth, sth);
				w2=(int)ceil(maximum(0, U.x, UR.x, R.x)-minimum(0, U.x, UR.x, R.x)), h2=(int)ceil(maximum(0, U.y, UR.y, R.y)-minimum(0, U.y, UR.y, R.y));

				int size2=w2*h2;
				b2=(int*)malloc(size2<<2);
				for(int k=0;k<size2;++k)
					b2[k]=bk;
				const double half=0.5;
				double bw_2=bw*half, bh_2=bh*half, w2_2=w2*half, h2_2=h2*half;
				if(interpolation_type==I_BILINEAR)
				{
					for(int ky=0;ky<h2;++ky)
					{
						for(int kx=0;kx<w2;++kx)
						{
							double x0=kx-w2_2, y0=ky-h2_2;
							double xd=bw_2+x0*cth+y0*sth, yd=bh_2-x0*sth+y0*cth;
							int x=(int)floor(xd), y=(int)floor(yd);
							double alpha_x=xd-x, alpha_y=yd-y;
							//byte *SW, *SE, *NE, *NW;
							//if((unsigned)x<(unsigned)bw)
							//{
							//	if((unsigned)y<(unsigned)bh)
							//	{
							//	}
							//	else
							//	{
							//	}
							//}
							byte *result=(byte*)(b2+w2*ky+kx),
								*NW=(unsigned)x  <(unsigned)bw&&(unsigned)y  <(unsigned)bh?(byte*)(buffer+bw* y   +x  ):(byte*)&bk,
								*SW=(unsigned)x  <(unsigned)bw&&(unsigned)y+1<(unsigned)bh?(byte*)(buffer+bw*(y+1)+x  ):(byte*)&bk,
								*SE=(unsigned)x+1<(unsigned)bw&&(unsigned)y+1<(unsigned)bh?(byte*)(buffer+bw*(y+1)+x+1):(byte*)&bk,
								*NE=(unsigned)x+1<(unsigned)bw&&(unsigned)y  <(unsigned)bh?(byte*)(buffer+bw* y   +x+1):(byte*)&bk;
							for(int kc=0;kc<4;++kc)
							{
								byte
									N=NW[kc]+int((NE[kc]-NW[kc])*alpha_x),
									S=SW[kc]+int((SE[kc]-SW[kc])*alpha_x);
								result[kc]=N+int((S-N)*alpha_y);
							}
						}
					}
				}
				else
				{
					for(int ky=0;ky<h2;++ky)
					{
						for(int kx=0;kx<w2;++kx)
						{
							double x0=kx-w2_2, y0=ky-h2_2;
							int x1=int(bw_2+x0*cth+y0*sth), y1=int(bh_2-x0*sth+y0*cth);
							//x1%=bw, x1+=bw&-(x1<0);
							//y1%=bh, y1+=bh&-(y1<0);
							if((unsigned)x1<(unsigned)bw&&(unsigned)y1<(unsigned)bh)
								b2[w2*ky+kx]=buffer[bw*y1+x1];
						}
					}
				}
			}
		}
		break;
	}
}
void			fliprotate(int fr)
{
	int *b2=nullptr, w2=0, h2=0;
	if(selection.nonzero())
	{
		if(selection_free)
		{
			fliprotate(sel_buffer, sw, sh, b2, w2, h2, fr, secondarycolor);
			int *b3=nullptr, w3=0, h3=0;
			fliprotate(sel_mask, sw, sh, b3, w3, h3, fr, false);
			free(sel_mask);
			sel_mask=b3;
		}
		else
			fliprotate(sel_buffer, sw, sh, b2, w2, h2, fr, secondarycolor);
		free(sel_buffer);
		sel_buffer=b2, sw=w2, sh=h2;

		selection.sort_coords();
		selection.f=selection.i+Point(sw, sh);
		//if(sel_start.x>sel_end.x)
		//	std::swap(sel_start.x, sel_end.x);
		//if(sel_start.y>sel_end.y)
		//	std::swap(sel_start.y, sel_end.y);
		//sel_end=sel_start+Point(sw, sh);
		//selection_assign();
	}
	else
	{
		fliprotate(image, iw, ih, b2, w2, h2, fr, secondarycolor);
		//if(history[histpos].buffer!=image)//
		//	int LOL_1=0;//
		hist_postmodify(b2, w2, h2);
	}
}
void			stretchskew(int *buffer, int bw, int bh, int *&b2, int &w2, int &h2, double stretchH, double stretchV, double skewH, double skewV, int bk)
{
	double
		cH=cos(skewH), sH=sin(skewH), tH=sH/cH,
		cV=cos(skewV), sV=sin(skewV), tV=sV/cV;
	double
		A=1/stretchH, B=tH/stretchH,
		C=tV/stretchV, D=(1+tH*tV)/stretchV;
	//double
	//	A=1/stretchH, B=-tH/stretchH,
	//	C=-tV/stretchV, D=(1+tH*tV)/stretchV;
	//double
	//	A=1/stretchH, B=-sH/(cH*stretchH),
	//	C=0, D=1/(cH*stretchV);
	//double
	//	A=1/stretchH, B=-sH/cH/stretchH,
	//	C=0, D=1/cH;
	
	double
		A2=1*stretchH, B2=tH*stretchH,
		C2=tV*stretchV, D2=(1+tH*tV)*stretchV;
	Point2d U(0, bh), UR(bw, bh), R(bw, 0);//determine new size
	U.transform(A2, B2, C2, D2);
	UR.transform(A2, B2, C2, D2);
	R.transform(A2, B2, C2, D2);
	w2=(int)ceil(maximum(0, U.x, UR.x, R.x)-minimum(0, U.x, UR.x, R.x)), h2=(int)ceil(maximum(0, U.y, UR.y, R.y)-minimum(0, U.y, UR.y, R.y));
//	w2=bw, h2=bh;//

	int size2=w2*h2;
	b2=(int*)malloc(size2<<2);
	for(int k=0;k<size2;++k)
		b2[k]=bk;
	const double half=0.5;
	double bw_2=bw*half, bh_2=bh*half, w2_2=w2*half, h2_2=h2*half;
	if(interpolation_type==I_BILINEAR)
	{
		for(int ky=0;ky<h2;++ky)
		{
			for(int kx=0;kx<w2;++kx)
			{
				double x0=kx-w2_2, y0=ky-h2_2;
				double xd=bw_2+x0*A+y0*B, yd=bh_2+x0*C+y0*D;
				int x=(int)floor(xd), y=(int)floor(yd);
				double alpha_x=xd-x, alpha_y=yd-y;
				byte *result=(byte*)(b2+w2*ky+kx),
					*NW=(unsigned)x  <(unsigned)bw&&(unsigned)y  <(unsigned)bh?(byte*)(buffer+bw* y   +x  ):(byte*)&bk,
					*SW=(unsigned)x  <(unsigned)bw&&(unsigned)y+1<(unsigned)bh?(byte*)(buffer+bw*(y+1)+x  ):(byte*)&bk,
					*SE=(unsigned)x+1<(unsigned)bw&&(unsigned)y+1<(unsigned)bh?(byte*)(buffer+bw*(y+1)+x+1):(byte*)&bk,
					*NE=(unsigned)x+1<(unsigned)bw&&(unsigned)y  <(unsigned)bh?(byte*)(buffer+bw* y   +x+1):(byte*)&bk;
				for(int kc=0;kc<4;++kc)
				{
					byte
						N=NW[kc]+int((NE[kc]-NW[kc])*alpha_x),
						S=SW[kc]+int((SE[kc]-SW[kc])*alpha_x);
					result[kc]=N+int((S-N)*alpha_y);
				}
			}
		}
	}
	else
	{
		for(int ky=0;ky<h2;++ky)
		{
			for(int kx=0;kx<w2;++kx)
			{
				double x0=kx-w2_2, y0=ky-h2_2;
				int x1=int(bw_2+x0*A+y0*B), y1=int(bh_2+x0*C+y0*D);
				if((unsigned)x1<(unsigned)bw&&(unsigned)y1<(unsigned)bh)
					b2[w2*ky+kx]=buffer[bw*y1+x1];
			}
		}
	}
}
void			stretchskew()
{
	double
		stretchH=getwindowdouble(hStretchSkew[SS_STRETCH_H])*0.01,
		stretchV=getwindowdouble(hStretchSkew[SS_STRETCH_V])*0.01,
		skewH=getwindowdouble(hStretchSkew[SS_SKEW_H])*torad,
		skewV=getwindowdouble(hStretchSkew[SS_SKEW_V])*torad;
	if(stretchH!=1||stretchV!=1||skewH||skewV)
	{
		int *b2=nullptr, w2=0, h2=0;
		if(selection.nonzero())
		{
			if(selection_free)
			{
				stretchskew(sel_buffer, sw, sh, b2, w2, h2, stretchH, stretchV, skewH, skewV, secondarycolor);
				int *b3=nullptr, w3=0, h3=0;
				stretchskew(sel_mask, sw, sh, b3, w3, h3, stretchH, stretchV, skewH, skewV, false);
				free(sel_mask);
				sel_mask=b3;
			}
			else
				stretchskew(sel_buffer, sw, sh, b2, w2, h2, stretchH, stretchV, skewH, skewV, secondarycolor);
			free(sel_buffer);
			sel_buffer=b2, sw=w2, sh=h2;

			selection.sort_coords();
			selection.f=selection.i+Point(sw, sh);
			//if(sel_start.x>sel_end.x)
			//	std::swap(sel_start.x, sel_end.x);
			//if(sel_start.y>sel_end.y)
			//	std::swap(sel_start.y, sel_end.y);
			//sel_end=sel_start+Point(sw, sh);
			//selection_assign();
		}
		else
		{
			stretchskew(image, iw, ih, b2, w2, h2, stretchH, stretchV, skewH, skewV, secondarycolor);
			hist_postmodify(b2, w2, h2);
		}
		//static const char zero[]="0", onehundred[]="100";
		//SetWindowTextA(hStretchHbox, onehundred);
		//SetWindowTextA(hStretchVbox, onehundred);
		//SetWindowTextA(hSkewHbox, zero);
		//SetWindowTextA(hSkewVbox, zero);
	}
}