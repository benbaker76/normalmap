/*
    normalmap - Convert PNGs from height maps to normal maps for games.
    Copyright (C) 2016 Tony Houghton <h@realh.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "normalmap.h"

static int byte_offset(char axis)
{
    switch (axis)
    {
        case 'r':
            return 0;
        case 'g':
            return 1;
        case 'b':
            return 2;
        case 'a':
            return 3;
    }
    return 0;
}

typedef struct
{
    double x, y, z;
} NormalVector;

/* Modify the sample_pixel function to work within tile bounds */
inline static unsigned char sample_pixel(NormalmapPng *heightmap,
        unsigned x, unsigned y)
{
    return heightmap->data[y * heightmap->info.width + x];
}

/* Modify pixel_diff to confine differences within the tile */
inline static int pixel_diff(NormalmapPng *heightmap,
        unsigned x1, unsigned x2, unsigned y1, unsigned y2,
        unsigned tile_x, unsigned tile_y, unsigned tile_width, unsigned tile_height)
{
    if (x1 < tile_x || x1 >= tile_x + tile_width || y1 < tile_y || y1 >= tile_y + tile_height)
        return 0;

    if (x2 < tile_x || x2 >= tile_x + tile_width || y2 < tile_y || y2 >= tile_y + tile_height)
        return 0;

    return (int)sample_pixel(heightmap, x2, y2) - (int)sample_pixel(heightmap, x1, y1);
}

/* Update row_dh to limit processing to within the tile */
static int row_dh(NormalmapPng *heightmap, unsigned x, unsigned y, int wrap,
                  unsigned tile_x, unsigned tile_y, unsigned tile_width, unsigned tile_height)
{
    unsigned w = tile_width;  // Tile width

    if (x == 1)
        return 0;

    if (x == 0)
    {
        if (wrap)
        {
            return -pixel_diff(heightmap, 1, w - 1, y, y, tile_x, tile_y, tile_width, tile_height);
        }
        else
        {
            return -pixel_diff(heightmap, 0, 1, y, y, tile_x, tile_y, tile_width, tile_height);
        }
    }
    else if (x == w - 1)
    {
        if (wrap)
        {
            return -pixel_diff(heightmap, w - 2, 0, y, y, tile_x, tile_y, tile_width, tile_height);
        }
        else
        {
            return -pixel_diff(heightmap, w - 2, w - 1, y, y, tile_x, tile_y, tile_width, tile_height);
        }
    }
    /* else */
    return -pixel_diff(heightmap, x - 1, x + 1, y, y, tile_x, tile_y, tile_width, tile_height);
}

/* Update col_dh to limit processing to within the tile */
static int col_dh(NormalmapPng *heightmap, unsigned x, unsigned y, int wrap,
                  unsigned tile_x, unsigned tile_y, unsigned tile_width, unsigned tile_height)
{
    unsigned h = tile_height;  // Tile height

    if (h == 1)
        return 0;

    if (y == 0)
    {
        if (wrap)
        {
            return pixel_diff(heightmap, x, x, 1, h - 1, tile_x, tile_y, tile_width, tile_height);
        }
        else
        {
            return pixel_diff(heightmap, x, x, 0, 1, tile_x, tile_y, tile_width, tile_height);
        }
    }
    else if (y == h - 1)
    {
        if (wrap)
        {
            return pixel_diff(heightmap, x, x, h - 2, 0, tile_x, tile_y, tile_width, tile_height);
        }
        else
        {
            return pixel_diff(heightmap, x, x, h - 2, h - 1, tile_x, tile_y, tile_width, tile_height);
        }
    }
    /* else */
    return pixel_diff(heightmap, x, x, y - 1, y + 1, tile_x, tile_y, tile_width, tile_height);
}

