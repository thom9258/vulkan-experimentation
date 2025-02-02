#pragma once

#include <vulkan/vulkan.hpp>

#include "Bitmap.hpp"

#include <iostream>
	
struct Texture2D
{
	explicit Texture2D() = default;
	~Texture2D() = default;

	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;

	Texture2D(Texture2D&& texture) noexcept;
	Texture2D& operator=(Texture2D&& texture) noexcept;

	AllocatedImage allocated;
	vk::Extent3D extent;
	vk::Format format;
	vk::ImageLayout layout;
};

Texture2D::Texture2D(Texture2D&& rhs) noexcept
{
	std::swap(allocated, rhs.allocated);
	std::swap(extent, rhs.extent);
	std::swap(format, rhs.format);
	std::swap(layout, rhs.layout);
}

Texture2D& Texture2D::operator=(Texture2D&& rhs) noexcept
{
	std::swap(allocated, rhs.allocated);
	std::swap(extent, rhs.extent);
	std::swap(format, rhs.format);
	std::swap(layout, rhs.layout);
	return *this;
}

vk::Image&
get_image(Texture2D& texture)
{
	return get_image(texture.allocated);
}

Texture2D
create_empty_texture(vk::PhysicalDevice& physical_device,
					 vk::Device& device,
					 const vk::Format format,
					 const vk::Extent3D extent,
					 const vk::ImageTiling tiling,
					 const vk::MemoryPropertyFlags propertyFlags,
					 const vk::ImageUsageFlags usageFlags)
{
	Texture2D texture{};
	texture.format = format;
	texture.extent = extent;
	texture.layout = vk::ImageLayout::eUndefined;
	texture.allocated = allocate_image(physical_device,
									   device,
									   extent,
									   format,
									   tiling,
									   propertyFlags,
									   usageFlags);
	return texture;
}

Texture2D
create_empty_general_texture(vk::PhysicalDevice& physical_device,
							 vk::Device& device,
							 const vk::Format format,
							 const vk::Extent3D extent,
							 const vk::ImageTiling tiling,
							 const vk::MemoryPropertyFlags propertyFlags)
{
	return create_empty_texture(physical_device,
								device,
								format,
								extent,
								tiling,
								propertyFlags,
								vk::ImageUsageFlagBits::eTransferDst
								| vk::ImageUsageFlagBits::eTransferSrc
								| vk::ImageUsageFlagBits::eSampled);
}

Texture2D
create_empty_rendertarget_texture(vk::PhysicalDevice& physical_device,
								  vk::Device& device,
								  const vk::Format format,
								  const vk::Extent3D extent,
								  const vk::ImageTiling tiling,
								  const vk::MemoryPropertyFlags propertyFlags)
{
	return create_empty_texture(physical_device,
								device,
								format,
								extent,
								tiling,
								propertyFlags,
								vk::ImageUsageFlagBits::eTransferDst
								| vk::ImageUsageFlagBits::eTransferSrc
								| vk::ImageUsageFlagBits::eSampled
								| vk::ImageUsageFlagBits::eColorAttachment);
}

Texture2D
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

	Texture2D texture = create_empty_general_texture(physical_device,
													 device,
													 vk::Format::eR8G8B8A8Srgb,
													 extent,
													 vk::ImageTiling::eOptimal,
													 propertyFlags);

	with_buffer_submit(device, command_pool, queue,
					   [&] (vk::CommandBuffer& commandbuffer)
					   {
						   texture.layout =
							   transition_image_color_override(get_image(texture),
															   commandbuffer);

						   copy_buffer_to_image(staging.buffer.get(),
												get_image(texture),
												texture.extent.width,
												texture.extent.height,
												commandbuffer);
					   });
	return texture;
}

vk::UniqueImageView
create_texture_view(vk::Device& device,
					Texture2D& texture,
					const vk::ImageAspectFlags aspect)
{
	const auto subresourceRange = vk::ImageSubresourceRange{}
		.setAspectMask(aspect)
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(1);

	const auto componentMapping = vk::ComponentMapping{}
		.setR(vk::ComponentSwizzle::eIdentity)		
		.setG(vk::ComponentSwizzle::eIdentity)
		.setB(vk::ComponentSwizzle::eIdentity)
		.setA(vk::ComponentSwizzle::eIdentity);

	const auto imageViewCreateInfo = vk::ImageViewCreateInfo{}
		.setImage(get_image(texture))
		.setFormat(texture.format)
		.setSubresourceRange(subresourceRange)
		.setViewType(vk::ImageViewType::e2D)
		.setComponents(componentMapping);

	return device.createImageViewUnique(imageViewCreateInfo);
}
