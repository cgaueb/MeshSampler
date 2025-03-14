#include "mesh.h"
#include <stdio.h>
#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtc/random.hpp>
#include <glm/detail/_swizzle.hpp>
#include <atomic>
#include <omp.h>
#include <algorithm>
#include "sampling.h"
#include "ply.h"
#include <iostream>
#include <sstream>
#include <functional>
#include "util.h"
#include "TextureManager.h"

#define STR_EQUAL(_a,_b)       (_a && _b && !_stricmp (_a,_b))


float smoothstep(float edge0, float edge1, float x)
{
	if (x < edge0)
		return 0;

	if (x >= edge1)
		return 1;

	// Scale/bias into [0..1] range
	x = (x - edge0) / (edge1 - edge0);

	return x * x * (3 - 2 * x);
}

bool vec_equal(glm::vec3 v1, glm::vec3 v2)
{
	//return glm::all(glm::equal(v1, v2));
	return glm::all(glm::lessThan(glm::abs(v1 - v2), glm::vec3(0.00001f)));
}

class TriSorter
{
public:
	bool operator () (const Triangle * & left, const Triangle * & right) 
	{
		return left->m_area < right->m_area;
	}
};


Mesh::~Mesh()
{

}
bool Mesh::readMTL(std::string filename)
{
	std::ifstream in(filename, std::ios::in);
	if (!in)
	{
		std::cerr << "Cannot open material " << filename << std::endl;
		exit(1);
	}
	
	std::string folder(filename);
	folder = getFolderPath(folder.c_str());

	//the materials to return
	Material temp;
	temp.m_name = "default";
	m_materials["default"] = temp;
	
	float r, g, b;

	std::string line;
	std::string cur_mat = "default";
	while (getline(in, line))
	{
		if (line.size() == 0) continue;

		{
			int pos = 0;
			while ((line[pos] == ' ' || line[pos] == '\n' || line[pos] == '\r' || line[pos] == '\t') && pos < line.size())
				pos++;
			line = line.substr(pos);
		}

		

		if (line.substr(0, 7) == "newmtl ") { // read vertices x,y,z
			std::istringstream s(line.substr(7));
			std::string str;
			s >> str;
			// if it is the default mtl
			str = (str.empty()) ? "default" : str;

			// find if the material with that name exists
			auto iter = m_materials.find(str);
			if (iter==m_materials.end()) // didn't find  the material
			{
				Material mat;
				mat.m_name = str;
				m_materials[str] = mat;
				cur_mat = str;
			}
		}
		else if (line.substr(0, 3) == "Kd ") { // read diffuse color
			std::istringstream s(line.substr(3));
			s >> r; s >> g; s >> b;
			m_materials[cur_mat].m_base_color[0] = r;
			m_materials[cur_mat].m_base_color[1] = g;
			m_materials[cur_mat].m_base_color[2] = b;
		}
		else if (line.substr(0, 3) == "Ks ") { // read specular color
			std::istringstream s(line.substr(3));
			s >> r; s >> g; s >> b;
			m_materials[cur_mat].m_reflectance = r;
			//m_materials[cur_mat].m_roughness = g;
			//m_materials[cur_mat].m_metallic = b;
		}
		else if (line.substr(0, 3) == "Ns ") { // read shineness
			std::istringstream s(line.substr(3));
			s >> r;
			r = 1.0f - r / 255.0f;
			m_materials[cur_mat].m_roughness = r * r;
		}
		else if (line.substr(0, 2) == "d ") { // read alpha
			
		}
		else if (compareStringIgnoreCase(line.substr(0, 7), "map_kd ")) { // read texture
			std::istringstream s(line.substr(7));
			s >> m_materials[cur_mat].m_texture_file_color;
			m_materials[cur_mat].m_texture_file_color = folder + m_materials[cur_mat].m_texture_file_color;
			m_materials[cur_mat].m_tid_color = TextureManager::getInstance().getTextureID(m_materials[cur_mat].m_texture_file_color);
		}
	}
	in.close();
}

