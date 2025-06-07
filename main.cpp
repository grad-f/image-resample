#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning(push, 0)
#pragma warning(disable : 26819 6262)
#include "stb_image/stb_image.h"
#include "stb_image/stb_image_write.h"
#pragma warning(pop)

struct Pixel {
	uint8_t r{};
	uint8_t g{};
	uint8_t b{};
	
	Pixel& operator+=(const Pixel& rhs) {
		this->r += rhs.r;
		this->g += rhs.g;
		this->b += rhs.b;
		return *this;
	}

	Pixel& operator/=(const float rhs) {
		this->r = static_cast<uint8_t>(this->r / rhs);
		this->g = static_cast<uint8_t>(this->g / rhs);
		this->b = static_cast<uint8_t>(this->b / rhs);
		return *this;
	}

};

Pixel operator*(const Pixel& lhs, const float rhs) {
	return Pixel{ static_cast<uint8_t>(lhs.r * rhs), static_cast<uint8_t>(lhs.g * rhs), static_cast<uint8_t>(lhs.b * rhs) };
}

struct Extent2D {
	int width{};
	int height{};
};

// scale mitchell_netravali filter based on whether we are upsampling or downsampling
int compute_radius(Extent2D in_image_size, Extent2D out_image_size) {

	int in_pixel_count{ in_image_size.width * in_image_size.height };
	int out_pixel_count{ out_image_size.width * out_image_size.height };
	
	return in_pixel_count <= out_pixel_count ? 1 : 3;
}

float mitchell_netravali(float x, int radiusFactor) {

	//negative lobes
	float radius_factor{ radiusFactor + 0.1f };

	float abs_x{ std::fabs(x / radiusFactor) };

	float B = 1.0f / 3.0f;
	float C = 1.0f / 3.0f;

	if (abs_x <= 1) {
		return ((12 - 9 * B - 6 * C) * abs_x * abs_x * abs_x +
			(-18 + 12 * B + 6 * C) * abs_x * abs_x + (6 - 2 * B)) /	6 / radius_factor;
	}
	else if ((1 <= abs_x) && (abs_x <= 2)) {
		return ((-B - 6 * C) * abs_x * abs_x * abs_x + (6 * B + 30 * C) * abs_x * abs_x +
			(-12 * B - 48 * C) * abs_x + (8 * B + 24 * C)) / 6 / radius_factor;
	}

	return 0.0f;
}

/*		image is 8 bits per channel, 24 bits per pixel		*/
int main()
{

	// read image data in
	int x{}, y{};
	// load image and reinterpret cast to Pixel* to improve code readability (avoid pointer arithmetic with magic numbers)
	Pixel* in_image{ reinterpret_cast<Pixel*>(stbi_load("images/red_panda.jpg", &x, &y, nullptr, STBI_rgb)) };

	// check if image load failed
	if (!in_image) {
		std::cout << stbi_failure_reason();
	}

	// set input and output image extent
	float scale_factor{ 1.0f/3.0f };
	const Extent2D in_image_extent{ x, y };
	const Extent2D out_image_extent{ static_cast<int>(scale_factor * in_image_extent.width), static_cast<int>(scale_factor * in_image_extent.height) };

	// compute filter radius
	int filter_radius{ compute_radius(in_image_extent, out_image_extent) };

	Pixel* out_image{ reinterpret_cast<Pixel*>(malloc(out_image_extent.width * out_image_extent.height * sizeof(Pixel))) };
	Pixel* S{ reinterpret_cast<Pixel*>(malloc(out_image_extent.width * in_image_extent.height * sizeof(Pixel))) };

	if (out_image == nullptr || S == nullptr) {
		std::cout << "Failed to allocate resample buffer" << '\n';
		return EXIT_FAILURE;
	}

	// compute new sample spacing
	float delta_x{ static_cast<float>(in_image_extent.width) / out_image_extent.width };
	float delta_y{ static_cast<float>(in_image_extent.height) / out_image_extent.height };

	/*	DEBUG	*/
	{
		std::cout << "resampling input image: " << in_image_extent.width << " by " << in_image_extent.height;
		std::cout << " to " << out_image_extent.width << " by " << out_image_extent.height << '\n';
		std::cout << "delta_x: " << delta_x << '\n';	std::cout << "delta_y: " << delta_y << '\n';
	}

	// horizontal pass
	for (int i{ 0 }; i < in_image_extent.height; ++i) {
		for (int j{ 0 }; j < out_image_extent.width; ++j) {

			// relative position in input image
			float _x{ j * delta_x + delta_x / 2.0f };
			int _x_ceil{ static_cast<int>(std::ceil(_x)) };
			S[i * out_image_extent.width + j] = Pixel{ 0,0,0 };
			float normalize_factor{ 0 };

			for (int j_prime{ _x_ceil - filter_radius }; j_prime <= _x + filter_radius; ++j_prime) {
				if (j_prime >= 0 && j_prime < in_image_extent.width) {
					float weight{ mitchell_netravali(_x - j_prime, filter_radius) };
					S[i * out_image_extent.width + j] += in_image[i * in_image_extent.width + j_prime] * weight;
					normalize_factor += weight;
				}
			}

			S[i * out_image_extent.width + j] /= normalize_factor;
		}
	}
	//vertical pass
	for (int i{ 0 }; i < out_image_extent.height; ++i) {
		float _y{ i * delta_y + delta_y / 2.0f };
		int _y_ceil{ static_cast<int>(std::ceil(_y)) };
		for (int j{ 0 }; j < out_image_extent.width; ++j) {

			// relative position in input image
 			out_image[i * out_image_extent.width + j] = Pixel{ 0,0,0 };
			float normalize_factor{ 0 };

			for (int i_prime{ _y_ceil - filter_radius }; i_prime <= _y + filter_radius; ++i_prime) {
				if (i_prime >= 0 && i_prime < in_image_extent.height) {
					float weight{ mitchell_netravali(_y - i_prime, filter_radius) };
					out_image[i * out_image_extent.width + j] += S[i_prime * out_image_extent.width + j] * weight;
					normalize_factor += weight;
				}
			}
			out_image[i * out_image_extent.width + j] /= normalize_factor;
		}
	}

	stbi_write_jpg("red_panda_cubic.jpg", static_cast<int>(out_image_extent.width), static_cast<int>(out_image_extent.height), 3, reinterpret_cast<void*>(out_image), 100);
	stbi_image_free(reinterpret_cast<void*>(in_image));
	free(reinterpret_cast<void*>(out_image));
	free(reinterpret_cast<void*>(S));

	return 0;
}