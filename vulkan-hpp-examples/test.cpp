#include <iostream>
#include <sstream>
#include <fstream>
#include <variant>
#include <algorithm>
#include <chrono>
#include <thread>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <polymorph/polymorph.hpp>

#include "../VulkanUtils/PhysicalDeviceScore.hpp"

constexpr uint64_t fenceTimeout = 100000000;

struct VertexPosColor
{
  float x, y, z, w;   // Position
  float r, g, b, a;   // Color
};


static const std::vector<VertexPosColor> coloredCubeData{
	// red face
	{ -1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
	{ -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
	{  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
	{  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
	{ -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
	{  1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
	// green face
	{ -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
	{  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
	{ -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
	{ -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
	{  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
	{  1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
	// blue face
	{ -1.0f,  1.0f,  1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
	{ -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
	{ -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
	{ -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
	{ -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
	{ -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
	// yellow face
	{  1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
	{  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
	{  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
	{  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
	{  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
	{  1.0f, -1.0f, -1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
	// magenta face
	{  1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
	{ -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
	{  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
	{  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
	{ -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
	{ -1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
	// cyan face
	{  1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
	{  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
	{ -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
	{ -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
	{  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
	{ -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
};


constexpr std::string resources_root = "../resources";

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
PhysicalDevice_string(const vk::raii::PhysicalDevice& physicaldevice)
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
std::optional<uint32_t>
get_graphics_queue_family_index(const vk::raii::PhysicalDevice& device)
{
	const auto has_graphics_index = [] (vk::QueueFamilyProperties const & qfp) 
	{ 
		return static_cast<bool>(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
	};
	auto queueFamilyProperties = device.getQueueFamilyProperties();
	auto graphicsQueueFamilyProperty =
		std::ranges::find_if(queueFamilyProperties, has_graphics_index);
	return static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(),
											   graphicsQueueFamilyProperty));
}

[[nodiscard]]
std::vector<uint32_t>
get_all_graphics_queue_family_indices2(const vk::raii::PhysicalDevice& device)
{
	const auto has_graphics_index = [] (vk::QueueFamilyProperties const & qfp) 
	{ 
		return static_cast<bool>(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
	};
	auto properties = device.getQueueFamilyProperties();

	std::vector<uint32_t> indices{};
	for (uint32_t i = 0; i < properties.size(); i++) {
		if (has_graphics_index(properties[i]))
			indices.push_back(i);
	}
	return indices;
}

[[nodiscard]]
std::vector<uint32_t>
get_all_graphics_queue_family_indices(const vk::raii::PhysicalDevice& device)
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
get_all_present_queue_family_indices(const vk::raii::SurfaceKHR& surface,
									 const vk::raii::PhysicalDevice& device)
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
std::optional<uint32_t>
find_memory_type(const vk::PhysicalDeviceMemoryProperties& memoryProperties,
				 uint32_t typeBits,
				 const vk::MemoryPropertyFlags requirementsMask)
{
	uint32_t typeIndex = uint32_t( ~0 );
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		const bool has_requirements =
			((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask);

		if ((typeBits & 1) && has_requirements) {
			typeIndex = i;
			break;
		}
		typeBits >>= 1;
	}

	if (typeIndex == uint32_t( ~0 ) )
		return {};
	return typeIndex;
}


template <typename T>
void copy_to_device(const vk::raii::DeviceMemory& deviceMemory,
					const T* pData,
					const size_t count,
					const vk::DeviceSize stride = sizeof(T))
{
	assert(sizeof(T) <= stride);
	uint8_t* deviceData = static_cast<uint8_t*>(deviceMemory.mapMemory(0, count * stride));
	if (stride == sizeof(T)) {
		memcpy(deviceData, pData, count * sizeof(T));
	}
	else {
		for (size_t i = 0; i < count; i++) {
			memcpy( deviceData, &pData[i], sizeof( T ) );
			deviceData += stride;
		}
	}
	deviceMemory.unmapMemory();
}

template <typename T>
void copy_to_device(const vk::raii::DeviceMemory& deviceMemory, const T& data)
{
  copy_to_device<T>(deviceMemory, &data, 1);
}



typedef VkBool32 (VKAPI_PTR *PFN_vkDebugUtilsMessengerCallbackEXT)(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
    void*                                            pUserData);



VKAPI_ATTR VkBool32 VKAPI_CALL
debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
							VkDebugUtilsMessageTypeFlagsEXT messageTypes,
							const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
							void* /*pUserData*/ )
{
	constexpr uint32_t validation_warning = 0x822806fa;
	constexpr uint32_t validation_performance_warning = 0xe8d1a9fe;
#if !defined( NDEBUG )
	switch (static_cast<uint32_t>(pCallbackData->messageIdNumber)) {
	case 0:
		// Validation Warning: Override layer has override paths set to C:/VulkanSDK/<version>/Bin
		return vk::False;
	case validation_warning:
		// Validation Warning: vkCreateInstance(): to enable extension VK_EXT_debug_utils,
		//but this extension is intended to support use by applications when
		// debugging and it is strongly recommended that it be otherwise avoided.
		return vk::False;
	case validation_performance_warning:
		// Validation Performance Warning: Using debug builds of the
		//validation layers *will* adversely affect performance.
		return vk::False;
	}
#endif

	std::cerr << vk::to_string(vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity)) 
			  << ": " << vk::to_string(vk::DebugUtilsMessageTypeFlagsEXT(messageTypes)) 
			  << ":\n";

	std::cerr << std::string( "\t" ) << "messageIDName   = <" 
			  << pCallbackData->pMessageIdName << ">\n";
	std::cerr << std::string( "\t" ) << "messageIdNumber = " 
			  << pCallbackData->messageIdNumber << "\n";
	std::cerr << std::string( "\t" ) << "message         = <" 
			  << pCallbackData->pMessage << ">\n";
	if ( 0 < pCallbackData->queueLabelCount ) {
		std::cerr << std::string( "\t" ) << "Queue Labels:\n";
		for ( uint32_t i = 0; i < pCallbackData->queueLabelCount; i++ ) {
			std::cerr << std::string( "\t\t" ) << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
		}
	}
	if ( 0 < pCallbackData->cmdBufLabelCount ) {
		std::cerr << std::string( "\t" ) << "CommandBuffer Labels:\n";
		for ( uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++ ) {
			std::cerr << std::string( "\t\t" ) << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
		}
	}
	if ( 0 < pCallbackData->objectCount ) {
		std::cerr << std::string( "\t" ) << "Objects:\n";
		for ( uint32_t i = 0; i < pCallbackData->objectCount; i++ ) {
			std::cerr << std::string( "\t\t" ) << "Object " << i << "\n";
			std::cerr << std::string( "\t\t\t" ) << "objectType   = " 
					  << vk::to_string( vk::ObjectType(pCallbackData->pObjects[i].objectType))
					  << "\n";
			std::cerr << std::string( "\t\t\t" ) << "objectHandle = " 
					  << pCallbackData->pObjects[i].objectHandle << "\n";
			if ( pCallbackData->pObjects[i].pObjectName ) {
				std::cerr << std::string( "\t\t\t" ) << "objectName   = <" 
						  << pCallbackData->pObjects[i].pObjectName << ">\n";
			}
		}
	}
	return vk::False;
}	

vk::DebugUtilsMessengerCreateInfoEXT makeDebugUtilsMessengerCreateInfoEXT()
{
	vk::DebugUtilsMessengerCreateInfoEXT debugMessenger{};
	debugMessenger
		.setFlags(vk::DebugUtilsMessengerCreateFlagsEXT())
		.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning 
							| vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
		.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral 
						| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance 
						| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
		.setPfnUserCallback(&debugUtilsMessengerCallback)
		;

	return debugMessenger;
// return { {},
//          vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning 
//	   | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
//          vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
//            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
//          &vk::su::debugUtilsMessengerCallback };
}


vk::SurfaceFormatKHR
pickSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& formats)
{
  assert( !formats.empty() );
  vk::SurfaceFormatKHR pickedFormat = formats[0];
  if ( formats.size() == 1 )
  {
    if ( formats[0].format == vk::Format::eUndefined )
    {
      pickedFormat.format     = vk::Format::eB8G8R8A8Unorm;
      pickedFormat.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    }
  }
  else
  {
    // request several formats, the first found will be used
    vk::Format        requestedFormats[]  = { vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8Unorm, vk::Format::eR8G8B8Unorm };
    vk::ColorSpaceKHR requestedColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    for ( size_t i = 0; i < sizeof( requestedFormats ) / sizeof( requestedFormats[0] ); i++ )
    {
      vk::Format requestedFormat = requestedFormats[i];
      auto       it              = std::find_if( formats.begin(),
                              formats.end(),
                              [requestedFormat, requestedColorSpace]( vk::SurfaceFormatKHR const & f )
                              { return ( f.format == requestedFormat ) && ( f.colorSpace == requestedColorSpace ); } );
      if ( it != formats.end() )
      {
        pickedFormat = *it;
        break;
      }
    }
  }
  assert( pickedFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear );
  return pickedFormat;
}

std::vector<uint32_t> read_binary_file(const std::string& filename) 
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
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

int main()
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Vulkan_LoadLibrary(nullptr);
	
	SDL_Window* window = SDL_CreateWindow("myapp",
										  SDL_WINDOWPOS_UNDEFINED,
										  SDL_WINDOWPOS_UNDEFINED,
										  50,
										  50,
										  0 | SDL_WINDOW_VULKAN);
	
	const auto sdl_extensions = get_sdl2_instance_extensions(window);
    vk::raii::Context context;
	vk::ApplicationInfo applicationInfo{};
	applicationInfo
		.setPApplicationName("app")
		.setPEngineName("engine")
		.setApplicationVersion(1)
		.setEngineVersion(1)
		.setApiVersion(VK_API_VERSION_1_1);

	vk::InstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.setPEnabledExtensionNames(sdl_extensions);
    vk::raii::Instance instance(context, instanceCreateInfo);

    //vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger(instance, vk::su::makeDebugUtilsMessengerCreateInfoEXT());

	VkSurfaceKHR raw_surface;
    SDL_Vulkan_CreateSurface(window, *instance, &raw_surface);
    vk::raii::SurfaceKHR surface(instance, raw_surface);
    vk::raii::PhysicalDevices physicalDevices(instance);
	
	const std::vector<const char*> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
//#if defined(__WIN32)
		//VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
//#else
		//VK_KHR_SURFACE_EXTENSION_NAME,
//#endif
	};
	
	using DeviceAndScore = std::tuple<vk::raii::PhysicalDevice, PhysicalDeviceScore::Score>;

	const auto get_device_score =
		[&surface, &device_extensions] (const vk::raii::PhysicalDevice& device) 
		-> DeviceAndScore 
	{
		const vk::MemoryPropertyFlags required_memory_properties =
			vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eDeviceLocal;

		const auto score =
			  PhysicalDeviceScore::IsDiscrete(uint32_t{5000}, uint32_t{0}, device)
			+ PhysicalDeviceScore::IsIntegrated(uint32_t{1000}, uint32_t{0}, device)
			+ PhysicalDeviceScore::IsCpu(PhysicalDeviceScore::NotSuitable{}, uint32_t{0}, device)
			+ PhysicalDeviceScore::RequireExtensions(device_extensions, device)
			+ PhysicalDeviceScore::RequireGraphicsQueue(device)
			+ PhysicalDeviceScore::RequirePresentQueue(surface, device)
			+ PhysicalDeviceScore::RequireMemoryProperties(required_memory_properties,
														   device)
			+ PhysicalDeviceScore::RequireMatchingGraphicsPresentQueues(surface,
																device);
		return {device, score};
	};	

	const auto is_score_suitable = [] (const DeviceAndScore& device_and_score) 
		-> bool
	{
		const auto& [device, score] = device_and_score;
		if (std::holds_alternative<uint32_t>(score))
			return true;
		return false;
	};
	
	const auto print_device_info_and_score = [] (const DeviceAndScore& device_and_score)
		-> DeviceAndScore
	{
		const auto& [device, score] = device_and_score;
		std::cout << PhysicalDevice_string(device)
				  << "> Device Score: " << std::get<uint32_t>(score) 
				  << std::endl;
		return device_and_score;
	};
	
#if 0
	const auto better_device_score = [] (const DeviceAndScore& lhs, const DeviceAndScore& rhs)
		-> DeviceAndScore
	{
		const auto score_lhs = std::get<1>(lhs);
		const auto score_rhs = std::get<1>(rhs);
		if (std::holds_alternative<DeviceScore::NotSuitable>(score_lhs))
			return rhs;
		if (std::holds_alternative<DeviceScore::NotSuitable>(score_rhs))
			return lhs;
		if (std::get<uint32_t>(score_lhs) < std::get<uint32_t>(score_rhs))
			return rhs;
		return lhs;
	};
#endif	
	
	const auto applicable_devices = physicalDevices 
		>>= polymorph::transform(get_device_score)
		>>= polymorph::filter(is_score_suitable)
		//>>= polymorph::transform(print_device_info_and_score)
		;
	// TODO: we need a sorting algorithm that does not require a init value...
	//>>= polymorph::fold_left(get_best_device, );
	
	const auto best_device_and_score = applicable_devices.front();
	std::cout << "===========================================================" 
			  << "\nChosen device:" << std::endl;
	print_device_info_and_score(best_device_and_score);

	const auto physical_device = std::get<0>(best_device_and_score);
	
	const auto graphics_indices = get_all_graphics_queue_family_indices(physical_device);
	std::cout << "> graphics flags: ";
	graphics_indices >>= polymorph::stream(std::cout, " ", "\n");
	const auto present_indices = get_all_present_queue_family_indices(surface, physical_device);
	std::cout << "> present flags: ";
	present_indices >>= polymorph::stream(std::cout, " ", "\n");
	
	//TODO: We demand here that graphics and present share a queue family index,
	//      But this is not strictly a requirement we can enforce,
	//      so it would be more fitting to have a variant like:
	//      std::variant<SharedQueueFamilyIndex, SplitQueueFamilyIndices>;
	//      so we can distinquish the logic below based on this..
	// NOTE: how we need to explicitly handle the split indices case in the
	//       swap chain below...
	const auto graphics_present_index = find_matching_queue_family_indices(graphics_indices,
																		   present_indices);
	if (graphics_present_index)
		std::cout << "> found matching index: " << *graphics_present_index << std::endl;
	
	std::cout << std::endl;
	
	/** ************************************************************************
	 * Create Device
	 */
	
	float queuePriority = 0.0f;
	
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo{};
	deviceQueueCreateInfo
		.setFlags({})
		.setQueueFamilyIndex(*graphics_present_index)
		.setPQueuePriorities(&queuePriority)
		.setQueueCount(1);

	vk::DeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo
		.setQueueCreateInfoCount(1)
		.setQueueCreateInfos(deviceQueueCreateInfo)
		.setPpEnabledExtensionNames(device_extensions.data())
		.setEnabledExtensionCount(device_extensions.size());
	
	vk::raii::Device device(physical_device, deviceCreateInfo);

	std::cout << "> created logical device" << std::endl;

	/** ************************************************************************
	 * Create Command Buffer
	 */
	// create a CommandPool to allocate a CommandBuffer from
    vk::CommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.setQueueFamilyIndex(*graphics_present_index);

    vk::raii::CommandPool commandPool(device, commandPoolCreateInfo);

    // allocate a CommandBuffer from the CommandPool
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo
		.setCommandPool(commandPool)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount(1);

    vk::raii::CommandBuffer commandBuffer =
		std::move(vk::raii::CommandBuffers(device, commandBufferAllocateInfo).front());

	/** ************************************************************************
	 * Re-Create Actual Render Window and its Presentation Surface
	 */
	SDL_DestroyWindow(window);
	window = SDL_CreateWindow("render window",
							  SDL_WINDOWPOS_UNDEFINED,
							  SDL_WINDOWPOS_UNDEFINED,
							  1200,
							  800,
							  0 | SDL_WINDOW_VULKAN);

    SDL_Vulkan_CreateSurface(window, *instance, &raw_surface);
    surface = vk::raii::SurfaceKHR(instance, raw_surface);

	const auto GetWindowSize = [] (SDL_Window* window) {
		int width{0};
		int height{0};
		vk::Extent2D extent{};
		SDL_GetWindowSize(window, &width, &height);
		extent.setWidth(width);
		extent.setHeight(height);
		return extent;
	};
	const auto window_extent = GetWindowSize(window);
	

	std::cout << "> re-created windot to draw to extent: "
			  << window_extent.width << " / " << window_extent.height << std::endl;

	/** ************************************************************************
	 * Determine SwapChain Surface Sizes
	 */
    const auto surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
    vk::Extent2D swapchain_extent;
	const bool surface_size_is_undefined = 
		surface_capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max();
    if (surface_size_is_undefined) {
      // If the surface size is undefined, the size is set to the size of the images requested.
      swapchain_extent.width = std::clamp(window_extent.width,
										  surface_capabilities.minImageExtent.width,
										  surface_capabilities.maxImageExtent.width);

      swapchain_extent.height = std::clamp(window_extent.height,
										   surface_capabilities.minImageExtent.height,
										   surface_capabilities.maxImageExtent.height);
    }
    else {
      // If the surface size is defined, the swap chain size must match
      swapchain_extent = surface_capabilities.currentExtent;
    }
	

	std::cout << "> SwapChain Extent width:  " << swapchain_extent.width
			  << "\n"
			  << "                   height: " << swapchain_extent.height
			  << std::endl;

	/** ************************************************************************
	 * Determine SwapChain Surface Format
	 */
    // get the supported VkFormats
    std::vector<vk::SurfaceFormatKHR> formats = physical_device.getSurfaceFormatsKHR( surface );
    assert(!formats.empty());
    vk::Format format =
		(formats[0].format == vk::Format::eUndefined) ? vk::Format::eB8G8R8A8Unorm 
		                                              : formats[0].format;

	/** ************************************************************************
	 * Create SwapChain
	 */
    // The FIFO present mode is guaranteed by the spec to be supported
    const vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;
	const auto supports_identity = 
		static_cast<bool>(surface_capabilities.supportedTransforms 
		                & vk::SurfaceTransformFlagBitsKHR::eIdentity);

    const vk::SurfaceTransformFlagBitsKHR preTransform =
		(supports_identity) ? vk::SurfaceTransformFlagBitsKHR::eIdentity
                            : surface_capabilities.currentTransform;
	

	// this spaghetti determines the best compositealpha flags....
	vk::CompositeAlphaFlagBitsKHR compositeAlpha =
      ( surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied )    ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
      : ( surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied ) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
      : ( surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit )        ? vk::CompositeAlphaFlagBitsKHR::eInherit
                                                                                                         : vk::CompositeAlphaFlagBitsKHR::eOpaque;

	const auto wanted_surface_image_count = 3u;
	uint32_t surface_image_count{0};
	if (surface_capabilities.maxImageCount == 0) {
		surface_image_count = std::min(wanted_surface_image_count,
									   surface_capabilities.minImageCount);
	
	}
	else {
		surface_image_count = std::clamp(wanted_surface_image_count,
										 surface_capabilities.minImageCount,
										 surface_capabilities.maxImageCount);
	}
	
	std::cout << "> surface capabilities:\n"
			  << "  > min image count: " << surface_capabilities.minImageCount << "\n"
			  << "  > max image count: " << surface_capabilities.maxImageCount << "\n"
			  << "  > requested image count: " << surface_image_count 
			  << std::endl;

    const std::array<uint32_t, 1> queueFamilyIndices = {*graphics_present_index};
    vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo
		.setFlags(vk::SwapchainCreateFlagsKHR())
		.setSurface(surface)
		.setMinImageCount(surface_image_count)
		.setImageFormat(format)
		.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
		.setImageExtent(swapchain_extent)
		.setQueueFamilyIndices(queueFamilyIndices)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment 
					   | vk::ImageUsageFlagBits::eTransferSrc)
		.setImageSharingMode(vk::SharingMode::eExclusive)
		.setPreTransform(preTransform)
		.setCompositeAlpha(compositeAlpha)
		.setPresentMode(swapchainPresentMode);

	//TODO: See TODO in the indices retrieval code above for improvements...
	//      and integrate it into this part so we can support different indices in the
	//      swapchain :)
#ifdef DIFFERENT_QUEUEFAMILY_INDICES_SUPPORTED
    std::array<uint32_t, 2> queueFamilyIndices = {
		graphicsQueueFamilyIndex,
		presentQueueFamilyIndex
	};
    if ( graphicsQueueFamilyIndex != presentQueueFamilyIndex )
    {
      // If the graphics and present queues are from different queue families, we either have to explicitly transfer
      // ownership of images between the queues, or we have to create the swapchain with imageSharingMode as
      // VK_SHARING_MODE_CONCURRENT
      swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
      swapChainCreateInfo.queueFamilyIndexCount =
		  static_cast<uint32_t>(queueFamilyIndices.size());
      swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    }
#endif

    vk::raii::SwapchainKHR swapChain(device, swapChainCreateInfo);

	/** ************************************************************************
	 * Get the SwapChain Images and their Views
	 */
    std::vector<vk::Image> swapChainImages = swapChain.getImages();

	std::cout << "> SwapChain image count: " << swapChainImages.size()
			  << std::endl;

	std::vector<vk::raii::ImageView> swapChainImageViews;
	swapChainImageViews.reserve( swapChainImages.size() );
	
	vk::ImageSubresourceRange subresourceRange{};
	subresourceRange
		.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(1);

	vk::ImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(format)
		.setSubresourceRange(subresourceRange);
	
	for (auto& image : swapChainImages)
	{
		imageViewCreateInfo.image = image;
		swapChainImageViews.push_back( { device, imageViewCreateInfo } );
	}
	
	/** ************************************************************************
	 * Create a Depth Image
	 */
    const vk::Format depthFormat = vk::Format::eD16Unorm;
    vk::FormatProperties formatProperties = physical_device.getFormatProperties(depthFormat);
	
    vk::ImageTiling tiling;
    if (formatProperties.linearTilingFeatures 
	    & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
    {
      tiling = vk::ImageTiling::eLinear;
    }
    else if (formatProperties.optimalTilingFeatures 
			 & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
    {
      tiling = vk::ImageTiling::eOptimal;
    }
    else
    {
      throw std::runtime_error("D16Unorm depth format DepthStencilAttachment not supported!");
    }
	

	vk::Extent3D depth_extent{};
	depth_extent
		.setWidth(swapchain_extent.width)
		.setHeight(swapchain_extent.height)
		.setDepth(1);

    vk::ImageCreateInfo imageCreateInfo{};
	imageCreateInfo
		.setFlags(vk::ImageCreateFlags())
		.setImageType(vk::ImageType::e2D)
		.setFormat(depthFormat)
		.setExtent(depth_extent)
		.setMipLevels(1)
		.setArrayLayers(1)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setTiling(tiling)
		.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
    vk::raii::Image depth_image = device.createImage(imageCreateInfo);

	/** ************************************************************************
	 * Determine the memory requirements of the depth image
	 */
	auto memoryRequirements = depth_image.getMemoryRequirements();
    auto depth_typeindex =
		find_memory_type(physical_device.getMemoryProperties(),
						 memoryRequirements.memoryTypeBits,
						 vk::MemoryPropertyFlagBits::eDeviceLocal);

	std::cout << "> Depth Buffer Type Index: " << *depth_typeindex << std::endl;

	/** ************************************************************************
	 * Setup the Image Memory and View
	 */
	std::cout << "> Depth Image Allocation Size: " << memoryRequirements.size << std::endl;

	vk::MemoryAllocateInfo depth_allocate_info_memory{};
	depth_allocate_info_memory
		.setAllocationSize(memoryRequirements.size)
		.setMemoryTypeIndex(*depth_typeindex);
    vk::raii::DeviceMemory depth_memory = device.allocateMemory(depth_allocate_info_memory);
    depth_image.bindMemory(depth_memory, 0);

	vk::ImageSubresourceRange depth_subresource_range{};
	depth_subresource_range
		.setAspectMask(vk::ImageAspectFlagBits::eDepth)
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(1);

    vk::ImageViewCreateInfo image_view_create_info{};
	image_view_create_info
		.setFlags(vk::ImageViewCreateFlags())
		.setImage(depth_image)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(depthFormat)
		.setComponents(vk::ComponentMapping())
		.setSubresourceRange(depth_subresource_range);
    vk::raii::ImageView depth_view(device, image_view_create_info);

	/** ************************************************************************
	 * Setup Camera Matrix
	 */
    glm::mat4x4 model = glm::mat4x4( 1.0f );
    glm::mat4x4 view =
		glm::lookAt(glm::vec3(-5.0f, 3.0f, -10.0f),
					glm::vec3(0.0f, 0.0f, 0.0f),
					glm::vec3(0.0f, -1.0f, 0.0f));
    glm::mat4x4 projection = glm::perspective(glm::radians( 45.0f ), 1.0f, 0.1f, 100.0f);

	//NOTE: vulkan clip space has inverted y and half z !
    glm::mat4x4 clip = glm::mat4x4(1.0f,  0.0f, 0.0f, 0.0f,
                                   0.0f, -1.0f, 0.0f, 0.0f,
                                   0.0f,  0.0f, 0.5f, 0.0f,
                                   0.0f,  0.0f, 0.5f, 1.0f);

    glm::mat4x4 mvpc = clip * projection * view * model;

	/** ************************************************************************
	 * Setup Buffer for Uniform Buffer
	 */
    vk::BufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo
		.setFlags(vk::BufferCreateFlags())
		.setSize(sizeof(mvpc))
		.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

    vk::raii::Buffer uniformDataBuffer(device, bufferCreateInfo);
    vk::MemoryRequirements uniformMemoryRequirements = uniformDataBuffer.getMemoryRequirements();
	
    auto uniform_type_index =
		find_memory_type(physical_device.getMemoryProperties(),
						 memoryRequirements.memoryTypeBits,
						 vk::MemoryPropertyFlagBits::eHostVisible 
						 | vk::MemoryPropertyFlagBits::eHostCoherent);

	vk::MemoryAllocateInfo uniform_memory_allocate_info{};
	uniform_memory_allocate_info
		.setAllocationSize(uniformMemoryRequirements.size)
		.setMemoryTypeIndex(*uniform_type_index);
    vk::raii::DeviceMemory uniformDataMemory(device, uniform_memory_allocate_info);

	std::cout << "> Uniform Buffer Allocation Size: " << uniformMemoryRequirements.size 
			  << std::endl;

	copy_to_device(uniformDataMemory, mvpc);

	/** ************************************************************************
	 * Create DescriptorSets and Pipeline
	 */
    // create a DescriptorSetLayout
    vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding{};
	descriptorSetLayoutBinding
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);


    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(descriptorSetLayoutBinding);

    vk::raii::DescriptorSetLayout descriptorSetLayout( device, descriptorSetLayoutCreateInfo );

    // create a PipelineLayout using that DescriptorSetLayout
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo
		.setFlags(vk::PipelineLayoutCreateFlags())
		.setSetLayouts(*descriptorSetLayout);
	
    vk::raii::PipelineLayout pipelineLayout(device, pipelineLayoutCreateInfo);


	/** ************************************************************************
	 * Create DescriptorPool
	 */
    vk::DescriptorPoolSize poolSize{};
	poolSize
		.setDescriptorCount(1)
		.setType(vk::DescriptorType::eUniformBuffer);

    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
		.setMaxSets(1)
		.setPoolSizes(poolSize);

    vk::raii::DescriptorPool descriptorPool(device, descriptorPoolCreateInfo);

	/** ************************************************************************
	 * Setup DescriptorSet for the Uniform Buffer
	 */
	vk::DescriptorSetAllocateInfo uniformAllocateInfo{};
	uniformAllocateInfo
		.setDescriptorSetCount(1)
		.setDescriptorPool(descriptorPool)
		.setSetLayouts(*descriptorSetLayout);

    vk::raii::DescriptorSet uniformDescriptorSet = std::move(vk::raii::DescriptorSets(device, uniformAllocateInfo).front());

	/** ************************************************************************
	 * Create Render Pass Attachments
	 */
    vk::Format colorFormat =
		pickSurfaceFormat(physical_device.getSurfaceFormatsKHR(surface)).format;

    std::array<vk::AttachmentDescription, 2> attachmentDescriptions;
    attachmentDescriptions[0] = vk::AttachmentDescription{};
	attachmentDescriptions[0]
		.setFlags(vk::AttachmentDescriptionFlags())
		.setFormat(colorFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
	attachmentDescriptions[1]
		.setFlags(vk::AttachmentDescriptionFlags())
		.setFormat(depthFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	

    vk::AttachmentReference colorReference{};
	colorReference
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	
    vk::AttachmentReference depthReference{};
	depthReference
		.setAttachment(1)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass{};
	subpass
		.setFlags(vk::SubpassDescriptionFlags())
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setInputAttachments({})
		.setColorAttachments(colorReference)
		.setResolveAttachments({})
		.setPDepthStencilAttachment(&depthReference);
	

    vk::RenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo
		.setFlags(vk::RenderPassCreateFlags())
		.setAttachments(attachmentDescriptions)
		.setSubpasses(subpass);
    vk::raii::RenderPass renderPass(device, renderPassCreateInfo);
	
	/** ************************************************************************
	 * Load Shader SPIR-V
	 */
    const auto vert = read_binary_file(resources_root + "/basic.vert.spv");
	if (vert.empty())
		throw std::runtime_error("COULD NOT LOAD VERTEX SHADER BINARY");

    vk::ShaderModuleCreateInfo vertexShaderModuleCreateInfo{};
	vertexShaderModuleCreateInfo
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(vert);
    vk::raii::ShaderModule vertexShaderModule(device, vertexShaderModuleCreateInfo);

	

    const auto frag = read_binary_file(resources_root + "/basic.frag.spv");
	if (frag.empty())
		throw std::runtime_error("COULD NOT LOAD FRAGMENT SHADER BINARY");
	
    vk::ShaderModuleCreateInfo fragmentShaderModuleCreateInfo{};
	fragmentShaderModuleCreateInfo
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(frag);
    vk::raii::ShaderModule fragmentShaderModule(device, fragmentShaderModuleCreateInfo);

	/** ************************************************************************
	 * Create Framebuffers
	 */
    std::array<vk::ImageView, 2> framebuffer_attachments;
    framebuffer_attachments[1] = depth_view;

    std::vector<vk::raii::Framebuffer> framebuffers;
    framebuffers.reserve(swapChainImageViews.size());
    for ( auto const & view : swapChainImageViews) {
      framebuffer_attachments[0] = view;
      vk::FramebufferCreateInfo framebufferCreateInfo{};
	  framebufferCreateInfo
		  .setFlags(vk::FramebufferCreateFlags())
		  .setAttachments(framebuffer_attachments)
		  .setWidth(swapchain_extent.width)
		  .setHeight(swapchain_extent.height)
		  .setLayers(1);
      framebuffers.push_back(vk::raii::Framebuffer(device, framebufferCreateInfo));
    }
	
	std::cout << "> Created Framebuffer Count: " << framebuffers.size() << std::endl;
	
	/** ************************************************************************
	 * Create Vertex Buffer
	 */
    // NOTE: in order to get a clean desctruction sequence, instantiate
	// the DeviceMemory for the vertex buffer first
    vk::raii::DeviceMemory vertexDeviceMemory(nullptr);

    // create a vertex buffer for some vertex and color data
    vk::BufferCreateInfo vertexBufferCreateInfo{};
	vertexBufferCreateInfo
		.setFlags(vk::BufferCreateFlags())
		.setSize(sizeof(coloredCubeData[0]) * coloredCubeData.size())
		.setUsage(vk::BufferUsageFlagBits::eVertexBuffer);
    vk::raii::Buffer vertexBuffer(device, vertexBufferCreateInfo);

	
    // allocate device memory for that buffer
    vk::MemoryRequirements vertexMemoryRequirements = vertexBuffer.getMemoryRequirements();

    auto vertexMemoryTypeIndex = find_memory_type(physical_device.getMemoryProperties(),
												  vertexMemoryRequirements.memoryTypeBits,
												  vk::MemoryPropertyFlagBits::eHostVisible 
												  | vk::MemoryPropertyFlagBits::eHostCoherent);

	if (!vertexMemoryTypeIndex)
		throw std::runtime_error("Could not get vertex memory type index");

    vk::MemoryAllocateInfo vertexMemoryAllocateInfo{};
	vertexMemoryAllocateInfo
		.setAllocationSize(vertexMemoryRequirements.size)
		.setMemoryTypeIndex(*vertexMemoryTypeIndex);
    vertexDeviceMemory = vk::raii::DeviceMemory(device, vertexMemoryAllocateInfo);
	
#if 0
    vk::MemoryAllocateInfo vertexMemoryAllocateInfo{};
	vertexMemoryAllocateInfo
		.setAllocationSize(vertexMemoryRequirements.size)
		.setMemoryTypeIndex(*vertexMemoryTypeIndex);
    vertexDeviceMemory = vk::raii::DeviceMemory(device, vertexMemoryAllocateInfo);
	
	copy_to_device(vertexDeviceMemory,
				   coloredCubeData.data(),
				   sizeof(coloredCubeData[0]) * coloredCubeData.size());
#endif
	
    // copy the vertex and color data into that device memory
    uint8_t* vertexDataPtr =
		static_cast<uint8_t*>(vertexDeviceMemory.mapMemory(0, vertexMemoryRequirements.size));

    std::memcpy(vertexDataPtr,
				coloredCubeData.data(),
				sizeof(coloredCubeData[0]) * coloredCubeData.size());
    vertexDeviceMemory.unmapMemory();

    // and bind the device memory to the vertex buffer
    vertexBuffer.bindMemory(vertexDeviceMemory, 0);
	
	std::cout << "> vertex buffer size: " << vertexMemoryRequirements.size << std::endl
			  << "> vertex size: " << sizeof(coloredCubeData[0]) << std::endl
			  << "> vertex count: " << coloredCubeData.size() << std::endl
			  << "> vertex memory combined needed: " 
			  << sizeof(coloredCubeData[0]) * coloredCubeData.size() << std::endl;
	
	/** ************************************************************************
	 * Create Shader Pipeline
	 */
	std::array<vk::PipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfos{};
	pipelineShaderStageCreateInfos[0]
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setFlags(vk::PipelineShaderStageCreateFlags())
		.setModule(vertexShaderModule)
		.setPName("main");
	pipelineShaderStageCreateInfos[1]
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setFlags(vk::PipelineShaderStageCreateFlags())
		.setModule(fragmentShaderModule)
		.setPName("main");

    vk::VertexInputBindingDescription vertexInputBindingDescription{};
	vertexInputBindingDescription
		.setBinding(0)
		.setStride(sizeof(coloredCubeData[0]));

    std::array<vk::VertexInputAttributeDescription, 2> vertexInputAttributeDescriptions{};
	vertexInputAttributeDescriptions[0]
		.setLocation(0)
		.setBinding(0)
		.setFormat(vk::Format::eR32G32B32A32Sfloat)
		.setOffset(0);
	vertexInputAttributeDescriptions[1]
		.setLocation(1)
		.setBinding(0)
		.setFormat(vk::Format::eR32G32B32A32Sfloat)
		.setOffset(16); // offset of color field in the vertex

	    vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
		pipelineVertexInputStateCreateInfo
			.setFlags(vk::PipelineVertexInputStateCreateFlags())
			.setVertexBindingDescriptions(vertexInputBindingDescription)
			.setVertexAttributeDescriptions(vertexInputAttributeDescriptions);

    vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
	pipelineInputAssemblyStateCreateInfo
		.setFlags(vk::PipelineInputAssemblyStateCreateFlags())
		.setPrimitiveRestartEnable(vk::False)
		.setTopology(vk::PrimitiveTopology::eTriangleList);
	
	vk::Viewport default_viewport{};
	default_viewport
		.setX(0.0f)
		.setY(0.0f)
		.setWidth(static_cast<float>(window_extent.width))
		.setHeight(static_cast<float>(window_extent.height))
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);
	
	std::cout << "> pipeline wiewport width:  " << default_viewport.width << std::endl
			  << "> pipeline wiewport height: " << default_viewport.height << std::endl;
	
	vk::Rect2D default_scissor{};
	default_scissor.offset
		.setX(0)
		.setY(0);
	
    vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
	pipelineViewportStateCreateInfo
		.setFlags(vk::PipelineViewportStateCreateFlags())
		.setViewportCount(1)
		.setViewports(default_viewport)
		.setScissorCount(1)
		.setScissors(default_scissor);
	
    vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
	pipelineRasterizationStateCreateInfo
		.setFlags(vk::PipelineRasterizationStateCreateFlags())
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eClockwise)
		.setDepthBiasEnable(false)
		.setDepthBiasConstantFactor(0.0f)
		.setDepthBiasClamp(0.0f)
		.setDepthBiasSlopeFactor(0.0f)
		.setLineWidth(1.0f);

    vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
	pipelineMultisampleStateCreateInfo
		.setFlags(vk::PipelineMultisampleStateCreateFlags())
		.setSampleShadingEnable(false)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);

    vk::StencilOpState stencilOpState{};
	stencilOpState
		.setFailOp(vk::StencilOp::eKeep)
		.setPassOp(vk::StencilOp::eKeep)
		.setDepthFailOp(vk::StencilOp::eKeep)
		.setCompareOp(vk::CompareOp::eAlways);

	vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
	pipelineDepthStencilStateCreateInfo
		.setFlags(vk::PipelineDepthStencilStateCreateFlags())
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
		.setDepthBoundsTestEnable(false)
		.setStencilTestEnable(false)
		.setFront(stencilOpState)
		.setBack(stencilOpState);
		
    vk::ColorComponentFlags colorComponentFlags(vk::ColorComponentFlagBits::eR 
												| vk::ColorComponentFlagBits::eG 
												| vk::ColorComponentFlagBits::eB 
												| vk::ColorComponentFlagBits::eA);

	vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
	pipelineColorBlendAttachmentState
		.setBlendEnable(false)
		.setSrcColorBlendFactor(vk::BlendFactor::eZero)
		.setDstColorBlendFactor(vk::BlendFactor::eZero)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eZero)
		.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
		.setAlphaBlendOp(vk::BlendOp::eAdd)
		.setColorWriteMask(colorComponentFlags);
		
	vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
	pipelineColorBlendStateCreateInfo
		.setFlags(vk::PipelineColorBlendStateCreateFlags())
		.setLogicOpEnable(false)
		.setLogicOp(vk::LogicOp::eNoOp)
		.setAttachments(pipelineColorBlendAttachmentState)
		.setBlendConstants({ 1.0f, 1.0f, 1.0f, 1.0f });
	

    std::array<vk::DynamicState, 2> dynamicStates{
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};
	
    vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
	pipelineDynamicStateCreateInfo
		.setFlags(vk::PipelineDynamicStateCreateFlags())
		.setDynamicStates(dynamicStates);
	
	vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo
		.setFlags(vk::PipelineCreateFlags())
		.setStages(pipelineShaderStageCreateInfos)
		.setPVertexInputState(&pipelineVertexInputStateCreateInfo)
		.setPInputAssemblyState(&pipelineInputAssemblyStateCreateInfo)
		.setPTessellationState(nullptr)
		.setPViewportState(&pipelineViewportStateCreateInfo)
		.setPRasterizationState(&pipelineRasterizationStateCreateInfo)
		.setPMultisampleState(&pipelineMultisampleStateCreateInfo)
		.setPDepthStencilState(&pipelineDepthStencilStateCreateInfo)
		.setPColorBlendState(&pipelineColorBlendStateCreateInfo)
		.setPDynamicState(&pipelineDynamicStateCreateInfo)
		.setLayout(pipelineLayout)
		.setRenderPass(renderPass);

	vk::raii::Pipeline graphicsPipeline(device, nullptr, graphicsPipelineCreateInfo);
    switch (graphicsPipeline.getConstructorSuccessCode()) {
	case vk::Result::eSuccess: break;
	case vk::Result::ePipelineCompileRequiredEXT:
		throw std::runtime_error("Creating a Shader pipeline returned vk::Result::ePipelineCompileRequiredEXT");
        break;
	default: assert( false );  // should never happen
    }
	
	/** ************************************************************************
	 * Create Command Queues
	 */
	vk::raii::Queue graphicsQueue(device, *graphics_present_index, 0);
	vk::raii::Queue presentQueue(device, *graphics_present_index, 0);
	vk::raii::Semaphore imageAcquiredSemaphore(device, vk::SemaphoreCreateInfo());

	/** ************************************************************************
	 * Frame Loop
	 */
	SDL_Event event{};
	bool exit = false;

	while (!exit) {
		/** ************************************************************************
		 * Handle Inputs
		 */
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				exit = true;
				break;
				
			case SDL_KEYDOWN:
				switch( event.key.keysym.sym ) {
				case SDLK_ESCAPE:
					exit = true;
					break;
				}
				
				break;
				
			case SDL_WINDOWEVENT: {
				const SDL_WindowEvent& wev = event.window;
				switch (wev.event) {
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					// DO RESIZE
					break;
				case SDL_WINDOWEVENT_CLOSE:
					exit = true;
					break;
				}
			} break;
			}
		}
		

		/** ************************************************************************
		 * Get Next Presentation Image Index
		 */
		
		auto [result, imageIndex] = swapChain.acquireNextImage(fenceTimeout,
															   imageAcquiredSemaphore);
		std::cout << "> presentation image index: "
				  << imageIndex 
				  << std::endl;
		assert(result == vk::Result::eSuccess);
		assert(imageIndex < swapChainImages.size());
		
		/** ************************************************************************
		 * Setup Drawing & Cleaing Information
		 */
		vk::ClearDepthStencilValue cleardepth_value{};
		cleardepth_value
			.setDepth(1.0f)
			.setStencil(0);

		std::array<vk::ClearValue, 2> clearValues;
		clearValues[0].color = vk::ClearColorValue(1.0f, 0.2f, 0.2f, 1.0f);
		clearValues[1].depthStencil = cleardepth_value;
		
		vk::Offset2D render_offset{};
		render_offset.setX(0).setY(0);
		vk::Rect2D render_rect{};
		render_rect
			.setOffset(render_offset)
			.setExtent(swapchain_extent);
		
		vk::RenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo
			.setRenderPass(renderPass)
			.setFramebuffer(framebuffers[imageIndex])
			.setRenderArea(render_rect)
			.setClearValues(clearValues);
		
		vk::Viewport viewport{};
		viewport
			.setX(0.0f)
			.setY(0.0f)
			.setWidth(static_cast<float>(window_extent.width))
			.setHeight(static_cast<float>(window_extent.height))
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f);
		
		vk::Rect2D scissor{};
		scissor.offset
			.setX(0)
			.setY(0);

		/** ************************************************************************
		 * Setup DescriptorSet and Vertex Buffers
		 */
		const uint32_t firstSetIndex = 0;
		const std::vector<vk::DescriptorSet> descriptor_sets = { uniformDescriptorSet };
	
		const uint32_t firstBufferIndex = 0;
		const std::vector<vk::Buffer> vertex_buffers {
			vertexBuffer
		};
		const std::vector<vk::DeviceSize> vertex_buffer_offsets {
			0
		};

		/** ************************************************************************
		 * Record Render Pass
		 */
		commandBuffer.begin({});
		commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
										 pipelineLayout,
										 firstSetIndex,
										 descriptor_sets,
										 nullptr);
		
		commandBuffer.bindVertexBuffers(firstBufferIndex,
										vertex_buffers,
										vertex_buffer_offsets);

		commandBuffer.setViewport(0, viewport);
		scissor.setExtent(window_extent);
		commandBuffer.setScissor(0,  scissor);
		commandBuffer.draw( 12 * 3, 1, 0, 0 );
		commandBuffer.endRenderPass();
		commandBuffer.end();
		
		/** ************************************************************************
		 * Submit Recorded Render Pass to Graphics Pipeline
		 */
		vk::raii::Fence drawFence(device, vk::FenceCreateInfo());
		
		vk::PipelineStageFlags waitDestinationStageMask =
			vk::PipelineStageFlagBits::eColorAttachmentOutput;
		
		vk::SubmitInfo submit_info{};
		submit_info
			.setCommandBuffers(*commandBuffer)
			.setWaitSemaphores(*imageAcquiredSemaphore)
			.setWaitDstStageMask(waitDestinationStageMask)
			.setSignalSemaphores(nullptr);
	
		graphicsQueue.submit(submit_info, drawFence);
		// Wait for rendering to complete 
		while (vk::Result::eTimeout == device.waitForFences({drawFence}, VK_TRUE, fenceTimeout))
			;
		
		/** ************************************************************************
		 * Present the Rendered Image to the Window
		 */
		
		vk::PresentInfoKHR presentInfoKHR{};
		presentInfoKHR
			.setWaitSemaphores(nullptr)
			.setSwapchains(*swapChain)
			.setImageIndices(imageIndex);
		
		result = presentQueue.presentKHR(presentInfoKHR);
		switch (result) {
		case vk::Result::eSuccess: break;
		case vk::Result::eSuboptimalKHR: 
			std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR!\n"
					  << "This means we should resize the swapchain..";
			break;
		default:
			assert( false );  // an unexpected result is returned !
		}

		// wait for each frame
	    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	

	/** ************************************************************************
	 * Wait Until Device is Not in Use Anymore Before Cleanup
	 */
    device.waitIdle();

	/** ************************************************************************
	 * Manual Internal Vulkan Data Cleanup
	 */
    // while all vk::raii objects are automatically destroyed on scope leave, the Image should to be destroyed before the bound DeviceMemory
    // but the standard destruction order would destroy the DeviceMemory before the Image, so destroy the Image here
    depth_image.clear();

    // while all vk::raii objects are automatically destroyed on scope leave, the Buffer should to be destroyed before the bound DeviceMemory
    // but the standard destruction order would destroy the DeviceMemory before the Buffer, so destroy the Buffer here
    uniformDataBuffer.clear();

	/** ************************************************************************
	 * Manual SDL Cleanup
	 */

	SDL_DestroyWindow(window);
	SDL_Vulkan_UnloadLibrary();
	SDL_Quit();
	return 0;
}