bool Mesh::readobj(std::string filename)
{
	FILE *file;
	char buf[256];
	char buf1[256];

	Material cur_material;
	TriangleGroup cur_group;
	fopen_s(&file, filename.c_str(), "rt");
	if (!file)
		return false;

	m_filename = filename;

	unsigned int vertices = 0;
	unsigned int normals = 0;
	unsigned int texcoords = 0;
	float x, y, z;
	unsigned int v, n, t;
	unsigned int n_drift = 0;

	// extract the directory path of the file to open
	size_t delim_pos = filename.rfind('\\');
	if (delim_pos == std::string::npos)
		delim_pos = filename.rfind('/');
	std::string path;
	if (delim_pos == std::string::npos)
		path = "";
	else
		path = filename.substr(0, delim_pos + 1);

	while (!feof(file))
	{
		fscanf_s(file, "%s", buf, 255);
		// matlib
		if (STR_EQUAL(buf, "mtllib"))
		{
			fscanf_s(file, "%s", buf1, 255);
			m_mtl_filename = std::string(buf1);
			std::string matlib_path = path + m_mtl_filename;
			// do something with the matlib file. Not needed for basic operation.
			readMTL(matlib_path);
		}
		
		if (STR_EQUAL(buf, "usemtl"))
		{
			fscanf_s(file, "%s", buf1, 255);
			std::string mat_name = std::string(buf1);
			auto itr = m_materials.find(mat_name);
			if (itr == m_materials.end())
			{
				cur_material = Material();
				cur_material.m_name = mat_name;
				m_materials[mat_name] = cur_material;
			}
			else
				cur_material = itr->second;
			cur_group.matname = mat_name;
		}
		
		glm::vec3 vec;

		switch (buf[0])
		{
		case '#':
			fgets(buf, sizeof(buf), file);
			break;
		case 'v': // v[?]
			switch (buf[1])
			{
			case '\0': // v
				fscanf_s(file, "%f %f %f", &x, &y, &z);
				vec = { x, y, z };
				m_vertex_buffer.push_back(vec);
				vertices++;
				if (x < m_min.x) m_min.x = x; if (y < m_min.y) m_min.y = y; if (z < m_min.z) m_min.z = z;
				if (x > m_max.x) m_max.x = x; if (y > m_max.y) m_max.y = y; if (z > m_max.z) m_max.z = z;
				break;
			case 'n': // vn
				fscanf_s(file, "%f %f %f", &x, &y, &z);
				m_normal_buffer.push_back(glm::vec3(x, y, z));
				normals++;
				break;
			case 't': // vt
				fscanf_s(file, "%f %f", &x, &y);
				m_coords_buffer.push_back(glm::vec3(x, y, 0.0f));
				texcoords++;
				break;
			}
			break;
		case 'g':
			fgets(buf, sizeof(buf), file);
			if (cur_group.m_length > 0)
				m_groups.push_back(cur_group);
			cur_group = TriangleGroup();
			cur_group.m_start = (unsigned int) m_triangles.size();
			break;
		case 'c':
			fgets(buf, sizeof(buf), file);
			sscanf_s(buf, "%f", &(cur_group.m_classification));
			break;
		case 'f':
			// if no tex coords are given, create a dummy pair.
			
			fscanf_s(file, "%s", buf, 255);
			if (strstr(buf, "//"))
			{
				Triangle tr;
				sscanf_s(buf, "%lu//%lu", &v, &n);
				tr.m_coords[0] = 0; tr.m_coords[1] = 0; tr.m_coords[2] = 0;
				tr.m_vertex[0] = v-1; tr.m_normal[0] = n - 1 + n_drift;
				fscanf_s(file, "%lu//%lu", &v, &n);
				tr.m_vertex[1] = v - 1; tr.m_normal[1] = n - 1 + n_drift;
				fscanf_s(file, "%lu//%lu", &v, &n);
				tr.m_vertex[2] = v - 1; tr.m_normal[2] = n - 1 + n_drift;
				tr.m_gid = m_groups.size();
				m_triangles.push_back(tr);
				cur_group.m_length++;
			}
			else if (sscanf_s(buf, "%lu/%lu/%lu", &v, &t, &n) == 3)
			{
				Triangle tr;
				tr.m_vertex[0] = v - 1; tr.m_normal[0] = n - 1 + n_drift; tr.m_coords[0] = t - 1;
				fscanf_s(file, "%lu/%lu/%lu", &v, &t, &n);
				tr.m_vertex[1] = v - 1; tr.m_normal[1] = n - 1 + n_drift; tr.m_coords[1] = t - 1;
				fscanf_s(file, "%lu/%lu/%lu", &v, &t, &n);
				tr.m_vertex[2] = v - 1; tr.m_normal[2] = n - 1 + n_drift; tr.m_coords[2] = t - 1;
				tr.m_gid = m_groups.size();
				m_triangles.push_back(tr);
				cur_group.m_length++;
			}
			else if (sscanf_s(buf, "%lu/%lu", &v, &t) == 2)
			{
				Triangle tr;
				tr.m_vertex[0] = v - 1; tr.m_coords[0] = t - 1;
				fscanf_s(file, "%lu/%lu", &v, &t);
				tr.m_vertex[1] = v - 1; tr.m_coords[1] = t - 1;
				fscanf_s(file, "%lu/%lu", &v, &t);
				tr.m_vertex[2] = v - 1; tr.m_coords[2] = t - 1;
				// per-vertex normal is missing, compute a geometric one
				glm::vec3 v0 = m_vertex_buffer[tr.m_vertex[0]];
				glm::vec3 v1 = m_vertex_buffer[tr.m_vertex[1]];
				glm::vec3 v2 = m_vertex_buffer[tr.m_vertex[2]];
				glm::vec3 n = glm::normalize( glm::cross(v1 - v0, v2 - v0));
				tr.m_normal[0] = tr.m_normal[1] = tr.m_normal[2] = (unsigned int) m_normal_buffer.size();
				m_normal_buffer.push_back(n);
				n_drift++;
				tr.m_gid = m_groups.size();
				m_triangles.push_back(tr);
				cur_group.m_length++;
			}
			else
			{
				Triangle tr;
				sscanf_s(buf, "%lu", &v);
				tr.m_vertex[0] = v - 1;
				sscanf_s(buf, "%lu", &v);
				tr.m_vertex[1] = v - 1;
				sscanf_s(buf, "%lu", &v);
				tr.m_vertex[2] = v - 1;
				// per-vertex normal is missing, compute a geometric one
				glm::vec3 v0 = m_vertex_buffer[tr.m_vertex[0]];
				glm::vec3 v1 = m_vertex_buffer[tr.m_vertex[1]];
				glm::vec3 v2 = m_vertex_buffer[tr.m_vertex[2]];
				glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));
				tr.m_normal[0] = tr.m_normal[1] = tr.m_normal[2] = (unsigned int) m_normal_buffer.size();
				m_normal_buffer.push_back(n);
				n_drift++;
				tr.m_coords[0] = 0; tr.m_coords[1] = 0; tr.m_coords[2] = 0;
				tr.m_gid = m_groups.size();
				m_triangles.push_back(tr);
				cur_group.m_length++;
			}
			break;
		default:
			fgets(buf, sizeof(buf), file);
		}

	}  // while read lines
	
	if (cur_group.m_length > 0)
		m_groups.push_back(cur_group);
	
	// if no tex coords are provided, create a dummy pair.
	if (m_coords_buffer.size() == 0)
	{
		m_coords_buffer.push_back(glm::vec3(0.0f,0.0f, 0.0f));
	}
	fclose(file);
	m_vertex_buffer.shrink_to_fit();
	m_coords_buffer.shrink_to_fit();
	m_normal_buffer.shrink_to_fit();
	m_triangles.shrink_to_fit();
	computeMetrics();
	return true;
}

