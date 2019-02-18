#pragma once
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <vulkan\vulkan.hpp>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static vk::VertexInputBindingDescription getBindingDescription();
	static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions();
};

struct UniformBufferObject {
	glm::mat4 invert;
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};