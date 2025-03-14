#pragma once

#include <string>
#include <algorithm>
#include "glm/glm.hpp"


float distanceSquare(glm::vec3 a);

char* readText(const char* filename);

std::string getFolderPath(const char* filename);

std::string tolowerCase(std::string str);

bool compareStringIgnoreCase(std::string str1, std::string str2);

inline void printMatrix4X4(const glm::mat4& mat)
{
	printf("| %7.4f %7.4f %7.4f %7.4f |\n",
		mat[0][0], mat[1][0], mat[2][0], mat[3][0]);
	printf("| %7.4f %7.4f %7.4f %7.4f |\n",
		mat[0][1], mat[1][1], mat[2][1], mat[3][1]);
	printf("| %7.4f %7.4f %7.4f %7.4f |\n",
		mat[0][2], mat[1][2], mat[2][2], mat[3][2]);
	printf("| %7.4f %7.4f %7.4f %7.4f |\n\n",
		mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
	
}