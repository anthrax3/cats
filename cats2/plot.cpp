#include "plot.hpp"

plot_t::plot_t(QObject* p) : QObject(p), m_image(nullptr), m_file("")
{
	return;
}

plot_t::~plot_t(void)
{
	if ( nullptr != m_image )
		delete m_image;

	return;
}

void 
plot_t::createPlot(QString& name, std::size_t height, std::size_t width)
{
	//QPainter* painter(nullptr);
	//QFont f;
	//QRect r;

	if ( nullptr != m_image ) {
		delete m_image;
	}

	/*f.setFamily("Tahoma");
	f.setPointSize(20);
	f.setBold(true);*/

	m_file = name;
	m_image = new QImage(width, height, QImage::Format::Format_ARGB32);

	m_image->fill(QColor(0, 0, 0, 0));
	/*painter = new QPainter(m_image);
	painter->setFont(f);
	painter->fillRect(m_image->rect(), Qt::transparent);
	painter->setCompositionMode(QPainter::CompositionMode::CompositionMode_Source);

	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
	m_image->rect().marginsAdded(QMargins(50, 50, 50, 50));
	painter->setPen(QColor("white"));
	painter->fillRect(20, 20, width - 30, height - 30, Qt::GlobalColor::white);
	painter->drawText(m_image->rect(), Qt::AlignHCenter, QString("Test text"));
	painter->end();
	delete painter;*/
	return;
}

bool
plot_t::write(void)
{
	QImageWriter w(m_file);

	if ( nullptr == m_image )
		return false;

	if ( false == w.canWrite() )
		return false;

	if ( false == w.write(*m_image) )
		return false;

	return true;
}

bool 
plot_t::write(QString p, QImage& i)
{
	QImageWriter w(p);

	if ( false == w.canWrite() )
		return false;

	if ( false == w.write(i) )
		return false;

	return true;
}

void 
plot_t::setPixel(std::size_t h, std::size_t w, float v)
{
	rgb_t	c = m_cmap.getColor(v);
	//QColor  qc(c.r, c.g, c.b);

	m_image->setPixel(w, h, QColor::fromRgbF(c.r, c.g, c.b).rgb()); 
	return;
}

void 
plot_t::plot(std::size_t x, std::size_t y, QRgb c)
{
	m_image->setPixel(x, m_image->height() - y, c);
	return;
}

void 
plot_t::plot(std::size_t x, std::size_t y, double r, double g, double b, uint8_t a)
{
	QColor c(r, g, b, a);

	this->plot(x, y, c.rgba());
	return;
}

void 
plot_t::plot(std::size_t x, std::size_t y, double v)
{
	this->plot(x, y, m_cmap.getColor(v));
	return;
}

void 
plot_t::plot(std::size_t x, std::size_t y, rgb_t c)
{
	this->plot(x, y, c.r, c.g, c.b);
	return;
}

void 
plot_t::line(std::size_t xfrom, std::size_t yfrom, std::size_t xto, std::size_t yto, QRgb c)
{
	ssize_t dy			= yto - yfrom;
	ssize_t dx			= xto - xfrom;
	ssize_t stepx		= 0;
	ssize_t stepy		= 0;
	ssize_t fraction	= 0;
	
	if ( 0 > dy ) {
		dy		= -dy;
		stepy	= -1;
	} else 
		stepy = 1;
	
	if ( 0 > dx ) {
		dx		= -dx;
		stepx	= -1;
	} else
		stepx = 1;

	dy <<= 1; // -- dy is now 2*dy
	dx <<= 1; // -- dx is now 2*dx

	this->plot(xfrom, yfrom, c);

	if ( dx > dy ) {
		fraction = dy - ( dx >> 1 );

		while ( xfrom != xto ) {
			if ( 0 <= fraction ) {
				yfrom += stepy;
				fraction -= dx;
			}

			xfrom += stepx;
			fraction += dy;
			this->plot(xfrom, yfrom, c);
		}
	} else {
		fraction = dx - ( dy >> 1 );

		while ( yfrom != yto ) {
			if ( 0 <= fraction ) {
				xfrom += stepx;
				fraction -= dy;
			}

			yfrom += stepy;
			fraction -= dy;
			this->plot(xfrom, yfrom, c);
		}
	}

	return;
}