/* Modify sobel to work within tile bounds */
static NormalVector sobel(NormalmapPng *heightmap, unsigned x, unsigned y,
        double scale, int wrap, unsigned tile_x, unsigned tile_y,
        unsigned tile_width, unsigned tile_height)
{
    NormalVector nv;
    int dh;
    unsigned w = tile_width;
    unsigned h = tile_height;
    double div = 5.0;

    dh = row_dh(heightmap, x, y, wrap, tile_x, tile_y, tile_width, tile_height) * 2;
    if (y == 0)
    {
        if (wrap)
            dh += row_dh(heightmap, x, h - 1, wrap, tile_x, tile_y, tile_width, tile_height);
        else
            div -= 1.0;
    }
    else
    {
        dh += row_dh(heightmap, x, y - 1, wrap, tile_x, tile_y, tile_width, tile_height);
    }
    if (y == h - 1)
    {
        if (wrap)
            dh += row_dh(heightmap, x, 0, wrap, tile_x, tile_y, tile_width, tile_height);
        else
            div -= 1.0;
    }
    else
    {
        dh += row_dh(heightmap, x, y + 1, wrap, tile_x, tile_y, tile_width, tile_height);
    }
    nv.x = scale * (double) dh / div;

    div = 5.0;
    dh = col_dh(heightmap, x, y, wrap, tile_x, tile_y, tile_width, tile_height) * 2;
    if (x == 0)
    {
        if (wrap)
            dh += col_dh(heightmap, w - 1, y, wrap, tile_x, tile_y, tile_width, tile_height);
        else
            div -= 1.0;
    }
    else
    {
        dh += col_dh(heightmap, x - 1, y, wrap, tile_x, tile_y, tile_width, tile_height);
    }
    if (x == w - 1)
    {
        if (wrap)
            dh += col_dh(heightmap, 0, y, wrap, tile_x, tile_y, tile_width, tile_height);
        else
            div -= 1.0;
    }
    else
    {
        dh += col_dh(heightmap, x + 1, y, wrap, tile_x, tile_y, tile_width, tile_height);
    }
    nv.y = scale * (double) dh / div;

    div = sqrt(nv.x * nv.x + nv.y * nv.y + 1.0);
    nv.x /= div;
    nv.y /= div;
    nv.z = 1.0 / div;

    return nv;
}

static unsigned char d_to_signed_byte(double v)
{
    v *= 128;
    if (v == 128)
        v = 127;
    return (unsigned char) v + 128;
}

static unsigned char d_to_unsigned_byte(double v)
{
    v *= 256;
    if (v == 256)
        v = 255;
    return (unsigned char) v;
}

/* Modify normalmap_convert to handle tile-by-tile processing with correct indexing */
NormalmapPng *normalmap_convert(NormalmapPng *heightmap,
        NormalmapOptions *options)
{
    unsigned bpp;           /* bytes per pixel */
    NormalmapPng *nmap;
    int xo, yo, zo;
    unsigned int p_y, p_x, n;
    double scale;
    unsigned int tile_x, tile_y, tile_width, tile_height;

    nmap = normalmap_png_new();
    nmap->info.width = heightmap->info.width;
    nmap->info.height = heightmap->info.height;
    if (strchr(options->xyz, 'a'))
    {
        bpp = 4;
        nmap->info.format = PNG_FORMAT_RGBA;
    }
    else
    {
        bpp = 3;
        nmap->info.format = PNG_FORMAT_RGB;
    }
    nmap->data = malloc(nmap->info.height * nmap->info.width * bpp);
    memset(nmap->data, 0, nmap->info.height * nmap->info.width * bpp);

    xo = byte_offset(options->xyz[0]);
    yo = byte_offset(options->xyz[1]);
    zo = byte_offset(options->xyz[2]);

    /* Normalization logic remains the same */
    if (options->normalise)
    {
        unsigned char min = 255, max = 0;
        unsigned char pix;

        p_x = heightmap->info.height * heightmap->info.width;
        for (p_y = 0; p_y < p_x; ++p_y)
        {
            pix = heightmap->data[p_y];
            if (pix > max)
                max = pix;
            if (pix < min)
                min = pix;
        }
        scale = (double) (max - min) / 255.0 * options->scale;
    }
    else
    {
        scale = options->scale / 255.0;
    }

    /* Process in tiles */
    tile_width = options->tilesize > 0 ? options->tilesize : nmap->info.width;
    tile_height = options->tilesize > 0 ? options->tilesize : nmap->info.height;

    for (tile_y = 0; tile_y < nmap->info.height; tile_y += tile_height)
    {
        for (tile_x = 0; tile_x < nmap->info.width; tile_x += tile_width)
        {
            /* Process each tile within bounds */
            for (p_y = 0; p_y < tile_height && tile_y + p_y < nmap->info.height; ++p_y)
            {
                for (p_x = 0; p_x < tile_width && tile_x + p_x < nmap->info.width; ++p_x)
                {
                    /* Use local coordinates for tile processing */
                    unsigned x = tile_x + p_x;
                    unsigned y = tile_y + p_y;
                    n = (y * nmap->info.width + x) * bpp;

                    /* Process the pixel using tile-local coordinates */
                    NormalVector v = sobel(heightmap, x, y, scale, options->wrap,
                                           tile_x, tile_y, tile_width, tile_height);

                    nmap->data[n + xo] = d_to_signed_byte(v.x);
                    nmap->data[n + yo] = d_to_signed_byte(v.y);
                    nmap->data[n + zo] = options->unsigned_z ?
                        d_to_unsigned_byte(v.z) : d_to_signed_byte(v.z);
                }
            }
        }
    }

    return nmap;
}
