///////////////////////////////////////////
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable

//64 bit ignored
//#pragma OPENCL EXTENSION cl_khr_global_int64_base_atomics : enable
//#pragma OPENCL EXTENSION cl_khr_global_int64_extended_atomics : enable
///////////////////////////////////////////
__kernel void memset(__global uint *dst, uint dstofs, uint val ) {  dst[get_global_id(0)+dstofs] = val; }
__kernel void memcpy(__global uint *dst, uint dstofs, __global uint *src, uint srcofs) {  dst[dstofs+get_global_id(0)] = src[srcofs+get_global_id(0)]; }	
///////////////////////////////////////////
#define loopi(start_l,end_l) for ( int i=start_l;i<end_l;++i )
#define loopj(start_l,end_l) for ( int j=start_l;j<end_l;++j )
#define loopk(start_l,end_l) for ( int k=start_l;k<end_l;++k )
///////////////////////////////////////////
#define OCTREE_DEPTH 18
#define OCTREE_DEPTH_AND ((1<<OCTREE_DEPTH)-1)
#define OCTREE_NODE_SHIFT 10
#define VIEW_DIST_MAX 1600000
#define LOD_ADJUST_COARSE 2
#define LOD_ADJUST 2
#define SCALE_MAX (1 << (OCTREE_DEPTH + 1))
#define Z_CONTINUITY (0.00625f*1.0f)
//#define HOLE_PIXEL_THRESHOLD 4
///////////////////////////////////////////
inline uchar get_bitcount(uchar b)
{
	//return popcount(b);
	b = (b & 0x55) + (b >> 1 & 0x55);
	b = (b & 0x33) + (b >> 2 & 0x33);
	b = (b + (b >> 4)) & 0x0f;
	return b;
}	
///////////////////////////////////////////
inline uint uncompress_dxt1(uint color, uint cmin, uint cmax)
{
	uint r1=cmin&255,g1=(cmin>>8)&255,b1=(cmin>>16)&255,a1=(cmin>>24)&255;
	uint r2=cmax&255,g2=(cmax>>8)&255,b2=(cmax>>16)&255,a2=(cmax>>24)&255;
	int weight1=color&3,weight2=3-weight1;

	//r1=r1>>3;r1=r1<<3;	r2=r2>>3;r2=r2<<3; // 16bit test
	//g1=g1>>3;g1=g1<<3;	g2=g2>>3;g2=g2<<3;
	//b1=b1>>3;b1=b1<<3;	b2=b2>>3;b2=b2<<3;

	r1=(r1*weight2+r2*weight1)/3;
	g1=(g1*weight2+g2*weight1)/3;
	b1=(b1*weight2+b2*weight1)/3;
	if (weight1>=1) a1=a2;

	return r1+(g1<<8)+(b1<<16)+(a1<<24);
}
///////////////////////////////////////////
// encode_normal
inline unsigned int encode_normal_uint(float3 n)
{
	int ix=n.x*127+128.5;
	int iy=n.y*127+128.5;
	int iz=n.z*127+128.5;
	return ix+(iy<<8)+(iz<<16);
}
///////////////////////////////////////////
// decode_normal
inline float3 decode_normal_uint(int n32)
{
	float3 n;
	n.x=(n32&255)-128;n32>>=8;
	n.y=(n32&255)-128;n32>>=8;
	n.z=(n32&255)-128;
	return fast_normalize(n);
}
///////////////////////////////////////////
// decode_normal
inline float3 decode_color_uint(int n32)
{
	float3 n;
	n.x=(n32&255);n32>>=8;
	n.y=(n32&255);n32>>=8;
	n.z=(n32&255);n/=255.0f;
	return n;
}
///////////////////////////////////////////
// encode_normal
inline unsigned int encode_color_uint(float3 n)
{
	int ix=n.x*255;
	int iy=n.y*255;
	int iz=n.z*255;
	return ix+(iy<<8)+(iz<<16);
}
///////////////////////////////////////////
inline uint uncompress_3dcx(uint pack, uint base32)
{
	float3 base=decode_normal_uint(base32);
	
	// matrix
	float3 mx,my,mz,n,a,some;
	some.x=0;some.y=0;some.z=1;
	//some.x=0.32145;
	//some.y=0.143453;
	//some.z=sqrt(1.0f-0.143453f*0.143453f-0.32145f*0.32145f);
	
	if((base.z>0.7f || base.z<-0.7f)){some.y=1.0f;some.z=0.0f;}

	//{ some.x=1;some.y=0;some.z=0; }

	mx=base;//fast_normalize(base);
	my=cross(mx,some);
	mz=cross(mx,my);
	//mx=fast_normalize(mx);
	//my=fast_normalize(my);
	//mz=fast_normalize(mz);
	float3 mxt={mx.x,my.x,mz.x};
	float3 myt={mx.y,my.y,mz.y};
	float3 mzt={mx.z,my.z,mz.z};
	uint  scale32=base32>>24;
	float scale=scale32;

	a.y=(pack>>2)&7;
	a.z=(pack>>5)&7;

	a.y=(a.y-3.0f)*scale/(3.0f*255.0f);
	a.z=(a.z-3.0f)*scale/(3.0f*255.0f);
	a.x=sqrt(max(0.0f,1.0f-a.y*a.y-a.z*a.z));
						
	n.x=dot(mxt,a);
	n.y=dot(myt,a);
	n.z=dot(mzt,a);
	//n=fast_normalize(n);
	n=fast_normalize(n);
	
	return encode_normal_uint(n);
}
///////////////////////////////////////////
#define FETCH_CHILD {\
	uint n=nodeid>>OCTREE_NODE_SHIFT;\
	uint nadd=node_index;\
	uint shr=0;\
	/*if (rekursion==7){uint a=a_octree[n-1]>>28;swapxz=a&2;a&=1;sign_x=sign_x0^a;sign_z=sign_z0^a;}*/\
	if (rekursion<=6)\
	{\
		if (rekursion==6){local_root=n=n<<6;} else n+=local_root;\
		nadd = get_bitcount(nodeid & ((node_test<<1)-1));\
		if(rekursion==2){node_index_lvl2=nadd;shr=(nadd&3)<<3;nadd>>=2;}\
	}\
	nodeid=a_octree[n+nadd]>>shr;\
	if (rekursion==7){uint a=(nodeid>>8)&3;swapxz=a&2;a&=1;sign_x=sign_x0^a;sign_z=sign_z0^a;}\
}
///////////////////////////////////////////
inline ulong fetchColor(__global uint *octree,
				uint a_node,
				uint a_node_before, 
				uint a_node_before2, 
				uint local_root, 
				uint rekursion,
				uint child_test,
				uint node_index_lvl2,
				uint swapxz
				)
{
	//return (ulong)((local_root>>6)*0x1232445433+1)+0xff0000000000;

	uint n=a_node>>OCTREE_NODE_SHIFT;
	uint nadd=8;
	uint color32=0,normal32=0;
	
	if(rekursion<=2)  
	{
		n=node_index_lvl2;uint ofs=local_root;

		if(rekursion==1)  
		{
			ofs+=(a_node_before2>>OCTREE_NODE_SHIFT);
			nadd=0+get_bitcount(a_node_before2&255);
			nadd+= get_bitcount(a_node_before & ((child_test<<1)-1));
		}
		else // 2
		{
			ofs+=(a_node_before>>OCTREE_NODE_SHIFT);
			nadd=1+get_bitcount(a_node_before&255);
		}
		
		loopi(1,n)	nadd+=get_bitcount((octree[ofs+(i>>2)]>>((i&3)<<3))&255);

		uint pack=(octree[ofs+(nadd>>2)]>>((nadd&3)<<3));
		color32=uncompress_dxt1(pack, octree[ofs-3], octree[ofs-2]);
		normal32=uncompress_3dcx(pack, octree[ofs-1]);
		//return normal32+(color32<<32);
	}
	else
	if(rekursion==3)
	{
		uint ofs=n+local_root;
		color32=octree[ofs-3];//uncompress_dxt1(pack, octree[ofs-3], octree[ofs-2]);
		normal32=octree[ofs-1];//uncompress_3dcx(pack, octree[ofs-1]);
		//return normal32+(color32<<32);
	}
	else
	if (rekursion<=7)
	{
		if (rekursion==7){local_root=n=n<<6;} else n+=local_root;		
		color32=octree[n];
		normal32=octree[n+1+get_bitcount(a_node&255)];
		//return normal32+(color32<<32);
	}	
	else
	{
		color32=octree[n+8];
		normal32=octree[n+9];
	}
	
	uint nx=normal32&0x000000ff;
	uint ny=normal32&0xff00ff00;
	uint nz=normal32&0x00ff0000;
	if(swapxz&1)//mirror
	{
		nx=255-nx;
		nz=0xff0000-nz;
	}	
	if(swapxz&2)
	{
		nx<<=16;
		nz>>=16;
	}

	normal32=nx|ny|nz;
	ulong n64=normal32;
	ulong c64=color32;
	return n64|(c64<<32);
}
///////////////////////////////////////////
#define CAST_RAY(BREAK_COND) \
const int CUBESIZE=SCALE_MAX;uint node_index_lvl2=0;\
sign_x = 0, sign_y = 0, sign_z = 0;\
rekursion= OCTREE_DEPTH;\
float epsilon = pow(2.0f,-9.0f);\
if(fabs(delta.x)<epsilon)delta.x=sign(delta.x)*epsilon;\
if(fabs(delta.y)<epsilon)delta.y=sign(delta.y)*epsilon;\
if(fabs(delta.z)<epsilon)delta.z=sign(delta.z)*epsilon;\
\
if (delta.x < 0.0f){ sign_x = 1; raypos.x = SCALE_MAX-raypos.x; delta.x= -delta.x; }\
if (delta.y < 0.0f){ sign_y = 1; raypos.y = SCALE_MAX-raypos.y; delta.y= -delta.y; }\
if (delta.z < 0.0f){ sign_z = 1; raypos.z = SCALE_MAX-raypos.z; delta.z= -delta.z; }\
\
grad0.x = 1;\
grad0.y = delta.y/delta.x;\
grad0.z = delta.z/delta.x;\
\
grad1.x = delta.x/delta.y;\
grad1.y = 1;\
grad1.z = delta.z/delta.y;\
\
grad2.x = delta.x/delta.z;\
grad2.y = delta.y/delta.z;\
grad2.z = 1;\
\
len0.x = length(grad0);\
len0.y = length(grad1);\
len0.z = length(grad2);\
\
float3 raypos_orig=raypos;\
raypos_int = convert_int3(raypos);\
nodeid = octree_root;\
local_root = 0;\
x0r = 0; x0ry = 0;\
raypos_before=raypos_int ;\
int swapxz=0;\
int sign_x0=sign_x;\
int sign_z0=sign_z;\
int ccount=0;\
\
/*int3 mapPos = convert_int3(floor(raypos));\
float3 deltaDist = abs(convert_float3(length(delta)) / delta);\
int3 rayStep = convert_int3(sign(delta));\
float3 sideDist = (sign(delta) * (convert_float3(mapPos) - raypos) + (sign(delta) * 0.5f) + 0.5f) * deltaDist;\
int3 mask;*/\
\
do\
{\
	int3 p0i = raypos_int >> rekursion;    \
	int3 bit = (p0i & 1) ;\
	bit.x^=sign_x;bit.y^=sign_y;bit.z^=sign_z;\
\
	int node_index= ((bit.x<<swapxz)|(bit.y<<1)|(bit.z<<(swapxz^2)));\
	    node_test = nodeid /*childBits 0..8*/ & (1<<node_index);\
\
	if ( node_test )\
	{\
		FETCH_CHILD\
		if(rekursion<=lod)break;\
		rekursion--;\
		/*if(rekursion<15)*/stack[stack_add+min(15,rekursion)]=nodeid;\
		continue;\
	}\
\
	float3 mul0=convert_float3((p0i+1)<<rekursion)-raypos;\
	raypos_before = raypos_int;\
	float3 isec_dist = mul0 * len0;\
	float3 isec0 = grad0*mul0.x;\
	if (isec_dist.y<isec_dist.x){isec_dist.x=isec_dist.y;isec0=grad1*mul0.y;}\
	if (isec_dist.z<isec_dist.x){isec_dist.x=isec_dist.z;isec0=grad2*mul0.z;}\
	raypos    += isec0;	\
	raypos_int = convert_int3(raypos);\
	distance  += isec_dist.x;\
	int3 xor_xyz = raypos_before ^ raypos_int;\
	x0r =xor_xyz.x^xor_xyz.y^xor_xyz.z;\
\
	int rekursion_new=log2(convert_float(x0r&OCTREE_DEPTH_AND));\
	if(rekursion_new<rekursion) continue;\
	rekursion		= rekursion_new+1;\
	nodeid = stack[stack_add+rekursion];\
	if(rekursion<7) continue;\
	if(rekursion>=15) {nodeid = octree_root;rekursion=OCTREE_DEPTH;}\
	sign_x=sign_x0;sign_z=sign_z0;swapxz=0;\
	/*if(rekursion == OCTREE_DEPTH) nodeid = octree_root;*/\
	if(distance > lodswitch) { lodswitch*=2;++lod;if(distance > VIEW_DIST_MAX)break; }\
	if(BREAK_COND&((OCTREE_DEPTH_AND+1)<<1))break;\
\
}while(++ccount<400);\
if(sign_x0){raypos.x = SCALE_MAX-raypos.x; delta.x= -delta.x; };\
if(sign_y ){raypos.y = SCALE_MAX-raypos.y; delta.y= -delta.y; };\
if(sign_z0){raypos.z = SCALE_MAX-raypos.z; delta.z= -delta.z; };
///////////////////////////////////////////

