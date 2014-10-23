#ifndef HAVE_MELFILTER_HPP
#define HAVE_MELFILTER_HPP

/*
-- This code is almost 100% a copy/paste of mel-frequency code in the Aquila DSP library 
-- written by Zbigniew Siciarz. It is only moderalte modified to make use of the data 
-- types/et cetera of mine. All credit/et cetera is due to the author of that code who 
-- did the actual work.
*/

#include <cstdint>
#include <cstddef>
#include <vector>

#include "fp_vector.hpp"
#include "complex_vector.hpp"


template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
class mel_filter_t
{
	private:
		double					m_sample;
		::fp_vector_t< T >		m_spectrum;

	protected:
		void 
		generateFilterSpectrum(double minFreq, double centerFreq, double maxFreq, std::size_t N)
		{
			std::size_t minPos = static_cast< std::size_t >( N * minFreq / m_sample );
			std::size_t maxPos = static_cast< std::size_t >( N * maxFreq / m_sample );
			const double maxv = 1.0;

			if ( maxPos <= minPos )
				throw std::invalid_argument("min/max frequency band spacing error due to invalid parameters.");

			if (N != m_spectrum.size() )
				m_spectrum.resize(N);

			for ( std::size_t idx = minPos; idx <= maxPos; idx++ ) {
				double cf = ( idx * m_sample ) / N;

				if ( cf < minFreq )
					continue;

				if ( cf < centerFreq )
					m_spectrum[ idx ] = ( cf - minFreq ) * maxv / ( centerFreq - minFreq );
				else
					if ( cf < maxFreq )
						m_spectrum[ idx ] = ( maxFreq - cf ) * maxv / ( maxFreq - centerFreq );

			}

			return;
		}

	public:
		mel_filter_t(void) : m_sample(0) { return; }
		mel_filter_t(double sr) : m_sample(sr) { return; }
		mel_filter_t(std::size_t sr) : m_sample(sr) { return; }

		~mel_filter_t(void) { return; }

		void sample_rate(double sr) { m_sample = sr; return; }
		static T linearToMel(T freq) { return 1127.01048 * std::log(1.0 + freq / 700.0); }
		static T melToLinear(T freq) { return 700.0 * ( std::exp(freq / 1127.01048) - 1.0 ); }
		std::size_t sample_rate(void) { return m_sample; }

		void
		createFilter(std::size_t filterNum, double filterWidth, std::size_t N)
		{
			double melMinFreq = filterNum * filterWidth / 2.0;
			double melCenterFreq = melMinFreq + filterWidth / 2.0;
			double melMaxFreq = melMinFreq + filterWidth;
			double minFreq = melToLinear(melMinFreq);
			double centerFreq = melToLinear(melCenterFreq);
			double maxFreq = melToLinear(melMaxFreq);

			generateFilterSpectrum(minFreq, centerFreq, maxFreq, N);
			return;
		}

		T 
		apply(const ::complex_vector_t< T >& v) const
		{
			T					val = 0.0;
			const std::size_t	N	= v.size();

			for ( std::size_t idx = 0; idx < N; idx++ ) {
				// Aquila just does val += v[idx] * m_spectrum[idx]
				// but best I can tell that is wrong and/or incomplete
				// -- Pi(k) = 1/N * |Si(k)|^2
				//T av = std::abs(v[ idx ]);
				//val += (1/N*(av*av)) * m_spectrum[ idx ];
				val += std::abs(v[ idx ]) * m_spectrum[ idx ];
			}

			return val;
		}
};

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
class mel_filter_bank_t
{
	private:
		std::size_t m_sample;
		std::size_t m_N;
		std::vector< mel_filter_t< T > > m_filters;

	protected:
	public:
		// -- Aquila defaults to 24 filters, whereas others use 24-26 or so
		// -- however a paper out of the University of Beijing appears to have
		// -- done some comprehensive research and discerned that the optimal
		// -- number of filters is 35.
		// -- 
		// -- The same paper seems to indicate that overalled rectangular filters 
		// -- actually performs better than triangular; however the gains were
		// -- minimal and so (currently) unimplemented.
		// --
		// -- ...
		mel_filter_bank_t(double sr, std::size_t l, double fw = 200.0, std::size_t bs = 24) : m_sample(sr), m_N(l)
		{
			m_filters.resize(bs);

			for ( std::size_t idx = 0; idx < bs; idx++ ) {
				m_filters[ idx ].sample_rate(sr);
				//m_filters.push_back(mel_filter_t< T >(sr));
				m_filters[ idx ].createFilter(idx, fw, m_N);
			}

			return;
		}

		~mel_filter_bank_t(void)
		{

			//for ( std::size_t idx = 0; idx < m_filters.size(); idx++ )
			//	delete m_filters[ idx ];

			m_filters.clear();
			return;
		}

		::fp_vector_t< T > 
		applyAll(const ::complex_vector_t< T > &fs) const
		{
			::fp_vector_t< T > out; 

			out.resize(size());

			for ( std::size_t idx = 0; idx < m_filters.size(); idx++ )
				out[ idx ] = m_filters[ idx ].apply(fs);

			return out;
		}

		std::size_t getSampleFrequency(void) const { return m_sample; }
		std::size_t getSpectrumLength() const { return m_N; }
		std::size_t size() const { return m_filters.size(); }

};


#endif