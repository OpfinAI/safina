#include "main.h"
#include "gbix.h"
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int gbixtoenv(uint8_t *rows, GBIXHeader *h, size_t rowcount, Environment *o)
{
	size_t bytes_ints = 5 * rowcount * sizeof(int32_t);
	size_t bytes_floats = 7 * rowcount * sizeof(float);
	void *block = malloc(bytes_ints + bytes_floats);

	int32_t *year = (int32_t *)block;
	int32_t *month = year + rowcount;
	int32_t *day = month + rowcount;
	int32_t *hour = day + rowcount;
	int32_t *wind_direction = hour + rowcount;

	float *temperature = (float *)(wind_direction + rowcount);
	float *dew_point = temperature + rowcount;
	float *relative_hum = dew_point + rowcount;
	float *precipitation = relative_hum + rowcount;
	float *wind_speed = precipitation + rowcount;
	float *pressure = wind_speed + rowcount;

	for (size_t r = 0; r < rowcount; ++r)
	{
		const uint8_t *cell_ptr = rows + r * h->rowb;

		for (int c = 0; c < h->ncols; ++c)
		{
			switch (h->types[c])
			{
			case TYPE_INT:
			{
				int32_t v = *(const int32_t *)cell_ptr;
				cell_ptr += sizeof(int32_t);

				switch (c)
				{
				case 0:
					year[r] = v;
					break;
				case 1:
					month[r] = v;
					break;
				case 2:
					day[r] = v;
					break;
				case 3:
					hour[r] = v;
					break;
				case 8:
					wind_direction[r] = v;
					break;
				default:
					break;
				}
				break;
			}
			case TYPE_FLOAT:
			{
				float vf = *(const float *)cell_ptr;
				cell_ptr += sizeof(float);

				switch (c)
				{
				case 4:
					temperature[r] = vf;
					break;
				case 5:
					dew_point[r] = vf;
					break;
				case 6:
					relative_hum[r] = vf;
					break;
				case 7:
					precipitation[r] = vf;
					break;
				case 9:
					wind_speed[r] = vf;
					break;
				case 10:
					pressure[r] = vf;
					break;
				default:
					break;
				}
				break;
			}
			default:
				break;
			}
		}
	}

	Environment env = {
		.data = block,
		.year = year,
		.month = month,
		.day = day,
		.hour = hour,
		.temp = temperature,
		.dew_point = dew_point,
		.relative_humidity = relative_hum,
		.precipitation = precipitation,
		.wind_direction = wind_direction,
		.wind_speed = wind_speed,
		.pressure = pressure,
		.altitude = 30,
		.latitude = -6.116130,
		.longitude = 39.311333,
		.albedo = 0.35,
	};

	*o = env;

	return 0;
}

int netradiation(Environment *env, int32_t nrows)
{
	if (env->temp == NULL || nrows <= 0)
		return -1;

	// Step 1: Compute T_mean from all temperature values
	float T_sum = 0.0f;
	for (int i = 0; i < nrows; ++i)
	{
		T_sum += env->temp[i];
	}
	float T_mean = T_sum / nrows;
	float T_k = T_mean + 273.16f;

	// Step 2: Geographic and radiation setup
	float altitude = env->altitude;
	float latitude = env->latitude;
	float phi = latitude * PI / 180.0f;
	int J = 100; // Approximate day of year for typical conditions

	// Step 3: Solar geometry
	float delta = 0.409f * sinf((2.0f * PI * J / 365.0f) - 1.39f);
	float dr = 1.0f + 0.033f * cosf(2.0f * PI * J / 365.0f);
	float omega_s = acosf(-tanf(phi) * tanf(delta));

	// Step 4: Extraterrestrial radiation (Ra)
	float Ra = (24.0f * 60.0f / PI) * GSC * dr *
			   (omega_s * sinf(phi) * sinf(delta) + cosf(phi) * cosf(delta) * sinf(omega_s));

	// Step 5: Surface solar radiation and net shortwave radiation
	float Rs = 0.75f * Ra;				   // Assumes 75% of Ra reaches surface
	float Rns = (1.0f - env->albedo) * Rs; // Net shortwave radiation

	// Step 6: Net longwave radiation (Rnl)
	float Rso = (0.75f + 2e-5f * altitude) * Ra;
	float ea = 2.5f;		 // Estimated vapor pressure (kPa)
	float sigma = 4.903e-9f; // Stefan–Boltzmann constant [MJ K^-4 m^-2 day^-1]

	float Rnl = sigma * (powf(T_k, 4.0f) + powf(T_k, 4.0f)) / 2.0f *
				(0.34f - 0.14f * sqrtf(ea)) * (1.35f * (Rs / Rso) - 0.35f);

	// Step 7: Net radiation
	float Rn = Rns - Rnl;
	env->tmean = T_mean;
	env->netrad = Rn;
	return 0;
}

