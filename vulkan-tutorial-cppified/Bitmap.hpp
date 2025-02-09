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

struct LoadedBitmap2D
{
	LoadedBitmap2D() = default;
	~LoadedBitmap2D();
	LoadedBitmap2D(const LoadedBitmap2D&) = delete;
	LoadedBitmap2D& operator=(const LoadedBitmap2D&) = delete;
	LoadedBitmap2D(LoadedBitmap2D&& rhs);
	LoadedBitmap2D& operator=(LoadedBitmap2D&& rhs);

	size_t memory_size() const noexcept;
	
	BitmapPixelFormat format;
	int width{0};
	int height{0};
	int channels{0};
	stbi_uc* pixels{nullptr};
};

LoadedBitmap2D::LoadedBitmap2D(LoadedBitmap2D&& rhs) {
	std::swap(format, rhs.format);
	std::swap(width, rhs.width);
	std::swap(height, rhs.height);
	std::swap(channels, rhs.channels);
	std::swap(pixels, rhs.pixels);
}

LoadedBitmap2D& LoadedBitmap2D::operator=(LoadedBitmap2D&& rhs) {
	std::swap(format, rhs.format);
	std::swap(width, rhs.width);
	std::swap(height, rhs.height);
	std::swap(channels, rhs.channels);
	std::swap(pixels, rhs.pixels);
	return *this;
}

template <typename F>
decltype(auto) operator|(LoadedBitmap2D&& bitmap, F&& f)
{
	return std::invoke(std::forward<F>(f), std::move(bitmap));
}

size_t
LoadedBitmap2D::memory_size() const noexcept
{
    return width * height * 4;
}

LoadedBitmap2D::~LoadedBitmap2D()
{
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

struct InvalidNativePixels
{
};

std::variant<LoadedBitmap2D,
			 InvalidPath,
			 InvalidNativePixels,
			 LoadError>
load_bitmap(const std::filesystem::path& path, const BitmapPixelFormat format) noexcept
{
	if (!std::filesystem::exists(path))
		return InvalidPath{path};
	
	LoadedBitmap2D bitmap;
	bitmap.format = format;
    bitmap.pixels = stbi_load(path.string().c_str(),
							  &bitmap.width,
							  &bitmap.height,
							  &bitmap.channels,
							  BitmapPixelFormatToSTBIFormat(format));
	
    if (!bitmap.pixels)
		return LoadError{stbi_failure_reason()};

	return bitmap;
}

uint8_t*
get_pixels(const LoadedBitmap2D& bitmap)
{
	return bitmap.pixels;
}
