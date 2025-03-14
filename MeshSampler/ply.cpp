#include "ply.h"

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
	fprintf(fp, "element vertex %u\n", count);
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





bool savePointCloud(std::string filename, const std::vector<glm::vec3>& vertices, const std::vector<float> & quality, bool text)
{
	FILE* fp = nullptr;
	if (!text)
	{
		fopen_s(&fp, filename.c_str(), "wb");
		if (!fp)
		{
			printf("Error writing file %s\n", filename.c_str());
			return false;
		}
		fprintf(fp, "ply\n");
		fprintf(fp, "format binary_little_endian 1.0\n");
		fprintf(fp, "element vertex %ld\n", (long)vertices.size());
		fprintf(fp, "property float x\n");
		fprintf(fp, "property float y\n");
		fprintf(fp, "property float z\n");
		fprintf(fp, "property float quality\n");
		fprintf(fp, "end_header\n");

		for (size_t i = 0; i < vertices.size(); i++)
		{
			fwrite(&(vertices[i]), 3 * sizeof(float), 1, fp);
			fwrite(&(quality[i]), sizeof(float), 1, fp);
		}
	}
	else 
	{
		fopen_s(&fp, filename.c_str(), "wt");
		if (!fp)
		{
			printf("Error writing file %s\n", filename.c_str());
			return false;
		}
		
		for (size_t i = 0; i < vertices.size(); i++)
		{
			fprintf(fp, " %f %f %f 0 0 0\n", vertices[i].x, vertices[i].y, vertices[i].z);
			
		}

	}

	fclose(fp);
	return true;
}

bool savePointCloudWithNormals(std::string filename, const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& normals, const std::vector<float>& quality, bool text)
{
	FILE* fp = nullptr;
	if (!text)
	{
		fopen_s(&fp, filename.c_str(), "wb");
		if (!fp)
		{
			printf("Error writing file %s\n", filename.c_str());
			return false;
		}
		fprintf(fp, "ply\n");
		fprintf(fp, "format binary_little_endian 1.0\n");
		fprintf(fp, "element vertex %ld\n", (long)vertices.size());
		fprintf(fp, "property float x\n");
		fprintf(fp, "property float y\n");
		fprintf(fp, "property float z\n");
		fprintf(fp, "property float nx\n");
		fprintf(fp, "property float ny\n");
		fprintf(fp, "property float nz\n");
		fprintf(fp, "property float quality\n");
		fprintf(fp, "end_header\n");

		for (size_t i = 0; i < vertices.size(); i++)
		{
			fwrite(&(vertices[i]), 3 * sizeof(float), 1, fp);
			fwrite(&(normals[i]), 3 * sizeof(float), 1, fp);
			fwrite(&(quality[i]), sizeof(float), 1, fp);
		}
	}
	else
	{
		fopen_s(&fp, filename.c_str(), "wt");
		if (!fp)
		{
			printf("Error writing file %s\n", filename.c_str());
			return false;
		}

		for (size_t i = 0; i < vertices.size(); i++)
		{
			fprintf(fp, " %f %f %f %f %f %f\n", vertices[i].x, vertices[i].y, vertices[i].z,
				normals[i].x, normals[i].y, normals[i].z);

		}

	}

	fclose(fp);
	return true;
}



bool saveGradients(std::string filename, const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& dirs, float size)
{
	FILE* fp = nullptr;
	fopen_s(&fp, filename.c_str(), "wb");
	if (!fp)
	{
		printf("Error writing file %s\n", filename.c_str());
		return false;
	}
	fprintf(fp, "ply\n");
	fprintf(fp, "format binary_little_endian 1.0\n");
	fprintf(fp, "element vertex %ld\n", (long)vertices.size()*3);
	fprintf(fp, "property float x\n");
	fprintf(fp, "property float y\n");
	fprintf(fp, "property float z\n");
	fprintf(fp, "element face %ld\n", (long)vertices.size());
	fprintf(fp, "property list uchar int vertex_index\n");
	//fprintf(fp, "property float quality\n");
	fprintf(fp, "end_header\n");

	for (size_t i = 0; i < vertices.size(); i++)
	{
		fwrite(&(vertices[i]), 3 * sizeof(float), 1, fp);
		glm::vec3 endpoint = vertices[i] + dirs[i] * size;
		fwrite(&endpoint, 3 * sizeof(float), 1, fp);
		glm::vec3 up = fabs(dirs[i].y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 1);
		glm::vec3 right = glm::normalize(glm::cross(dirs[i], up));
		endpoint = vertices[i] + right * size * 0.1f;
		fwrite(&endpoint, 3 * sizeof(float), 1, fp);
	}

	for (size_t i = 0; i < vertices.size(); i++)
	{
		unsigned char l = 3;
		fwrite(&l, 1, 1, fp);
		glm::ivec3 idx(3*i,3*i+1,3*i+2);
		fwrite(&idx, 3 * sizeof(int), 1, fp);
	}

	fclose(fp);
	return true;
}
