#ifndef CFSR_NC_H
#define CFSR_NC_H

#include <time.h>
#include "latlon.h"

#ifdef  __cplusplus
extern  "C" {
#endif

typedef struct cfsr_nc_dataset_t cfsr_nc_dataset_t;

int cfsr_nc_load(cfsr_nc_dataset_t* dataset, struct tm date);
char* cfsr_nc_filename(cfsr_nc_dataset_t* dataset, struct tm date);
int cfsr_nc_free(cfsr_nc_dataset_t* dataset);
double cfsr_nc_bilinear(cfsr_nc_dataset_t* dataset, struct tm date, latlon_t loc);

extern cfsr_nc_dataset_t *CFSR_NC_OCNU5, *CFSR_NC_OCNV5;

#ifdef  __cplusplus
}
#endif

#endif