#include <random>
#include "sampling.h"
#include "mesh.h"
#include <bitset>
#include <filesystem>
#include "TextureManager.h"

std::uniform_real_distribution<> _real_dist(0.0f,1.0f);
std::random_device _rd;
std::mt19937 _gen(_rd());

float sampleUniform0to1()
{
	return (float)_real_dist(_gen);
}

glm::vec3 sampleUnitSphere()
{
	glm::vec3 v = glm::vec3(sampleUniform0to1(), sampleUniform0to1(), sampleUniform0to1());
	return glm::normalize(v - glm::vec3(0.5f));
}

void MeshSampler::computeChunkSamples()
{
	std::bitset<8> attribs = m_attribs;
	m_chunk_samples = m_mem_limit / (sizeof(float) * attribs.count() * 3);
}

bool MeshSampler::writeChunk()
{
	bool res;
	res = plyUpdateHeader(m_output, m_total_samples);
	res *= plyAppendPoints(m_output, m_attribs, &m_vertices, &m_colors, &m_normals);
	m_vertices.clear();
	m_colors.clear();
	m_normals.clear();

	printf("\b\b\b\b\b%4.1f%%", 100.0f*std::min(1.0f,m_total_samples/(float)m_requested_samples));

	return res;
}

bool MeshSampler::sampleUniform()
{
	m_vertices.clear();
	m_colors.clear();
	m_normals.clear();
	m_total_samples = 0;

	if (m_attribs & MASK_VERTICES) m_vertices.reserve(m_chunk_samples);
	if (m_attribs & MASK_COLORS)   m_colors.reserve(m_chunk_samples);
	if (m_attribs & MASK_NORMALS)  m_normals.reserve(m_chunk_samples);

	printf("Progress: %4.1f%%", 0.0f);

	// try to sample triangles in the same order as they appear in the mesh
	// so that samples are more spatially coherent by construction
	for (size_t tr = 0; tr < m_mesh->m_triangles.size(); tr++)
	{
		double prob = m_mesh->m_triangles[tr].m_area / (double)m_mesh->m_area;
		
		// sample each triangle at least once, except for zero-area ones.
		int num_samples = (int) floor(m_requested_samples * prob);
		if (num_samples == 0)
		{
			if (sampleUniform0to1() < m_requested_samples * prob)
				num_samples = 1;
		}
		for (int i = 0; i < num_samples; i++)
		{
			float xsi = sampleUniform0to1();
			float psi = sampleUniform0to1();
			if (xsi + psi > 1.0f)
			{
				xsi = 1.0f - xsi;
				psi = 1.0f - psi;
			}
			
			glm::vec3 uvw = glm::vec3(1.0f-xsi-psi,xsi,psi);
			glm::vec3 pos, normal, color;

			if (m_attribs & MASK_VERTICES)
			{
				pos = m_mesh->sampleTrianglePosition(tr, uvw);
				m_vertices.push_back(pos);
			}
			if (m_attribs & MASK_COLORS)
			{
				color = m_mesh->sampleTriangleColor(tr, uvw);
				m_colors.push_back(color);
			}
			if (m_attribs & MASK_NORMALS)
			{
				normal = m_mesh->sampleTriangleNormal(tr, uvw);
				m_normals.push_back(normal);
			}
			m_total_samples++;

			// flush buffer to PLY file if chunk size has been reached.
			if (m_vertices.size() >= m_chunk_samples ||
				m_normals.size() >= m_chunk_samples ||
				m_colors.size() >= m_chunk_samples)
			{
				writeChunk();
			}

		}

	}

	writeChunk();

	return true;
}

void MeshSampler::setTextureFiltering(int f)
{
	TextureManager::getInstance().setSamplingMethod(f); 
}

bool MeshSampler::sample()
{
	bool ok = true;

	if (!plyInit(m_output, m_attribs, 0))
		return false;


	if (m_mode == SAMPLER_MODE_UNIFORM)
		ok = sampleUniform();
	else
		return false;

	printf("\b\b\b\b\b100.0%%...");

	if (!ok)
	{
		printf("Error while creating the output file\n");
		return false;
	}

	printf("done.\n");

}