struct BVHNode  // if members of this structure are changed, most cuda fetch-functions (and normal shaders) must be adjusted, too!
{
	uint childs[2];        // offset of each child
	float p[12];
	//float3 p[4];
};
struct BVHChild  // if members of this structure are changed, most cuda fetch-functions (and normal shaders) must be adjusted, too!
{
	float3 min,max;
	float4 m0;
	float4 m1;
	float4 m2;
	float4 m3;
	uint octreeroot;
};
///////////////////////////////////////////
__kernel    void raycast_splatting(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global ulong *a_color_normal,
			__global uint *mem_screen_pbo_mem,
			int  res_x, int res_y, int frame,int ofs_add,
			float4 a_m0, 
			float4 a_mx, 
            float4 a_my, 
			float4 a_mz,
			float fovx,
			float fovy)
			
{
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	const int idx = get_global_id(0)	,idy = get_global_id(1);
	if(idx >= res_x || idy >= res_y ) return;
	
	int tid=ty*bw+tx;
	int ofs=idy*res_x+idx;

	ofs+=ofs_add; //backbuffer
	uint srcofs=ofs;
	uint col=a_screenBuffer[ofs];
	if(col==0xffffff00) return;
	
	float3 pcache;
	pcache.x = a_backBuffer[ofs*3+0];
	pcache.y = a_backBuffer[ofs*3+1];
	pcache.z = a_backBuffer[ofs*3+2];

	ofs-=ofs_add; //frontbuffer

	float3 p=pcache.xyz-a_m0.xyz;
	float3 phit; 
	phit.x = dot(p,a_mx.xyz);
	phit.y = dot(p,a_my.xyz);
	phit.z = dot(p,a_mz.xyz);
	
	if (phit.z<0.05) { a_screenBuffer[srcofs]=0xffffff00; return;}

	float3 scrf01;
	scrf01.x=phit.x/(fovx*phit.z);
	scrf01.y=phit.y/(fovy*phit.z);
	float3 scrf;
	//scrf.x=scrf01.x*convert_float(res_y)+convert_float(res_x)/2;
	//scrf.y=scrf01.y*convert_float(res_y)+convert_float(res_y)/2;	
	scrf.x=scrf01.x+convert_float(res_x)/2;
	scrf.y=scrf01.y+convert_float(res_y)/2;
	scrf.z=phit.z;
	int2 scr;
	scr.x=scrf.x;
	scr.y=scrf.y;
	
	if(scr.x>=res_x-1 || scr.x<0 || scr.y>=res_y-1 || scr.y<0)	
	{
		a_screenBuffer[srcofs]=0xffffff00; 
		return;
	}

	ofs=scr.y*res_x+scr.x;									
	int val=phit.z*1024; 
	if(a_screenBuffer[ofs] <= val) { a_screenBuffer[srcofs]=0xffffff00; return;}
	if(atom_min(&a_screenBuffer[ofs],val)<val) return;

	barrier(CLK_GLOBAL_MEM_FENCE);

	if(a_screenBuffer[ofs]!=val) return;

	a_backBuffer[ofs*3+0] = pcache.x;
	a_backBuffer[ofs*3+1] = pcache.y;
	a_backBuffer[ofs*3+2] = pcache.z;
	a_color_normal [ofs] = a_color_normal [srcofs];
}
///////////////////////////////////////////
__kernel    void raycast_remove_outlier(
			__global uint *a_screenBuffer,
			//__global float *a_backBuffer,
			//__global int *a_ybuffer,
			int  res_x, int res_y, int frame)
{
	//__local 
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	const int idx = get_global_id(0)	,idy = get_global_id(1);
	if(idx >= res_x || idy >= res_y || idx<0 || idy<0 ) return;

	uint  of=idy*res_x+idx;
	/*
	if(idx < 3)			{a_ybuffer[of]=0xffffff00;return;}
	if(idx >= res_x-3)	{a_ybuffer[of]=0xffffff00;return;}
	if(idy < 3)			{a_ybuffer[of]=0xffffff00;return;}
	if(idy >= res_y-3)	{a_ybuffer[of]=0xffffff00;return;}
    */
	if(idx >= res_x-1 || idy >= res_y-1 || idx<1 || idy<1 ) return;

	uint  c0=a_screenBuffer[of];

	//a_ybuffer[of]=c0;

	if(c0==0xffffff00) return;

	float z0=convert_float(a_screenBuffer[of])/1024.0f;
	float z1=z0;
	uint  c1=c0;

	// todo add x/y velocity check
	
	loopi(-1,2) loopj(-1,2)
	{
		uint ofs=of+j*res_x+i;
		if(a_screenBuffer[ofs]==0xffffff00) continue;

		float z=convert_float(a_screenBuffer[ofs])/1024.0f;
		if(z<z1){ z1=z; }		
	}

	if(z0<=z1)return;

	//if(fabs(z1-z0)>min(z0,z1)*Z_CONTINUITY*4.0f)
	if (z1<80 )	if (z1+5<z0) 
	//if (abs(dx-dx0)>0 || abs(dy-dy0)>0 )
	//if ((dx>=0 && xi>=0)||(dx<=0 && xi<=0)||(dy>=0 && yj>=0)||(dy<=0 && yj<=0))
	{
		a_screenBuffer[of]=0xffffff00;
	}
}
///////////////////////////////////////////
__kernel    void raycast_counthole(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_idBuffer,
			int  res_x, int res_y, int frame,int HOLE_PIXEL_THRESHOLD)
			//int wx1,int wx2,int wy1,int wy2)
{
	const int idx = get_global_id(0);
	const int idy = get_global_id(1);
	if(idx >= res_x/16 ) return;
	if(idy >= res_y/16 ) return;
	
	int count=0;
	int x=idx*16;
	int y=idy*16;

	int ofs=x+y*res_x;
	int dx=min(res_x-x,16)/2;
	int dy=min(res_y-y,16)/2;

	loopj(0,dy) 
	loopi(0,dx) 
	{
		int o=ofs+i*2+j*2*res_x;
		int a=0;
		if(a_screenBuffer[o]==0xffffff00 )a++;   // || ((i^j)==(frame&15))) 
		if(a_screenBuffer[o+1]==0xffffff00 )a++;   // || ((i^j)==(frame&15))) 
		if(a_screenBuffer[o+1+res_x]==0xffffff00 )a++;   // || ((i^j)==(frame&15))) 
		if(a_screenBuffer[o+res_x]==0xffffff00 )a++;   // || ((i^j)==(frame&15))) 
		if(a>=HOLE_PIXEL_THRESHOLD)count+=4;
	}
	a_idBuffer[idx+idy*(res_x/16)]=count;
}
///////////////////////////////////////////
__kernel    void raycast_sumids(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_idBuffer,
			int  res_x, int res_y, int frame)
{
	const int idx = get_global_id(0);

	if(idx > 0 ) return;
	
	int ofs=0;
	int size=(res_x/16)*(res_y/16);

	loopi(0,size)
	{
		int len=a_idBuffer[i];
		a_idBuffer[i+size]=ofs;
		ofs+=len;
	}
	a_idBuffer[0]=ofs;	
}
///////////////////////////////////////////
__kernel    void raycast_writeids(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_idBuffer,
			int  res_x, int res_y, int frame,int HOLE_PIXEL_THRESHOLD)
			//int wx1,int wx2,int wy1,int wy2)
{
	const int idx = get_global_id(0);
	const int idy = get_global_id(1);
	if(idx >= res_x/16 ) return;
	if(idy >= res_y/16 ) return;

	int size=(res_x/16)*(res_y/16);
	int count=0;
	int x=idx*16;
	int y=idy*16;
	//if(x >=wx1 && x <wx2 && y >=wy1 && y <wy2) return;

	int ofs=x+y*res_x;	
	int dstofs=a_idBuffer[idx+idy*(res_x/16)+size]+size*2;
	int dx=min(res_x-x,16)/2;
	int dy=min(res_y-y,16)/2;

	loopj(0,dy) 
	loopi(0,dx) 
	{
		int a=0,o=ofs+i*2+j*2*res_x,val=x+i*2+((j*2+y)<<16);
		int v=val;

		if(a_screenBuffer[o]==0xffffff00 )			a++;
		if(a_screenBuffer[o+1]==0xffffff00 )		a++;
		if(a_screenBuffer[o+1+res_x]==0xffffff00 )	a++;
		if(a_screenBuffer[o+res_x]==0xffffff00 )	a++;

		if(a>=HOLE_PIXEL_THRESHOLD)
		{
			a_idBuffer[dstofs]=val;	dstofs++;
			a_idBuffer[dstofs]=val+1;	dstofs++;
			a_idBuffer[dstofs]=val+1+(1<<16);dstofs++;
			a_idBuffer[dstofs]=val+(1<<16);	dstofs++;
		}
	}
}
///////////////////////////////////////////
__kernel    void raycast_holes(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_octree,
			__global struct BVHNode *a_bvh_nodes,
			__global struct BVHChild *a_bvh_childs,
			__global uint *a_stack,
			__global uint *a_idbuffer,
		//	__global uint *a_mem_x,
			__global ulong *mem_colorbuffer,
			uint octree_root,
			int  res_x, int res_y, int frame, int idbuf_size,
			float4 a_cam, 
			float4 a_origin, 
            float4 a_dx, 
			float4 a_dy,
	
			float4 a_m0, 
			float4 a_mx, 
            float4 a_my, 
			float4 a_mz,
			float fovx,
			float fovy)
{
	//__local 
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	//const int idx = get_global_id(0)	,idy = get_global_id(1);
	const int id = get_global_id(0);
	if(id>=idbuf_size) return;

	int idsize=(res_x/16)*(res_y/16);
	uint idxy=a_idbuffer[id+idsize*2];

	int idx=idxy&0xffff;
	int idy=idxy>>16;

	if(idx >= res_x || idy >= res_y ) return;
	
	// 16x16 threads?
	__local uint stack[16000/4]; // octree stack
	int thread_num = tx + ty * bw;
	uint stack_add=(thread_num * 15)-1;

	float3 delta1;
	delta1.x=convert_float(idx-res_x/2+0.5f)*fovx;///convert_float(res_y);
	delta1.y=convert_float(idy-res_y/2+0.5f)*fovy;///convert_float(res_y);
	delta1.z=1;

	float3 delta;
	delta.x=dot(delta1,a_mx.xyz);//a_mx*delta1.x;
	delta.y=dot(delta1,a_my.xyz);
	delta.z=dot(delta1,a_mz.xyz);

	float rayLength=0,distance=0;
	ulong  col=0xffffffff;
	float3 phit;

	// Params
	float3 raypos = a_m0.xyz*2.0f;

	// --------- // Raycast > Begin
	int sign_x, sign_y, sign_z,rekursion;
	float3  grad0,grad1,grad2,len0;
	int3 raypos_int,raypos_before; 
	int lod=1.0f, x0r, x0rx, x0ry, x0rz, sign_xyz;
	uint nodeid, local_root, node_test ;
	float lodswitch = res_x*LOD_ADJUST;//3/2;
	CAST_RAY(x0r);
	// --------- // Raycast > End
	{
		if((distance >= VIEW_DIST_MAX)||((x0r&(2*OCTREE_DEPTH_AND+2))) ) 
		col = 0; 
		else
		col = fetchColor(  a_octree,
							nodeid,
							stack[stack_add+rekursion],
							stack[stack_add+rekursion+1],
							local_root, 
							rekursion,node_test,node_index_lvl2,
							swapxz+(sign_x0^sign_x)
							  );
		phit=raypos/2.0f;
	}	
	
	uint shadow = 0;
	if(col!=255)
	{
		// Shadow
		raypos-=delta*2.0f*(1<<lod);
		delta.x=0.5f;
		delta.y=0.5f;
		delta.z=0.5f;
		delta=normalize(delta);
		distance=0.0f;
		CAST_RAY(x0ry);
		if(raypos.y<8100.0f){ shadow=1;col=((col&0xfefefe00000000)>>shadow)+(col&0xff000000ffffffff); }
	}
	

	int ofs=idy*res_x+idx;
	//a_mem_x[ofs]=(idy*1080/res_y)*1920+(idx*1920/res_x);
	
	loopi(0,1)
	{
		mem_colorbuffer[ofs]= col;
		a_screenBuffer[ofs] = 0;//col;
		a_backBuffer[ofs*3+0] = phit.x;
		a_backBuffer[ofs*3+1] = phit.y;
		a_backBuffer[ofs*3+2] = phit.z;
		ofs+=res_x*res_y;
	}
}
///////////////////////////////////////////
__kernel    void raycast_block_coarse(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_octree,
			__global float *a_mem_coarse,
			__global uint  *a_mem_coarse_col,
			uint octree_root,
			int  res_x, int res_y, int frame, 
			int add_x,int add_y,
			int block_dx,int block_dy,
			float4 a_cam, 
			float4 a_origin, 
            float4 a_dx, 
			float4 a_dy,
	
			float4 a_m0, 
			float4 a_mx, 
            float4 a_my, 
			float4 a_mz,
			float fovx,
			float fovy)
{
	//__local 
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	
	int idx0= get_global_id(0)  ,idy0= get_global_id(1)  ;
	if (idx0 > block_dx || idy0 > block_dy ) return;

	int idx = add_x+idx0*4	,idy = idy0*4+add_y;
	
	// 16x16 threads?
	__local uint stack[16000/4]; // octree stack
	int thread_num = tx + ty * bw;
	uint stack_add=(thread_num * 15)-1;

	float3 delta1;
	delta1.x=(convert_float(idx-res_x/2.0f)+0.5f)*fovx;///convert_float(res_y);
	delta1.y=(convert_float(idy-res_y/2.0f)+0.5f)*fovy;///convert_float(res_y);
	delta1.z=1;

	float3 delta;
	delta.x=dot(delta1,a_mx.xyz);
	delta.y=dot(delta1,a_my.xyz);
	delta.z=dot(delta1,a_mz.xyz);
	
	float rayLength=0,distance=0;
		
	ulong  col=0xff0000f2;
	float3 phit;

	// Params
	float3 raypos = a_m0.xyz*2.0f;

	// --------- // Raycast > Begin
	int sign_x, sign_y, sign_z,rekursion;
	float3  grad0,grad1,grad2,len0;
	int3 raypos_int,raypos_before; 
	int lod=2.0f, x0r, x0rx, x0ry, x0rz, sign_xyz;
	uint nodeid, local_root, node_test ;
	float lodswitch = res_x*LOD_ADJUST_COARSE;//3/2;//*2;//*3/2;
	CAST_RAY(x0r);
	// --------- // Raycast > End

	a_mem_coarse[idy0*block_dx+idx0]=distance;
	/*
	col=0;//0x88;
	{
		if((distance >= VIEW_DIST_MAX)||((x0r&(2*OCTREE_DEPTH_AND+2))) ) 
			col = 0; 
		else
			col = fetchColor(	a_octree,
								nodeid,
								stack[stack_add+rekursion],
								stack[stack_add+rekursion+1],
								local_root, 
								rekursion,node_test ,node_index_lvl2,
								swapxz+(sign_x0^sign_x)
								 );
		a_mem_coarse_col[ idy0*block_dx+idx0 ]=col;
	}*/
	
}
///////////////////////////////////////////
__kernel    void raycast_block(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_octree,
			__global float *a_mem_coarse,
			__global uint *a_mem_coarse_col,
		//	__global uint *a_mem_x,
			__global ulong *mem_colorbuffer,
			uint octree_root,
			int  res_x, int res_y, int frame, 
			int add_x,int add_y,
			int block_dx,int block_dy,
			float4 a_cam, 
			float4 a_origin, 
            float4 a_dx, 
			float4 a_dy,
	
			float4 a_m0, 
			float4 a_mx, 
            float4 a_my, 
			float4 a_mz,
			float fovx,
			float fovy)
{
	//__local 
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	
	int idx0= get_global_id(0)  ,idy0= get_global_id(1)  ;
	int idx = idx0				,idy = idy0;

	idx+=add_x;
	idy+=add_y;

	if(idx >= res_x || idy >= res_y ) return;
	
	// 16x16 threads?
	__local uint stack[16000/4]; // octree stack
	int thread_num = tx + ty * bw;
	uint stack_add=(thread_num * 15)-1;

	float3 delta1;
	delta1.x=convert_float(idx-res_x/2+0.5f)*fovx;///convert_float(res_y);
	delta1.y=convert_float(idy-res_y/2+0.5f)*fovy;///convert_float(res_y);
	delta1.z=1;
	float3 dir_normalized=normalize(delta1);

	float3 delta;
	delta.x=dot(delta1,a_mx.xyz);
	delta.y=dot(delta1,a_my.xyz);
	delta.z=dot(delta1,a_mz.xyz);
	

	float rayLength=0;//,distance=0;
		
	int idy_coarse=idy0>>2;
	int idx_coarse=idx0>>2;
	int idy_coarse2=idy_coarse+1;
	int idx_coarse2=idx_coarse+1;
	float d1=a_mem_coarse[ idy_coarse*block_dx + idx_coarse ];
	float d2=a_mem_coarse[ idy_coarse*block_dx + idx_coarse2];
	float d3=a_mem_coarse[ idy_coarse2 *block_dx + idx_coarse ];
	float d4=a_mem_coarse[ idy_coarse2 *block_dx + idx_coarse2];
	float distance=max(min(min(min(d1,d2),d3),d4)-1.0f,0.0f);

	ulong  col=0xff0000f2;
	float3 phit;

	// Params
	
	float3 raypos = a_m0.xyz*2.0f +normalize(delta)*distance;;
//	float3 raypos = a_m0.xyz*16.0f+normalize(delta)*distance;;

	/*
	if(d1<=250*4)
	{
		col=a_mem_coarse_col[ idy_coarse*block_dx + idx_coarse ];
		phit=raypos/2.0f;
		int ofs=idy*res_x+idx;
		loopi(0,3)
		{	
			mem_colorbuffer[ofs]= col;
			a_screenBuffer[ofs] = dir_normalized.z*distance*1024.0f/2.0f;///16.0;//col;;0xff000000;//+col;//^35345;
			a_backBuffer[ofs*3+0] = phit.x;
			a_backBuffer[ofs*3+1] = phit.y;
			a_backBuffer[ofs*3+2] = phit.z;
			ofs+=res_x*res_y;
		}
		return;
	}*/
	

	// --------- // Raycast > Begin
	int sign_x, sign_y, sign_z,rekursion;
	float3  grad0,grad1,grad2,len0;
	int3 raypos_int,raypos_before; 
	int lod=1, x0r, x0rx, x0ry, x0rz, sign_xyz;
	uint	nodeid=octree_root, 
			local_root=0, 
			node_test ;
	float lodswitch = res_x*LOD_ADJUST;//3/2;//*3/2;
	
	if(distance>lodswitch) {lodswitch*=2;lod++;}
	if(distance>lodswitch) {lodswitch*=2;lod++;}
	if(distance>lodswitch) {lodswitch*=2;lod++;}
	if(distance>lodswitch) {lodswitch*=2;lod++;}
	
	
	col=0;//0x88;
	{
		CAST_RAY(x0r);

		if((distance >= VIEW_DIST_MAX)||((x0r&(2*OCTREE_DEPTH_AND+2))) ) 
			col = 0; 
		else
		{
			col = fetchColor(	a_octree,
								nodeid,
								stack[stack_add+rekursion],
								stack[stack_add+rekursion+1],
								local_root, 
								rekursion,node_test ,node_index_lvl2,
								swapxz+(sign_x0^sign_x) );
		}

	}
	phit=raypos/2.0f;
	uint shadow = 0;

	// Shadow
	{
		/*
		raypos-=delta*2.0f*(1<<lod);
		delta.x=0.5f;
		delta.y=0.5f;
		delta.z=0.5f;
		delta=normalize(delta);
		distance=0.0f;//2.0*(1<<lod);
		CAST_RAY(x0ry);
		if(raypos.y<8100.0f){ shadow=1;col=((col&0xfefefe00000000)>>shadow)+(col&0xff000000ffffffff); }
		*/

		/*
		barrier(CLK_LOCAL_MEM_FENCE);
		stack[tx+ty*16]=shadow;
		barrier(CLK_LOCAL_MEM_FENCE);
		shadow=0;
		loopi(0,2)loopj(0,2) shadow|=stack[((tx+ty*16)&0xee)+i+j*16];
		*/
		
		//if(col!=255){col=((col&15)>>shadow)+(col&0xf0);}
	}					
	
	int ofs=idy*res_x+idx;
	loopi(0,1)
	{	
		
		a_screenBuffer[ofs] = dir_normalized.z*distance*1024.0f/2.0f;
		a_backBuffer[ofs*3+0] = phit.x;
		a_backBuffer[ofs*3+1] = phit.y;
		a_backBuffer[ofs*3+2] = phit.z;

		/*
		//shadow
		raypos-=delta*2.0f*(1<<lod);
		delta.x=0.5;
		delta.y=0.5;
		delta.z=0.5;
		delta=normalize(delta);
		distance=0.0;//2.0*(1<<lod);
		CAST_RAY(x0ry);
		if(distance<20){ shadow=1;col=0xff000000ffffffff;}//((col&0xfefefe00000000)>>shadow)+(col&0xff000000ffffffff); }
		*/

		mem_colorbuffer[ofs]= col;
		ofs+=res_x*res_y;
	}
}
///////////////////////////////////////////
inline float3 get_col3f(uint c)
{
	float3 f;
	f.x=c&255;
	f.y=(c>>8)&255;
	f.z=(c>>16)&255;
	return f;
}
///////////////////////////////////////////
inline uint get_col3u(float3 c)
{
	return 
		min(255,convert_int(c.x))+
		min(255,convert_int(c.y))*256+
		min(255,convert_int(c.z))*256*256;
}
///////////////////////////////////////////
inline float4 get_col4f(uint c)
{
	float4 f;
	f.x=c&255;
	f.y=(c>>8)&255;
	f.z=(c>>16)&255;
	f.w=(c>>24)&255;
	return f;
}
///////////////////////////////////////////
inline uint get_col4u(float4 c)
{
	return convert_int(c.x)+convert_int(c.y)*256+convert_int(c.z)*256*256+convert_int(c.w)*256*256*256;
}
///////////////////////////////////////////
__kernel    void raycast_fillhole(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_tmp,
			int  res_x, int res_y, int frame
			//int wx1,int wx2,int wy1,int wy2
			//,float4 vx,float4 vy
			)
{
	//__local 
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	const int idx = get_global_id(0)	,idy = get_global_id(1);
	if(idx >= res_x || idy >= res_y  ) return;

	int ofs=idy*res_x+idx;
	uint col=a_screenBuffer[ofs];

	if(idx >= res_x-5 || idy >= res_y-5 || idx<=5 || idy<=5 )
	{
		a_tmp[ofs]=col;
	}
	//a_tmp[ofs]=col;return;
	//if(idx >=wx1 && idx <wx2 && idy >=wy1 && idy <wy2) return;

	//if(col!=0xffffff00) return;

	float zm=99999999999;
	if(col!=0xffffff00)zm=col/1000;//a_backBuffer[ofs*3+3];

	loopi(-3,4)
	loopj(-3,4)
	{
		int of=(idy+i)*res_x+(idx+j);
		if(a_screenBuffer[of]==0xffffff00)continue;
		float z=a_screenBuffer[of]/256000;
		if(z<zm)zm=z;
	}

	//if(fabs(z1-z0)>min(z0,z1)*Z_CONTINUITY*4.0)
	
	int r1=3,r2=4;
	if (zm>30 )r2--;
	if (zm>40 )r1--;
	if (zm>50 )r2--;
	if (zm>70 )r1--;
	if (zm>90 )r2--;
	if (zm>150 )r1--;
	if (zm>200 )r2--;
//	if (zm>320 )r--;

	uint c=0;
	float colcnt=0;
	float3 csum;
	csum.x=0;
	csum.y=0;
	csum.z=0;

	if(col!=0xffffff00){csum+=get_col3f(col);colcnt+=1.0f;c++;}

	float3 csumall;
	csumall.x=0.0f;
	csumall.y=0.0f;
	csumall.z=0.0f;

	loopi(-r1,r2)
	loopj(-r1,r2)
	{
		uint of=(idy+i)*res_x+(idx+j);
		uint col=a_screenBuffer[of];
		if(col==0xffffff00)continue;
		float z=a_screenBuffer[of]/1000;
		//if(fabs(zm-z)>zm*Z_CONTINUITY*32.0)
		//if(fabs(zm-z)<zm*Z_CONTINUITY*64.0)
		{
			c++;
			csum+=get_col3f(col);
			colcnt+=1.0;
		}
		csumall+=get_col3f(col);
	}
	uint colfinal=col;
	if(colcnt>0.0f)//if(colcnt>r1*r2/6) 
		colfinal=get_col3u(csum/colcnt); 

	if(colfinal==0xffffff00)
	{
		colfinal=get_col3u(csumall/convert_float(2*r1*2*r2));
	}

	
		a_tmp[ofs]=colfinal; 
}

