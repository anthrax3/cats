#ifndef HAVE_MFCC_HPP
#define HAVE_MFCC_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <numeric>
#include <array>

#include "signal.hpp"
#include "complex_vector.hpp"
#include "fft_factory.hpp"
#include "mel_filter.hpp"
#include "dct.hpp"

// lifter lifted from Author: Vladimir Zyablitskiy <https://github.com/rainlabs>

template < typename T, typename std::enable_if< std::is_floating_point< T >::value, bool >::type = true >
class lifter_t
{
	private:
		fp_vector_t< T > m_bank;

	protected:
	public:
		lifter_t(std::size_t n, std::size_t len)
		{
			m_bank.resize(len);

			for ( std::size_t idx = 0; idx < len; idx++ )
				m_bank[ idx ] = ( 1.0 + 0.5 * static_cast<T>(n)* std::sin(static_cast<T>( ( idx + 1 ) ) * M_PI / static_cast<T>( n )) );

			return;
		}

		virtual ~lifter_t(void) { m_bank.clear(); return; }

		fp_vector_t< T > 
		apply(fp_vector_t< T >& v)
		{
			for ( std::size_t idx = 0; idx < v.size(); idx++ )
				v[ idx ] = v[ idx ] * m_bank[ idx ];

			return v;
		}

};

template < typename T, typename std::enable_if< std::is_floating_point< T >::value, bool >::type = true >
struct features_base_t {
	fp_vector_t< T >	features;
	//std::array< T, 13 > features;
	T					energy;

	features_base_t(void) : features(12), energy(T(0.0)) { return; }
};

template < typename T, typename std::enable_if< std::is_floating_point< T >::value, bool >::type = true >
struct features_mse_t {
	T feature_sum;
	T differential_sum;
	T acceleration_sum;
	T energy;
	T differential_energy;
	T acceleration_energy;
};

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool >::type = true >
struct features_t
{
	fp_vector_t< T >		features;
	//std::array< T, 13 >		features;
	T						energy;
	features_base_t< T >	differential;
	features_base_t< T >	acceleration;

	features_t(void) : features(12), energy(T(0.0)) { return; }
};


template < typename T, typename std::enable_if< std::is_floating_point< T >::value, bool >::type = true > using features_vector_t = std::vector < features_t< T > >;


template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool >::type = true >
class mfcc_t
{
	private:
	protected:
		static inline signal_t< T >&
		apply_window(signal_t< T >& chunk)
		{
			std::size_t n = chunk.size();

			for ( std::size_t idx = 0; idx < chunk.size(); idx++ )
				chunk[ idx ] *= ( 0.54 - 0.46 * std::cos(idx * 2.0 * M_PI / n - 1) );

			return chunk;
		}

		static inline signal_t< T >&
		pre_empasize(signal_t< T >& chunk, double coef = 0.95)
		{
			for ( std::size_t idx = 1; idx < chunk.size(); idx++ )
				chunk[ idx ] = ( chunk[ idx ] - coef * chunk[ idx - 1 ] );

			return chunk;
		}

		static inline signal_t< T >&
		emphasize_and_window(signal_t< T >& chunk, double coef = 0.95)
		{
			if ( 0 == chunk.size() )
				throw std::invalid_argument("Attempted to window chunk size of zero.");

			pre_empasize(chunk, coef);
			return apply_window(chunk);
		}

		static inline T
		energy(::signal_t< T >& chunk)
		{
			T r = 0.0;

			// -- E = log(sigma(n=1:N) x^2[n])

			for ( std::size_t idx = 1; idx <= chunk.size(); idx++ )
				r += chunk[ idx ] * chunk[ idx ];

			r = std::sqrt(r);

			// lifted from https://github.com/rainlabs/stranger/blob/master/src/misc.cpp
			if ( 1.0 > r )
				r = 1.0;

			return 2.0 * std::log(r);
		}

		static inline void
		compute_deltas(std::vector< features_t< T > >& v, std::size_t N = 2 )
		{
			std::size_t bl = v.at(0).features.size();
			T B = 0.0;
			T A1 = 0.0;
			T A2 = 0.0;
			T AE1 = 0.0;
			T AE2 = 0.0;

			// -- delta c[m] = sigma(i=1, k) i*(c[m+i] - c(m-i) / 2*sigma(i=1, k) i^2
			// -- delta d[t] = sigma(n=1, N) n*(c[t+n] - c[t-n] / 2*sigma(n=1, N) n^2
			// -- common value of N is 2

			for ( std::size_t theta = 1; theta <= N; theta++ )
				B += theta*theta;

			for ( std::size_t t = 0; t < v.size(); t++ ) {

				for ( std::size_t theta = 1; theta <= N; theta++ ) {
					AE1 = ( static_cast<int16_t>( t - theta ) < 0 ? v[ 0 ].energy : v[ t - theta ].energy );
					AE2 = ( t + theta >= v.size() ? v[ v.size() - 1 ].energy : v[ t + theta ].energy );
					v[ t ].differential.energy = ( theta * ( AE2 - AE1 ) ) / ( 2.0 * B );

				}

				for ( std::size_t n = 0; n  < v[t].features.size(); n++ ) {
					T sum = 0.0;

					for ( std::size_t theta = 1; theta <= N; theta++ ) {
						A1 = ( static_cast<int16_t>( t - theta ) < 0 ? v[ 0 ].features[ n ] : v[ t - theta ].features[ n ] );
						
						A2 = ( t + theta >= v.size() ? v[ v.size() - 1 ].features[ n ] : v[ t + theta ].features[ n ] );					
						
						sum += theta * ( A2 - A1 );
						
					}

					sum /= ( 2.0 * B );	
					v[ t ].differential.features[ n ] = sum;
				}
			}

			for ( std::size_t t = 0; t < v.size(); t++ ) {
				for ( std::size_t theta = 1; theta <= N; theta++ ) {
					AE1 = ( static_cast<int16_t>( t - theta ) < 0 ? v[ 0 ].differential.energy : v[ t - theta ].differential.energy );
					AE2 = ( t + theta >= v.size() ? v[ v.size() - 1 ].differential.energy : v[ t + theta ].differential.energy );
					v[ t ].acceleration.energy = ( theta * ( AE2 - AE1 ) ) / ( 2.0 * B );
				}

				for ( std::size_t n = 0; n < v[ t ].differential.features.size(); n++ ) {
					T sum = 0.0;

					for ( std::size_t theta = 1; theta <= N; theta++ ) {
						A1 = ( static_cast<int16_t>( t - theta ) < 0 ? v[ 0 ].differential.features[ n ] : v[ t - theta ].differential.features[ n ] );
						A2 = ( t + theta >= v.size() ? v[ v.size() - 1 ].differential.features[ n ] : v[ t + theta ].differential.features[ n ] );
						sum += theta * ( A2 - A1 );
					}

					sum /= ( 2.0 * B );
					v[ t ].acceleration.features[ n ] = sum;
				}
			}

			return;
		}

