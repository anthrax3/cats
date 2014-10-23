#ifndef PLOT_HPP
#define PLOT_HPP

#include <cstdint>
#include <QObject>
#include <QString>
#include <QImageWriter>
#include <QImage>
#include <QColor>
#include <QRgb>
#include <QPainter>
#include <QFont>

#include "colormap.hpp"

#ifndef ssize_t
typedef ptrdiff_t ssize_t;
#endif

/*
basically all of this code was shamelessly ripped from pngplot.cc or whatever
most of the implementation scared the hell out of me, so i ripped and cleaned up
the parts that didnt and sucked it into Qt world.

Yes, qpainter. I know. I found out afterwards, but this 'just works', and it seems 
like often enough Qt stuff that should just work either doesn't or is some funky
wtf ever that i would never think to try and use the only way they want me to.
id est, why are there like 300 ways to instantiate an audio file in qt, but none
of them include from memory?

that said, i tried all of the classes and libraries commonly used for plotting,
inclusive of QtChart and their data visualization offerings and found them all
to be a mix of ass backwards and super-heavy on resources or otherwise lacking 
in some feature i needed, which sometimes was as small as remedial documentation.

this, just works. thats what i wanted and needed. "next!"
*/

class plot_t : public QObject
{
	Q_OBJECT

	private:
		QImage*		m_image;
		QString		m_file;
		colorMap_t	m_cmap;

	public:
		plot_t(QObject* p = nullptr);
		~plot_t(void);
		void createPlot(QString&, std::size_t, std::size_t);
		bool write(void);
		bool write(QString, QImage&);
		void setPixel(std::size_t, std::size_t, float);

		void plot(std::size_t, std::size_t, QRgb);
		void plot(std::size_t, std::size_t, double, double, double, uint8_t a = 255);
		void plot(std::size_t, std::size_t, double);
		void plot(std::size_t, std::size_t, rgb_t);

		void line(std::size_t, std::size_t, std::size_t, std::size_t, QRgb);
		void line(std::size_t, std::size_t, std::size_t, std::size_t, double, double, double, uint8_t a = 255);
		void line(std::size_t, std::size_t, std::size_t, std::size_t, double);
		void line(std::size_t, std::size_t, std::size_t, std::size_t, rgb_t);

		void bezier(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, QRgb);
		void bezier(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, double, double, uint8_t a = 255);
		void bezier(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double);
		void bezier(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, rgb_t);

		void arrow(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, QRgb);
		void arrow(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, double, double, double, uint8_t a = 255);
		void arrow(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, double);
		void arrow(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, rgb_t);

		void filledarrow(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, QRgb);
		//void filledarrow(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, double, double, double, uint8_t a = 255);
		//void filledarrow(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, double);
		// void filledarrow(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, rgb_t);

		void drawtop(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, QRgb);
		// void drawtop(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, double, double, uint8_t a = 255);
		// void drawtop(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double);
		// void drawtop(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, rgb_t);

		void drawbottom(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, QRgb);
		// void drawbottom(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, double, double, uint8_t a = 255);
		// void drawbottom(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double);
		// void drawbottom(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, rgb_t);

		void filledtriangle(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, QRgb);
		// void filledtriangle(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, double, double, uint8_t a = 255);
		// void filledtriangle(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double);
		// void filledtriangle(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, rgb_t);

		void cross(std::size_t, std::size_t, std::size_t, std::size_t, QRgb);
		// void cross(std::size_t, std::size_t, std::size_t, std::size_t, double, double, double, uint8_t a = 255);
		// void cross(std::size_t, std::size_t, std::size_t, std::size_t, double);
		// void cross(std::size_t, std::size_t, std::size_t, std::size_t, rgb_t);

		void maltesecross(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, QRgb);
		// void maltesecross(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double, double, double, uint8_t a = 255);
		// void maltesecross(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, double);
		// void maltesecross(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, rgb_t);

		void diamond(std::size_t, std::size_t, std::size_t, std::size_t, QRgb);
		// void diamond(std::size_t, std::size_t, std::size_t, std::size_t, double, double, double, uint8_t a = 255);
		// void diamond(std::size_t, std::size_t, std::size_t, std::size_t, double);
		// void diamond(std::size_t, std::size_t, std::size_t, std::size_t, rgb_t);

		void filleddiamond(std::size_t, std::size_t, std::size_t, std::size_t, QRgb);
		// void filleddiamond(std::size_t, std::size_t, std::size_t, std::size_t, double, double, double, uint8_t a = 255);
		// void filleddiamond(std::size_t, std::size_t, std::size_t, std::size_t, double);
		// void filleddiamond(std::size_t, std::size_t, std::size_t, std::size_t, rgb_t);
};

#endif // PLOT_HPP