void Mesh::flatten()
{
	// make an equally-sized buffer for all attributes.
	unsigned int total_verts = m_triangles.size() * 3;
	glm::vec3 * vertices = new glm::vec3[total_verts];
	glm::vec3 * normals = new glm::vec3[total_verts];
	glm::vec3 * texcoords = new glm::vec3[total_verts]; // third coord is the group id (as float).
	unsigned int index = 0;
	unsigned int gid = 0;
	for (auto g : m_groups)
	{
		for (int i = g.m_start; i<g.m_start + g.m_length; i++)
		{
			Triangle & tr = m_triangles[i];
			vertices[index + 0] = m_vertex_buffer[tr.m_vertex[0]];
			vertices[index + 1] = m_vertex_buffer[tr.m_vertex[1]];
			vertices[index + 2] = m_vertex_buffer[tr.m_vertex[2]];
			normals[index + 0] = m_normal_buffer[tr.m_normal[0]];
			normals[index + 1] = m_normal_buffer[tr.m_normal[1]];
			normals[index + 2] = m_normal_buffer[tr.m_normal[2]];
			texcoords[index + 0].x = m_coords_buffer[tr.m_coords[0]].x;
			texcoords[index + 0].y = m_coords_buffer[tr.m_coords[0]].y;
			texcoords[index + 0].z = gid;
			texcoords[index + 1].x = m_coords_buffer[tr.m_coords[1]].x;
			texcoords[index + 1].y = m_coords_buffer[tr.m_coords[1]].y;
			texcoords[index + 1].z = gid;
			texcoords[index + 2].x = m_coords_buffer[tr.m_coords[2]].x;
			texcoords[index + 2].y = m_coords_buffer[tr.m_coords[2]].y;
			texcoords[index + 2].z = gid;

			tr.m_vertex[0] = index + 0;
			tr.m_vertex[1] = index + 1;
			tr.m_vertex[2] = index + 2;
			tr.m_normal[0] = index + 0;
			tr.m_normal[1] = index + 1;
			tr.m_normal[2] = index + 2;
			tr.m_coords[0] = index + 0;
			tr.m_coords[1] = index + 1;
			tr.m_coords[2] = index + 2;

			tr.m_face_normal = glm::cross(vertices[index + 1] - vertices[index + 0], vertices[index + 2] - vertices[index + 0]);
			tr.m_area = glm::length(tr.m_face_normal)*0.5f;
			tr.m_face_normal = glm::normalize(tr.m_face_normal);
			tr.m_gid = gid;

			index += 3;
		}
		gid++;
	}
	m_vertex_buffer.resize(total_verts);
	memcpy(&(m_vertex_buffer[0]), vertices, total_verts * sizeof(glm::vec3));

	m_normal_buffer.resize(total_verts);
	memcpy(&(m_normal_buffer[0]), normals, total_verts * sizeof(glm::vec3));

	m_coords_buffer.resize(total_verts);
	memcpy(&(m_coords_buffer[0]), texcoords, total_verts * sizeof(glm::vec3));

	m_color_buffer.resize(total_verts, glm::vec3(1.0f, 1.0f, 1.0f));

	m_area = 0.0f;
	for (auto tr : m_triangles)
		m_area += tr.m_area;
	m_area_cdf.resize(m_triangles.size());
	m_area_cdf[0] = m_triangles[0].m_area / m_area;
	for (size_t i = 1; i < m_triangles.size(); i++)
	{
		m_area_cdf[i] = m_area_cdf[i-1] + m_triangles[i].m_area / m_area;
	}

	delete[] vertices;
	delete[] normals;
	delete[] texcoords;
}

