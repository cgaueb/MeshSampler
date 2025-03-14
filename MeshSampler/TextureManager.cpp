#include "TextureManager.h"
#include <algorithm>
#include <SDL2/SDL_image.h>
#include <iostream>
#include "sampling.h"

// Texture
TextureManager::TextureManager()
{

}

Texture* TextureManager::loadTexture(std::string name)
{
	SDL_Surface* surf = IMG_Load(name.c_str());
	if (surf == 0)
	{
		printf("Could not Load texture %s\n", name.c_str());
		printf("SDL load Error %s\n", SDL_GetError());
		return 0; // error
	}

	Texture * tex = new Texture();
	tex->m_name = name;
	
	switch (surf->format->BytesPerPixel)
	{
	case 3: // no alpha channel
		if (surf->format->Rmask == 0x000000ff) tex->m_format = TEXFORMAT_RGB;
		else tex->m_format = TEXFORMAT_BGR;
		break;
	case 4: // contains alpha channel
		if (surf->format->Rmask == 0x000000ff)	 tex->m_format = TEXFORMAT_RGBA;
		else tex->m_format = TEXFORMAT_BGRA;
		break;

	default:
		printf("Error in number of colors in %s\n", name.c_str());
	}

	unsigned char* data = new unsigned char[surf->w * surf->h * surf->format->BytesPerPixel];

	tex->m_width = surf->w;
	tex->m_height = surf->h;
	
	// flip image
	for (int y = 0; y < surf->h; y++)
	{
		memcpy(
			&data[(surf->h - y - 1) * surf->w * surf->format->BytesPerPixel],
			&static_cast<unsigned char*>(surf->pixels)[y * surf->w * surf->format->BytesPerPixel],
			surf->w * surf->format->BytesPerPixel * sizeof(unsigned char));
	}

	int num_channels = surf->format->BytesPerPixel;
	tex->m_channels = num_channels;

	SDL_LockSurface(surf);
	tex->m_data.resize(surf->w * surf->h * num_channels);
	for (size_t k = 0; k < surf->w * surf->h; k++)
	{
		switch (tex->m_format)
		{
		case TEXFORMAT_RGB:
			tex->m_data[k * num_channels + 0] = data[k * num_channels + 0] / 255.0f;
			tex->m_data[k * num_channels + 1] = data[k * num_channels + 1] / 255.0f;
			tex->m_data[k * num_channels + 2] = data[k * num_channels + 2] / 255.0f;
			break;
		case TEXFORMAT_BGR:
			tex->m_data[k * num_channels + 0] = data[k * num_channels + 2] / 255.0f;
			tex->m_data[k * num_channels + 1] = data[k * num_channels + 1] / 255.0f;
			tex->m_data[k * num_channels + 2] = data[k * num_channels + 0] / 255.0f;
			break;
		case TEXFORMAT_RGBA:
			tex->m_data[k * num_channels + 0] = data[k * num_channels + 0] / 255.0f;
			tex->m_data[k * num_channels + 1] = data[k * num_channels + 1] / 255.0f;
			tex->m_data[k * num_channels + 2] = data[k * num_channels + 2] / 255.0f;
			tex->m_data[k * num_channels + 3] = data[k * num_channels + 3] / 255.0f;
		case TEXFORMAT_BGRA:
			tex->m_data[k * num_channels + 0] = data[k * num_channels + 2] / 255.0f;
			tex->m_data[k * num_channels + 1] = data[k * num_channels + 1] / 255.0f;
			tex->m_data[k * num_channels + 2] = data[k * num_channels + 0] / 255.0f;
			tex->m_data[k * num_channels + 3] = data[k * num_channels + 3] / 255.0f;
			break;
		}
	}
	SDL_UnlockSurface(surf);

	delete[] data;
	
	if (surf) SDL_FreeSurface(surf);
	return tex;
}

Texture* TextureManager::getTexture(int id)
{
	if (id < m_textures.size() && id >= 0)
		return m_textures[id];
	else
		return 0;
}

Texture* TextureManager::getTexture(std::string name)
{
	return getTexture(getTextureID(name));
}

int TextureManager::getTextureID(std::string name)
{
	auto iter = m_texture_index.find(name);
	if (iter != m_texture_index.end())
		return iter->second;
	Texture* tex = loadTexture(name);
	if (!tex)
		return -1;
	int id = (int)m_textures.size();
	m_texture_index[name] = id;
	m_textures.push_back(tex);
	return id;
}

glm::vec4 TextureManager::sampleTexture(int id, float u, float v)
{
	if (id >= 0 && id < m_textures.size())
		return m_textures[id]->sample(m_sampling, u, v);
	return glm::vec4(1.0f);
}

TextureManager::~TextureManager()
{
	clear();
}

void TextureManager::clear()
{
	for (auto t : m_textures)
		delete t;
	m_textures.clear();
	m_texture_index.clear();
}

glm::vec4 Texture::sample(int mode, float u, float v)
{
	float x = u * (m_width - 1);
	float y = v * (m_height - 1);
	glm::vec4 color;

	float low[2], high[2];
	glm::vec4 ll, lh, hl, hh, top, bottom;

	switch (mode)
	{
	case TEXSAMPLING_LINEAR:
	case TEXSAMPLING_SHARP:
		low[0] = floor(x);
		low[1] = floor(y);
		high[0] = ceil(x);
		high[1] = ceil(y);
		ll = getTexel((int)low[0], (int)low[1]);
		lh = getTexel((int)low[0], (int)high[1]);
		hl = getTexel((int)high[0], (int)low[1]);
		hh = getTexel((int)high[0], (int)high[1]);
		if (mode == TEXSAMPLING_SHARP)
		{
			top = COSERP(lh, hh, (x - low[0]));
			bottom = COSERP(ll, hl, (x - low[0]));
			return COSERP(bottom, top, (y - low[1]));
		}
		else
		{
			top = LERP(lh, hh, (x - low[0]));
			bottom = LERP(ll, hl, (x - low[0]));
			return LERP(bottom, top, (y - low[1]));
		}
		break;
	case TEXSAMPLING_SMOOTH:
		color = glm::vec4(0);
		for (int i = 0; i < 16; i++)
		{
			float r = 1.414f * sampleUniform0to1();
			float theta = 2 * PI * sampleUniform0to1();
			float sx = x + r * cos(theta);
			float sy = y + r * sin(theta);
			color += getTexel((int)floor(sx), (int)floor(sy));
		}
		return color / 16.0f;
		break;
	default:
		return getTexel((int)floor(x + 0.5), (int)floor(y + 0.5));
	}


	return color;
}

glm::vec4 Texture::getTexel(int x, int y)
{
	glm::vec4 color;
	int indx = (m_width + x % m_width) % m_width;
	int indy = (m_height + y % m_height) % m_height;
	
	color.r = m_data[(indy * m_width + indx) * m_channels + 0];
	color.g = m_data[(indy * m_width + indx) * m_channels + 1];
	color.b = m_data[(indy * m_width + indx) * m_channels + 2];
	if (m_channels == 4)
		color.a = m_data[(indy * m_width + indx) * m_channels + 4];
	else
		color.a = 1.0f;
	return color;
}
