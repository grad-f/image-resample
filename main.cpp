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


/*		image is 8 bits per channel, 24 bits per pixel		*/
int main()
{
	// read image data in
	int x, y;
	unsigned char* data{ stbi_load("images/red_panda.jpg", &x, &y, nullptr, STBI_rgb) };

	if (stbi_failure_reason()) {
		std::cout << stbi_failure_reason();
	}

	Pixel* image_data{ reinterpret_cast<Pixel*>(data) };
	const Extent2D imageExtent{static_cast<uint32_t>(x),static_cast<uint32_t>(y)};
	const Extent2D desired_image_extent{ imageExtent.width / 4, imageExtent.height / 4 };
	std::cout << "desired samples: " << desired_image_extent.width << " x " << desired_image_extent.height << '\n';

	uint32_t delta_x{ imageExtent.width / desired_image_extent.width };
	uint32_t delta_y{ imageExtent.height / desired_image_extent.height };

	std::cout << "delta_x: " << delta_x << '\n';
	std::cout << "delta_y: " << delta_y << '\n';

	void* resampledImage{ malloc(desired_image_extent.width * desired_image_extent.height * sizeof(Pixel)) };
	if (resampledImage == nullptr) {
		std::cout << "Failed to allocate resample buffer" << '\n';
		return EXIT_FAILURE;
	}
	Pixel* resampledImageData{ reinterpret_cast<Pixel*>(resampledImage) };

	for (std::size_t i{ 0 }; i < desired_image_extent.height; ++i) { // row
		std::size_t _x{ i * delta_x };
		std::cout << "_x: " << _x << '\n';
		for (std::size_t j{ 0 }; j < desired_image_extent.width; ++j) { // column
			std::size_t _y{ j * delta_y };
			resampledImageData[i * desired_image_extent.width + j] = image_data[_x * imageExtent.width + _y];
		}
	}

	stbi_write_jpg("red_panda_export.jpg", static_cast<int>(desired_image_extent.width), static_cast<int>(desired_image_extent.height), 3, reinterpret_cast<unsigned char*>(resampledImage), 100);
	stbi_image_free(data);
	free(resampledImage);

	return 0;
}