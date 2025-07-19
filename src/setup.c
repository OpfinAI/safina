#define _GNU_SOURCE
#include "shared.h"
#include <alloca.h>
#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

FieldType detect_type(const char *s, uint8_t out[4])
{
    if (s == NULL || *s == '\0')
        return TYPE_MISSING;

    char *end;

    int32_t i = strtol(s, &end, 10);
    if (*end == '\0')
    {
        memcpy(out, &i, sizeof(int32_t));
        return TYPE_INT;
    }

    float f = strtof(s, &end);
    if (*end == '\0')
    {
        memcpy(out, &f, sizeof(float));
        return TYPE_FLOAT;
    }

    return TYPE_STRING;
}

int convert_type(const char *s, FieldType type, uint8_t out[4])
{
    switch (type)
    {
    case TYPE_MISSING:
    {
        // TODO FUTURE
        return -1;
    }
    case TYPE_INT:
    {
        int32_t value;
        if (s == NULL || *s == '\0')
            value = INT32_MIN + 1;
        else if (strcmp(s, "-inf") == 0)
            value = INT32_MIN;
        else if (strcmp(s, "inf") == 0 || strcmp(s, "+inf") == 0)
            value = INT32_MAX;
        else
            value = strtol(s, NULL, 10);
        memcpy(out, &value, sizeof(int32_t));
        return 0;
    }
    case TYPE_FLOAT:
    {
        float value;
        if (s == NULL || *s == '\0')
            value = -FLT_MAX + 1;
        else if (strcmp(s, "-inf") == 0)
            value = -FLT_MAX;
        else if (strcmp(s, "inf") == 0 || strcmp(s, "+inf") == 0)
            value = FLT_MAX;
        else
            value = strtof(s, NULL);
        memcpy(out, &value, sizeof(float));
        return 0;
    }
    default:
        return -1;
    }
}

int csvtogbix(FILE *csv, FILE *bin)
{
    char delim = ',';
    char *line = NULL;
    size_t len;
    size_t linecap = 0;
    int32_t ncols = 1;
    int32_t nrows = 0;
    char *start, *end, *field;
    uint8_t out[4];

    // TODO FUTURE Check for "*,*" fields
    if (getline(&line, &linecap, csv) == -1)
        return EXIT_FAILURE;
    else
    {
        size_t linelen = strlen(line);
        if (linelen > 0 && line[linelen - 1] == '\n')
            line[--linelen] = '\0';
        if (linelen > 0 && line[linelen - 1] == '\r')
            line[--linelen] = '\0';
        for (char *p = line; *p; p++)
        {
            if (*p == delim)
                ncols++;
        }
    }

    FieldType types[ncols];
    memset(types, 0, ncols * sizeof(FieldType));

    fwrite(&nrows, sizeof(int32_t), 1, bin);
    fwrite(&ncols, sizeof(int32_t), 1, bin);
    fwrite(types, sizeof(FieldType), ncols, bin);

    int col = 0;
    start = line;

    while ((end = strchr(start, delim)))
    {
        // TODO convert to function
        len = end - start;
        field = alloca(len + 1);
        memcpy(field, start, len);
        field[len] = '\0';
        types[col] = detect_type(field, out); // TODO FUTURE type detection can account for more than just row 0
        if (types[col] == TYPE_STRING)
            return -1; // TODO FUTURE
        fwrite(out, sizeof(out), 1, bin);
        start = end + 1;
        col++;
    }

    len = strlen(start);
    field = alloca(len + 1);
    memcpy(field, start, len);
    field[len] = '\0';
    types[col] = detect_type(field, out); // TODO FUTURE type detection can account for more than just row 0
    if (types[col] == TYPE_STRING)
        return -1; // TODO FUTURE
    fwrite(out, sizeof(out), 1, bin);
    col++;
    nrows++;

    fseek(bin, sizeof(nrows) + sizeof(ncols), SEEK_SET);
    fwrite(types, sizeof(FieldType), ncols, bin);
    fseek(bin, 0, SEEK_END);

    while (getline(&line, &linecap, csv) != -1)
    {
        size_t linelen = strlen(line);
        if (linelen > 0 && line[linelen - 1] == '\n')
            line[--linelen] = '\0';
        if (linelen > 0 && line[linelen - 1] == '\r')
            line[--linelen] = '\0';
        int col = 0;
        start = line;

        while ((end = strchr(start, delim)))
        {
            // TODO convert to function
            len = end - start;
            field = alloca(len + 1);
            memcpy(field, start, len);
            field[len] = '\0';
            if (types[col] == TYPE_STRING)
                return -1; // TODO FUTURE
            else if (convert_type(field, types[col], out) < 0)
                return -1;
            fwrite(out, sizeof(out), 1, bin);
            start = end + 1;
            col++;
        }

        len = strlen(start);
        field = alloca(len + 1);
        memcpy(field, start, len);
        field[len] = '\0';
        if (types[col] == TYPE_STRING)
            return -1; // TODO FUTURE
        else if (convert_type(field, types[col], out) < 0)
            return -1;
        fwrite(out, sizeof(out), 1, bin);
        col++;
        nrows++;
    }

    fseek(bin, 0, SEEK_SET);
    fwrite(&nrows, sizeof(int32_t), 1, bin);

    free(line);
    fclose(csv);
    fclose(bin);

    return EXIT_SUCCESS;
}

int main()
{
    FILE *csv = fopen("/home/greedy/safina/data/kisauni_weather.csv", "r");
    if (!csv)
        return -1;

    FILE *bin = fopen("/home/greedy/safina/data/kisauni_weather.gbix", "w+b");
    if (!bin)
        return -1;

    csvtogbix(csv, bin);
}
