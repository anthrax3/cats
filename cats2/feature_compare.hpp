#ifndef HAVE_FEATURE_COMPARE_HPP
#define HAVE_FEATURE_COMPARE_HPP

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>

#include "signal.hpp"
#include "complex_vector.hpp"
#include "mfcc.hpp"
#include "db.hpp"

typedef enum { 
	OPERAND_FEATURES = 0,	OPERAND_DIFFERENTIAL_FEATURES,	OPERAND_ACCELERATION_FEATURES, 
	OPERAND_ENERGY,			OPERAND_DIFFERENTIAL_ENERGY,	OPERAND_ACCELERATION_ENERGY 
} feature_operand_t;

struct signal_mse_t
{
	fp_vector_t< double >	features;
	fp_vector_t< double >	differential;
	fp_vector_t< double >	acceleration;

	signal_mse_t(void) { return; }
	signal_mse_t(fp_vector_t< double >& f, fp_vector_t< double >& d, fp_vector_t< double >& a) : 
		features(f), differential(d), acceleration(a)
	{
		return;
	}

	~signal_mse_t(void)
	{
		features.clear();
		differential.clear();
		acceleration.clear();
		return;
	}
};

typedef signal_mse_t signal_variance_t;
typedef signal_variance_t signal_standard_deviation_t;

struct signal_variance_all_t
{
	double features;
	double differential;
	double acceleration;

	signal_variance_all_t(void) { return; }
	signal_variance_all_t(double f, double d, double a) : features(f), differential(d), acceleration(a)
	{
		return;
	}

	~signal_variance_all_t(void)
	{
		features = 0.0;
		differential = 0.0;
		acceleration = 0.0;
		return;
	}
};

typedef signal_variance_all_t signal_standard_deviation_all_t;

class feature_compare_t
{
	private:
	protected:
		static fp_vector_t< double > fmse(features_vector_t< double >&, std::vector< features_t< double > >&);
		static fp_vector_t< double > dmse(features_vector_t< double >&, std::vector< features_t< double > >&);
		static fp_vector_t< double > amse(features_vector_t< double >&, std::vector< features_t< double > >&);

		static fp_vector_t< double > fvar(features_vector_t< double >&);
		static fp_vector_t< double > dvar(features_vector_t< double >&);
		static fp_vector_t< double > avar(features_vector_t< double >&);

		static double fvar_all(features_vector_t< double >&);
		static double dvar_all(features_vector_t< double >&);
		static double avar_all(features_vector_t< double >&);

	public:
		feature_compare_t(void);
		virtual ~feature_compare_t(void);

		static signal_mse_t mse(features_vector_t< double >&, std::vector< signal_sample_t >&);
		static signal_variance_t variance(features_vector_t< double >&);
		static signal_variance_all_t variance_all(features_vector_t< double >&);

		static signal_standard_deviation_t std(features_vector_t< double >&);
		static signal_standard_deviation_all_t std_all(features_vector_t< double >&);
		//void rmse(void);
		//void se(void);
		//void sd(void);
		//void rmsd(void);

};

#endif
