#pragma once

#include <raylib.h>
#include <time.h>
#include <math.h>

namespace Dithering
{
	#define COLOR_CHANEL unsigned char
	//Color struct for FloydSteinberg dithering (uses floats insted of unsigned chars as r, g and b values)
	struct ColorFloat
	{
		float r, g, b;

		//Sets the values to the coresponding values of a normal Color
		void FromColor(const Color& color)
		{
			r = (float)color.r / 255.0f;
			g = (float)color.g / 255.0f;
			b = (float)color.b / 255.0f;
		}

		//Creates a regular Color struct from the data in this structure
		Color ToColor() const
		{
			Color c;
			c.r = r * 255;
			c.g = g * 255;
			c.b = b * 255;
			return c;
		}
	};

	//Random dithering
	void Random(Image &image, bool colored)
	{
		SetRandomSeed(time(0));
		if (!colored)
			ImageColorGrayscale(&image);

		for (int x = 0; x < image.width; x++)
		{
			for (int y = 0; y < image.height; y++)
			{
				Color color = GetImageColor(image, x, y);

				color.r = GetRandomValue(0, 255) < color.r ? 255 : 0;
				color.g = GetRandomValue(0, 255) < color.g ? 255 : 0;
				color.b = GetRandomValue(0, 255) < color.b ? 255 : 0;

				ImageDrawPixel(&image, x, y, color);
			}
		}
	}

	//Template method that accepts any kind of pattern size
	template<int PatternSize>
	static void DitherOrdered(Image& image, COLOR_CHANEL pattern[PatternSize][PatternSize])
	{
		//Check every pixel in the image
		for (int x = 0; x < image.width; x++)
		{
			for (int y = 0; y < image.height; y++)
			{
				Color currentColor = GetImageColor(image, x, y);
				int patternX = x % PatternSize;
				int patternY = y % PatternSize;
				//For each chanel check if the current color value is higher than the value of the pattern at the required index
				currentColor.r = currentColor.r > pattern[patternX][patternY] ? 255 : 0;
				currentColor.g = currentColor.g > pattern[patternX][patternY] ? 255 : 0;
				currentColor.b = currentColor.b > pattern[patternX][patternY] ? 255 : 0;
				//Draw the quantized pixel
				ImageDrawPixel(&image, x, y, currentColor);
			}
		}
	}

	//Orderd dithering using a 2x2 Bayer matrix
	void Ordered2x2(Image& image, bool colored)
	{
		static COLOR_CHANEL pattern[2][2] = {{0, 128}, 
											 {192, 64}};
		if(!colored)
			ImageColorGrayscale(&image);

		DitherOrdered<2>(image, pattern);
	}

	//Orderd dithering using a 4x4 Bayer matrix
	void Ordered4x4(Image& image, bool colored)
	{
		static COLOR_CHANEL pattern[4][4] = {{0, 128, 32, 160},
										 	 {192, 64, 224, 96},
										 	 {48, 176, 16, 144},
										 	 {240, 112, 208, 80}};
		if (!colored)
			ImageColorGrayscale(&image);

		DitherOrdered<4>(image, pattern);
	}

	//Orderd dithering using a 8x8 Bayer matrix
	void Ordered8x8(Image& image, bool colored)
	{
		static COLOR_CHANEL pattern[8][8] = {{0, 128, 32, 160, 8, 136, 40, 168},
											 {192, 64, 224, 96, 200, 72, 232, 104},
										 	 {48, 176, 16, 144, 56, 184, 24, 152},
										 	 {240, 112, 208, 80, 248, 120, 216, 22},
										 	 {12, 140, 44, 172, 4, 132, 36, 164},
										 	 {204, 76, 236, 108, 196, 68, 228, 100},
										 	 {60, 188, 28, 156, 52, 180, 20, 148},
										 	 {252, 124, 220, 92, 244, 116, 212, 84}};
		if (!colored)
			ImageColorGrayscale(&image);

		DitherOrdered<8>(image, pattern);
	}

