#pragma once
#include <cmath>

struct FVector2D {
	float X, Y;

	FVector2D() : X(0), Y(0) {}
	FVector2D(float X, float Y) : X(X), Y(Y) {}
	FVector2D(int X, int Y) : X((float)X), Y((float)Y) {}

	FVector2D Rotate(const float angle) const
	{
		const auto ca = cosf(angle);
		const auto sa = sinf(angle);
		return FVector2D(X * ca + Y * -sa, X * sa + Y * ca);
	}

	float SizeSquared() const
	{
		return X * X + Y * Y;
	}

	float Size() const
	{
		return sqrt(SizeSquared());
	}

	FVector2D operator+(const FVector2D& other) const
	{
		return FVector2D(X + other.X, Y + other.Y);
	}

	FVector2D operator-(const FVector2D& other) const
	{
		return FVector2D(X - other.X, Y - other.Y);
	}

	FVector2D operator*(float scalar) const
	{
		return FVector2D(X * scalar, Y * scalar);
	}

	FVector2D operator/(float scalar) const
	{
		return FVector2D(X / scalar, Y / scalar);
	}
};

template<typename T>
T min_(T a, T b)
{
	return a < b ? a : b;
}

template<typename T>
T max_(T a, T b)
{
	return a > b ? a : b;
}

bool line_box_intersection(
	const FVector2D &box_min, const FVector2D&box_max,
	const FVector2D&line_a, const FVector2D&line_b,
	float *entry_fraction, float *exit_fraction);
