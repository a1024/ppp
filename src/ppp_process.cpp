#include		"ppp.h"
#include		"generic.h"
#include		<string>
//const char	file[]=__FILE__;
void			exec_FFmpeg_command(const wchar_t *executable, std::wstring const &args, std::string &result)//executable is FFmpeg or FFprobe
{
	std::wstring command=L"ffmpeg -hide_banner "+args;
	//std::wstring command=L"\""+FFmpeg_path+L'/'+executable+L"\" -hide_banner "+args;
	//auto &end=*FFmpeg_path.rbegin();
	//if(end!=L'\\'&&end!=L'/')
	//	FFmpeg_path.push_back(L'/');
	//std::wstring command=FFmpeg_path+executable+(FFmpeg_path_has_quotes?L"\" ":L" ")+L"-hide_banner "+args;
	//std::wstring command=FFmpeg_path+executable+(FFmpeg_path_has_quotes?L"\" ":L" ")+L"-hide_banner -loglevel warning "+args;
	
	SECURITY_ATTRIBUTES saAttr={sizeof(SECURITY_ATTRIBUTES), nullptr, 1};
	void *g_hChildStd_IN_Rd=nullptr, *g_hChildStd_IN_Wr=nullptr, *g_hChildStd_OUT_Rd=nullptr, *g_hChildStd_OUT_Wr=nullptr;
	if(!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))//Create a pipe for the child process's STDOUT.
		error_exit(TEXT("StdoutRd CreatePipe"), __LINE__);
	if(!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))//Ensure the read handle to the pipe for STDOUT is not inherited.
		error_exit(TEXT("Stdout SetHandleInformation"), __LINE__);
	if(!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))//Create a pipe for the child process's STDIN.
		error_exit(TEXT("Stdin CreatePipe"), __LINE__);
	if(!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))//Ensure the write handle to the pipe for STDIN is not inherited.
		error_exit(TEXT("Stdin SetHandleInformation"), __LINE__);
	
	//create child process
	PROCESS_INFORMATION piProcInfo={0};//Set up members of the PROCESS_INFORMATION structure.
	STARTUPINFOW siStartInfo=
	{//Set up members of the STARTUPINFO structure. This structure specifies the STDIN and STDOUT handles for redirection.
		sizeof(STARTUPINFO), nullptr, nullptr, nullptr,//resetved, desktop, title
		0, 0, 0, 0, 0, 0, 0,//x, y, xsize, ysize, xcountchars, ycountchars, fill attribute
		STARTF_USESTDHANDLES,//flags
		0, 0, nullptr,//show window, reserved2, reserved2
		g_hChildStd_IN_Rd, g_hChildStd_OUT_Wr, g_hChildStd_OUT_Wr,//stdin, stdout, stderr
	};
	int bSuccess=CreateProcessW(nullptr,//Create the child process.
		&command[0],	//command line
		nullptr,			//process security attributes
		nullptr,			//primary thread security attributes
		TRUE,				//handles are inherited
		CREATE_NO_WINDOW,	//creation flags
		nullptr,			//use parent's environment
		nullptr,			//use parent's current directory
		&siStartInfo,		//STARTUPINFO pointer
		&piProcInfo);		//receives PROCESS_INFORMATION
	if(!bSuccess)//If an error occurs, exit the application.
		error_exit(TEXT("CreateProcess"), __LINE__);
	else
	{
		//Close handles to the child process and its primary thread.
		//Some applications might keep these handles to monitor the status of the child process, for example.
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
		
		//Close handles to the stdin and stdout pipes no longer needed by the child process.
		//If they are not explicitly closed, there is no way to recognize that the child process has ended.
		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(g_hChildStd_IN_Rd);
	}
	result.clear();
	unsigned long read=0;
	do
	{
		memset(g_buf, 0, g_buf_size);
		bSuccess=ReadFile(g_hChildStd_OUT_Rd, g_buf, g_buf_size, &read, nullptr);
		result+=g_buf;
	}while(bSuccess&&read);
}
bool			check_FFmpeg_path(std::wstring &path)//path is stripped of doublequotes and trailing slash
{
	if(!path.size())
		return false;
	std::wstring FFmpeg_command=L"ffmpeg";
	//std::wstring FFmpeg_command=L"\""+path+L"/FFmpeg\"";
	//auto &end=*path.rbegin();
	//if(end!=L'\\'&&end!=L'/')
	//	path.push_back(L'/');
	//std::wstring FFmpeg_command=path+L"FFmpeg";

	SECURITY_ATTRIBUTES saAttr={sizeof(SECURITY_ATTRIBUTES), nullptr, 1};
	void *g_hChildStd_IN_Rd=nullptr, *g_hChildStd_IN_Wr=nullptr, *g_hChildStd_OUT_Rd=nullptr, *g_hChildStd_OUT_Wr=nullptr;
	if(!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))//Create a pipe for the child process's STDOUT.
		error_exit(TEXT("StdoutRd CreatePipe"), __LINE__);
	if(!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))//Ensure the read handle to the pipe for STDOUT is not inherited.
		error_exit(TEXT("Stdout SetHandleInformation"), __LINE__);
	if(!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))//Create a pipe for the child process's STDIN.
		error_exit(TEXT("Stdin CreatePipe"), __LINE__);
	if(!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))//Ensure the write handle to the pipe for STDIN is not inherited.
		error_exit(TEXT("Stdin SetHandleInformation"), __LINE__);
	
	//create child process
	PROCESS_INFORMATION piProcInfo={0};//Set up members of the PROCESS_INFORMATION structure.
	STARTUPINFOW siStartInfo=
	{//Set up members of the STARTUPINFO structure. This structure specifies the STDIN and STDOUT handles for redirection.
		sizeof(STARTUPINFO), nullptr, nullptr, nullptr,//resetved, desktop, title
		0, 0, 0, 0, 0, 0, 0,//x, y, xsize, ysize, xcountchars, ycountchars, fill attribute
		STARTF_USESTDHANDLES,//flags
		0, 0, nullptr,//show window, reserved2, reserved2
		g_hChildStd_IN_Rd, g_hChildStd_OUT_Wr, g_hChildStd_OUT_Wr,//stdin, stdout, stderr
	};
	int bSuccess=CreateProcessW(nullptr,//Create the child process.
		&FFmpeg_command[0],	//command line
		nullptr,			//process security attributes
		nullptr,			//primary thread security attributes
		TRUE,				//handles are inherited
		CREATE_NO_WINDOW,	//creation flags
		nullptr,			//use parent's environment
		nullptr,			//use parent's current directory
		&siStartInfo,		//STARTUPINFO pointer
		&piProcInfo);		//receives PROCESS_INFORMATION
	if(!bSuccess)//If an error occurs, exit the application.
		error_exit(TEXT("CreateProcess"), __LINE__);
	else
	{
		//Close handles to the child process and its primary thread.
		//Some applications might keep these handles to monitor the status of the child process, for example.
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
		
		//Close handles to the stdin and stdout pipes no longer needed by the child process.
		//If they are not explicitly closed, there is no way to recognize that the child process has ended.
		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(g_hChildStd_IN_Rd);
	}
	unsigned long read=0;
	std::string result;
	do
	{
		memset(g_buf, 0, g_buf_size);
		bSuccess=ReadFile(g_hChildStd_OUT_Rd, g_buf, g_buf_size, &read, nullptr);
		result+=g_buf;
	}while(bSuccess&&read);
	if(!result.size())
		return false;
	const char ex_start[]="ffmpeg version";
	int k=0, size=result.size();
	for(int kEnd=minimum(size, strlen(ex_start));k<kEnd;++k)
		if(result[k]!=ex_start[k])
			return false;
	for(;k<size&&result[k]==' ';++k);
	int versionstart=k;
	for(;k<size&&result[k]!=' ';++k);
	FFmpeg_version.assign(result.begin()+versionstart, result.begin()+k);
	return true;
}
bool			check_FFmpeg_path(std::string &path)//path is stripped of doublequotes and trailing slash
{
	if(!path.size())
		return false;
	std::string FFmpeg_command='\"'+path+"/FFmpeg\"";

	SECURITY_ATTRIBUTES saAttr={sizeof(SECURITY_ATTRIBUTES), nullptr, 1};
	void *g_hChildStd_IN_Rd=nullptr, *g_hChildStd_IN_Wr=nullptr, *g_hChildStd_OUT_Rd=nullptr, *g_hChildStd_OUT_Wr=nullptr;
	if(!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))//Create a pipe for the child process's STDOUT.
		error_exit(TEXT("StdoutRd CreatePipe"), __LINE__);
	if(!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))//Ensure the read handle to the pipe for STDOUT is not inherited.
		error_exit(TEXT("Stdout SetHandleInformation"), __LINE__);
	if(!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))//Create a pipe for the child process's STDIN.
		error_exit(TEXT("Stdin CreatePipe"), __LINE__);
	if(!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))//Ensure the write handle to the pipe for STDIN is not inherited.
		error_exit(TEXT("Stdin SetHandleInformation"), __LINE__);
	
	//create child process
	PROCESS_INFORMATION piProcInfo={0};//Set up members of the PROCESS_INFORMATION structure.
	STARTUPINFOA siStartInfo=
	{//Set up members of the STARTUPINFO structure. This structure specifies the STDIN and STDOUT handles for redirection.
		sizeof(STARTUPINFO), nullptr, nullptr, nullptr,//resetved, desktop, title
		0, 0, 0, 0, 0, 0, 0,//x, y, xsize, ysize, xcountchars, ycountchars, fill attribute
		STARTF_USESTDHANDLES,//flags
		0, 0, nullptr,//show window, reserved2, reserved2
		g_hChildStd_IN_Rd, g_hChildStd_OUT_Wr, g_hChildStd_OUT_Wr,//stdin, stdout, stderr
	};
	int bSuccess=CreateProcessA(nullptr,//Create the child process.
		&FFmpeg_command[0],	//command line
		nullptr,			//process security attributes
		nullptr,			//primary thread security attributes
		TRUE,				//handles are inherited
		CREATE_NO_WINDOW,	//creation flags
		nullptr,			//use parent's environment
		nullptr,			//use parent's current directory
		&siStartInfo,		//STARTUPINFO pointer
		&piProcInfo);		//receives PROCESS_INFORMATION
	if(!bSuccess)//If an error occurs, exit the application.
		return false;
	//	error_exit(TEXT("CreateProcess"), __LINE__);
	else
	{
		//Close handles to the child process and its primary thread.
		//Some applications might keep these handles to monitor the status of the child process, for example.
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
		
		//Close handles to the stdin and stdout pipes no longer needed by the child process.
		//If they are not explicitly closed, there is no way to recognize that the child process has ended.
		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(g_hChildStd_IN_Rd);
	}
	unsigned long read=0;
	std::string result;
	do
	{
		memset(g_buf, 0, g_buf_size);
		bSuccess=ReadFile(g_hChildStd_OUT_Rd, g_buf, g_buf_size, &read, nullptr);
		result+=g_buf;
	}while(bSuccess&&read);
	if(!result.size())
		return false;
	const char ex_start[]="ffmpeg version";
	int k=0, size=result.size();
	for(int kEnd=minimum(size, strlen(ex_start));k<kEnd;++k)
		if(result[k]!=ex_start[k])
			return false;
	for(;k<size&&result[k]==' ';++k);
	int versionstart=k;
	for(;k<size&&result[k]!=' ';++k);
	FFmpeg_version.assign(result.begin()+versionstart, result.begin()+k);
	return true;
}