__kernel    void raycast_fillhole_screen(
			__global uint *a_screenBuffer,
		//	__global float *a_colorBuffer,
			int  res_x, int res_y, int frame
			//int wx1,int wx2,int wy1,int wy2
			//,float4 vx,float4 vy
			)
{
	//__local 
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	const int idx = get_global_id(0)	,idy = get_global_id(1);
	if(idx >= res_x-3 || idy >= res_y-3 || idx<=3 || idy<=3 ) return;
	//if(idx >=wx1 && idx <wx2 && idy >=wy1 && idy <wy2) return;

	int ofs=idy*res_x+idx;
	uint col=a_screenBuffer[ofs];
	if(col!=0xffffff00 ) return;

	//a_screenBuffer[ofs+1*res_x*res_y]=0xffffff00;
	//a_screenBuffer[ofs+2*res_x*res_y]=0xffffff00;

	int col1=a_screenBuffer[ofs+1];
	int col2=a_screenBuffer[ofs-1];
	int col3=a_screenBuffer[ofs+res_x];
	int col4=a_screenBuffer[ofs-res_x];

	if(col1!=0xffffff00 ) 
	if(col2!=0xffffff00 ) 
	if(col3!=0xffffff00 ) 
	if(col4!=0xffffff00 ) 
	{

		a_screenBuffer[ofs]=( (col1&0xfcfcfcfc)+(col2&0xfcfcfcfc)+(col3&0xfcfcfcfc)+(col4&0xfcfcfcfc))>>2;
		return;
	}

	if(col1!=0xffffff00 ) 
	if(col2!=0xffffff00 ) 
	{
		a_screenBuffer[ofs]=( (col1&0xfefefefe)+(col2&0xfefefefe) )>>1;
		return;
	}

	if(col3!=0xffffff00 ) 
	if(col4!=0xffffff00 ) 
	{
		a_screenBuffer[ofs]=( (col3&0xfefefefe)+(col4&0xfefefefe) )>>1;
		return;
	}

	loopi(0,4)
	{
		int o=ofs+(i&1)+((i>>1)&1)*res_x;
		col=a_screenBuffer[o];
		if(col!=0xffffff00)break;
	}

	
	if(col==0xffffff00 )
	loopi(-2,3)
	loopj(-2,3)
	{
		int o=ofs+i+j*res_x;		
		col=a_screenBuffer[o];
		if(col!=0xffffff00)break;
	}	
	if(col==0xffffff00 )
	loopi(-3,4)
	loopj(-3,4)
	{
		int o=ofs+i+j*res_x;		
		col=a_screenBuffer[o];
		if(col!=0xffffff00)break;
	}

	a_screenBuffer[ofs]=col;
}


