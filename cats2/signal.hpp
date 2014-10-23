#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <QObject>

#define _USE_MATH_DEFINES

#include <math.h>
#include <cstdint>
#include <valarray>
#include <vector>
#include <limits>
#include <type_traits>
#include <complex>
#include <exception>

#include <iostream>

#include "window.hpp"
#include "complex_vector.hpp"

/*#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif*/

#include <DspFilters/Dsp.h>
#include "fp_vector.hpp"


template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool >::type = true >
class signal_t : public ::fp_vector_t < T >
{
	private:
		std::size_t			m_sample;
		//::fp_vector_t< T >	m_data;

	protected:
	public:
		signal_t(void) : m_sample(0) { return; }

		signal_t(std::vector< T >& v, std::size_t sampleRate) : m_sample(sampleRate)
		{
			resize(v.size());

			for ( std::size_t idx = 0; idx < v.size(); idx++ )
				( *this )[ idx ] = v[ idx ];

			return;
		}


		signal_t(::slice_vector_t< T >& s, std::size_t sampleRate) : m_sample(sampleRate)
		{
			std::size_t			idx = s.slice().start();
			const std::size_t	len = s.slice().length();
			const std::size_t	end = idx + len;

			resize(s.slice().length());

			for ( std::size_t cnt = 0; idx < end; idx++, cnt++ )
				( *this )[ cnt ] = s.vector()[ idx ];

			return;
		}

		void sample_rate(std::size_t sr) { m_sample = sr; return; }
		std::size_t sample_rate(void) const { return m_sample; }

		signal_t< T, true >
		normalizeOffset(void)
		{
			const std::size_t	len				= size();
			T					su				= sum();
			T					ma				= max();
			T					mi				= min();
			T					off				= static_cast<T> ( -su / ma );
			T					ratio			= std::pow(10.0, -1.0) / 20.0;
			T					extent			= ( std::fabs(ma) > std::fabs(mi) ? std::fabs(ma) : std::fabs(mi) );
			T					mult			= ( extent > 0 ? ratio / extent : 1.0 );


			if ( 0 == ma )
				return *this;

			for ( std::size_t idx = 0; idx < len; idx++ )
				( *this )[ idx ] = ( ( *this )[ idx ] * mult );

			return *this;
		}

		signal_t< T, true >
		normalize(void)
		{
			T ma		= max();

			if ( 0 == ma )
				return *this;

			for ( std::size_t idx = 0; idx < size(); idx++ ) 
				( *this )[ idx ] = ( ( *this )[ idx ] / ma );

			return *this;
		}

		signal_t< T, true >
		absolute_normalize(void)
		{
			if ( 0.0 == max() )
				return signal_t< T >(); // ?? THROW ?

			return apply([](const T& v)->T { return std::abs(v) / max(); })
		}

		signal_t< T, true >
		log10scale(void)
		{
			return ::log10(1 + 9 * *this);
		}

		signal_t< T, true >
		log10scale_inv(void)
		{
			return ::pow(10, *this) - 1 / 9;
		}

		signal_t< T, true > dB(void)
		{
			return 20.0 * ::log10(*this);
		}

		std::vector< signal_t< T > >
		get_enveloped_signals(T minv = 0.01, std::size_t attackMs = 10, std::size_t releaseMs = 250)
		{
			std::vector< signal_t< T > >	ret;
			std::size_t						start = 0;
			std::size_t						end = size();
			T								attack = ( attackMs*m_sample / 1000 );
			T								release = ( releaseMs*m_sample / 1000 );
			T								val = 0.0;
			bool							flag = false;

			for ( std::size_t idx = 0; idx < size(); idx++ ) {
				val = std::abs(( *this )[ idx ]);

				if ( val < minv ) {
					flag = false;
					continue;
				} else {
					if ( false == flag ) {
						flag = true;
						start = idx;
					}

					if ( idx >= ( start + attack ) )
						break;
				}

				flag = false;

				for ( std::size_t eidx = start + 1; eidx < size(); eidx++ ) {
					val = std::abs(( *this )[ eidx ]);

					if ( val > minv ) {
						flag = false;
						continue;
					} else {
						if ( false == flag ) {
							flag = true;
							end = eidx;
						}

						if ( eidx >= ( end + release ) )
							break;
					}
				}

				if ( start == end )
					continue;
	
				if ( end == size() ) {
					end--;
					idx = end;
				} else
					idx = end;

				ret.push_back(signal_t< T >(( *this )[ ::fp_slice_t(start, end - start) ], m_sample));
			}

			return ret;
		}