	public:
		mfcc_t(void) { return; }
		virtual ~mfcc_t(void) { return; }


		template< class T >
		static inline T
		mse(const std::vector< features_t< T > >& v0, std::vector< features_t< T > >& v1)
		{
			T					sum = 0.0;
			std::size_t			cnt = 0;
			features_t< T >		diff;

			for ( std::size_t idx = 0; idx < v0.size(); idx++ ) {
				if ( idx >= v1.size() )
					break;

				sum += ::mse< double >(v0[ idx ].features, v1[ idx ].features);
			}

			return sum / v0.size();
		}

		template< class T >
		static inline features_mse_t< T >
		mse(const features_t< T >& v0, const features_t< T >& v1)
		{
			features_mse_t< T >	ret = { 0 };
			features_t< T >		diff;

			diff.features				= v0.features - v1.features;
			diff.features				= ( diff.features*diff.features );
			diff.differential.features	= v0.differential.features - v1.differential.features;
			diff.differential.features	= ( diff.differential.features*diff.differential.features );
			diff.acceleration.features	= v0.acceleration.features - v1.acceleration.features;
			diff.acceleration.features	= ( diff.acceleration.features*diff.acceleration.features );

			diff.energy					= v0.energy - v1.energy;
			diff.differential.energy	= v0.differential.energy - v1.differential.energy;
			diff.acceleration.energy	= v0.acceleration.energy - v1.acceleration.energy;
			
			diff.energy					= ( diff.energy*diff.energy );
			diff.differential.energy	= ( diff.differential.energy*diff.differential.energy );
			diff.acceleration.energy	= ( diff.acceleration.energy*diff.acceleration.energy );

			for ( std::size_t idx = 0; idx < diff.features.size(); idx++ )
				ret.feature_sum += diff.features[ idx ];
			for ( std::size_t idx = 0; idx < diff.differential.features.size(); idx++ )
				ret.differential_sum += diff.differential.features[ idx ];
			for ( std::size_t idx = 0; idx < diff.acceleration.features.size(); idx++ )
				ret.acceleration_sum += diff.acceleration.features[ idx ];

			ret.energy				= diff.energy;
			ret.differential_energy = diff.differential.energy;
			ret.acceleration_energy = diff.acceleration.energy;

			ret.feature_sum			/= diff.features.size();
			ret.differential_sum	/= diff.differential.features.size();
			ret.acceleration_sum	/= diff.acceleration.features.size();
			ret.energy				/= 1;
			ret.differential_energy /= 1;
			ret.acceleration_energy /= 1;

			return ret;
		}

		static std::vector< features_t< T > > //std::vector< ::cosine_t< T > >
		calculate(signal_t< T >& sig, std::size_t ms, std::size_t fc, std::mutex* m = nullptr)
		{
			//std::vector< cosine_t< T > >	retvec;
			std::vector< features_t< T > >	features;
			signal_frames_t< T >			frames(sig, ms, 0.8);
			fft_t< T >*						fft(fft_factory_t< T >::getFFT(m));
			mel_filter_bank_t< T >			bank(frames.getFrame(0).sample_rate(), frames.getFrame(0).size());
			dct_t< T >						dct;

			for ( std::size_t idx = 0; idx < frames.count(); idx++ ) {
				features_t< T >				fe;
				::signal_t< T >				sig(emphasize_and_window(frames.getFrame(idx)));
				::complex_vector_t< T >		cv = fft->fft(sig);
				::fp_vector_t< T >			fo = log(bank.applyAll(cv));
				::cosine_t< T >				co(dct.dct(fo, fc));
				lifter_t< T >				lf(1, co.size());

				co = lf.apply(co);

				T mean = ::average(co);

				fe.features.resize(co.size());
				for ( std::size_t midx = 0; midx < co.size(); midx++ )
						fe.features[ midx ] = (co[ midx ] - mean);

				fe.energy = energy(sig);
				features.push_back(fe);
			}

			compute_deltas(features);
			delete fft;
			return features;
			//return retvec;
		}
};

#endif