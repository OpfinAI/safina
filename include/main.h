#pragma once

typedef struct Crop
{
    const int n;         // plants / ha
    const float height;  // meters
    const float Kc;      // crop coefficient
    const float wetfrac; // frac wetted area
} Crop;

typedef struct CashCrop
{
    Crop crop;
    const int tgrow; // days
} CashCrop;

typedef struct Moringa
{
    Crop crop;
} Moringa;

typedef struct Corn
{
    Crop crop;
    const int tgrow;   // days
    const int massear; // grams
    const int earcal;  // calories
} Corn;

typedef struct Environment
{
    // Base pointer for the contiguous memory block holding all time‑series arrays;
    // call free(env.data) to release the entire allocation.
    void *data;

    // each element of an array is an hour
    int *year;                // years
    int *month;               // months
    int *day;                 // days
    int *hour;                // hours
    float *temp;              // celsius
    float *dew_point;         // celsius
    float *relative_humidity; // %
    float *precipitation;     // mm
    int *wind_direction;      // degrees
    float *wind_speed;        // km/h
    float *pressure;          // hPa

    // assuming invariant
    float altitude;  // meters
    float latitude;  // degrees
    float longitude; // degrees
    float albedo;    // frac reflection

    // calculated
    float netrad; // MJ/m²/day
    float tmean;  // mean daily air temperature at 2m [°C]
    float et0;    // mm/day
} Environment;

typedef struct WaterSource
{
    const int drip_irrigation; // bool
} WaterSource;

#define PI 3.141592653589793
#define GSC 0.0820    // Solar constant [MJ m^-2 min^-1]
#define CP 1.013e-3   // Specific heat of air [MJ/kg°C]
#define EPSILON 0.622 // Ratio of molecular weight of water vapor/dry air
