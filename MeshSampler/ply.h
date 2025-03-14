#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

bool plyInit(std::string filename, unsigned char mask, size_t count);
bool plyAppendPoints(std::string filename, unsigned char mask,
	const std::vector<glm::vec3>* vertices,
	const std::vector<glm::vec3>* colors,
	const std::vector<glm::vec3>* normals);
	
