/*
image_jpeg.cpp
Inkplate 6 Arduino library
David Zovko, Borna Biro, Denis Vajak, Zvonimir Haramustek @ e-radionica.com
September 24, 2020
https://github.com/e-radionicacom/Inkplate-6-Arduino-library

For support, please reach over forums: forum.e-radionica.com/en
For more info about the product, please check: www.inkplate.io

This code is released under the GNU Lesser General Public License v3.0: https://www.gnu.org/licenses/lgpl-3.0.en.html
Please review the LICENSE file included with this example.
If you have any questions about licensing, please contact techsupport@e-radionica.com
Distributed as-is; no warranty is given.
*/

#include "image.hpp"

#include "tjpg_decoder.hpp"

extern Image *_imagePtrJpeg;

bool Image::drawJpegFromSd(const char *fileName, int x, int y, bool dither, bool invert)
{
    SdFile dat;
    if (dat.open(fileName, O_RDONLY))
        return drawJpegFromSd(&dat, x, y, dither, invert);
    return 0;
}

bool Image::drawJpegFromSd(SdFile *p, int x, int y, bool dither, bool invert)
{
    uint8_t ret = 0;

    blockW = -1;
    blockH = -1;
    lastY = -1;
    memset(ditherBuffer, 0, sizeof ditherBuffer);

    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(drawJpegChunk);

    uint32_t pnt = 0;
    uint32_t total = p->fileSize();
    uint8_t *buff = (uint8_t *)ps_malloc(total);

    if (buff == NULL)
        return 0;

    while (pnt < total)
    {
        uint32_t toread = p->available();
        if (toread > 0)
        {
            int read = p->read(buff + pnt, toread);
            if (read > 0)
                pnt += read;
        }
    }
    p->close();

    if (TJpgDec.drawJpg(x, y, buff, total, dither, invert) == 0)
        ret = 1;

    free(buff);

    return ret;
}

bool Image::drawJpegFromWeb(const char *url, int x, int y, bool dither, bool invert)
{
    bool ret = 0;

    int32_t defaultLen = E_INK_WIDTH * E_INK_HEIGHT * 4;
    uint8_t *buff = downloadFile(url, &defaultLen);

    ret = drawJpegFromBuffer(buff, defaultLen, x, y, dither, invert);
    free(buff);

    return ret;
}

bool Image::drawJpegFromWebAtPosition(const char *url, const Position& position, const bool dither, const bool invert)
{
    bool ret = 0;

    int32_t defaultLen = 800 * 600 * 4;
    uint8_t *buff = downloadFile(url, &defaultLen);

    uint16_t w = 0;
    uint16_t h = 0;
    TJpgDec.setJpgScale(1);
    JRESULT r = TJpgDec.getJpgSize(&w, &h, buff, defaultLen);
    if(r != JDR_OK) {
        free(buff);
        return false;
    }

    uint16_t posX, posY;
    getPointsForPosition(position, w, h, 800, 600, &posX, &posY);

    ret = drawJpegFromBuffer(buff, defaultLen, posX, posY, dither, invert);
    free(buff);

    return ret;
}

bool Image::drawJpegFromBuffer(uint8_t *buff, int32_t len, int x, int y, bool dither, bool invert)
{
    bool ret = 0;

    blockW = -1;
    blockH = -1;
    lastY = -1;

    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(drawJpegChunk);

    int err = TJpgDec.drawJpg(x, y, buff, len, dither, invert);
    if (err == 0)
        ret = 1;

    return ret;
};

bool Image::drawJpegChunk(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap, bool dither, bool invert)
{
    if (!_imagePtrJpeg)
        return 0;

    if (dither && y != _imagePtrJpeg->lastY)
    {
        _imagePtrJpeg->ditherSwap(E_INK_WIDTH);
        _imagePtrJpeg->lastY = y;
    }

    _imagePtrJpeg->startWrite();
    for (int j = 0; j < h; ++j)
    {
        for (int i = 0; i < w; ++i)
        {
            uint16_t rgb = bitmap[j * w + i];
            uint8_t val;
            if (dither)
                val = _imagePtrJpeg->ditherGetPixelJpeg(RGB8BIT(RED(rgb), GREEN(rgb), BLUE(rgb)), i, j, x, y, w, h);
            else
                val = RGB3BIT(RED(rgb), GREEN(rgb), BLUE(rgb));
            if (invert)
                val = 7 - val;
            if (_imagePtrJpeg->getDisplayMode() == INKPLATE_1BIT)
                val = (~val >> 2) & 1;
            _imagePtrJpeg->writePixel(x + i, y + j, val);
        }
    }
    if (dither)
        _imagePtrJpeg->ditherSwapBlockJpeg(x);
    _imagePtrJpeg->endWrite();

    return 1;
}