void Mesh::computeMetrics()
{
	m_area = 0.0f;
#pragma omp parallel for
	for (long i = 0; i < m_triangles.size(); i++)
	{
		
		Triangle& tr = m_triangles[i];
		tr.m_face_normal = glm::cross(m_vertex_buffer[tr.m_vertex[1]] - m_vertex_buffer[tr.m_vertex[0]],
			                          m_vertex_buffer[tr.m_vertex[2]] - m_vertex_buffer[tr.m_vertex[0]]);
		tr.m_area = glm::length(tr.m_face_normal) * 0.5f;
		tr.m_face_normal = glm::normalize(tr.m_face_normal);
	}

	for (long i = 0; i < m_triangles.size(); i++)
	{
		m_area += m_triangles[i].m_area;
	}

}

glm::vec3 Mesh::sampleTrianglePosition(uint32_t trid, glm::vec3 uvw)
{

	glm::vec3 v0 = m_vertex_buffer[m_triangles[trid].m_vertex[0]];
	glm::vec3 v1 = m_vertex_buffer[m_triangles[trid].m_vertex[1]];
	glm::vec3 v2 = m_vertex_buffer[m_triangles[trid].m_vertex[2]];
	
	return v0 * uvw.x + uvw.y * v1 + uvw.z * v2;

}

