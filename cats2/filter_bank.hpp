#ifndef HAVE_FILTER_BANK_HPP
#define HAVE_FILTER_BANK_HPP

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <algorithm>

#include "window.hpp"

typedef std::pair< std::size_t, std::size_t > filter_range_t;

class filter_bank_t
{
	private:
		const double		m_bandwidth;
		const std::size_t	m_start; 
		const double		m_step;
		const double		m_scale;

	protected:
	public:
		filter_bank_t(double, double, double, double);
		virtual ~filter_bank_t(void);
		std::size_t number_bands_estimate(double);
		filter_range_t get_band(std::size_t) const;
		std::size_t get_center(std::size_t idx) const;

};

#endif