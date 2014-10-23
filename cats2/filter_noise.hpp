#ifndef HAVE_FILTER_NOISE_HPP
#define HAVE_FILTER_NOISE_HPP

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <stdexcept>

#include "filter_bank.hpp"
#include "signal.hpp"
#include "complex_vector.hpp"
#include "fft_factory.hpp"

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
class filter_noise_t
{
	private:
		const double		m_base;
		const double		m_max;
		const double		m_bandwidth;
		const double		m_overlap;
		const std::size_t	m_sample;
		filter_bank_t*		m_bank;
		window_t< T >*		m_window;

	protected:
		signal_t< T >
		get_envelope(fft_t< T >& fft, complex_vector_t< T >& band)
		{
			complex_vector_t< T >&	shifted(band);
			signal_t< T >			envelope;
			signal_t< T >			shift;

			if ( band.size() <= 1 )
				throw std::invalid_argument("Invalid band length passed as a parameter");
			
			for ( std::size_t idx = 0; idx < shifted.size(); idx++ )
				shifted[ idx ] = std::conj(std::complex< T >(shifted[ idx ].imag(), shifted[ idx ].real()));

			envelope	= fft.ifft(band);
			shift		= fft.ifft(shifted);

			for ( std::size_t idx = 0; idx < envelope.size(); idx++ )
				envelope[ idx ] = std::sqrt(envelope[ idx ] * envelope[ idx ] + shift[ idx ] * shift[ idx ]);

			return envelope;
		}

	public:
		filter_noise_t(double base, double max, double bandwidth, double overlap, std::size_t sr)
			:	m_base(base), m_max(max), m_bandwidth(bandwidth), 
				m_overlap(overlap), m_sample(sr), 
				m_bank(nullptr), m_window(nullptr)
		{ 
			return; 
		}

		filter_noise_t(std::size_t sr)
			:	m_base(0.0), m_max(static_cast< double >( sr / 2 )), 
				m_overlap(0.8), m_sample(sr), m_bandwidth(100.0),
				m_bank(nullptr), m_window(nullptr)
		{
			return;
		}

		virtual 
		~filter_noise_t(void)
		{
			return;
		}
	
		signal_t< T >
		apply(signal_t< T >& signal)
		{
			signal_frames_t< T >	sf(signal, 1000, m_overlap);
			signal_frames_t< T >	ret;
			signal_vector_t< T >	tmp;
			fft_t< T >*				fft(fft_factory_t< T >::getFFT());
			complex_vector_t< T >	freq;
			/*complex_vector_t< T >	cv(fft->fft(signal));
			const double			scale(static_cast<double>( ( cv.size() * 2 ) / signal.sample_rate() ));
			std::size_t				bands(0);
			std::size_t				top(m_max > cv.size() / 2 ? m_max*scale : ( cv.size() / 2 )*scale);
			signal_t< T >			ret;*/

			if ( signal.sample_rate() != m_sample )
				throw std::invalid_argument("Invalid/differing sample rate");

			ret.sample_rate(signal.sample_rate());

			for ( std::size_t fidx = 0; fidx < sf.count(); fidx++ ) {
				complex_vector_t< T >	spectrum(fft->fft(sf.getFrame(fidx)));
				double					scale(static_cast<double>( ( spectrum.size() * 2 ) / signal.sample_rate() ));
				std::size_t				bands(0);
				std::size_t				top(m_max > spectrum.size() / 2 ? m_max*scale : ( spectrum.size() / 2 )*scale);

				if ( nullptr == m_bank )
					m_bank = new filter_bank_t(scale, m_base, m_bandwidth, m_overlap);

				if ( top > spectrum.size() )
					top = spectrum.size(); // ?? throw

				for ( std::size_t idx = 0;; idx++ ) {
					filter_range_t			range = m_bank->get_band(idx);
					complex_vector_t< T >	slice;
					signal_t< T >			sig;
					std::size_t				center = m_bank->get_center(idx);
					std::size_t				offset = 0; // std::max((size_t)0, center - slice.size() / 2);

					if ( range.first >= top )
						break;

					if ( range.second < range.first )
						throw std::runtime_error("Band range indices invalid");

					slice.resize(range.second - range.first);
					::memcpy_s(&slice[ 0 ], slice.size(), &spectrum[ range.first ], std::min(range.second, top) - range.first);

					if ( top < range.second ) {
						for ( std::size_t tidx = top - range.first; tidx < slice.size(); tidx++ )
							slice[ tidx ] = std::complex< T >(0, 0);
					}

					if ( nullptr == m_window )
						m_window = new window_t< T >(slice.size());

					m_window->apply(slice);

					offset = std::max((size_t)0, center - slice.size() / 2);
					freq.grow(slice.size());

					for ( size_t i = 0; i < slice.size(); ++i ) {
						if ( offset + i > 0 && offset + i < freq.size() )
							freq[ offset + i ] += slice[ i ];
					}


					ret.putFrame(fft->ifft(freq));
					freq.clear();
					//sig = get_envelope(*fft, slice);

				}

				ret.sample_rate(signal.sample_rate());
				//tmp.push_back(ret);
			}

			//ret = fft->ifft(freq);
			//ret.sample_rate(signal.sample_rate());
			
			delete fft;
			delete m_window;
			delete m_bank;
			return overlap_add(ret, m_overlap);
		}
};

#endif