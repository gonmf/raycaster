#include "global.h"

double fit_angle(double d) {
    while (d < 0.0) {
        d += 360.0;
    }
    while (d >= 360.0) {
        d -= 360.0;
    }
    return d;
}