glm::vec3 Mesh::sampleTriangleNormal(uint32_t trid, glm::vec3 uvw)
{
	glm::vec3 n0 = m_normal_buffer[m_triangles[trid].m_normal[0]];
	glm::vec3 n1 = m_normal_buffer[m_triangles[trid].m_normal[1]];
	glm::vec3 n2 = m_normal_buffer[m_triangles[trid].m_normal[2]];
	return glm::normalize(n0 * uvw.x + uvw.y * n1 + uvw.z * n2);
	 
}

glm::vec3 Mesh::sampleTriangleColor(uint32_t trid, glm::vec3 uvw)
{
	TriangleGroup& group = m_groups[m_triangles[trid].m_gid];
	Material & mat = m_materials[group.matname];
	if (mat.m_tid_color == -1)
	{
		return mat.m_base_color;
	}
	
	glm::vec3 tc0 = m_coords_buffer[m_triangles[trid].m_coords[0]];
	glm::vec3 tc1 = m_coords_buffer[m_triangles[trid].m_coords[1]];
	glm::vec3 tc2 = m_coords_buffer[m_triangles[trid].m_coords[2]];

	glm::vec3 texcoord = tc0 * uvw.x + tc1 * uvw.y + tc2 * uvw.z;
	glm::vec4 color = TextureManager::getInstance().sampleTexture(mat.m_tid_color, texcoord.x, texcoord.y);

	return glm::vec3(color);
}

