#pragma once

template<size_t N>
class bitmask {
	unsigned int words[(N + 31) / 32];

public:
	bool get(int bit)
	{
		return words[bit / 32] >> (bit % 32);
	}

	void set(int bit, bool value)
	{
		if (value)
			words[bit / 32] |= 1 << (bit % 32);
		else
			words[bit / 32] &= ~(1 << (bit % 32));
	}
};
