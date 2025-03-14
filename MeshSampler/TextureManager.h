#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include "SDL2/SDL.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "defs.h"

struct Texture
{
public:
	std::vector<float> m_data;
	int m_width;
	int m_height;
	int m_channels;
	int m_format;
	std::string m_name;

	glm::vec4 sample(int mode, float u, float v);
	glm::vec4 getTexel(int x, int y);
};



// Singleton Class of Texture Manager
class TextureManager
{
protected:
	std::vector<Texture*> m_textures;
	std::unordered_map<std::string, int> m_texture_index;
	int m_sampling = TEXSAMPLING_LINEAR;

	Texture* loadTexture(std::string name);

public:
	Texture* getTexture(int id);
	Texture* getTexture(std::string name);
	int getTextureID(std::string name);
	glm::vec4 sampleTexture(int id, float u, float v);
	void setSamplingMethod(int method) { m_sampling = method; }

	// get the static instance of Texture Manager
	static TextureManager& getInstance()
	{
		static TextureManager manager;
		return manager;
	}
	~TextureManager();

	// delete all textures
	void clear();

	
protected:
	TextureManager();	
	void operator=(TextureManager const&);
};

#endif