void 
plot_t::line(std::size_t xfrom, std::size_t yfrom, std::size_t xto, std::size_t yto, double r, double g, double b, uint8_t a)
{
	QColor c(r, g, b, a);

	this->line(xfrom, yfrom, xto, yto, c.rgba());
	return;
}

void 
plot_t::line(std::size_t xfrom, std::size_t yfrom, std::size_t xto, std::size_t yto, double v)
{
	this->line(xfrom, yfrom, xto, yto, m_cmap.getColor(v));
	return;
}

void 
plot_t::line(std::size_t xfrom, std::size_t yfrom, std::size_t xto, std::size_t yto, rgb_t c)
{
	this->line(xfrom, yfrom, xto, yto, c.r, c.g, c.b);
	return;
}

void 
plot_t::bezier(	std::size_t startPtX, std::size_t startPtY, std::size_t startCtrlX, std::size_t startCtrlY,
				std::size_t endPtX, std::size_t endPtY, std::size_t endCtrlX, std::size_t endCtrlY,
				QRgb color)
{
	double cx = 3.0*( startCtrlX - startPtX );
	double bx = 3.0*( endCtrlX - startCtrlX ) - cx;
	double ax = double(endPtX - startPtX - cx - bx);

	double cy = 3.0*( startCtrlY - startPtY );
	double by = 3.0*( endCtrlX - startCtrlX ) - cy;
	double ay = double(endPtY - startPtY - cy - by);

	double x, y, newx, newy;
	x = startPtX;
	y = startPtY;

	for ( double t = 0.0; t <= 1.005; t += 0.005 ) {
		newx = startPtX + t*( double(cx) + t*( double(bx) + t*( double(ax) ) ) );
		newy = startPtY + t*( double(cy) + t*( double(by) + t*( double(ay) ) ) );
		
		this->line(int(x), int(y), int(newx), int(newy), color);
		x = newx;
		y = newy;
	}

	return;
}

void 
plot_t::bezier(	std::size_t startPtX, std::size_t startPtY, std::size_t startCtrlX, std::size_t startCtrlY,
				std::size_t endPtX, std::size_t endPtY, std::size_t endCtrlX, std::size_t endCtrlY,
				double r, double g, double b, uint8_t a)
{
	QColor c(r, g, b, a);

	this->bezier(startPtX, startPtY, startCtrlX, startCtrlY, endPtX, endPtY, endCtrlX, endCtrlY, c.rgba());
	return;
}

void 
plot_t::bezier(std::size_t startPtX, std::size_t startPtY, std::size_t startCtrlX, std::size_t startCtrlY,
				std::size_t endPtX, std::size_t endPtY, std::size_t endCtrlX, std::size_t endCtrlY, double v)
{
	this->bezier(startPtX, startPtY, startCtrlX, startCtrlY, endPtX, endPtY, endCtrlX, endCtrlY, m_cmap.getColor(v));
	return;
}

void 
plot_t::bezier(std::size_t startPtX, std::size_t startPtY, std::size_t startCtrlX, std::size_t startCtrlY,
				std::size_t endPtX, std::size_t endPtY, std::size_t endCtrlX, std::size_t endCtrlY, rgb_t c)
{
	this->bezier(startPtX, startPtY, startCtrlX, startCtrlY, endPtX, endPtY, endCtrlX, endCtrlY, c.r, c.g, c.b);
	return;
}


void 
plot_t::arrow(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2, std::size_t size, double head_angle, QRgb c)
{
	double th		= 3.141592653589793 + head_angle;
	double costh	= std::cos(th);
	double sinth	= std::sin(th);
	double t1		= 0.0;
	double t2		= 0.0; 
	double r		= 0.0;
	double advancex = 0.0;
	double advancey = 0.0;

	this->line(x1, y1, x2, y2, c);
	//   double th = 3.141592653589793 + (head_angle)*3.141592653589793/180.0;  //degrees

	t1 = ( ( x2 - x1 )*costh - ( y2 - y1 )*sinth );
	t2 = ( ( x2 - x1 )*sinth + ( y2 - y1 )*costh );
	r = std::sqrt(t1*t1 + t2*t2);

	advancex = size*t1 / r;
	advancey = size*t2 / r;
	this->line(x2, y2, static_cast< std::size_t >(x2 + advancex), static_cast< std::size_t >(y2 + advancey), c);
	t1 = ( x2 - x1 )*costh + ( y2 - y1 )*sinth;
	t2 = ( y2 - y1 )*costh - ( x2 - x1 )*sinth;

	advancex = size*t1 / r;
	advancey = size*t2 / r;
	this->line(x2, y2, static_cast< std::size_t >( x2 + advancex ), static_cast<std::size_t>( y2 + advancey ), c );

	return;
}

