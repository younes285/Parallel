#pragma once

#include <cstddef>
#include <vector>

class Grid {
public:
    explicit Grid(int N);

    int intervals() const { return N_; }
    double spacing() const { return 1.0 / N_; }
    std::size_t size() const { return temperatures_.size(); }

    double* data() { return temperatures_.data(); }
    const double* data() const { return temperatures_.data(); }

    double& operator()(int i, int j);
    const double& operator()(int i, int j) const;
    void fill(double temperature);

private:
    std::size_t index(int i, int j) const;

    int N_;
    std::vector<double> temperatures_;
};
