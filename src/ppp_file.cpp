#include		"ppp.h"
#include		"generic.h"
#include		"lodepng.h"//for saving PNG

#define			STBI_WINDOWS_UTF8
#include		"stb_image.h"//for opening PNG
#include		"lodepng.h"//for saving PNG

#include		<shellapi.h>
#include		<vector>
#include		<fstream>
#include		<sstream>
#include		<codecvt>
#include		<sys/stat.h>
static const char file[]=__FILE__;

//primitive file operations
void			read_binary(const wchar_t *filename, std::vector<byte> &binary_data)
{
	std::ifstream input(filename, std::ios::binary);
	binary_data.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
	input.close();
}
void			write_file(const wchar_t *filename, std::string const &output)
{
	std::ofstream file(filename, std::ios::out);
	file<<output;
	file.close();
}
void			write_binary(const wchar_t *filename, std::vector<byte> const &binary_data)
{
	std::string str(binary_data.begin(), binary_data.end());
	std::ofstream file(filename, std::ios::binary|std::ios::out);
	if(!file.is_open())
		messagebox(ghWnd, L"Error", L"Failed to save:\n%s", filename);
	file<<str;
	file.close();
}
void			read_utf8_file(const wchar_t *filename, std::wstring &output)
{
	std::wifstream wif(filename);
	if(wif.is_open())
	{
		wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
		std::wstringstream wss;
		wss<<wif.rdbuf();
		auto &str=wss.str();
		if(str.size()&&str[0]==0xFEFF)//may start with byte-order mark (BOM)
			output.assign(str.begin()+1, str.end());
		else
			output=std::move(wss.str());
		wif.close();
	}
}
int				write_utf8_file(const wchar_t *filename, std::wstring &input)
{
	FILE *file;
	_wfopen_s(&file, filename, L"w, ccs=UTF-8");
	if(!file)
		return 0;
	int written=fwrite(input.c_str(), sizeof(wchar_t), input.size(), file);
	fclose(file);
	return written;
}
/*inline bool		file_exists(std::wstring const &name)//moved to g_file.c
{
	struct _stat32 info;
	int error=_wstat32(name.c_str(), &info);
	return !error;
#if 0
	FILE *file;
	_wfopen_s(&file, name.c_str(), L"r");
	if(file)
	{
		fclose(file);
		return true;
	}
	return false;
#endif
}//*/

char			errormsg[G_BUF_SIZE]={0};
const char*		shfileerror2str(int error)
{
	switch(error)
	{//https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shfileoperationw
	case 0x71:return "The source and destination files are the same file.";
	case 0x72:return "Multiple file paths were specified in the source buffer, but only one destination file path.";
	case 0x73:return "Rename operation was specified but the destination path is a different directory. Use the move operation instead.";
	case 0x74:return "The source is a root directory, which cannot be moved or renamed.";
	case 0x75:return "The operation was canceled by the user, or silently canceled if the appropriate flags were supplied to SHFileOperation.";
	case 0x76:return "The destination is a subtree of the source.";
	case 0x78:return "Security settings denied access to the source.";
	case 0x79:return "The source or destination path exceeded or would exceed MAX_PATH.";
	case 0x7A:return "The operation involved multiple destination paths, which can fail in the case of a move operation.";
	case 0x7C:return "The path in the source or destination or both was invalid.";
	case 0x7D:return "The source and destination have the same parent folder.";
	case 0x7E:return "The destination path is an existing file.";
	case 0x80:return "The destination path is an existing folder.";
	case 0x81:return "The name of the file exceeds MAX_PATH.";
	case 0x82:return "The destination is a read-only CD-ROM, possibly unformatted.";
	case 0x83:return "The destination is a read-only DVD, possibly unformatted.";
	case 0x84:return "The destination is a writable CD-ROM, possibly unformatted.";
	case 0x85:return "The file involved in the operation is too large for the destination media or file system.";
	case 0x86:return "The source is a read-only CD-ROM, possibly unformatted.";
	case 0x87:return "The source is a read-only DVD, possibly unformatted.";
	case 0x88:return "The source is a writable CD-ROM, possibly unformatted.";
	case 0xB7:return "MAX_PATH was exceeded during the operation.";
	case 0x402:return "An unknown error occurred. This is typically due to an invalid path in the source or destination. This error does not occur on Windows Vista and later.";
	case 0x10000:return "An unspecified error occurred on the destination.";
	case 0x10074:return "Destination is a root directory and cannot be renamed.";
	}
	char *msgBuf=0;
	int size=FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&msgBuf, 0, 0);
	size=minimum(size+1, g_buf_size);
	memcpy(errormsg, msgBuf, size-1);
	errormsg[size-1]='\0';
	LocalFree(msgBuf);
	return errormsg;
}
void			remove_directory(const wchar_t *dir)//Fully qualified name of the directory being deleted, without trailing backslash
{//https://stackoverflow.com/questions/213392/what-is-the-win32-api-function-to-use-to-delete-a-folder
	std::wstring from=dir;
	from+=L'\0';
	_SHFILEOPSTRUCTW file_op=
	{
		nullptr,
		FO_DELETE,
		from.c_str(),
		nullptr,

		0,
	//	FOF_NOCONFIRMATION|FOF_NOERRORUI|FOF_SILENT,

		false,
		0,
		L""
	};
	int error=SHFileOperationW(&file_op);
	if((error||file_op.fAnyOperationsAborted)&&error!=1223)//1223: the operation was cancelled by the user
	{
		const char *msgbuf=shfileerror2str(error);
		messageboxa(0, "Error", "Failed to delete temporary directory:\n\n%s", msgbuf);
	}
}