		std::vector< T >
		toStdVector(void) const
		{
			std::vector< T > v;

			for ( std::size_t idx = 0; idx < size(); idx++ )
				v.push_back(( *this )[ idx ]);

			return v;
		}
};

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
class signal_frames_t
{
	private:
		std::size_t						m_fsize;
		std::vector< signal_t< T > >	m_data;
		std::size_t						m_sample;
		float							m_overlap;

	protected:
		
		inline signal_t< T > 
		pad(signal_t< T > sig, std::size_t len)
		{
			signal_t< T > frames; // ( sig.size() + len );

			if ( SIZE_MAX - len < sig.size() )
				throw std::overflow_error("Additive integer overflow error!");

			frames.resize(sig.size() + len);
			frames.sample_rate(sig.sample_rate());

			for ( std::size_t idx = 0; idx < sig.size(); idx++ )
				frames[ idx ] = sig[ idx ];

			for ( std::size_t idx = sig.size(); idx < sig.size() + len; idx++ )
				frames[ idx ] = 0.0;

			return frames;
		}

		void init(signal_t< T >& sig, std::size_t durationMs, float overlap)
		{
			std::size_t		durFrameCount = ( durationMs*sig.sample_rate() / 1000 );
			std::size_t		overlapFrameCount = ( overlap * durFrameCount );
			std::size_t		remainder = ( sig.size() % durFrameCount );
			std::size_t		padSize = ( durFrameCount - remainder );
			std::size_t		stepSize = durFrameCount - overlapFrameCount;
			signal_t< T >	frames = pad(sig, padSize); //signal_t< T>(sig.size() + padSize));

			if ( durFrameCount > sig.size() )
				throw std::invalid_argument("Duration exceeds size of the signal");

			if ( overlapFrameCount > durFrameCount )
				throw std::invalid_argument("Overlap size is greater than 100%");

			m_fsize = durFrameCount;

			//m_data.push_back(signal_t< T >(frames[ ::fp_slice_t(0, durFrameCount) ], sig.sample_rate()));

			for ( std::size_t idx = /*durFrameCount - overlapFrameCount*/ 0; idx < frames.size() - durFrameCount; idx += stepSize )
				m_data.push_back(signal_t< T >(frames[ ::fp_slice_t(idx, durFrameCount) ], sig.sample_rate()));
		}

	public:
		signal_frames_t(void) : m_fsize(0), m_data(0), m_sample(0), m_overlap(0) { return;  }
		
		signal_frames_t(signal_t< T >& sig, std::size_t durationMs, float overlap = 0.8) 
			:	m_fsize(0), 
				m_sample(sig.sample_rate()), 
				m_overlap(overlap)
		{

			init(sig, durationMs, overlap);
			return;

		}

		virtual ~signal_frames_t(void)
		{
			for ( auto& itr : m_data )
				itr.resize(0);

			m_data.clear();

			return;
		}

		std::size_t
		count(void)
		{
			return m_data.size();
		}

		std::size_t
		frameSize(void)
		{
			return m_fsize;
		}

		void 
		putFrame(signal_t< T >& sig, std::size_t durationMs,  float overlap = 0.8) 
		{
			if ( 0 != m_data.size() ) {
				if ( sig.size() != m_fsize && sig.size() > m_fsize )
					throw std::invalid_argument("Invalid signal specified (overlong size)");
			} else {
				m_fsize		= sig.size();
				m_sample	= sig.sample_rate();
				m_overlap	= overlap;

				//init(sig, durationMs, m_overlap);
				//return;

			}

			if ( 0 != m_sample )
				if ( m_sample != sig.sample_rate() )
					throw std::invalid_argument("Invalid signal specified (differing sample rate)");

			if ( sig.size() < m_fsize )
				pad(sig, m_fsize - sig.size());

			m_data.push_back(sig);

			return;
		}

		signal_t< T >&
		getFrame(std::size_t idx)
		{
			if ( idx >= count() )
				throw std::invalid_argument("Invalid index specified");

			return m_data[ idx ];
		}

		void 
		sample_rate(std::size_t sr)
		{
			m_sample = sr;
			return;
		}

		inline std::size_t
		sample_rate(void)
		{
			return m_sample;
		}

};

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
class signal_vector_t
{
	private:
		std::vector< signal_t< T > >	m_data;
		std::size_t						m_sample;

	protected:
	public:
		signal_vector_t(void) : m_sample(0) { return; }

		signal_vector_t(const signal_t< T >& v)
		{
			m_data.push_back(v);
			m_sample = v.sample_rate();

			return;
		}

