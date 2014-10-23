#include "feature_compare.hpp"

feature_compare_t::feature_compare_t(void)
{
	return;
}

/*feature_compare_t::feature_compare_t(challenge_t* c, signal_samples_vector_t* s) //: m_challenge(c), m_samples(s)
{
	std::lock_guard< std::mutex > l(m_mutex);

	if ( nullptr == s )
		throw std::invalid_argument("Invalid (nullptr) parameter passed to feature_compare_t constructor.");

	return;
}*/

feature_compare_t::~feature_compare_t(void)
{
	return;
}


/*void
feature_compare_t::setVectors(challenge_t* c, signal_samples_vector_t* s)
{
	std::lock_guard< std::mutex > l(m_mutex);

	if ( nullptr == s )
		throw std::invalid_argument("Invalid (nullptr) parameter passed to feature_compare_t constructor.");

	if ( nullptr != m_samples ) {
		if ( s != m_samples ) {
			for ( std::size_t idx = 0; idx < m_samples->size(); idx++ ) {
				delete m_samples->at(idx);
				m_samples->at(idx) = nullptr;
			}

			m_samples->clear();
			delete m_samples;
		}
	}

	if ( nullptr != m_challenge ) {

		if ( c != m_challenge ) {
			m_challenge->sigvec.resize(0);

			for ( std::size_t idx = 0; idx < m_challenge->features.size(); idx++ ) {
				for ( std::size_t fidx = 0; fidx < m_challenge->features[ idx ].size(); fidx++ ) {
					delete m_challenge->features[ idx ][ fidx ];
					m_challenge->features[ idx ][ fidx ] = nullptr;
					m_challenge->features[ idx ].clear();
				}
			}

			m_challenge->features.clear();
			delete m_challenge;
		}
	}

	m_samples	= s;
	m_challenge = c;
	return;
}*/

/*fp_vector_t< double >
feature_compare_t::fmse(std::vector< features_t< double > * >& c, std::vector< features_t< double > * >& s) const
{
	fp_vector_t< double > errs;

	if ( 0 == c.size() || 0 == s.size())
		throw std::runtime_error("mse() called while object is in an invalid state");

	errs.resize(c.size());

	for ( std::size_t idx = 0; idx < c.size(); idx++ ) {
		fp_vector_t< double > diff;

		if ( idx >= s.size() )
			break;
		
		diff = ::square(c[ idx ]->features - s[ idx ]->features);
		errs[ idx ] = diff.sum() / diff.size();
	}

	return errs;
}*/

fp_vector_t< double >
feature_compare_t::fmse(features_vector_t< double >& c, std::vector< features_t< double > >& s) 
{
	fp_vector_t< double >	errs;
	std::size_t				siz(0);

	if ( 0 == c.size() || 0 == s.size() )
		throw std::runtime_error("mse() called while object is in an invalid state");

	if ( c.size() > s.size() )
		siz = s.size();
	else
		siz = c.size();

	errs.resize(siz);

	for ( std::size_t idx = 0; idx < siz; idx++ ) {
		if ( idx >= s.size() )
			break;

		fp_vector_t< double > diff = c[idx].features - s[ idx ].features;
		diff = diff*diff;
		errs[ idx ] = diff.sum() / diff.size();
	}

	return errs;
}


/*fp_vector_t< double >
feature_compare_t::dmse(std::vector< features_t< double > * >& c, std::vector< features_t< double > * >& s) const
{
	fp_vector_t< double > errs;

	if ( 0 == c.size() || 0 == s.size() )
		throw std::runtime_error("mse() called while object is in an invalid state");

	errs.resize(c.size());

	for ( std::size_t idx = 0; idx < c.size(); idx++ ) {
		fp_vector_t< double > diff;

		if ( idx >= s.size() )
			break;

		diff = ::square(c[ idx ]->differential.features - s[ idx ]->differential.features);
		errs[ idx ] = diff.sum() / diff.size();
	}

	return errs;
}*/

