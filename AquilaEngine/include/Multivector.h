#pragma once

#include <PrecompiledHeader.h>

template<typename T, unsigned int  N>
struct multi_vector {

	using vec_type = std::vector<T>;

	
	void push_back(T & thing) { get_vector().push_back(thing); };

	
	vec_type& get_vector() { return vectors[GetCurrentProcessorNumber()]; };

	
	vec_type& get_vector(unsigned int n) { return vectors[n]; };

	void clear_all()
	{
		for (auto & a : vectors)
		{
			a.clear();
		}
	}
	size_t size()
	{
		size_t s = 0;
		for (auto & a : vectors)
		{
			s += a.size();
		}
		return s;
	}
	T & operator[](size_t i)
	{
		for (auto & a : vectors)
		{
			if (i < a.size())
			{
				return a[i];
			}
			else
			{
				i -= a.size();
			}
		}
	}
	std::array<vec_type, N> vectors;
private:
	
};