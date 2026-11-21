#pragma once

class Grid;

struct VerificationErrors {
    double infinity;
    double l2;
};

double exact_temperature(double x, double y, double time);
VerificationErrors verification_errors(const Grid& grid, double time);
