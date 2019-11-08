
int rnd_seed = 0;
float sample_rnd(int x,int y,int z)
{
	//return float((x&y&z)&1)*4;///1.5;//
	int a=rnd_seed+x+y+z;
	return float((rnd_seed+
		x*1531359+
		y*8437113+
		z*3413256+
		x*a*4134353+
		z*a*6466245+
		532515)&65535)/65535;
}

float cubicInterpolate (float p[4], float x) {
	return p[1] + 0.5 * x*(p[2] - p[0] + x*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + x*(3.0*(p[1] - p[2]) + p[3] - p[0])));
}

float bicubicInterpolate (float p[4][4], float x, float y) {
	float arr[4];
	arr[0] = cubicInterpolate(p[0], y);
	arr[1] = cubicInterpolate(p[1], y);
	arr[2] = cubicInterpolate(p[2], y);
	arr[3] = cubicInterpolate(p[3], y);
	return cubicInterpolate(arr, x);
}

float tricubicInterpolate (float p[4][4][4], float x, float y, float z) {
	float arr[4];
	arr[0] = bicubicInterpolate(p[0], y, z);
	arr[1] = bicubicInterpolate(p[1], y, z);
	arr[2] = bicubicInterpolate(p[2], y, z);
	arr[3] = bicubicInterpolate(p[3], y, z);
	return cubicInterpolate(arr, x);
}

float sample_vol(int px,int py,int pz,int level)
{
	int rndx=px>>level; float x01=(px&((1<<level)-1))/float(1<<level);
	int rndy=py>>level; float y01=(py&((1<<level)-1))/float(1<<level);
	int rndz=pz>>level; float z01=(pz&((1<<level)-1))/float(1<<level);

	int sizex=(lvl1x*lvl0)>>level;
	int sizey=(lvl1y*lvl0)>>level;
	int sizez=(lvl1z*lvl0)>>level;

	float a[4][4][4];
	loopijk(0,0,0,4,4,4)
		a[i][j][k]=sample_rnd( (i+rndx)%sizex, (j+rndy)%sizey, (k+rndz)%sizez);

	return tricubicInterpolate (a,  x01,  y01 ,z01);
}

float sample_vol2(int px,int py,int pz,int level)
{
	uint anx=(lvl1x*lvl0)-1;
	uint any=(lvl1y*lvl0)-1;
	uint anz=(lvl1z*lvl0)-1;
	uint an=(1<<level)-1;
	uint xi=px>>level;float x=float(px&an)/float(an+1);float x1=1-x;
	uint yi=py>>level;float y=float(py&an)/float(an+1);float y1=1-y;
	uint zi=pz>>level;float z=float(pz&an)/float(an+1);float z1=1-z;
	uint xi1=((px+(1<<level))&anx)>>level;
	uint yi1=((py+(1<<level))&any)>>level;
	uint zi1=((pz+(1<<level))&anz)>>level;
	
	float v000=sample_rnd(xi ,yi ,zi );
	float v001=sample_rnd(xi ,yi ,zi1);
	float v010=sample_rnd(xi ,yi1,zi );
	float v011=sample_rnd(xi ,yi1,zi1);
	float v100=sample_rnd(xi1,yi ,zi );
	float v101=sample_rnd(xi1,yi ,zi1);
	float v110=sample_rnd(xi1,yi1,zi );
	float v111=sample_rnd(xi1,yi1,zi1);

	return
		v000 * (x1)* (y1)* (z1)+
		v100 * ( x)* (y1)* (z1)+
		v010 * (x1)* ( y)* (z1)+
		v001 * (x1)* (y1)* ( z)+
		v101 * ( x)* (y1)* ( z)+
		v011 * (x1)* ( y)* ( z)+
		v110 * ( x)* ( y)* (z1)+
		v111 * ( x)* ( y)* ( z);
	
}
void make_terrain_perlin(int seed=0)
{
	rnd_seed = seed+seed*3456+seed*23521;
	loopi(0,lvl1x*lvl1y*lvl1z)blocks[i].clear();//.dirty=0;

	int sx=lvl1x*lvl0/1;float fx=sx;
	int sy=lvl1y*lvl0/1;float fy=sy;
	int sz=lvl1z*lvl0/1;float fz=sz;
	VoxelData vd;

	loopijk(0,0,0,sx,sy,sz)
	{
		float v=
			sample_vol(i,j,k,5)+
			sample_vol(i,j,k,4)/2+
			sample_vol(i,j,k,3)/4+
			sample_vol(i,j,k,2)/8+
			sample_vol(i,j,k,1)/16+
			0.6-1.2*float(j)/fy;
				
		v*=30000;

		v=min(v,65535);	v=max(v,0);
		vd.color=0xff;
		vd.intensity=v;//v;
		set_voxel(i,j,k,vd);
	}
}
