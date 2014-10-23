#ifndef HAVE_COMPLEX_VECTOR_HPP
#define HAVE_COMPLEX_VECTOR_HPP

#include <type_traits>
#include <complex>
#include <cstdint>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <memory>
#include <mutex>

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


template <class T > class complex_slice_vector_t;

class complex_slice_t
{
	private:
		std::size_t m_start;
		std::size_t m_length;

	protected:
	public:
		complex_slice_t(void) : m_start(0), m_length(0) { return; }
		complex_slice_t(std::size_t start, std::size_t length) : m_start(start), m_length(length) { return; }
		virtual ~complex_slice_t(void) { m_start = 0; m_length = 0; return; }

		std::size_t start(void) const { return m_start; }
		std::size_t length(void) const { return m_length; }

};

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
struct complex_vector_base_t
{
	std::complex< T >* RESTRICT m_data;
	std::size_t					m_size;
	std::mutex					m_mutex;

	complex_vector_base_t(void) : m_data(nullptr), m_size(0) { return; }
	complex_vector_base_t(std::size_t s) : m_data(nullptr), m_size(s)
	{
		allocate(s);
		return;
	}

	~complex_vector_base_t(void)
	{
		deallocate();
		return;
	}

	void
	allocate(std::size_t n)
	{
		std::lock_guard< std::mutex > l(m_mutex);

		if ( SIZE_MAX / sizeof(std::complex< T >) < n )
			throw std::overflow_error("Multiplicative integer overflow condition encountered.");

		if ( 0 != n ) {
			m_data = static_cast< std::complex< T > * >( ::malloc(n * sizeof(std::complex< T >)) );

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


		if ( SIZE_MAX - m_size / sizeof(std::complex< T >) < n )
			throw std::overflow_error("Multiplicative integer overflow condition encountered.");

		//s += m_size;

		if ( 0 != n ) {

			m_data = static_cast< std::complex< T > * >( ::realloc(m_data, n * sizeof(std::complex< T >)) );

			if ( nullptr == m_data )
				throw std::bad_alloc();

			m_size = n;
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
	memcpy_sec(std::complex< T >* RESTRICT dst, std::size_t dlen, std::complex< T >* RESTRICT src, std::size_t slen)
	{

		if ( SIZE_MAX / sizeof(std::complex< T >) < slen || SIZE_MAX / sizeof(std::complex< T >) < dlen )
			throw std::overflow_error("Multiplicative integer overflow condition encountered.");

#ifndef _MSC_BUILD
		if ( slen > dlen )
			throw std::bad_exception("Your C runtime presumably does not support memcpy_s(), "	
										"so we emulated it. You tried to memcpy too much data, "
										"sorry it didn't work out.");

		std::memcpy(dst, src, slen*sizeof(T));
#else
		::memcpy_s(dst, dlen, src, slen);
#endif
		return;
	}

};

template< typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true >
class complex_vector_t : private complex_vector_base_t< T >
{
	private:
	protected:
	public:
		struct no_init { };

		complex_vector_t(std::size_t c, no_init) : complex_vector_base_t< T >(c) { return; }

		complex_vector_t(void) : complex_vector_base_t< T >() { return; }

		explicit complex_vector_t(std::size_t c) : complex_vector_base_t< T >(c)
		{
			::memset(m_data, 0, m_size*sizeof(std::complex< T >));
			return;

		}

		complex_vector_t(const std::complex< T >& v, std::size_t c) : complex_vector_base_t< T >(c)
		{
			::memset(m_data, v, m_size*sizeof(std::complex< T >));
			return;
		}

		complex_vector_t(const std::complex< T >* v, size_t c) : complex_vector_base_t< T >(c)
		{
			if ( nullptr == v )
				throw std::invalid_argument("nullptr exception");

			memcpy_sec(m_data, m_size*sizeof(std::complex< T >), v, c*sizeof(std::complex< T >));
			return;
		}

		complex_vector_t(const complex_vector_t< T >& o) : complex_vector_base_t< T >(o.m_size)
		{

			memcpy_sec(m_data, m_size*sizeof(std::complex< T >), o.m_data, o.m_size*sizeof(std::complex< T >));
			return;
		}

		complex_vector_t(complex_vector_t< T >&& o) : complex_vector_base_t< T >(o.m_size)
		{

			memcpy_sec(m_data, m_size*sizeof(std::complex< T >), o.m_data, o.m_size*sizeof(std::complex< T >));
			return;
		}

		complex_vector_t(std::initializer_list< std::complex< T > > i) : complex_vector_base_t< T >(i.size())
		{
			std::size_t idx = 0;
			auto		ibeg = i.begin();
			auto		iend = i.end();

			for ( auto itr = ibeg; itr != iend; itr++, idx++ )
				m_data[ idx ] = *itr;

			return;
		}

		complex_vector_t(const complex_slice_vector_t< T >& sv) : complex_vector_base_t< T >(sv.m_slice.length())
		{
			*this = sv;
			return;
		}

		virtual ~complex_vector_t(void)
		{
			std::allocator< std::complex< T > > alloc;

			for ( std::size_t idx = 0; idx < m_size*sizeof(std::complex< T >); idx += sizeof(std::complex< T >) )
				alloc.destroy(&m_data[ idx ]);
			
				//std::allocator< std::complex< T > >::destroy( &m_data[ idx ]);

			return;
		}

		complex_vector_t< T >&
		operator=( const complex_vector_t< T >& o )
		{
			if ( this != &o ) {
				if ( m_size != o.size() )
					resize(o.size());

				memcpy_sec(m_data, m_size*sizeof(std::complex< T >), o.m_data, o.m_size*sizeof(std::complex< T >));
			}

			return *this;
		}

		complex_vector_t< T >&
		operator=( complex_vector_t< T >&& o )
		{
			if ( this != &o ) {
				if ( m_size != o.size() )
					resize(o.size());

				memcpy_sec(m_data, m_size*sizeof(std::complex< T >), o.m_data, o.m_size*sizeof(std::complex< T >));
			}
			return *this;
		}

		complex_vector_t<T>&
		operator=( const std::complex< T >& v )
		{
			if ( m_size < sizeof(std::complex< T >) )
				resize(sizeof(std::complex< T >));

			::memset(m_data, m_size*sizeof(std::complex< T >), v);

			return *this;
		}

		complex_vector_t< T >&
		operator=( const complex_slice_vector_t< T >& o )
		{
			std::size_t			idx = o.m_slice.start();
			const std::size_t	len = o.m_slice.length();
			const std::size_t	end = idx + len;

			if ( m_size != o.size() )
				resize(o.size());

			for ( std::size_t cnt = 0; idx < end; idx++, cnt++ )
				m_data[ cnt ] = o.m_vector[ idx ];

			//memcpy_sec(m_data, m_size, &o.m_vector.m_data[ o.m_slice.start() ], o.m_slice.length());
			return *this;
		}


		complex_vector_t< T >&
		operator=( std::initializer_list< std::complex< T > > i )
		{
			if ( m_size < i.size() * sizeof(std::complex< T >) )
				resize(i.size());

			std::copy(i.begin(), i.end(), m_data);
			return *this;
		}

		const std::complex< T >&
		operator[](std::size_t pos) const
		{
			return m_data[ pos ];
		}

		std::complex< T >&
		operator[](std::size_t pos)
		{
			return m_data[ pos ];
		}

		inline complex_slice_vector_t< T >
		operator[](complex_slice_t s)
		{
			if ( s.start() + s.length() >= size() )
				throw std::range_error("The range specified by the slice is longer than the vector.");

			if ( SIZE_MAX - s.start() < s.length() )
				throw std::overflow_error("Additive overflow with slice start+length parameters.");


			return complex_slice_vector_t< T >(s, *this);
		}

		complex_vector_t< T >
		operator[](complex_slice_t s) const
		{
			complex_vector_t< T >	tmp(s.length(), no_init());

			memcpy_sec(tmp.m_data, tmp.m_size*sizeof(std::complex< T >), m_data[ s.start() ], s.length()*sizeof(std::complex< T >));
			return tmp;
		}


		complex_vector_t< T >
		operator+=( const complex_vector_t< T >& v )
		{
			complex_vector_t< T >	tmp(0, no_init());
			std::size_t				len = m_size + v.size();
			std::size_t				asz = v.size();
			std::size_t				idx = 0;

			if ( SIZE_MAX - m_size > v.size() )
				throw std::overflow_error("Additive integer overflow condition encountered.");

			tmp.resize(len);

			for ( idx = 0; idx < m_size; idx++ )
				tmp[ idx ] = ( *this )[ idx ];

			for ( ; idx < m_size + asz; idx++ )
				tmp[ idx ] = v[ idx ];

			return tmp;
		}

		complex_vector_t< T >&
		operator+=( const std::complex< T >& v )
		{
			for ( std::size_t idx = 0; idx < m_size; idx++ )
				( *this )[ idx ] += v;

			return *this;
		}

		complex_vector_t< T >&
		operator-=( const std::complex< T >& v )
		{
			for ( std::size_t idx = 0; idx < m_size; idx++ )
				( *this )[ idx ] -= v;

			return *this;
		}

		complex_vector_t< T >&
		operator*=( const std::complex< T >& v )
		{
			for ( std::size_t idx = 0; idx < m_size; idx++ )
				( *this )[ idx ] *= v;

			return *this;
		}

		complex_vector_t< T >&
		operator/=( const std::complex< T >& v )
		{
			for ( std::size_t idx = 0; idx < m_size; idx++ )
				( *this )[ idx ] /= v;

			return *this;
		}

		complex_vector_t< T >&
		operator%=( const std::complex< T >& v )
		{
			for ( std::size_t idx = 0; idx < m_size; idx++ )
				( *this )[ idx ] = ( ( *this )[ idx ] % v );

			return *this;
		}

		std::size_t size(void) const { return m_size; }

		void
		resize(std::size_t c, std::complex< T > v = std::complex< T >(0.0))
		{
			std::allocator< std::complex< T > > alloc;

			for ( std::size_t idx = 0; idx < m_size*sizeof(std::complex< T >); idx += sizeof(std::complex< T >) )
				alloc.destroy(&m_data[ idx ]);

			this->complex_vector_t< T >::deallocate();
			this->complex_vector_t< T >::allocate(c);

			//memcpy_sec(m_data, m_size, ( T const[ m_size ] ) { 0 }, m_size);
			::memset(m_data, 0, m_size*sizeof(std::complex< T >));

			return;
		}

		complex_vector_t< T >
		apply(std::complex< T > f(std::complex< T >)) const
		{
			complex_vector_t< T > r = complex_vector_t< T >(*this);
			std::size_t midx = r.size();

			for ( std::size_t idx = 0; idx < midx; idx++ )
				r[ idx ] = f(r[ idx ]);

			return r;
		}

		complex_vector_t< T >
		apply(std::complex< T > f(const std::complex< T >&)) const
		{
			complex_vector_t< T > r = complex_vector_t< T >(*this);
			std::size_t midx = r.size();

			for ( std::size_t idx = 0; idx < midx; idx++ )
				r[ idx ] = f(r[ idx ]);

			return r;
		}

		std::vector< std::complex< T > >
		toStdVector(void)
		{
			std::vector< std::complex< T > > ret;

			for ( std::size_t idx = 0; idx < m_size; idx++ )
				ret.push_back(m_data[ idx ]);

			return ret;
		}

		fp_vector_t< T >
		real(void)
		{
			fp_vector_t< T > ret(m_size, fp_vector_t< T >::no_init());

			for ( std::size_t idx = 0; idx < m_size; idx++ )
				ret[ idx ] = m_data[ idx ].real();

			return ret;
		}

		fp_vector_t< T >
		imag(void)
		{
			fp_vector_t< T > ret(m_size, fp_vector_t< T >::no_init());

			for ( std::size_t idx = 0; idx < m_size; idx++ )
				ret[ idx ] = m_data[ idx ].imag();

			return ret;
		}

		void
		grow(std::size_t n, T v =  T(0.0))
		{
			std::size_t		max = m_size;

			if ( SIZE_MAX - m_size < n )
				throw std::overflow_error("Additive integer overflow encountered");

			reallocate(n + m_size);

			::memset(&m_data[ max ], v, n * sizeof(std::complex< T >));

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
		concat(const complex_vector_t< T >& o)
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
		concat(const std::vector< std::complex< T > >& o)
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
		push_back(const std::complex< T >& v)
		{
			std::size_t idx = m_size;

			grow(1);
			m_data[ idx ] = v;

			return;
		}

		void
		push_back(const complex_vector_t< T >& o)
		{
			concat(o);
			return;
		}

		void
		push_back(const std::vector< std::complex< T > >& o)
		{
			concat(o);
			return;
		}


		void
		push_back(T r, T i)
		{
			std::complex< T > tmp(r, i);
			push_back(tmp);
			return;
		}

		T 
		sum(void)
		{
			T val = 0.0;

			for ( std::size_t idx = 0; idx < m_size; idx++ )
				val += m_data[ idx ].real() + m_data[ idx ].imag();

			return val;
		}
};

template< class T >
class complex_slice_vector_t
{
	private:
		friend class complex_vector_t < T > ;

		complex_slice_t			m_slice;
		complex_vector_t< T >&	m_vector;

		complex_slice_vector_t(void);
		complex_slice_vector_t(const complex_slice_t& s, complex_vector_t< T >& v) : m_slice(s), m_vector(v) { return; }

	protected:
	public:
		complex_slice_vector_t slice(void) { return m_slice; }
		complex_slice_vector_t< T >& vector(void) { return m_vector; }

		void
		operator=( const complex_vector_t< T >& o ) //const
		{
			std::size_t			idx = m_slice.start();
			const std::size_t	len = m_slice.length();
			const std::size_t	end = idx + len;

			for ( ; idx < end; idx++ )
				m_vector[ idx ] = o[ idx ];

			return;
		}

		void operator=( const std::complex< T >& v ) //const
		{
			std::size_t			idx = m_slice.start();
			const std::size_t	len = m_slice.length();
			const std::size_t	end = idx + len;

			for ( ; idx < end; idx++ )
				m_vector[ idx ] = v;

			return;
		}

		virtual ~complex_slice_vector_t(void) { return; }
};

template< typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true >
inline T
squareAmplitude(const std::complex< T >& v)
{
	return ( v.real()*v.real() ) + ( v.imag()*v.imag() );
}

template< typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true >
inline T
magnitude(const std::complex< T >& v)
{
	return std::abs(squareAmplitude(v));
}

template< class T >
inline complex_vector_t< T >
abs(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::abs(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
acos(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::acos(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
asin(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::asin(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
atan(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::atan(v[ idx ]);


	return tmp;
}

template< class T >
inline complex_vector_t< T >
atan2(const complex_vector_t< T >& x, const complex_vector_t< T >& y)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::atan2(x[ idx ], y[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
atan2(const complex_vector_t< T >& v, const T& c)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::atan2(v[ idx ], c);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
atan2(const T& c, const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::atan2(c, v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
cos(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::cos(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
cosh(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::cosh(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
exp(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::exp(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
log(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::log(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
log10(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::log10(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
pow(const complex_vector_t< T >& x, const complex_vector_t< T >& y)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::pow(x[ idx ], y[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
pow(const complex_vector_t< T >& v, const T& c)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::pow(v[ idx ], c);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
pow(const T& c, const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::pow(c, v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
sin(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::sin(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
sinh(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::sinh(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
sqrt(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::sqrt(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
tan(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::tan(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
tanh(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = std::tanh(v[ idx ]);

	return tmp;
}

template< class T >
inline complex_vector_t< T >
square(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = ( v[ idx ] * v[ idx ] );

	return tmp;
}

template< class T >
inline complex_vector_t< T >
cube(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = ( v[ idx ] * v[ idx ] * v[ idx ] );

	return tmp;
}

template< class T >
inline complex_vector_t< T >
absSquare(const complex_vector_t< T >& v)
{
	return ::square(::abs(v));
}

template< class T >
inline complex_vector_t< T >
absCube(const complex_vector_t< T >& v)
{
	return ::cube(::abs(v));
}

template< class T >
inline T
average(complex_vector_t< T >& v)
{
	T val = v.sum();

	return val / v.size();
}

template< class T >
inline T
rms(const complex_vector_t< T >& v)
{
	return ::sqrt(::average(::square(v)));
}

template< class T >
inline T
rss(const complex_vector_t< T >& v)
{
	return std::sqrt(v.sum());
}

template< class T >
inline T
variance(const complex_vector_t< T >& v)
{
	complex_vector_t< T >	tmp(v.size(), complex_vector_t< T >::no_init());
	std::size_t				siz(tmp.size());
	T						avg = ::average(v);

	tmp -= avg;

	for ( std::size_t idx = 0; idx < siz; idx++ )
		tmp[ idx ] = ( v[ idx ] - tmp[ idx ] ) * ( v[ idx ] - tmp[ idx ] );

	return ::average(tmp);
}

template< class T >
inline T
standardDeviation(const complex_vector_t< T >& v)
{
	return ::sqrt(::variance(v));
}

template< class T >
inline T
absMax(const complex_vector_t< T >& v)
{
	return std::abs(v.max());
}

template< class T >
inline T
absMin(const complex_vector_t< T >& v)
{
	return std::abs(v.min());
}

template< class T >
inline T
absSum(const complex_vector_t< T >& v)
{
	complex_vector_t< T > tmp(::abs(v));
	return tmp.sum();
}

template< class T >
inline complex_vector_t< T >
operator-( const complex_vector_t< T >& lhs, const complex_vector_t< T >& rhs )
{
	complex_vector_t< T > tmp(lhs.size(), complex_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		if ( idx > rhs.size() )
			tmp[ idx ] = lhs[ idx ];
		else
			tmp[ idx ] = lhs[ idx ] - rhs[ idx ];

	return tmp;

}

template< class T >
inline complex_vector_t< T >
operator-( const complex_vector_t< T >& lhs, const T rhs )
{
	complex_vector_t< T > tmp(lhs.size(), complex_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		tmp[ idx ] = lhs[ idx ] - rhs;

	return tmp;

}

template< class T >
inline complex_vector_t< T >
operator+( const complex_vector_t< T >& lhs, const complex_vector_t< T >& rhs )
{
	complex_vector_t< T > tmp(lhs.size(), complex_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		if ( idx > rhs.size() )
			tmp[ idx ] = lhs[ idx ];
		else
			tmp[ idx ] = lhs[ idx ] + rhs[ idx ];

	return tmp;

}

template< class T >
inline complex_vector_t< T >
operator+( const complex_vector_t< T >& lhs, const T rhs )
{
	complex_vector_t< T > tmp(lhs.size(), complex_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		tmp[ idx ] = lhs[ idx ] + rhs;

	return tmp;

}

template< class T >
inline complex_vector_t< T >
operator*( const complex_vector_t< T >& lhs, const complex_vector_t< T >& rhs )
{
	complex_vector_t< T > tmp(lhs.size(), complex_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		if ( idx > rhs.size() )
			tmp[ idx ] = lhs[ idx ];
		else
			tmp[ idx ] = lhs[ idx ] * rhs[ idx ];

	return tmp;

}

template< class T >
inline complex_vector_t< T >
operator*( const complex_vector_t< T >& lhs, const T rhs )
{
	complex_vector_t< T > tmp(lhs.size(), complex_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		tmp[ idx ] = lhs[ idx ] * rhs;

	return tmp;

}

template< class T >
inline complex_vector_t< T >
operator/( const complex_vector_t< T >& lhs, const complex_vector_t< T >& rhs )
{
	complex_vector_t< T > tmp(lhs.size(), complex_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		if ( idx > rhs.size() )
			tmp[ idx ] = lhs[ idx ];
		else
			tmp[ idx ] = lhs[ idx ] / rhs[ idx ];

	return tmp;

}

template< class T >
inline complex_vector_t< T >
operator/( const complex_vector_t< T >& lhs, const T rhs )
{
	complex_vector_t< T > tmp(lhs.size(), complex_vector_t< T >::no_init());

	for ( std::size_t idx = 0; idx < lhs.size(); idx++ )
		tmp[ idx ] = lhs[ idx ] / rhs;

	return tmp;

}

template< class T >
inline T
mse(const ::complex_vector_t< T >& v0, const ::complex_vector_t< T >& v1)
{
	T					val(0.0);
	::complex_vector_t< T >	diff(( v0.size() > v1.size() ? v0.size() : v1.size() ));

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