fp_vector_t< double >
feature_compare_t::dmse(features_vector_t< double >& c, std::vector< features_t< double > >& s) 
{
	fp_vector_t< double >	errs;
	std::size_t				siz(0);

	if ( 0 == c.size() || 0 == s.size() )
		throw std::runtime_error("mse() called while object is in an invalid state");

	if ( c.size() > s.size() )
		siz = s.size();
	else
		siz = c.size();

	errs.resize(siz);

	for ( std::size_t idx = 0; idx < siz; idx++ ) {
		fp_vector_t< double > diff = c[idx].differential.features - s[ idx ].differential.features;
		diff = diff*diff;
		errs[ idx ] = diff.sum() / diff.size();
	}

	return errs;
}

/*fp_vector_t< double >
feature_compare_t::amse(std::vector< features_t< double > * >& c, std::vector< features_t< double > * >& s) const
{
	fp_vector_t< double > errs;

	if ( 0 == c.size() || 0 == s.size() )
		throw std::runtime_error("mse() called while object is in an invalid state");

	errs.resize(c.size());

	for ( std::size_t idx = 0; idx < c.size(); idx++ ) {
		fp_vector_t< double > diff;

		if ( idx >= s.size() )
			break;

		diff = ::square(c[ idx ]->acceleration.features - s[ idx ]->acceleration.features);
		errs[ idx ] = diff.sum() / diff.size();
	}

	return errs;
}*/

fp_vector_t< double >
feature_compare_t::amse(features_vector_t< double >& c, std::vector< features_t< double > >& s) 
{
	fp_vector_t< double >	errs;
	std::size_t				siz(0);

	if ( 0 == c.size() || 0 == s.size() )
		throw std::runtime_error("mse() called while object is in an invalid state");

	if ( c.size() > s.size() )
		siz = s.size();
	else
		siz = c.size();

	errs.resize(siz);

	for ( std::size_t idx = 0; idx < siz; idx++ ) {
		fp_vector_t< double > diff = c[idx].acceleration.features - s[ idx ].acceleration.features;
		diff = diff*diff;
		errs[ idx ] = diff.sum() / diff.size();
	}

	return errs;
}

fp_vector_t< double >
feature_compare_t::fvar(features_vector_t< double >& vec)
{
	fp_vector_t< double >					mean(vec.size());
	fp_vector_t< double >					variance(vec.size());
	std::vector< fp_vector_t< double > >	tmp_var(vec.size());

	for ( std::size_t idx = 0; idx < vec.size(); idx++ ) {
		mean[ idx ] = ::average(vec[ idx ].features);
		
	}
	
	for ( std::size_t idx = 0; idx < vec.size(); idx++ ) {
		tmp_var[ idx ] = vec[ idx ].features - mean[ idx ];
		tmp_var[ idx ] = ( tmp_var[ idx ] * tmp_var[ idx ] );
		variance[ idx ] = ::average(tmp_var[ idx ]);
	}

	return variance;
}

double
feature_compare_t::fvar_all(features_vector_t< double >& vec)
{
	double					mean(0.0);
	double					ret(0.0);
	fp_vector_t< double >	variance(vec.size());

	for ( std::size_t idx = 0; idx < vec.size(); idx++ )
		variance.concat(vec[ idx ].features);

	for ( std::size_t idx = 0; idx < variance.size(); idx++ )
		mean += variance[ idx ];

	mean /= variance.size();

	for ( std::size_t idx = 0; idx < variance.size(); idx++ ) {
		variance[ idx ] -= mean;
		variance[ idx ] = ( variance[ idx ] * variance[ idx ] );
	}

	ret = ::average< double >(variance);

	return ret;
}

fp_vector_t< double >
feature_compare_t::dvar(features_vector_t< double >& vec)
{
	fp_vector_t< double >					mean(vec.size());
	fp_vector_t< double >					variance(vec.size());
	std::vector< fp_vector_t< double > >	tmp_var(vec.size());

	for ( std::size_t idx = 0; idx < vec.size(); idx++ ) {
		mean[ idx ] = ::average(vec[ idx ].differential.features);

	}

	for ( std::size_t idx = 0; idx < vec.size(); idx++ ) {
		tmp_var[ idx ] = vec[ idx ].differential.features - mean[ idx ];
		tmp_var[ idx ] = ( tmp_var[ idx ] * tmp_var[ idx ] );
		variance[ idx ] = ::average(tmp_var[ idx ]);
	}

	return variance;
}

