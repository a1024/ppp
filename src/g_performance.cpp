//profiler
#include		"generic.h"
#include		<string>
#include		<vector>
#include		<Windows.h>
#ifdef PROFILER_CLIPBOARD
#include		<sstream>
#endif
#ifdef PROFILER_CYCLES
#define			ELAPSED_FN		elapsed_cycles
#else
#define			ELAPSED_FN		elapsed_ms
#endif

static const char file[]=__FILE__;
int				prof_on=0;
#ifdef PROFILER_TITLE
std::wstring	title;
#endif
void			prof_toggle()
{
	prof_on=!prof_on;
#ifdef PROFILER_TITLE
	if(prof_on)//start
	{
		const int size=1024;
		title.resize(size);
		GetWindowTextW(ghWnd, &title[0], size);
	}
	else//end
		SetWindowTextW(ghWnd, title.data());
#elif defined PROFILER_CMD
	if(prof_on)//start
		log_start(LL_PROGRESS);
	else//end
		log_end();
#endif
}
typedef std::pair<std::string, double> ProfInfo;
std::vector<ProfInfo> prof;
int				prof_array_start_idx=0;
double			checkpoint=0;
//void			prof_start(){ELAPSED_FN(checkpoint);}
void			prof_add(const char *label, int divisor)
{
	if(prof_on)
	{
		double elapsed=ELAPSED_FN(checkpoint);
		prof.push_back(ProfInfo(label, elapsed));
	}
}
void			prof_sum(const char *label, int count)//add the sum of last 'count' steps
{
	if(prof_on)
	{
		double sum=0;
		for(int k=prof.size()-1, k2=0;k>=0&&k2<count;--k, ++k2)
			sum+=prof[k].second;
		prof.push_back(ProfInfo(label, sum));
	}
}
void			prof_loop_start(const char **labels, int n)//describe the loop body parts in 'labels'
{
	if(prof_on)
	{
		prof_array_start_idx=prof.size();
		for(int k=0;k<n;++k)
			prof.push_back(ProfInfo(labels[k], 0));
	}
}
void			prof_add_loop(int idx)//call on each part of loop body
{
	if(prof_on)
	{
		double elapsed=ELAPSED_FN(checkpoint);
		prof[prof_array_start_idx+idx].second+=elapsed;
	}
}
void			prof_print(PROF_PRINT_ARGS)
{
	if(prof_on)
	{
		static double frametime=0;
		double framedelay=ELAPSED_FN(frametime), fps=1000./framedelay;

#ifdef PROFILER_SCREEN
		const double px_per_ms=2;//0.2 in G2
		const int fontH=16;//18
		//int xpos=w-400, xpos2=w-200;
		GUIPrint(hDC, xlabels, 0, "fps=%lf, T=%lfms", fps, framedelay);
		for(int k=0, kEnd=prof.size();k<kEnd;++k)
		{
			auto &p=prof[k];
			int ypos=(k+2)<<4;
			GUIPrint(hDC, xlabels, ypos, p.first.c_str());
			GUIPrint(hDC, xnumbers, ypos, "%lf", p.second);
		//	GUIPrint(hDC, xnumbers, ypos, "%g", p.second);

			int xstart=xlabels-10, ymiddle=ypos+8;
			MoveToEx(hDC, xstart, ymiddle, 0);
			LineTo(hDC, int(xstart-px_per_ms*p.second), ymiddle);
		}
#elif defined PROFILER_TITLE
		int len=0;
		if(prof.size())
		{
			len+=sprintf_s(g_buf+len, g_buf_size-len, "%s: %lf", prof[0].first.c_str(), prof[0].second);
			for(int k=1, kEnd=prof.size();k<kEnd;++k)
				len+=sprintf_s(g_buf+len, g_buf_size-len, ", %s: %lf", prof[k].first.c_str(), prof[k].second);
			//	len+=sprintf_s(g_buf+len, g_buf_size-len, "%s\t\t%lf\r\n", prof[k].first.c_str(), prof[k].second);
		}
		int success=SetWindowTextA(ghWnd, g_buf);
		SYS_ASSERT(success);
#elif defined PROFILER_CMD
		printf("\n");
		for(int k=0, kEnd=prof.size();k<kEnd;++k)
		{
			auto &p=prof[k];
			printf("%s\t%lf\n", p.first.c_str(), p.second);
		}
		printf("\n");
#endif

		//copy to clipboard
#ifdef PROFILER_CLIPBOARD
		std::stringstream LOL_1;
		for(int k=0, kEnd=prof.size();k<kEnd;++k)
			LOL_1<<prof[k].first<<"\t\t"<<prof[k].second<<"\r\n";
		auto &str=LOL_1.str();
		copy_to_clipboard(str.c_str(), str.size());
	//	int len=0;
	//	for(int k=0, kEnd=prof.size();k<kEnd;++k)
	//		len+=sprintf_s(g_buf+len, g_buf_size-len, "%s\t\t%lf\r\n", prof[k].first.c_str(), prof[k].second);
	//	copy_to_clipboard(g_buf, len);
#endif
		prof.clear();
		//prof_start();
	}
}