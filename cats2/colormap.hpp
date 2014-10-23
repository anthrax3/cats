#ifndef COLORMAP_HPP
#define COLORMAP_HPP

#include <cstdint>
#include <algorithm>
#include <vector>

// ripped and slightly modified from http://www.andrewnoske.com/wiki/Code_-_heatmaps_and_color_gradients
// tyvm.

struct colorPoint_t
{
	double r, g, b;
	double val;

	colorPoint_t(double red, double green, double blue, double value) : r(red), g(green), b(blue), val(value) { return; }
};

struct rgb_t
{
	double r, g, b;
	rgb_t(void) : r(0.), g(0.), b(0.) { }
	rgb_t(double red, double green, double blue) : r(red), g(green), b(blue) { return; }
	rgb_t(colorPoint_t& p) : r(p.r), g(p.g), b(p.b) { return; }
};

class colorMap_t
{

	private:
	std::vector< colorPoint_t > m_colors;

	public:
	colorMap_t(void) { createDefaultMap(); return; }
	~colorMap_t(void) { m_colors.clear(); return; }

	void addColorPoint(float, float, float, float);
	void clearGradients(void) { m_colors.clear(); return; }
	void createDefaultMap(void);

	rgb_t getColor(const double);
	uint8_t scaleToRGB(double);

};

#endif // COLORMAP_HPP
