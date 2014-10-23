#ifndef HAVE_WINDOW_HPP
#define HAVE_WINDOW_HPP

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <type_traits>
#include <utility>
#include <algorithm>

#include "fp_vector.hpp"
#include "complex_vector.hpp"


// -- Most of these have been transcribed verbatim from wikipedia
// -- and have not been tested/used. If you encounter an issue with
// -- one, by all means let me know and I will fix it.

typedef enum window_class_t
{
	WINDOW_HANN = 0,			WINDOW_HAMM,				WINDOW_BLACKMAN,
	WINDOW_BARTLETT,			WINDOW_BLACKMAN_HARRIS,		WINDOW_WELCH,
	WINDOW_FLATTOP,				WINDOW_COSINE,				WINDOW_NUTALL,
	WINDOW_BLACKMAN_NUTALL,		WINDOW_RECTANGULAR,			WINDOW_TUKEY,
	WINDOW_LANCZOS,				/*WINDOW_GAUSSIAN,*/		WINDOW_BARTLETT_HANN,
	WINDOW_HANN_POISSON,
	WINDOW_INVALID
};

#define WINDOW_TRIANGULAR WINDOW_BARTLETT
#define WINDOW_SINE WINDOW_COSINE

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
class window_t
{
	private:
	window_class_t		m_type;
	std::size_t			m_size;
	fp_vector_t< T >	m_window;
	T					m_real;

	protected:

		static inline T sinc(const T x)
		{
			return std::sin(M_PI*x) / M_PI*x;
		}

		void initialize(void)
		{
			std::size_t cnt = 0;

			if ( m_window.size() != m_size )
				m_window.resize(m_size);


			if ( WINDOW_BARTLETT == m_type )
				cnt = m_size / 2;
			else if ( WINDOW_TUKEY == m_type )
				cnt = m_size - 1;
			else
				cnt = m_size;

			for ( std::size_t idx = 0; idx < cnt; idx++ ) {
				switch ( m_type ) {
					case WINDOW_HANN:
						m_window[ idx ] = 0.5 * ( 1.0 - std::cos(M_PI * 2.0 * idx / ( m_size - 1.0 )) );
						break;

					case WINDOW_HAMM:
						m_window[ idx ] = 0.54 - 0.46 * std::cos(2.0 * M_PI * idx / ( m_size - 1.0 ));
						break;

					case WINDOW_BLACKMAN:
						m_window[ idx ] = 0.42 - 0.5 * std::cos(2.0 * M_PI * idx / ( m_size - 1.0 )) + 0.08 *
										std::cos(4.0 * M_PI * idx / ( m_size - 1.0 ));
						break;

					case WINDOW_BARTLETT:
						m_window[ idx ] = ( idx / static_cast<T>( ( m_size / 2.0 ) ) );
						m_window[ idx + ( m_size / 2.0 ) ] = ( 1.0 - ( idx / static_cast<T>( ( m_size / 2.0 ) ) ) );
						break;

					case WINDOW_BLACKMAN_HARRIS:
						m_window[ idx ] = 0.35875 - 0.48829 * std::cos(2.0 * M_PI * idx / ( m_size - 1.0 ))
										+ 0.14128 * std::cos(4.0 * M_PI * idx / ( m_size - 1.0 )) - 0.01168
										* std::cos(6.0 * M_PI * idx / ( m_size - 1.0 ));
						break;

					case WINDOW_WELCH:
						m_window[ idx ] = 4.0 * idx / static_cast< T >(m_size)* ( 1.0 - ( idx / static_cast< T >( m_size ) ) );
						break;

					case WINDOW_FLATTOP:
						m_window[ idx ] = 1.0 - 1.93 * std::cos(2.0 * M_PI * idx / ( m_size - 1.0 ))
										+ 1.29 * std::cos(4.0*M_PI*idx / ( m_size - 1.0 )) - 0.388
										* std::cos(6.0*M_PI*idx / ( m_size - 1.0 )) + 0.028
										* std::cos(8.0*M_PI*idx / ( m_size - 1.0 ));
						break;

					case WINDOW_COSINE:
						m_window[ idx ] = std::cos((M_PI * idx / (m_size - 1.0)) - (M_PI / 2.0));
						break;

					case WINDOW_NUTALL:
						m_window[ idx ] = 0.355768 - 0.487396 * std::cos(2.0 * M_PI * idx / (m_size - 1.0))
										+ 0.144232 * std::cos(4.0 * M_PI * idx / (m_size - 1.0)) - 0.012604
										* std::cos(6.0 * M_PI * idx / (m_size - 1.0));
						break;

					case WINDOW_BLACKMAN_NUTALL:
						m_window[ idx ] = 0.355768 - 0.487396 * std::cos(2.0 * M_PI * idx / (m_size - 1.0))
										+ 0.144232 * std::cos(4.0 * M_PI * idx / (m_size - 1.0)) - 0.012604
										* std::cos(6.0 * M_PI * idx / (m_size - 1.0));
						break;

					case WINDOW_RECTANGULAR:
						m_window[ idx ] = 1.0;
						break;

					// -- https://github.com/JuliaDSP/DSP.jl/blob/master/src/windows.jl
					case WINDOW_TUKEY:
						if ( 0.0 > m_real || 1.0 < m_real )
							throw std::invalid_argument("Alpha value must be >= 0.0 && <= 1.0");

						if ( 0.0 == m_real )
							m_window[ idx ] = 1.0;
						else {
							T m = m_real * ( idx - 1.0 ) / 2.0;

							if ( idx <= m )
								m_window[ idx + 1 ] = 0.5 * ( 1.0 + std::cos(M_PI * ( idx / m - 1.0 )) );
							else if ( idx <= m_size - 1.0 - m )
								m_window[ idx + 1 ] = 1.0;
							else
								m_window[ idx ] = 0.5 * ( 1.0 + std::cos(M_PI * ( (idx / (m - 2.0)) / (m_real + 1.0) )) );

						}
		
						break;

					// -- https://github.com/JuliaDSP/DSP.jl/blob/master/src/windows.jl
					case WINDOW_LANCZOS:
						m_window[idx] = sinc(2 * idx / ( m_size - 1.0 ) - 1.0);
						break;
					
						// -- https://github.com/JuliaDSP/DSP.jl/blob/master/src/windows.jl
					/*case WINDOW_GAUSSIAN:
						if ( 0.0 > m_real || 0.5 < m_real )
							throw std::invalid_argument("Sigma value must be >= 0.0 && <= 0.5");

						m_window[ idx ] = std::exp(-0.5*( ( idx - ( m_size - 2.0 ) / 2.0 ) 
										/ ( m_real * ( (m_size - 1.0) / 2.0 ) ) ) ^ 2);
						break;*/

					case WINDOW_BARTLETT_HANN:
						m_window[ idx ] = 0.62 - 0.48 * std::abs((idx / ( m_size - 1.0 )) - 1.0 / 2.0) - 
										0.38 * std::cos(2.0 * M_PI * idx / (m_size - 1.0));
						break;

					case WINDOW_HANN_POISSON:
						if ( 0.0 <= m_real )
							m_real *= -1.0;

						m_window[ idx ] = 1.0 / 2.0 * ( 1.0 - std::cos(2.0 * M_PI * idx / (m_size - 1.0)) ) * 
										std::exp(m_real * std::abs((m_size - 1.0 - 2.0) * idx / (m_size - 1.0)));
						break;

					default:
						throw std::invalid_argument("Unknown or unsupported window type requested");
				};

				
			}

			return;
		}
	public:

