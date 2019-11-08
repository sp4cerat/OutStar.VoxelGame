/**
 *  Marching Cubes Demo, by glUser3f [gluser3f@gdnmail.net]
 *  Feel free to use this code in your own projects.
 *  If you do something cool with it, please email me so I can see it!
 *  
 *  Credits:
 *  Theory by Paul Bourke        [http://astronomy.swin.edu.au/~pbourke/modelling/polygonise/]
 *  OpenGL basecode by NeHe      [nehe.gamedev.net]
 *  Lookup Tables by Paul Bourke [http://astronomy.swin.edu.au/~pbourke/modelling/polygonise/]
 *  The rest is done by me, glUser3f ;)
 *
 */

struct GridPoint { float x, y, z; uint val,color; };
struct BlockVBO{  GLuint vbo_tri;uint id;};
class VoxelData 
{ 
	public: ushort intensity; uint color;
	VoxelData(){intensity=0;color=0;};
	VoxelData(ushort i,uint c){intensity=i;color=c;};
};
VoxelData voxeldata_empty( 0,0 );
std::vector<BlockVBO> blocks_vbo;

class Block;
std::vector<Block> blocks;
std::vector<Block> blocks_undo;

int EdgeTable[256] = {
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};

int TriTable[256][16] = {
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
	{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
	{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
	{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
	{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
	{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
	{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
	{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
	{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
	{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
	{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
	{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
	{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
	{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
	{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
	{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
	{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
	{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
	{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
	{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
	{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
	{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
	{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
	{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
	{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
	{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
	{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
	{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
	{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
	{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
	{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
	{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
	{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
	{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
	{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
	{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
	{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
	{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
	{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
	{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
	{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
	{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
	{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
	{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
	{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
	{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
	{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
	{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
	{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
	{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
	{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
	{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
	{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
	{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
	{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
	{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
	{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
	{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
	{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
	{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
	{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
	{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
	{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
	{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
	{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
	{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
	{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
	{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
	{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
	{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
	{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
	{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
	{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
	{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
	{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
	{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
	{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
	{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
	{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
	{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
	{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
	{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
	{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
	{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
	{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
	{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
	{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
	{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
	{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
	{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
	{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
	{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
	{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
	{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
	{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
	{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
	{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};

vec3f grid_interpolate(uint isoLevel, GridPoint& gp1, GridPoint& gp2) {
	float mu;
	vec3f p;
	mu = (float(isoLevel)-float(gp1.val)) / (float(gp2.val)-float(gp1.val));
	p.x = gp1.x + mu * (gp2.x - gp1.x);
	p.y = gp1.y + mu * (gp2.y - gp1.y);
	p.z = gp1.z + mu * (gp2.z - gp1.z);
	return p;
}

void grid_polygoniseAndInsert(
		GridPoint *point, 
		std::vector<float> &tri,
		uint color)
{
	uint isoLevel=128*256;
	int cubeIndex = 0;

	for(int i=7;i>=0;i--)
	if (point[i].val < isoLevel) 
	{
		cubeIndex |= 1<<i;
		//color=point[i].val;
	}else color=point[i].color;
	/*
	if (point[1].val < isoLevel) {cubeIndex |= 2;color=point[1].val;}
	if (point[2].val < isoLevel) {cubeIndex |= 4;color=point[2].val;}
	if (point[3].val < isoLevel) {cubeIndex |= 8;color=point[3].val;}
	if (point[4].val < isoLevel) {cubeIndex |= 16;color=point[4].val;}
	if (point[5].val < isoLevel) {cubeIndex |= 32;color=point[5].val;}
	if (point[6].val < isoLevel) {cubeIndex |= 64;color=point[6].val;}
	if (point[7].val < isoLevel) {cubeIndex |= 128;color=point[7].val;}
	*/

	if (EdgeTable[cubeIndex] == 0) return;
	if (cubeIndex == 255) return;
	if (cubeIndex == 0) return;

	//color&=255;

	vec3f vertices[12];
	if (EdgeTable[cubeIndex] & 1) {
		vertices[0] = grid_interpolate(isoLevel, point[0], point[1]);
	}
	if (EdgeTable[cubeIndex] & 2) {
		vertices[1] = grid_interpolate(isoLevel, point[1], point[2]);
	}
	if (EdgeTable[cubeIndex] & 4) {
		vertices[2] = grid_interpolate(isoLevel, point[2], point[3]);
	}
	if (EdgeTable[cubeIndex] & 8) {
		vertices[3] = grid_interpolate(isoLevel, point[3], point[0]);
	}
	if (EdgeTable[cubeIndex] & 16) {
		vertices[4] = grid_interpolate(isoLevel, point[4], point[5]);
	}
	if (EdgeTable[cubeIndex] & 32) {
		vertices[5] = grid_interpolate(isoLevel, point[5], point[6]);
	}
	if (EdgeTable[cubeIndex] & 64) {
		vertices[6] = grid_interpolate(isoLevel, point[6], point[7]);
	}
	if (EdgeTable[cubeIndex] & 128) {
		vertices[7] = grid_interpolate(isoLevel, point[7], point[4]);
	}
	if (EdgeTable[cubeIndex] & 256) {
		vertices[8] = grid_interpolate(isoLevel, point[0], point[4]);
	}
	if (EdgeTable[cubeIndex] & 512) {
		vertices[9] = grid_interpolate(isoLevel, point[1], point[5]);
	}
	if (EdgeTable[cubeIndex] & 1024) {
		vertices[10] = grid_interpolate(isoLevel, point[2], point[6]);
	}
	if (EdgeTable[cubeIndex] & 2048) {
		vertices[11] = grid_interpolate(isoLevel, point[3], point[7]);
	}

	//vec3f color3f;color3f.decode_color_uint(color);//palette_bmp.get_pixel3f(color&15,color>>4);
	
	//int triCount = 0;
	for (int i = 0; TriTable[cubeIndex][i] != -1; i+=3) 
	{
		vec3f p[3];
		loopj(0,3) p[2-j] = vertices[TriTable[cubeIndex][i+j]];

		vec3f n;
		n.cross(p[1]-p[0],p[2]-p[0]);
		n.norm();

		loopj(0,3)
		{
			tri.push_back(p[j].x);
			tri.push_back(p[j].y);
			tri.push_back(p[j].z);
			intfloat infl;
			infl.ui=color;//color3f.encode_color_uint();//colori;
			tri.push_back(infl.f);
			infl.ui=n.encode_normal_signed_uint();//colori;
			tri.push_back(infl.f);
		}
	}
}

vec3f get_normal(int x,int y,int z);

class Block
{
	public:
		
	std::vector<VoxelData>  vxl;
	std::vector<float> tri; // float xyz uint color 
	int dirty;
	GLuint vbo_tri;
	int drawcount;

	Block()	{ clear(); }
	~Block(){  }

	void clear()
	{
		std::vector<VoxelData>().swap(vxl);
		std::vector<float>().swap(tri);
		dirty=false;
		drawcount=0;
		vbo_tri=-1;
	}

	void inline add_face(int x,int y,int z,int dir,uint color,uint normal)
	{
		/*
		{
			int r=color&255;
			int g=(color>>8)&255;
			int b=(color>>16)&255;
			r=(r*brightness)>>8;
			g=(g*brightness)>>8;
			b=(b*brightness)>>8;
			color=r+g*256+b*256*256;
		}
		*/
		
		int p[4][3];
		int t[]={0,1,2,0,2,1,1,0,2,1,2,0,2,1,0,2,0,1};

		loopi(0,4)
		{
			int j=dir*3;
			p[i][t[j++]]=0;
			p[i][t[j++]]=(i&1)^(i>>1);
			p[i][t[j  ]]=i>>1;
			tri.push_back(p[i][0]+x);
			tri.push_back(p[i][1]+y);
			tri.push_back(p[i][2]+z);

			intfloat infl;
			infl.i=color;
			tri.push_back(infl.f);
			infl.i=normal;
			tri.push_back(infl.f);
		}
	}

	void triangulate(int x1,int y1,int z1)
	{
		//error_check_mem(this,sizeof(Block));
		if(vxl.size()==0)return;
		if(!dirty)return;

		//printf("block %d %d %d \n",x1,y1,z1);

		std::vector<float>().swap(tri);
		dirty=false;
		drawcount=0;
		//vbo_tri=-1;
		//dirty=drawcount=0;
		//tri.clear();

		int x0=x1<<4,y0=y1<<4,z0=z1<<4;

		loopk(0,lvl0+1){ int ofsk=k*18*18;
		loopj(0,lvl0+1){ int ofsj=ofsk+j*18;
		loopi(0,lvl0+1)
		{
			ushort v[4];
			uint ofs=i+ofsj;

			uint color=vxl[ofs].color;

			// Smooth
			if(!render_cubes)
			{
				GridPoint point[8];
				int o=0;
				float xs=x0+i-1;
				float ys=y0+j-1;
				float zs=z0+k-1;

				loop(kk,0,8)
				{
					int addx[8]={0,1,1,0,0,1,1,0};
					int addy[8]={0,0,0,0,1,1,1,1};
					int addz[8]={0,0,1,1,0,0,1,1};
					int x=addx[kk];
					int y=addy[kk];
					int z=addz[kk];
					VoxelData &vd=vxl[ofs+x+y*18+z*18*18];
					point[kk].val=vd.intensity;
					point[kk].color=vd.color;
					point[kk].x=xs+x;
					point[kk].y=ys+y;
					point[kk].z=zs+z;					
				}
				grid_polygoniseAndInsert(point,tri,color);
				continue;
			}

			// Blocky
			v[0]=vxl[ofs].intensity>>15;
			v[1]=vxl[ofs+1].intensity>>15;
			v[2]=vxl[ofs+18].intensity>>15;
			v[3]=vxl[ofs+18*18].intensity>>15;

			if( v[0]+v[1]+v[2]+v[3] ==0)continue;
			if((v[0]&v[1]&v[2]&v[3])!=0)continue;

			uint w[4]={
				vxl[ofs].color,
				vxl[ofs+1].color,
				vxl[ofs+18].color,
				vxl[ofs+18*18].color
			};
	
			int x=x0+i-1, y=y0+j-1, z=z0+k-1;

			int n=get_normal(x,y,z).encode_normal_signed_uint();//get_shade(x,y,z);

			//const int lvl=128*256;

			if(v[0])
			{
				if (!v[1])if(j>0)if(j<17)if(k>0)if(k<17) add_face(x+1,y,z,0,w[0],n);
				if (!v[2])if(i>0)if(i<17)if(k>0)if(k<17) add_face(x,y+1,z,3,w[0],n);
				if (!v[3])if(j>0)if(j<17)if(i>0)if(i<17) add_face(x,y,z+1,5,w[0],n);
			}
			else
			{
				if ( v[1])if(j>0)if(j<17)if(k>0)if(k<17) add_face(x+1,y,z,1,w[1],get_normal(x+1,y,z).encode_normal_signed_uint());
				if ( v[2])if(i>0)if(i<17)if(k>0)if(k<17) add_face(x,y+1,z,2,w[2],get_normal(x,y+1,z).encode_normal_signed_uint());
				if ( v[3])if(j>0)if(j<17)if(i>0)if(i<17) add_face(x,y,z+1,4,w[3],get_normal(x,y,z+1).encode_normal_signed_uint());
			}
		}}}
	}
	int draw(int id)
	{
		int numints=5;// per vertex

		if(tri.size()==0)return 2;

		drawcount++;

		int cache_drawcount=20;

		if(drawcount==cache_drawcount)
		{
			glGenBuffers(1,&vbo_tri);
			glBindBuffer(GL_ARRAY_BUFFER, vbo_tri);
			glBufferData(GL_ARRAY_BUFFER, tri.size()*4, &tri[0], GL_DYNAMIC_DRAW);

			BlockVBO bvbo;
			bvbo.vbo_tri=vbo_tri;
			bvbo.id=id;
			blocks_vbo.push_back(bvbo);
		}

		static Shader tri_shader("../shader/editor/voxel");
		tri_shader.begin();
		
		float *ptr=&tri[0];

		if(drawcount>=cache_drawcount)
		{
			glBindBuffer(GL_ARRAY_BUFFER, vbo_tri);
			ptr=0;
		}
		else
		{
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}			

		ogl_check_error();
		glVertexPointer(3,GL_FLOAT,4*numints,ptr);ogl_check_error();		
		glColorPointer (4,GL_UNSIGNED_BYTE,4*numints,ptr+3);ogl_check_error();
		glNormalPointer(GL_BYTE,4*numints,ptr+4);ogl_check_error();
		
		//tri_shader.setUniformMatrix4fv("projectionMatrix", 1, 0, Projection);	
		//tri_shader.setUniformMatrix4fv("modelViewMatrix", 1, 0, Modelview);		
		//tri_shader.setUniform3f("lightPos",0,-10,0);
		//tri_shader.setUniform4f("ambient",0,0,0,0);
		//tri_shader.setUniform4f("diffuse",1,0,0,0);
		tri_shader.setUniform4f("color_effect",	color_effects.x,color_effects.y,color_effects.z,color_effects.w	);
		//tri_shader.setUniform1i("texDiffuse",0);
		//tri_shader.setUniform1i("texAmbient",1);

		if(!render_cubes)
		{
			glDrawArrays(GL_TRIANGLES,0,tri.size()/numints);
			trianglecount+=tri.size()/(3*numints);
		}
		else
		{
			glDrawArrays(GL_QUADS,0,tri.size()/numints);
			trianglecount+=tri.size()/(4*numints);
		}				

		tri_shader.end();

		return (ptr==0) ? 1 : 0;
	}

};

void blocks_garbage_collect()
{
	static std::vector<BlockVBO> tmp;

	tmp.clear();

	loopi(0,blocks_vbo.size())
	{
		if(blocks_vbo[i].vbo_tri<0)continue;

		BlockVBO &b=blocks_vbo[i];

		if( blocks[b.id].vbo_tri == b.vbo_tri ||
			blocks_undo[b.id].vbo_tri == b.vbo_tri	)
		{
			tmp.push_back(b);
		}
		else
		{
			glDeleteBuffers (1,&b.vbo_tri);
		}
	}
	blocks_vbo=tmp;
}

VoxelData get_voxel(int x,int y,int z)
{
	if(x<0)return voxeldata_empty;
	if(y<0)return voxeldata_empty;
	if(z<0)return voxeldata_empty;
	int xg=x>>4;if(xg>=lvl1x)return voxeldata_empty;
	int yg=y>>4;if(yg>=lvl1y)return voxeldata_empty;
	int zg=z>>4;if(zg>=lvl1z)return voxeldata_empty;

	Block &b=blocks[xg+(yg+zg*lvl1y)*lvl1x];
	if(b.vxl.size()==0)return voxeldata_empty;

	int xb=x&15;xb++;
	int yb=y&15;yb++;
	int zb=z&15;zb++;

	return b.vxl[xb+(yb*18)+(zb*18*18)];
}


void inline set_in_block(int bo,int of,VoxelData data)
{
	Block &b=blocks[bo];

	if(b.vxl.size()==0)b.vxl.resize(18*18*18,voxeldata_empty);

	blocks[bo].vxl[of]=data;
	blocks[bo].dirty=1;
}

void set_voxel(int x,int y,int z,VoxelData data, char init=1)
{

	int sx=lvl1x*lvl0;
	int sy=lvl1y*lvl0;
	int sz=lvl1z*lvl0;

	if(brush_loop_x)x=x%(sx);
	if(brush_loop_y)y=y%(sy);
	if(brush_loop_z)z=z%(sz);

	if(init==1)
	{
		if(brush_mirror_x)	set_voxel(sx-x,y,z,data,0);
		if(brush_mirror_y)	
		{
			brush_mirror_y=0;
			set_voxel(x,sy-y,z,data,1);
			brush_mirror_y=1;
		}
		if(brush_mirror_z)	
		{
			brush_mirror_z=0;
			set_voxel(x,y,sz-z,data,1);
			brush_mirror_z=1;
		}
	}

	int xg=x>>4;if(xg>=lvl1x)return;if(x<0)return;
	int yg=y>>4;if(yg>=lvl1y)return;if(y<0)return;
	int zg=z>>4;if(zg>=lvl1z)return;if(z<0)return;

	int bo=xg+(yg+zg*lvl1y)*lvl1x;

	Block &b=blocks[bo];

	if(b.vxl.size()==0)b.vxl.resize(18*18*18,voxeldata_empty);
	b.dirty=1;

	int xb=x&15;xb++;
	int yb=y&15;yb++;
	int zb=z&15;zb++;

	int of=xb+(yb*18)+(zb*18*18);

	b.vxl[of]=data;
	
	if(xb==1||xb==16)if(yb==1||yb==16)if(zb==1||zb==16)
	{
		int addbx= xb==1 ? -1:1;
		int addby= yb==1 ? -1:1;
		int addbz= zb==1 ? -1:1;
		int addox= xb==1 ? 17 : 0;
		int addoy= yb==1 ? 17*18: 0;
		int addoz= zb==1 ? 17*18*18 : 0;
		if(xg+addbx>=0) if(xg+addbx<lvl1x) 
		if(yg+addby>=0) if(yg+addby<lvl1y) 
		if(zg+addbz>=0) if(zg+addbz<lvl1z) 
			set_in_block(bo+addbx+addby*lvl1x+addbz*lvl1x*lvl1y ,addox+addoy+addoz,data);
	}


	if(xb==1)if(xg>0) 
	{
		set_in_block(bo-1			 ,of+16,data);

		if(yb==1)if(yg>0) set_in_block(bo-lvl1x-1 ,of+16+16*18,data);
		if(zb==1)if(zg>0) set_in_block(bo-lvl1x*lvl1y-1,of+16+16*18*18,data);
		if(yb==16)if(yg<lvl1y-1) set_in_block(bo+lvl1x-1		,of-16*18+16,data);
		if(zb==16)if(zg<lvl1z-1) set_in_block(bo+lvl1x*lvl1y-1,of-16*18*18+16,data);
	}
	if(yb==1)if(yg>0)
	{
		set_in_block(bo-lvl1x		 ,of+16*18,data);
		
		if(zb==1)if(zg>0)        set_in_block(bo-lvl1x*lvl1y-lvl1x,of+16*18+16*18*18,data);
		if(zb==16)if(zg<lvl1z-1) set_in_block(bo+lvl1x*lvl1y-lvl1x,of+16*18-16*18*18,data);
	}
	if(zb==1)if(zg>0) set_in_block(bo-lvl1x*lvl1y,of+16*18*18,data);
		
	if(xb==16)if(xg<lvl1x-1)
	{
		set_in_block(bo+1			,of-16,data);

		if(yb==16)if(yg<lvl1y-1) set_in_block(bo+lvl1x+1		,of-16*18-16,data);
		if(zb==16)if(zg<lvl1z-1) set_in_block(bo+lvl1x*lvl1y+1,of-16*18*18-16,data);
		if(yb==1)if(yg>0) set_in_block(bo-lvl1x+1 ,of-16+16*18,data);
		if(zb==1)if(zg>0) set_in_block(bo-lvl1x*lvl1y+1,of-16+16*18*18,data);
	}
	if(yb==16)if(yg<lvl1y-1)
	{
		set_in_block(bo+lvl1x		,of-16*18,data);

		if(zb==16)if(zg<lvl1z-1) set_in_block(bo+lvl1x*lvl1y+lvl1x,of-16*18*18-16*18,data);
		if(zb==1) if(zg>0)       set_in_block(bo-lvl1x*lvl1y+lvl1x,of-16*18+16*18*18,data);
	}
	if(zb==16)if(zg<lvl1z-1) set_in_block(bo+lvl1x*lvl1y,of-16*18*18,data);
}


vec3f get_normal(int x,int y,int z)
{
	vec3f n_default (1,0,0);
	int xg=x>>4;if(xg>=lvl1x)return n_default;if(x<0)return n_default;
	int yg=y>>4;if(yg>=lvl1y)return n_default;if(y<0)return n_default;
	int zg=z>>4;if(zg>=lvl1z)return n_default;if(z<0)return n_default;

	uint bo=xg+(yg+zg*lvl1y)*lvl1x;

	Block &b=blocks[bo];
	if(b.vxl.size()==0)return n_default;

	uint intensity=get_voxel(x,y,z).intensity;
	if(intensity<0x3fff)return n_default;

	vec3f n(0,0,0);
	loopijk(-1,-1,-1,2,2,2)
	{
		int a=get_voxel(x+i,y+j,z+k).intensity>>4;
		n.x+=i*int(a);
		n.y+=j*int(a);
		n.z+=k*int(a);
	}
	n=n*(-1);
	n.norm();
	return n;
}

float get_shade(int x,int y,int z)
{
	vec3f n=get_normal(x,y,z);
	n.x= n.x*127+128;
	n.y=-n.y*127+128;
	n.z= n.z*127+128;
	return  (n.x+n.y)/2;
}

void set_shade(int x,int y,int z)
{
	return;
	/*
	int xg=x>>4;if(xg>=lvl1x)return;if(x<0)return;
	int yg=y>>4;if(yg>=lvl1y)return;if(y<0)return;
	int zg=z>>4;if(zg>=lvl1z)return;if(z<0)return;

	uint bo=xg+(yg+zg*lvl1y)*lvl1x;

	Block &b=blocks[bo];
	if(b.vxl.size()==0)return;

	uint color=get_voxel(x,y,z);
	if(color<0x7fffffff)return;

	vec3f n(0,0,0);

	loopi(-1,2)
	loopj(-1,2)
	loopk(-1,2)
	{
		int a=get_voxel(x+i,y+j,z+k)>>24;
		n.x+=i*int(a);
		n.y+=j*int(a);
		n.z+=k*int(a);
	}
	n.norm();
	n.x= n.x*127+128;
	n.y=-n.y*127+128;
	n.z= n.z*127+128;

	float c=(n.x+n.y)/2;

	color=(color&0xffff0000)+uint(c);

	set_voxel(x,y,z,color);
	*/
}

