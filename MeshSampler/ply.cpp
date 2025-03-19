#include "ply.h"
#include "defs.h"
#include <fstream>
#include <iostream>

bool plyInit(std::string filename, unsigned char mask, size_t count)
{
	FILE* fp = nullptr;
	fopen_s(&fp, filename.c_str(), "wt");
	if (!fp)
	{
		printf("Error creating file %s\n", filename.c_str());
		return false;
	}
	fprintf(fp, "ply\n");
	fprintf(fp, "format binary_little_endian 1.0\n");
	fprintf(fp, "element vertex %16zu\n", count);
	if (mask & MASK_VERTICES)
	{
		fprintf(fp, "property float x\n");
		fprintf(fp, "property float y\n");
		fprintf(fp, "property float z\n");
	}
	if (mask & MASK_NORMALS)
	{
		fprintf(fp, "property float nx\n");
		fprintf(fp, "property float ny\n");
		fprintf(fp, "property float nz\n");
	}
	if (mask & MASK_COLORS)
	{
		fprintf(fp, "property uchar red\n");
		fprintf(fp, "property uchar green\n");
		fprintf(fp, "property uchar blue\n");
	}
	fprintf(fp, "end_header\n");
	fclose(fp);
}

bool plyUpdateHeader(std::string filename, size_t count)
{
	FILE* fp = nullptr;
	fopen_s(&fp, filename.c_str(), "r+");
	if (!fp)
	{
		printf("Error opening file %s for r/w.\n", filename.c_str());
		return false;
	}
	fseek(fp, 0, 0);
	char buf[256];
	while (!feof(fp))
	{
		fgets(buf, 256, fp);
		if (strcmp("format binary_little_endian 1.0\n", buf) == 0)
		{
			auto p = ftell(fp);
			fseek(fp, p, SEEK_SET);
			fprintf(fp, "element vertex %16zu\n", count);
			fclose(fp);
			return true;
		}
	}
	fclose(fp);
	return false;
}


bool plyAppendPoints(std::string filename, unsigned char mask,
	const std::vector<glm::vec3>* vertices,
	const std::vector<glm::vec3>* colors,
	const std::vector<glm::vec3>* normals)
{
	FILE* fp = nullptr;
	fopen_s(&fp, filename.c_str(), "ab");
	if (!fp)
	{
		printf("Error opening file %s\n", filename.c_str());
		return false;
	}
	
	//fseek(fp, 0, SEEK_END);

	for (size_t i = 0; i < vertices->size(); i++)
	{
		if (mask & MASK_VERTICES && vertices)
			fwrite(&((* vertices)[i]), 3 * sizeof(float), 1, fp);
		if (mask & MASK_NORMALS && normals)
			fwrite(&((*normals)[i]), 3 * sizeof(float), 1, fp);
		if (mask & MASK_COLORS && colors)
		{
			unsigned char c[3];
			c[0] = (unsigned char) ((*colors)[i].r * 255);
			c[1] = (unsigned char) ((*colors)[i].g * 255);
			c[2] = (unsigned char) ((*colors)[i].b * 255);
			fwrite(c, 3, 1, fp);
		}

	}
	fclose(fp);

	return true;
}

