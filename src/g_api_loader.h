#ifndef	G_API_LOADER_H
#define	G_API_LOADER_H
//#include"ppp.h"
//#include<Windows.h>
enum			APIState
{
	API_NOT_LOADED, API_LOAD_ERROR, API_LOADED,
};
static int		API_state=API_NOT_LOADED;
static HMODULE	handle=nullptr;
inline int		check_p(const char *srcfile, int srcline, void (*p)(), const char *funcname)
{
	if(!p)
	{
		//LOG_ERROR("Failed to load %s()", funcname);
		log_error(srcfile, srcline, "Failed to load %s()", funcname);
		return 1;
	}
	return 0;
}
#define	DECL_FN(FUNCNAME)		decltype(&FUNCNAME) p_##FUNCNAME=nullptr
#define LOAD_FN(FUNCNAME)		p_##FUNCNAME=(decltype(&FUNCNAME))GetProcAddress(handle, #FUNCNAME), error|=check_p(file, __LINE__, (void(*)())p_##FUNCNAME, #FUNCNAME)
#endif