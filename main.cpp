#include <iostream>
#include <vector>
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
};

struct Extent2D {
	uint32_t width{};
	uint32_t height{};
};

float mitchell_netravali(float x, [[maybe_unused]]float r) {

	float s{ 4.0 };

	float abs_x{ std::fabs(x) / s };

	float B = 1.0f / 3.0f;
	float C = 1.0f / 3.0f;

	if (abs_x <= 1) {
		return ((12 - 9 * B - 6 * C) * abs_x * abs_x * abs_x +
			(-18 + 12 * B + 6 * C) * abs_x * abs_x + (6 - 2 * B)) /	6 / s;
	}
	else if ((1 <= abs_x) && (abs_x <= 2)) {
		return ((-B - 6 * C) * abs_x * abs_x * abs_x + (6 * B + 30 * C) * abs_x * abs_x +
			(-12 * B - 48 * C) * abs_x + (8 * B + 24 * C)) / 6 / s;
	}

	return 0.0f;
}

/*		image is 8 bits per channel, 24 bits per pixel		*/
int main()
{

	// read image data in
	int x, y;
	unsigned char* data{ stbi_load("images/red_panda.jpg", &x, &y, nullptr, STBI_rgb) };

	if (!data) {
		std::cout << stbi_failure_reason();
	}

	Pixel* image_data{ reinterpret_cast<Pixel*>(data) };
	const Extent2D imageExtent{static_cast<uint32_t>(x),static_cast<uint32_t>(y)};
	const Extent2D desired_image_extent{ imageExtent.width / 4, imageExtent.height / 4 };
	std::cout << "desired samples: " << desired_image_extent.width << " x " << desired_image_extent.height << '\n';

	float delta_x{ static_cast<float>(imageExtent.width) / desired_image_extent.width };
	float delta_y{ static_cast<float>(imageExtent.height) / desired_image_extent.height };

	std::cout << "delta_x: " << delta_x << '\n';
	std::cout << "delta_y: " << delta_y << '\n';

	void* resampledImage{ malloc(desired_image_extent.width * desired_image_extent.height * sizeof(Pixel)) };
	if (resampledImage == nullptr) {
		std::cout << "Failed to allocate resample buffer" << '\n';
		return EXIT_FAILURE;
	}
	Pixel* resampledImageData{ reinterpret_cast<Pixel*>(resampledImage) };

	for (std::size_t i{ 0 }; i < desired_image_extent.height; ++i) { // row
		std::size_t _x{ static_cast<std::size_t>(i * delta_x + delta_x/2) };
		//std::cout << "_x: " << _x << '\n';
		for (std::size_t j{ 0 }; j < desired_image_extent.width; ++j) { // column
			std::size_t _y{ static_cast<std::size_t>(j * delta_y + delta_y/2) };

			//resampledImageData[i * desired_image_extent.width + j] = image_data[_x * imageExtent.width + _y];

			Pixel pixel{};
			float normFactor{ 0 };

			for (std::size_t i_prime{ _x - 2 }; i_prime <= _x + 2; ++i_prime) {
				for (std::size_t j_prime{ _y - 2 }; j_prime <= _y + 2; ++j_prime) {
					if ((i_prime >= 0 && i_prime < imageExtent.height) && (j_prime >= 0 && j_prime <= imageExtent.width)) {
						float weight{ mitchell_netravali(_x - static_cast<float>(i_prime), 2) * mitchell_netravali(_y - static_cast<float>(j_prime), 2) };
						normFactor += weight;
						pixel.r += static_cast<uint8_t>(image_data[i_prime * imageExtent.width + j_prime].r * weight);
						pixel.g += static_cast<uint8_t>(image_data[i_prime * imageExtent.width + j_prime].g * weight);
						pixel.b += static_cast<uint8_t>(image_data[i_prime * imageExtent.width + j_prime].b * weight);
					}
				}
			}

			pixel.r = static_cast<uint8_t>(pixel.r / normFactor);
			pixel.g = static_cast<uint8_t>(pixel.g / normFactor);
			pixel.b = static_cast<uint8_t>(pixel.b / normFactor);
			resampledImageData[i * desired_image_extent.width + j] = pixel;
		}
	}

	stbi_write_jpg("red_panda_export.jpg", static_cast<int>(desired_image_extent.width), static_cast<int>(desired_image_extent.height), 3, reinterpret_cast<unsigned char*>(resampledImage), 100);
	stbi_image_free(data);
	free(resampledImage);

	return 0;
}