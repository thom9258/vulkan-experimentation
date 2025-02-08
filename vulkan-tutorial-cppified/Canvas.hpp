#pragma once

struct Pixel8bitRGBA
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

struct Canvas8bitRGBA
{
	Canvas8bitRGBA() = default;
	~Canvas8bitRGBA() = default;
	Canvas8bitRGBA(const Canvas8bitRGBA&) = delete;
	Canvas8bitRGBA& operator=(const Canvas8bitRGBA&) = delete;
	Canvas8bitRGBA(Canvas8bitRGBA&& rhs);
	Canvas8bitRGBA& operator=(Canvas8bitRGBA&& rhs);

	size_t memory_size() const noexcept;

	std::optional<std::reference_wrapper<Pixel8bitRGBA>>
	at(const uint32_t x, const uint32_t y) noexcept;

	uint32_t width{0};
	uint32_t height{0};
	std::vector<Pixel8bitRGBA> pixels;
};

Canvas8bitRGBA::Canvas8bitRGBA(Canvas8bitRGBA&& rhs) {
	std::swap(width, rhs.width);
	std::swap(height, rhs.height);
	std::swap(pixels, rhs.pixels);
}

Canvas8bitRGBA& Canvas8bitRGBA::operator=(Canvas8bitRGBA&& rhs) {
	std::swap(width, rhs.width);
	std::swap(height, rhs.height);
	std::swap(pixels, rhs.pixels);
	return *this;
}

size_t
Canvas8bitRGBA::memory_size() const noexcept
{
    return width * height * sizeof(decltype(pixels)::value_type);
}

std::optional<std::reference_wrapper<Pixel8bitRGBA>>
Canvas8bitRGBA::at(const uint32_t x, const uint32_t y) noexcept
{
	if (x < width && y < height) {
		return pixels[x*width + y];
	}
	return std::nullopt;
}


uint8_t*
get_pixels(Canvas8bitRGBA& canvas)
{
	return reinterpret_cast<uint8_t*>(canvas.pixels.data());
}

uint8_t const*
get_pixels(const Canvas8bitRGBA& canvas)
{
	return reinterpret_cast<uint8_t const*>(canvas.pixels.data());
}

[[nodiscard]]
Canvas8bitRGBA
create_canvas(const Pixel8bitRGBA color,
			  const uint32_t width,
			  const uint32_t height)
{
	Canvas8bitRGBA canvas{};
	canvas.width = width;
	canvas.height = height;
	canvas.pixels.resize(width * height, color);
	return canvas;
}

[[nodiscard]]
Canvas8bitRGBA
draw_rectangle(const Pixel8bitRGBA& color,
			   const uint32_t x,
			   const uint32_t y,
			   const uint32_t w,
			   const uint32_t h,
			   Canvas8bitRGBA&& canvas)
{
	for (uint32_t dw = 0; dw < w; dw++) {
		for (uint32_t dh = 0; dh < h; dh++) {
			if (dw + x < canvas.width && dh + y < canvas.height) {
				canvas.at(dw + x, dh + y).value().get() = color;
			}
		}
	}

	return canvas;
}

[[nodiscard]]
Canvas8bitRGBA
draw_checkerboard(const Pixel8bitRGBA& color,
				  const uint32_t size,
				  Canvas8bitRGBA&& canvas)
{
	bool apply_height_offset = false;
	for (uint32_t h = 0; h < canvas.height; h += size) {
		for (uint32_t w = 0; w < canvas.width; w += size*2) {
			if (apply_height_offset)
				canvas = draw_rectangle(color, w + size, h, size, size, std::move(canvas));
			else
				canvas = draw_rectangle(color, w, h, size, size, std::move(canvas));
		}
		apply_height_offset = !apply_height_offset;
	}
	return canvas;
}





template <typename F> requires
requires (F&& f, Canvas8bitRGBA&& canvas) 
{
	{ std::invoke(std::forward<F>(f), std::move(canvas)) } -> std::same_as<Canvas8bitRGBA>;
}
Canvas8bitRGBA operator|(Canvas8bitRGBA&& canvas, F&& f)
{
	return std::invoke(std::forward<F>(f), std::move(canvas));
}

struct draw_checkerboard_p
{
	const Pixel8bitRGBA color;
	const uint32_t size;

	draw_checkerboard_p(const Pixel8bitRGBA color, const uint32_t size)
		: color(color), size(size) 
	{}
	
	Canvas8bitRGBA
	operator()(Canvas8bitRGBA&& canvas)
	{
		return draw_checkerboard(color, size, std::move(canvas));
	}
};

draw_checkerboard_p
draw_checkerboard(const Pixel8bitRGBA& color, const uint32_t size)
{
	return draw_checkerboard_p(color, size);
}


struct draw_rectangle_p
{
	const Pixel8bitRGBA color;
	const uint32_t x;
	const uint32_t y;
	const uint32_t w;
	const uint32_t h;

	draw_rectangle_p(const Pixel8bitRGBA color,
					 const uint32_t x, const uint32_t y,
					 const uint32_t w, const uint32_t h)
		: color(color), x(x), y(y), w(w), h(h)
	{}
	
	Canvas8bitRGBA
	operator()(Canvas8bitRGBA&& canvas)
	{
		return draw_rectangle(color, x, y, w, h, std::move(canvas));
	}
};
draw_rectangle_p
draw_rectangle(const Pixel8bitRGBA& color,
					 const uint32_t x, const uint32_t y,
					 const uint32_t w, const uint32_t h)
{
	return draw_rectangle_p(color, x, y, w, h);
}
