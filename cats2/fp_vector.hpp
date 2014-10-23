#ifndef HAVE_FP_VECTOR_HPP
#define HAVE_FP_VECTOR_HPP

#include <type_traits>
#include <cstdint>
#include <stdexcept>
#include <valarray>
#include <vector>
#include <numeric>
#include <algorithm>
#include <memory>
#include <mutex>

#if defined(DEBUG)
#include <random>
#endif

/*#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif*/

#ifdef _MSC_BUILD
#define RESTRICT __restrict 
#else
#define RESTRICT __restrict__
#endif


template< class T > class slice_vector_t;

class fp_slice_t
	{
		private:
			std::size_t m_start;
			std::size_t m_length;
	
		protected:
		public:
			fp_slice_t(void) : m_start(0), m_length(0) { return; }
			fp_slice_t(std::size_t start, std::size_t length) : m_start(start), m_length(length) { return; }
			virtual ~fp_slice_t(void) { m_start = 0; m_length = 0; return; }
			
			std::size_t start(void) const { return m_start; }
			std::size_t length(void) const { return m_length; }

	};

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
struct fp_vector_base_t
	{
		T* RESTRICT			m_data;
		std::size_t			m_size;
		std::mutex			m_mutex;

		fp_vector_base_t(void) :  m_data(nullptr), m_size(0) { return; }

		fp_vector_base_t(std::size_t s) :  m_data(nullptr), m_size(0)
		{
			this->allocate(s);
			return;
		}

		virtual ~fp_vector_base_t(void)
		{
			this->deallocate();
			return;
		}

		void 
		allocate(std::size_t n)
		{
			std::lock_guard< std::mutex > l(m_mutex);

			if ( SIZE_MAX / sizeof(T) < n )
				throw std::overflow_error("Multiplicative integer overflow condition encountered.");

			if ( 0 != n ) {

				m_data = static_cast<T*>( ::malloc(n * sizeof(T)) );

				if ( nullptr == m_data ) 
					throw std::bad_alloc();

				m_size = n;

			} else {
				m_data = nullptr;
				m_size = 0;
			}

			return;
		}

		void 
		reallocate(std::size_t n)
		{
			std::lock_guard< std::mutex >	l(m_mutex);
			//std::size_t						s(n);


			if ( SIZE_MAX-m_size / sizeof(T) < n )
				throw std::overflow_error("Multiplicative integer overflow condition encountered.");

			//s += m_size;

			if ( 0 != n ) {

				m_data = static_cast< T* >( ::realloc(m_data, n * sizeof(T)) );

				if ( nullptr == m_data )
					throw std::bad_alloc();

				m_size = n; //s;
			}

			return;
		}
	
		void 
		deallocate(void)
		{
			std::lock_guard< std::mutex > l(m_mutex);

			if ( nullptr != m_data ) {
				::free(m_data);
				m_data = nullptr;
				m_size = 0;
			}

			return;
		}


		static void
		memcpy_sec(T* RESTRICT dst, std::size_t dlen, T* RESTRICT src, std::size_t slen)
		{

			if (SIZE_MAX / sizeof(T) < slen || SIZE_MAX / sizeof(T) < dlen)
				throw std::overflow_error("Multiplicative integer overflow condition encountered.");

#ifndef _MSC_BUILD
			if ( slen > dlen )
				throw std::bad_exception("Your C runtime presumably does not support memcpy_s(), "
										"so we emulated it. You tried to memcpy too much data, "
										"sorry it didn't work out.");

			std::memcpy(dst, src, slen*sizeof(T));
#else
			::memcpy_s(dst, dlen*sizeof(T), src, slen*sizeof(T));
#endif
			return;
		}
	};

template< typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true >
class fp_vector_t : private fp_vector_base_t< T >
{
	private:
	protected:
	public:
		struct no_init { };

		fp_vector_t(std::size_t c, no_init) : fp_vector_base_t< T >(c) { return; }
		fp_vector_t(void) : fp_vector_base_t< T >() { return; }

		explicit fp_vector_t(std::size_t c) : fp_vector_base_t< T >(c)
		{
			for ( std::size_t idx = 0; idx < m_size; idx++ )
				m_data[ idx ] = 0.0;

			//::memset(m_data, T(), m_size*sizeof(T));
			return;

		}

		fp_vector_t(const T& v, std::size_t c) : fp_vector_base_t< T >(c)
		{
			::memset(m_data, v, m_size*sizeof(T));
			return;
		}
			
