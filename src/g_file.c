#include		<sys/stat.h>
#if !defined __linux__
#if _MSC_VER<1800
#define	S_IFMT		00170000//octal
#define	S_IFREG		 0100000
#endif
#define	S_ISREG(m)	(((m)&S_IFMT)==S_IFREG)
#endif
int				file_is_readablea(const char *filename)//0: not readable, 1: regular file, 2: folder
{
	struct stat info;
	int error=stat(filename, &info);
	if(!error)
		return 1+!S_ISREG(info.st_mode);
	return 0;
}
int				file_is_readablew(const wchar_t *filename)//0: not readable, 1: regular file, 2: folder
{
	struct _stat32 info;
	int error=_wstat32(filename, &info);
	if(!error)
		return 1+!S_ISREG(info.st_mode);
	return 0;
}
/*int				file_existsw(const wchar_t *name)
{
	struct _stat32 info;
	int error=_wstat32(name, &info);
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
}
int				file_existsa(const char *name)
{
	struct _stat32 info;
	int error=_stat32(name, &info);
	return !error;
}//*/