		window_t(void) : m_type(WINDOW_INVALID), m_size(0), m_window(0), m_real(0.0) { return; }
		window_t(std::size_t n, window_class_t t = WINDOW_HANN, T r = T(0.0)) : m_size(n), m_type(t), m_window(n), m_real(r)
		{
			initialize();
		}

		void 
		apply(fp_vector_t< T >& vec, bool resize = false) 
		{
			std::size_t vsize = vec.size();
			std::size_t wsize = m_window.size();

			if ( vec.size() != m_window.size() && vec.size() > m_window.size() ) {
				if (false == resize)
					throw std::invalid_argument("Attempted to apply a window to a signal with a different size!");
				else {
					m_window.clear();
					m_window.resize(vec.size());
					initialize();
				}
			}

			for ( std::size_t idx = 0; idx < vec.size(); idx++ )
				vec[ idx ] *= m_window[ idx ];

			return;
		}

		void
		apply(complex_vector_t< T >& vec, bool resize = false)
		{
			std::size_t vsize = vec.size();
			std::size_t wsize = m_window.size();

			if ( vec.size() != m_window.size() && vec.size() > m_window.size() ) {
				if ( false == resize )
					throw std::invalid_argument("Attempted to apply a window to a signal with a different size!");
				else {
					m_window.clear();
					m_window.resize(vec.size());
					initialize();
				}
			}

			for ( std::size_t idx = 0; idx < vec.size(); idx++ )
				vec[ idx ] *= m_window[ idx ];

			return;
		}

		void 
		setWindowType(std::size_t n, window_class_t t = WINDOW_HANN, T r = T(0.0))
		{
			m_window.clear();
			m_window.resize(n);

			m_size = n;
			m_type = t;
			m_real = r;
			initialize();

			return;
		}
};

#endif