		fp_vector_t(const T* v, size_t c) : fp_vector_base_t< T >(c)
		{
			if ( nullptr == v )
				throw std::invalid_argument("nullptr exception");

			memcpy_sec(m_data, m_size, v, c);
			return;
		}

		fp_vector_t(const fp_vector_t< T >& o) : fp_vector_base_t< T >(o.m_size)
		{

			memcpy_sec(m_data, m_size, o.m_data, o.m_size);
			return;
		}

		fp_vector_t(fp_vector_t< T >&& o) : fp_vector_base_t< T >(o.m_size)
		{

			memcpy_sec(m_data, m_size, o.m_data, o.m_size);
			return;
		}

		fp_vector_t(std::initializer_list< T > i) : fp_vector_base_t< T >(i.size())
		{
			std::size_t idx		= 0;
			auto		ibeg	= i.begin();
			auto		iend	= i.end();

			for ( auto itr = ibeg; itr != iend; itr++, idx++ )
					m_data[ idx ] = *itr;

			return;
		}

		fp_vector_t(const slice_vector_t< T >& sv) : fp_vector_base_t< T >(sv.m_slice.length())
		{
			*this = sv;
			return;
		}

		virtual ~fp_vector_t(void)
		{
			// -- We only support 3 data types, all of which are
			// trivial, so there is no need to call their destructor.
			return;
		}

/*			fp_vector_t< T > operator+( void ) const { return *this; }

			fp_vector_t< T > 
			operator-( void ) const 
			{
				fp_vector_t< T > tmp(m_size);

				#pragma omp parallel
				{
					#pragma omp for
					for ( std::size_t idx = 0; idx < m_size; idx++ )
						tmp[ idx ] = -( *this )[ idx ];

				}
				return 
			}*/

		fp_vector_t<T>&	
		operator=( const fp_vector_t< T >& o) 
		{

			if ( this != &o ) {
				if (m_size != o.m_size) 
					resize(o.m_size);

				memcpy_sec(m_data, m_size, o.m_data, o.m_size);
			}

			return *this;
		}
			
		fp_vector_t<T>&	
		operator=( fp_vector_t<T>&& o ) 
		{

			if ( this != &o ) 
				if ( m_size != o.m_size )
					resize(o.m_size);

				memcpy_sec(m_data, m_size, o.m_data, o.m_size);

				return *this;
		}

		fp_vector_t<T>&	
		operator=( const T& v ) 
		{
			if ( m_size < 1 )
				resize(1);

			::memset(m_data, m_size*sizeof(T), v);

			return *this;
		}

		fp_vector_t< T >&
		operator=( const slice_vector_t< T >& o )
		{
			std::size_t			idx = o.m_slice.start();
			const std::size_t	len = o.m_slice.length();
			const std::size_t	end = idx + len;

			if (m_size != len)
				resize(len);

			for ( std::size_t cnt = 0; idx < end; idx++, cnt++ )
				m_data[ cnt ] = o.m_vector[ idx ];

			//memcpy_sec(m_data, m_size, &o.m_vector.m_data[ o.m_slice.start() ], o.m_slice.length());
			return *this;
		}

		fp_vector_t<T>&	
		operator=( std::initializer_list< T > i )
		{
			resize(i.size());

			std::copy(i.begin(), i.end(), m_data);
			return *this;
		}

		const T& 
		operator[](std::size_t pos) const
		{
			return m_data[ pos ];
		}

		T& 
		operator[](std::size_t pos)
		{
			return m_data[ pos ];
		}

		inline slice_vector_t< T >
		operator[](fp_slice_t s)
		{
			if ( s.start() + s.length() > size())
				throw std::range_error("The range specified by the slice is longer than the vector.");

			if ( SIZE_MAX - s.start() < s.length() )
				throw std::overflow_error("Additive overflow with slice start+length parameters.");

			return slice_vector_t< T >(s, *this);
		}

		fp_vector_t<T>	
		operator[](fp_slice_t s) const
		{
			fp_vector_t< T >	tmp(s.length(), no_init());

			memcpy_sec(tmp.m_data, tmp.m_size, m_data[ s.start() ], s.length());
			return tmp;
		}


/*		fp_vector_t< T >
		operator+=( const fp_vector_t< T >& v )
		{
			//fp_vector_t< T >	tmp(0, no_init());
			//std::size_t			len = m_size + v.size();
			//std::size_t			asz = v.size();
			//std::size_t			idx = 0;


			for ( std::size_t idx = 0; idx < m_size; idx++ )
				( *this )[ idx ] += v[idx];

			return *this;*/

