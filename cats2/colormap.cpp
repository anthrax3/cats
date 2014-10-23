#include "colormap.hpp"

void
colorMap_t::addColorPoint(float r, float g, float b, float v)
{
	for ( std::size_t idx = 0; idx < m_colors.size(); idx++ )
		if ( m_colors[ idx ].val > v ) {
		m_colors.insert(m_colors.begin() + idx, colorPoint_t(r, g, b, v));
		return;
		}

	m_colors.push_back(colorPoint_t(r, g, b, v));
	return;
}

void
colorMap_t::createDefaultMap(void)
{
	m_colors.clear();
	m_colors.push_back(colorPoint_t(0., 0., 0., 0.0f));		// -- black
	m_colors.push_back(colorPoint_t(0., 0., 1., 0.05f));	// -- blue
	m_colors.push_back(colorPoint_t(0., 1., 1., 0.30f));	// -- cyan
	m_colors.push_back(colorPoint_t(0., 1., 0., 0.55f));    // -- green
	m_colors.push_back(colorPoint_t(1., 1., 0., 0.75f));	// -- yellow.
	m_colors.push_back(colorPoint_t(1., 0., 0., 0.85f));	// -- red
	m_colors.push_back(colorPoint_t(1., 1., 1., 1.00f));	// -- white
}

uint8_t
colorMap_t::scaleToRGB(double v)
{
	return static_cast< uint8_t >( v == 1.0 ? 255 : v * 256.0 );
}

rgb_t
colorMap_t::getColor(const double v)
{
	rgb_t ret;

	if ( 0 == m_colors.size() )
		throw std::runtime_error("Impossible error encountered in colorMap_t::getColor()");

	for ( std::size_t idx = 0; idx < m_colors.size(); idx++ ) {
		colorPoint_t& current = m_colors[ idx ];

		if ( v < current.val ) {
			colorPoint_t&	prev = m_colors[static_cast< std::size_t >(std::max(static_cast< signed int >( 0 ), 
																		static_cast< signed int >(idx - 1))) ];
			double			diff = ( prev.val - current.val );
			/// XXX JF FIXME - truncation
			double			frac = ( diff == 0 ) ? 0 : ( v-current.val ) / diff;

			ret.r = ( prev.r - current.r ) * frac + current.r;
			ret.g = ( prev.g - current.g ) * frac + current.g;
			ret.b = ( prev.b - current.b ) * frac + current.b;

			return ret;
		}
	}

	return rgb_t(m_colors.back());
}
