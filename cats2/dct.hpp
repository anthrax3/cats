#ifndef HAVE_DCT_HPP
#define HAVE_DCT_HPP

/* This implementation of Discrete Cosine Transform (DCT) calculations is almost entirely
 * Based on the implementation contained within the Aquila DSP library written by Zbigniew Siciarz.
 * Mostly, it was modified to fit my own personal style and to utilize the data types I've written.
*/

#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <iterator>
#include <cstddef>
#include <map>
#include <utility>
#include <vector>

#include "fp_vector.hpp"

template< class T > using cosine_t = ::fp_vector_t < T > ;
typedef std::pair< std::size_t, std::size_t > dct_key_t;
template< class T > using cosine_vector_t = std::vector< cosine_t< T > >;
template< class T > using cosine_cache_t = std::map < dct_key_t, cosine_vector_t< T >* >;

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
class dct_t
{
	private:
		cosine_cache_t< T > m_cache;


	protected:
		cosine_vector_t< T >&
		getCosineVector(std::size_t il, std::size_t ol)
		{
			cosine_vector_t< T >* cos = fetchFromCache(il, ol);
			return *cos;
		}
		
		cosine_vector_t< T >* 
		fetchFromCache(std::size_t il, std::size_t ol)
		{
			cosine_vector_t< T >* ret(nullptr);

			auto key = std::make_pair(il, ol);

			if ( m_cache.end() != m_cache.find(key) )
				return m_cache[ key ];

			ret = new cosine_vector_t< T >(ol);

			for ( std::size_t n = 0; n < ol; n++ ) {
				(*ret)[ n ].resize(il);

				for ( std::size_t k = 0; k < il; k++ )
					(*ret)[ n ][ k ] = std::cos((M_PI * ( 2 * k + 1 ) * n) / ( 2.0 * il ));
			}

			m_cache[ key ] = ret;
			return ret;
		}

		void 
		clearCache(void)
		{
			for ( auto itr = m_cache.begin(); itr != m_cache.end(); itr++ ) {
				auto					key = itr->first;
				cosine_vector_t< T >*	vec = itr->second;
				std::size_t				len = key.second;

				itr->second->resize(0);
				delete itr->second;

			}

			m_cache.clear();
			return;
		}

	public:
		dct_t(void) : m_cache() { return; }
		~dct_t(void) { clearCache(); return; }

		cosine_t< T > 
		dct(::fp_vector_t< T >& v, std::size_t ol)
		{
			cosine_t< T >			ret(ol);
			std::size_t				il(v.size());
			cosine_vector_t< T >&	cos(getCosineVector(il, ol));
			T						c0(std::sqrt(1.0 / il));
			T						cn(std::sqrt(2.0 / il));

			for ( std::size_t n = 0; n < ol; n++ ) {
				for ( std::size_t k = 0; k < il; k++ )
					ret[ n ] += ( v[ k ] * cos[ n ][ k ] ); // v0 * v1;
				
				ret[ n ] *= ( 0 == n ) ? c0 : cn;
			}

			return ret;
		}

};

#endif
