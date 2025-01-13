//#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <polymorph/polymorph.hpp>

#include <variant>
#include <optional>

[[nodiscard]]
const std::string
MemoryType_string(const vk::MemoryType& type) {
	std::stringstream ss{};
	size_t bitcount = 0;

	const auto add_propertyflag = [&ss, &bitcount] (const char* bitstr) {
		if (bitcount > 0)
			ss << " | " << bitstr;
		else
			ss << bitstr;
		bitcount++;
	};
	
	ss << "[HeapIndex: " << type.heapIndex << "]  ";
	if (type.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
		add_propertyflag("MemoryPropertyFlagBits::eDeviceLocal");
	if (type.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
		add_propertyflag("MemoryPropertyFlagBits::eHostVisible");
	if (type.propertyFlags & vk::MemoryPropertyFlagBits::eHostCached)
		add_propertyflag("MemoryPropertyFlagBits::eHostCached");
	if (type.propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent)
		add_propertyflag("MemoryPropertyFlagBits::eHostCoherent");
	if (type.propertyFlags & vk::MemoryPropertyFlagBits::eLazilyAllocated)
		add_propertyflag("MemoryPropertyFlagBits::eLazilyAllocated");
	if (type.propertyFlags & vk::MemoryPropertyFlagBits::eProtected)
		add_propertyflag("MemoryPropertyFlagBits::eProtected");
	return ss.str();
}

[[nodiscard]]
const std::string
PhysicalDeviceType_string(const vk::PhysicalDeviceType& type) {
	if (type == vk::PhysicalDeviceType::eCpu)
		return "PhysicalDeviceType::eCpu";
	if (type == vk::PhysicalDeviceType::eDiscreteGpu)
		return "PhysicalDeviceType::eDiscreteGpu";
	if (type == vk::PhysicalDeviceType::eIntegratedGpu)
		return "PhysicalDeviceType::eIntegratedGpu";
	if (type == vk::PhysicalDeviceType::eVirtualGpu)
		return "PhysicalDeviceType::eVirtualGpu";
	return "PhysicalDeviceType::eOther";
}

[[nodiscard]]
const std::string
PhysicalDevice_string(const vk::PhysicalDevice& physicaldevice)
{
	std::stringstream ss{};
	ss << physicaldevice.getProperties().deviceName << "\n"
	   << "> Type: "
	   << PhysicalDeviceType_string(physicaldevice.getProperties().deviceType) << std::endl;
	
	auto memory_properties = physicaldevice.getMemoryProperties();
	for (size_t i = 0; i < memory_properties.memoryHeapCount; i++) {
		const auto memorytype_str = MemoryType_string(memory_properties.memoryTypes[i]);
		ss << "> Propertyflags: " << memorytype_str << std::endl;
		ss << "> Heap size: " << memory_properties.memoryHeaps[i].size << std::endl;
	}

	return ss.str();
}

[[nodiscard]]
std::vector<const char*>
get_sdl2_instance_extensions(SDL_Window* window)
{
    uint32_t length;
    SDL_Vulkan_GetInstanceExtensions(window, &length, nullptr);
	std::vector<const char*> extensions(length);
    SDL_Vulkan_GetInstanceExtensions(window, &length, extensions.data());
	return extensions;
}

[[nodiscard]]
std::vector<uint32_t>
get_all_graphics_queue_family_indices(const vk::PhysicalDevice& device)
{
	using enumerated_properties = polymorph::enumerated<vk::QueueFamilyProperties>;
	const auto has_graphics_index = [] (const enumerated_properties& eps)
	{ 
		return static_cast<bool>(eps.value.queueFlags & vk::QueueFlagBits::eGraphics);
	};
	const auto take_index = [] (const enumerated_properties& eps)
	{ 
		return static_cast<uint32_t>(eps.index);
	};

	return device.getQueueFamilyProperties() 
		>>= polymorph::enumerate()
		>>= polymorph::filter(has_graphics_index)
		>>= polymorph::transform(take_index);
}
[[nodiscard]]
std::vector<uint32_t>
get_all_present_queue_family_indices(const vk::SurfaceKHR& surface,
									 const vk::PhysicalDevice& device)
{
	using enumerated_properties = polymorph::enumerated<vk::QueueFamilyProperties>;
	const auto has_present_index = [&] (const enumerated_properties& eps)
	{ 
		return device.getSurfaceSupportKHR(eps.index, surface);
	};
	const auto take_index = [] (const enumerated_properties& eps)
	{ 
		return static_cast<uint32_t>(eps.index);
	};

	return device.getQueueFamilyProperties()
		>>= polymorph::enumerate()
		>>= polymorph::filter(has_present_index)
		>>= polymorph::transform(take_index);
}

struct SharedGraphicsPresentIndex {
	uint32_t shared;
};

struct SplitGraphicsPresentIndices {
	uint32_t graphics;
	uint32_t present;
};

using GraphicsPresentIndices = std::variant<SharedGraphicsPresentIndex,
											SplitGraphicsPresentIndices>;

[[nodiscard]]
uint32_t
present_index(GraphicsPresentIndices& graphics_present)
{
	if (std::holds_alternative<SharedGraphicsPresentIndex>(graphics_present)) {
		return std::get<SharedGraphicsPresentIndex>(graphics_present).shared;
	}
	return std::get<SplitGraphicsPresentIndices>(graphics_present).present;
}

[[nodiscard]]
uint32_t
graphics_index(GraphicsPresentIndices& graphics_present)
{
	if (std::holds_alternative<SharedGraphicsPresentIndex>(graphics_present)) {
		return std::get<SharedGraphicsPresentIndex>(graphics_present).shared;
	}

	return std::get<SplitGraphicsPresentIndices>(graphics_present).graphics;
}



[[nodiscard]]
std::optional<uint32_t>
find_matching_queue_family_indices(const std::vector<uint32_t> a,
								   const std::vector<uint32_t> b)
{
	for (const auto index: a) {
		const auto matching = std::ranges::find(b, index);
		if (matching != b.end())
			return *matching;
	}

	return {};
}

[[nodiscard]]
std::optional<GraphicsPresentIndices>
get_graphics_present_indices(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface)
{
	const auto graphics_indices = get_all_graphics_queue_family_indices(physical_device);
	std::cout << "> graphics flags: ";
	graphics_indices >>= polymorph::stream(std::cout, " ", "\n");
	const auto present_indices = get_all_present_queue_family_indices(surface, physical_device);
	std::cout << "> present flags: ";
	present_indices >>= polymorph::stream(std::cout, " ", "\n");
	
	if (graphics_indices.empty() || present_indices.empty())
		return std::nullopt;
	
	const auto matching = find_matching_queue_family_indices(graphics_indices,
															 present_indices);
	
	if (matching)
		return SharedGraphicsPresentIndex{*matching};
	return SplitGraphicsPresentIndices{graphics_indices[0], present_indices[0]};
}

struct SharedIndexQueue {
	vk::Queue shared;
};

struct SplitIndexQueues {
	vk::Queue graphics;
	vk::Queue present;
};

using IndexQueues = std::variant<SharedIndexQueue,
								 SplitIndexQueues>;

[[nodiscard]]
IndexQueues
get_index_queues(const vk::Device& device, const GraphicsPresentIndices& indices)
{
	if (std::holds_alternative<SharedGraphicsPresentIndex>(indices)) {
		const auto index = std::get<SharedGraphicsPresentIndex>(indices).shared;
		return SharedIndexQueue{device.getQueue(index, 0)};
	}

	const auto graphics = std::get<SplitGraphicsPresentIndices>(indices).graphics;
	const auto present = std::get<SplitGraphicsPresentIndices>(indices).present;
	return SplitIndexQueues{device.getQueue(graphics, 0),
		                    device.getQueue(present, 1)};
}

[[nodiscard]]
vk::Queue&
present_queue(IndexQueues& queues)
{
	if (std::holds_alternative<SharedIndexQueue>(queues)) {
		return std::get<SharedIndexQueue>(queues).shared;
	}
	return std::get<SplitIndexQueues>(queues).present;
}

[[nodiscard]]
vk::Queue&
graphics_queue(IndexQueues& queues)
{
	if (std::holds_alternative<SharedIndexQueue>(queues)) {
		return std::get<SharedIndexQueue>(queues).shared;
	}
	return std::get<SplitIndexQueues>(queues).graphics;
}

[[nodiscard]]
vk::SurfaceFormatKHR
get_swapchain_surface_format(const std::vector<vk::SurfaceFormatKHR>& availables) {
    for (const auto& available : availables) {
        if (available.format == vk::Format::eB8G8R8A8Srgb 
		 && available.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return available;
        }
    }

    return availables.front();
}

[[nodiscard]]
std::optional<std::vector<uint32_t>>
read_binary_file(const std::string& filename) 
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        return std::nullopt;

    size_t bytecount = static_cast<size_t>(file.tellg());
	constexpr size_t scaling_factor = sizeof(uint32_t) / sizeof(char);
    size_t read_times = bytecount / scaling_factor;
    std::vector<uint32_t> buffer;
	buffer.resize(read_times);
	//std::cout << "Byte Count: " << bytecount 
	//<< "\nBuffer Length: " << buffer.size() 
	//<< "\nBuffer Length * 4: " << buffer.size() * 4
	//<< "\nScaling Factor: " << scaling_factor
	//<< std::endl;
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), sizeof(buffer[0]) * buffer.size());
    file.close();
    return buffer;
}
