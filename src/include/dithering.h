#pragma once

#include <raylib.h>

// All implemented dithering algorithms
namespace Dithering
{
	// Random dithering
	void Random(Image &image, bool colored);
	// Orderd dithering using a 2x2 Bayer matrix
	void Ordered2x2(Image &image, bool colored);
	// Orderd dithering using a 4x4 Bayer matrix
	void Ordered4x4(Image &image, bool colored);
	// Orderd dithering using a 8x8 Bayer matrix
	void Ordered8x8(Image &image, bool colored);
	// Orderd dithering using a 16x16 Bayer matrix
	void Ordered16x16(Image &image, bool colored);
	// Error diffusion dithering using Floyd-Steinberg algorithm
	void FloydSteinberg(Image &image, bool colored);
}