void calculate_ET0(Environment *env, int32_t nrows)
{
	float U_sum = 0.0f;
	float P_sum = 0.0f;
	float RH_sum = 0.0f;

	for (int32_t i = 0; i < nrows; ++i)
	{
		U_sum += env->wind_speed[i] * (1.0f / 3.6f); // km/h → m/s
		P_sum += env->pressure[i];					 // hPa (same as mbar)
		RH_sum += env->relative_humidity[i];
	}

	float Rn = env->netrad;
	float T = env->tmean;
	float u2 = U_sum / nrows;		 // wind speed at 2m height [m/s]
	float P = P_sum / nrows / 10.0f; // Convert hPa → kPa, atmospheric pressure [kPa]
	float RH = RH_sum / nrows;		 // mean relative humidity [%]

	// Step 1: Calculate saturation vapor pressure (es)
	float es = 0.6108 * exp((17.27 * T) / (T + 237.3));

	// Step 2: Actual vapor pressure (ea) from RH
	float ea = es * (RH / 100.0f);

	// Step 3: Slope of vapor pressure curve (delta)
	float delta = 4098 * es / powf((T + 237.3f), 2);

	// Step 4: Psychrometric constant (gamma)
	float gamma = CP * P / (EPSILON * 2.45); // 2.45 MJ/kg is latent heat of vaporization

	// Step 5: Calculate ET0 using FAO-56 Penman–Monteith equation
	float numerator = (0.408f * delta * Rn) +
					  (gamma * (900.0f / (T + 273.0f)) * u2 * (es - ea));
	float denominator = delta + gamma * (1.0f + 0.34f * u2);

	float ET0 = numerator / denominator;

	env->et0 = ET0; // mm/day
}

float calculate_ETc_drip(Environment env, WaterSource wsrc, Crop crop)
{
	if (wsrc.drip_irrigation)
		return env.et0 * crop.Kc * crop.wetfrac;
	else
		return env.et0 * crop.Kc;
}

int main()
{
	const float ha = 0.2;
	const CashCrop cashcrop = {
		.crop = {
			.n = 800,
			.height = 1.8,
			.Kc = 1.2,
			.wetfrac = 0.02,
		},
		.tgrow = 90,
	};
	const Moringa moringa = {
		.crop = {
			.n = 1600,		 // Safina internal
			.height = 10,	 // https://research.fs.usda.gov/treesearch/30357
			.Kc = 1.0,		 // Safina internal
			.wetfrac = 0.08, // Safina internal
		},
	};
	const Corn corn = {
		// small ear of sweetcorn, worst case outcomes
		.crop = {
			.n = 18750,		 // Safina internal
			.height = 1.82,	 // https://gardeningsolutions.ifas.ufl.edu/plants/edibles/vegetables/corn/
			.Kc = 1.25,		 // Safina internal
			.wetfrac = 0.04, // Safina internal
		},
		.tgrow = 90,   // https://southafrica.co.za/sweet-corn-planting.html , you're supposed to calculate it though https://ndawn.ndsu.nodak.edu/help-corn-growing-degree-days.html https://kingsagriseeds.com/wp-content/uploads/2014/12/Growing-degree-days-and-growth-requirements-for-corn.pdf
		.massear = 73, // https://weighschool.com/sweet-corn-weights-servings/
		.earcal = 63,  // https://www.nutritionix.com/i/usda/corn-1-ear-small-5-0.5-to-6-1-2-long/513fceb575b8dbbc2100151f
	};
	const WaterSource cstp = {
		.drip_irrigation = 1,
	};

	// 90-day period, starting 2025 01 02 00, ending 2025 03 31 23
	GBIXHeader h;
	uint8_t *rows;
	int s = 14;
	int e = 2174;

	if (gbixrows("/home/greedy/safina/data/kisauni_weather.gbix", &h, s, e, &rows) < 0)
		return EXIT_FAILURE;

	Environment env;
	if (gbixtoenv(rows, &h, e - s, &env) != 0)
		return EXIT_FAILURE;

	if (netradiation(&env, e - s) < 0)
		return EXIT_FAILURE;
	printf("Net Radiation: %f MJ/m²/day\n", env.netrad);

	calculate_ET0(&env, e - s);
	printf("ET_0: %f\n", env.et0);

	float cash_crop_etc = calculate_ETc_drip(env, cstp, cashcrop.crop);
	float moringa_etc = calculate_ETc_drip(env, cstp, moringa.crop);
	float corn_etc = calculate_ETc_drip(env, cstp, corn.crop);

	printf("Cash Crop ET_c: %f\n", cash_crop_etc);
	printf("Moringa ET_c: %f\n", moringa_etc);
	printf("Corn ET_c: %f\n", corn_etc);

	free(h.types);
	free(rows);
	free(env.data);

	int etc_to_L_per_ha = 10000;
	float water_per_ha = (cash_crop_etc * etc_to_L_per_ha + moringa_etc * etc_to_L_per_ha + corn_etc * etc_to_L_per_ha);

	printf("water L / Ha / day: %f\n", water_per_ha);
	printf("prepilot water L / day: %f\n", ha * water_per_ha);

	return EXIT_SUCCESS;
}
