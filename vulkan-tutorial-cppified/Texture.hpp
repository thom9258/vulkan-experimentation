#pragma once

#include <vulkan/vulkan.hpp>

#include "Bitmap.hpp"

#include <iostream>
	
struct Texture
{
	explicit Texture() = default;
	~Texture() = default;

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	Texture(Texture&& texture);
	Texture& operator=(Texture&& texture);

	AllocatedImage allocated;
	vk::Extent3D extent;
	vk::Format format;
	vk::ImageLayout layout;
};


Texture::Texture(Texture&& rhs)
{
	std::swap(allocated, rhs.allocated);
	std::swap(extent, rhs.extent);
	std::swap(format, rhs.format);
}

Texture& Texture::operator=(Texture&& rhs)
{
	std::swap(allocated, rhs.allocated);
	std::swap(extent, rhs.extent);
	std::swap(format, rhs.format);
	return *this;
}

Texture
create_empty_texture(vk::PhysicalDevice& physical_device,
					 vk::Device& device,
					 const vk::Format format,
					 const vk::Extent3D extent,
					 const vk::ImageTiling tiling,
					 const vk::MemoryPropertyFlags propertyFlags)
{
	Texture texture{};
	texture.format = format;
	texture.extent = extent;
	texture.layout = vk::ImageLayout::eUndefined;
	texture.allocated = allocate_image(physical_device,
									   device,
									   extent,
									   format,
									   tiling,
									   propertyFlags);
	return texture;
}

Texture
copy_bitmap_to_gpu(vk::PhysicalDevice& physical_device,
				   vk::Device& device,
				   vk::CommandPool& command_pool,
				   vk::Queue& queue,
				   const vk::MemoryPropertyFlags propertyFlags,
				   const Bitmap2D& bitmap)
{
	AllocatedMemory staging = create_staging_buffer(physical_device,
													device,
													bitmap.pixels,
													bitmap.memory_size());
	const auto extent = vk::Extent3D{}
		.setWidth(bitmap.width)
		.setHeight(bitmap.height)
		.setDepth(1);

	Texture texture = create_empty_texture(physical_device,
										   device,
										   vk::Format::eR8G8B8A8Srgb,
										   extent,
										   vk::ImageTiling::eOptimal,
										   propertyFlags);

	with_buffer_submit(device, command_pool, queue,
					   [&] (vk::CommandBuffer& commandbuffer)
					   {
						   texture.layout =
							   transition_image_color_override(texture.allocated.image.get(),
															   commandbuffer);

						   copy_buffer_to_image(staging.buffer.get(),
												texture.allocated.image.get(),
												texture.extent.width,
												texture.extent.height,
												commandbuffer);
					   });
	return texture;
}
