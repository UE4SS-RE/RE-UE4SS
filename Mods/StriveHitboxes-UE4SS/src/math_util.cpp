#include "math_util.h"
#include <cmath>
#include <utility>

// Clip a line's Y values to segment within min_x and max_x
static void clip_line_y(
	const FVector2D &line_a, const FVector2D &line_b,
	float min_x, float max_x,
	float *min_y, float *max_y)
{
	const auto delta = line_b - line_a;

	if (abs(delta.X) > FLT_EPSILON) {
		const auto slope = delta.Y / delta.X;
		const auto intercept = line_a.Y - slope * line_a.X;
		*min_y = slope * min_x + intercept;
		*max_y = slope * max_x + intercept;
	} else {
		*min_y = line_a.Y;
		*max_y = line_b.Y;
	}

	if (*min_y > *max_y)
		std::swap(*min_y, *max_y);
}

bool line_box_intersection(
	const FVector2D &box_min, const FVector2D &box_max,
	const FVector2D &line_a, const FVector2D &line_b,
	float *entry_fraction, float *exit_fraction)
{
	// No intersection if line runs along the edge of the box
	if (line_a.X == line_b.X && (line_a.X == box_min.X || line_a.X == box_max.X))
		return false;

	if (line_a.Y == line_b.Y && (line_a.Y == box_min.Y || line_a.Y == box_max.Y))
		return false;

	// Clip X values to segment within box_min.X and box_max.X
	const auto min_x = max_(min_(line_a.X, line_b.X), box_min.X);
	const auto max_x = min_(max_(line_a.X, line_b.X), box_max.X);

	// Check if the line is in the bounds of the box on the X axis
	if (min_x > max_x)
		return false;

	// Clip Y values to segment within min_x and max_x
	float min_y, max_y;
	clip_line_y(line_a, line_b, min_x, max_x, &min_y, &max_y);

	// Clip Y values to segment within box_min.Y and box_max.Y
	min_y = max_(min_y, box_min.Y);
	max_y = min_(max_y, box_max.Y);

	// Check if the clipped line is in the bounds of the box on the Y axis
	if (min_y > max_y)
		return false;

	const FVector2D entry(
		line_a.X < line_b.X ? min_x : max_x,
		line_a.Y < line_b.Y ? min_y : max_y);

	const FVector2D exit(
		line_a.X > line_b.X ? min_x : max_x,
		line_a.Y > line_b.Y ? min_y : max_y);

	const auto length = (line_b - line_a).Size();
	*entry_fraction = (entry - line_a).Size() / length;
	*exit_fraction = (exit - line_a).Size() / length;

	return true;
}