//ppp settings file
int				parse_path_field(const wchar_t *text, int size, int &k, int &k2, int &advance, const wchar_t *label_path)//path ends with newline
{
	advance=0;
	if(label_path)
	{
		if(acme_matchlabel_ci(text, size, k, label_path, advance))
			return true;
		if(!advance)
		{
			messagebox(ghWnd, L"Error", L"Expected \'path\', found:\n\n%*s%s", minimum(size-k, 50), text+k, size-k<10?L"":L"...");
			return true;
		}
		if(skip_whitespace(text, k))
			return true;
	}
	k+=text[k]==L':';
	if(skip_whitespace(text, k))
		return true;
	k2=k;
	int eof=skip_till_newline_or_null(text, size, k2, advance);
	return eof;
}
void			read_settings()
{
	std::wstring text;
	read_utf8_file((programpath+L'/'+settingsfilename).c_str(), text);

	const wchar_t *labels[]=
	{
		L"FFmpeg",
		L"Workspace",
		L"Canvas",
		L"Font",
	};
	static const int nlabels=SIZEOF(labels);
	//static int label_sizes[nlabels];

	const wchar_t label_path[]=L"path";
	//static int label_path_size;

	//const wchar_t label_w=L'w', label_x=L'x', label_h=L'h', label_b=L'b', label_i=L'i', label_u=L'u', label_colon=L':', label_LPR=L'(', label_RPR=L')';
	const wchar_t explanation[]=L"(wxh):";
	const int expl_size=SIZEOF(explanation)-1;

	//static bool uninitialized=true;
	//if(uninitialized)
	//{
	//	uninitialized=false;
	//	for(int k=0;k<nlabels;++k)
	//		label_sizes[k]=strlen(labels[k]);
	//	label_path_size=strlen(label_path);
	//}
	const wchar_t *a=text.c_str();
	int size=text.size();
	for(int k=0;;)
	{
		if(skip_whitespace(text, k))
			break;
		int k_match=0, advance=0, eof=false;
		for(;k_match<nlabels&&!(eof=acme_matchlabel_ci(a, size, k, labels[k_match], advance))&&!advance;)
			++k_match;
		if(k_match==nlabels||eof)
			break;
		if(skip_whitespace(text, k))
			break;
		switch(k_match)
		{
		case 0://FFmpeg
			{
				int k2;
				int eof=parse_path_field(a, size, k, k2, advance, label_path);
				if(eof)
					break;
				if(advance)
				{
					assign_path(text, k, k2, FFmpeg_path);
					k=k2;
				}
				//{
				//	k+=text[k]==doublequote;
				//	int k3=k2-(text[k2-1]==doublequote);
				//	FFmpeg_path.assign(text.begin()+k, text.begin()+k3);
				//}
			}
			continue;
		case 1://Workspace
			{
				int k2;
				int eof=parse_path_field(a, size, k, k2, advance, nullptr);
				if(eof)
					break;
				if(advance)
				{
					assign_path(text, k, k2, workspace);
					//workspace.assign(text.begin()+k, text.begin()+k2);
					k=k2;
				}
			}
			continue;
		case 2://Canvas
			{
				bool stop=false;
				for(int k2=0;k2<expl_size;++k2)
				{
					k+=text[k]==explanation[k2];
					if(skip_whitespace(text, k))
					{
						stop=true;
						break;
					}
				}
				if(stop)
					break;

				iw=acme_wtoi(a+k, &advance);
				k+=advance;
				if(skip_whitespace(text, k))
					break;

				k+=a[k]==L'x';
				if(skip_whitespace(text, k))
					break;

				ih=acme_wtoi(a+k, &advance);
				k+=advance;
				if(skip_whitespace(text, k))
					break;
			}
			continue;
		case 3://Font
			k+=text[k]==L':';
			if(skip_whitespace(text, k))
				break;

			font_idx=acme_wtoi(a+k, &advance);
			k+=advance;
			if(skip_whitespace(text, k))
				break;

			font_size=acme_wtoi(a+k, &advance);
			k+=advance;
			if(skip_whitespace(text, k))
				break;

			bold=acme_tolower(text[k])==L'b';
			k+=bold;
			if(skip_whitespace(text, k))
				break;

			italic=acme_tolower(text[k])==L'i';
			k+=italic;
			if(skip_whitespace(text, k))
				break;

			underline=acme_tolower(text[k])==L'u';
			k+=underline;
			if(skip_whitespace(text, k))
				break;
			continue;
		default:
			messagebox(ghWnd, L"Error", L"Unexpected text in settings file:\n\n%*s%s", minimum(text.size()-k, 50), a+k, size-k<10?"":"...");
			break;
		}
		break;
	}
	//int k=0;
	//FFmpeg_path_has_quotes=parse_path_field(text, k, L"FFmpeg", FFmpeg_path);
	//workspace_has_quotes=parse_path_field(text, k, L"Workspace", workspace);

	if(!workspace.size())
		workspace=programpath;
	//{
	//	workspace=programpath;
	//	for(int k2=0, size=workspace.size();k2<size;++k2)
	//	{
	//		if(workspace[k2]==' ')
	//		{
	//			if(workspace[0]!=doublequote)
	//				workspace.insert(workspace.begin(), doublequote);
	//			auto end=*workspace.rbegin();
	//			if(end==doublequote)
	//				workspace.pop_back();
	//			workspace_has_quotes=true;
	//			break;
	//		}
	//	}
	//}
}
void			write_settings()
{
	std::wstringstream LOL_1;
	LOL_1<<L"FFmpeg path: "<<FFmpeg_path<<L'\n';
	LOL_1<<L"Workspace:";
	if(workspace!=programpath)
		LOL_1<<L' '<<workspace;
	LOL_1<<L'\n';
	LOL_1<<L"Canvas(w x h): "<<iw<<L'x'<<ih<<L'\n';
	LOL_1<<L"Font: "<<font_idx<<L' '<<font_size;
	if(bold)
		LOL_1<<L" B";
	if(italic)
		LOL_1<<L"I";
	if(underline)
		LOL_1<<L"U";
	LOL_1<<L'\n';

	auto filename=programpath+L'/'+settingsfilename;
	auto &contents=LOL_1.str();
	int written=write_utf8_file(filename.c_str(), contents);
	if(!written&&contents.size())
	{
		strerror_s(g_buf, g_buf_size, errno);
		messagebox(ghWnd, L"Error", L"Failed to save settings to:\n\n%s\n\nfopen_s returned: %S", filename.c_str(), g_buf);
	}
}