			/*if ( SIZE_MAX - m_size > v.size() )
				throw std::overflow_error("Additive integer overflow condition encountered.");

			tmp.resize(len);

			for ( idx = 0; idx < m_size; idx++ )
				tmp[ idx ] = ( *this )[ idx ];

			for ( ; idx < m_size + asz; idx++ )
				tmp[ idx ] = v[ idx ];

			return tmp;*/
		//}

		fp_vector_t< T >&	
		operator+=( const T& v )
		{
			for ( std::size_t idx = 0; idx < m_size; idx++ )
				( *this )[ idx ] += v;

			return *this;
		}

		fp_vector_t< T >&			
		operator-=( const T& v )
		{
			for ( std::size_t idx = 0; idx < m_size; idx++ )
				( *this )[ idx ] -= v;

			return *this;
		}

		fp_vector_t< T >&
		operator*=( const T& v )
		{
			for ( std::size_t idx = 0; idx < m_size; idx++ )
				( *this )[ idx ] *= v;

			return *this;
		}

		fp_vector_t< T >&			
		operator/=( const T& v )
		{
			for ( std::size_t idx = 0; idx < m_size; idx++ )
				( *this )[ idx ] /= v;

			return *this;
		}

		fp_vector_t< T >&			
		operator%=( const T& v )
		{
			for ( std::size_t idx = 0; idx < m_size; idx++ )
				( *this )[ idx ] = ( ( *this )[ idx ] % v );

			return *this;
		}

		std::size_t size(void) const { return m_size; }

		void 
		resize(std::size_t c, T v = T(0.0))
		{
			deallocate();
			allocate(c);

			for ( std::size_t idx = 0; idx < m_size; idx++ )
				m_data[ idx ] = v;

			//::memset(m_data, v, m_size*sizeof(T));
	
			return;
		}
			
		T 
		sum(void) const
		{
			T v = 0.0;

			for ( std::size_t idx = 0; idx < m_size; idx++ )
				v += m_data[ idx ];

			return v;
		}

		T 
		max(void) const
		{
			T mv = 0.0;

			for ( std::size_t idx = 0; idx < m_size; idx++ )
				if ( m_data[ idx ] > mv )
					mv = m_data[ idx ];
			
			return mv;
			//return std::max_element(m_data[ 0 ], m_data[ m_size - 1 ]);
		}

		T 
		min(void) const
		{
			T mv = 0.0;

			for ( std::size_t idx = 0; idx < m_size; idx++ )
				if ( m_data[ idx ] < mv )
					mv = m_data[ idx ];

			return mv;
			//return std::min_element(m_data[ 0 ], m_data[ m_size - 1 ]);
		}

		fp_vector_t< T > 
		apply(T f(T)) const
		{
			fp_vector_t< T > r = fp_vector_t< T >(*this);
			std::size_t midx = r.size();

			for ( std::size_t idx = 0; idx < midx; idx++ )
				r[ idx ] = f(r[ idx ]);

			return r;
		}

		fp_vector_t< T > 
		apply(T f(const T&)) const
		{
			fp_vector_t< T > r = fp_vector_t< T >(*this);
			std::size_t midx = r.size();

			for ( std::size_t idx = 0; idx < midx; idx++ )
				r[ idx ] = f(r[ idx ]);

			return r;
		}

		void
		grow(std::size_t n, T v = T(0.0))
		{
			std::size_t		max = m_size;
			
			if ( SIZE_MAX - m_size < n )
				throw std::overflow_error("Additive integer overflow encountered");

			reallocate(n + m_size);
		
			::memset(&m_data[ max ], v, n * sizeof(T));

			return;
		}

		void
		shrink(std::size_t n)
		{
			std::size_t old = m_size;

			if ( n > m_size )
				throw std::overflow_error("Integer underflow encountered");

			reallocate(n);

			return;
		}

		void
		concat(const fp_vector_t< T >& o)
		{
			std::size_t c = 0;
			std::size_t m = m_size;

			if ( SIZE_MAX - m_size < o.size() )
				throw std::overflow_error("Additive integer overflow encountered");

			reallocate(m_size+o.size());


			for ( std::size_t idx = m; idx < m + o.size(); c++, idx++ )
				m_data[ idx ] = o[ c ];

			return;
		}

