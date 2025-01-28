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

	AllocatedImage image;
	//vk::UniqueImageView view;
	vk::Extent3D extent;
	vk::Format format;
	vk::ImageLayout layout;
};


Texture::Texture(Texture&& rhs)
{
	std::swap(image, rhs.image);
	std::swap(extent, rhs.extent);
	std::swap(format, rhs.format);
}

Texture& Texture::operator=(Texture&& rhs)
{
	std::swap(image, rhs.image);
	std::swap(extent, rhs.extent);
	std::swap(format, rhs.format);
	return *this;
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

	Texture texture;
	texture.format = vk::Format::eR8G8B8A8Srgb;
	texture.extent = vk::Extent3D{}
		.setWidth(bitmap.width)
		.setHeight(bitmap.height)
		.setDepth(1);

	texture.image = allocate_image(physical_device,
								   device,
								   texture.extent,
								   texture.format,
								   vk::ImageTiling::eOptimal,
								   propertyFlags);
	

	with_buffer_submit(device, command_pool, queue,
					   [&] (vk::CommandBuffer& commandbuffer)
					   {
						   texture.layout =
							   transition_image_for_buffer_write(texture.image.image.get(),
																 commandbuffer);

						   copy_buffer_to_image(staging.buffer.get(),
												texture.image.image.get(),
												texture.extent.width,
												texture.extent.height,
												commandbuffer);
					   });
	return texture;
}
