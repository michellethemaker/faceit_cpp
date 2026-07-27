#include "CommonMath.h"
#include <math.h>

double CommonMath::EuclDist(float x1, float x2, float y1, float y2) {
	return (sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2)));
}