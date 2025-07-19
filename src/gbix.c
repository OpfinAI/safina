#include "gbix.h"
#include "shared.h"
#include <stdio.h>
#include <stdlib.h>

int _gbixhead(FILE *fp, GBIXHeader *h)
{
    fread(&h->nrows, sizeof(int32_t), 1, fp);
    fread(&h->ncols, sizeof(int32_t), 1, fp);

    h->types = malloc(sizeof(FieldType) * h->ncols);
    fread(h->types, sizeof(FieldType), h->ncols, fp);

    uint64_t b = 0;
    for (int32_t i = 0; i < h->ncols; i++)
    {
        FieldType t = h->types[i];
        switch (t)
        {
        case TYPE_INT:
            b += sizeof(int32_t);
            break;

        case TYPE_FLOAT:
            b += sizeof(float);
            break;

        case TYPE_STRING:
            return -1; // TODO FUTURE

        default:
            return -1;
        }
    }

    h->gbixhb = sizeof(h->nrows) + sizeof(h->ncols) + h->ncols * sizeof(FieldType);
    h->rowb = b;

    return 0;
}

int gbixhead(char *p, GBIXHeader *h)
{
    FILE *fp = fopen(p, "rb");
    if (!fp)
        return -1;

    _gbixhead(fp, h);

    fclose(fp);
    return 0;
}

int gbixrows(char *p, GBIXHeader *h, int s, int e, uint8_t **o)
{
    FILE *fp = fopen(p, "rb");
    if (!fp)
        return -1;

    _gbixhead(fp, h);

    *o = (uint8_t *)malloc((e - s) * (h->rowb) * sizeof(uint8_t));

    const long offset = (long)h->gbixhb + (s * h->rowb);

    fseek(fp, offset, SEEK_SET);
    fread(*o, h->rowb, e - s, fp);

    fclose(fp);
    return 0;
}

int gbixcell(char *p, int32_t r, int32_t c, GBIXCell *cell)
{
    FILE *fp = fopen(p, "rb");
    if (!fp)
        return -1;

    GBIXHeader h;

    _gbixhead(fp, &h);

    const size_t byte_offset = h.gbixhb + (size_t)r * (h.rowb) + (size_t)c * sizeof(int32_t);

    fseek(fp, (long)byte_offset, SEEK_SET);
    fread(cell->raw, sizeof(cell->raw), 1, fp);

    free(h.types);
    fclose(fp);
    return 0;
}

void printheader(GBIXHeader h)
{
    printf("%d, %d, %ld, %ld\n", h.ncols, h.nrows, h.gbixhb, h.rowb);
}

void printcell(FieldType t, GBIXCell cell)
{
    switch (t)
    {
    case TYPE_INT:
        printf("cell (int) = %d\n", cell.i);
        break;
    case TYPE_FLOAT:
        printf("cell (float) = %f\n", cell.f);
        break;
    default:
        printf("cell = (unsupported type)\n");
        break;
    }
}
