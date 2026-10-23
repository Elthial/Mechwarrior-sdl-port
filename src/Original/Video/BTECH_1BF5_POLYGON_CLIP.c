#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

/*1BF5:2FB5. Both X boundaries in one walk, then both Y boundaries in one
 * walk. The original endpoint-emission order is not four Sutherland passes. */
void BtechClipScreenPolygonToViewport(void)
{
    if ((int16_t)polygonVertexCount<=1) return;
    for (uint16_t axis=0;axis<2;++axis)
    {
        int16_t *sourceX=axis?polygonWorkX:screenPolygonX;
        int16_t *sourceY=axis?polygonWorkY:screenPolygonY;
        int16_t *destinationX=axis?screenPolygonX:polygonWorkX;
        int16_t *destinationY=axis?screenPolygonY:polygonWorkY;
        int16_t *plane=axis?sourceY:sourceX,*other=axis?sourceX:sourceY;
        int16_t *outPlane=axis?destinationY:destinationX,*outOther=axis?destinationX:destinationY;
        int16_t minimum=axis?clipMinimumY:clipMinimumX;
        int16_t maximum=axis?clipMaximumY:clipMaximumX;
        uint8_t minimumBit=axis?8:1,maximumBit=axis?4:2;
        if (polygonVertexCount>COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: polygonVertexCount>COMBAT_POLYGON_VERTEX_WORDS");
        uint16_t previous=polygonVertexCount-1,output=0;
        uint8_t previousOutside=(plane[previous]<minimum?minimumBit:0)|(plane[previous]>maximum?maximumBit:0);
        for (uint16_t current=0;current!=polygonVertexCount;++current)
        {
            uint8_t currentOutside=(plane[current]<minimum?minimumBit:0)|(plane[current]>maximum?maximumBit:0);
            if (!(previousOutside|currentOutside))
            {
                if (output>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=COMBAT_POLYGON_VERTEX_WORDS");
                destinationX[output]=sourceX[current];destinationY[output]=sourceY[current];++output;
            }
            else if (!(previousOutside&currentOutside))
            {
                for (uint16_t endpoint=0;endpoint<2;++endpoint)
                {
                    uint8_t outside=endpoint?currentOutside:previousOutside;
                    if (!outside) continue;
                    uint16_t base=endpoint?current:previous,next=endpoint?previous:current;
                    if (output>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=COMBAT_POLYGON_VERTEX_WORDS");
                    int16_t boundary=outside&minimumBit?minimum:maximum;
                    outPlane[output]=boundary; /* Native store precedes IDIV, even on a fault. */
                    int16_t distance=boundary-plane[base];
                    int16_t difference=other[next]-other[base];
                    int32_t product=(int32_t)difference*distance;
                    int16_t divisor=plane[next]-plane[base];
                    if (!divisor || (product==INT32_MIN && divisor==-1)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !divisor || (product==INT32_MIN && divisor==-1)");
                    int32_t quotient=product/divisor;
                    if (quotient<INT16_MIN || quotient>INT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: quotient<INT16_MIN || quotient>INT16_MAX");
                    outOther[output]=(int16_t)(quotient+other[base]);++output;
                }
                if (!currentOutside)
                {
                    if (output>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=COMBAT_POLYGON_VERTEX_WORDS");
                    destinationX[output]=sourceX[current];destinationY[output]=sourceY[current];++output;
                }
            }
            previous=current;previousOutside=currentOutside;
        }
        polygonVertexCount=output;
        if (!axis && (int16_t)polygonVertexCount<=1)
        {
            for (uint16_t coordinateAxis=0;coordinateAxis<2;++coordinateAxis)
            {
                int16_t *source=coordinateAxis?polygonWorkY:polygonWorkX;
                int16_t *destination=coordinateAxis?screenPolygonY:screenPolygonX;
                for (uint16_t i=0;i<polygonVertexCount;++i) destination[i]=source[i];
            }
            return;
        }
    }
}