	//Orderd dithering using a 16x16 Bayer matrix
	void Ordered16x16(Image& image, bool colored)
	{
		static COLOR_CHANEL pattern[16][16] = {{0, 191, 48, 239, 12, 203, 60, 251, 3, 194, 51, 242, 15, 206, 63, 254},
										 	   {127, 64, 175, 112, 139, 76, 187, 124, 130, 67, 178, 115, 142, 79, 190, 127},
										 	   {32, 223, 16, 207, 44, 235, 28, 219, 35, 226, 19, 210, 47, 238, 31, 222},
										 	   {159, 96, 143, 80, 171, 108, 155, 92, 162, 99, 146, 83, 174, 111, 158, 95},
										 	   {8, 199, 56, 247, 4, 195, 52, 243, 11, 202, 59, 250, 7, 198, 55, 246},
										 	   {135, 72, 183, 120, 131, 68, 179, 116, 138, 75, 186, 123, 134, 71, 182, 119},
										 	   {40, 231, 24, 215, 36, 227, 20, 211, 43, 234, 27, 218, 39, 230, 23, 214},
										 	   {167, 104, 151, 88, 163, 100, 147, 84, 170, 107, 154, 91, 166, 103, 150, 87},
										 	   {2, 193, 50, 241, 14, 205, 62, 253, 1, 192, 49, 240, 13, 204, 61, 252},
										 	   {129, 66, 177, 114, 141, 78, 189, 126, 128, 65, 176, 113, 140, 77, 188, 125},
										 	   {34, 225, 18, 209, 46, 237, 30, 221, 33, 224, 17, 208, 45, 236, 29, 220},
										 	   {161, 98, 145, 82, 173, 110, 157, 94, 160, 97, 144, 81, 172, 109, 156, 93},
										 	   {10, 201, 58, 249, 6, 197, 54, 245, 9, 200, 57, 248, 5, 196, 53, 244},
										 	   {137, 74, 185, 122, 133, 70, 181, 118, 136, 73, 184, 121, 132, 69, 180, 117},
										 	   {42, 233, 26, 217, 38, 229, 22, 213, 41, 232, 25, 216, 37, 228, 21, 212},
										 	   {169, 106, 153, 90, 165, 102, 149, 86, 168, 105, 152, 89, 164, 101, 148, 85}};
		if (!colored)
			ImageColorGrayscale(&image);

		DitherOrdered<16>(image, pattern);
	}

	//Error diffusion dithering using Floyd-Steinberg algorithm
	void FloydSteinberg(Image& image, bool colored)
	{
		ColorFloat *colorData = new ColorFloat[image.width * image.height];

		if(!colored)
			ImageColorGrayscale(&image);

		//Copy to float colors
		for (int x = 0; x < image.width; x++)
		{
			for (int y = 0; y < image.height; y++)
				colorData[x + y * image.width].FromColor(GetImageColor(image, x, y));
		}

		//Do dithering
		for (int y = 0; y < image.height; y++)
		{
			for (int x = 0; x < image.width; x++)
			{
				int index = x + y * image.width;
				ColorFloat currentColor = colorData[index];

				float oldR = currentColor.r;
				float oldG = currentColor.g;
				float oldB = currentColor.b;

				float newR = oldR > 0.5f ? 1.0f : 0.0f;
				float newG = oldG > 0.5f ? 1.0f : 0.0f;
				float newB = oldB > 0.5f ? 1.0f : 0.0f;

				colorData[index].r = newR;
				colorData[index].g = newG;
				colorData[index].b = newB;

				ImageDrawPixel(&image, x, y, colorData[index].ToColor());

				float errorR = oldR - newR;
				float errorG = oldG - newG;
				float errorB = oldB - newB;

				if (x + 1 < image.width)
				{
					index = x + 1 + y * image.width;
					colorData[index].r += errorR * 7 / 16.0f;
					colorData[index].g += errorG * 7 / 16.0f;
					colorData[index].b += errorB * 7 / 16.0f;
				}

				if (y + 1 < image.height)
				{
					if (x - 1 >= 0)
					{
						index = x - 1 + (y + 1) * image.width;
						colorData[index].r += errorR * 3 / 16.0f;
						colorData[index].g += errorG * 3 / 16.0f;
						colorData[index].b += errorB * 3 / 16.0f;
					}					

					index = x + (y + 1) * image.width;
					colorData[index].r += errorR * 5 / 16.0f;
					colorData[index].g += errorG * 5 / 16.0f;
					colorData[index].b += errorB * 5 / 16.0f;

					if (x + 1 < image.height)
					{
						index = x + 1 + (y + 1) * image.width;
						colorData[index].r += errorR * 1 / 16.0f;
						colorData[index].g += errorG * 1 / 16.0f;
						colorData[index].b += errorB * 1 / 16.0f;
					}
				}
			}
		}
		delete[] colorData;
	}
}