#include "mechwarrior/combat_render.h"
/*22B6:6073..6472. Authored CS ratio-angle table, NOT304E state. */
uint16_t headingRatioAngles[0x200];
uint16_t orientationCosines[0x801];
uint16_t coefficientAngles[0x402];
uint8_t vertexNearPlaneState[0xFF];
BtechGeometryCoordinateCache geometryViewX,geometryViewDepth,geometryViewZ;
BtechGeometryCoordinateCache geometryProjectedX,geometryProjectedY;
uint8_t polygonShadeDeltas[POLYGON_SHADE_CLASSES][POLYGON_SHADE_STEPS];
uint8_t polygonVertexIndices[POLYGON_VERTEX_INDEX_BYTES];
uint32_t wideScreenX[WIDE_POLYGON_VERTICES],wideScreenY[WIDE_POLYGON_VERTICES];
uint32_t widePolygonWorkX[WIDE_POLYGON_VERTICES],widePolygonWorkY[WIDE_POLYGON_VERTICES];
