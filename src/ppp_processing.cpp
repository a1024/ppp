#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif
#include		"ppp.h"
#include		"generic.h"
#include		"stb_image.h"
#include		"lodepng.h"
#include		<locale>
#include		<codecvt>
#include		<iostream>
#include		<sstream>
#include		<algorithm>
#include		<map>
#include		<queue>
#include		<stack>
#include		<assert.h>
const char		file[]=__FILE__;
std::string		to_string(const char *format, ...)
{
	int len=vsprintf_s(g_buf, g_buf_size, format, (char*)(&format+1));
	return g_buf;
}
std::string		get_filename(const char *msg)//path ends with slash		if !size, cancel the operation
{
	std::string path;
	printf(msg);
	for(;;)
	{
		std::cin.clear(); std::cin.sync();
		std::getline(std::cin, path);
		assign_path(path, 0, path.size(), path);
		if(path=="x")
		{
			log_end();
			break;
		}
		if(file_is_readablea(path.c_str())==1)
			break;
		printf("\nNot a file or inaccessible. Try again, or enter \'x\' to cancel the operation.\n\n");
	}
	return path;
}
std::string		get_path(const char *msg)//path ends with slash		if !size, cancel the operation
{
	std::string path;
	printf(msg);
	for(;;)
	{
		std::cin.clear(); std::cin.sync();
		std::getline(std::cin, path);
		assign_path(path, 0, path.size(), path);
		if(path=="x")
		{
			log_end();
			break;
		}
		if(file_is_readablea(path.c_str())==2)
		{
			path+='/';
			break;
		}
		printf("\nNot a folder or inaccessible. Try again, or enter \'x\' to cancel the operation.\n\n");
	}
	return path;
}
struct			Folder//ASCII
{
	_WIN32_FIND_DATAA data;
	void *hSearch;
	std::string path;
	bool open(std::string const &path)//path ends with slash, returns true on success
	{
		this->path=path;
		hSearch=FindFirstFileA((path+'*').c_str(), &data);//skip .
		if(hSearch==INVALID_HANDLE_VALUE)
			return false;
		FindNextFileA(hSearch, &data);//skip ..
		return true;
	}
	bool open_control(std::string const &path)
	{
		bool success=open(path);
		if(!success)
		{
			printf("Invalid path, try again\n");
			log_pause(LL_CRITICAL);
			log_end();
		}
		return success;
	}
	void close()
	{
		int success=FindClose(hSearch);
		SYS_ASSERT(success);
	}
	std::string next_file()//if !size then no more files
	{
		std::string filename;
		int success;
		for(;success=FindNextFileA(hSearch, &data);)
		{
			if(!(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
			{
				filename=path+data.cFileName;
				break;
			}
		}
		return filename;
	}
	unsigned char* next_image(int &ret_width, int &ret_height, int &ret_nch, int nch_desired, std::string *ret_filename)//if nullptr then no more images, call free() each time
	{
		unsigned char *original_image=nullptr;
		std::string filename;
		for(;;)
		{
			filename=next_file();
			if(!filename.size())
				break;
			original_image=stbi_load(filename.c_str(), &ret_width, &ret_height, &ret_nch, nch_desired);//'nch_desired': '1'..'4' to force that many components per pixel, but 'nch' will always be the number that it would have been if you said 0
			if(original_image)
				break;
			printf("Can't open \'%s\'\n", filename.c_str());
		}
		if(ret_filename)
			*ret_filename=std::move(filename);
		return original_image;
	}
};

struct			NoscopeCBInfo
{
	int *buf2;
	unsigned char *im;
	int iw, ih, idx;
	double cx, cy, weight;//result
};
void			noscope_cb(void *data, int x1, int x2, int y)//fill callback
{
	auto p=(NoscopeCBInfo*)data;
	for(int kx=x1, index=p->iw*y+kx;kx<x2;++kx, ++index)
	{
		p->buf2[index]=p->idx;
		int idx3=index*3;
		double weight=p->im[idx3]+p->im[idx3+1]+p->im[idx3+2];
		p->cx+=kx*weight;
		p->cy+=y*weight;
		p->weight+=weight;
	}
}
bool			inside_circle(Point2d &c, double r, Point2d &p)
{
	double dx=p.x-c.x, dy=p.y-c.y;
	return dx*dx+dy*dy<=r*r;
}
void			normalize(double *fimage, int width, int height, int imcount)
{
	double normal=1./imcount;
	for(int k=0, nc=width*height*3;k<nc;++k)
		fimage[k]*=normal;
}
void			render_double3(double *fimage, int width, int height)
{
	int pxcount=width*height;
	std::vector<byte> result2(pxcount<<2);//8bit
	int *im4=(int*)result2.data();
	for(int ki=0, ko=0;ko<pxcount;ki+=3, ++ko)
	{
		auto p=(unsigned char*)(im4+ko);
		p[2]=(int)fimage[ki  ];
		p[1]=(int)fimage[ki+1];
		p[0]=(int)fimage[ki+2];
		p[3]=-1;
	}
	hist_premodify(image, width, height);
	memcpy(image, im4, pxcount<<2);
	render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
	printf("About to save...\n");
	log_pause(LL_CRITICAL);
}
void			save_rgbF64_as_rgba16(double *fimage, int width, int height, std::string const &folder, const char *name)//folder ends with slash
{
	int pxcount=width*height;
	std::vector<byte> result(pxcount<<3);//16bit
	auto *im3=(long long*)result.data();
	for(int ki=0, ko=0;ko<pxcount;ki+=3, ++ko)
	{
		auto p=(unsigned short*)(im3+ko);
		p[0]=(int)fimage[ki  ];
		p[1]=(int)fimage[ki+1];
		p[2]=(int)fimage[ki+2];
		p[3]=-1;
	}
	lodepng::encode(folder+name, result, width, height, LCT_RGBA, 16);
}
int				add_frame(double *fimage, byte *original_image, int masterw, int masterh, char *valid, Point2d const *masterpoints, Point2d const *points)
{
	Point2d const *master[3]={}, *src[3]={};
	int npivots=0;
	for(int k=0;k<3;++k)
	{
		if(valid[k])
		{
			master[npivots]=masterpoints+k;
			src[npivots]=points+k;
			++npivots;
		}
	}
	if(!npivots)
		return 0;
#define		PA(NAME)	(*NAME[0])
#define		PB(NAME)	(*NAME[1])
#define		PC(NAME)	(*NAME[2])
	double matrix[4];
	Point2d AB;
	if(npivots==3)
	{
		AB=PB(master)-PA(master);
		Point2d AC=PC(master)-PA(master);
		double invdet=1/(AB.x*AC.y-AB.y*AC.x);
		matrix[0]= AC.y*invdet,	matrix[1]=-AC.x*invdet;
		matrix[2]=-AB.y*invdet,	matrix[3]= AB.x*invdet;
		//for(int k=0;k<4;++k)
		//	if(!std::isfinite(matrix[k]))
		//		return;
	}
	else if(npivots==2)
		AB=PB(master)-PA(master);
	for(int ky=0;ky<masterh;++ky)
	{
		for(int kx=0;kx<masterw;++kx)
		{
			double fx, fy;
			if(npivots==3)
			{
				double
					dx=kx-PA(master).x, dy=ky-PA(master).y,
					c1=matrix[0]*dx+matrix[1]*dy,
					c2=matrix[2]*dx+matrix[3]*dy;
				fx=PA(src).x+c1*(PB(src).x-PA(src).x)+c2*(PC(src).x-PA(src).x);
				fy=PA(src).y+c1*(PB(src).y-PA(src).y)+c2*(PC(src).y-PA(src).y);
			}
			else if(npivots==2)
			{
				Point2d ab=PB(src)-PA(src);
				double invabs=1/sqrt((AB.x*AB.x+AB.y*AB.y)*(ab.x*ab.x+ab.y*ab.y));
				double
					c=(AB.x*ab.x+AB.y*ab.y)*invabs,
					s=(AB.x*ab.y-AB.y*ab.x)*invabs;
				double dx=kx-PA(master).x, dy=ky-PA(master).y;
				fx=c*dx-s*dy+PA(src).x;
				fy=s*dx+c*dy+PA(src).y;
			}
			else//npivots==1
			{
				fx=kx+PA(master).x-PA(src).x;
				fy=ky+PA(master).y-PA(src).y;
			}
			int ix=(int)floor(fx), iy=(int)floor(fy);
			fx-=floor(fx), fy-=floor(fy);
			bool
				ix0IB=ix>=0&&ix<masterw, ix1IB=ix+1>=0&&ix+1<masterw,
				iy0IB=iy>=0&&iy<masterh, iy1IB=iy+1>=0&&iy+1<masterh;
			int i00=masterw*iy+ix, i01=i00+1, i10=i00+masterw, i11=i10+1;
			i00*=3, i01*=3, i10*=3, i11*=3;
			byte rx[]=
			{
				ix0IB&&iy0IB?original_image[i00  ]:0, ix1IB&&iy0IB?original_image[i01  ]:0,
				ix0IB&&iy1IB?original_image[i10  ]:0, ix1IB&&iy1IB?original_image[i11  ]:0,
			};
			byte gx[]=
			{
				ix0IB&&iy0IB?original_image[i00+1]:0, ix1IB&&iy0IB?original_image[i01+1]:0,
				ix0IB&&iy1IB?original_image[i10+1]:0, ix1IB&&iy1IB?original_image[i11+1]:0,
			};
			byte bx[]=
			{
				ix0IB&&iy0IB?original_image[i00+2]:0, ix1IB&&iy0IB?original_image[i01+2]:0,
				ix0IB&&iy1IB?original_image[i10+2]:0, ix1IB&&iy1IB?original_image[i11+2]:0,
			};
			double
				r0=rx[0]+fx*(rx[1]-rx[0]), r1=rx[2]+fx*(rx[3]-rx[2]), r=r0+fy*(r1-r0),
				g0=gx[0]+fx*(gx[1]-gx[0]), g1=gx[2]+fx*(gx[3]-gx[2]), g=g0+fy*(g1-g0),
				b0=bx[0]+fx*(bx[1]-bx[0]), b1=bx[2]+fx*(bx[3]-bx[2]), b=b0+fy*(b1-b0);
			int idx=(masterw*ky+kx)*3;
			fimage[idx]+=r, fimage[idx+1]+=g, fimage[idx+2]+=b;
		}
	}
	return 1;
#undef		PA
#undef		PB
#undef		PC
}

struct			StarFile
{
	std::string filename;
	std::vector<Point2d> centroids;
	StarFile(std::string const &filename):filename(filename){}
};
struct			StarWorkInfo
{
	int idx;
	Point2d a, b, c;
	StarWorkInfo(int idx, Point2d const &a, Point2d const &b, Point2d const &c):idx(idx), a(a), b(b), c(c){}
};
int				noscope_closest_centroid(StarFile &file, Point2d &master, double r)
{
	int kcc=-1;
	double mindist=-1, r2=r*r;
	for(int kc=0, nc=file.centroids.size();kc<nc;++kc)
	{
		auto &p2=file.centroids[kc];
		double dx=p2.x-master.x, dy=p2.y-master.y, dist2=dx*dx+dy*dy;
		if(dist2<r2)
			if(mindist==-1||mindist>dist2)
				kcc=kc;
	}
	return kcc;
}
void			center_bright_object()
{
	log_start(LL_CRITICAL);
	printf("\nCenter at brightest spot.\n");

	std::string infolder=get_path("Input folder: ");
	if(!infolder.size())
		return;
	Folder workspace;
	if(!workspace.open_control(infolder))
		return;
	//_WIN32_FIND_DATAA data;//open
	//void *hSearch=FindFirstFileA((infolder+'*').c_str(), &data);
	//if(hSearch==INVALID_HANDLE_VALUE)
	//{
	//	printf("Invalid path, try again\n");
	//	log_pause(LL_CRITICAL);
	//	log_end();
	//	return;
	//}

	std::string outfolder=get_path("Output folder: ");
	if(!outfolder.size())
		return;

	printf("Index of first frame (Note: FFmpeg starts count from 1): ");
	int fstart=0;
	scanf_s("%d", &fstart);

	printf("Bright spot brightness threshold [0~255] (recommended 250 for Jupiter, 240 for Saturn): ");
	int threshold=250;
	scanf_s("%d", &threshold);
	if(threshold<0||threshold>=256)
	{
		log_end();
		return;
	}
	printf("Square size in pixels: ");
	int width=0;
	scanf_s("%d", &width);

	std::string filename;
	unsigned char *original_image=nullptr;
	int iw2=0, ih2=0, nch2=0;
	for(int k=0;original_image=workspace.next_image(iw2, ih2, nch2, 4, &filename);++k)
	{
		printf("\rProcessing file %d...\t\t", k);
		auto im2=(int*)original_image;
		//int jsize=iw2*ih2;
		//auto im2=(int*)malloc(jsize<<2);
		//for(int k2=0, kc=0;k2<jsize;++k2, kc+=3)
		//{
		//	auto p=(unsigned char*)(im2+k2);
		//	p[0]=original_image[kc  ];
		//	p[1]=original_image[kc+1];
		//	p[2]=original_image[kc+2];
		//	p[3]=0xFF;
		//}
		int count=0;
		double cx=0, cy=0;
		for(int ky=0;ky<ih2;++ky)
		{
			for(int kx=0;kx<iw2;++kx)
			{
				auto p=(unsigned char*)(im2+iw2*ky+kx);
				if((p[0]>threshold)|(p[1]>threshold)|(p[2]>threshold))
					++count, cx+=kx, cy+=ky;
			}
		}
		int size=width*width, w_2=width>>1;
		int icx=(int)(cx/count)-w_2, icy=(int)(cy/count)-w_2;//top-left corner of crop
		int pxstart=maximum(0, -icx);
		//int xstart=icx, xend=icx+width,
		//	ystart=icy, yend=icy+width;
		//if(xstart<0)
		//	xstart=0;
		//if(xend>width)
		//	xend=width;
		//if(ystart<0)
		//	ystart=0;
		//if(yend>width)
		//	yend=width;
		std::vector<byte> result(size<<2);
		int *im3=(int*)result.data(), black=0xFF000000;
		memfill(im3, &black, size<<2, 1<<2);
		for(int ky=0;ky<width;++ky)
		{
			int jy=icy+ky;
			if(jy>=0&&jy<ih2)
				memcpy(im3+width*ky+pxstart, im2+iw2*jy+maximum(0, icx), (width-pxstart)<<2);
			//{
			//	for(int kx=0;kx<width;++kx)
			//	{
			//		int jx=icx+kx;
			//		if(jx>=0&&jx<iw2)
			//			im3[width*ky+kx]=im2[iw2*jy+jx];
			//	}
			//}
		}
		sprintf_s(g_buf, g_buf_size, "%08d.PNG", fstart+k);
		std::string outfile=outfolder+g_buf;
		lodepng::encode(outfile, result, width, width);
		free(original_image);
		//free(im2);
	}
	//int success=FindNextFileA(hSearch, &data);//skip .
	//for(int k=0;success=FindNextFileA(hSearch, &data);++k)//skip ..
	//{
	//	if(!(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))//for each file
	//	{
	//		//{//
	//		//	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	//		//	std::wstring wide=converter.from_bytes(infolder+data.cFileName);
	//		//	open_media(wide);
	//		//	break;
	//		//}//
	//	}
	//}
	printf("\nDone.\n");
	workspace.close();
	//FindClose(hSearch);
	log_pause(LL_CRITICAL);
	log_end();

/*	log_start(LL_CRITICAL);
	printf("\n\nCenter at brightest spot. Enter -1 to cancel.\nBright spot brightness threshold [0~255]: ");
	scanf_s("%d", &threshold);
	int count=0;
	double cx=0, cy=0;
	if(threshold>=0&&threshold<256)
	{
		for(int ky=0;ky<ih;++ky)
		{
			for(int kx=0;kx<iw;++kx)
			{
				auto p=(unsigned char*)(image+iw*ky+kx);
				if(p[0]>threshold|p[1]>threshold|p[2]>threshold)
					++count, cx+=kx, cy+=ky;
			}
		}
		planetx=cx/count, planety=cy/count;
		render(REDRAW_IMAGE_NOSCROLL, 0, w, 0, h);
	}
	log_end();//*/
}
void			simple_average_stacker()//stack images
{
	log_start(LL_CRITICAL);
	printf("\nSimple average stacker.\n");

	std::string infolder=get_path("Input folder: ");
	if(!infolder.size())
		return;
	Folder workspace;
	if(!workspace.open_control(infolder))
		return;
				
	int width=0, height=0;
	printf("Common width: ");
	scanf_s("%d", &width);
	printf("Common height: ");
	scanf_s("%d", &height);

	int pxcount=width*height, compcount=pxcount*3, imcount=0;
	double *fimage=new double[compcount];
	memset(fimage, 0, compcount<<2);
	
	std::string filename;
	unsigned char *original_image=nullptr;
	int iw2=0, ih2=0, nch2=0;
	for(int ki=0;original_image=workspace.next_image(iw2, ih2, nch2, 3, &filename);++ki)
	{
		printf("\rProcessing file %d...\t\t", ki);
		if(iw2!=width||ih2!=height)
		{
			printf("Skipping frame \'%s\' of wrong size %dx%d != %dx%d\n", workspace.data.cFileName, iw2, ih2, width, height);
			free(original_image);
			continue;
		}
		int isize=iw2*ih2;
		if(nch2!=3&&nch2!=4)
		{
			printf("Skipping frame \'%s\' with %d channels (expecting 4)\n", workspace.data.cFileName, nch2);
			free(original_image);
			continue;
		}
		//if(nch2==3)
		//{
			for(int k2=0;k2<compcount;++k2)
				fimage[k2]+=original_image[k2];
		//}
		//else if(nch2==4)
		//{
		//	int *im2=(int*)original_image;
		//	for(int ki=0, ko=0;ki<isize;++ki, ko+=3)
		//	{
		//		auto p=(unsigned char*)(im2+ki);
		//		fimage[ko  ]+=p[0];
		//		fimage[ko+1]+=p[1];
		//		fimage[ko+2]+=p[2];
		//	}
		//}
		++imcount;
		free(original_image);
	}

	printf("\nNormalizing...\n");
	normalize(fimage, width, height, imcount);
	render_double3(fimage, width, height);
	save_rgbF64_as_rgba16(fimage, width, height, infolder, "out.PNG");
	//save_16bit_and_show(fimage, width, height, infolder+"out.PNG");//infolder ends with slash
	delete[] fimage;
	log_end();
/*	double normal=1./imcount;

	std::vector<byte> result(pxcount<<3);//16bit
	auto *im3=(long long*)result.data();
	for(int ki=0, ko=0;ko<pxcount;ki+=3, ++ko)
	{
		auto p=(unsigned short*)(im3+ko);
		p[0]=(int)(fimage[ki  ]*normal);
		p[1]=(int)(fimage[ki+1]*normal);
		p[2]=(int)(fimage[ki+2]*normal);
		p[3]=0xFF;
	}
	{
		std::vector<byte> result2(pxcount<<2);//8bit
		int *im4=(int*)result2.data();
		for(int ki=0, ko=0;ko<pxcount;ki+=3, ++ko)
		{
			auto p=(unsigned char*)(im4+ko);
			p[2]=(int)(fimage[ki  ]*normal);
			p[1]=(int)(fimage[ki+1]*normal);
			p[0]=(int)(fimage[ki+2]*normal);
			p[3]=0xFF;
		}
		hist_premodify(image, width, height);
		memcpy(image, im4, pxcount<<2);
		render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
		//printf("Done.\n");
		printf("About to save...\n");
		log_pause(LL_CRITICAL);
		//log_end();
	}

	delete[] fimage;
	std::string outfile=infolder+"out.PNG";//infolder ends with slash
	lodepng::encode(outfile, result, width, height, LCT_RGBA, 16);
	log_end();//*/
}
void			stack_sky_images_v1()
{
	printf("\nSky image stacker.\n\nNote: point objects only, won't work with the moon in images\n\n");

	printf("Brightness threshold (~128): ");
	int threshold=0;
	scanf_s("%d", &threshold);
	if(threshold<0||threshold>=256)
	{
		log_end();
		return;
	}

	std::string infolder=get_path("Input folder: ");
	if(!infolder.size())
		return;
	Folder workspace;
	if(!workspace.open_control(infolder))
		return;

	std::string outfolder=get_path("Output folder: ");
	if(!outfolder.size())
		return;

	int masterw=-1, masterh=-1, size=-1;
	std::vector<StarFile> files;
	double nstars=0;
	
	std::string filename;
	unsigned char *original_image=nullptr;
	int iw2=0, ih2=0, nch2=0;
	for(int ki=0;original_image=workspace.next_image(iw2, ih2, nch2, 3, &filename);++ki)//extract points
	{
		//printf("\rOpening file %d...\t\t", ki);
		if(masterw==-1||masterh==-1)
			masterw=iw2, masterh=ih2, size=masterw*masterh;
		if(masterw!=iw2||masterh!=ih2)
		{
			printf("Skipping frame \'%s\' of wrong size %dx%d != %dx%d\n", workspace.data.cFileName, iw2, ih2, masterw, masterh);
			continue;
		}
		files.push_back(StarFile(filename));
		auto &starfile=*files.rbegin();
		//filenames.push_back(filename);
		int *buf2=new int[size];
		for(int k=0, kc=0;k<size;++k, kc+=3)//apply thresholding
		{
			double intensity=(original_image[kc]+original_image[kc+1]+original_image[kc+2])*(1./3);
			buf2[k]=intensity>threshold;
		}
		int star_idx=2;
		NoscopeCBInfo info={buf2, original_image, masterw, masterh};
		for(int ky=0;ky<masterh;++ky)//calculate centroids
		{
			for(int kx=0;kx<masterw;++kx)
			{
				if(buf2[masterw*ky+kx]==1)//eliminate all labels 1 in buf2
				{
					info.idx=star_idx;
					info.cx=info.cy=info.weight=0;
					fill_v2(buf2, masterw, masterh, kx, ky, noscope_cb, &info, nullptr);
					info.cx/=info.weight, info.cy/=info.weight;
					starfile.centroids.push_back(Point2d(info.cx, info.cy));
					++star_idx;
					++nstars;
				}
			}
		}
		printf("\r%d files, %g stars per file...\t\t", ki+1, nstars/(ki+1));
		delete[] buf2;
		free(original_image);
	}//end extract loop
	printf("\n");
	int nfiles=files.size();
	nstars/=nfiles;
	log_pause(LL_CRITICAL);

	printf("\nGenerating image with detected stars...\n");
	std::vector<byte> preview(size<<2);
	int *map=(int*)preview.data();
	int black=0xFF000000;
	memfill(map, &black, size<<2, 1<<2);
	const int minlevel=55,//mininum centroid brightness
		modulus=255-minlevel;
	for(int ki=0;ki<nfiles;++ki)//generate image with centroids
	{
		auto &file=files[ki];
		int r=ki*10%modulus, g=ki*100/modulus%modulus, b=ki*1000/(modulus*modulus);
		int color=0xFF000000|r<<16|g<<8|b;
		color^=0x00FFFFFF;
		for(int kc=0, nc=file.centroids.size();kc<nc;++kc)
		{
			auto &p=file.centroids[kc];
			int ix=(int)p.x, iy=(int)p.y;
			if(ix>=0&&ix<masterw&&iy>=0&&iy<masterh)
				map[masterw*iy+ix]=color;
		}
	}
	std::string outfile=outfolder+"preview.PNG";
	printf("About to write to \'%s\'\n", outfile.c_str());
	log_pause(LL_CRITICAL);
	lodepng::encode(outfile, preview, masterw, masterh, LCT_RGBA, 8);

	Point2d A, B, C;//pivots
	double r1, r2, r3;
	int npivots=3;
	printf("Enter number of pivot stars to use (1, 2, or 3): ");
	scanf_s("%d", &npivots);
	if(npivots<1||npivots>3)
	{
		log_end();
		return;
	}
	printf(
		"\nEnter the coordinates of %d pivot stars from \'preview.PNG\', and %d radii that contain the correct groups.\n"
		"The pivots should be as far from each other as possible.\n%s"
		, npivots, npivots, npivots==3?"The pivots should NOT be collinear.\n":"");
	printf("x1: "), scanf_s("%lf", &A.x);
	printf("y1: "), scanf_s("%lf", &A.y);
	printf("r1: "), scanf_s("%lf", &r1);
	printf("\n");
	if(npivots>1)
	{
		printf("x2: "), scanf_s("%lf", &B.x);
		printf("y2: "), scanf_s("%lf", &B.y);
		printf("r2: "), scanf_s("%lf", &r2);
		printf("\n");
		if(npivots==3)
		{
			printf("x3: "), scanf_s("%lf", &C.x);
			printf("y3: "), scanf_s("%lf", &C.y);
			printf("r3: "), scanf_s("%lf", &r3);
			printf("\n");
		}
	}

	printf("Selecting closest points...\n");
	std::vector<StarWorkInfo> workinfo;
	for(int ki=0;ki<nfiles;++ki)
	{
		auto &file=files[ki];
		int kccA, kccB, kccC;//indices of closest star
		if(npivots==3)
		{
			kccA=noscope_closest_centroid(file, A, r1);
			kccB=noscope_closest_centroid(file, B, r2);
			kccC=noscope_closest_centroid(file, C, r3);
			if(kccA!=-1&&kccB!=-1&&kccC!=-1)
			{
				Point2d &a=file.centroids[kccA], &b=file.centroids[kccB], &c=file.centroids[kccC];
				if(!collinear(a, b, c))
				//if(!collinear(a, b, c)&&inside_circle(A, r1, a)&&inside_circle(B, r2, b)&&inside_circle(C, r3, c))
					workinfo.push_back(StarWorkInfo(ki, a, b, c));
			}
		}
		else if(npivots==2)
		{
			kccA=noscope_closest_centroid(file, A, r1);
			kccB=noscope_closest_centroid(file, B, r2);
			if(kccA!=-1&&kccB!=-1&&kccA!=kccB)
				workinfo.push_back(StarWorkInfo(ki, file.centroids[kccA], file.centroids[kccB], Point2d()));
		}
		else//1 pivot
		{
			kccA=noscope_closest_centroid(file, A, r1);
			if(kccA!=-1)
				workinfo.push_back(StarWorkInfo(ki, file.centroids[kccA], Point2d(), Point2d()));
		}
	}
	printf("%d valid files\n", workinfo.size());
	if(!workinfo.size())
	{
		printf("Stopped.\n");
		log_pause(LL_CRITICAL);
		log_end();
		return;
	}

	printf("Positioning & accumulating images...\n");
	double *im3=new double[size*3];
	memset(im3, 0, size*3<<3);
	
	Point2d masterpoints[]={A, B, C};
	for(int kw=0, nw=workinfo.size();kw<nw;++kw)//position & accumulate images
	{
		auto &work=workinfo[kw];
		auto &file=files[work.idx];
		int iw2=0, ih2=0, nch2=0;
		unsigned char *original_image=stbi_load(file.filename.c_str(), &iw2, &ih2, &nch2, 0);
		if(!original_image)
		{
			printf("Can't open \'%s\'\n", file.filename.c_str());
			continue;
		}
		printf("\rProcessing file %d...\t\t", work.idx);
		char valid[3]={npivots>0, npivots>1, npivots>2};
		add_frame(im3, original_image, masterw, masterh, valid, masterpoints, &work.a);
		free(original_image);
	}
	printf("\nNormalizing...\n");
	normalize(im3, masterw, masterh, workinfo.size());
	render_double3(im3, masterw, masterh);
	save_rgbF64_as_rgba16(im3, masterw, masterh, outfolder, "out.PNG");
	//save_16bit_and_show(im3, masterw, masterh, outfolder+"out.PNG");
	delete[] im3;
	log_end();
}

struct			StarFile_v2
{
	std::string filename;
	std::vector<Point2d> centroids;
	std::vector<int> trajectories;//available trajectories
	StarFile_v2(std::string const &filename):filename(filename){}
	bool trajectory_available(int idx)const
	{
		return std::find(trajectories.begin(), trajectories.end(), idx)!=trajectories.end();
	}
};
struct			NoscopePoint
{
	int frame, point;//indeces in 'files' vector
	NoscopePoint(int frame, int point):frame(frame), point(point){}
};
struct			TrackedObject
{
	std::vector<NoscopePoint> trajectory;
	bool find_point(std::vector<StarFile_v2> const &files, int frame, Point2d &p, const char *error_msg, ...)const
	{
		int ki3=0, ni=trajectory.size();
		for(;ki3<ni;++ki3)
			if(trajectory[ki3].frame==frame)
				break;

		if(ki3<ni)
		{
			p=files[frame].centroids[trajectory[ki3].point];
			return true;
		}
		if(error_msg)
		{
			vprintf(error_msg, (char*)(&error_msg+1));
			log_pause(LL_CRITICAL);
		}
		return false;//unreachable
	}
};
//inline bool operator<(TrackedObject const &a, TrackedObject const &b){return a.trajectory.size()<b.trajectory.size();}
bool			find_pivots(std::vector<StarFile_v2> const &files, int batchsize, int ki, std::vector<TrackedObject> const &objects, std::vector<int> const &objorder, int npivots_wanted, int &npivots, Point2d *masterpoints)
{
	bool found=false;
	if((int)objects.size()>=npivots_wanted)
	{
		int idx[3];
		for(int k=0;k<npivots_wanted;++k)
			idx[k]=objorder[k];
		int ki2=0, nfiles=files.size();
		for(;ki2<batchsize&&ki+ki2<nfiles;++ki2)//find frame with A, B & C
		{
			auto &file=files[ki+ki2];
			found=true;
			for(int k2=0;k2<npivots_wanted;++k2)
				found&=file.trajectory_available(idx[k2]);
			if(found)
				break;
		}
		if(found)
		{
			int frame=ki+ki2;
			npivots=npivots_wanted;

			const char *format="\nError: trajectory %d not in frame %d\n";
			for(int k2=0;k2<npivots;++k2)
				if(!objects[idx[k2]].find_point(files, frame, masterpoints[k2], format, idx[k2], frame))
					found=false;
		}
	}
	return found;
}
void			stack_sky_images_v2(bool loud)
{
	printf("\nSky image stacker v2.\nFor static camera only.\nOutput is placed in subfolder \'out\'.\n\n");

	std::string infolder=get_path("Input folder: ");
	if(!infolder.size())
		return;
	Folder workspace;
	if(!workspace.open_control(infolder))
		return;

	std::string outfolder=infolder+"out";
	if(file_is_readablea(outfolder.c_str()))
	{
		printf("Warning: the folder \'out\' already exists. Overwrite output? [y/n] ");
		char c;
		while(!scanf("%c", &c));
		if(!(c=='y'||c=='Y'))
		{
			log_pause(LL_CRITICAL);
			log_end();
			workspace.close();
		}
	}
	else
	{
		int success=CreateDirectoryA(outfolder.c_str(), nullptr);
		if(!success)
		{
			printf("Failed to create \'out\' folder. Canceling");
			log_pause(LL_CRITICAL);
			log_end();
			workspace.close();
			return;
		}
	}
	outfolder+='/';
	
	printf("Brightness threshold [0~255], recommended 128: ");
	int threshold=0;
	scanf_s("%d", &threshold);
	if(threshold<0||threshold>=256)
	{
		workspace.close();
		log_end();
		return;
	}

	int masterw=-1, masterh=-1, size=-1;
	std::vector<StarFile_v2> files;
	double nstars=0;
	
	std::string filename;
	unsigned char *original_image=nullptr;
	int iw2=0, ih2=0, nch2=0;
	for(int ki=0;original_image=workspace.next_image(iw2, ih2, nch2, 3, &filename);++ki)//read files, extract points
	{
		//printf("\rOpening file %d...\t\t", ki);
		if(masterw==-1||masterh==-1)
			masterw=iw2, masterh=ih2, size=masterw*masterh;
		if(masterw!=iw2||masterh!=ih2)
		{
			printf("Skipping frame \'%s\' of wrong size %dx%d != %dx%d\n", workspace.data.cFileName, iw2, ih2, masterw, masterh);
			continue;
		}
		files.push_back(StarFile_v2(filename));
		auto &starfile=*files.rbegin();
		int *buf2=new int[size];
		for(int k=0, kc=0;k<size;++k, kc+=3)//apply thresholding
		{
			double intensity=(original_image[kc]+original_image[kc+1]+original_image[kc+2])*(1./3);
			buf2[k]=intensity>threshold;
		}
		int star_idx=2;
		NoscopeCBInfo info={buf2, original_image, masterw, masterh};
		for(int ky=0;ky<masterh;++ky)//calculate centroids
		{
			for(int kx=0;kx<masterw;++kx)
			{
				if(buf2[masterw*ky+kx]==1)//eliminate all labels 1 in buf2
				{
					info.idx=star_idx;
					info.cx=info.cy=info.weight=0;
					fill_v2(buf2, masterw, masterh, kx, ky, noscope_cb, &info, nullptr);
					info.cx/=info.weight, info.cy/=info.weight;
					starfile.centroids.push_back(Point2d(info.cx, info.cy));
					++star_idx;
					++nstars;
				}
			}
		}
		printf("\r%d file(s), %g star(s)/file...\t\t", ki+1, nstars/(ki+1));
		delete[] buf2;
		free(original_image);
	}//end extraction loop
	printf("\n");
	int nfiles=files.size();
	if(!nfiles)
	{
		printf("No image files found.\n");
		log_pause(LL_CRITICAL);
		log_end();
		return;
	}
	nstars/=nfiles;
	
	printf("Maximum displacement per frame (recommended 3 pixels): ");
	double maxdisp=3;
	scanf_s("%lf", &maxdisp);

	printf("Enter batch size (recommended 10, or a divisor of frame count, or 0 for one batch): ");
	int batchsize=10;
	scanf_s("%d", &batchsize);
	if(!batchsize)
		batchsize=nfiles;

	printf("\nAbout to write.\n\n");
	log_pause(LL_CRITICAL);

	printf("Tracking & accumulating %d frames in batches of %d...\n", nfiles, batchsize);
	double *fimage=new double[size*3];
	for(int kb=0, ki=0;ki<nfiles;++kb)//for each batch
	{
		std::vector<TrackedObject> objects;
		printf("Batch %d:\n", kb);
		for(int ki2=0;ki2<batchsize&&ki+ki2<nfiles;++ki2)//for each frame in batch			//object rank algorithm
		{
			auto &file=files[ki+ki2];
			for(int kp=0, np=file.centroids.size();kp<np;++kp)//for each point
			{
				auto &p=file.centroids[kp];
				bool found=false;
				int ko=0;
				for(int no=objects.size();ko<no;++ko)//find point
				{
					auto &obj=objects[ko];
					if(obj.trajectory.size())
					{
						auto &p2=*obj.trajectory.rbegin();
						if(inside_circle(files[p2.frame].centroids[p2.point], maxdisp+0.02*(ki+ki2-p2.frame), p))//radius increases by 20px/1000frames
						{
							found=true;
							break;
						}
					}
				}
				if(!found)
				{
					ko=objects.size();
					objects.push_back(TrackedObject());
				}
				objects[ko].trajectory.push_back(NoscopePoint(ki+ki2, kp));
				file.trajectories.push_back(ko);
				printf("\rFile %d/%d, point %d/%d, %d objects\t\t", ki2+1, batchsize, kp+1, np, objects.size());
			}
		}//end tracking loop
		int nobj=objects.size();
		printf("\nSorting %d objects by count of appearance...\n", nobj);
		std::vector<int> objorder(nobj);
		for(int k=0;k<nobj;++k)
			objorder[k]=k;
		std::sort(objorder.begin(), objorder.end(), [&](int idx1, int idx2)
		{
			return objects[idx1].trajectory.size()>objects[idx2].trajectory.size();//descending order of size
		});
		if(loud)
		{
			printf("\n%d trajectories detected.\nTrajectories by count of appearance:\n", nobj);
			for(int ko=0;ko<nobj;++ko)							//print trajectories
			{
				auto &obj=objects[objorder[ko]];
				int pointcount=obj.trajectory.size();
				printf("Object %d appears %d time(s)", ko, pointcount);

				printf("\n");
				for(int kp=0;kp<pointcount;++kp)
				{
					auto &tp=obj.trajectory[kp];
					auto point=&files[tp.frame].centroids[tp.point];
					printf("frame %d [%d](%g,\t%g)\n", tp.frame, tp.point, point->x, point->y);
				}
				//if(pointcount)
				//{
				//	auto &start=obj.trajectory[0];
				//	auto point=&files[start.frame].centroids[start.point];
				//	printf(", frame %d [%d](%g, %g)", start.frame, start.point, point->x, point->y);
				//	if(pointcount>1)
				//	{
				//		auto &end=*obj.trajectory.rbegin();
				//		point=&files[end.frame].centroids[end.point];
				//		printf(" -> frame %d [%d](%g, %g)", end.frame, end.point, point->x, point->y);
				//	}
				//}

				printf("\n");
			}
			printf("\n");
		}

		int npivots=0;
		Point2d masterpoints[3];//up to 3 most persistent objects from same frame (any frame in batch)
		
		bool found=find_pivots(files, batchsize, ki, objects, objorder, 3, npivots, masterpoints);
		if(!found)
			found=find_pivots(files, batchsize, ki, objects, objorder, 2, npivots, masterpoints);
		if(!found)
		{
			found=find_pivots(files, batchsize, ki, objects, objorder, 1, npivots, masterpoints);
			if(found)
				printf("\nWarning: batch %d has only one pivot\n\n", kb);
		}
		if(!found)
		{
			auto &file=files[ki];
			printf("\nError: batch %d (at \'%s\') has no pivots. Skipping.\n\n", kb, file.filename.c_str());
			continue;
		}
		printf("%d pivots\n",  npivots);
		TrackedObject *obj2[3];
		for(int k=0;k<npivots;++k)
			obj2[k]=&objects[objorder[k]];

		memset(fimage, 0, size*3<<3);
		
		int framecount=0, addedframes=0;
		for(;framecount<batchsize&&ki+framecount<nfiles;++framecount)//position & accumulate images in batch
		{
			auto &file=files[ki+framecount];

			char valid[3]={};
			Point2d points[3];
			for(int kp=0;kp<npivots;++kp)
				valid[kp]=obj2[kp]->find_point(files, ki+framecount, points[kp], nullptr);

			printf("\rOpening frame %d/%d...", framecount+1, batchsize);
			int iw2=0, ih2=0, nch2=0;
			unsigned char *original_image=stbi_load(file.filename.c_str(), &iw2, &ih2, &nch2, 0);
			if(!original_image)
			{
				printf("\nCan't open \'%s\'\n", file.filename.c_str());
				continue;
			}
			printf("\tAdding frame...\t\t");
			addedframes+=add_frame(fimage, original_image, masterw, masterh, valid, masterpoints, points);
			free(original_image);
		}//end accumulation loop
		ki+=framecount;

		if(addedframes)
		{
			printf("\nNormalizing...\n");
			normalize(fimage, masterw, masterh, addedframes);
		}
		printf("Saving batch %d/%d...\n", kb+1, nfiles/batchsize+(nfiles%batchsize!=0));
		save_rgbF64_as_rgba16(fimage, masterw, masterh, outfolder, to_string("%08d.PNG", kb+1).c_str());
		//objects.clear();
		printf("\n\n");
	}//end batch loop
	delete[] fimage;

	printf("Done.\n");
	log_pause(LL_CRITICAL);
	log_end();
}

void			stack_sky_images_v3()
{
	printf("Sky Image Stacker v3.\n\n");

	printf("Not implemented yet\n\n");

	printf("Done.\n");
	log_pause(LL_CRITICAL);
	log_end();
}

void			custom_buffer_to_sstream(std::stringstream &LOL_1, char *b, int size, int op_width)
{
	int idx=0;
	for(int ky=0, kyEnd=size/op_width;ky<kyEnd;++ky)
	{
		LOL_1<<'\t';

		for(int kx=0;kx<op_width;++kx, ++idx)		//print as initializer list
		{
			auto &px=((unsigned char*)b)[idx];
			LOL_1<<(unsigned)px<<',';

			//auto &px=b[idx];
			//LOL_1<<(int)px<<',';
		}

		//LOL_1<<'\"';
		//for(int kx=0;kx<op_width;++kx, ++idx)//print as string literal
		//{
		//	auto &px=((unsigned char*)b)[idx];
		//	print_char(LOL_1, px);
		//}
		//LOL_1<<'\"';

		LOL_1<<"\r\n";
	}
	if(idx<size)
	{
		LOL_1<<'\t';
		for(;idx<size;++idx)
		{
			auto &px=((unsigned char*)b)[idx];
			LOL_1<<(unsigned)px<<',';

			//auto &px=b[idx];
			//LOL_1<<(int)px<<',';
		}
	}
}
void			custom_buffer_to_clipboard(char *b, int size, int op_width)
{
	std::stringstream LOL_1;
	custom_buffer_to_sstream(LOL_1, b, size, op_width);
	copy_to_clipboard(LOL_1.str());
}
struct			HuffmanData
{
	int original_size;//in bytes
	int h_size_ints, d_size_bits, d_size_bytes;
	int *histogram;//(-value, positions...) tuples
	unsigned char *data;
	HuffmanData():histogram(nullptr), data(nullptr){}
	~HuffmanData()
	{
		if(histogram)
			free(histogram);
		if(data)
			free(data);
	}
	void to_clipboard()
	{
		std::stringstream LOL_1;
		for(int k=0;k<h_size_ints;++k)
			LOL_1<<histogram[k]<<',';
		LOL_1<<"\r\n\r\n";
		LOL_1<<"d_size_bits="<<d_size_bits<<"\r\n";
		custom_buffer_to_sstream(LOL_1, (char*)data, d_size_bytes, 50);
		copy_to_clipboard(LOL_1.str());
	}
};
struct			Node
{
	Node *branch[2];
	unsigned short value;
	int count;
};
int nodecount=0;
Node*			make_node(int symbol, int count, Node *left, Node *right)//https://gist.github.com/pwxcoo/72d7d3c5c3698371c21e486722f9b34b
{
	++nodecount;
	Node *n=new Node();
	n->value=symbol, n->count=count;
	n->branch[0]=left, n->branch[1]=right;
	return n;
}
Node*			build_tree(int *histogram, int nlevels)
{
	auto cmp=[](Node* const &a, Node* const &b){return a->count>b->count;};
	std::priority_queue<Node*, std::vector<Node*>, decltype(cmp)> pq(cmp);
	for(int k=0;k<nlevels;++k)
		pq.push(make_node(k, histogram[k], nullptr, nullptr));
	while(pq.size()>1)//build Huffman tree
	{
		Node *left=pq.top();	pq.pop();
		Node *right=pq.top();	pq.pop();
		pq.push(make_node(0, left->count+right->count, left, right));
	}
	return pq.top();
}
void			make_alphabet(Node *root, std::vector<bool> str, std::vector<bool> *alphabet)//depth-first
{
	typedef std::pair<Node*, std::vector<bool>> TraverseInfo;
	std::stack<TraverseInfo> s;
	s.push(TraverseInfo(root, std::vector<bool>()));
	while(s.size())
	{
		auto &info=s.top();
		Node *r2=info.first;
		if(!r2)
		{
			s.pop();
			continue;
		}
		if(!r2->branch[0]&&!r2->branch[1])
			alphabet[r2->value]=std::move(info.second);
		std::vector<bool> left=std::move(info.second),
				right=left;
		s.pop();
		left.push_back(0), right.push_back(1);
		if(r2->branch[0])
			s.push(TraverseInfo(r2->branch[0], std::move(left)));
		if(r2->branch[1])
			s.push(TraverseInfo(r2->branch[1], std::move(right)));
	}

/*	typedef std::pair<Node*, std::vector<bool>> TraverseInfo;
	std::stack<TraverseInfo> s;
	s.push(TraverseInfo(root, std::vector<bool>()));
	while(s.size())
	{
		auto &info=s.top();
		Node *r2=info.first;
		if(!r2)
		{
			s.pop();
			continue;
		}
		if(!r2->branch[0]&&!r2->branch[1])
			alphabet[r2->value]=info.second;
		std::vector<bool> left=info.second, right=info.second;
		s.pop();
		left.push_back(0), right.push_back(0);
		s.push(TraverseInfo(r2->branch[0], std::move(left)));
		s.push(TraverseInfo(r2->branch[1], std::move(right)));
	}//*/

/*	if(!root)
		return;
	if(!root->branch[0]&&!root->branch[1])
		alphabet[root->value]=str;
	std::vector<bool> left=str, right=str;
	//std::vector<bool> *left=new std::vector<bool>(str), *right=new std::vector<bool>(str);
	left.push_back(0), right.push_back(1);
	make_alphabet(root->branch[0], left, alphabet);
	make_alphabet(root->branch[1], right, alphabet);
	//delete left, right;//*/
}
void			free_tree(Node *root)
{
	if(root->branch[0])
		free_tree(root->branch[0]);
	if(root->branch[1])
		free_tree(root->branch[1]);
	free(root);
}
void			print_alphabet(std::vector<bool> *alphabet, int *histogram, int nlevels, int symbols_to_compress)
{
	printf("symbol");
	if(histogram)
		printf(", freq, %%");
	printf("\n");
	for(int k=0;k<nlevels;++k)//print alphabet
	{
		printf("%4d ", k);
		if(histogram)
		{
			printf("%6d ", histogram[k]);
			printf("%2d ", histogram[k]*100/symbols_to_compress);
		}
		auto &symbol=alphabet[k];
		for(int k2=0, k2End=symbol.size();k2<k2End;++k2)
			printf("%c", char('0'+symbol[k2]));
		printf("\n");

		//std::cout<<k<<'\t';
		//if(histogram)
		//	std::cout<<histogram[k]<<'\t';
		//auto &symbol=alphabet[k];
		//for(int k2=0, k2End=symbol.size();k2<k2End;++k2)
		//	std::cout<<char('0'+symbol[k2]);
		//std::cout<<'\n';
	}
}
void			huffman_compress(const unsigned short *in, int in_size, HuffmanData &out)
{
	RedirectIOToConsole();//
	const int nsymbols_total=1024;
	int histogram[nsymbols_total]={0};
	for(int k=0;k<in_size;++k)//fill histogram
		++histogram[in[k]];
//	histogram_to_clipboard(histogram);//

	Node *root=build_tree(histogram, nsymbols_total);
	out.original_size=in_size;
	std::vector<bool> alphabet[nsymbols_total];
	make_alphabet(root, std::vector<bool>(), alphabet);

	//for(int k=0;k<nsymbols_total;++k)//print alphabet
	//{
	//	std::cout<<k<<'\t'<<histogram[k]<<'\t';
	//	auto &symbol=alphabet[k];
	//	for(int k2=0, k2End=symbol.size();k2<k2End;++k2)
	//		std::cout<<char('0'+symbol[k2]);
	//	std::cout<<'\n';
	//}

	std::vector<bool> data;
	for(int k=0;k<in_size;++k)//compress data
	{
		auto &symbol=alphabet[in[k]];
		data.insert(data.end(), symbol.begin(), symbol.end());
	}

	//std::cout<<'\n';
	//for(int k=0, size=data.size();k<size;k+=8)//print compressed data
	//{
	//	unsigned c=0;
	//	for(int k2=0;k2<8&&k+k2<size;++k2)
	//		c|=data[k+k2]<<k2;
	//	std::cout<<std::hex<<c;
	//}
	//for(int k=0, size=data.size();k<size;++k)//
	//	std::cout<<char('0'+data[k]);//

	out.d_size_bits=data.size(), out.d_size_bytes=(out.d_size_bits>>3)+((out.d_size_bits&7)!=0);
	{//compress histogram
		std::map<int, std::vector<int>> compressed_hist;
		for(int k=0;k<nsymbols_total;++k)
		{
			if(histogram[k])
			{
				compressed_hist[histogram[k]].push_back(k);
			}
		}
		std::vector<int> nvpp;//(-value, positions...) tuples
		for(auto it=compressed_hist.begin();it!=compressed_hist.end();++it)
		{
			auto &positions=it->second;
			nvpp.push_back(-it->first);
			for(int k=0, kEnd=positions.size();k<kEnd;++k)
				nvpp.push_back(positions[k]);
		}
		out.h_size_ints=nvpp.size();
		out.histogram=(int*)malloc(out.h_size_ints<<2);
		memcpy(out.histogram, &nvpp[0], out.h_size_ints<<2);
	}
	out.data=(unsigned char*)malloc(out.d_size_bytes);
	memset(out.data, 0, out.d_size_bytes);
	for(int kb=0, nbits=data.size();kb<nbits;++kb)
		out.data[kb>>3]|=data[kb]<<(kb&7);
	free_tree(root);
}
void			huffman_decompress(HuffmanData const &in, unsigned char *&out, int &out_size)
{
	RedirectIOToConsole();//
	const int nsymbols_total=1024;
	int histogram[nsymbols_total]={0};
	for(int k=0;;)//uncompress histogram
	{
		int value=in.histogram[k];
		if(value<0)
		{
			value=-value;
			for(++k;k<in.h_size_ints&&in.histogram[k]>=0;++k)
				histogram[in.histogram[k]]=value;
		}
		if(k==in.h_size_ints)
			break;
	}
//	histogram_to_clipboard(histogram);//

	Node *root=build_tree(histogram, nsymbols_total);
	out_size=in.original_size;
	out=(unsigned char*)malloc(out_size);
	memset(out, 0, out_size);//
	int bit_idx=0;
	unsigned char *out2=out;
	std::vector<bool> cdata(in.d_size_bits);
	for(int k=0;k<in.d_size_bits;++k)//copy compressed data into a bitstring
		cdata[k]=in.data[k>>3]>>(k&7)&1;

	for(int nbits=cdata.size();bit_idx<nbits;++bit_idx)
	{
		Node *n=root->branch[cdata[bit_idx]];
		for(int bit=cdata[bit_idx];n;)
		{
			if(!n->branch[bit])
			{
				*out2=(unsigned char)n->value;
				++out2;
			//	std::cout<<std::hex<<(unsigned)n->value<<'\n';
				break;
			}
			++bit_idx;
			bit=cdata[bit_idx];
			n=n->branch[bit];
		}
	}
	//	huffman_decode(root->branch[cdata[bit_idx]], out2, cdata, bit_idx);
	free_tree(root);
}

struct AGHeader//16 bytes
{
	unsigned int ALLG;//'A'|'L'<<8|'L'<<16|'G'<<24
	unsigned int ver;//AG format version = 1 (uses code A: 00, 01, 10, 1100, ...1110, 11110000, ...)
	unsigned int bayerInfo;//'G'|'R'<<8|'B'<<16|'G'<<24 for Galaxy A70
	unsigned short depth;//uncompressed bit depth
	unsigned short nPlanes;//number of planes
	int dim[];//dimensions of plane0, plane1, ...plane[nPlanes-1]
};
struct AGData//8 bytes
{
	unsigned short PL;//'P'|'L'<<8
	unsigned short planeNo;//[0, nPlanes-1]
	int cSize;//compressed size in bits
	byte data[];
};
void			factorize(int x, std::vector<short> &factors)
{
	for(int k=2;x>1;)
	{
		if(!(x%k))
		{
			factors.push_back(k);
			x/=k;
		}
		else
			++k;
	}
}
int				min4(int a, int b, int c, int d)
{
	return minimum(minimum(a, b), minimum(c, d));
}
void push_int(std::vector<byte> &data, int x)
{
	data.insert(data.end(), &x, &x+1);
	//auto p=(byte*)&x;
	//data.insert(data.end(), p, p+4);
}
void compress_plane(std::vector<byte> &file, short *plane, int nSymbols, int kp)
{
	int filesize=file.size();
	unsigned short newPlaneMark='P'|'L'<<8;
	AGData cPlane={newPlaneMark, kp};
	file.insert(file.end(), (byte*)&cPlane, (byte*)(&cPlane+1));
	auto filePlane=(AGData*)(file.data()+filesize);

	int kb=0, data_start=file.size();
	file.resize(data_start+(nSymbols*4>>3));//preliminary resize: an average of 4 bits per pixel
	for(int ks=0;ks<nSymbols;++ks)//code A, big endian
	{
		int pixel=(unsigned)plane[ks];
		byte symbol[4]={}, symbolbits=0;
		if(pixel<3)
			symbol[0]=pixel, symbolbits=2;
		else if(pixel<6)
			symbol[0]=0xC|(pixel-3), symbolbits=4;
		else if(pixel<21)
			symbol[0]=0xF0|(pixel-6), symbolbits=8;
		else if(pixel<276)
			symbol[0]=0xFF, symbol[1]=pixel-21, symbolbits=16;
		else
		{
			pixel-=276;
			symbol[0]=0xFF, symbol[1]=0xFF, symbol[2]=pixel>>16&0xFF, symbol[3]=pixel&0xFF, symbolbits=32;
		}
		int newBitCount=kb+symbolbits,
			newByteCount=(newBitCount>>3)+((newBitCount&7)!=0);
		if(data_start+newByteCount>(int)file.size())
			file.resize(data_start+newByteCount);
		for(int kb2=0;kb2<symbolbits;++kb2)//slow reference loop		//pack bits
		{
			byte srcbit=symbol[kb2>>3]>>(kb2&7)&1;
			auto &dst=file[data_start+((kb+kb2)>>3)];
			dst|=srcbit<<((kb+kb2)&7);
		}
		kb+=symbolbits;
	}

	filePlane->cSize=kb;
}
void compress(const short *image, int width, int height, int depth, std::vector<byte> &file)
{
	std::vector<short> wFactors, hFactors;
	factorize(width, wFactors);
	factorize(height, hFactors);
	while(wFactors.size()<hFactors.size())
		wFactors.push_back(1);
	while(hFactors.size()<wFactors.size())
		hFactors.push_back(1);

	int planesSize=0;
	for(int k=0;k<(int)wFactors.size();++k)
		planesSize+=wFactors[k]*hFactors[k];
	auto planes=new short[planesSize];
	int w2=width>>1, h2=height>>1;
	for(int ky=0;ky<h;ky+=2)//separate the Bayer channels
	{
		for(int kx=0;kx<w;kx+=2)
		{
			planes[width*ky+kx]=image[width*ky+kx];
			planes[width*ky+kx+w2]=image[width*ky+kx+1];
			planes[width*(ky+h2)+kx]=image[width*(ky+1)+kx];
			planes[width*(ky+h2)+kx+w2]=image[width*(ky+1)+kx+1];
		}
	}

	file.resize(sizeof(AGHeader));
	auto header=(AGHeader*)file.data();
	header->ALLG='A'|'L'<<8|'L'<<16|'G'<<24;
	header->ver=1;
	header->bayerInfo='G'|'R'<<8|'B'<<16|'G'<<24;
	header->depth=depth;
	header->nPlanes=wFactors.size();
	union Dimensions
	{
		int x[2];
		byte b[8];
	};
	Dimensions dimensions;
	dimensions.x[0]=width;
	dimensions.x[1]=height;
	//int dimensions[2]={width, height};
	for(int kp=0;kp<(int)wFactors.size();++kp)
	{
		file.insert(file.end(), dimensions.b, dimensions.b+8);
		//file.insert(file.end(), dimensions, dimensions+2);
		dimensions.x[0]/=wFactors[kp];
		dimensions.x[1]/=wFactors[kp];
	}

	int w=width, h=height, planeOffset=0;
	for(int kp=0;kp<(int)wFactors.size()-1;++kp)//for each plane
	{
		//generate plane
		int nextPlaneOffset=planeOffset+w*h;
		int wFactor=wFactors[kp], hFactor=hFactors[kp];
		w2=w/wFactor;
		h2=h/hFactor;
		for(int ky=0, ky2=0;ky<h2;ky+=hFactor, ++ky2)
		{
			for(int kx=0, kx2=0;kx<w2;kx+=wFactor, ++kx2)
			{
				auto &DC=planes[nextPlaneOffset+w2*ky2+kx2];
				auto
					&v00=planes[planeOffset+w*ky+kx],
					&v01=planes[planeOffset+w*ky+kx+1],
					&v10=planes[planeOffset+w*(ky+1)+kx],
					&v11=planes[planeOffset+w*(ky+1)+kx+1];
				DC=min4(v00, v01, v10, v11);
				v00-=DC;
				v01-=DC;
				v10-=DC;
				v11-=DC;
			}
		}

		//compress plane

		planeOffset=nextPlaneOffset;
		w=w2;
		h=h2;

		if(kp==wFactors.size()-2)//compress last plane
		{
		}
	}
	delete[] planes;
}

static void		print_first_16sq_separate(short *buffer, int bw, int bh)
{
	for(int ky=0;ky<16;ky+=2)
	{
		for(int kx=0;kx<16;kx+=2)
			printf(" %2d", buffer[bw*ky+kx]);
		printf(" ");
		for(int kx=0;kx<16;kx+=2)
			printf(" %2d", buffer[bw*ky+kx+1]);
		printf("\n");
	}
	printf("\n");
	for(int ky=0;ky<16;ky+=2)
	{
		for(int kx=0;kx<16;kx+=2)
			printf(" %2d", buffer[bw*(ky+1)+kx]);
		printf(" ");
		for(int kx=0;kx<16;kx+=2)
			printf(" %2d", buffer[bw*(ky+1)+kx+1]);
		printf("\n");
	}
	printf("\n");
}
static void		print_subimage(const int *buffer, int bw, int bh, int x0, int y0, int dx, int dy, int digits)
{
	for(int ky=0;ky<dy;++ky)
	{
		for(int kx=0;kx<dy;++kx)
			printf(" %*d", digits, buffer[bw*(y0+ky)+x0+kx]);
		printf("\n");
	}
	printf("\n");
}
static void		print_first_16sq(const int *buffer, int bw, int bh)
{
	printf("    ");
	for(int k=0;k<16;++k)
		printf(" %2X", k);
	printf("\n");
	for(int ky=0;ky<16;++ky)
	{
		printf("%X   ", ky);
		for(int kx=0;kx<16;++kx)
			printf(" %2d", buffer[bw*ky+kx]);
		printf("\n");
	}
	printf("\n");
}

static void		print_histogram(int *histogram, int nlevels, int scanned_size)
{
	int histmax=0;
	for(int k=0;k<nlevels;++k)
		if(histmax<histogram[k])
			histmax=histogram[k];
	const int consolechars=79-15;
	if(!histmax)
		return;
	for(int k=0;k<nlevels;++k)
	{
		printf("%4d %6d %2d ", k, histogram[k], 100*histogram[k]/scanned_size);
		for(int kr=0, count=histogram[k]*consolechars/histmax;kr<count;++kr)
			printf("*");
		printf("\n");
	}
	printf("\n");
}
static int*		generate_histogram(int *buffer, int size, int &nlevels)//delete it after use
{
	const int maxlevels=1024*1024;
	if(nlevels>maxlevels)
		nlevels=maxlevels;
	auto histogram=new int[nlevels];
	memset(histogram, 0, nlevels<<2);
	for(int k=0;k<size;++k)
	{
		int val=buffer[k];
		if(val==0x80000000||val==0xCDCDCDCD||val==0xCCCCCCCC||val==0xFDFDFDFD||val==0xABABABAB||val==0xFEEEFEEE)
			continue;
		if(val>=0&&val<nlevels)
			++histogram[val];
	}
	return histogram;
}
static int		find_nlevels(const int *buffer, int imsize, bool clamp)
{
	const int nlevels_max=1024*1024;
	int nlevels=-1;
	for(int k=0;k<imsize;++k)
		if(nlevels==-1||nlevels<buffer[k])
			nlevels=buffer[k];
	++nlevels;//account for zero
	if(clamp&&nlevels>nlevels_max)
		nlevels=nlevels_max;
	return nlevels;
}
static void		print_histogram_from_image(int *buffer, int size, int nlevels=-1)//delete it after use
{
	if(nlevels==-1)
		nlevels=find_nlevels(buffer, size, true);
	int* histogram=generate_histogram(buffer, size, nlevels);
	print_histogram(histogram, 128, size);
	delete[] histogram;
}
static void		separate_bayer_channels(const short *src, short *dst, int bw, int bh)
{
	int w2=bw>>1, h2=bh>>1;
	for(int ky=0, ky2=0;ky<bh;ky+=2, ++ky2)//separate the Bayer channels
	{
		for(int kx=0, kx2=0;kx<bw;kx+=2, ++kx2)
		{
			dst[bw*ky2+kx2]=src[bw*ky+kx];
			dst[bw*ky2+kx2+w2]=src[bw*ky+kx+1];
			dst[bw*(ky2+h2)+kx2]=src[bw*(ky+1)+kx];
			dst[bw*(ky2+h2)+kx2+w2]=src[bw*(ky+1)+kx+1];
		}
	}
}
static void		separate_bayer_channels(const short *src, int *dst, int bw, int bh)
{
	int w2=bw>>1, h2=bh>>1;
	for(int ky=0, ky2=0;ky<bh;ky+=2, ++ky2)//separate the Bayer channels
	{
		for(int kx=0, kx2=0;kx<bw;kx+=2, ++kx2)
		{
			dst[bw*ky2+kx2]=src[bw*ky+kx];
			dst[bw*ky2+kx2+w2]=src[bw*ky+kx+1];
			dst[bw*(ky2+h2)+kx2]=src[bw*(ky+1)+kx];
			dst[bw*(ky2+h2)+kx2+w2]=src[bw*(ky+1)+kx+1];
		}
	}
}
static void		generate_block_sizes(std::vector<short> &factors)
{
	//int min_block_size=8;
	int min_block_size=2;
	for(int k=0;k+1<(int)factors.size();)
	{
		if(factors[k]<min_block_size)
		{
			factors[k]*=factors[k+1];
			factors.erase(factors.begin()+k+1);
		}
		else
			++k;
	}
}

static void		print_block(const int *buffer, int bw, int bh, int x0, int y0, int dx, int dy, const char *msg, ...)
{
	if(msg)
		vprintf(msg, (char*)(&msg+1));
	for(int ky=0;ky<dy;++ky)
	{
		for(int kx=0;kx<dx;++kx)
		{
			unsigned val=buffer[bw*(y0+ky)+x0+kx];
			if(val==0x80000000||val==0xCDCDCDCD||val==0xFEEEFEEE||val==0xCCCCCCCC)
				printf("-1 ");
			else
				printf("%3d ", val);
		}
		printf("\n");
	}
	printf("\n");
}
//static void	smad0(const int *buffer, int bw, int bh, int *current, int currw, int currh, int logcw, int logch, int *next, int nextw, int nexth)
static void		smad0(const int *buffer, int bw, int bh, int *current, int currw, int currh, int *next, int nextw, int nexth, int planeNo)
{//swapped min among deltas		current & next dimensions: block sizes, not out buffers sizes
	int w2=bw/currw, h2=bh/currh, currentcount=currw*currh-1;
	int max=0;
	for(int ky=0, ky2=0;ky<bh;ky+=currh, ++ky2)//for each block		//(x,y) top-left corner of block in original buffer
	{
		for(int kx=0, kx2=0;kx<bw;kx+=currw, ++kx2)					//(x2,y2) block idx
		{
			int min=-1, xmin=0, ymin=0;
			//print_block(buffer, bw, bh, kx, ky, currw, currh, "Source block (%d, %d):\n", kx2, ky2);
			for(int kyb=0;kyb<currh;++kyb)//find min value			//(xb,yb) point inside the block
			{
				for(int kxb=0;kxb<currw;++kxb)
				{
					int val=buffer[bw*(ky+kyb)+kx+kxb];
					if(min==-1||min>val)
						min=val, xmin=kxb, ymin=kyb;
				}
			}
			for(int kyb=0, kd=0;kyb<currh;++kyb)//store difference in current plane
			{
				for(int kxb=0;kxb<currw;++kxb)						//kd: position in result group (of block size - 1)
				{
					if(kxb!=xmin||kyb!=ymin)
					{
						auto &val=current[currentcount*(w2*ky2+kx2)+kd];
						val=buffer[bw*(ky+kyb)+kx+kxb]-min;
						if(max<val)
							max=val;
						++kd;
					}
				}
			}
			//print_block(current, currentcount*w2, h2, 0, 0, currentcount*w2, h2, "Result:\n", kx2, ky2);
			//print_block(current, currentcount*w2, h2, currentcount*kx2, currentcount*ky2, currentcount, 1, "Result:\n", kx2, ky2);
			next[w2*ky2+kx2]=currw*(currh*min+ymin)+xmin;//store min with its location
			//next[w2*ky2+kx2]=min<<(logcw+logch)|ymin<<logcw|xmin;
		}
	}
#if 0
	if(max>1048576)
		max=1048576;
	int nlevels=max;
	auto histogram=new int[nlevels];
	memset(histogram, 0, nlevels<<2);
	int planesize=w2*h2*currentcount;

	//for(int k=0;k<planesize;++k)
	//	printf("%08X ", current[k]);

	for(int k=0;k<planesize;++k)
		if(current[k]<nlevels)
			++histogram[current[k]];
	printf("Plane %d:\n", planeNo);
	print_histogram(histogram, 128, planesize);
	delete[] histogram;
#endif
}
static int*		smad(const short *buffer, int bw, int bh)
{
	std::vector<short> wFactors, hFactors;
	factorize(bw, wFactors);
	factorize(bh, hFactors);
	generate_block_sizes(wFactors);
	generate_block_sizes(hFactors);
	if(*wFactors.rbegin()!=1)
		wFactors.push_back(1);
	if(*hFactors.rbegin()!=1)
		hFactors.push_back(1);
	while(wFactors.size()<hFactors.size())
		wFactors.push_back(1);
	while(hFactors.size()<wFactors.size())
		hFactors.push_back(1);

	int imsize=bw*bh;
	auto dst=new int[imsize];

	separate_bayer_channels(buffer, dst, bw, bh);
	//print_histogram_from_image(dst, imsize, 1024);//

	double sum=0;
	for(int k=0;k<imsize;++k)
		sum+=buffer[k];
	printf("Energy before: %g\n", sum);
	
	auto result=new int[imsize], temp=new int[imsize];
	int offset=0;
	int w2=bw, h2=bh;
	for(int kp=0;kp<(int)wFactors.size()-1;++kp)
	{
		int blockw=wFactors[kp], blockh=hFactors[kp];
		smad0(dst, w2, h2, result+offset, blockw, blockh, temp, wFactors[kp+1], wFactors[kp+1], kp);
		offset+=w2*h2/(blockw*blockh);
		w2/=blockw;
		h2/=blockh;
		std::swap(dst, temp);
	}
	delete[] dst, temp;

	sum=0;
	for(int k=0;k<imsize;++k)
	{
		int val=result[k];
		if(val!=0x80000000&&val!=0xCDCDCDCD&&val!=0xFEEEFEEE&&val!=0xCCCCCCCC)
			sum+=val;
	}
	printf("Energy after: %g\n", sum);
	
	int max=0;
	for(int k=0;k<imsize;++k)
		if(max<result[k])
			max=result[k];
	printf("Max value = %d\n", max);

	int *histogram=generate_histogram(result, imsize, max);
	const int alphabet_size=128;
	Node *root=build_tree(histogram, alphabet_size);
	std::vector<bool> *alphabet=new std::vector<bool>[alphabet_size];
	make_alphabet(root, std::vector<bool>(), alphabet);
	free_tree(root);
	print_alphabet(alphabet, histogram, alphabet_size, imsize);

	int bitsize=0;
	for(int k=0;k<alphabet_size;++k)//estimate compressed size
		bitsize+=alphabet[k].size()*histogram[k];
	printf("Uncompressed: %9d bits\n", bw*bh*10);
	printf("SMAD+Huffman: %9d bits\n", bitsize);

	delete[] histogram;

	return result;
}
static void		estimate_huffman(const int *buffer, int buf_size, int nlevels=-1)
{
	//int nlevels_max=1024*1024;
	//int imsize=bw*bh;
	if(nlevels==-1)
		nlevels=find_nlevels(buffer, buf_size, true);
	//if(nlevels>nlevels_max)
	//	nlevels=nlevels_max;
	int *histogram=new int[nlevels];
	memset(histogram, 0, nlevels<<2);
	//for(int ky=0;ky<bh;ky+=2)
	//	for(int kx=0;kx<bw;kx+=2)
	//		++histogram[buffer[bw*ky+kx]];
	for(int k=0;k<buf_size;++k)
	{
		int val=buffer[k];
		if(val>=0&&val<nlevels)
			++histogram[val];
	}

	const int alphabet_size=nlevels>128?128:nlevels;
	Node *root=build_tree(histogram, alphabet_size);
	std::vector<bool> *alphabet=new std::vector<bool>[alphabet_size];
	make_alphabet(root, std::vector<bool>(), alphabet);
	free_tree(root);
	print_alphabet(alphabet, histogram, alphabet_size, buf_size);
	
	int bitsize=0;
	for(int k=0;k<alphabet_size;++k)
		bitsize+=alphabet[k].size()*histogram[k];
	printf("\tHuffman:   %8d bits\n", bitsize);
	//printf("Estimated size for 1st Bayer channel:\n");
	//printf("\tJust Huffman:   %8d bits\n", bitsize);
	delete[] histogram;
}
static void		estimate_huffman(const byte *buffer, int buf_size, int nlevels=-1)
{
	if(nlevels==-1)
	{
		int nlevels=-1;
		for(int k=0;k<buf_size;++k)
			if(nlevels==-1||nlevels<buffer[k])
				nlevels=buffer[k];
	}
	int *histogram=new int[nlevels];
	memset(histogram, 0, nlevels<<2);
	for(int k=0;k<buf_size;++k)
	{
		int val=buffer[k];
		if(val>=0&&val<nlevels)
			++histogram[val];
	}

	Node *root=build_tree(histogram, nlevels);
	std::vector<bool> *alphabet=new std::vector<bool>[nlevels];
	make_alphabet(root, std::vector<bool>(), alphabet);
	free_tree(root);
	print_alphabet(alphabet, histogram, nlevels, buf_size);
	
	int bitsize=0;
	for(int k=0;k<nlevels;++k)
		bitsize+=alphabet[k].size()*histogram[k];
	printf("\tHuffman:   %8d bits\n", bitsize);
	//printf("Estimated size for 1st Bayer channel:\n");
	//printf("\tJust Huffman:   %8d bits\n", bitsize);
	delete[] histogram;
}

#define			CEIL_UNITS(BIT_SIZE)		(((BIT_SIZE)>>LBPU)+(((BIT_SIZE)&BPU_MASK)!=0))
#define			BYTE_SIZE(BIT_SIZE)			(CEIL_UNITS(BIT_SIZE)<<LOG_BYTES_PER_UNIT)
struct			vector_bool
{
	static const int
		LOG_BYTES_PER_UNIT=2,

		LBPU=LOG_BYTES_PER_UNIT+3,
		BPU_MASK=(1<<LBPU)-1,
		UNIT_BYTES=1<<(LBPU-3);

	std::vector<int> data;
	int bitSize;
	vector_bool():bitSize(0){}//default constructor
	void realloc(int newBitSize)
	{
		assert(newBitSize>=0);
		if(CEIL_UNITS(newBitSize)!=CEIL_UNITS(bitSize))
		{
			int newSize=BYTE_SIZE(newBitSize);
			data.resize(newSize);
		}
	}
	void clear_tail()//call at the end before retrieving bits
	{
		data[(bitSize-1)>>LBPU]&=(1<<(bitSize&BPU_MASK))-1;//container of last bit
	}
	int size_bytes()
	{
		return BYTE_SIZE(bitSize);
	}
	void set(int bitIdx, bool bit)
	{
		data[bitIdx>>LBPU]|=(bit!=false)<<(bitIdx&BPU_MASK);
	}
	bool get(int bitIdx)
	{
		return data[bitIdx>>LBPU]>>(bitIdx&BPU_MASK)&1;
	}
	void push_back(bool val)
	{
		realloc(bitSize+1);
		if(bitSize&BPU_MASK)
			data[bitSize>>LBPU]|=(val!=false)<<(bitSize&BPU_MASK);
		else
			data[bitSize>>LBPU]=val!=false;
		++bitSize;
	}
	void push_back(vector_bool const &v)
	{
		if(v.bitSize<1)
			return;
		realloc(bitSize+v.bitSize);
		int remainingInUnit=BPU_MASK-(bitSize&BPU_MASK);
		
		int floorUnitCount=v.bitSize>>LBPU;
		if(remainingInUnit>0&&remainingInUnit<BPU_MASK+1)
		{
			data[bitSize>>LBPU]|=v.data[0]<<(BPU_MASK-remainingInUnit);//fill till the end of unit
			for(int k=1;k<floorUnitCount;++k)//fill the rest units
				data[(bitSize>>LBPU)+k]=v.data[k]<<(BPU_MASK-remainingInUnit)|v.data[k-1]>>remainingInUnit;
		}
		else
		{
			for(int k=0;k<floorUnitCount;++k)//assign all
				data[(bitSize>>LBPU)+k]=v.data[k];
		}

		bitSize+=v.bitSize;
	}
};
#if 0
#define			CEIL_UNITS(BIT_SIZE)		(((BIT_SIZE)>>LBPU)+(((BIT_SIZE)&BPU_MASK)!=0))
#define			BYTE_SIZE(BIT_SIZE)			(CEIL_UNITS(BIT_SIZE)<<LOG_BYTES_PER_UNIT)
struct			vector_bool
{
	static const int
		LOG_BYTES_PER_UNIT=2,

		LBPU=LOG_BYTES_PER_UNIT+3,
		BPU_MASK=(1<<LBPU)-1,
		UNIT_BYTES=1<<(LBPU-3);

	int *data;//int must be 32bit
	int bitSize;
#ifdef _DEBUG
	int allocBytes;
	void check_bit_access(int bitIdx)
	{
		if((bitIdx>>3)>=allocBytes)
			int LOL_1=0;
		assert((bitIdx>>3)<allocBytes);
	}
	void check_int_access(int intIdx)
	{
		if((intIdx<<2)>=allocBytes)
			int LOL_2=0;
		assert((intIdx<<2)<allocBytes);
	}
#else
#define	check_bit_access(...)
#define	check_int_access(...)
#endif
	vector_bool():bitSize(0), data(nullptr)//default constructor
#ifdef _DEBUG
		, allocBytes(0)
#endif
	{}
	vector_bool(vector_bool const &v):bitSize(0), data(nullptr)//copy constructor
#ifdef _DEBUG
		, allocBytes(0)
#endif
	{
		realloc(v.bitSize);
		bitSize=v.bitSize;
#ifdef _DEBUG
		allocBytes=v.allocBytes;
#endif
		if(bitSize)
		{
			check_bit_access(bitSize-1);
			memcpy(data, v.data, BYTE_SIZE(bitSize));
		}
	}
	vector_bool(vector_bool &&v)//move constructor
	{
		if(&v!=this)
		{
			data=v.data;
			bitSize=v.bitSize;
#ifdef _DEBUG
			allocBytes=v.allocBytes;
#endif
			v.data=nullptr;
			v.bitSize=0;
		}
	}
	~vector_bool()//destructor
	{
		if(data)
			free(data);
		data=nullptr;
		bitSize=0;
#ifdef _DEBUG
		allocBytes=0;
#endif
	}
	vector_bool& operator=(vector_bool const &v)//assignment
	{
		if(&v!=this)
		{
			if(!v.bitSize)
				int LOL_1=0;
			realloc(v.bitSize);
			bitSize=v.bitSize;
			if(bitSize)
			{
				check_bit_access(bitSize-1);
				memcpy(data, v.data, BYTE_SIZE(bitSize));
			}
		}
		return *this;
	}
	vector_bool& operator=(vector_bool &&v)//move assignment
	{
		if(&v!=this)
		{
			data=v.data;
			bitSize=v.bitSize;
			v.data=nullptr;
			v.bitSize=0;
		}
		return *this;
	}
	void realloc(int newBitSize)
	{
		assert(newBitSize>=0);
		if(CEIL_UNITS(newBitSize)!=CEIL_UNITS(bitSize))
		{
			int newSize=BYTE_SIZE(newBitSize);
#ifdef _DEBUG
			allocBytes=newSize;
			if(newBitSize>32)
				int LOL_3=0;
#endif
			int *newData=(int*)::realloc(data, newSize);
			if(newSize&&!newData)
				return;
			//assert(newData);
			data=newData;
		}//note: realloc returns 0xCDCDCDCD
	}
	void clear_tail()//call at the end before retrieving bits
	{
		check_bit_access(bitSize-1);
		data[(bitSize-1)>>LBPU]&=(1<<(bitSize&BPU_MASK))-1;//container of last bit
	}
	int size_bytes()
	{
		return BYTE_SIZE(bitSize);
		//return (bitSize>>3)+((bitSize&7)!=0);
	}
	void set(int bitIdx, bool bit)
	{
		check_bit_access(bitIdx);
		data[bitIdx>>LBPU]|=(bit!=false)<<(bitIdx&BPU_MASK);
	}
	bool get(int bitIdx)
	{
		check_bit_access(bitIdx);
		return data[bitIdx>>LBPU]>>(bitIdx&BPU_MASK)&1;
	}
	void push_back(bool val)
	{
		realloc(bitSize+1);
		check_bit_access(bitSize);
		if(bitSize&BPU_MASK)
			data[bitSize>>LBPU]|=(val!=false)<<(bitSize&BPU_MASK);
		else
			data[bitSize>>LBPU]=val!=false;
		++bitSize;
	}
	void push_back(vector_bool const &v)
	{
		if(v.bitSize<1)
			return;
		realloc(bitSize+v.bitSize);
		int remainingInUnit=BPU_MASK-(bitSize&BPU_MASK);
		//int ceilBitSize=(bitSize&~BPU_MASK)+(((bitSize&BPU_MASK)!=0)<<LBPU);
		//int remainingInUnit=ceilBitSize-bitSize;
		
		int floorUnitCount=v.bitSize>>LBPU;
		if(remainingInUnit>0&&remainingInUnit<BPU_MASK+1)
		{
			data[bitSize>>LBPU]|=v.data[0]<<(BPU_MASK-remainingInUnit);//fill till the end of unit
			for(int k=1;k<floorUnitCount;++k)//fill the rest units
			{
				check_int_access((bitSize>>LBPU)+k);
				data[(bitSize>>LBPU)+k]=v.data[k]<<(BPU_MASK-remainingInUnit)|v.data[k-1]>>remainingInUnit;
			}
		}
		else
		{
			for(int k=0;k<floorUnitCount;++k)//assign all
			{
				check_int_access((bitSize>>LBPU)+k);
				data[(bitSize>>LBPU)+k]=v.data[k];
			}
		}

		bitSize+=v.bitSize;
	}
};
#endif
struct			HuffHeader
{
	char HUFF[4];//'H'|'U'<<8|'F'<<16|'F'<<24
	unsigned version;//1
	unsigned width, height;//uncompressed dimensions
	unsigned bayerInfo;//'G'|'R'<<8|'B'<<16|'G'<<24 for Galaxy A70
	unsigned nLevels;//1<<bitdepth, also histogram size
	unsigned histogram[];//data begins at histogram+nLevels
};
struct			HuffDataHeader
{
	char DATA[4];//'D'|'A'<<8|'T'<<16|'A'<<24
	unsigned long long bitSize;//compressed data size in bits
	unsigned data[];
};
int recursion_depth=0;

#if 0
std::vector<char> bits;
void			print_tree(Node *root)
{
	if(root->value)
	{
		printf("%d:\t", root->value);
		for(int k=0;k<(int)bits.size();++k)
			printf("%d", (int)bits[k]);
		bits.clear();
		printf("\n");
	}
/*	if(!root->branch[0]&&!root->branch[1])
	{
		printf("%d:\t", root->value);
		for(int k=0;k<(int)bits.size();++k)
			printf("%d", (int)bits[k]);
		bits.clear();
		printf("\n");
	}
	if(root->branch[0])
	{
		bits.push_back(0);
		print_tree(root->branch[0]);
	}
	if(root->branch[1])
	{
		bits.push_back(1);
		print_tree(root->branch[1]);
	}//*/

/*	//if(recursion_depth>1024)
	//{
	//	printf("Recursion depth=1024\n");
	//	return;
	//}
	++recursion_depth;
	if(!root->branch[0]&&!root->branch[1])
		printf("\n");
	if(root->branch[0])
	{
		printf("0");
		//printf("<");
		print_tree(root->branch[0]);
		//printf(">");
	}
	if(root->branch[1])
	{
		printf("1");
		//printf("[");
		print_tree(root->branch[1]);
		//printf("]");
	}//*/
}
#endif
void			print_alphabet(vector_bool *alphabet, int *histogram, int nLevels, int total_count)
{
	printf("symbol");
	if(histogram)
		printf(", freq, %%");
	printf("\n");
	for(int k=0;k<nLevels;++k)//print alphabet
	{
		printf("%4d ", k);
		if(histogram)
		{
			printf("%6d ", histogram[k]);
			printf("%2d ", histogram[k]*100/total_count);
		}
		auto &symbol=alphabet[k];
		for(int k2=0, k2End=symbol.bitSize;k2<k2End;++k2)
			printf("%c", char('0'+symbol.get(k2)));
		printf("\n");
	}
}
void			make_alphabet(Node *root, vector_bool str, vector_bool *alphabet)//depth-first
{
	//print_tree(root);//

	typedef std::pair<Node*, vector_bool> TraverseInfo;
	std::stack<TraverseInfo> s;
	s.push(TraverseInfo(root, vector_bool()));
	vector_bool left, right;
	while(s.size())
	{
		auto &info=s.top();
		Node *r2=info.first;
		if(!r2)
		{
			s.pop();
			continue;
		}
		if(!r2->branch[0]&&!r2->branch[1])
			alphabet[r2->value]=std::move(info.second);
		//left.data=nullptr;//
		left=std::move(info.second);
		right=left;
		s.pop();
		left.push_back(0), right.push_back(1);
		if(r2->branch[0])
			s.push(TraverseInfo(r2->branch[0], std::move(left)));
		if(r2->branch[1])
			s.push(TraverseInfo(r2->branch[1], std::move(right)));
	}
}
void			calculate_histogram(const short *image, int size, int *histogram, int nLevels)
{
	memset(histogram, 0, nLevels*sizeof(int));
	for(int k=0;k<size;++k)
		++histogram[image[k]];
}
static int		compress_huff(const short *buffer, int bw, int bh, int depth, int bayer, std::vector<int> &data)
{
	short *temp=nullptr;
	const short *b2;
	int width, height, imSize;
	if(bayer)//raw color
		width=bw, height=bh, imSize=width*height, b2=buffer;
	else//grayscale
	{
		width=bw>>1, height=bh>>1, imSize=width*height;
		depth+=2;
		temp=new short[imSize];
		for(int ky=0;ky<height;++ky)
		{
			int ky2=ky<<1;
			const short *row=buffer+bw*ky2, *row2=buffer+bw*(ky2+1);
			for(int kx=0;kx<width;++kx)
			{
				int kx2=kx<<1;
				temp[width*ky+kx]=row[kx2]+row[kx2+1]+row2[kx2]+row2[kx2+1];
			}
		}
		b2=temp;
	}
	int nLevels=1<<depth;

	data.resize(sizeof(HuffHeader)+nLevels);
	auto header=(HuffHeader*)data.data();
	header->HUFF[0]='H';
	header->HUFF[1]='U';
	header->HUFF[2]='F';
	header->HUFF[3]='F';
	header->version=1;
	header->width=width;
	header->height=height;
	header->bayerInfo=bayer;
	header->nLevels=nLevels;

	int *histogram=(int*)((HuffHeader*)data.data())->histogram;
	calculate_histogram(b2, imSize, histogram, nLevels);


	Node *root=build_tree(histogram, nLevels);
	vector_bool *alphabet=new vector_bool[nLevels];
	make_alphabet(root, vector_bool(), alphabet);
	free_tree(root);
	//print_alphabet(alphabet, histogram, nLevels, imSize);

	//Node *root=build_tree(histogram, 256);
	//vector_bool *alphabet=new vector_bool[256];
	//make_alphabet(root, vector_bool(), alphabet);
	//free_tree(root);
	//print_alphabet(alphabet, histogram, 256, imSize);

	//Node *root=build_tree(histogram, 256);
	//std::vector<bool> alphabet[256];
	//make_alphabet(root, std::vector<bool>(), alphabet);
	//free_tree(root);
	//print_alphabet(alphabet, histogram, 256, imSize);
	//return 0;

	vector_bool bits;
	for(int k=0;k<imSize;++k)
		bits.push_back(alphabet[b2[k]]);
	bits.clear_tail();
	int data_start=data.size();
	data.resize(data_start+sizeof(HuffDataHeader)+bits.size_bytes()/sizeof(int));
	auto dataHeader=(HuffDataHeader*)(data.data()+data_start);
	dataHeader->DATA[0]='D';
	dataHeader->DATA[1]='A';
	dataHeader->DATA[2]='T';
	dataHeader->DATA[3]='A';
	dataHeader->bitSize=bits.bitSize;
	memcpy(dataHeader->data, bits.data.data(), bits.size_bytes());
	//memcpy(dataHeader->data, bits.data, bits.size_bytes());

	delete[] alphabet;
	if(!bayer)
	{
		delete[] temp;
		b2=buffer;
	}
	return data_start;
}
static void		factorize_dimensions(int width, int height, std::vector<short> &wFactors, std::vector<short> &hFactors)
{
	factorize(width, wFactors);
	factorize(height, hFactors);
	generate_block_sizes(wFactors);
	generate_block_sizes(hFactors);
	if(*wFactors.rbegin()!=1)
		wFactors.push_back(1);
	if(*hFactors.rbegin()!=1)
		hFactors.push_back(1);
	while(wFactors.size()<hFactors.size())
		wFactors.push_back(1);
	while(hFactors.size()<wFactors.size())
		hFactors.push_back(1);
}
static bool		bcheck(int x, int y, int bw, int bh)
{
	return x<0||x>=bw||y<0||y>=bh;
}
static int*		compress_v5(const short *buffer, int bw, int bh)//Hadamard transform	X will inflate noisy raw images
{
	int imsize=bw*bh;
	auto result=new int[imsize];
	separate_bayer_channels(buffer, result, bw, bh);
	
	printf("Before Hadamard transform:\n");
	print_first_16sq(result, bw, bh);//
	//print_histogram_from_image(result, bw, bh);//

	int w2=bw>>1;
	for(int ky=0;ky<bh;ky+=16)
	{
		for(int ky=0;ky<bh;ky+=16)//for each 16x16 block
		{
			for(int h=1;h<16;h<<=1)
			{
			}
		}
	}
	//print_histogram_from_image(result, bw, bh);//
	//log_pause(LL_CRITICAL);//
	
	//estimate_huffman(result, bw, bh);//
	printf("After Hadamard transform:\n");
	print_first_16sq(result, bw, bh);//

	return result;
}
static int*		compress_v4(const short *buffer, int bw, int bh)//based on RVL
{
	int imsize=bw*bh;
	auto result=new int[imsize];
	separate_bayer_channels(buffer, result, bw, bh);
	
	printf("Histogram before processing:\n");
	print_histogram_from_image(result, bw, bh);//

	int w2=bw>>1;
	for(int ky=0;ky<bh;++ky)
	{
		int prev=result[bw*ky];
		for(int kx=1;kx<w2;++kx)
		{
			auto &val=result[bw*ky+kx];
			int current=val;
			val-=prev;
			int negative=val<0;
			val=((val^-negative)+negative)<<1|negative;
			prev=current;
		}
		prev=result[bw*ky+w2];
		for(int kx=1;kx<w2;++kx)
		{
			auto &val=result[bw*ky+w2+kx];
			int current=val;
			val-=prev;
			int negative=val<0;
			val=((val^-negative)+negative)<<1|negative;
			prev=current;
		}
	}
	printf("Processed histogram:\n");
	print_histogram_from_image(result, bw, bh);//
	log_pause(LL_CRITICAL);//
	
	estimate_huffman(result, bw, bh);//
	print_first_16sq(result, bw, bh);//

	return result;
}
//static void		plot_q11(double T, double nSamples)
//{
//	//y(k) = u(k-3) + 0.75y(k-1) - 0.125y(k-4)
//	double y,//output
//		y1=0, y2=0, y3=0, y4=0;//previous values
//	for(int k=0;k<nSamples;++k)
//	{
//		y=(k>=3)+0.75*y1-0.125*y4;
//	//	DrawLine((k-1)*T, y1, k*T, y);	//broken line
//		DrawLine(k*T, 0, k*T, y);		//bars
//		y4=y3, y3=y2, y2=y1, y1=y;		//update previous values
//	}
//}
static int*		compress_v3(const short *buffer, int bw, int bh)
{
	std::vector<short> wFactors, hFactors;
	factorize_dimensions(bw, bh, wFactors, hFactors);
	
	int imsize=bw*bh;
	auto result=new int[imsize];
	separate_bayer_channels(buffer, result, bw, bh);

	for(int k=0;k<imsize;++k)//
		result[k]>>=4;//

	//printf("Uncompressed histogram:\n");
	//print_histogram_from_image(result, bw, bh);//
	//log_pause(LL_CRITICAL);//

	int w2=bw/wFactors[0], h2=bh/hFactors[0];
	int *xbuf=new int[imsize>>2];
	int *ybuf=new int[imsize>>2];
	memset(xbuf, 0, imsize);
	memset(ybuf, 0, imsize);

	estimate_huffman(result, bw*bh, 1024);//
	print_first_16sq(result, bw, bh);//
	
	//printf("YX\n");
	for(int kp=0, xstep=1, ystep=1;kp<(int)wFactors.size()-1;++kp)//for each iteration
	{
		int nextxstep=xstep*wFactors[kp], nextystep=ystep*hFactors[kp];
		for(int ky=0, ky2=0;ky2<bh;++ky, ky2+=nextystep)
		{
			for(int kx=0, kx2=0;kx2<bw;++kx, kx2+=nextxstep)//for each block
			{
				//if(kp==0&&kx2==10&&ky2==0)
				//	int LOL_4=0;
				//typedef std::pair<int, int> Coords;
				//std::vector<Coords> visited;

				int min=-1, xpos=0, ypos=0;
				for(int ky3=0, ky4=0;ky4<nextystep;++ky3, ky4+=ystep)
				{
					for(int kx3=0, kx4=0;kx4<nextxstep;++kx3, kx4+=xstep)//find min in block
					{
						//visited.push_back(Coords(kx2+kx4, ky2+ky4));
						if(bcheck(kx2+kx4, ky2+ky4, bw, bh))
							int LOL_1=0;
						int val=result[bw*(ky2+ky4)+kx2+kx4];
						if(min==-1||min>val)
							min=val, xpos=kx3, ypos=ky3;
					}
				}
				
				for(int ky4=0, k5=0;ky4<nextystep;ky4+=ystep)
				{
					for(int kx4=0;kx4<nextxstep;kx4+=xstep, ++k5)//subtract min
					{
						//auto &coords=visited[k5];
						//if(coords.first!=kx2+kx4||coords.second!=ky2+ky4)
						//	int LOL_3=0;
						if(bcheck(kx2+kx4, ky2+ky4, bw, bh))
							int LOL_2=0;
						result[bw*(ky2+ky4)+kx2+kx4]-=min;
						//if(result[0]!=54)
						//	int LOL_6=0;
						//if(result[bw*0+0xA]!=64)//
						//	int LOL_5=0;//
					}
				}
				//for(int k5=0;k5<(int)visited.size();++k5)//
				//	printf("%X%X ", visited[k5].second, visited[k5].first);//YX
				//printf("\n");//

				int idx=bw*(ky2+ystep*ypos)+kx2+xstep*xpos;
				//result[idx]=wFactors[kp]*(hFactors[kp]*min+ypos)+xpos;//store min with its position
				//result[idx]=hFactors[kp]*min+ypos;//store min with its y coordinate in block
				result[idx]=min;//test
				std::swap(result[bw*ky2+kx2], result[idx]);//swap min with first element in group
				//if(result[0]!=54)
				//	int LOL_6=0;
				//if(result[bw*0+0xA]!=64)//
				//	int LOL_5=0;//

				//idx=bw*ky2+kx2+xstep;
				//result[idx]=wFactors[kp]*result[idx]+xpos;//store min's x coordinate one step right in block

				if(kp==0)//
				{
				auto &xval=xbuf[w2*ky+kx], &yval=ybuf[w2*ky+kx];
				xval=wFactors[kp]*xval+xpos;
				yval=hFactors[kp]*yval+ypos;
				}
			}
			//printf("\n");//
		}
		//print_first_16sq(result, bw, bh);//
		xstep=nextxstep, ystep=nextystep;
	}

	printf("Processed histogram:\n");
	print_histogram_from_image(result, bw, bh);//
	log_pause(LL_CRITICAL);//
	
	estimate_huffman(result, bw, bh);//
	print_first_16sq(result, bw, bh);//
	
	//printf("Accumulated X-coordinates of mins:\n");
	//for(int ky=0, ystep=32;ky<64;ky+=ystep)
	//	print_subimage(xbuf, bw>>1, bh>>1, 0, ky, 16, ystep, 4);
	//printf("Accumulated Y-coordinates of mins:\n");
	//print_subimage(ybuf, bw>>1, bh>>1, 0, 0, 32, 32, 4);
	//print_first_16sq(xbuf, bw>>1, bh>>1);//
	//print_first_16sq(ybuf, bw>>1, bh>>1);//
	estimate_huffman(xbuf, bw>>1, bh>>1);
	estimate_huffman(ybuf, bw>>1, bh>>1);
	//printf("Accumulated X-coordinates of mins:\n");
	//print_histogram_from_image(xbuf, imsize>>2);
	//printf("Accumulated Y-coordinates of mins:\n");
	//print_histogram_from_image(ybuf, imsize>>2);

	delete[] xbuf, ybuf;

	return result;
}
static int*		compress_v2(const short *buffer, int bw, int bh)
{
	std::vector<short> wFactors, hFactors;
	factorize_dimensions(bw, bh, wFactors, hFactors);
	
	int imsize=bw*bh;
	auto result=new int[imsize];
	separate_bayer_channels(buffer, result, bw, bh);

	estimate_huffman(result, bw*bh, 1024);//
	print_first_16sq(result, bw, bh);//
	
	for(int kp=0, xstep=1, ystep=1;kp<(int)wFactors.size()-1;++kp)//for each iteration
	{
		int nextxstep=xstep*wFactors[kp], nextystep=ystep*hFactors[kp];
		for(int ky=0, ky2=0;ky2<bh;++ky, ky2+=nextystep)
		{
			for(int kx=0, kx2=0;kx2<bw;++kx, kx2+=nextxstep)//for each block
			{
				int min=-1, xpos=0, ypos=0;
				for(int ky3=0, ky4=0;ky4<nextystep;++ky3, ky4+=ystep)
				{
					for(int kx3=0, kx4=0;kx4<nextxstep;++kx3, kx4+=xstep)//find min in block
					{
						if(bcheck(kx2+kx4, ky2+ky4, bw, bh))
							int LOL_1=0;
						int val=result[bw*(ky2+ky4)+kx2+kx4];
						if(min==-1||min>val)
							min=val, xpos=kx3, ypos=ky3;
					}
				}
				
				for(int ky4=0;ky4<nextystep;ky4+=ystep)
				{
					for(int kx4=0;kx4<nextxstep;kx4+=xstep)//subtract min
					{
						if(bcheck(kx2+kx4, ky2+ky4, bw, bh))
							int LOL_2=0;
						result[bw*(ky2+ky4)+kx2+kx4]-=min;
					}
				}

				int idx=bw*ky2+(kx2+ystep*ypos)+xstep*xpos;
				//result[idx]=wFactors[kp]*(hFactors[kp]*min+ypos)+xpos;//store min with its position
				result[idx]=hFactors[kp]*min+ypos;//store min with its y coordinate in block
				//result[idx]=min;//test
				std::swap(result[bw*ky2+kx2], result[idx]);//swap min with first element in group

				idx=bw*ky2+kx2+xstep;
				result[idx]=wFactors[kp]*result[idx]+xpos;//store min's x coordinate one step right in block

			}
		}
		xstep=nextxstep, ystep=nextystep;
	}
	
	estimate_huffman(result, bw, bh);//
	print_first_16sq(result, bw, bh);//

	return result;
}
static int*		compress(const short *buffer, int bw, int bh)
{
	std::vector<short> wFactors, hFactors;
	factorize_dimensions(bw, bh, wFactors, hFactors);
	
	int imsize=bw*bh;
	auto result=new int[imsize];
	separate_bayer_channels(buffer, result, bw, bh);

	estimate_huffman(result, bw*bh, 1024);//
	print_first_16sq(result, bw, bh);//
	
	for(int kp=0, xstep=1, ystep=1;kp<(int)wFactors.size()-1;++kp)//for each iteration
	{
		int nextxstep=xstep*wFactors[kp], nextystep=ystep*hFactors[kp];
		for(int ky=0, ky2=0;ky2<bh;++ky, ky2+=nextystep)
		{
			for(int kx=0, kx2=0;kx2<bw;++kx, kx2+=nextxstep)//for each block
			{
				int min=-1, xpos=0, ypos=0;
				//typedef std::pair<int, int> Coords;
				//std::vector<Coords> visited1;
				for(int ky3=0, ky4=0;ky4<nextystep;++ky3, ky4+=ystep)
				{
					for(int kx3=0, kx4=0;kx4<nextxstep;++kx3, kx4+=xstep)//find min in block
					{
						if(bcheck(kx2+kx4, ky2+ky4, bw, bh))
							int LOL_1=0;
						//visited1.push_back(Coords(kx2+kx4, ky2+ky4));
						int val=result[bw*(ky2+ky4)+kx2+kx4];
						//if(val<0)
						//	int LOL_1=0;
						//printf("%d ", val);//
						if(min==-1||min>val)
							min=val, xpos=kx3, ypos=ky3;
					}
				}
				//printf("\n");
				
				//std::vector<Coords> visited2;
				for(int ky4=0;ky4<nextystep;ky4+=ystep)
				{
					for(int kx4=0;kx4<nextxstep;kx4+=xstep)//subtract min
					{
						if(bcheck(kx2+kx4, ky2+ky4, bw, bh))
							int LOL_2=0;
						//visited2.push_back(Coords(kx2+kx4, ky2+ky4));
						result[bw*(ky2+ky4)+kx2+kx4]-=min;
						//if(result[bw*(ky2+ky4)+kx2+kx4]<0)
						//	int LOL_2=0;
						//printf("%d ", result[bw*(ky2+ky4)+kx2+kx4]);//
					}
				}
				//printf("\n");

				int idx=bw*ky2+(kx2+ystep*ypos)+xstep*xpos;
				auto val=wFactors[kp]*((unsigned long long)hFactors[kp]*min+ypos)+xpos;
				if(val>0xFFFFFFFF)//
					int LOL_1=0;//
				result[idx]=(int)val;//store min with its position in group
				if(result[idx]<0)
					int LOL_2=0;
				std::swap(result[bw*ky2+kx2], result[idx]);//swap min with first element in group
			}
		}

	/*	for(int ky=0, ky2=0;ky2<bh;++ky, ky2+=ystep)
		{
			for(int kx=0, kx2=0;kx2<bw;++kx, kx2+=nextxstep)//for each horizontal group
			{
				int min=-1, pos=0;
				for(int kx3=0, kx4=0;kx4<nextxstep;++kx3, kx4+=xstep)//find min in group
				{
					int val=result[bw*ky2+kx2+kx4];
					if(min==-1||min>val)
						min=val, pos=kx3;
				}
				for(int kx4=0;kx4<nextxstep;kx4+=xstep)//subtract min
					result[bw*ky2+kx2+kx4]-=min;
				int idx=bw*ky2+kx2+xstep*pos;
				result[idx]=min*wFactors[kp]+pos;//store min with its position in group
				std::swap(result[bw*ky2+kx2], result[idx]);//swap min with first element in group
			}
		}
		for(int ky=0, ky2=0;ky2<bh;++ky, ky2+=nextystep)
		{
			for(int kx=0, kx2=0;kx2<bw;++kx, kx2+=xstep)//for each vertical group: do the same vertically
			{
				int min=-1, pos=0;
				for(int ky3=0, ky4=0;ky4<nextystep;++ky3, ky4+=ystep)//find min in group
				{
					int val=result[bw*(ky2+ky4)+kx2];
					if(min==-1||min>val)
						min=val, pos=ky3;
				}
				for(int ky4=0;ky4<nextystep;ky4+=ystep)//subtract min
					result[bw*(ky2+ky4)+kx2]-=min;
				int idx=bw*(ky2+ystep*pos)+kx2;
				result[idx]=min*hFactors[kp]+pos;//store min with its position in group
				std::swap(result[bw*ky2+kx2], result[idx]);
			}
		}//*/
		xstep=nextxstep, ystep=nextystep;
	}
	
	estimate_huffman(result, bw, bh);//
	print_first_16sq(result, bw, bh);//

	return result;
}

struct			HistogramElement
{
	int symbol, count;
};
void			calculate_histogram(const byte *data, int size, HistogramElement *histogram, int nLevels)
{
	memset(histogram, 0, nLevels*sizeof(int));
	for(int k=0;k<size;++k)
	{
		auto &elem=histogram[data[k]];
		elem.symbol=data[k];
		++elem.count;
	}
}
static void		print_histogram(HistogramElement *histogram, int nlevels, int scanned_size)
{
	int histmax=0;
	for(int k=0;k<nlevels;++k)
		if(histmax<histogram[k].count)
			histmax=histogram[k].count;
	const int consolechars=79-15;
	if(!histmax)
		return;
	for(int k=0;k<nlevels;++k)
	{
		auto &elem=histogram[k];
		printf("%4d %6d %2d ", elem.symbol, elem.count, 100*elem.count/scanned_size);
		for(int kr=0, count=elem.count*consolechars/histmax;kr<count;++kr)
			printf("*");
		printf("\n");
	}
	printf("\n");
}
void			image_compression()
{
	printf("Experimental Image Compression\n\n");

	printf("This format is only for raw images with bit depth of 10bits (.r10.png).\n");
	std::string filename=get_filename("Input image: ");
	int iw2=0, ih2=0, nch2=0;
	unsigned char *original_image=stbi_load(filename.c_str(), &iw2, &ih2, &nch2, 0);
	if(!original_image)
	{
		printf("\nCan't open \'%s\'\n", filename.c_str());
		log_pause(LL_CRITICAL);
		log_end();
		return;
	}
	int *im2=(int*)original_image;
	int width=iw2<<1, height=ih2, imsize=width*height;
	short *src=new short[imsize];
	const int bitdepth=10;
	const int mask=(1<<bitdepth)-1;
	for(int k=0;k<imsize;k+=2)
	{
		int elem=im2[k>>1];
		elem=swap_rb(elem);
		int px1=elem&mask, px2=elem>>bitdepth&mask;
		src[k  ]=px1;
		src[k+1]=px2;
	}

	//int nbytes=imsize*5>>2;
	//byte *dst=new byte[nbytes];
	//for(int k=0, k2=0;k<imsize;k+=4, k2+=5)
	//{
	//	dst[k2  ]=src[k  ]>>2;
	//	dst[k2+1]=src[k+1]>>2;
	//	dst[k2+2]=src[k+2]>>2;
	//	dst[k2+3]=src[k+3]>>2;
	//	dst[k2+4]=src[k+3]<<6|src[k+2]<<4|src[k+1]<<2|src[k];
	//}
	//printf("huff(android.RAW10):\n");
	//estimate_huffman(dst, nbytes, 256);


	std::vector<int> data;
	auto t1=time_ms();
	int data_start=compress_huff(src, width, height, 10, 'G'|'R'<<8|'B'<<16|'G'<<24, data);
	auto t2=time_ms();
	printf("Compression took %lfms\n", t2-t1);
	auto dataHeader=(HuffDataHeader*)(data.data()+data_start);
	HistogramElement histogram[256]={};
	calculate_histogram((const byte*)dataHeader->data, (int)(dataHeader->bitSize>>3), histogram, 256);
	
	//auto p=(const unsigned char*)dataHeader->data;
	//memset(histogram, 0, 256*sizeof(int));
	//for(int k=0, datasize=dataHeader->bitSize>>3, vsize=data.size();k<datasize;++k)
	//{
	//	if(data_start+(k>>2)>=vsize)
	//		continue;
	//	++histogram[p[k]];
	//}

	printf("Compressed data histogram:\n");
	print_histogram(histogram, 256, (int)(dataHeader->bitSize>>3));
	std::sort(histogram, histogram+256, [](HistogramElement const &a, HistogramElement const &b){return a.count>b.count;});
	//std::sort(histogram, histogram+256, [](int a, int b){return a>b;});
	printf("Compressed data sorted histogram:\n");
	print_histogram(histogram, 256, (int)(dataHeader->bitSize>>3));
	//estimate_huffman((byte*)data.data(), data.size()*sizeof(int), 256);//2nd huffman
	printf("\tFirst compression: %d\n", data.size()*32);

	//auto result=smad(src, width, height);
	//auto result=smad(src, 64, 64);
	//auto result=compress(src, width, height);
	//auto result=compress(src, 48, 48);
	//auto result=compress_v2(src, width, height);
	//auto result=compress_v2(src, 16, 16);
	//auto result=compress_v3(src, width, height);
	//auto result=compress_v3(src, 16, 16);
	//auto result=compress_v4(src, width, height);
	//delete[] result;

	free(original_image);
	delete[] src;
	//delete[] dst;
	//delete[] ds3;
	//delete[] dst, ds2;

	printf("Done.\n");
	log_pause(LL_CRITICAL);
	log_end();
}

void			stack_sky_images()
{
	log_start(LL_CRITICAL);
	printf(
		"Image stacking\n"
		"Enter:\t\tfor:\n"
		" 1:  Image stacker v1: moving cam\n"
		" 2:  Image stacker v2: static cam\n"
		" 3:  Image stacker v2: (loud)\n"
		" 4:  Image stacker v3\n"
		" 5:  Experimental image format test\n");
	char c;
	while(!scanf("%c", &c));
	switch(c)
	{
	case '1':
		stack_sky_images_v1();
		break;
	case '2':
	case '3':
		stack_sky_images_v2(c==3);
		break;
	case '4':
		stack_sky_images_v3();
		break;
	case '5':
		image_compression();
		break;
	default:
		log_end();
		break;
	}
}