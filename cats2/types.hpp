#ifndef TYPES_HPP
#define TYPES_HPP

// -- shamelessly ripped from http://www.ms.mff.cuni.cz/~krajj7am/big/spectrogram-doc/types_8hpp_source.html
// -- thanks to whomever for whatever this unnamed package is.

#include <complex>
#include <vector>

#define PI 3.1415926535897932384626433832795

typedef std::complex<float> Complex;
typedef std::vector<float> real_vec;
typedef std::vector<Complex> complex_vec;

#endif