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

int fit_angle_int(int d) {
    while (d < 0) {
        d += 360;
    }
    while (d >= 360) {
        d -= 360;
    }
    return d;
}

double distance(double a_x, double a_y, double b_x, double b_y) {
    double diff_x = a_x - b_x;
    double diff_y = a_y - b_y;
    return sqrt(diff_x * diff_x + diff_y * diff_y);
}