bool Mesh::closestPointToTriangle(glm::vec3 & cp, const Triangle & tr, const glm::vec3 & pos, float & min_distance, glm::vec3 & normal, bool compute_normal) const
{
	glm::vec3 vertex[3];
	glm::vec3 normals[3];
	vertex[0] = m_vertex_buffer[tr.m_vertex[0]];
	vertex[1] = m_vertex_buffer[tr.m_vertex[1]];
	vertex[2] = m_vertex_buffer[tr.m_vertex[2]];
	normals[0] = m_normal_buffer[tr.m_normal[0]];
	normals[1] = m_normal_buffer[tr.m_normal[1]];
	normals[2] = m_normal_buffer[tr.m_normal[2]];


	float dist = glm::dot(pos, tr.m_face_normal) - glm::dot(vertex[0], tr.m_face_normal);

	if (fabs(dist) > fabs(min_distance)) // no distance to triangle surf, edge or vertex can be closer, so exit
		return false;

	// Project p onto the plane by stepping the distance from p to the plane
	// in the direction opposite the normal: proj = p - dist * n
	glm::vec3 proj = pos - dist * tr.m_face_normal;

	// Find out if the projected point falls within the triangle -- see:
	// http://blackpawn.com/texts/pointinpoly/default.html

	// Compute edge vectors
	glm::vec3 v02 = vertex[2] - vertex[0];
	glm::vec3 v01 = vertex[1] - vertex[0];
	glm::vec3 v2 = proj - vertex[0];

	// Compute dot products
	float dot00 = dot(v02, v02);
	float dot01 = dot(v02, v01);
	float dot02 = dot(v02, v2);
	float dot11 = dot(v01, v01);
	float dot12 = dot(v01, v2);

	// Compute barycentric coordinates (u, v) of projection point
	float denom = (dot00 * dot11 - dot01 * dot01);
	bool degenerate = (fabs(denom) < 1.0e-12);

	float invDenom = 1.0f / denom;
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	float t=min_distance;

	// Check barycentric coordinates
	if (!degenerate && (u >= 0.0f) && (v >= 0.0f) && (u + v < 1.0f))
	{
		// Nearest orthogonal projection point is in triangle
		min_distance = fabs(dist);
		glm::vec3 b = glm::vec3(1.0f - u - v, v, u);
		cp = proj;
		if (compute_normal) normal = glm::normalize(normals[0] * b[0] + normals[1] * b[1] + normals[2] * b[2]);
		return true;
	}

	// Nearest orthogonal projection point is outside triangle
	bool found = false;

	// Try the vertices.
	for (int i = 0; i < 3; i++)
	{
		dist = glm::distance(pos, vertex[i]);
		if (dist < t)
		{
			cp = vertex[i];
			if (compute_normal) normal = glm::normalize(normals[i]);
			t = dist;
			found = true;
		}
	}

	// Try the edges.
	glm::vec3 v12 = vertex[2] - vertex[1];

	float s01 = glm::dot(pos - vertex[0], v01) / glm::dot(v01,v01);
	if (s01 >= 0.0f && s01 <= 1.0f)
	{
		proj = vertex[0] + v01 * s01;
		dist = glm::distance(pos, proj);
		if (dist < t)
		{
			cp = proj;
			if (compute_normal) normal = glm::normalize(s01 * normals[1] + (1.0f - s01) * normals[0]);
			t = dist;
			found = true;
		}
	}

	float s12 = glm::dot(pos - vertex[1], v12) / glm::dot(v12, v12);
	if (s12 >= 0.0f && s12 <= 1.0f)
	{
		proj = vertex[1] + v12 * s12;
		dist = glm::distance(pos, proj);
		if (dist < t)
		{
			cp = proj;
			if (compute_normal) normal = glm::normalize(s12 * normals[2] + (1.0f - s12) * normals[1]);
			t = dist;
			found = true;
		}
	}
	
	float s20 = glm::dot(pos - vertex[2], -v02) / glm::dot(v02, v02);
	if (s20 >= 0.0f && s20 <= 1.0f)
	{
		proj = vertex[2] - v02 * s20;
		dist = glm::distance(pos, proj);
		if (dist < t)
		{
			cp = proj;
			if (compute_normal) normal = glm::normalize(s20 * normals[0] + (1.0f - s20) * normals[2]);
			t = dist;
			found = true;
		}
	}

	if (!found)
		return false;

	min_distance = t;
	return true;
}

void Mesh::sampleAreaWeighted(glm::vec3 & pos, glm::vec3 & normal, uint32_t & trid, float * pdf)
{
	float xsi = sampleUniform0to1();
	auto iter = std::upper_bound(m_area_cdf.begin(), m_area_cdf.end(), xsi);
	trid = std::min<size_t>(std::distance(m_area_cdf.begin(), iter),m_area_cdf.size()-1);
	if (pdf) *pdf = 1.0f / m_area;

	xsi = sampleUniform0to1();
	float psi = sampleUniform0to1();
	if (xsi + psi > 1.0f)
	{
		xsi = 1.0f - xsi;
		psi = 1.0f - psi;
	}
	glm::vec3 v0 = m_vertex_buffer[m_triangles[trid].m_vertex[0]];
	glm::vec3 v1 = m_vertex_buffer[m_triangles[trid].m_vertex[1]];
	glm::vec3 v2 = m_vertex_buffer[m_triangles[trid].m_vertex[2]];
	glm::vec3 n0 = m_normal_buffer[m_triangles[trid].m_normal[0]];
	glm::vec3 n1 = m_normal_buffer[m_triangles[trid].m_normal[1]];
	glm::vec3 n2 = m_normal_buffer[m_triangles[trid].m_normal[2]];


	pos = v0 * (1.0f - xsi - psi) + xsi * v1 + psi * v2;
	normal = glm::normalize(n0 * (1.0f - xsi - psi) + xsi * n1 + psi * n2);
}

