#ifndef HAVE_FFTFACTORY_HPP
#define HAVE_FFTFACTORY_HPP

#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <mutex>

#include <fftw3.h>

#include "signal.hpp"
#include "complex_vector.hpp"

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
class fft_t
{
	private:
	protected:
		std::mutex* m_mutex;

		std::size_t
		smallprimes(std::size_t x)
		{
			std::size_t p[ 3 ] = { 2, 3, 5 };

			for ( std::size_t idx = 0; idx < 3; idx++ )
				while ( x % p[ idx ] == 0 )
					x /= p[ idx ];

			return x;
		}


		std::size_t
		padded_size(std::size_t x)
		{
			while ( 1 != smallprimes(x) )
				x++;

			return x;
		}

	public:
		fft_t(void) : m_mutex(nullptr) { return; }
		fft_t(std::mutex* m) : m_mutex(m) { return; }
		virtual ~fft_t(void) { return; }
		virtual ::complex_vector_t< T > fft(signal_t< T >&) = 0;
		virtual signal_t< T > ifft(::complex_vector_t< T >&) = 0;
};

class dfft_t : public fft_t< double >
{

	private:
	protected:
	public:
		dfft_t(void) { return; }
		dfft_t(std::mutex* m) : fft_t(m) { return; }
		virtual ~dfft_t(void) { return; }

		complex_vector_t< double >
		fft(signal_t< double >& in)
		{
			fftw_plan							plan	= nullptr;
			const size_t						n		= in.size();
			const size_t						padded	= n > 256 ? padded_size(n) : n;
			::complex_vector_t< double >		ocv(padded/2+1);

			if ( 0 > in.size())
				throw std::invalid_argument("Invalid parameter passed; empty vector");

			if ( padded != n )
				in.grow(padded);

			if ( nullptr != m_mutex )
				m_mutex->lock();

			plan = fftw_plan_dft_r2c_1d(padded, &in[ 0 ], (fftw_complex*)&ocv[ 0 ], FFTW_ESTIMATE);

			if ( nullptr != m_mutex )
				m_mutex->unlock();

			if ( nullptr == plan )
				throw std::runtime_error("fftwf_plan_dft_r2c_1d() returned null. WE DONT HAVE A PLAN!");

			fftw_execute(plan);

			if ( nullptr != m_mutex )
				m_mutex->lock();

			fftw_destroy_plan(plan);

			if ( nullptr != m_mutex )
				m_mutex->unlock();

			in.shrink(n);
			return ocv;
		}

		signal_t< double >
		ifft(complex_vector_t< double >& in)
		{
			const size_t						n		= ( in.size() - 1 ) * 2;
			const size_t						padded	= n > 256 ? padded_size(n) : n;
			signal_t< double >					ocv;
			::complex_vector_t< double >		inv;

			if ( 1 > in.size() )
				throw std::invalid_argument("Invalid parameter passed; vector with only 1 element.");

			inv.resize(padded / 2 + 1);
			ocv.resize(padded);

			for ( std::size_t idx = 0; idx < in.size(); idx++ )
				inv[ idx ] = in[ idx ];

			// note: fftw3 destroys the input array for c2r transform

			if ( nullptr != m_mutex )
				m_mutex->lock();

			fftw_plan plan = fftw_plan_dft_c2r_1d(padded, (fftw_complex*)&inv[ 0 ], &ocv[ 0 ], FFTW_ESTIMATE);

			if ( nullptr != m_mutex )
				m_mutex->unlock();

			if ( nullptr == plan )
				throw std::runtime_error("fftwf_plan_dft_r2c_1d() returned null. WE DONT HAVE A PLAN!");

			fftw_execute(plan);
			fftw_destroy_plan(plan);

			//in.shrink(n / 2 + 1);
			return ocv;
		}
};

class ffft_t : public fft_t< float >
{

	private:
	protected:
	public:
		ffft_t(void) { return; }
		ffft_t(std::mutex* m) : fft_t(m) { return; }

		complex_vector_t< float >
		fft(signal_t< float >& in)
		{
			fftwf_plan							plan	= nullptr;
			const size_t						n		= in.size();
			const size_t						padded	= n > 256 ? padded_size(n) : n;
			::complex_vector_t< float >			ocv(padded / 2 + 1);

			if ( 0 > in.size() )
				throw std::invalid_argument("Invalid parameter passed; empty vector");

			if ( padded != n )
				in.grow(padded);

			if ( nullptr != m_mutex )
				m_mutex->lock();

			plan = fftwf_plan_dft_r2c_1d(padded, &in[ 0 ], (fftwf_complex*)&ocv[ 0 ], FFTW_ESTIMATE);

			if ( nullptr != m_mutex )
				m_mutex->unlock();

			if ( nullptr == plan )
				throw std::runtime_error("fftwf_plan_dft_r2c_1d() returned null. WE DONT HAVE A PLAN!");

			fftwf_execute(plan);

			if ( nullptr != m_mutex )
				m_mutex->lock();

			fftwf_destroy_plan(plan);

			if ( nullptr != m_mutex )
				m_mutex->unlock();

			in.shrink(n);
			return ocv;
		}

