#include <cmath>

// Estimate reference evapotranspiration (ET0) using temp and altitude
// Based on a simplified FAO approach (not full Penman-Monteith)
double calculateET0(double temperature, double altitude)
	{
	// Constants
	const double solarRadiation = 20.0;  // MJ/m2/day (approx. constant, you can adjust per region)
	const double lambda = 2.45;		  // latent heat of vaporization (MJ/kg)
	const double gamma = 0.067;		  // psychrometric constant (kPa/°C)
	
	// Atmospheric pressure from altitude (in kPa)
	double pressure = 101.3 * pow((293.0 - 0.0065 * altitude) / 293.0, 5.26);

	// Saturation vapor pressure (es) from temperature
	double es = 0.6108 * exp((17.27 * temperature) / (temperature + 237.3));  // in kPa

	// Slope of vapor pressure curve (delta)
	double delta = (4098 * es) / pow((temperature + 237.3), 2);  // in kPa/°C

	// Estimate ET0 (mm/day)
	double ET0 = (0.408 * delta * solarRadiation) / (delta + gamma * (1 + 0.34));
	
	return ET0;  // in mm/day
	}
