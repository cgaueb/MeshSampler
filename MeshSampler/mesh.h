#pragma once
#include <map>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <fstream>

struct Triangle
{
	unsigned int m_vertex[3];
	unsigned int m_normal[3];
	unsigned int m_coords[3];
	float m_area;
	int m_gid = 0;
	glm::vec3 m_face_normal;
};

struct TriangleGroup
{
	unsigned int m_start = 0;
	unsigned int m_length = 0;
	float m_classification = 0.5f;
	std::string matname;
};

struct Material
{
	std::string			m_name = "default";
	glm::vec3			m_base_color = { 0.95f, 0.93f, 0.89f };
	float				m_reflectance = 0.05f;
	float				m_metallic = 0.0f;
	float				m_roughness = 0.8f;
	std::string			m_texture_file_color = "";
	std::string			m_texture_file_normal = "";
	std::string			m_texture_file_mask = "";
	int					m_tid_color = -1;
	int					m_tid_normal = -1;
	int					m_tid_mask = -1;
};

class Mesh
{
protected:
	
public:
	enum file_type_t {OBJ};

	float				m_area;
	std::string			m_filename;

	std::vector<glm::vec3> m_vertex_buffer;
	std::vector<glm::vec3> m_normal_buffer;
	std::vector<glm::vec3> m_color_buffer;
	std::vector<glm::vec3> m_coords_buffer; // 3rd coord is the gid
	std::vector<Triangle> m_triangles;
	std::vector<TriangleGroup> m_groups;
	std::vector<float> m_area_cdf; 

	std::map<std::string, Material> m_materials;
	std::string m_mtl_filename;

	glm::vec3 m_min = {  FLT_MAX, FLT_MAX,  FLT_MAX };
	glm::vec3 m_max = { -FLT_MAX,-FLT_MAX, -FLT_MAX };
	
	virtual ~Mesh();
	bool readMTL(std::string filename);
	bool readobj(std::string filename);
	void flatten();
	void computeMetrics();

	glm::vec3 sampleTrianglePosition(uint32_t trid, glm::vec3 uvw);
	glm::vec3 sampleTriangleNormal(uint32_t trid, glm::vec3 uvw);
	glm::vec3 sampleTriangleColor(uint32_t trid, glm::vec3 uvw);


	void sampleAreaWeighted(glm::vec3 & pos, glm::vec3 & normal, uint32_t & trid, float * pdf = nullptr);
	bool closestPointToTriangle(glm::vec3 & cp, const Triangle & tr, const glm::vec3 & pos, float & max_distance, glm::vec3 & normal, bool compute_normal = false) const;

	float getPointToMeshDistance(const glm::vec3& q, glm::vec3& p_closest, glm::vec3& n_closest) const;
};