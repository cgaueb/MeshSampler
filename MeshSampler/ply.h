#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

#define MASK_VERTICES 1
#define MASK_COLORS 2
#define MASK_NORMALS 4


bool plyInit(std::string filename, unsigned char mask, size_t count);
bool plyUpdateHeader(std::string filename, size_t n_vertices);
bool plyAppendPoints(std::string filename, unsigned char mask,
	const std::vector<glm::vec3>* vertices,
	const std::vector<glm::vec3>* colors,
	const std::vector<glm::vec3>* normals);
	
