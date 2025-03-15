#include "mesh.h"
#include "sampling.h"
#include "defs.h"
#include <string>

void printHelp()
{
	printf("\nMeshSampler [OPTIONS] filename\n\n");
	printf("Description: Samples an input mesh and generates and stores surface samples\n");
	printf("             in an out-of-core manner, to support very large numbers of\n");
	printf("             samples. Results are written to a PLY file with an extension\n");
	printf("             \".sampled.ply\".\n");
	printf("\n");
	printf("filename: an OBJ file to load and sample.\n");
	printf("Options:\n");
	printf("  -s NUMBER: number of samples to draw. Default is 1000000.\n");
	printf("  -m MEMORY: maximum memory to use for the sample storage in Mbytes.\n");
	printf("             Default is 64 (Mbytes). More memory -> fewer disk accesses\n");
	printf("             to append chunks of samples to file.\n");
	printf("  -c:        Additionally, sample colors. Textures are properly sampled,\n");
	printf("             if present.\n");
	printf("  -n:        Additionally, sample normals.\n");
	printf("  -f FILTER: Texture sampling magnification filter. FILTER:\n");
	printf("             \"nearest\": samples the closest texel.\n");
	printf("             \"linear\": linearly blend the 4 closest texels. Default filter.\n");
	printf("             \"sharp\": blend the 4 closest texels with cosine interpolation.\n");
	printf("             \"smooth\": 16-tap random texel selection with cosine distance weighting.\n");
	printf("\n");
	printf("Example:\n");
	printf("MeshSampler -s 20000000 -m 100 -c -n -f sharp data\\cloister.obj\n");
}

struct Params
{
	int attribs = MASK_VERTICES;
	int texfilter = TEXSAMPLING_LINEAR;
	size_t numsamples = 1000000;
	int mem = 64;
	std::string filename;
};

void parseArgs(int argc, char** argv, Params& params)
{
	for (int a = 1; a < argc; a++)
	{
		if (strcmp("-s", argv[a]) == 0)
			params.numsamples = std::stoll(argv[++a]);
		else if (strcmp("-m", argv[a]) == 0)
			params.mem = std::stoi(argv[++a]);
		else if (strcmp("-c", argv[a]) == 0)
			params.attribs |= MASK_COLORS;
		else if (strcmp("-n", argv[a]) == 0)
			params.attribs |= MASK_NORMALS;
		else if (strcmp("-f", argv[a]) == 0)
		{
			if (strcmp("nearest", argv[++a]) == 0)
				params.texfilter = TEXSAMPLING_NEAREST;
			else if (strcmp("sharp", argv[a]) == 0)
				params.texfilter = TEXSAMPLING_SHARP;
			else if (strcmp("smooth", argv[a]) == 0)
				params.texfilter = TEXSAMPLING_SMOOTH;
		}
		else
			params.filename = argv[a];
	}
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Needs at least one argument. Syntax:\n");
		printHelp();
	}
	Params params;
	parseArgs(argc, argv, params);

	Mesh mesh;
	mesh.readobj(params.filename);

	printf("Read OBJ model %s with %u faces\n", mesh.m_filename.c_str(), mesh.m_triangles.size());

	MeshSampler sampler(&mesh);

	sampler.setNumSamples(params.numsamples);
	sampler.setMode(SAMPLER_MODE_UNIFORM);
	sampler.setOutputFilename(mesh.m_filename + ".sampled.ply");
	sampler.setMemoryLimit(params.mem); // in mb.
	sampler.setSamplingAttributeMask(params.attribs);
	

	if (!sampler.sample())
		return -1;

	return 0;
}