//ppp workspace
void			writeworkfolder2wbuf()//TODO: use string&
{
	time_t open_time=time(0);
	tm open_date;
	localtime_s(&open_date, &open_time);
	swprintf_s(g_wbuf, L"%ls/%04d%02d%02d-%02d%02d%02d", workspace.c_str(), 1900+open_date.tm_year, open_date.tm_mon+1, open_date.tm_mday, open_date.tm_hour, open_date.tm_min, open_date.tm_sec);
	//swprintf_s(g_wbuf, L"%ls%04d%02d%02d-%02d%02d%02d\\", workspace.c_str()+(workspace[0]=='\"'), 1900+open_date.tm_year, open_date.tm_mon+1, open_date.tm_mday, open_date.tm_hour, open_date.tm_min, open_date.tm_sec);
}
void			create_workfolder_from_wbuf()
{
	workfolder.assign(g_wbuf);
	int success=CreateDirectoryW(workfolder.c_str(), nullptr);
	if(!success)
		messagebox(ghWnd, L"Error: CreateDirectory", L"GetLastError() returned %d", GetLastError());
}
void			delete_workfolder()
{
	if(workfolder.size())
	{
	//	MessageBoxW(0, L"About to delete work directory.", L"Information", MB_OK);//does not show
		//auto &last=*workfolder.rbegin();
		//if(last==L'\\'||last==L'/')
		//	workfolder.pop_back();
		remove_directory(workfolder.c_str());
		//int success=RemoveDirectoryW(workfolder.c_str());
		//if(!success)
		//{
		//	int errorcode=GetLastError();//ERROR_ALREADY_EXISTS;
		//	messagebox(nullptr, L"Error", L"GetLastError() returned %d", errorcode);
		//}
	}
}

//frames
void			load_frame(int frame_number)
{
	std::wstring framepath=workfolder+L'/'+framenames[frame_number];
	//std::wstring framepath(workfolder.begin()+(workfolder[0]=='\"'), workfolder.end());
	//framepath+=framenames[frame_number];
	//std::wstring framepath=workfolder+framenames[frame_number];
	//if(workspace_has_quotes)
	//	framepath+=L'\"';
	
	stbi_convert_wchar_to_utf8(g_buf, g_buf_size, framepath.c_str());
	unsigned char *original_image=stbi_load(g_buf, &iw, &ih, &nchannels, 0);
	image_size=iw*ih;
	image=hist_start(iw, ih);
//	image=(int*)realloc(image, image_size<<2);
	switch(nchannels)//set the 4 nchannels & swap red/blue channels
	{
	case 1:
		for(int k=0;k<image_size;++k)
		{
			auto p=(unsigned char*)(image+k);
			p[0]=original_image[k];
			p[1]=original_image[k];
			p[2]=original_image[k];
			p[3]=0xFF;
		}
		break;
	case 2:
		for(int k=0, k2=0;k<image_size;++k, k2+=2)
		{
			auto p=(unsigned char*)(image+k);
			p[0]=original_image[k2];
			p[1]=original_image[k2];
			p[2]=original_image[k2];
			p[3]=original_image[k2+1];
			//p[0]=0;
			//p[1]=original_image[k2+1];
			//p[2]=original_image[k2];
			//p[3]=0;
		}
		nchannels=4;//
		break;
	case 3:
		for(int k=0, k2=0;k<image_size;++k, k2+=3)
		{
			auto p=(unsigned char*)(image+k);
			p[0]=original_image[k2+2];
			p[1]=original_image[k2+1];
			p[2]=original_image[k2];
			p[3]=0xFF;
		}
		break;
	case 4:
		swap_rb(image, (int*)original_image, image_size);
		//for(int k=0;k<image_size;++k)
		//{
		//	auto p=(unsigned char*)(image+k), p2=(unsigned char*)((int*)original_image+k);
		//	p[0]=p2[2];
		//	p[1]=p2[1];
		//	p[2]=p2[0];
		//	p[3]=p2[3];
		//}
	//	memcpy(image, original_image, image_size<<2);
		break;
	}
	stbi_image_free(original_image);
	original_image=nullptr;

	//Gdiplus::Bitmap bmp(framepath.c_str());
	//iw=bmp.GetWidth(), ih=bmp.GetHeight(), image_size=iw*ih;
	//image=(int*)realloc(image, image_size<<2);
	//Gdiplus::BitmapData data;
	//Gdiplus::Rect rect(0, 0, iw, ih);
	//bmp.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
	//memcpy(image, data.Scan0, image_size<<2);
	//bmp.UnlockBits(&data);
}
void			save_frame(int frame_number)
{
	std::wstring framepath=workfolder+L'/'+framenames[frame_number];
	//std::wstring framepath(workfolder.begin()+(workfolder[0]=='\"'), workfolder.end());
	//framepath+=framenames[frame_number];
	//std::wstring framepath=workfolder+framenames[frame_number];
	//if(workspace_has_quotes)
	//	framepath+=L'\"';

	int *image2=(int*)malloc(image_size<<2);
	swap_rb(image2, image, image_size);
	//for(int k=0;k<image_size;++k)
	//	image2[k]=swap_rb(image[k]);
	std::vector<byte> png;
	unsigned error=lodepng::encode(png, (byte*)image2, iw, ih);
	if(error)
		messagebox(ghWnd, L"Error", L"Failed to encode PNG:\n\n%s", framepath.c_str());
	else
		write_binary(framepath.c_str(), png);//possibly overwrite frame
	free(image2);
}
void			assign_thumbnail(int *thumbnail, int *image)
{
	for(int ky=0;ky<th_h;++ky)//no averaging
	{
		int iy=ky*ih/th_h;
		for(int kx=0;kx<th_w;++kx)
		{
			int ix=kx*iw/th_w;
			thumbnail[th_w*ky+kx]=swap_rb(image[iw*iy+ix]);
		}
	}
}
void			load_thumbnails()
{
	std::vector<byte> png, bitmap;
	for(int kf=0;kf<nframes;++kf)
	{
		//std::wstring framepath=doublequote+workfolder+L'/'+framenames[kf]+doublequote;
		std::wstring framepath=workfolder+L'/'+framenames[kf];
		//std::wstring framepath=workfolder+framenames[kf];
		//if(workspace_has_quotes)
		//	framepath+=L'\"';

		unsigned iw, ih;
		read_binary(framepath.c_str(), png);
		bitmap.clear();
		lodepng::decode(bitmap, iw, ih, &png[0], png.size());//5th overload
		int *image2=(int*)&bitmap[0];

		auto &thumbnail=thumbnails[kf];
		if(!thumbnail)
			thumbnail=(int*)malloc(th_w*th_h<<2);
		assign_thumbnail(thumbnail, image2);
	}
}

