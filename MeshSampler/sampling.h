#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "ply.h"
#include "defs.h"

float sampleUniform0to1();

glm::vec3 sampleUnitSphere();


class MeshSampler
{
	class Mesh* m_mesh = nullptr;
	std::string m_output = "out.ply";
	size_t m_mem_limit = 1024 * 1024 * 32;
	unsigned char m_attribs = MASK_VERTICES | MASK_COLORS;
	size_t m_chunk_samples = 1;
	float m_jitter = 0.0f;
	int m_mode = SAMPLER_MODE_UNIFORM;
	size_t m_next_chunk_start = 0;
	size_t m_total_samples = 0;
	size_t m_requested_samples = 1000;

	std::vector<glm::vec3> m_vertices;
	std::vector<glm::vec3> m_colors;
	std::vector<glm::vec3> m_normals;

	void computeChunkSamples();

	bool writeChunk();

	bool sampleUniform();

public:
	MeshSampler() {}
	MeshSampler(Mesh* m) { m_mesh = m; }
	~MeshSampler() {}

	void setOutputFilename(std::string file) { m_output = file; }
	void setSamplingAttributeMask(int mask) { m_attribs = mask; computeChunkSamples(); }
	void setMemoryLimit(int mbytes) 
	{ 
		m_mem_limit = 1024 * 1024 * mbytes; 
		computeChunkSamples(); 
	}
	void setMode(int mode) { m_mode = mode; }
	void setNumSamples(size_t n) { m_requested_samples = n; }
	void setTextureFiltering(int f); 
	bool sample();

};