__kernel    void raycast_colorize(
			__global ulong *mem_colorbuffer,
			__global uint *p_depthBuffer,
			__global float *p_backBuffer,
			__global uint *p_screenBuffer_tex,
			//__global uint *p_xy,
			//__global uint *p_palette,
			//float4 camera_dir,
			float4 light_pos,
			float4 camera_pos,
			float4 camera_x  ,
			float4 camera_y  ,
			float4 camera_z  ,
			float4 color_effect  ,
			int p_width, 
			int p_height
			)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	if( x >= p_width ) return;
	if( y >= p_height ) return;
	
	int of=x+y*p_width;
	uint col=p_depthBuffer[of];
	
	if(col!=0xffffff00)
	{	
		float z=p_backBuffer[of*4+3];
		ulong cn=mem_colorbuffer[of];

		if(cn==0)
		{
			col=0xff888888; // background
		}
		else
		{
			float3 n_in=decode_normal_uint(cn);
			float3 n=n_in;
			n.x=-dot(n_in,camera_x.xyz);
			n.y=-dot(n_in,camera_y.xyz);
			n.z=-dot(n_in,camera_z.xyz);

			float3 p,view_in;
			p.x=p_backBuffer[of*3+0];
			p.y=p_backBuffer[of*3+1];
			p.z=p_backBuffer[of*3+2];

			uint cc=cn>>32;

			float3 c=decode_color_uint(cc);

			// specular
			float specular_power=(cn>>(32+24))&3; specular_power/=3.0f;specular_power*=specular_power;

			// effects
			float effect_power=1.0f;
			int effect_index=(cn>>(32+28))&15;
			
			if(effect_index==1) effect_power=color_effect.x;
			if(effect_index==2) effect_power=1.0f-color_effect.x;
			if(effect_index==3) effect_power=color_effect.y;
			if(effect_index==4) effect_power=color_effect.z;
			if(effect_index==5) effect_power=color_effect.w;
			
			view_in=p-camera_pos.xyz;
			float3 view ;
			view.x=-dot(view_in,camera_x.xyz);
			view.y=-dot(view_in,camera_y.xyz);
			view.z=-dot(view_in,camera_z.xyz);
			view=fast_normalize(view);		

			float3 light = light_pos.xyz;
			
			{
				float  diffuse = clamp (dot ( light , n ),-1.0f,1.0f)*0.5f+0.5f;//clamp ( dot ( light , n ) ,0.0,1.0 );
				float3 r =  -view - 2.0f * dot(n, -view) * n; //reflect ( -view, n );
				//r=0.7*r;//fast_normalize(r);
				float  m = clamp ( dot ( r , light ) ,0.0f,1.0f);
				float  specular = m*m; specular=specular*specular*specular_power;
				
				float3 diffuse3; 
				diffuse3.z=clamp(diffuse,0.0f,1.0f);
				diffuse3.x=diffuse3.z=diffuse3.y=diffuse3.z*diffuse3.z;
				//diffuse3.x=(diffuse > 0.0f) ? diffuse3.z:diffuse*diffuse*diffuse*diffuse*0.5;

				diffuse3.x=clamp(diffuse3.x,0.0f,1.0f);
				diffuse3.y=clamp(diffuse3.y,0.0f,1.0f);
				diffuse3.z=clamp(diffuse3.z,0.0f,1.0f);

				diffuse3=diffuse3*effect_power;
				specular=specular*effect_power;
				c=c*diffuse*effect_power+specular;
				c=clamp(c,0.0f,1.0f);
				
			}
			uint color32=encode_color_uint(c);
			col=(color32);//&0xf0f0f0;//+(int(z)<<24);
		}

	}
	p_screenBuffer_tex[of]=col;
	//p_depthBuffer[of]=x^y;
	//p_screenBuffer_tex[of_dst]=col;
}

