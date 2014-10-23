#ifndef PERCEPTUAL_HPP
#define PERCEPTUAL_HPP

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <exception>
#include <mutex>
#include <memory>

#include "signal.hpp"
#include "mel_filter.hpp"
#include "fft_factory.hpp"

typedef std::pair < std::size_t, std::size_t > band_t;

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool >::type = true >
class linear_filterbank_t
{
	private:
		std::mutex			m_mutex;
		const T				m_bandwidth;
		const T				m_scale;
		const std::size_t	m_start;
		const T				m_step;

	protected:
	public:
		linear_filterbank_t(T scale, T base, T hzbandwidth, T overlap) : m_scale(scale), m_bandwidth(hzbandwidth*scale), 
																m_start(std::max(m_scale*base - m_bandwidth / 2, 0.0)),
																m_step((1-overlap)*m_bandwidth)
		{
			std::lock_guard< std::mutex > l(m_mutex);

			if ( 0 > m_step )
				throw std::invalid_argument("Invalid parameter(s) resulted in an invalid filter bank step");

			return;
		}


		virtual ~linear_filterbank_t(void)
		{
			std::lock_guard< std::mutex > l(m_mutex);
			return;
		}

		std::size_t 
		num_bands_est(T maxfreq) const
		{
			return ( maxfreq * m_scale - m_start ) / m_step;
		}

		band_t
		band(std::size_t idx) const
		{
			band_t out;

			out.first	= m_start + idx * m_step;
			out.second	= out.first + m_bandwidth;

			return out;
		}

		std::size_t
		center(std::size_t idx) const
		{
			return m_start + idx * m_step + m_bandwidth / 2.0;
		}


};

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool >::type = true >
class perceptual_t 
{
	private:
		template< class T > using mfb_ptr = std::auto_ptr < mel_filter_bank_t< T > >;

		std::mutex					m_mutex;
		signal_t< T >				m_signal;
		mfb_ptr< T >				m_bank;
		//linear_filterbank_t< T >*

	protected:
	public:
		perceptual_t(QObject* p = nullptr) : m_bank(nullptr) { return; }
		~perceptual_t(void) 
		{ 
			m_bank.release();
			return; 
		}
		
		void 
		signal(signal_t< T >& signal)
		{
			const T		maxfreq	= signal.sample_rate() / 2;
			const T		fscale	= ( static_cast< T >( signal.size() ) * 2 ) / signal.sample_rate();
			std::size_t top		= static_cast< std::size_t >( maxfreq*fscale );

			m_signal.resize(signal.size());
			m_signal = signal;

			m_bank.release();

			m_bank = std::auto_ptr< mel_filter_bank_t< T > >(new mel_filter_bank_t< T >(signal.sample_rate(), 110));

			return;
		}

		signal_t< T >& 
		signal(void)
		{
			return m_signal;
		}



};

#endif // PERCEPTUAL_HPP
