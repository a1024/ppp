#include		"ppp.h"
#include		"generic.h"
#include		<string>
#include		<wchar.h>//toupper
#include		<assert.h>
template<typename INT> inline INT readint(const wchar_t *str, int *advance)
{
	if(!str||(str[0]<'0'||str[0]>'9')&&(!str[0]||str[0]!='-'&&str[0]!='+'||str[1]<'0'||str[1]>'9'))
//	if(!str||!(*str>='0'&&*str<='9'||*str&&(*str=='-'||*str=='+')&&str[1]>='0'&&str[1]<='9'))
	{
		if(advance)
			*advance=0;
		return 0;
	}
	int start=str[0]=='-'||str[0]=='+', end=start;
	for(;str[end]&&str[end]>='0'&&str[end]<='9';++end);
	INT p=1, val=0;
	for(int k=end-1;k>=start;--k, p*=10)
		val+=(str[k]-'0')*p;
	if(advance)
		*advance=end;
	int neg=str[0]=='-';
	return (val^-(INT)neg)+neg;
}
extern "C"
{
	long long	acme_wtoll(const wchar_t *str, int *ret_advance){return readint<long long>(str, ret_advance);}
	int			acme_wtoi(const wchar_t *str, int *ret_advance){return readint<int>(str, ret_advance);}
}

void			assign_path(std::wstring const &text, int start, int end, std::wstring &pathret)//pathret can be text
{
	for(;end>0&&(text[end-1]==' '||text[end-1]=='\t'||text[end-1]=='\r'||text[end-1]=='\n');--end);//skip trailing whitespace
	start+=text[start]==doublequote, end-=text[end-1]==doublequote;
	assert(start<end);
	std::wstring temp(text.begin()+start, text.begin()+end);
	int size=temp.size();
	for(int k=0;k<size;++k)
	{
		if(temp[k]==L'\\')
			temp[k]=L'/';
	}
	if(temp[size-1]=='/')
		temp.pop_back();
	pathret=std::move(temp);
}
void			assign_path(std::string const &text, int start, int end, std::wstring &pathret)//pathret can be text
{
	for(;end>0&&(text[end-1]==' '||text[end-1]=='\t'||text[end-1]=='\r'||text[end-1]=='\n');--end);//skip trailing whitespace
	start+=text[start]==doublequote, end-=text[end-1]==doublequote;
	assert(start<end);
	std::wstring temp(text.begin()+start, text.begin()+end);
	int size=temp.size();
	for(int k=0;k<size;++k)
	{
		if(temp[k]==L'\\')
			temp[k]=L'/';
	}
	if(temp[size-1]=='/')
		temp.pop_back();
	pathret=std::move(temp);
}
void			assign_path(std::string const &text, int start, int end, std::string &pathret)//pathret can be text
{
	assert(text.size());
	if(end>=(int)text.size())
		end=text.size();
	for(;end>0&&(text[end-1]==' '||text[end-1]=='\t'||text[end-1]=='\r'||text[end-1]=='\n');--end);//skip trailing whitespace
	start+=text[start]==doublequote, end-=text[end-1]==doublequote;
	assert(start<end);
	std::string temp(text.begin()+start, text.begin()+end);
	int size=temp.size();
	for(int k=0;k<size;++k)
	{
		if(temp[k]==L'\\')
			temp[k]=L'/';
	}
	if(temp[size-1]=='/')
		temp.pop_back();
	pathret=std::move(temp);
}
void			get_name_from_path(std::wstring const &path, std::wstring &name)
{
	int start=-1;
	for(int k=path.size()-1;k>=0;--k)
	{
		if(path[k]==L'/'||path[k]==L'\\')
		{
			start=k+1;
			break;
		}
	}
	if(start==-1)
		start=0;//unreachable
	name.assign(path.begin()+start, path.end());
}