		signal_vector_t(const signal_vector_t< T >& v)
		{
			if ( 0 == v.size() )
				return;

			m_sample = v[ 0 ].sample_rate();

			for ( std::size_t idx = 0; idx < v.size(); idx++ )
				if ( v[ idx ].sample_rate() != m_sample )
					throw std::runtime_error("Sample rate changed!");
				else
					m_data.push_back(v[ idx ]);

			return;
		}

		virtual ~signal_vector_t(void)
		{
			m_data.clear();
		}

		void 
		resize(std::size_t n)
		{
			m_data.resize(n);
			return;
		}

		void 
		assign(std::size_t pos, signal_t< T >& f)
		{
			if ( 0 == m_sample )
				m_sample = f.sample_rate();
			else if ( f.sample_rate() != m_sample )
				throw std::runtime_error("Sample rate changed!");

			m_data[ pos ] = f;
			return;
		}

		const signal_t< T >&
		operator[](std::size_t pos) const
		{
			return m_data[ pos ];
		}


		signal_t< T >&
		operator[](std::size_t pos)
		{
			return m_data[ pos ];
		}

		signal_vector_t< T >&
		operator+=( const signal_t< T >& v )
		{
			if ( 0 == m_sample )
				m_sample = v.sample_rate();
			else if ( v.sample_rate() != m_sample )
				throw std::runtime_error("Sample rate changed!");

			m_data.push_back(v);
			return *this;
		}

		void 
		push_back(const signal_t< T >& v)
		{
			if ( 0 == m_sample )
				m_sample = v.sample_rate();
			else if ( v.sample_rate() != m_sample )
				throw std::runtime_error("Sample rate changed!");

			m_data.push_back(v);
			return;
		}

		signal_vector_t< T >&
		operator=( const signal_vector_t< T >& o )
		{
			if ( this != &o )
				for ( std::size_t idx = 0; idx < o.size(); idx++ )
					m_data.push_back(o[ idx ]);

			return *this;
		}

		signal_vector_t< T >&
		operator=( signal_vector_t< T >&& o )
		{
			if ( this != &o )
				for ( std::size_t idx = 0; idx < o.size(); idx++ )
					m_data.push_back(o[ idx ]);

			return *this;
		}

		signal_vector_t< T >&
		operator+=( const signal_vector_t< T >& v )
		{
			for ( std::size_t idx = 0; idx < v.size(); idx++ )
				m_data.push_back(v[ idx ]);

			return *this;
		}

		std::size_t
		size(void) const
		{
			return m_data.size();
		}

		signal_t< T >
		join(void)
		{
			signal_t< T >	ret;
			std::size_t		len = 0;  
			std::size_t		cnt = 0;

			ret.sample_rate(m_sample);

			if ( 1 == m_data.size() ) 
				return m_data[ 0 ];

			for ( std::size_t idx = 0; idx < m_data.size(); idx++ ) {
				if ( SIZE_MAX - len < m_data[ idx ].size() )
					throw std::overflow_error("Additive integer overflow encountered");

				len += m_data[ idx ].size();
			}

			ret.resize(len);

			for ( std::size_t idx = 0; idx < m_data.size(); idx++ ) {
				signal_t< T >& s = m_data[ idx ];

				if ( 0 == idx )
					ret.sample_rate(s.sample_rate());
				else if ( s.sample_rate() != ret.sample_rate() )
					throw std::runtime_error("Encountered signal vector with varying sample rates!");

				for ( std::size_t didx = 0; didx < s.size(); didx++ )
					ret[ cnt++ ] = s[ didx ];
			}

			return ret;
		}
};

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
overlap_add(signal_frames_t< T > frames, float overlap = 0.8)
{
	std::size_t		overlapCount = overlap * frames.sample_rate();
	std::size_t		skipSize = ( overlap != 0 ? frames.frameSize() % overlapCount : 0 );
	signal_t< T >	signal;

	if ( 0 == overlap || frames.frameSize() < overlapCount)
		throw std::runtime_error("Object in an invalid state; overlap width not specified");

	signal.sample_rate(frames.sample_rate());
	signal.resize(frames.count()*frames.frameSize());

	for ( std::size_t idx = 0; idx < frames.count(); idx++ ) {
		std::size_t		off = idx * ( frames.frameSize() - overlapCount );
		signal_t< T >	tmp = frames.getFrame( idx );

		for ( std::size_t tidx = /*skipSize*/ 0; tidx < tmp.size(); off++, tidx++ ) {
			//signal[ off ] = tmp[ tidx ];

			if ( tidx <= skipSize ) 
				signal[ off ] = tmp[ tidx ];

			signal[ off ] += tmp[ tidx ];

		}
	}

	return signal;
}


inline std::size_t 
msToIndex(std::size_t dms, std::size_t sr)
{
	return dms * sr / 1000;
}

