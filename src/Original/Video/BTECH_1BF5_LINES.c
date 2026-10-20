#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

/*1BF5:035C. Original top/left/bottom/right order and endpoint exchanges.
 * Lower bounds are signed; upper bounds and final X ordering are UNSIGNED. */
void BtechDrawClippedLine(int16_t firstX,int16_t firstY,int16_t secondX,int16_t secondY)
{
    uint16_t x0=(uint16_t)firstX,y0=(uint16_t)firstY,x1=(uint16_t)secondX,y1=(uint16_t)secondY;
    if (screenPolygonNeedsClip)
        for (uint16_t boundary=0;boundary<4;++boundary)
        {
            bool vertical=!(boundary&1),upper=boundary>=2;
            uint16_t limit=vertical?(upper?clipMaximumY:clipMinimumY):(upper?clipMaximumX:clipMinimumX);
            uint16_t start=vertical?y0:x0,end=vertical?y1:x1;
            bool startOutside=upper?start>limit:(int16_t)start<(int16_t)limit;
            bool endOutside=upper?end>limit:(int16_t)end<(int16_t)limit;
            if (!startOutside)
            {
                if (!endOutside) continue;
                uint16_t saved=x0;x0=x1;x1=saved;saved=y0;y0=y1;y1=saved;
            }
            else if (endOutside) return;
            start=vertical?y0:x0;end=vertical?y1:x1;
            int16_t difference=vertical?x1-x0:y1-y0;
            int16_t distance=limit-start;
            int32_t product=(int32_t)difference*distance;
            int16_t divisor=end-start;
            if (!divisor || (product==INT32_MIN && divisor==-1)) abort();
            int32_t quotient=product/divisor;
            if (quotient<INT16_MIN || quotient>INT16_MAX) abort();
            if (vertical) {x0=(uint16_t)(x0+quotient);y0=limit;}
            else {y0=(uint16_t)(y0+quotient);x0=limit;}
        }
    if (x0>x1) {uint16_t saved=x0;x0=x1;x1=saved;saved=y0;y0=y1;y1=saved;}
    if (!graphicsDrawLine) abort(); /* Real EGA line backend remains a dependency. */
    graphicsDrawLine(x0,y0,x1,y1,drawPageSelector);
}

/*1BF5:082E. Native register entry; next endpoint is passed before previous. */
void BtechDrawScreenPolyline(uint16_t segments,const int16_t *x,const int16_t *y)
{
    if (!segments || segments>=COMBAT_POLYGON_VERTEX_WORDS) abort(); /* Native zero wraps into an unbounded source walk. */
    do
    {
        int16_t previousY=*y,previousX=*x;++x;++y;
        int16_t nextY=*y,nextX=*x;
        BtechDrawClippedLine(nextX,nextY,previousX,previousY);
    } while (--segments);
}
