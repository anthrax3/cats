#include "filter_bank.hpp"

filter_bank_t::filter_bank_t(double scale, double base, double bandwidth, double overlap) :
									m_scale(scale), m_bandwidth(bandwidth*scale),
									m_start(std::max(scale*base - bandwidth / 2, 0.0)),
									m_step(( 1 - overlap )*bandwidth)
{
	if ( 0 > static_cast< int64_t >( m_step ) ) {
		throw std::invalid_argument("One or more parameters were invalid, resulting in a negative step-size");

	}

	return;
}

filter_bank_t::~filter_bank_t(void)
{
	return;
}

std::size_t
filter_bank_t::number_bands_estimate(double mf)
{
	return ( mf * m_scale - m_start ) / m_step;
}

filter_range_t
filter_bank_t::get_band(std::size_t idx) const
{
	filter_range_t o;

	o.first = m_start + idx * m_step;
	o.second = o.first + m_bandwidth;

	return o;
}

std::size_t
filter_bank_t::get_center(std::size_t idx) const
{
	return m_start + idx * m_step + m_bandwidth / 2.0;
}