#pragma once

#include <vulkan/vulkan.hpp>
#include "glm.hpp"

#include <vector>
#include <array>

struct VertexPosColor {
	explicit VertexPosColor() {};
	explicit VertexPosColor(const glm::vec3 pos, const glm::vec3 color)
		: pos(pos), color(color)
	{}

    glm::vec3 pos;
	float _padding0{0};
    glm::vec3 color;
	float _padding1{0};
};

const std::vector<VertexPosColor> triangle_pos_color = {
	VertexPosColor({0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}),
	VertexPosColor({0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}),
	VertexPosColor({-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}),
};

std::array<vk::VertexInputBindingDescription, 1>
get_binding_descriptions(const VertexPosColor&)
{
	return std::array<vk::VertexInputBindingDescription, 1>{
		vk::VertexInputBindingDescription{}
		.setBinding(0)
		.setStride(sizeof(VertexPosColor))
		.setInputRate(vk::VertexInputRate::eVertex),
	};
}

	
std::array<vk::VertexInputAttributeDescription, 2>
get_attribute_descriptions(const VertexPosColor&)
{
	return std::array<vk::VertexInputAttributeDescription, 2>{
		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(0)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(VertexPosColor, pos)),
		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(0)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(VertexPosColor, color)),
	};
}


