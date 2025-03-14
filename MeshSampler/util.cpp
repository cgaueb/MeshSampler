#include "util.h"
#include <fstream>

std::string getFolderPath(const char* filename)
{
	std::string str(filename);
	size_t found;
	found = str.find_last_of("/\\");
	return str.substr(0, found + 1);
}

std::string tolowerCase(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

bool compareStringIgnoreCase(std::string str1, std::string str2)
{
	str1 = tolowerCase(str1);
	str2 = tolowerCase(str2);
	return (str2.compare(str1) == 0);
}

float distanceSquare(glm::vec3 a)
{
	return glm::dot(a, a); 
}

char* readText(const char* filename)
{
	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
	if (!in.is_open())
		return nullptr;

	size_t length = in.tellg();
	in.seekg(0, in.beg);

	char* buffer = new char[length + 1];
	in.read(buffer, length);
	buffer[length] = '\0';

	if (!in)
	{
		// error in reading the file
		delete[] buffer;
		buffer = nullptr;
	}
	in.close();
	return buffer;
}