void 
plot_t::arrow(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2, std::size_t size, double head_angle, double r, double g, double b, uint8_t a)
{
	QColor c(r, g, b, a);

	this->arrow(x1, y1, x2, y2, size, head_angle, c.rgba());
	return;
}

void 
plot_t::arrow(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2, std::size_t size, double head_angle, double c)
{
	this->arrow(x1, y1, x2, y2, size, head_angle, m_cmap.getColor(c));
	return;
}

void 
plot_t::arrow(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2, std::size_t size, double head_angle, rgb_t c)
{
	this->arrow(x1, y1, x2, y2, size, head_angle, c.r, c.g, c.b);
	return;
}

void 
plot_t::filledarrow(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2, std::size_t size, double head_angle, QRgb c)
{
	double th = 3.141592653589793 + head_angle;
	double costh = cos(th);
	double sinth = sin(th);
	double t11, t12, t21, t22, r1, r2;
	int p1x, p2x, p3x, p1y, p2y, p3y;

	this->line(x1, y1, x2, y2, c);

	t11 = ( ( x2 - x1 )*costh - ( y2 - y1 )*sinth );
	t21 = ( ( x2 - x1 )*sinth + ( y2 - y1 )*costh );
	t12 = ( x2 - x1 )*costh + ( y2 - y1 )*sinth;
	t22 = ( y2 - y1 )*costh - ( x2 - x1 )*sinth;

	r1 = sqrt(t11*t11 + t21*t21);
	r2 = sqrt(t12*t12 + t22*t22);

	double advancex1 = size*t11 / r1;
	double advancey1 = size*t21 / r1;
	double advancex2 = size*t12 / r2;
	double advancey2 = size*t22 / r2;

	p1x = x2;
	p1y = y2;

	p2x = int(x2 + advancex1);
	p2y = int(y2 + advancey1);

	p3x = int(x2 + advancex2);
	p3y = int(y2 + advancey2);


	this->filledtriangle(p1x, p1y, p2x, p2y, p3x, p3y, c);

}

// drwatop(), drawbottom() and filledtriangle() were contributed by Gurkan Sengun
// ( <gurkan@linuks.mine.nu>, http://www.linuks.mine.nu/ )
void 
plot_t::drawtop(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2, std::size_t x3, QRgb c)
{
	// This swaps x2 and x3
	// if(x2>x3) x2^=x3^=x2^=x3;
	if ( x2>x3 ) {
		x2 ^= x3;
		x3 ^= x2;
		x2 ^= x3;
	}

	long posl = x1 * 256;
	long posr = posl;

	long cl = ( ( x2 - x1 ) * 256 ) / ( y2 - y1 );
	long cr = ( ( x3 - x1 ) * 256 ) / ( y2 - y1 );

	for ( int y = y1; y<y2; y++ ) {
		this->line(posl / 256, y, posr / 256, y, c);
		posl += cl;
		posr += cr;
	}

	return;
}

// drwatop(), drawbottom() and filledtriangle() were contributed by Gurkan Sengun
// ( <gurkan@linuks.mine.nu>, http://www.linuks.mine.nu/ )

void 
plot_t::drawbottom(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t x3, std::size_t y3, QRgb c)
{
	//Swap x1 and x2
	//if(x1>x2) x2^=x1^=x2^=x1;
	if ( x1>x2 ) {
		x2 ^= x1;
		x1 ^= x2;
		x2 ^= x1;
	}

	long posl = x1 * 256;
	long posr = x2 * 256;

	long cl = ( ( x3 - x1 ) * 256 ) / ( y3 - y1 );
	long cr = ( ( x3 - x2 ) * 256 ) / ( y3 - y1 );

	for ( int y = y1; y<y3; y++ ) {
		this->line(posl / 256, y, posr / 256, y, c);

		posl += cl;
		posr += cr;
	}
}

