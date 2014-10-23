#ifndef HAVE_MP3_T_HPP
#define HAVE_MP3_T_HPP

#include <cstdint>
#include <exception>
#include <limits>
#include <vector>
#include <cstdio>

extern "C" {
#include <lame.h>
}

#include "signal.hpp"

/*#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif*/

class mpegException_t : public std::exception
{
	private:
	protected:
		std::string m_msg;
	public:
		explicit mpegException_t(const char* m) : std::exception(m), m_msg(m) { return; }
		explicit mpegException_t(const std::string& m) : std::exception(m.c_str()), m_msg(m) { return; }
		virtual ~mpegException_t(void) throw( ) { return; }
		virtual const char* what(void) throw( ) { return m_msg.c_str(); }
};

class mpegInitializationException_t : public mpegException_t
{
	private:
	protected:
	public:
		mpegInitializationException_t(void) : mpegException_t("An Error occurred while initializing the LAME MP3 Library.") { return; }
		explicit mpegInitializationException_t(const char* m) : mpegException_t(m) { return; }
		explicit mpegInitializationException_t(const std::string& m) : mpegException_t(m) { return; }
};

class mpegBadStateException_t : public mpegException_t
{
	private:
	protected:
	public:
		mpegBadStateException_t(void) : mpegException_t("The class or LAME library encountered an invalid state, likely a nullptr.") { return; }
		explicit mpegBadStateException_t(const char* m) : mpegException_t(m) { return; }
		explicit mpegBadStateException_t(const std::string& m) : mpegException_t(m) { return; }
};

class mpegIntegerOverflowException_t : public mpegException_t
{
	private:
	protected:
	public:
		mpegIntegerOverflowException_t(void) : mpegException_t("One or more values passed into the mpeg_t class would result in an integer overflow if utilized.") { return; }
		explicit mpegIntegerOverflowException_t(const char* m) : mpegException_t(m) { return; }
		explicit mpegIntegerOverflowException_t(const std::string& m) : mpegException_t(m) { return; }
};

class mpegDecoderErrorException_t : public mpegException_t
{
	private:
	protected:
	public:
		mpegDecoderErrorException_t(void) : mpegException_t("An unknown error was encountered while decoding the MP3.") { return; }
		explicit mpegDecoderErrorException_t(const char* m) : mpegException_t(m) { return; }
		explicit mpegDecoderErrorException_t(const std::string& m) : mpegException_t(m) { return; }
};

class mpegEncoderErrorException_t : public mpegException_t
{
	private:
	protected:
	public:
		mpegEncoderErrorException_t(void) : mpegException_t("An unknown error was encountered while encoding the MP3.") { return; }
		explicit mpegEncoderErrorException_t(const char* m) : mpegException_t(m) { return; }
		explicit mpegEncoderErrorException_t(const std::string& m) : mpegException_t(m) { return; }
};

/* 
I am just going to stick with converting to int16_t's even though its ugly and it would be
nice to avoid that all together if I could.

class mpegLameIsLameException_t : public mpegException_t
{
	private:
	protected:
	public:
	mpegLameIsLameException_t(void) : mpegException_t("There's really no good reason why you can't instantiate a long double "
													  "version of this template class, except, LAME is lame. It's probably "
													  "irrelevant anyway, because the /reason/ they want your floating points "
													  "to be scaled is because they just truncate them. Although there is "
													  "support for 'user-supplied transforms' voodoo, it looked like trying "
													  "to add the support was pointless without a fair amount of effort in "
													  "LAMEs internals. SO. No long doubles for you. (Did you /really/ need "
													  "them anyway?)") { return; }
	explicit mpegLameIsLameException_t(const char* m) : mpegException_t(m) { return; }
	explicit mpegLameIsLameException_t(const std::string& m) : mpegException_t(m) { return; }
};*/

class mp3_t
{
	private:
	protected:
		static void debug(const char*, va_list) { return; }
		static void msg(const char*, va_list)	{ return; }

		static void 
		error(const char* fmt, va_list ap) 
		{ 
			char*						buf = nullptr;
// -- SDL related; irrelevant here, but I don't want to disable the warnings
// -- simply because of one function that i'd prefer not to have in the first
// -- place, except LAME is lame and stuff.
#if defined(_WIN32) || defined(_WIN64) 
			signed int					ret = ::vsnprintf_s(nullptr, 0, 0, fmt, ap);
#else
			signed int					ret = std::vsnprintf(nullptr, 0, fmt, ap);
#endif

			if ( 0 > ret )
				throw mpegBadStateException_t();

			buf = new char[ ret + 1 ];

			std::memset(buf, 0, ret + 1);

#if defined(_WIN32) || defined(_WIN64)
			ret = ::vsnprintf_s(buf, static_cast< std::size_t >(ret + 1), _TRUNCATE, fmt, ap);
#else
			ret = std::vsnprintf(buf, ret + 1, fmt, ap);
#endif
			if ( 0 >= ret )
				throw mpegBadStateException_t();

			throw mpegException_t(buf);
			
			// -- purely pedantic habit.
			delete[] buf;
			return;
		}

