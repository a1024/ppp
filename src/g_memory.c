#include		<string.h>
//#include		<assert.h>
static const char file[]=__FILE__;
void			memfill(void *dst, const void *src, int dstbytes, int srcbytes)//repeating pattern
{
#if 1//1st place	1.83~2.11ms
	int copied;
	char *d=(char*)dst;
	const char *s=(const char*)src;
	if(dstbytes<=0||srcbytes<=0)//
		return;//
	if(dstbytes<srcbytes)
	{
		memcpy(dst, src, dstbytes);
		return;
	}
	copied=srcbytes;
	memcpy(d, s, copied);
	while(copied<<1<=dstbytes)
	{
		memcpy(d+copied, d, copied);
		copied<<=1;
	}
	if(copied<dstbytes)
		memcpy(d+copied, d, dstbytes-copied);
#endif
#if 0//1st place	1.83~2.46ms
	unsigned char *p1, *p2, *end;
	const int sizelong=sizeof(long), sizelongmask=sizelong-1;
	if(dstbytes<srcbytes)
	{
		memcpy(dst, src, dstbytes);
		return;
	}
	memcpy(dst, src, srcbytes);//initialize first pattern
	p1=(unsigned char*)dst, p2=p1+srcbytes, end=p1+dstbytes;
	__stosd((unsigned long*)p2, *(unsigned long*)p1, (dstbytes-srcbytes)/sizelong);
	__stosb(p1+(dstbytes&~sizelongmask), *p1, dstbytes&sizelongmask);
#endif
#if 0//2nd place	3.64ms
	char *p1, *p2, *end;
	if(dstbytes<srcbytes)
	{
		memcpy(dst, src, dstbytes);
		return;
	}
	memcpy(dst, src, srcbytes);//initialize first pattern
	p1=(char*)dst, p2=p1+srcbytes, end=p1+dstbytes;
	while(p2<end)//fill pattern
		*p2++=*p1++;
#endif
#if 0//last place	3.9ms
	unsigned char *c1, *c2;
	int phaseshift, k, dx, xround;
	if(dstbytes<srcbytes)
	{
		memcpy(dst, src, dstbytes);
		return;
	}
	memcpy(dst, src, srcbytes);
	c1=(unsigned char*)dst;
	for(k=srcbytes;k<16;++k)//in case srcbytes<16
		c1[k]=c1[k-srcbytes];
	phaseshift=k;
	dx=dstbytes-phaseshift;
	xround=dx&~15;
	c2=c1+phaseshift;
	for(k=0;k<xround;k+=phaseshift)
		_mm_storeu_ps((float*)(c2+k), _mm_loadu_ps((float*)(c1+k)));
	for(k=xround;k<dx;++k)
		c2[k]=c1[k];
#endif
}