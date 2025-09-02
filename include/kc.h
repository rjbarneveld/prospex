double calculateKcFromGDD(double gdd_cumulative)
	{
	// Stage thresholds in °C-days
	const double GDD1 = 200;
	const double GDD2 = 600;
	const double GDD3 = 1000;
	const double GDD4 = 1400;
	//values from https://pestweb.montana.edu/Owbm/Home/Gdd/
	const double Kc_ini = 0.35;
	const double Kc_mid = 1.15;
	const double Kc_end = 0.4;

	double kc=0;

	if (gdd_cumulative <= GDD1 && gdd_cumulative > 0) {
		kc = Kc_ini;
	}
	else if (gdd_cumulative <= GDD2) {
		double f = (gdd_cumulative - GDD1) / (GDD2 - GDD1);
		kc = Kc_ini + f * (Kc_mid - Kc_ini);
	}
	else if (gdd_cumulative <= GDD3) {
		kc = Kc_mid;
	}
	else if (gdd_cumulative <= GDD4) {
		double f = (gdd_cumulative - GDD3) / (GDD4 - GDD3);
		kc = Kc_mid - f * (Kc_mid - Kc_end);
	}
	else if (gdd_cumulative > 0) {
		kc = Kc_end;
	}
	return kc;
	}
