#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include "EmbeddedDef.h"

typedef enum
{
    GRAPHICS_COLOR_FMT_RGB565, // 16bit
    GRAPHICS_COLOR_FMT_RGB888, // 24bit
    GRAPHICS_COLOR_FMT_ARGB8888, // 32bit
    GRAPHICS_COLOR_FMT_RGBA8888, // 32bit
    GRAPHICS_COLOR_FMT_ARGB1555, // 16bit
}GRAPHICS_COLOR_FMT_ET;

typedef struct
{
    int32_t x;
    int32_t y;
}GRAPHICS_POINT_ST;

typedef struct
{
    GRAPHICS_POINT_ST pTopLeft;
    int32_t w;
    int32_t h;
}GRAPHICS_RECT_ST;

typedef struct
{
    GRAPHICS_POINT_ST p1;
    GRAPHICS_POINT_ST p2;
}GRAPHICS_LINE_ST;

typedef union
{
    struct
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    }argb;

    uint32_t data32;
}GRAPHICS_PIX_UT;

typedef struct
{
    int32_t w;
    int32_t h;

    int32_t pixSz;
    int32_t canvasSz;
    int32_t lineSz;
    GRAPHICS_COLOR_FMT_ET colorFmt;

    uint8_t canvas[0];
}CANVAS_ATTR_ST;


#define CANVAS_PIX(pCanvas, x, y) (pCanvas->canvas[pCanvas->lineSz * (y) + (x) * pCanvas->pixSz])


#ifdef __cplusplus
extern "C" 
{
#endif

CANVAS_ATTR_ST *CanvasCreate(int32_t w, int32_t h, GRAPHICS_COLOR_FMT_ET fmt);

void CanvasDestroy(CANVAS_ATTR_ST *pCanvas);

void CanvasDrawPoint(CANVAS_ATTR_ST *pCanvas, const GRAPHICS_POINT_ST *p, GRAPHICS_PIX_UT pixColor);

void CanvasDrawLine(CANVAS_ATTR_ST *pCanvas, const GRAPHICS_LINE_ST *pLine, GRAPHICS_PIX_UT pixColor);

void CanvasDrawRect(CANVAS_ATTR_ST *pCanvas, const GRAPHICS_RECT_ST *pRect, GRAPHICS_PIX_UT pixColor);

void CanvasClear(CANVAS_ATTR_ST *pCanvas, GRAPHICS_PIX_UT pixColor);


#ifdef __cplusplus
}
#endif

#endif