double
feature_compare_t::dvar_all(features_vector_t< double >& vec)
{
	double					mean(0.0);
	double					ret(0.0);
	fp_vector_t< double >	variance(vec.size());

	for ( std::size_t idx = 0; idx < vec.size(); idx++ )
		variance.concat(vec[ idx ].differential.features);

	for ( std::size_t idx = 0; idx < variance.size(); idx++ )
		mean += variance[ idx ];

	mean /= variance.size();

	for ( std::size_t idx = 0; idx < variance.size(); idx++ ) {
		variance[ idx ] -= mean;
		variance[ idx ] = ( variance[ idx ] * variance[ idx ] );
	}

	ret = ::average< double >(variance);

	return ret;
}

fp_vector_t< double >
feature_compare_t::avar(features_vector_t< double >& vec)
{
	fp_vector_t< double >					mean(vec.size());
	fp_vector_t< double >					variance(vec.size());
	std::vector< fp_vector_t< double > >	tmp_var(vec.size());

	for ( std::size_t idx = 0; idx < vec.size(); idx++ ) {
		mean[ idx ] = ::average(vec[ idx ].acceleration.features);

	}

	for ( std::size_t idx = 0; idx < vec.size(); idx++ ) {
		tmp_var[ idx ] = vec[ idx ].acceleration.features - mean[ idx ];
		tmp_var[ idx ] = ( tmp_var[ idx ] * tmp_var[ idx ] );
		variance[ idx ] = ::average(tmp_var[ idx ]);
	}

	return variance;
}

double
feature_compare_t::avar_all(features_vector_t< double >& vec)
{
	double					mean(0.0);
	double					ret(0.0);
	fp_vector_t< double >	var(vec.size());

	for ( std::size_t idx = 0; idx < vec.size(); idx++ )
		var.concat(vec[ idx ].acceleration.features);

	for ( std::size_t idx = 0; idx < var.size(); idx++ )
		mean += var[ idx ];

	mean /= var.size();

	for ( std::size_t idx = 0; idx < var.size(); idx++ ) {
		var[ idx ] -= mean;
		var[ idx ] = ( var[ idx ] * var[ idx ] );
	}

	ret = ::average< double >(var);

	return ret;
}

signal_mse_t
feature_compare_t::mse(features_vector_t< double >& c, std::vector< signal_sample_t >& s)
{
	signal_mse_t				mse;

	mse.features.clear();
	mse.differential.clear();
	mse.acceleration.clear();

	for ( std::size_t idx = 0; idx < s.size(); idx++ ) {
		mse.features.concat(fmse(c, s[ idx ].features));
		mse.differential.concat(dmse(c, s[ idx ].features));
		mse.acceleration.concat(amse(c, s[ idx ].features));
	}


	return mse;
}

signal_variance_t 
feature_compare_t::variance(features_vector_t< double >& v) 
{
	signal_variance_t var;

	var.features.concat(feature_compare_t::fvar(v));
	var.differential.concat(feature_compare_t::dvar(v));
	var.acceleration.concat(feature_compare_t::avar(v));

	return var;
}

signal_variance_all_t
feature_compare_t::variance_all(features_vector_t< double >& v)
{
	signal_variance_all_t var;

	var.features		= feature_compare_t::fvar_all(v);
	var.differential	= feature_compare_t::dvar_all(v);
	var.acceleration	= feature_compare_t::avar_all(v);

	return var;
}

signal_standard_deviation_t 
feature_compare_t::std(features_vector_t< double >& v)
{
	signal_standard_deviation_t	ret;
	signal_variance_t			var = variance(v);

	ret.features.concat(::sqrt(var.features));
	ret.differential.concat(::sqrt(var.differential));
	ret.acceleration.concat(::sqrt(var.acceleration));

	return ret;
}

signal_standard_deviation_all_t 
feature_compare_t::std_all(features_vector_t< double >& v)
{
	signal_standard_deviation_all_t	ret;
	signal_variance_all_t			var = variance_all(v);

	ret.features		= std::sqrt(var.features);
	ret.differential	= std::sqrt(var.differential);
	ret.acceleration	= std::sqrt(var.acceleration);

	return ret;
}