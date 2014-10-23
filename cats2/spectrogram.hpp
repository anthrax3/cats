#ifndef SPECTROGRAM_HPP
#define SPECTROGRAM_HPP

#include <cmath>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <limits>
#include <vector>
#include <complex>
#include <memory>

#include <QImage>
#include <QColor>

#include "fft.hpp"
#include "samplerate.hpp"
#include "colormap.hpp"
#include "signal.hpp"


// -- shamelessly ripped from http://www.ms.mff.cuni.cz/~krajj7am/big/spectrogram-doc/spectrogram_8cpp_source.html
// -- thanks to 'jan' apparently for this code.

#define PI 3.1415926535897932384626433832795

typedef std::complex< float > Complex;
typedef std::vector< float > real_vec;
typedef std::vector< Complex > complex_vec;

 // ---

enum window_type {
	WINDOW_TYPE_HANN,
	WINDOW_TYPE_BLACKMAN,
	WINDOW_TYPE_RECTANGULAR,
	WINDOW_TYPE_TRIANGULAR
};

enum axisScale_t { SCALE_LINEAR, SCALE_LOGARITHMIC };
//enum SynthesisType { SYNTHESIS_SINE, SYNTHESIS_NOISE };
enum bcorrection_t { BRIGHT_NONE, BRIGHT_SQRT };

struct ret_t
{
	QString name;
	QImage	image;
};

class spectrogram_t : public QObject
{
	Q_OBJECT
	signals:
		void updateStatus(QString);

	private:
		double				m_bandwidth;
		double				m_basefreq;
		double				m_maxfreq;
		double				m_overlap;
		double				m_ppsec;
		window_type			m_window;
		axisScale_t			m_iaxis;
		axisScale_t			m_faxis;
		bcorrection_t		m_correction;
		colorMap_t			m_cmap;

	protected:
		QImage make_image(const std::vector<real_vec>& data);
		void apply_window(complex_vec& chunk, int lowidx, double filterscale);

	public:
		spectrogram_t(QObject* p = nullptr);
		~spectrogram_t(void) { return; }

		QImage to_image(real_vec& signal, int samplerate);
		
};

 // --

typedef std::pair<int, int> intpair;

class Filterbank
{
	public:
		static std::auto_ptr<Filterbank> get_filterbank(axisScale_t type, double scale, 
														double base, double hzbandwidth, double overlap);
	
		Filterbank(double scale);
		virtual intpair get_band(int i) const = 0;
		virtual int get_center(int i) const = 0;
		virtual int num_bands_est(double maxfreq) const = 0;
		virtual ~Filterbank();

	protected:
		const double scale_;
};

class LinearFilterbank : public Filterbank
{
	public:
		LinearFilterbank(double scale, double base, double hzbandwidth, double overlap);
		intpair get_band(int i) const;
		int get_center(int i) const;
		int num_bands_est(double maxfreq) const;
	private:
		const double bandwidth_;
		const int startidx_;
		const double step_;
};

class LogFilterbank : public Filterbank
{
	public:
		LogFilterbank(double scale, double base, double centsperband, double overlap);
		intpair get_band(int i) const;
		int get_center(int i) const;
		int num_bands_est(double maxfreq) const;
	private:
		const double centsperband_;
		const double logstart_;
		const double logstep_;
};

#endif