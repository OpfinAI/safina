#pragma once
#include "shared.h"
#include <inttypes.h>
#include <stddef.h>

typedef struct GBIXHeader
{
    int32_t nrows;
    int32_t ncols;
    FieldType *types;
    size_t gbixhb;
    uint64_t rowb;
} GBIXHeader;

typedef union GBIXCell
{
    int32_t i;
    float f;
    uint8_t raw[4];
} GBIXCell;

int gbixhead(char *p, GBIXHeader *h);
int gbixrows(char *p, GBIXHeader *h, int s, int e, uint8_t **o);
int gbixcell(char *p, int32_t r, int32_t c, GBIXCell *cell);
void printheader(GBIXHeader h);
void printcell(FieldType t, GBIXCell cell);
