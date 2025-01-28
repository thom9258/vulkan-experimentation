#pragma once
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#include "stb_image.h"

#include <filesystem>

enum class BitmapPixelFormat 
{
	RGBA,
};

constexpr uint32_t
BitmapPixelFormatToSTBIFormat(const BitmapPixelFormat format) noexcept
{
	switch (format) {	
	case BitmapPixelFormat::RGBA:
		return STBI_rgb_alpha;
	default:
		break;
	};
	return STBI_rgb_alpha;
}

struct Bitmap2D
{
	Bitmap2D() = default;
	~Bitmap2D();

	Bitmap2D(const Bitmap2D&) = delete;
	Bitmap2D& operator=(const Bitmap2D&) = delete;

	//todo the move operators should swap data, to ensure existing data is freed aswell
	Bitmap2D(Bitmap2D&& rhs) {
		std::swap(width, rhs.width);
		std::swap(height, rhs.height);
		std::swap(channels, rhs.channels);
		std::swap(pixels, rhs.pixels);
	}

	Bitmap2D& operator=(Bitmap2D&& rhs) {
		std::swap(width, rhs.width);
		std::swap(height, rhs.height);
		std::swap(channels, rhs.channels);
		std::swap(pixels, rhs.pixels);
		return *this;
	}

	size_t memory_size() const noexcept;
	
	BitmapPixelFormat pixel_format;
	int width{0};
	int height{0};
	int channels{0};
	stbi_uc* pixels{nullptr};
};

size_t
Bitmap2D::memory_size() const noexcept
{
	// TODO: 4 should be channels i guess?
    return width * height * 4;
}

Bitmap2D::~Bitmap2D()
{
	if (pixels)
		stbi_image_free(pixels);
}

struct InvalidPath
{
	std::filesystem::path path;
};

struct LoadError
{
	const char* why{nullptr};
};

using LoadedBitmap = std::variant<Bitmap2D,
								  InvalidPath,
								  LoadError>;

LoadedBitmap 
load_bitmap(const std::filesystem::path& path, const BitmapPixelFormat format) noexcept
{
	//std::cout << "Image path: " << path.string() << std::endl;
	if (!std::filesystem::exists(path))
		return InvalidPath{path};
	
	Bitmap2D bitmap;
	bitmap.pixel_format = format;
    //bitmap.pixels = stbi_load("../texture.jpg",
    bitmap.pixels = stbi_load(path.string().c_str(),
							  &bitmap.width,
							  &bitmap.height,
							  &bitmap.channels,
							  BitmapPixelFormatToSTBIFormat(format));
	
    if (!bitmap.pixels)
		return LoadError{stbi_failure_reason()};

	return bitmap;
}