		template< typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true >
		static inline int16_t scale_round_to_int16(T val)
		{

			if ( 0.0 < val )
				return 1 <= val ? std::numeric_limits< int16_t >::max() : static_cast<int16_t>( val * std::numeric_limits< int16_t >::max() + 0.5f );

			return -1 >= val ? std::numeric_limits< int16_t >::min() : static_cast<int16_t>( -val * std::numeric_limits< int16_t >::min() - 0.5f );
		}

	public:
		mp3_t(void) { return; }
		virtual ~mp3_t(void) { return; }

		template< typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true >
		static void 
		encode(signal_t< T >& signal, std::vector< uint8_t >& rvec)
		{
			lame_global_flags*		gfp = ::lame_init();
			signed int				ret = 0;
			std::vector< uint8_t >	buf;
			std::size_t				off = 0;
			std::vector< int16_t >  data;


			if ( nullptr == gfp ) 
				throw mpegInitializationException_t();

			::lame_set_debugf(gfp, &mp3_t::debug);
			::lame_set_errorf(gfp, &mp3_t::error);
			::lame_set_msgf(gfp, &mp3_t::msg);

			::lame_set_num_channels(gfp, 1);
			::lame_set_mode(gfp, MPEG_mode::MONO);
			::lame_set_in_samplerate(gfp, signal.sample_rate());
			::lame_set_error_protection(gfp, 0);
			::lame_set_write_id3tag_automatic(gfp, 0);
			::lame_set_bWriteVbrTag(gfp, 0);

			if ( 0 > ::lame_init_params(gfp) ) {
				::lame_close(gfp);
				throw mpegInitializationException_t();
			}

			std::size_t dblen = 1.25*signal.size() + 7200 + (8192*10);
			unsigned char* dbuf = new unsigned char[ dblen ]; 
			rvec.clear();
	
			for ( std::size_t idx = 0; idx < signal.size(); idx++ )
				data.push_back(scale_round_to_int16(signal[ idx ]));

			ret = ::lame_encode_buffer(gfp, data.data(), nullptr, data.size(), dbuf, dblen); 

			if ( 0 > ret )
				throw mpegEncoderErrorException_t();

			rvec.insert(rvec.end(), dbuf, dbuf + ret);

			ret = ::lame_encode_flush(gfp, dbuf, dblen);

			if ( 0 > ret )
				throw mpegEncoderErrorException_t();

			if ( 0 < ret )
				rvec.insert(rvec.end(), dbuf, dbuf+ret);

			::lame_mp3_tags_fid(gfp, nullptr);
			::lame_close(gfp);

			return;
		}

		template< typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true >
		static void 
		decode(std::vector< uint8_t >& in, signal_vector_t< T >& sigvec) 
		{
			lame_global_flags*						gfp = ::lame_init();
			hip_t									hip = nullptr;
			mp3data_struct 							mpd = { 0 };
			signed int								ret = 0;
			std::vector < std::vector< int16_t > >	data;
			std::size_t								inlen = 0;
			int16_t									left[1152] = { 0 };
			int16_t									right[1152 ] = { 0 };

			if ( nullptr == gfp ) 
				throw mpegInitializationException_t();

			::lame_set_debugf(gfp, &mp3_t::debug);
			::lame_set_errorf(gfp, &mp3_t::error);
			::lame_set_msgf(gfp, &mp3_t::msg);

			::lame_set_decode_only(gfp, 1);

			if ( 0 > lame_init_params(gfp) ) {
				::lame_close(gfp);
				throw mpegInitializationException_t();
			}

			hip = ::hip_decode_init();

			if ( nullptr == hip ) {
				::lame_close(gfp);
				throw mpegInitializationException_t();
				return;
			}

			::hip_set_debugf(hip, &mp3_t::debug);
			::hip_set_errorf(hip, &mp3_t::error);
			::hip_set_msgf(hip, &mp3_t::msg);

			inlen = in.size();

			while ( !mpd.header_parsed ) {
				
				ret = ::hip_decode_headers(hip, in.data(), inlen, left, right, &mpd);

				if ( 0 > ret )
					throw mpegDecoderErrorException_t();

				inlen = 0;
			}

			data.resize(mpd.stereo);

			do {

				ret = ::hip_decode1_headers(hip, in.data(), inlen, left, right, &mpd);

				if ( 0 > ret )
					throw mpegDecoderErrorException_t();

				if ( 0 < ret ) {
					data[ 0 ].insert(data[ 0 ].end(), left, left + ret);

					if ( 1 < mpd.stereo )
						data[ 1 ].insert(data[ 1 ].end(), right, right + ret);
				}

			} while ( 0 < ret );

			sigvec.resize(mpd.stereo);

			for ( std::size_t idx = 0; idx < sigvec.size(); idx++ ) {
				sigvec[ idx ].resize(data[ idx ].size());
				sigvec[ idx ].sample_rate(mpd.samplerate);

				for ( std::size_t nidx = 0; nidx < sigvec[ idx ].size(); nidx++ )
					sigvec[ idx ][ nidx ] = ( 1.0 * data[ idx ][ nidx ] / INT16_MAX );
			}

			::lame_close(gfp);
			::hip_decode_exit(hip);
			return;
		}

};



#endif