//media
union			Field32
{
	unsigned i;
	unsigned short s[2];
	byte c[4];
};
int				load32(byte const *data, int byteoffset)
{
	Field32 x;
	//byte *p=(byte*)&i;
	x.c[0]=data[byteoffset];
	x.c[1]=data[byteoffset+1];
	x.c[2]=data[byteoffset+2];
	x.c[3]=data[byteoffset+3];
	return x.i;
}
struct			IcoCurHeader
{
	unsigned short zero, type, count;
};
struct			IcoCurDirectory
{
	byte width, height, ncolors, zero;
	unsigned short xx, yy;
	unsigned size, offset;
};
/*struct		IcoCurHeader
{
//	unsigned short zero;//not included because of padding
	unsigned short type, nimages;
	byte width, height, ncolors, zero2;
	unsigned short xx, yy;
	unsigned size, offset;
};//*/
bool			open_media(std::wstring filepath_u16)//pass by copy
{
	if(filepath_u16.find(L' ')!=std::string::npos)
	{
		if(filepath_u16[0]!=doublequote)
			filepath_u16.insert(filepath_u16.begin(), doublequote);
		if(*filepath_u16.rbegin()!=doublequote)
			filepath_u16.push_back(doublequote);
	}
	std::string result;
	
	log_start(LL_OPERATIONS);
	writeworkfolder2wbuf();
	if(workfolder.size())
		logi(LL_OPERATIONS,
			"About to delete the old folder:\n"
			"\t%ls\n"
			"and extract frame(s) to the new folder:\n"
			"\t%ls\n", workfolder.c_str(), g_wbuf);
	else
		logi(LL_OPERATIONS,
			"About to extract frame(s) to the new folder:\n"
			"\t%ls\n", g_wbuf);
	log_pause(LL_OPERATIONS);

	delete_workfolder();			//delete old work folder if exists
	create_workfolder_from_wbuf();
	logi(LL_PROGRESS, "Extracting frame(s)...");

	std::wstring filename;
	assign_path(filepath_u16, 0, filepath_u16.size(), filename);
	int size=filename.size();
	bool frames_extracted=false;
	if(size>=8)
	{
		if(!acme_strcmp_ci(filename.c_str()+size-8, L".r10.png"))
		{
			stbi_convert_wchar_to_utf8(g_buf, g_buf_size, filename.c_str());
			int iw2, ih2, nch2;
			unsigned char *original_image=stbi_load(g_buf, &iw2, &ih2, &nch2, 0);
			if(nch2!=4)
				return false;

			imagetype=IM_FLOAT32_MOSAIC;
			modified=false;
			int *im=(int*)original_image;
			//r10_subimage_to_clipboard(im, iw2, ih2, 16, 16);
			iw=iw2<<1, ih=ih2, image_size=iw*ih, nchannels=nch2;//double width because each 32bit pixel packs two mosaic elements
			image=hist_start(iw, ih);
			//image_size=iw*ih, image=(int*)realloc(image, image_size<<2);
			int bitdepth=10;
			int mask=(1<<bitdepth)-1;
			float normal=1.f/mask;
			float *fimage=(float*)image;
			for(int k=0;k<image_size;k+=2)
			{
				int couple=im[k>>1];
				couple=swap_rb(couple);
				int px1=couple&mask, px2=couple>>bitdepth&mask;
				fimage[k  ]=px1*normal;
				fimage[k+1]=px2*normal;
			}
			//for(int k=0;k<image_size;k+=2)
			//{
			//	int couple=im[k>>1];
			//	couple=swap_rb(couple);
			//	image[k  ]=0xFF000000|couple			&mask;
			//	image[k+1]=0xFF000000|couple>>bitdepth	&mask;
			//}
			log_end();
			return true;

			//std::vector<byte> png;
			//unsigned error=lodepng::encode(png, (byte*)savedimage, width, height);
			//if(error)
			//{
			//	messagebox(ghWnd, L"Error", L"Failed to encode frame %d to PNG", ki);
			//	continue;
			//}
			//swprintf_s(g_wbuf, g_buf_size, L"%08d.PNG", ki+1);
			//framenames[ki]=g_wbuf;
			//std::wstring framepath=workfolder+L'/'+framenames[ki];
			//write_binary(framepath.c_str(), png);//possibly overwrite frame
		}
	}
	if(size>=4)
	{
		if(!acme_strcmp_ci(filename.c_str()+size-4, L".ani"))
		{
			std::vector<byte> data;
			read_binary(filename.c_str(), data);
			byte *bits=data.data();
			int bytesize=data.size();
			int k=0;
			const int riff='R'|'I'<<8|'F'<<16|'F'<<24;//0x46464952
			if(bytesize<8)
				goto standard_open;
			int label=load32(bits, k);
			k+=4;
			if(label!=riff)
				goto standard_open;
			int filesize=load32(bits, k), listsize=0, titlesize=0, authorsize=0, framesize=0;
			k+=4;
			for(;k<bytesize;)//https://www.gdgsoft.com/anituner/help/aniformat.htm
			{
				label=load32(bits, k);
				k+=4;
				switch(label)
				{
				case 'A'|'C'<<8|'O'<<16|'N'<<24://nothing
					continue;
				case 'L'|'I'<<8|'S'<<16|'T'<<24://length of icon list
					listsize=load32(bits, k);
					k+=4;
					continue;
				case 'I'|'N'<<8|'A'<<16|'M'<<24://title
					titlesize=load32(bits, k);
					k+=4;
					k+=titlesize;
					continue;
				case 'I'|'A'<<8|'R'<<16|'T'<<24://author
					authorsize=load32(bits, k);
					k+=4;
					k+=authorsize;
					continue;
				case 'f'|'r'<<8|'a'<<16|'m'<<24://start of frames
					nframes=0;
					framenames.clear();
					for(int kt=0, size=thumbnails.size();kt<size;++kt)
						if(thumbnails[kt])
							free(thumbnails[kt]), thumbnails[kt]=nullptr;
					thumbnails.clear();
					continue;
				case 'i'|'c'<<8|'o'<<16|'n'<<24://frame
					{
						framesize=load32(bits, k)-4;
						k+=4;
					}
					continue;
				case 'a'|'n'<<8|'i'<<16|'h'<<24://header
					continue;
				default:
					messagebox(ghWnd, L"Error", L"Failed to parse:\n\n%s\n\nat %d bytes: 0x%08X", filename.c_str(), k, label);
					return false;
				}
			}
			return true;//
		}
		else if(!acme_strcmp_ci(filename.c_str()+size-4, L".ico")||!acme_strcmp_ci(filename.c_str()+size-4, L".cur"))
		{
			std::vector<byte> data;
			read_binary(filename.c_str(), data);
			byte *bits=data.data();
			int bytesize=data.size();
			if(bytesize<22)
				goto standard_open;

			IcoCurHeader *header=(IcoCurHeader*)bits;
			if(header->zero)
				goto standard_open;
			int k=sizeof(IcoCurHeader);
			IcoCurDirectory directory;
			BITMAPINFOHEADER bmi;
			framenames.resize(header->count);
			int *tempimage=nullptr, *savedimage=nullptr;
			for(int ki=0;ki<header->count;++ki)
			{
				memcpy(&directory, bits+k, sizeof(IcoCurDirectory));
				k+=sizeof(IcoCurDirectory);
				int bmisize=load32(bits, directory.offset);
				if(bmisize!=sizeof(BITMAPINFOHEADER))
					goto standard_open;
				byte *pbmi=bits+directory.offset;
				memcpy(&bmi, pbmi, sizeof(BITMAPINFOHEADER));
				if(bmi.biCompression!=BI_RGB)
					goto standard_open;
				int width=bmi.biWidth, height=abs(bmi.biHeight), upsidedown=bmi.biHeight>0;
				int palettesize=bmi.biClrUsed|(1<<bmi.biBitCount&-!bmi.biClrUsed);//count of RGBQUADs
				int *palette=(int*)(pbmi+sizeof(BITMAPINFOHEADER));
				byte *srcimage=(byte*)(palette+palettesize);
				if(bmi.biBitCount==32)
					savedimage=(int*)srcimage;
				else
				{
					int image_size=width*height;
					tempimage=(int*)realloc(tempimage, image_size<<2);
					savedimage=tempimage;
					switch(bmi.biBitCount)
					{
					case 24:
						for(int kp=0, kp2=0;kp<image_size;++kp, kp2+=3)
						{
							auto p=(byte*)(tempimage+kp);
							p[0]=srcimage[kp2];
							p[1]=srcimage[kp2+1];
							p[2]=srcimage[kp2+2];
							p[3]=0xFF;
						}
						break;
					case 8:
						for(int kp=0, kp2=0;kp<image_size;++kp, kp2+=3)
							tempimage[kp]=0xFF000000|palette[srcimage[kp2]];
							//tempimage[kp]=palette[srcimage[kp2]];
						break;
					case 4:
						for(int kp=0, kp2=0;kp<image_size;++kp2)
						{
							tempimage[kp]=0xFF000000|palette[srcimage[kp2]&0x0F];
							++kp;
							if(kp>=image_size)
								break;
							tempimage[kp]=0xFF000000|palette[srcimage[kp2]>>4&0x0F];
							++kp;
						}
						break;
					case 1:
						for(int ky=0, w8=width>>3;ky<height;++ky)
						{
							int *dstrow=tempimage+width*ky;
							byte *srcrow;
							if(upsidedown)
								srcrow=srcimage+w8*(height-1-ky);
							else
								srcrow=srcimage+w8*ky;
							for(int kx=0, kx2=0;kx2<w8;kx+=8, ++kx2)
							{
								byte octet=srcrow[kx2];
								if(octet)
									int LOL_1=0;
								for(int kb=0;kb<8;++kb)
								{
									int color=palette[octet>>(7-kb)&1];
									if(color)
										int LOL_2=0;
									dstrow[kx+kb]=0xFF000000|color;
								}
							}
						}
						break;
					default:
						messageboxa(ghWnd, "Error", "Frame %d has unsupported bitcount = %d", ki, bmi.biBitCount);
						return false;
					}
					//iw=width, ih=height, image_size=iw*ih, image=(int*)realloc(image, image_size<<2);//
					//memcpy(image, tempimage, image_size<<2);//
				}
				std::vector<byte> png;
				unsigned error=lodepng::encode(png, (byte*)savedimage, width, height);
				if(error)
				{
					messagebox(ghWnd, L"Error", L"Failed to encode frame %d to PNG", ki);
					continue;
				}
				swprintf_s(g_wbuf, g_buf_size, L"%08d.PNG", ki+1);
				framenames[ki]=g_wbuf;
				std::wstring framepath=workfolder+L'/'+framenames[ki];
				write_binary(framepath.c_str(), png);//possibly overwrite frame
			}
			if(tempimage)
				free(tempimage);
			frames_extracted=true;

		/*	unsigned short zero;
			memcpy(&zero, bits, 2);
			if(zero)
				return false;
			IcoCurHeader header;
			memcpy(&header, bits+2, sizeof(IcoCurHeader));
			int width=header.width|(256&-!header.width),
				height=header.height|(256&-!header.height),
				ncolors=header.ncolors|(256&-!header.ncolors);//*/
		/*	int k=0;
			Field32 field;
			//header
			field.i=load32(bits, k);//0,0,T,T
			k+=2;//not 4
			if(field.s[0])//1st short==0
				return false;
			field.i=load32(bits, k);//T,T,N,N
			k+=4;
			short type=field.s[0],//1: .ico, 2: .cur
				nimages=field.s[1];

			//directory
			field.i=load32(bits, k);//W,H,C,0
			k+=4;
			short width=field.c[2]|(255&-!field.c[2]), height=field.c[3]|(255&-!field.c[3]);
			short colorcount=field.c[2];//field.c[3]==0//*/
			//return true;
		}
	}
standard_open:
	//std::wstring wfoldercmd;
	//if(workfolder.size()&&workfolder[0]!=doublequote)
	//	wfoldercmd=doublequote+workfolder;
	//else
	//	wfoldercmd=workfolder;
	if(!frames_extracted)
	{
		std::wstring args=L"-i "+filepath_u16+L" "+doublequote+workfolder+L"/%08d."+imageformats[tempformat]+doublequote;			//extract video frames
		exec_FFmpeg_command(L"FFmpeg", args, result);
	}
	//exec_FFmpeg_command(L"FFmpeg", L"-i "+filepath_u16+L" "+wfoldercmd+L"%08d."+imageformats[tempformat]+doublequote, result);
	//exec_FFmpeg_command(L"ffmpeg", L"-i "+filepath_u16+L" "+wfoldercmd+L"%08d."+imageformats[tempformat]+(workspace_has_quotes?L"\"":L""), result);
	logi(LL_PROGRESS, "\tDone.\n");

	//list frame names	//https://stackoverflow.com/questions/2802188/file-count-in-a-directory-using-c
	framenames.clear();
	_WIN32_FIND_DATAW data;
	workfolder.append(L"/*");
	//workfolder.push_back(L'*');
	//if(workspace_has_quotes)
	//	workfolder.push_back(L'\"');
	void *hSearch=FindFirstFileW(workfolder.c_str(), &data);
	workfolder.erase(workfolder.size()-2);
	//workfolder.pop_back();
	//if(workspace_has_quotes)
	//	workfolder.pop_back();
	if(hSearch==INVALID_HANDLE_VALUE)
		formatted_error(L"FindFirstFileW", __LINE__);
	//	messagebox(ghWnd, L"Error", L"FindFirstFileW Returned invalid handle. GetLastError() returned %d. Line %d", GetLastError(), __LINE__);
	//	printf("Failed to find path: %ls", workfolder.c_str());
	else//Managed to locate and create a handle to that folder.
	{
		do
		{
			std::wstring name=data.cFileName;
			int k=maximum(0, name.size()-4);
			if(!(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&compare_string_caseinsensitive(name, k, L".PNG", 4))
				framenames.push_back(data.cFileName);
		}
		while(FindNextFileW(hSearch, &data));
		FindClose(hSearch);
	}
	nframes=framenames.size();
	if(!nframes)
	{
		messageboxa(ghWnd, "Error", "FFmpeg returned:\n\n%s", result.c_str());
		log_end();
		return false;
	}
	for(int kt=0, size=thumbnails.size();kt<size;++kt)
		if(thumbnails[kt])
			free(thumbnails[kt]), thumbnails[kt]=nullptr;
	thumbnails.resize(nframes);
	animated=nframes>1;

	if(animated)//get framerate
	{
		logi(LL_PROGRESS, "Querying framerate...");
		exec_FFmpeg_command(L"ffprobe", L"-v 0 -of csv=p=0 -select_streams v:0 -show_entries stream=r_frame_rate "+filepath_u16, result);//query framerate num & den https://askubuntu.com/questions/110264/how-to-find-frames-per-second-of-any-video-file
		int k=0, size=result.size();
		framerate_num=read_unsigned_int(result, k);
		if(k<size&&result[k]=='/')
		{
			++k;
			framerate_den=read_unsigned_int(result, k);
		}
		else
			framerate_den=1;
		logi(LL_PROGRESS, "\tDone: %d/%d=%lf\n", framerate_num, framerate_den, double(framerate_num)/framerate_den);
	}
	
	logi(LL_PROGRESS, "Opening first frame...");
	current_frame=0;//load first frame
	load_frame(current_frame);
	logi(LL_PROGRESS, "\tDone.\n");

	const int thumbnail_size=90;//calculate thumbnails size
	int maxd=maximum(iw, ih);
	if(maxd>thumbnail_size)
		th_w=int(iw*(double)thumbnail_size/maxd), th_h=int(ih*(double)thumbnail_size/maxd);
	else
		th_w=iw, th_h=ih;
	thbox_h=(10+16+th_h)*nframes+10;
	logi(LL_PROGRESS, "Loading thumbnails...");
	if(animated)
		load_thumbnails();
	logi(LL_PROGRESS, "\tDone.\n");

	modified=false;
	log_pause(LL_PROGRESS);
	log_end();
	return true;
}
void			open_media_pt2()
{
	if(open_media(filepath))
	{
		SetWindowTextW(ghWnd, (filepath+L" - Paint++").c_str());
		render(REDRAW_IMAGEWINDOW, 0, w, 0, h);
	}
}
void			open_media()
{
	wchar_t szFile[MAX_PATH]={'\0'};
	tagOFNW ofn={sizeof(ofn), ghWnd, 0, L"All files(*.*)\0*.*\0", 0, 0, 1, szFile, sizeof(szFile), 0, 0, 0, 0, OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST, 0, 0, 0, 0, 0, 0};
	if(GetOpenFileNameW(&ofn))
	{
		filepath=ofn.lpstrFile;
		//std::wstring filepath_u16=ofn.lpstrFile;
		//int length=stbi_convert_wchar_to_utf8(g_buf, g_buf_size, ofn.lpstrFile);
		//filepath=g_buf;//utf8
		int psize=filepath.size();
		get_name_from_path(filepath, filename);
		
		if(filepath.size())
		{
			if(modified)
			{
				int result=ask_to_save();
				switch(result)
				{
				case IDYES:
					save_media_as();
				case IDNO:
					open_media_pt2();
					break;
				case IDCANCEL:
					break;
				}
			}
			else
				open_media_pt2();
		}
	}
}
void			save_media(std::wstring const &outputpath_u16)
{
	std::wstring outputpath_cmd;
	outputpath_cmd.reserve(outputpath_u16.size()+2);
	if(outputpath_u16[0]!=doublequote)
		outputpath_cmd+=doublequote;
	outputpath_cmd+=outputpath_u16;
	if(*outputpath_u16.rbegin()!=doublequote)
		outputpath_cmd+=doublequote;

	//std::wstring outputpath_cmd=outputpath_u16;
	//bool has_space=false;
	//int size=outputpath_cmd.size();
	//for(int k=0;k<size;++k)
	//{
	//	if(outputpath_cmd[k]==L' ')
	//	{
	//		has_space=true;
	//		break;
	//	}
	//}
	//if(has_space)
	//{
	//	if(outputpath_cmd[0]!=L'\"')
	//		outputpath_cmd.insert(outputpath_cmd.begin(), L'\"');
	//	if(outputpath_cmd[size-1]!=L'\"')
	//		outputpath_cmd.push_back(L'\"');
	//}
	if(file_is_readablew(outputpath_u16.c_str()))
		_wremove(outputpath_u16.c_str());

//	std::wstring framename;
	if((unsigned)current_frame>=framenames.size()||!workfolder.size())//workfolder wasn't created yet, save frame as 00000001.PNG
	{
		writeworkfolder2wbuf();
		create_workfolder_from_wbuf();
		framenames.push_back(L"00000001.PNG"), nframes=framenames.size(), current_frame=0;
	}

	//overwrite frame if modified, use FFmpeg
	std::wstring framepath=doublequote+workfolder+L'/'+framenames[current_frame]+doublequote;
	//std::wstring framepath=workspace_has_quotes&&workfolder[0]!=doublequote?L"\"":L"";
	//framepath+=workfolder+framenames[current_frame]+(workspace_has_quotes?L"\"":L"");
	if(modified)
		save_frame(current_frame);
	std::string result;
	if(animated)
	{
		if(!framerate_num)
			framerate_num=25;
		if(!framerate_den)
			framerate_den=1;

		//swprintf_s(g_wbuf, g_buf_size, L"%%d");//
		swprintf_s(g_wbuf, g_buf_size, L" -framerate %lf -i %ls%%08d.PNG -plays 0 %ls", (double)framerate_num/framerate_den, workfolder.c_str(), outputpath_cmd.c_str());
	//	swprintf_s(g_wbuf, g_buf_size, L" -framerate %lf -i %ls%%08d.PNG %ls", (double)framerate_num/framerate_den, workfolder.c_str(), outputpath_cmd.c_str());//doesn't loop
	//	swprintf_s(g_wbuf, g_buf_size, L" -framerate %lf -i %ls%%08d.PNG \"D:\\Share Box\\!\\LOL.GIF\"", (double)framerate_num/framerate_den, workfolder.c_str());
		exec_FFmpeg_command(L"FFmpeg", g_wbuf, result);

		//exec_FFmpeg_command(L"FFmpeg",
		//	L" -i "+workfolder+L"%08d.PNG"+(workspace_has_quotes?L"\"":L"")
		//	+L" -vf palettegen "+workfolder+L"palette.png"+(workspace_has_quotes?L"\"":L""), result);
		//swprintf_s(g_wbuf, g_buf_size, L" -framerate %lf -i %ls\%08d.PNG -i %lspalette.PNG %ls", (double)framerate_num/framerate_den, workfolder.c_str(), workfolder.c_str(), outputpath_cmd.c_str());
		//exec_FFmpeg_command(L"FFmpeg", g_wbuf, result);

	//	exec_FFmpeg_command(L"FFmpeg", L" -i "+workfolder+L"%08d.PNG"+(workspace_has_quotes?L"\" ":L" ")+outputpath_cmd, result);
	}
	else
		exec_FFmpeg_command(L"FFmpeg", L" -i "+framepath+L' '+outputpath_cmd, result);
	const char failmsg[]="Conversion failed!\r\n";//TODO: put this as global string visible on top (may change with FFmpeg version)
	const int fm_size=sizeof(failmsg);
	bool conversion_failed=true;
	for(int k=fm_size-2, k2=result.size()-1;k>=0&&k2>=0;--k, --k2)//compare strings backwards
	{
		if(result[k2]!=failmsg[k])
		{
			conversion_failed=false;
			break;
		}
	}
	if(conversion_failed)
	{
		log_start(LL_OPERATIONS);
		logi(LL_OPERATIONS, result.c_str());
		logi(LL_OPERATIONS, "\n");
		log_pause(LL_OPERATIONS);
		log_end();
	}
	//int length=stbi_convert_wchar_to_utf8(g_buf, g_buf_size, ofn.lpstrFile);//not used
	//if(length)
	//{
	//	if(!animated&&ofn.nFilterIndex>0&&ofn.nFilterIndex<=5)//PNG, GIF, JPG, BMP, TIF: just Gdiplus
	//	{
	//		std::vector<byte> png;
	//		unsigned error=lodepng::encode(png, (byte*)image, iw, ih);
	//		//static const wchar_t *encodernames[]=
	//		//{
	//		//	L"image/png",
	//		//	L"image/gif",
	//		//	L"image/jpeg",
	//		//	L"image/bmp",
	//		//	L"image/tiff",//image/bmp, image/jpeg, image/gif, image/tiff, image/png
	//		//};
	//		//CLSID encoderClsid;//https://docs.microsoft.com/en-us/windows/win32/gdiplus/-gdiplus-converting-a-bmp-image-to-a-png-image-use
	//		//int success=GetEncoderClsid(encodernames[ofn.nFilterIndex], &encoderClsid);
	//		//if(success)
	//		//{
	//		//	Gdiplus::Bitmap bmp(iw, ih, iw<<2, PixelFormat32bppARGB, (unsigned char*)image);
	//		//	Gdiplus::Status status=bmp.Save(g_wbuf, &encoderClsid, nullptr);
	//		//	if(status)
	//		//	{
	//		//		swprintf_s(g_wbuf, g_buf_size, L"Bitmap::Save() -> %d", status);
	//		//		error_exit(g_wbuf, __LINE__);
	//		//	}
	//		//}
	//	}
	//	else//animated or other format: save frame if modified, then use FFmpeg
	//	{
	//	}
	//}
	modified=false;
	SetWindowTextW(ghWnd, (outputpath_u16+L" - Paint++").c_str());
}
bool			save_media_as()
{
	wchar_t filetitle[]=L"Untitled.PNG";
	memcpy(g_wbuf, filetitle, sizeof filetitle);
	tagOFNW ofn=
	{
		sizeof(tagOFNW), ghWnd, 0,
		L"PNG (*.PNG)\0*.PNG\0"//lpstrFilter
		L"Animated PNG (*.APNG)\0*.APNG\0"
		L"GIF (*.GIF)\0*.GIF\0"
		L"JPEG (*.JPG;*.JPEG;*.JPE;*.JFIF)\0*.JPG\0"
		L"24-bit Bitmap (*.bmp;*.dib)\0*.bmp\0"
		L"TIFF (*.TIF;*.TIFF)\0*.TIF\0",
		0, 0,//custom filter
		1,//filter index: 0 is custom filter, 1 is first, ...
					
		g_wbuf, g_buf_size,//file (output)
					
		0, 0,//file title
		0, 0, OFN_ENABLESIZING|OFN_EXPLORER|OFN_NOTESTFILECREATE|OFN_PATHMUSTEXIST|OFN_EXTENSIONDIFFERENT|OFN_OVERWRITEPROMPT,//flags

		0,//nFileOffset
		8,//nFileExtension
		L"PNG",//default extension
					
		0, 0, 0
	};
	if(GetSaveFileNameW(&ofn))
	{
		filepath=ofn.lpstrFile;
		get_name_from_path(filepath, filename);
	//	std::wstring outputpath_u16=ofn.lpstrFile, outputpath_cmd=outputpath_u16;
		save_media(filepath);
		return true;
	}
	return false;
}
//struct			MyDropTarget:public IDropTarget
//{
//	long __stdcall QueryInterface(/*[in]*/ IID const &riid, /*[iid_is][out]*/ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject){return 0;}
//	unsigned long __stdcall AddRef(){return 0;}
//	unsigned long __stdcall Release(){return 0;}
//	long __stdcall DragEnter(/*[unique][in]*/ __RPC__in_opt IDataObject *pDataObj, /*[in]*/ DWORD grfKeyState, /*[in]*/ POINTL pt, /*[out][in]*/ __RPC__inout DWORD *pdwEffect){return 0;}
//        
//	long __stdcall DragOver(/*[in]*/ DWORD grfKeyState, /*[in]*/ POINTL pt, /*[out][in]*/ __RPC__inout DWORD *pdwEffect){return 0;}
//	long __stdcall DragLeave(void){return 0;}
//	long __stdcall Drop(/*[unique][in]*/ __RPC__in_opt IDataObject *pDataObj, /*[in]*/ DWORD grfKeyState, /*[in]*/ POINTL pt, /*[out][in]*/ __RPC__inout DWORD *pdwEffect){return 0;}
//} mdt;