bool			skip_whitespace(std::wstring const &text, int &k)
{
	const int size=text.size();
	for(;k<size&&(text[k]==L' '||text[k]==L'\t'||text[k]==L'\r'||text[k]==L'\n');++k);
	return k>=size;
}
bool			skip_whitespace(std::string const &text, int &k)
{
	const int size=text.size();
	for(;k<size&&(text[k]==' '||text[k]=='\t'||text[k]=='\r'||text[k]=='\n');++k);
	return k>=size;
}
bool			compare_string_caseinsensitive(std::wstring const &text, int &k, const wchar_t *label, int label_size)
{
	const int size=text.size();
	if(size-k<label_size)
		return false;
	int k2=k;
	for(int k3=0;k2<size&&k3<label_size;++k2, ++k3)
		if(towupper(text[k2])!=towupper(label[k3]))
			return false;
	k=k2;
	return true;
}
bool			compare_string_caseinsensitive(std::string const &text, int &k, const char *label, int label_size)
{
	const int size=text.size();
	if(size-k<label_size)
		return false;
	int k2=k;
	for(int k3=0;k2<size&&k3<label_size;++k2, ++k3)
		if(towupper(text[k2])!=towupper(label[k3]))
			return false;
	k=k2;
	return true;
}
int				seek_path(std::wstring const &text, int &k)//k should point at path start / opening quote, returns true if path is enclosed in double quotes
{
	const int size=text.size();
	char quotes=text[k]==doublequote;
	if(quotes)
	{
		++k;
		for(;k<size&&text[k]!=doublequote;++k);
		return true;//k at closing quote
	}
	for(;k<size&&text[k]!=L' '&&text[k]!=L'\t'&&text[k]!=L'\r'&&text[k]!=L'\n';++k);//skip till whitespace
	return false;
}
bool			seek_token(std::string const &text, int &k, const char *token)//skip till token is found
{
	const int size=text.size(), token_size=strlen(token);
	for(int k2=k;k2<size;++k2)
	{
		bool match=true;
		for(int k3=0;k2+k3<size&&k3<token_size;++k3)
		{
			if(text[k2+k3]!=token[k3])
			{
				match=false;
				break;
			}
		}
		if(match)
		{
			k=k2;
			return true;
		}
	}
	return false;
}
bool			parse_path_field(std::wstring const &text, int &k, const wchar_t *token, std::wstring &result)//returns true if has spaces & quotes
{
	const wchar_t path_token[]=L"path:";
	int path_size=lstrlenW(path_token), token_size=lstrlenW(token);
	skip_whitespace(text, k);
	char match=compare_string_caseinsensitive(text, k, token, token_size);
	skip_whitespace(text, k);
	match|=(char)compare_string_caseinsensitive(text, k, path_token, path_size);
	skip_whitespace(text, k);
	if(match)
	{
		int k0=k;
		int quotes=seek_path(text, k)!=0;
		result.assign(text.begin()+k0, text.begin()+k);
		k+=quotes;
		//for(int k2=0, size=result.size();k2<size;++k)
		//{
		//	if(result[k2]==' ')
		//	{
		//		if(result[0]!=doublequote)
		//			result.insert(result.begin(), doublequote);//unreachable
		//		quotes=true;
		//		break;
		//	}
		//}
		return quotes!=0;
	}
	return false;
}
unsigned		read_unsigned_int(std::string const &text, int &k)//k at first decimal digit
{
	const int size=text.size();
	int k0=k, p=1;
	for(;k<size&&text[k]>='0'&&text[k]<='9';++k);//skip till end of number
	unsigned value=0;
	for(int k2=k-1;k2>=k0;--k2, p*=10)
		value+=(text[k2]-'0')*p;
	return value;
}
double			read_unsigned_float(std::string const &text, int &k)//k at first decimal digit
{
	const int size=text.size();
	int k0=k, kpoint=-1;
	double p=1;
	for(;k<size&&(text[k]>='0'&&text[k]<='9'||text[k]=='.');++k)//skip till end of number
	{
		if(text[k]=='.'&&kpoint==-1)
			kpoint=k;
		else if(kpoint!=-1)
			p*=0.1;
	}
	double value=0;
	for(int k2=k-1;k2>=k0;--k2)
		if(text[k2]!='.')
			value+=(text[k2]-'0')*p, p*=10;
	return value;
}

extern "C" int	acme_strcmp_ci(const wchar_t *s1, const wchar_t *s2)//returns zero on match, n+1 on mismatch at n
{
	int k=1;
	for(;*s1&&*s2&&acme_tolower(*s1)==acme_tolower(*s2);++s1, ++s2, ++k);
	if(*s1==*s2)
		return 0;
	return k;
}
int				acme_matchlabel_ci(const wchar_t *text, int size, int &k, const wchar_t *label, int &advance)
{
	int kL=0, kT=k;
	for(;label[kL]&&text[kT]&&acme_tolower(text[kT])==acme_tolower(label[kL]);++kT, ++kL);
	advance=kL&-!label[kL];
	k+=advance;
	return k>=size;
}
int				skip_till_newline_or_null(const wchar_t *text, int size, int &k, int &advance)
{
	for(;text[k]&&text[k]!='\r'&&text[k]!='\n';++k);
	return k>=size;
}