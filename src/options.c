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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <popt.h>

#include "options.h"

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "normalmap"
#endif 

static struct poptOption popt_options[] =
{
    {   "input", 'i', POPT_ARG_STRING,
        NULL, 0,
        "Input file, or '-' for stdin", "FILENAME"
    },
    {   "output", 'o', POPT_ARG_STRING,
        NULL, 0,
        "Output file, or '-' for stdout", "FILENAME"
    },
    {   "xyz", 'x', POPT_ARG_STRING,
        NULL, 0,
        "Mapping of XYZ to output colour channels; eg 'rgb', 'agb'", "RGB"
    },
    {   "normalise", 'n', POPT_ARG_NONE,
        NULL, 0,
        "Scale input heightmaps values to fill range 0.0-1.0", NULL
    },
    {   "scale", 's', POPT_ARG_DOUBLE,
        NULL, 1,
        "Scale of heightmap (implies --normalise) relative to a pixel; "
        "defaults to 1.0", NULL
    },
    {   "unsigned", 'u', POPT_ARG_NONE,
        NULL, 0,
        "Z values in output are unsigned char (0-255) "
        "instead of signed (128-255)", NULL
    },
    {   "wrap", 'w', POPT_ARG_NONE,
        NULL, 0,
        "Texture wraps around for tiling", NULL
    },
    {   "tm", 't', POPT_ARG_NONE,
        NULL, 0,
        "Use Trackmania format (--xyz=agb)",
        NULL
    },
    {   "tilesize", 'z', POPT_ARG_INT, 
        NULL, 0, 
        "Set the tile size for normal map generation (default is 0 for no tiles)", "TILESIZE"
    },
    POPT_AUTOHELP
    POPT_TABLEEND
};

static int validate_xyz(char *xyz)
{
    int m, n;

    if (strlen(xyz) != 3)
        return 0;

    for (n = 0; n < 3; ++n)
    {
        xyz[n] = tolower(xyz[n]);
    }

    if (strspn(xyz, "rgba") != 3)
        return 0;

    for (n = 0; n < 3; ++n)
    {
        for (m = 0; m < 3; ++m)
        {
            if (n != m && xyz[n] == xyz[m])
                return 0;
        }
    }

    return 1;
}

NormalmapOptions *normalmap_options_get(int argc, char **argv)
{
    poptContext pc;
    NormalmapOptions *no = malloc(sizeof(NormalmapOptions));
    memset(no, 0, sizeof(NormalmapOptions));
    char const **alias_argv = malloc(2 * sizeof(char const **));
    struct poptAlias alias_data =
    {
        "tm",
        't',
        1,
        alias_argv
    };
    int result;

    no->input = NULL;
    no->output = NULL;
    no->xyz = NULL;
    no->scale = 1.0;
    no->unsigned_z = 0;
    no->normalise = 0;
    no->tilesize = 0;

    popt_options[0].arg = &no->input;
    popt_options[1].arg = &no->output;
    popt_options[2].arg = &no->xyz;
    popt_options[3].arg = &no->normalise;
    popt_options[4].arg = &no->scale;
    popt_options[5].arg = &no->unsigned_z;
    popt_options[6].arg = &no->wrap;
    popt_options[8].arg = &no->tilesize;  // Added tile size argument handling

    pc = poptGetContext(PACKAGE_NAME, argc, (const char **) argv,
            popt_options, 0);

    alias_argv[0] = "--xyz=agb";
    alias_argv[1] = NULL;
    poptAddAlias(pc, alias_data, 0);

    while ((result = poptGetNextOpt(pc)) >= 0)
    {
        if (result == 1)
            no->normalise = 1;
    }
    if (result < -1)
    {
        fprintf(stderr, "%s: %s\n", poptBadOption(pc, 0), poptStrerror(result));
        exit(1);
    }

    if (!no->xyz)
        no->xyz = strdup("rgb");
    if (!validate_xyz(no->xyz))
    {
        fprintf(stderr, "Bad value for --xyz: %s\n", no->xyz);
        exit(1);
    }

    if (!no->input)
    {
        fprintf(stderr, "Input must be specified\n");
        poptPrintUsage(pc, stderr, 0);
        exit(1);
    }

    if (!no->output)
    {
        fprintf(stderr, "Output must be specified\n");
        poptPrintUsage(pc, stderr, 0);
        exit(1);
    }

    poptFreeContext(pc);
    return no;
}
