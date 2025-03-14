## Usage
MeshSampler [OPTIONS] filename
Description: Samples an input mesh and generates and stores surface samples in an out-of-core manner, to support very large numbers of samples. Results are written to a PLY file with an extension ".sampled.ply". Double the disk space occupied by the final PLY file is required during operation.
	
filename: an OBJ file to load and sample.
Options:
-s NUMBER: number of samples to draw. Default is 1000000.
-m MEMORY: maximum memory to use for the sample storage in Mbytes. Default is 64 (Mbytes). More memory -> fewer disk accesses to append chunks of samples to file.\n");
-c:        Additionally, sample colors. Textures are properly sampled, if present.
-n:        Additionally, sample normals.\n");
-f FILTER: Texture sampling magnification filter. FILTER:
           "nearest": samples the closest texel.
           "linear": linearly blend the 4 closest texels. Default filter.
	         "sharp": blend the 4 closest texels with cosine interpolation. 
           "smooth": 16-tap random texel selection with cosine distance weighting.
	
 Example:
 MeshSampler -s 20000000 -m 100 -c -n -f sharp data\\cloister.obj
 
