#ifndef			PPP_INLINE_CHECK_H
#define			PPP_INLINE_CHECK_H
extern int		w, h;
extern int		iw, ih;
inline bool		checkx(int x){return x<0||x>=w;}
inline bool		checky(int y){return y<0||y>=h;}
inline bool		check(int x, int y){return x<0||x>=w||y<0||y>=h;}
inline bool		icheckx(int x){return x<0||x>=iw;}
inline bool		ichecky(int y){return y<0||y>=ih;}
inline bool		icheck(int x, int y){return x<0||x>=iw||y<0||y>=ih;}
#endif