// drwatop(), drawbottom() and filledtriangle() were contributed by Gurkan Sengun
// ( <gurkan@linuks.mine.nu>, http://www.linuks.mine.nu/ )
void 
plot_t::filledtriangle(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2, 
						std::size_t x3, std::size_t y3, QRgb c)
{
	if ( ( x1 == x2 && x2 == x3 ) || ( y1 == y2 && y2 == y3 ) ) return;

	if ( y2<y1 ) {
		// x2^=x1^=x2^=x1;
		x2 ^= x1;
		x1 ^= x2;
		x2 ^= x1;
		// y2^=y1^=y2^=y1;
		y2 ^= y1;
		y1 ^= x2;
		y2 ^= y1;
	}

	if ( y3<y1 ) {
		//x3^=x1^=x3^=x1;
		x3 ^= x1;
		x1 ^= x3;
		x3 ^= x1;
		//y3^=y1^=y3^=y1;
		y3 ^= y1;
		y1 ^= y3;
		y3 ^= y1;
	}

	if ( y3<y2 ) {
		//x2^=x3^=x2^=x3;
		x2 ^= x3;
		x3 ^= x2;
		x2 ^= x3;
		//y2^=y3^=y2^=y3;
		y2 ^= y3;
		y3 ^= y2;
		y2 ^= y3;
	}

	if ( y2 == y3 ) {
		this->drawtop(x1, y1, x2, y2, x3, c);
	} else {
		if ( y1 == y3 || y1 == y2 ) {
			this->drawbottom(x1, y1, x2, x3, y3, c);
		} else {
			int new_x = x1 + (int)( (double)( y2 - y1 )*(double)( x3 - x1 ) / (double)( y3 - y1 ) );
			this->drawtop(x1, y1, new_x, y2, x2, c);
			this->drawbottom(x2, y2, new_x, x3, y3, c);
		}
	}

}

void 
plot_t::cross(std::size_t x, std::size_t y, std::size_t xwidth, std::size_t yheight, QRgb c)
{
	this->line(int(x - xwidth / 2.0), y, int(x + xwidth / 2.0), y, c);
	this->line(x, int(y - yheight / 2.0), x, int(y + yheight / 2.0), c);
}

void
plot_t::maltesecross(std::size_t x, std::size_t y, std::size_t xwidth, 
					std::size_t yheight, std::size_t x_bar_height, std::size_t y_bar_width, QRgb c)
{
	this->line(int(x - xwidth / 2.0), y, int(x + xwidth / 2.0), y, c);
	this->line(x, int(y - yheight / 2.0), x, int(y + yheight / 2.0), c);
	// Bars on ends of vertical line
	this->line(int(x - y_bar_width / 2.0), int(y + yheight / 2.0), int(x + y_bar_width / 2.0), int(y + yheight / 2.0), c);
	this->line(int(x - y_bar_width / 2.0), int(y - yheight / 2.0), int(x + y_bar_width / 2.0), int(y - yheight / 2.0), c);
	// Bars on ends of horizontal line.
	this->line(int(x - xwidth / 2.0), int(y - x_bar_height / 2.0), int(x - xwidth / 2.0), int(y + x_bar_height / 2.0), c);
	this->line(int(x + xwidth / 2.0), int(y - x_bar_height / 2.0), int(x + xwidth / 2.0), int(y + x_bar_height / 2.0), c);
}

void 
plot_t::filleddiamond(std::size_t x, std::size_t y, std::size_t width, std::size_t height, QRgb c)
{
	this->filledtriangle(int(x - width / 2.0), y, x, y, x, int(y + height / 2.0), c);
	this->filledtriangle(int(x + width / 2.0), y, x, y, x, int(y + height / 2.0), c);
	this->filledtriangle(int(x - width / 2.0), y, x, y, x, int(y - height / 2.0), c);
	this->filledtriangle(int(x + width / 2.0), y, x, y, x, int(y - height / 2.0), c);
}

void 
plot_t::diamond(std::size_t x, std::size_t y, std::size_t width, std::size_t height, QRgb c)
{
	this->line(int(x - width / 2.0), y, x, int(y + height / 2.0), c);
	this->line(int(x + width / 2.0), y, x, int(y + height / 2.0), c);
	this->line(int(x - width / 2.0), y, x, int(y - height / 2.0), c);
	this->line(int(x + width / 2.0), y, x, int(y - height / 2.0), c);
}