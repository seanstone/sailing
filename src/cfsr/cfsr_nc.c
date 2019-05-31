#include "cfsr/cfsr_nc.h"
#include "cfsr/cfsr.h"
#include "latlon.h"
#include <netcdf.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

typedef struct cfsr_nc_dataset_t
{
    int ncid[CFSR_END_YEAR-CFSR_START_YEAR][12];
    char* str;
    long Ni;
    long Nj;
    double lon0;
    double lat0;
    double lon1;
    double lat1;
    double dx;
    double dy;
    double scale_factor;
    double add_offset;
    uint16_t missing_value;
} cfsr_nc_dataset_t;

cfsr_nc_dataset_t cfsr_nc_ocnu5 = {.str = "ocnu5"},
                  cfsr_nc_ocnv5 = {.str = "ocnv5"};
cfsr_nc_dataset_t *CFSR_NC_OCNU5 = &cfsr_nc_ocnu5;
cfsr_nc_dataset_t *CFSR_NC_OCNV5 = &cfsr_nc_ocnv5;

int* cfsr_ncid(cfsr_nc_dataset_t* dataset, struct tm date)
{
    return &dataset->ncid[1900+date.tm_year-CFSR_START_YEAR][date.tm_mon];
}

int cfsr_nc_load(cfsr_nc_dataset_t* dataset, struct tm date)
{
    int err;
    char* filename = cfsr_nc_filename("data", dataset, date);
    if ((err = nc_open(filename, NC_NOWRITE, cfsr_ncid(dataset, date)))) {
        filename = cfsr_nc_filename("/data", dataset, date);
        if ((err = nc_open(filename, NC_NOWRITE, cfsr_ncid(dataset, date)))) {
            printf("cfsr_nc_load: failed to load file %s\n", filename);
            return err;
        }
    }

    if (dataset == CFSR_NC_OCNU5 || dataset == CFSR_NC_OCNV5)
    {
        dataset->Ni = 720;
        dataset->Nj = 360;
        dataset->lat0 = 89.750000;
        dataset->lon0 = 0.250000;
        dataset->lat1 = -89.750000;
        dataset->lon1 = 359.750000;
        dataset->dy = -0.500000;
        dataset->dx = 0.500000;

        dataset->scale_factor = 5.6536401507637375E-5;
        dataset->add_offset = 0.03947173179924627;

        dataset->missing_value = -32767;
    }

    return 0;
}

char* cfsr_nc_filename(const char* root, cfsr_nc_dataset_t* dataset, struct tm date)
{
    static char filename[128];
    snprintf(filename, sizeof(filename), "%s/%s.gdas.%04u%02u.nc", root, dataset->str, 1900+date.tm_year, date.tm_mon+1);
    return filename;
}

int cfsr_nc_free(cfsr_nc_dataset_t* dataset)
{
    for (int i = 0; i < CFSR_END_YEAR-CFSR_START_YEAR; i++)
        for (int j = 0; j < 12; j++)
    nc_close(dataset->ncid[i][j]);
}

double bilinear(double* v, double di, double dj)
{
    int nan_count = 0;
    for (int i = 0; i < 4; i++)
        if (v[i] == NAN)
            nan_count++;

    switch (nan_count)
    {
        case 0: return v[0]*(1-di)*(1-dj) + v[1]*(1-di)*dj + v[2]*di*(1-dj) + v[3]*di*dj;
        case 1:
            if (v[0] == NAN) return v[1]*(1-di) + v[2]*(1-dj) + v[3]*di*dj;
            if (v[1] == NAN) return v[0]*(1-di) + v[2]*di*(1-dj) + v[3]*dj;
            if (v[2] == NAN) return v[0]*(1-dj) + v[1]*(1-di)*dj + v[3]*di;
            if (v[3] == NAN) return v[0]*(1-di-dj) + v[1]*dj + v[2]*di;
        default: return NAN;
    }
}

double cfsr_nc_bilinear(cfsr_nc_dataset_t* dataset, struct tm date, latlon_t loc)
{
    int ncid = *cfsr_ncid(dataset, date);
    if (!ncid) {
        if (cfsr_nc_load(dataset, date) < 0) return NAN;
        ncid = *cfsr_ncid(dataset, date);
    }
    double i = mod((loc.lon-dataset->lon0)/dataset->dx, dataset->Ni);
    double j = mod((loc.lat-dataset->lat0)/dataset->dy, dataset->Nj);
    
    double i0 = floor(i);
    double j0 = floor(j);
    double i1 = mod(i0+1, dataset->Ni);
    double j1 = mod(j0+1, dataset->Nj);
    double di = i - i0;
    double dj = j - j0;

    size_t dim[4][5] = {
        { date.tm_mday-1, date.tm_hour/6, date.tm_hour%6, j0, i0 },
        { date.tm_mday-1, date.tm_hour/6, date.tm_hour%6, j1, i0 },
        { date.tm_mday-1, date.tm_hour/6, date.tm_hour%6, j0, i1 },
        { date.tm_mday-1, date.tm_hour/6, date.tm_hour%6, j1, i1 },
    };
    double v[4];
    for (int p = 0; p < 4; p++) {
        int16_t s;
        nc_get_var1_short(ncid, 5, dim[p], &s);
        v[p] = (s == dataset->missing_value) ? NAN : (dataset->add_offset + s * dataset->scale_factor);
    }

    return bilinear(v, di, dj);
}