		void
		concat(const std::vector< T >& o)
		{
			std::size_t c = 0;
			std::size_t m = m_size;

			if ( SIZE_MAX - m_size < o.size() )
				throw std::overflow_error("Additive integer overflow encountered");

			reallocate(m_size + o.size());

			for ( std::size_t idx = m; idx < m + o.size(); c++, idx++ )
				m_data[ idx ] = o[ c ];

			return;
		}

		void
		clear(void)
		{
			deallocate();
			m_data = nullptr;
			m_size = 0;
			return;
		}

		void
		push_back(const T& v)
		{
			std::size_t idx = m_size;

			grow(1);
			m_data[ idx ] = v;

			return;
		}

		void
		push_back(const fp_vector_t< T >& o)
		{
			concat(o);
			return;
		}

		void
		push_back(const std::vector< T >& o)
		{
			concat(o);
			return;
		}

};

template< class T > 
class slice_vector_t
{
	private:
		friend class fp_vector_t < T > ;

		fp_slice_t			m_slice;
		fp_vector_t< T >&	m_vector; 

		slice_vector_t(void);
		slice_vector_t(const fp_slice_t& s, fp_vector_t< T >& v) : m_slice(s), m_vector(v) { return; }

	protected:
	public:
		fp_slice_t slice(void) { return m_slice; }
		fp_vector_t< T >& vector(void) { return m_vector; }

		void 
		operator=( const fp_vector_t< T >& o) //const
		{
			std::size_t			idx = m_slice.start();
			const std::size_t	len = m_slice.length();
			const std::size_t	end = idx + len;

			for ( ; idx < end; idx++ )
				m_vector[ idx ] = o[ idx ];

			return;
		}

		void operator=( const T& v ) //const
		{
			std::size_t			idx = m_slice.start();
			const std::size_t	len = m_slice.length();
			const std::size_t	end = idx + len;

			for ( ; idx < end; idx++ )
				m_vector[ idx ] = v;

			return;
		}

		virtual ~slice_vector_t(void) { return; }
};


template< class T >
inline fp_vector_t< T >
abs(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::abs(v[ idx ]);

	return tmp;
}


