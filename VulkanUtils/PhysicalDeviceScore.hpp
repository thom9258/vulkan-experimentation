#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace PhysicalDeviceScore {

struct NotSuitable {};

using Score = std::variant<uint32_t, NotSuitable>;
	
Score operator+(Score lhs, Score rhs)
{
	if (std::holds_alternative<NotSuitable>(lhs) 
	 || std::holds_alternative<NotSuitable>(rhs))
		return NotSuitable{};

	return std::get<uint32_t>(lhs) + std::get<uint32_t>(rhs);
}

[[nodiscard]]
Score
IsDiscrete(const Score good,
		   const Score bad, 
		   const vk::raii::PhysicalDevice& device)
{
	if (device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		return good;
	return bad;
}
	
[[nodiscard]]
Score
IsIntegrated(const Score good,
			 const Score bad,
			 const vk::raii::PhysicalDevice& device)
{
	if (device.getProperties().deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
		return good;
	return bad;
}
	
[[nodiscard]]
Score
IsCpu(const Score good,
	  const Score bad,
	  const vk::raii::PhysicalDevice& device)
{
	if (device.getProperties().deviceType == vk::PhysicalDeviceType::eCpu)
		return good;
	return bad;
}
	
[[nodiscard]]
Score
RequireNotCpu(const vk::raii::PhysicalDevice& device)
{
	return IsCpu(NotSuitable{}, uint32_t{0}, device);
}

	
[[nodiscard]]
Score
HasMemoryProperties(const Score good,
					const Score bad,
					const vk::MemoryPropertyFlags required,
					const vk::raii::PhysicalDevice& device) 
{
	auto memoryProperties = device.getMemoryProperties();
	for (size_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		const auto type = memoryProperties.memoryTypes[i];
		if (type.propertyFlags | required)
			return good;
	}
	return bad;
}
	
[[nodiscard]]
Score
RequireMemoryProperties(const vk::MemoryPropertyFlags required,
						const vk::raii::PhysicalDevice& device) 
{
	return HasMemoryProperties(uint32_t{0}, NotSuitable{}, required, device);
}
	
[[nodiscard]]
Score
HasGraphicsQueue(const Score good,
				 const Score bad,
				 const vk::raii::PhysicalDevice& device)
{
	const auto has_graphics_index = [] (const vk::QueueFamilyProperties& qfp) 
	{ 
		return static_cast<bool>(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
	};

	const auto properties = device.getQueueFamilyProperties();
	const auto property =
		std::ranges::find_if(properties, has_graphics_index);
	if (property == properties.end())
		return bad;
	return good;
}

[[nodiscard]]
Score
RequireGraphicsQueue(const vk::raii::PhysicalDevice& device)
{
	return HasGraphicsQueue(uint32_t{0}, NotSuitable{}, device);
}
	
[[nodiscard]]
Score
HasPresentQueue(const Score good,
				const Score bad,
				const vk::raii::SurfaceKHR& surface,
				const vk::raii::PhysicalDevice& device)
{
	const auto properties = device.getQueueFamilyProperties();
	for (uint32_t i = 0; i < properties.size(); i++)
		if (device.getSurfaceSupportKHR(i, surface))
			return good;
	return bad;
}

[[nodiscard]]
Score
RequirePresentQueue(const vk::raii::SurfaceKHR& surface,
					const vk::raii::PhysicalDevice& device)
{
	return HasPresentQueue(uint32_t{0}, NotSuitable{}, surface, device);
}
	
[[nodiscard]]
Score
HasMatchingGraphicsPresentQueues(const Score good,
								 const Score bad,
								 const vk::raii::SurfaceKHR& surface,
								 const vk::raii::PhysicalDevice& device)
{
	const auto has_graphics_index = [] (const vk::QueueFamilyProperties& qfp) 
	{ 
		return static_cast<bool>(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
	};

	const auto properties = device.getQueueFamilyProperties();
	for (uint32_t i = 0; i < properties.size(); i++)
		if (has_graphics_index(properties[i]) && device.getSurfaceSupportKHR(i, surface))
			return good;
	return bad;
}
	
[[nodiscard]]
Score
RequireMatchingGraphicsPresentQueues(const vk::raii::SurfaceKHR& surface,
									 const vk::raii::PhysicalDevice& device)
{
	return HasMatchingGraphicsPresentQueues(uint32_t{0}, NotSuitable{}, surface, device);
}
	
[[nodiscard]]
Score
HasExtensions(const Score good,
			  const Score bad,
			  const std::vector<const char*>& requireds,
			  const vk::raii::PhysicalDevice& device)
{
	//https://github.com/KhronosGroup/Vulkan-Hpp/blob/main/samples/PhysicalDeviceExtensions/PhysicalDeviceExtensions.cpp
	//const auto available = device.enumerateDeviceExtensionProperties();
	//for (const auto required: requireds) {
	//std::find_if(available.begin(), available.end(), required);
//}

	static_cast<void>(bad);
	static_cast<void>(requireds);
	static_cast<void>(device);
	return good;
}
	
[[nodiscard]]
Score
RequireExtensions(const std::vector<const char*> requireds,
				  const vk::raii::PhysicalDevice& device)
{
	return HasExtensions(uint32_t{0}, NotSuitable{}, requireds, device);
}
	
}
