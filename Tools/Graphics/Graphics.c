#include "Graphics.h"
#include "stdio.h"
#include <string.h>
#include "MmMngr.h"
#include "Log.h"

static inline uint32_t colorCnv(CANVAS_ATTR_ST *pCanvas, GRAPHICS_PIX_UT pixColor)
{
    uint32_t pix = 0;

    switch(pCanvas->colorFmt)
    {
        case GRAPHICS_COLOR_FMT_RGB565:
        {
            pix = pixColor.argb.r & 0x1F;
            pix <<= 5;

            pix |= pixColor.argb.g & 0x3F;
            pix <<= 6;

            pix |= pixColor.argb.b & 0x1F;
        }break;
        
        case GRAPHICS_COLOR_FMT_ARGB1555:
        {
            pix |= pixColor.argb.a ? 1 : 0;
            pix <<= 1;

            pix = pixColor.argb.r & 0x1F;
            pix <<= 5;
            
            pix |= pixColor.argb.g & 0x1F;
            pix <<= 5;
            
            pix |= pixColor.argb.b & 0x1F;
        }break;

        case GRAPHICS_COLOR_FMT_ARGB8888:
        {
            return pixColor.data32;
        }break;
        
        case GRAPHICS_COLOR_FMT_RGBA8888:
        {
            pix = pixColor.data32;
            pix <<= 8;

            pix |= pixColor.argb.a;
        }break;
        
        case GRAPHICS_COLOR_FMT_RGB888:
        {
            pix = pixColor.data32 & 0x00FFFFFF;
        }break;

        default:
        {
            pix = pixColor.data32;
        }break;
    }

    return pix;
}

void CanvasDrawPoint(CANVAS_ATTR_ST *pCanvas, const GRAPHICS_POINT_ST *pt, GRAPHICS_PIX_UT pixColor)
{
    uint32_t pix = colorCnv(pCanvas, pixColor);
    memcpy(&CANVAS_PIX(pCanvas, pt->x, pt->y), &pix, pCanvas->pixSz);
}


void CanvasDrawLine(CANVAS_ATTR_ST *pCanvas, const GRAPHICS_LINE_ST *pLine, GRAPHICS_PIX_UT pixColor)
{
    GRAPHICS_POINT_ST p = {0};

    int dx = pLine->p2.x - pLine->p1.x;
    int dy = pLine->p2.y - pLine->p1.y;
    int ux = ((dx > 0) << 1) - 1;//x的增量方向，取1或-1
    int uy = ((dy > 0) << 1) - 1;//y的增量方向，取1或-1
    
    int eps = 0;//eps为累加误差

    dx = abs(dx);
    dy = abs(dy);

    p.x = pLine->p1.x;
    p.y = pLine->p1.y;
    
    if (dx > dy) 
    {
        for (; p.x != pLine->p2.x; p.x += ux)
        {
            CanvasDrawPoint(pCanvas, &p, pixColor);
            
            eps += dy;
            if ((eps << 1) >= dx)
            {
                p.y += uy;
                eps -= dx;
            }
        }
        CanvasDrawPoint(pCanvas, &p, pixColor);
    }
    else
    {
        for (; p.y != pLine->p2.y; p.y += uy)
        {
            CanvasDrawPoint(pCanvas, &p, pixColor);
            
            eps += dx;
            if ((eps << 1) >= dy)
            {
                p.x += ux;
                eps -= dy;
            }
        }
        CanvasDrawPoint(pCanvas, &p, pixColor);
    }
}

void CanvasDrawRect(CANVAS_ATTR_ST *pCanvas, const GRAPHICS_RECT_ST *pRect, GRAPHICS_PIX_UT pixColor)
{
    GRAPHICS_LINE_ST line[4] = {0};

    line[0].p1.x = pRect->pTopLeft.x;
    line[0].p1.y = pRect->pTopLeft.y;
    line[0].p2.x = pRect->pTopLeft.x + pRect->w;
    line[0].p2.y = pRect->pTopLeft.y;
    
    line[1].p1.x = pRect->pTopLeft.x;
    line[1].p1.y = pRect->pTopLeft.y;
    line[1].p2.x = pRect->pTopLeft.x;
    line[1].p2.y = pRect->pTopLeft.y + pRect->h;

    line[2].p1.x = pRect->pTopLeft.x + pRect->w;
    line[2].p1.y = pRect->pTopLeft.y;
    line[2].p2.x = pRect->pTopLeft.x + pRect->w;
    line[2].p2.y = pRect->pTopLeft.y + pRect->h;

    line[3].p1.x = pRect->pTopLeft.x;
    line[3].p1.y = pRect->pTopLeft.y + pRect->h;
    line[3].p2.x = pRect->pTopLeft.x + pRect->w;
    line[3].p2.y = pRect->pTopLeft.y + pRect->h;
    
    CanvasDrawLine(pCanvas, &line[0], pixColor);
    CanvasDrawLine(pCanvas, &line[1], pixColor);
    CanvasDrawLine(pCanvas, &line[2], pixColor);
    CanvasDrawLine(pCanvas, &line[3], pixColor);
}


void CanvasClear(CANVAS_ATTR_ST *pCanvas, GRAPHICS_PIX_UT pixColor)
{
    uint32_t pix = colorCnv(pCanvas, pixColor);
    
    for(int32_t i = 0; i < pCanvas->canvasSz; i += pCanvas->pixSz)
    {
        memcpy(&pCanvas->canvas[i], &pix, pCanvas->pixSz);
    }
}

CANVAS_ATTR_ST *CanvasCreate(int32_t w, int32_t h, GRAPHICS_COLOR_FMT_ET fmt)
{
    int pixSz = 0;

    switch(fmt)
    {
        case GRAPHICS_COLOR_FMT_RGB565:
        case GRAPHICS_COLOR_FMT_ARGB1555:
        {
            pixSz = 2;
        }break;

        case GRAPHICS_COLOR_FMT_ARGB8888:
        case GRAPHICS_COLOR_FMT_RGBA8888:
        {
            pixSz = 4;
        }break;
        
        case GRAPHICS_COLOR_FMT_RGB888:
        {
            pixSz = 3;
        }break;

        default:
        {
            pixSz = 3; // 24 bit
        }break;
    }

    CANVAS_ATTR_ST *pCanvas = MmMngrMalloc(w * h * pixSz + sizeof *pCanvas);
    if(!pCanvas)
    {
        LOGE("canvas malloc failed w = %d, h = %d, pixsz = %d\r\n", w, h, pixSz);
        goto err1;
    }

    pCanvas->w = w;
    pCanvas->h = h;

    pCanvas->pixSz = pixSz;
    pCanvas->colorFmt = fmt;
    pCanvas->lineSz = pCanvas->w * pCanvas->pixSz;
    pCanvas->canvasSz = pCanvas->lineSz * pCanvas->h;

    return pCanvas;
err1:
    return NULL;
}


void CanvasDestroy(CANVAS_ATTR_ST *pCanvas)
{
    MmMngrFree(pCanvas);
}