template< class T >
inline fp_vector_t< T >
acos(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::acos(v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
asin(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::asin(v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
atan(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::atan(v[ idx ]);


	return tmp;
}

template< class T >
inline fp_vector_t< T >
atan2(const fp_vector_t< T >& x, const fp_vector_t< T >& y)
{
	fp_vector_t< T >	tmp(x.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::atan2(x[ idx ], y[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
atan2(const fp_vector_t< T >& v, const T& c)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::atan2(v[ idx ], c);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
atan2(const T& c, const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::atan2(c, v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
cos(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::cos(v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
cosh(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::cosh(v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
exp(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::exp(v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
log(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::log(v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
log10(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::log10(v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
pow(const fp_vector_t< T >& x, const fp_vector_t< T >& y)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::pow(x[ idx ], y[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
pow(const fp_vector_t< T >& v, const T& c)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::pow(v[ idx ], c);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
pow(const T& c, const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::pow(c, v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
sin(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::sin(v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
sinh(const fp_vector_t< T >& v)
{
		fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
		std::size_t			siz(tmp.size());

		for ( std::size_t idx = 0; idx < siz; idx++ )
			tmp[ idx ] = std::sinh(v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
sqrt(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

		for ( std::size_t idx = 0; idx < siz; idx++ )
			tmp[ idx ] = std::sqrt(v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T > 
tan(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

		for ( std::size_t idx = 0; idx < siz; idx++ )
			tmp[ idx ] = std::tan(v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T > 
tanh(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::tanh(v[ idx ]);

	return tmp;
}

template< class T >
inline fp_vector_t< T >
square(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = ( v[ idx ] * v[ idx ] );

	return tmp;
}
	
template< class T >
inline fp_vector_t< T >
cube(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = ( v[ idx ] * v[ idx ] * v[ idx ] );

	return tmp;
}
	
template< class T >
inline fp_vector_t< T >
absSquare(const fp_vector_t< T >& v)
{
	return ::square(::abs(v));
}

template< class T >
inline fp_vector_t< T >
absCube(const fp_vector_t< T >& v)
{
	return ::cube(::abs(v));
}

template< class T >
inline T
average(const fp_vector_t< T >& v)
{
	return v.sum() / v.size();
}
	
template< class T >
inline T
rms(const fp_vector_t< T >& v)
{
	return ::sqrt(::average(::square(v)));
}

template< class T >
inline T
rss(const fp_vector_t< T >& v)
{
	return std::sqrt(v.sum());
}

template< class T >
inline T
variance(const fp_vector_t< T >& v)
{
	fp_vector_t< T >	tmp(v.size(), fp_vector_t< T >::no_init());
	std::size_t			siz(tmp.size());
	T					avg = ::average(v);

	tmp -= avg;

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = ( v[ idx ] - tmp[ idx ] ) * ( v[ idx ] - tmp[ idx ] );

	return ::average(tmp);
}

template< class T >
inline T
standardDeviation(const fp_vector_t< T >& v)
{
	return ::sqrt(::variance(v));
}

template< class T > 
inline T
absMax(const fp_vector_t< T >& v)
{
	return std::abs(v.max());
}

template< class T > 
inline T
absMin(const fp_vector_t< T >& v)
{
	return std::abs(v.min());
}

template< class T > 
inline T
absSum(const fp_vector_t< T >& v)
{
	fp_vector_t< T > tmp(::abs(v));
	return tmp.sum();
}

template< class T >
inline fp_vector_t< T >
operator-( const fp_vector_t< T >& lhs, const fp_vector_t< T >& rhs )
{
	fp_vector_t< T > tmp(lhs.size(), fp_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		if ( idx > rhs.size() )
			tmp[ idx ] = lhs[ idx ];
		else
			tmp[ idx ] = lhs[ idx ] - rhs[ idx ];

	return tmp;

}

template< class T >
inline fp_vector_t< T >
operator-( const fp_vector_t< T >& lhs, const T rhs )
{
	fp_vector_t< T > tmp(lhs.size(), fp_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
			tmp[ idx ] = lhs[ idx ] - rhs;

	return tmp;

}

template< class T >
inline fp_vector_t< T >
operator+( const fp_vector_t< T >& lhs, const fp_vector_t< T >& rhs )
{
	fp_vector_t< T > tmp(lhs.size(), fp_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		if ( idx > rhs.size() )
			tmp[ idx ] = lhs[ idx ];
		else
			tmp[ idx ] = lhs[ idx ] + rhs[ idx ];

	return tmp;

}

template< class T >
inline fp_vector_t< T >
operator+( const fp_vector_t< T >& lhs, const T rhs )
{
	fp_vector_t< T > tmp(lhs.size(), fp_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		tmp[ idx ] = lhs[ idx ] + rhs;

	return tmp;

}

template< class T >
inline fp_vector_t< T >
operator*( const fp_vector_t< T >& lhs, const fp_vector_t< T >& rhs )
{
	fp_vector_t< T > tmp(lhs.size(), fp_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		if ( idx > rhs.size() )
			tmp[ idx ] = lhs[ idx ];
		else
			tmp[ idx ] = lhs[ idx ] * rhs[ idx ];

	return tmp;

}

template< class T >
inline fp_vector_t< T >
operator*( const fp_vector_t< T >& lhs, const T rhs )
{
	fp_vector_t< T > tmp(lhs.size(), fp_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		tmp[ idx ] = lhs[ idx ] * rhs;

	return tmp;

}

template< class T >
inline fp_vector_t< T >
operator/( const fp_vector_t< T >& lhs, const fp_vector_t< T >& rhs )
{
	fp_vector_t< T > tmp(lhs.size(), fp_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		if ( idx > rhs.size() )
			tmp[ idx ] = lhs[ idx ];
		else
			tmp[ idx ] = lhs[ idx ] / rhs[ idx ];

	return tmp;

}

template< class T >
inline fp_vector_t< T >
operator/( const fp_vector_t< T >& lhs, const T rhs )
{
	fp_vector_t< T > tmp(lhs.size(), fp_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		tmp[ idx ] = lhs[ idx ] / rhs;

	return tmp;

}

template< class T >
inline T
mse(const ::fp_vector_t< T >& v0, const ::fp_vector_t< T >& v1)
{
	T					val(0.0);
	::fp_vector_t< T >	diff(( v0.size() > v1.size() ? v0.size() : v1.size() ));

	for ( std::size_t idx = 0; idx < diff.size(); idx++ ) {
		if ( idx >= v0.size() )
			diff[ idx ] = v1[ idx ];
		else if ( idx >= v1.size() )
			diff[ idx ] = v0[ idx ];
		else
			diff[ idx ] = v0[ idx ] - v1[ idx ];
	}

	diff = ::square(diff);
	val = diff.sum();

	val /= diff.size();
	return val;
}
#endif