		signal_t< float >
		ifft(complex_vector_t< float >& in)
		{
			const size_t						n = ( in.size() - 1 ) * 2;
			const size_t						padded = n > 256 ? padded_size(n) : n;
			signal_t< float >					ocv;
			::complex_vector_t< float >			inv;

			if ( 1 < in.size() )
				throw std::invalid_argument("Invalid parameter passed; vector with only 1 element.");

			inv.resize(padded / 2 + 1);
			ocv.resize(padded);

			for ( std::size_t idx = 0; idx < in.size(); idx++ )
				inv[ idx ] = in[ idx ];

			if ( nullptr != m_mutex )
				m_mutex->lock();

			fftwf_plan plan = fftwf_plan_dft_c2r_1d(padded, (fftwf_complex*)&inv[ 0 ], &ocv[ 0 ], FFTW_ESTIMATE);

			if ( nullptr != m_mutex )
				m_mutex->unlock();

			if ( nullptr == plan )
				throw std::runtime_error("fftwf_plan_dft_r2c_1d() returned null. WE DONT HAVE A PLAN!");

			fftwf_execute(plan);
			fftwf_destroy_plan(plan);

			return ocv;
		}
};

class lfft_t : public fft_t< long double >
{

	private:
	protected:
	public:
		lfft_t(void) : fft_t(nullptr) { return; }
		lfft_t(std::mutex* m) : fft_t(m) { return; }

		complex_vector_t< long double >
		fft(signal_t< long double >& in)
		{

			fftwl_plan							plan = nullptr;
			const size_t						n = in.size();
			const size_t						padded = n > 256 ? padded_size(n) : n;
			::complex_vector_t< long double >	ocv(padded / 2 + 1);

			if ( 0 > in.size() )
				throw std::invalid_argument("Invalid parameter passed; empty vector");

			if ( padded != n )
				in.grow(padded);

			if ( nullptr != m_mutex )
				m_mutex->lock();

			plan = fftwl_plan_dft_r2c_1d(padded, &in[ 0 ], (fftwl_complex*)&ocv[ 0 ], FFTW_ESTIMATE);

			if ( nullptr != m_mutex )
				m_mutex->unlock();

			if ( nullptr == plan )
				throw std::runtime_error("fftwf_plan_dft_r2c_1d() returned null. WE DONT HAVE A PLAN!");

			fftwl_execute(plan);

			if ( nullptr != m_mutex )
				m_mutex->lock();

			fftwl_destroy_plan(plan);

			if ( nullptr != m_mutex )
				m_mutex->unlock();

			in.shrink(n);
			return ocv;
		}

		signal_t< long double >
		ifft(complex_vector_t< long double >& in)
		{
			fftwl_plan							plan	= nullptr;
			const size_t						n		= ( in.size() - 1 ) * 2;
			const size_t						padded	= n > 256 ? padded_size(n) : n;
			signal_t< long double >				ocv;
			::complex_vector_t< long double >	inv;

			if ( 1 < in.size() )
				throw std::invalid_argument("Invalid parameter passed; vector with only 1 element.");

			inv.resize(padded / 2 + 1);
			ocv.resize(padded);

			for ( std::size_t idx = 0; idx < in.size(); idx++ )
				inv[ idx ] = in[ idx ];

			if ( nullptr != m_mutex )
				m_mutex->lock();

			plan = fftwl_plan_dft_c2r_1d(padded, (fftwl_complex*)&inv[ 0 ], &ocv[ 0 ], FFTW_ESTIMATE);

			if ( nullptr != m_mutex )
				m_mutex->unlock();

			if ( nullptr == plan )
				throw std::runtime_error("fftwf_plan_dft_r2c_1d() returned null. WE DONT HAVE A PLAN!");

			fftwl_execute(plan);
			fftwl_destroy_plan(plan);

			return ocv;
		}
};

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
class fft_factory_t
{
	private:
	protected:
	public:
		fft_factory_t(void) { return; }

		virtual ~fft_factory_t(void) { return; }
		static fft_t< T >* getFFT(std::mutex* m = nullptr)
		{
			if ( typeid( T ) == typeid( double ) )
				return dynamic_cast< fft_t< T >* >( new dfft_t(m) );

			else if ( typeid( T ) == typeid( float ) )
				return dynamic_cast< fft_t< T >* >( new ffft_t(m) );

			else if ( typeid( T ) == typeid( long double ) )
				return dynamic_cast< fft_t< T >* >( new lfft_t(m) );

			throw std::invalid_argument("Unpossible");
			return nullptr;
		}
};

#endif
