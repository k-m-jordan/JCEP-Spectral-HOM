#include "tpx3.h"

#include <QVector>

#include <dlib/optimization.h>

using namespace spec_hom;

LinePair::LinePair(bool vertical, double line_1_pos, double line_2_pos, double line_1_sigma, double line_2_sigma) :
    mIsVertical(vertical),
    mLine1Pos(line_1_pos),
    mLine2Pos(line_2_pos),
    mLine1Sigma(line_1_sigma),
    mLine2Sigma(line_2_sigma) {

    // Do nothing

}

using input_vec = dlib::matrix<double, 1, 1>; // definitions used in the fitting code
using param_vec = dlib::matrix<double, 6, 1>; // definitions used in the fitting code

// Functors needed for fitting
double model(const input_vec &in, const param_vec &pm) {
    return pm(0) * std::exp(-0.5 * (in(0) - pm(1)) * (in(0) - pm(1)) / pm(2) / pm(2)) +
           pm(3) * std::exp(-0.5 * (in(0) - pm(4)) * (in(0) - pm(4)) / pm(5) / pm(5));
}
double residual(const std::pair<input_vec, double> &data, const param_vec &params) {
    return model(data.first, params) - data.second;
}

param_vec fit_data(QVector<double> &x, QVector<double> &y, bool h_lines) {

    unsigned max_ix, max_jx;
    if (h_lines) {
        max_ix = Tpx3Image::HEIGHT;
        max_jx = Tpx3Image::WIDTH;
    } else {
        max_ix = Tpx3Image::WIDTH;
        max_jx = Tpx3Image::HEIGHT;
    }

    std::vector<std::pair<input_vec, double>> data_samples;
    for (auto ix = 0; ix < x.size(); ++ix) {
        data_samples.emplace_back(input_vec{x[ix]}, y[ix]);
    }

    // Before we perform the fit, we need to find an initial guess for the parameters
    // Sort the points from highest y to lowest y, and traverse list to find peaks

    std::vector<std::size_t> indices(max_ix);
    for(auto ix = 0; ix < max_ix; ++ix)
        indices.push_back(ix);

    std::sort(indices.begin(), indices.end(), [y](auto lhs, auto rhs){
        return y[lhs] > y[rhs];
    });

    std::vector<bool> processed;
    processed.insert(processed.begin(), false, max_ix);

    auto fit_m1 = static_cast<double>(indices[0]);
    auto fit_left_1 = fit_m1;
    auto fit_right_1 = fit_m1;

    double fit_m2 = -1;

    for(auto ix = 1; ix < max_ix; ++ix) {
        auto next_highest = indices[ix];
        if (next_highest == fit_left_1 - 1) {
            --fit_left_1;
        } else if (next_highest == fit_right_1 + 1) {
            ++fit_right_1;
        } else {
            fit_m2 = indices[ix];
            break;
        }
    }

    if(fit_m2 == -1)
        fit_m2 = max_ix / 2.0;

    // Do the fit
    auto max_y = std::max_element(y.cbegin(), y.cend());
    param_vec soln{ *max_y, fit_m1, 3, *max_y, fit_m2, 3 };

    dlib::solve_least_squares_lm(dlib::objective_delta_stop_strategy(1e-3),
                                 residual,
                                 dlib::derivative(residual), // use numerical derivatives; a bit slow, but much simpler
                                 data_samples,
                                 soln);

    return soln;

}

LinePair LinePair::find(ImageXY<unsigned int> &image, bool h_lines, QVector<double> *x_out, QVector<double> *y_out, QVector<double> *fit_y_out) {

    unsigned max_ix, max_jx;
    if (h_lines) {
        max_ix = Tpx3Image::HEIGHT;
        max_jx = Tpx3Image::WIDTH;
    } else {
        max_ix = Tpx3Image::WIDTH;
        max_jx = Tpx3Image::HEIGHT;
    }

    std::vector<unsigned> slice;
    slice.reserve(max_ix);

    for (unsigned ix = 0; ix < max_ix; ++ix) {
        unsigned slice_tot = 0;

        for (unsigned jx = 0; jx < max_jx; ++jx) {
            unsigned x_ix, y_ix;
            if (h_lines) {
                x_ix = jx;
                y_ix = ix;
            } else {
                x_ix = ix;
                y_ix = jx;
            }

            slice_tot += image[x_ix][y_ix];
        }

        slice.push_back(slice_tot);
    }

    QVector<double> x, y;
    x.reserve(max_ix);
    y.reserve(max_ix);

    for (auto ix = 0; ix < max_ix; ++ix) {
        x.push_back(ix);
        y.push_back(slice[ix]);
    }

    // We need to fit the data
    param_vec soln = fit_data(x, y, h_lines);

    decltype(y) fit_y(y.size());
    for (auto ix = 0; ix < fit_y.size(); ++ix) {
        fit_y[ix] = model({x[ix]}, soln);
    }

    if(x_out)
        *x_out = x;
    if(y_out)
        *y_out = y;
    if(fit_y_out)
        *fit_y_out = fit_y;

    auto m1 = soln(1) * PIXEL_SIZE;
    auto s1 = soln(2) * PIXEL_SIZE;
    auto m2 = soln(4) * PIXEL_SIZE;
    auto s2 = soln(5) * PIXEL_SIZE;

    return {
        h_lines,
        m1,
        m2,
        s1,
        s2
    };

}

void LinePair::getRectBounds(double &min1, double &max1, double &min2, double &max2, double num_sigma) {

    auto m1 = mLine1Pos;
    auto s1 = mLine1Sigma;
    auto m2 = mLine2Pos;
    auto s2 = mLine2Sigma;

    min1 = std::min(m1 - num_sigma * s1, m2 - num_sigma * s2);
    max1 = std::min(m1 + num_sigma * s1, m2 + num_sigma * s2);
    min2 = std::max(m1 - num_sigma * s1, m2 - num_sigma * s2);
    max2 = std::max(m1 + num_sigma * s1, m2 + num_sigma * s2);

}

int LinePair::closestLine(double x, double y) {

    if(mIsVertical) {
        double bottom = std::min(mLine1Pos, mLine2Pos);
        double top = std::max(mLine1Pos, mLine2Pos);
        double mid = (bottom + top) / 2.0;
        if (y <= mid)
            return 1;
        else
            return 2;
    } else {
        double left = std::min(mLine1Pos, mLine2Pos);
        double right = std::max(mLine1Pos, mLine2Pos);
        double mid = (left + right) / 2.0;
        if (x <= mid)
            return 1;
        else
            return 2;
    }

}