inline std::size_t
indexToMs(std::size_t idx, std::size_t sr)
{
	return idx * 1000 / sr;
}

#define REAL_SQUARE(x) x.real()*x.real()
#define IMAG_SQUARE(x) x.imag()*x.imag()

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
fp_vector_t< T >
getProfile(signal_t< T >& noise, std::size_t durationMs, float overlap = 0.8)
{
	T										min = 0;
	signal_frames_t< T >					nf(noise, durationMs, overlap); 
	window_t< T >							window(nf.frameSize(), WINDOW_HANN);
	fft_t< T >*								fft(fft_factory_t< T >::getFFT());
	fp_vector_t< T >						nthr;
	complex_vector_t< T >					freq;

	for ( std::size_t idx = 0; idx < nf.count(); idx++ ) {
		window.apply(nf.getFrame(idx));

		freq	= fft->fft(nf.getFrame(idx));
		min		= ( freq[ 0 ].real()*freq[ 0 ].real() ) + ( freq[ 0 ].imag()*freq[ 0 ].imag() );

		if ( !nthr.size() ) {
			nthr.resize(freq.size());
			nthr[ 0 ] = min;
		}

		for ( std::size_t fidx = 0; fidx < freq.size(); fidx++ ) {
			if ( min >(freq[ fidx ].real()*freq[ fidx ].real()) + ( freq[ fidx ].imag()*freq[ fidx ].imag() ) )
				min = ( freq[ fidx ].real()*freq[ fidx ].real() ) + ( freq[ fidx ].imag()*freq[ fidx ].imag() );

			if ( min > nthr[ fidx ] )
				nthr[ fidx ] = min;
		}
	}

	return nthr;
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
noise_gate(signal_t< T >& signal, signal_t< T >& noise, float overlap = 0.8)
{
	const std::size_t		orig_len = signal.size();
	const std::size_t		sampleRate(signal.sample_rate());
	const std::size_t		chunkSize(sampleRate); //11025); //(2048);
	const std::size_t		overlapFrameCount( overlap * chunkSize );
	const std::size_t		stepSize(chunkSize - overlapFrameCount);
	window_t< T >			window(chunkSize, WINDOW_BLACKMAN);
	fft_t< T >*				fft(fft_factory_t< T >::getFFT());
	signal_vector_t< T >	sig_vec;
	signal_vector_t< T >	noise_vec;
	std::vector< fp_vector_t < T > > nspec_vec;
	std::vector< complex_vector_t< T > > cspec_vec;
	signal_vector_t< T >	spec_vec;
	signal_vector_t< T >	phase_vec;
	signal_vector_t< T >	gains;
	T						atten(-1.0); // std::pow(10.0, -48.0 / 20.0));
	T						attackDecay(std::pow(10.0, ( -48.0 / ( 10.0 * 1.0 ) )));
	complex_vector_t< T >	nsignal;
	signal_frames_t< T >	nframes;
	fp_vector_t< T > 		thresh_hold;
	T		 				min = 0.0;
	signal_t< T >			ret;

	if ( chunkSize > noise.size()) {
		std::size_t idx = noise.size();
		noise.grow(chunkSize - noise.size());
		::memset(&noise[ idx ], 0, chunkSize - noise.size());
	}

	if ( chunkSize > signal.size() ) {
		std::size_t idx = signal.size();
		signal.grow(chunkSize - signal.size());
		::memset(&signal[ idx ], 0, chunkSize - signal.size());
	}

	if ( noise.size() % chunkSize ) {
		std::size_t idx = noise.size();
		std::size_t rem = idx % chunkSize;
		std::size_t psz = chunkSize - rem;

		noise.grow(psz);
		::memset(&noise[ idx ], 0, psz);
	}

	if ( signal.size() % chunkSize ) {
		std::size_t idx = signal.size();
		std::size_t rem = idx % chunkSize;
		std::size_t psz = chunkSize - rem;

		signal.grow(psz);
		::memset(&signal[ idx ], 0, psz);
	}

	if ( noise.size() > chunkSize )
		for ( std::size_t idx = 0; idx < noise.size() - chunkSize; idx += stepSize )
			noise_vec.push_back(signal_t< T >(noise[ fp_slice_t(idx, chunkSize) ], noise.sample_rate()));
	else
		noise_vec.push_back(noise);

	if ( signal.size() > chunkSize )
		for ( std::size_t idx = 0; idx < signal.size() - chunkSize; idx += stepSize )
			sig_vec.push_back(signal_t< T >(signal[ fp_slice_t(idx, chunkSize) ], signal.sample_rate()));
	else
		sig_vec.push_back(signal);


	for ( std::size_t idx = 0; idx < noise_vec.size(); idx++ ) {
		complex_vector_t< T >	freq;
		//fp_vector_t< T >		magnitudes;

		window.apply(noise_vec[ idx ]);

		freq = fft->fft(noise_vec[ idx ]);

		nspec_vec.push_back(sqrt(absSquare(freq.real()) + absSquare(freq.imag())));
	}
	
	thresh_hold.resize(nspec_vec[ 0 ].size(), 0); //atten);

	for ( std::size_t idx = 0; idx < nspec_vec.size(); idx++ ) {
		for ( std::size_t j = 0; j < nspec_vec[ idx ].size(); j++ ) {
			T min = nspec_vec[ idx ][ j ];

			for ( std::size_t i = idx + 1; i < nspec_vec.size(); i++ ) {
				if ( min > nspec_vec[ i ][ j ] )
					min = nspec_vec[ i ][ j ];
			}

			if ( thresh_hold[ j ] < min )
				thresh_hold[ j ] = min;
		}
	}

/*	min = magnitudes[ 1 ];
		//min = ( freq[ 0 ].real()*freq[ 0 ].real() ) + ( freq[ 0 ].imag()*freq[ 0 ].imag() );

		if ( !thresh_hold.size() ) {
			thresh_hold.resize(freq.size());
			thresh_hold[ 0 ] = min;
		}



		for ( std::size_t fidx = 1; fidx < magnitudes.size(); fidx++ ) {
			if ( min > magnitudes[ fidx ] ) //(freq[ fidx ].real()*freq[ fidx ].real()) + ( freq[ fidx ].imag()*freq[ fidx ].imag() ) )
				min = magnitudes[ fidx ]; //( freq[ fidx ].real()*freq[ fidx ].real() ) + ( freq[ fidx ].imag()*freq[ fidx ].imag() );

			if ( min > thresh_hold[ fidx ] )
				thresh_hold[ fidx ] = min;
		}
	}*/

	spec_vec.resize(sig_vec.size());
	phase_vec.resize(sig_vec.size());

	for ( std::size_t idx = 0; idx < sig_vec.size(); idx++ ) {
		complex_vector_t< T >	freq;
		fp_vector_t< T >		mag;
		fp_vector_t< T >		phase;

		window.apply(sig_vec[ idx ]);
		freq = fft->fft(sig_vec[ idx ]);
		
		phase	= atan2(freq.imag(), freq.real());
		mag		= sqrt(absSquare(freq.real()) + absSquare(freq.imag()));

		spec_vec[ idx ].resize(mag.size());
		phase_vec[ idx ].push_back(phase); //push_back(atan2(freq.imag(), freq.real()));

		for ( std::size_t fidx = 0; fidx < freq.size(); fidx++ )
			spec_vec[ idx ][ fidx ] = mag[ fidx ]; //std::sqrt(( freq[ fidx ].real()*freq[ fidx ].real() ) + ( freq[ fidx ].imag()*freq[ fidx ].imag() ));

		cspec_vec.push_back(freq);
	}

	gains.resize(sig_vec.size());

	for ( std::size_t idx = 0; idx < gains.size(); idx++ )
		gains[ idx ].resize(spec_vec[ idx ].size(), atten);

	for ( std::size_t idx = 0; idx < spec_vec.size(); idx++ ) {
		T sensitivityFactor = std::pow(10.0, 0.0 / 10.0);

		for ( std::size_t j = 0; j < spec_vec[ idx ].size(); j++ ) {
			std::size_t center = gains.size() / 2;
			min = spec_vec[ idx ][ j ];

			for ( std::size_t i = idx + 1; i < spec_vec.size(); i++ ) {
				if ( min > spec_vec[ i ][ j ] )
					min = spec_vec[ i ][ j ];
			}

			T tmp = sensitivityFactor * thresh_hold[ j ];
			if ( min > tmp /*sensitivityFactor * thresh_hold[ j ]*/ && gains[ center ][ j ] < 1.0 )
				gains[ center ][ j ] = 1.0;

			//if ( min > sensitivityFactor * thresh_hold[ j ] && gains[ center ][ j ] < 1.0 )
			//	gains[ center ][ j ] = 1.0;
			//else
			//	gains[ center ][ j ] = 1.0;
			
		}
	}

	for ( std::size_t idx = 1; idx < gains.size()-1; idx++ ) {
		std::size_t center = gains[ idx ].size() / 2;

		for ( std::size_t cidx = center + 1; cidx < gains[ idx ].size(); cidx++ ) {
			if ( gains[ idx ][ cidx ] < gains[ idx - 1 ][ cidx ] * attackDecay )
				gains[ idx ][ cidx ] = gains[ idx - 1][ cidx ] * attackDecay;

			if ( gains[ idx ][ cidx ] < atten )
				gains[ idx ][ cidx ] = atten;
		}

		for ( std::size_t cidx = center - 1; cidx != 0; cidx-- ) {
			if ( gains[ idx ][ cidx ] < gains[ idx + 1 ][ cidx ] * attackDecay )
				gains[ idx ][ cidx ] = gains[ idx + 1][ cidx ] * attackDecay;

			if ( gains[ idx ][ cidx ] < atten )
				gains[ idx ][ cidx ] = atten;
		}
	}

	for ( std::size_t idx = 0; idx < cspec_vec.size(); idx++ )
		for ( std::size_t midx = 0; midx < cspec_vec[ idx ].size(); midx++ ) {
			T r = cspec_vec[ idx ][ midx ].real() * gains[ idx ][ midx ];
			T i = cspec_vec[ idx ][ midx ].imag() * gains[ idx ][ midx ];

			spec_vec[ idx ][ midx ] = std::sqrt(r*r + i*i);
		}

	/*for ( std::size_t idx = 0; idx < spec_vec.size(); idx++ ) {
		for ( std::size_t midx = 0; midx < spec_vec[ idx ].size(); midx++ )
			spec_vec[ idx ][ midx ] *= gains[ idx ][ midx ];
	}*/

	for ( std::size_t idx = 0; idx < phase_vec.size(); idx++ ) {
		std::size_t		overlapCount = overlap * chunkSize; //chunkSize;
		std::size_t		skipSize = ( overlap != 0 ? chunkSize % overlapCount : 0 );
		std::size_t		off = idx * ( chunkSize - overlapCount );

		for ( std::size_t pidx = 0; pidx < phase_vec[ idx ].size(); pidx++ )
			nsignal.push_back(spec_vec[ idx ][ pidx ] * std::cos(phase_vec[ idx ][ pidx ]), spec_vec[ idx ][ pidx ] * std::sin(phase_vec[ idx ][ pidx ]));

		window.setWindowType(nsignal.size(), WINDOW_BLACKMAN);
		window.apply(nsignal);

		signal_t< T > itmp = fft->ifft(nsignal);
		itmp.sample_rate(sampleRate);
		nsignal.clear();

		ret.grow(itmp.size());
		for ( std::size_t tidx = 0; tidx < itmp.size(); off++, tidx++ )
			if ( off >= ret.size() )
				throw std::runtime_error("Bad index: offset");
			else {
				/*if ( tidx <= skipSize )
					ret[ off ] = itmp[ tidx ];
				else*/
					ret[ off ] += itmp[ tidx ];
			}
	}

	ret.sample_rate(signal.sample_rate());
	ret.shrink(orig_len);
	return get_enveloped_signals(amplify(level(ret.normalize()), -0.65)).join(); //level(get_enveloped_signals(ret).join()).normalize();
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
T
getNoiseMean(signal_t< T >& noise, std::size_t durationMs, float overlap = 0.8)
{
	signal_frames_t< T >					nf(noise, (msToIndex(durationMs, noise.sample_rate()) > noise.size() ?
												indexToMs(noise.size() - 1, noise.sample_rate()) : durationMs ), overlap);
	window_t< T >							win(nf.frameSize(), WINDOW_BLACKMAN);
	fft_t< T >*								fft(fft_factory_t< T >::getFFT());
	std::vector< complex_vector_t< T >	>	freq;
	T										mean(0);

	for ( std::size_t idx = 0; idx < nf.count(); idx++ ) {
		win.apply(nf.getFrame(idx));
		mean += average(abs(fft->fft(nf.getFrame(idx))));
	}

	mean /= nf.count();
	return mean;
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
spectral_subtraction(signal_t< T >& signal, signal_t< T >& noise, std::size_t durationMs, float overlap = 0.8)
{
	signal_frames_t< T >	frames(signal, durationMs, overlap);
	signal_frames_t< T >	nframes; // ( noise, durationMs, overlap );
	signal_t< T >			ret;
	window_t< T >			window(frames.frameSize(), WINDOW_BLACKMAN);
	window_t< T >			nwin;
	fft_t< T >*				fft(fft_factory_t< T >::getFFT());
	complex_vector_t< T >	nsignal;
	//complex_vector_t< T >	cnoise; // ( fft->fft(noise) );
	T						mean(getNoiseMean(noise, durationMs, overlap)); //( ::average(::abs(cnoise)) );

	for ( std::size_t idx = 0; idx < frames.count(); idx++ ) {
		complex_vector_t< T >	freq;
		fp_vector_t< T >		phase; 
		fp_vector_t< T >		magnitude;

		window.apply(frames.getFrame(idx));
		freq = fft->fft(frames.getFrame(idx));

		phase		= atan2(freq.imag(), freq.real());
		magnitude	= sqrt( absSquare(freq.real()) + absSquare(freq.imag())); 

		if ( phase.size() != magnitude.size() )
			throw std::runtime_error("Unexpected error encountered, phase and magnitude vectors are different sizes");

		for ( std::size_t midx = 0; midx < magnitude.size(); midx++ ) {
			magnitude[ midx ] = ( magnitude[ midx ] ); //- mean );

			if ( 0 > magnitude[ midx ] )
				magnitude[ midx ] = 0;
		}

		for ( std::size_t cidx = 0; cidx < phase.size(); cidx++ )
			nsignal.push_back(magnitude[ cidx ] * std::cos(phase[ cidx ]), magnitude[ cidx ] * std::sin(phase[ cidx ]));

		nwin.setWindowType(nsignal.size(), WINDOW_BLACKMAN);
		nwin.apply(nsignal);

		signal_t< T > itmp = fft->ifft(nsignal);
		itmp.sample_rate(frames.sample_rate());
		nframes.putFrame(itmp, itmp.size(), (itmp.size()*1000)/frames.sample_rate());
		nsignal.clear();
	}


	nframes.sample_rate(signal.sample_rate());

	//for ( std::size_t idx = 0; idx < nframes.count(); idx++ ) 
	//	window.apply(frames.getFrame(idx)); //nframes.getFrame( idx ));

	ret = overlap_add(nframes, overlap);
	ret.sample_rate(signal.sample_rate());

	return level(get_enveloped_signals(ret.normalize()).join());
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool >::type = true >
signal_t< T >
trim(signal_t< T >& sig, T minv = 0.01)
{
	signal_t< T >	tmp;
	signal_t< T >	ret;
	std::size_t		start	= 0;
	std::size_t		end		= 0;

	for ( std::size_t idx = 0; idx < sig.size(); idx++ ) {
		T val = std::abs(sig[ idx ]);

		if ( val < minv )
			continue;
		else {
			start = idx;
			break;
		}
	}

	if ( start == sig.size() )
		return signal_t< T >();

	tmp.push_back(sig[ fp_slice_t(start, sig.size() - start) ]);

	for ( std::size_t idx = tmp.size() - 1; idx != 0; idx-- ) {
		T val = std::abs(sig[ idx ]);

		if ( val < minv )
			continue;
		else {
			start = idx;
			break;
		}
	}

	if ( start == 0 )
		return tmp;

	ret.sample_rate(sig.sample_rate());
	ret.push_back(tmp[ fp_slice_t(0, tmp.size() - start) ]);
	return ret;
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_vector_t< T >
get_enveloped_signals(signal_t< T >& sig, T minv = 0.01, std::size_t attackMs = 10, std::size_t releaseMs = 250)
{
	signal_vector_t< T >			ret;
	std::size_t						start = 0;
	std::size_t						end = sig.size();
	T								attack = ( attackMs*sig.sample_rate() / 1000 );
	T								release = ( releaseMs*sig.sample_rate() / 1000 );
	T								val = 0.0;
	bool							flag = false;

	for ( std::size_t idx = 0; idx < sig.size(); idx++ ) {
		val = std::abs(sig[ idx ]);

		if ( val < minv ) {
			flag = false;
			continue;
		} else {
			if ( false == flag ) {
				flag = true;
				start = idx;
			}

			if ( idx >= ( start + attack ) )
				break;
		}

		flag	= false;
		end		= start + 1;

		for ( std::size_t eidx = start + 1; eidx < sig.size(); eidx++ ) {
			val = std::abs(sig[ eidx ]);
			
			if ( val > minv ) {
				flag = false;
				continue;
			} else {
				if ( false == flag ) {
					flag = true;
					end = eidx;
				}

				if ( eidx >= ( end + release ) )
					break;
			}
		}

		if ( start == end )
			continue;

		if ( end == sig.size() ) {
			end--;
			idx = end;
		} else
			idx = end;

		ret.push_back(signal_t< T >(sig[ ::fp_slice_t(start, end - start) ], sig.sample_rate()));
	}

	return ret;
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
get_silence(std::size_t ms, std::size_t sr)
{
	std::size_t		len = ( ms*sr / 1000 );
	signal_t< T >	sil;

	sil.resize(len);
	sil.sample_rate(sr);
	//signal_t< T > psig(ms*sr/1000);

	//for ( std::size_t idx = 0; idx < len; idx++ )
	//	sil[ idx ] = 0.0;

	return sil;
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
filter_silence(signal_t< T >& sig, std::size_t padMs = 0, T minv = 0.01, std::size_t attackMs = 10, std::size_t releaseMs = 250)
{
	signal_t< T >			psig	= get_silence< T >(padMs, sig.sample_rate());
	signal_vector_t< T >	svec	= get_enveloped_signals(sig, minv, attackMs, releaseMs);
	signal_vector_t< T >	pvec;

	if ( 0 == padMs )
		return svec.join();

	pvec.push_back(psig);
	for ( std::size_t idx = 0; idx < svec.size(); idx++ ) {
		pvec.push_back(svec[idx]);
		pvec.push_back(psig);
	}

	return pvec.join();
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
bandstop(signal_t< T >& s, T bandstart, T bandwidth)
{
	T*															p = &s[ 0 ];
	Dsp::SimpleFilter< Dsp::Butterworth::BandStop< 24 >, 1 >	f;

	f.setup(24, s.sample_rate(), bandstart + ( bandwidth / 2 ), bandwidth);
	f.process(s.size(), &p);

	return s;
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
lowpass(signal_t< T >& s, T cutoff)
{
	T*															p = &s[ 0 ];
	Dsp::SimpleFilter< Dsp::Butterworth::LowPass< 24 >, 1 >		f;

	f.setup(24, s.sample_rate(), cutoff);
	f.process(s.size(), &p);

	return s;
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
highpass(signal_t< T >& s, T cutoff)
{
	T*															p = &s[ 0 ];
	Dsp::SimpleFilter< Dsp::Butterworth::HighPass< 24 >, 1 >	f;

	f.setup(24, s.sample_rate(), cutoff);
	f.process(s.size(), &p);

	return s;
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
bandpass(signal_t< T >& s, T center, T width)
{
	T*															p = &s[ 0 ];
	Dsp::SimpleFilter< Dsp::Butterworth::BandPass< 24 >, 1 >	f;

	f.setup(24, s.sample_rate(), center, width);
	f.process(s.size(), &p);

	return s;
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
generate_tone(std::size_t length, std::size_t sampleRate, T base_freq, T bandwidth = T(100.0), T amplitude = T(0.999) )
{
	signal_t< T >			tone;
	T						c = base_freq + ( bandwidth / 2 );

	tone.resize(length);
	tone.sample_rate(sampleRate);

	for ( std::size_t idx = 0; idx < length; idx++ ) 
		tone[ idx ] = amplitude * std::sin(2 * M_PI * base_freq * idx / sampleRate);

	return tone;
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
heterodyne_mix(signal_t< T >& s, T base_freq, T bandwidth = T(100.0), T amplitude = T(0.999))
{
	
	signal_t< T > tone = generate_tone((std::size_t)s.size(), (std::size_t)s.sample_rate(), base_freq, bandwidth, amplitude);

	for ( std::size_t idx = 0; idx < s.size(); idx++ ) 
		s[ idx ] = ( s[ idx ] * tone[idx] );

	return s;
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
amplify(signal_t< T >& s, T ratio = 1.0, bool clip = false)
{
	T peak = s.max();

	if ( false == clip && 1.0 < ratio*peak )
		ratio = 1.0 / peak;

	for ( std::size_t idx = 0; idx < s.size(); idx++ )
		s[ idx ] *= ratio;

	return s;
}

#define LEVELER_FACTORS 6

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, bool>::type = true >
signal_t< T >
level(signal_t< T >& s)
{	
	static T limit[ LEVELER_FACTORS ]		= { 0.0001, 0.0, 0.1, 0.3, 0.5, 1.0 };
	static T adjust[ LEVELER_FACTORS ]		= { 0 };
	static T addon[ LEVELER_FACTORS ]		= { 0 };
	static T adjfactor[ LEVELER_FACTORS ]	= { 0.80, 1.00, 1.20, 1.20, 1.00, 0.80 };

	T sign		= 0.0;
	T abs_val	= 0.0;

	for ( std::size_t idx = 0; idx < s.size(); idx++ ) {
		T val = s[ idx ];

		if ( 0.0 > val )
			sign = -1.0;
		else
			sign = 1.0;


		abs_val = std::abs(val);

		for ( std::size_t lidx = 0; lidx < LEVELER_FACTORS; lidx++ ) {
			if ( limit[ lidx ] >= abs_val ) {
				val *= static_cast< T >( adjfactor[ lidx ] );
				val += static_cast< T >( addon[ lidx ] * sign);
				s[ idx ] = val;
				break;
			}
		}
	}

	return s;
}

#endif // SIGNAL_HPP
