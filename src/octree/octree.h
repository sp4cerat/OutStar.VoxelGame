namespace octree// namespace begin
{
#define OCTREE_MAX_SIZE (256*1024*1024)
#define OCTREE_DEPTH 18
#define OCTREE_COMPACT_DEPTH 6
#define OCTREE_COMPACT_START (2097152*2)
#define OCTREE_COMPACT_BLOCK_START (1048576*3)
#define OCTREE_DIM (1<<OCTREE_DEPTH)
#define OCTREE_NODE_SHIFT 10

struct Node  // if members of this structure are changed, most cuda fetch-functions (and normal shaders) must be adjusted, too!
{
	uint childOffset[8];        // offset of each child
	uint normals[8];        // offset of each child
	uint color;
	uint normal;
};
struct ColorBlock
{
	std::vector<uint> input32; //24bit
	std::vector<uchar> output;//6bit per byte
	uint cmin,cmax;

	uint uncompress_dxt1(uchar color, uint cmin, uint cmax)
	{
		uint r1=cmin&255,g1=(cmin>>8)&255,b1=(cmin>>16)&255,a1=(cmin>>24)&255;
		uint r2=cmax&255,g2=(cmax>>8)&255,b2=(cmax>>16)&255,a2=(cmax>>24)&255;
		int weight1=color&3,weight2=3-weight1;

		r1=(r1*weight1+r2*weight2)>>2;
		g1=(g1*weight1+g2*weight2)>>2;
		b1=(b1*weight1+b2*weight2)>>2;
		if (weight1>=1) a1=a2;

		return r1+(g1<<8)+(b1<<16)+(a1<<24);
	}

	void compress_dxt1()
	{
		output.clear();

		int minr=255,ming=255,minb=255,mina=255,mini=255*4;
		int maxr=0  ,maxg=0  ,maxb=0  ,maxa=0  ,maxi=0;

		loopi(0,input32.size())
		{
			uint 
			color=input32[i],
			r=color&255,g=(color>>8)&255,b=(color>>16)&255,a=(color>>24)&255,
			intensity=r+g+b;

			if(intensity<=mini)	{ mini=intensity;	minr=r;ming=g;minb=b;mina=a;	}
			if(intensity>=maxi)	{ maxi=intensity;	maxr=r;maxg=g;maxb=b;maxa=a;	}
		}
		
		cmin=minr+(ming<<8)+(minb<<16)+(mina<<24);
		cmax=maxr+(maxg<<8)+(maxb<<16)+(maxa<<24);

		loopi(0,input32.size())
		{
			uint 
			color=input32[i],
			r=color&255,g=(color>>8)&255,b=(color>>16)&255,
			intensity=r+g+b;
			float value=0;
			if(maxi!=mini) value=(intensity-mini)*3/float(maxi-mini);
			output.push_back(uint(value));
		}
		input32.clear();
	}
};
struct NormalBlock
{
	std::vector<uint> input32; //24bit
	std::vector<vec3f> input; //float
	std::vector<uchar> output;//6bit per byte
	uint base;

	uint uncompress_3dcx(uchar pack, uint base32)
	{
		vec3f base;
		base.decode_normal_uint(base32);

		// matrix
		vec3f mx=base,my,mz,n,a;
		my.cross(mx,vec3f(0, mx.y>0.5 ? 0:1 , mx.y>0.5 ? 1:0  ));
		mz.cross(mx,my);
		mx.norm();
		my.norm();
		mz.norm();
		vec3f mxt(mx.x,my.x,mz.x);
		vec3f myt(mx.y,my.y,mz.y);
		vec3f mzt(mx.z,my.z,mz.z);
		float scale=base32>>24;

		a.y=(a.y-3.0)*scale/(3.0*255);
		a.z=(a.z-3.0)*scale/(3.0*255);
		a.x=sqrt(1-a.y*a.y-a.z*a.z);
						
		n.x=mxt.dot(a);
		n.y=myt.dot(a);
		n.z=mzt.dot(a);

		return n.encode_normal_uint();
	}


	void compress_3dcx(bool debug=0)
	{
		input.clear();
		output.clear();

		vec3f n_avg(0,0,0);

		// if many equal normals, use the most popular
		
		uint maxeq=0,numeq=0,maxn=0;
		{
			std::vector<uint> inp=input32;
			std::sort (inp.begin(), inp.end());
			//loopi(0,inp.size())	inp[i]&=0xfefefe;
			loopi(0,inp.size()-1)	
			{
				if((inp[i]&0xfefefe)==(inp[i+1]&0xfefefe))
					numeq++;
				else
				{
					if(numeq>maxeq){maxeq=numeq;maxn=inp[i-1];}
					numeq=0;
				}
			}
		}

		loopi(0,input32.size())
		{
			vec3f n;
			n.decode_normal_uint(input32[i]);
			input.push_back( n );
			if(debug)printf("n %2.2f %2.2f %2.2f\n",n.x,n.y,n.z);

			n_avg=n_avg+n;
		}
		n_avg.norm();

		if(1)
		if(input.size()>=4)
		{
			float delta_avg=0;
			loopi(0,input.size())
			{
				float l=(input[i]-n_avg).len();
				delta_avg+=sqrt(l);
			}
			delta_avg/=float(input.size());
			delta_avg=delta_avg*delta_avg;
			vec3f n_avg2=vec3f(0,0,0);
			loopi(0,input.size())
			{
				if((input[i]-n_avg).len()<=delta_avg) n_avg2=n_avg2+input[i];
			}
			n_avg2.norm();
			n_avg=n_avg2;
		}




		// closest to avg
		if(0)
		{			
			if(0)
			{
				float lmin=99;int n_final=0;

				loopi(0,input.size())
				{
					float l=(input[i]-n_avg).len();
					if( l<lmin ) { lmin=l;n_final=i;}
				}
				n_avg=input[n_final];
			}
			else
			{
				float lmin=99;
				/*
				int n_final=0;
				vec3f inp[6+8+12]=
				{
					vec3f(0,0, 1),	vec3f(0,0,-1),
					vec3f(0, 1,0),	vec3f(0,-1,0),
					vec3f( 1,0,0),	vec3f(-1,0,0),
					vec3f(-1,-1,-1),	vec3f(-1,-1, 1),	
					vec3f(-1, 1,-1),	vec3f(-1, 1, 1),	
					vec3f( 1,-1,-1),	vec3f( 1,-1, 1),	
					vec3f( 1, 1,-1),	vec3f( 1, 1, 1),
					vec3f( 1, 0, 1),vec3f( 1, 0,-1),vec3f(-1, 0, 1),vec3f(-1, 0,-1),
					vec3f( 1,  1,0),vec3f( 1, -1,0),vec3f(-1,  1,0),vec3f(-1, -1,0),
					vec3f(0, 1,  1),vec3f(0, 1, -1),vec3f(0,-1,  1),vec3f(0,-1, -1)
				};*/
				/*
				vec3f n_final;
				loopi(0,6)
				loopj(-4,5)
				loopk(-4,5)
				{
					//vec3f n=inp[i];n.norm();
					vec3f n;
					n[(i  )%3]=(i>=3) ? 1 : -1;
					n[(i+1)%3]=float(j)/4.0;
					n[(i+2)%3]=float(j)/4.0;

					float l=(n-n_avg).len();
					if( l<lmin ) { lmin=l;n_final=n;}
				}
				n_avg=n_final;n_avg.norm();
				*/
			}
		}

		base=n_avg.encode_normal_uint();
		
		if(0)
		if(maxeq>4)
		{
			base=maxn;n_avg.decode_normal_uint(maxn); n_avg.norm();
			printf("max eq %d %x \n",maxeq,maxn);
		}
		
		
		// matrix
		vec3f mx=n_avg,my,mz;
		//vec3f some(0.32145,0.143453,sqrt(1.0-0.143453*0.143453-0.32145*0.32145));
		vec3f some(0,0,1);
		if(n_avg.z>0.7 || n_avg.z<-0.7)some=vec3f(0,1,0);

		my.cross(mx,some);//vec3f(0,0,1));//vec3f(0, mx.y>0.5 ? 1:0 , mx.y>0.5 ? 0:1  ));
		mz.cross(mx,my);
		mx.norm();
		my.norm();
		mz.norm();
		matrix33 m;
		m.set(	mx.x,my.x,mz.x,
				mx.y,my.y,mz.y,
				mx.z,my.z,mz.z);

		matrix33 m_inv=m;
		m_inv.transpose();

		if(debug)printf("\n");
		
		float max_yz=0;

		loopi(0,input.size())
		{
			vec3f n=m*input[i];
			max_yz=max(max_yz,n.y >= 0 ? n.y : -n.y );
			max_yz=max(max_yz,n.z >= 0 ? n.z : -n.z );
		}
		float scale=255;

		max_yz=int(max_yz*scale);

		base|=uint(max_yz)<<24; // scale

		if(debug)printf("max %2.2f \n",max_yz);
		if(debug)printf("\n");

		vec3f mxt(mx.x,my.x,mz.x);
		vec3f myt(mx.y,my.y,mz.y);
		vec3f mzt(mx.z,my.z,mz.z);

		loopi(0,input.size())
		{
			vec3f n=m*input[i];

			if(debug)printf("o %2.2f %2.2f %2.2f\n",n.x,n.y,n.z);

			vec3f a;
			a.x=n.x<0 ? 1 : 0;
			a.y=n.y*3.0*scale/max_yz+3.0;//a.y=int(a.y);
			a.z=n.z*3.0*scale/max_yz+3.0;//a.z=int(a.z);
			
			int packy=a.y+0.5,packz=a.z+0.5;

			output.push_back( packy+packz*8 );

			if(debug) 
			{
		//		printf("a %2.2f %2.2f %2.2f\n",a.x,a.y,a.z);
				a.y=(float(int(a.y))-3.0)*max_yz/(3.0*scale);
				a.z=(float(int(a.z))-3.0)*max_yz/(3.0*scale);
				a.x=sqrt(1-a.y*a.y-a.z*a.z);
						
				vec3f n_orig1=input[i];
				vec3f n_orig2;
				n_orig2.x=mxt.dot(a);
				n_orig2.y=myt.dot(a);
				n_orig2.z=mzt.dot(a);

				if(1-a.y*a.y-a.z*a.z<0.0) 
				{
					printf("error packy %d packz %d a.y %2.2f a.z %2.2f\n",packy,packz,a.y,a.z);
					printf("sum2 %2.2f\n",a.y*a.y+a.z*a.z);
					printf("max yz %2.2f scale %2.2f\n",max_yz,scale);
					
					printf("a %2.2f %2.2f %2.2f\n",a.x,a.y,a.z);
					printf("n %2.2f %2.2f %2.2f\n",n.x,n.y,n.z);
					printf("* %2.2f %2.2f %2.2f\n",n_orig1.x,n_orig1.y,n_orig1.z);
					printf("* %2.2f %2.2f %2.2f\n",n_orig2.x,n_orig2.y,n_orig2.z);
					printf("e %2.2f deg\n",a.angle(n)*180/M_PI);
					printf("\n");
				}
			}
		}
		input32.clear();
	}

};

uint root = 0;
uint root_compact = 0;
std::vector<Node>	octree_array; // CPU
std::vector<uint>	octree_array_compact; // -> to GPU

int _iteration = 0;

uint octree_normal_offset=0; // 0-2M octree_array_compact offset 
uint octree_block_offset=OCTREE_COMPACT_BLOCK_START; // block area

uint get_bytebitcount(uint b)
{
	b = (b & 0x55) + (b >> 1 & 0x55);
	b = (b & 0x33) + (b >> 2 & 0x33);
	b = (b + (b >> 4)) & 0x0f;
	return b;
}	

void clear()
{
	octree_array.clear();
	octree_array_compact.clear();

	Node node;
	memset (&node,0,sizeof(	Node ));
	octree_array.push_back(node);

	octree_normal_offset=0;
}

//FILE* fff=fopen("../test.txt","wb");

void set_voxel(uint x,uint y,uint z,uint color,uint normal=255,bool debug=0)
{
	//x+=OCTREE_DIM/2+256;
	//y+=OCTREE_DIM/2;
	//z+=OCTREE_DIM/2;
	//num_voxels++;
	uint offset = 0;
	uint parent_offset = 0;
	uint parent_childbit = 0;
	uint bit = OCTREE_DEPTH-1;
	uint child_offset  =0 ;
	uint childbit = 0;

	if(normal==0)normal=255;

	Node node;
	memset (&node,0,sizeof(	Node ));

	node.color=color;
	node.normal=normal;

	while(bit>=0)
	{
		uint bitx = (x >> bit)&1;
		uint bity = (y >> bit)&1;
		uint bitz = (z >> bit)&1;

		parent_childbit=childbit;

		childbit =  (bitx+bity*2+bitz*4);

		if((offset>>8)>=octree_array.size())error_stop("out of range");
		child_offset = octree_array[offset>>8].childOffset[childbit];
		
		bool newvoxel=0;

		if (bit<OCTREE_DEPTH-1) 
		{
			if((parent_offset>>8)>=octree_array.size())error_stop("out of range");

			newvoxel=(octree_array[parent_offset>>8].childOffset[parent_childbit] & (1<<childbit)) ? 0 : 1;

			octree_array[parent_offset>>8].childOffset[parent_childbit] |= 1<<childbit;
		}
		else
			root |= 1<<childbit;

		if (bit == 0 )
		{
			if((offset>>8)>=octree_array.size())error_stop("out of range");
			octree_array[offset>>8].childOffset[childbit]=color;
			octree_array[offset>>8].normals[childbit]=normal;

			//if(newvoxel) fPP(fff,"%d %d %d %d %d %d\n",x,y,z,
			//	normal&255, (normal>>8)&255,(normal>>16)&255 );

			break;
		}

		parent_offset=offset;

		if (child_offset==0)
		{	
			//node.color.w=bit;
			uint insert_ofs = octree_array.size()<<8;
			if((offset>>8)>=octree_array.size())error_stop("out of range");
			octree_array[offset>>8].childOffset[childbit]=insert_ofs;
			octree_array.push_back( node );
			offset = insert_ofs;
		}
		else
		{
			offset = child_offset;
		}
		bit --;
	}
}

///////////////////////////////////////////

uint child_4x4x4_total=0;
uint child_4x4x4_n=0;
uint stat_num_voxel=0;

void set_octree_compact(uint index,uint val)
{
	if(index>=octree_array_compact.size())
		error_stop("set_octree_compact out of bounds %d > %d",index,octree_array_compact.size());

	octree_array_compact[index]=val;
}

uint convert_subtree(uint offset,uint local_root)
{
	_iteration++;

	Node node = octree_array[offset>>8];

	uint osize=octree_array_compact.size();
	uint current_ofs = ((osize-local_root) << OCTREE_NODE_SHIFT) + 256*0 + (offset&0xff) ;

	if (_iteration != OCTREE_DEPTH-1 )
	{
		uint usedchilds=2+get_bytebitcount(offset&255);
		octree_array_compact.resize(osize+usedchilds);
		set_octree_compact(osize++,node.color);

		loopj(0,8)
		{
			uint childofs = node.childOffset[j];
			if ( childofs > 0 )
			{
				set_octree_compact(osize++,convert_subtree(childofs,local_root));
			}
		}
		//set_octree_compact(osize++,node.color); // normal
		set_octree_compact(osize++,node.normal); // normal

		_iteration--;

		return current_ofs;
	}

	static std::vector<uchar> color_block;
	color_block.clear();
	color_block.push_back(0);//node.color);

	// 4x4 block
	//
	// 2 col 2*(555 col 2 spc 2 glow 2 reflect 2 ext)=64 bit
	// 3 normal 24bit
	//
	static NormalBlock nblock; 
	static ColorBlock  cblock;

	loopj(0,8)
	{
		uint childofs = node.childOffset[j];
		if ( childofs > 0 )
			color_block.push_back(childofs); // childbits
	}
	
	loopj(0,8)
	{
		uint childofs = node.childOffset[j];
		if ( childofs > 0 )
		{
			Node n = octree_array[childofs>>8];
			loopi(0,8)
			{
				uint color=n.childOffset[i];
				uint normal=n.normals[i];

				if ( normal > 0 )
				{
					stat_num_voxel++;
					nblock.input32.push_back(normal);
					cblock.input32.push_back(color);
				}
			}
		}
	}
	nblock.compress_3dcx();
	cblock.compress_dxt1();

	loopi(0,nblock.output.size())
	{
		color_block.push_back( 
			 cblock.output[i] 
		|	(nblock.output[i] <<2 ));
	}

	octree_array_compact.push_back(cblock.cmin);//color 1
	octree_array_compact.push_back(cblock.cmax);//color 2
	octree_array_compact.push_back(nblock.base);//normal 24 + open angle 8

	uint dw=0, i03=0;
	loopi (0, color_block.size())
	{
		i03=i&3;
		dw|=uint(color_block[i])<<(i03*8);
		if(i03==3)
		{
			octree_array_compact.push_back(dw);
			dw=0;
		}
	}
	if(i03<3) octree_array_compact.push_back(dw);

	_iteration--;
	return current_ofs+(3<<OCTREE_NODE_SHIFT);
}

uint convert_tree(uint offset)
{
	_iteration++;

	Node node = octree_array[offset>>8];

	for (int j=0;j<8;j++)
	{
		uint childofs = node.childOffset[j];

		if ( childofs > 0 )
		{
			if (_iteration < OCTREE_DEPTH-OCTREE_COMPACT_DEPTH ) 
			{
				// normal nodes			
				node.childOffset[j] = convert_tree(childofs);
			}			
			else
			{
				// attach 64 block
				uint local_root=((octree_array_compact.size()+63)>>6)<<6;
				octree_array_compact.resize(local_root);
				node.childOffset[j] = ((local_root>>6)<<OCTREE_NODE_SHIFT) | (0<<8) | (childofs&255);
				convert_subtree(childofs,local_root);
			}
		}
	}

	loopj(0,8)//for (int j=0;j<8;j++)
	{
		octree_array_compact[octree_normal_offset++]=node.childOffset[j];
	}
	octree_array_compact[octree_normal_offset++] = *((int*)(&node.color)) ;
	octree_array_compact[octree_normal_offset++] = *((int*)(&node.normal)) ;

	_iteration--;

	return ((octree_normal_offset-10)<< OCTREE_NODE_SHIFT) | (0<<8) | (offset & 255);
}

void compact()
{
	printf("--- %s\n",__FUNCTION__);

	stat_num_voxel=0;

	if(octree_array_compact.size()==0) octree_array_compact.resize(OCTREE_COMPACT_START);

	root_compact = convert_tree( root );

	uint size_uncompressed=octree::octree_array.size()*sizeof(octree::Node);
	uint size_compressed1=(octree_array_compact.size()-OCTREE_COMPACT_START)*4;
	uint size_compressed2=octree_normal_offset*4;

	printf("Octree normal  size : %d MB (%2.2f bytes/voxel)\n",
		size_uncompressed/(1024*1024),float(size_uncompressed)/float(stat_num_voxel));

	std::vector<Node>().swap(octree_array);

    printf("Octree compact total: %2.2f MB\n",float((octree_array_compact.size()*4)/(1024*1024)));	
    printf("Octree compact used: %2.2f MB\n",float(size_compressed1)/(1024*1024));	
    printf("Octree compact main: %2.2f kb\n",float(size_compressed2/(1024)));	
    printf("Octree compact : %2.2f bytes/voxel\n",float(size_compressed1+size_compressed2)/float(stat_num_voxel));	
    printf("Octree compression ratio 1:%2.2f\n",float(size_uncompressed)/float(size_compressed1+size_compressed2));	

}

void save_compact(char* filename)
{
	uint zipped_len=0;
	uint* zipped=(uint*)zip::compress((uchar*)&octree_array_compact[0], octree_array_compact.size()*4, &zipped_len);
	uint size=octree_array_compact.size();

	zipped=(uint*)realloc(zipped,zipped_len+16);
	memmove(((uchar*)zipped)+16,zipped,zipped_len);

	zipped[0]=size;
	zipped[1]=zipped_len;
	zipped[2]=root_compact;
	zipped[3]=octree_normal_offset;

	printf("size:%d\n",size);
	printf("zipped_len:%d\n",zipped_len);
	printf("root_compact:%d\n",root_compact);

	file_write(filename, (char*)zipped,zipped_len+16);

	free(zipped);
}
void load_compact(char* filename)
{
	printf("octree::load_compact:%s\n\n",filename);

	octree_array_compact.clear();

	uint  file_len=0;
	uint* zipped = (uint*)file_read(filename,&file_len);
	
	uint size			=zipped[0];
	uint zipped_len		=zipped[1];
	root_compact		=zipped[2];
	octree_normal_offset=zipped[3];

	printf("size:%d\n",size);
	printf("zipped_len:%d\n",zipped_len);
	printf("root_compact:%d\n",root_compact);

	uint  unzipped_len=0;
	uint* data=(uint*)zip::uncompress((uchar*)&zipped[4], zipped_len, &unzipped_len);

	octree_array_compact.resize(size);
	memcpy(&octree_array_compact[0],data,size*4);

	free(zipped);
	free(data);
}

namespace collide // namespace begin
{
	uint get_bitcount(uint b)
	{
		b = (b & 0x55) + (b >> 1 & 0x55);
		b = (b & 0x33) + (b >> 2 & 0x33);
		b = (b + (b >> 4)) & 0x0f;
		return b;
	}

	bool voxel(int x,int y,int z)
	{
		uint nodeid = octree::root_compact;
		uint rekursion = OCTREE_DEPTH-1;
		uint local_root=0;

		while(rekursion>0)
		{
			uint bitx = ((x) >> rekursion)&1;
			uint bity = ((y) >> rekursion)&1;
			uint bitz = ((z) >> rekursion)&1;
			uint node_index =  (bitx+bity*2+bitz*4);

			if( (nodeid & (1<<node_index))==0 ) 
			{
				//printf("collide 0 level %d node bits %x test %d\n",rekursion,nodeid&255,node_index );
				return false;
			}

			uint node_test = nodeid & (1<<node_index);

			if( node_test  ) 
			if(rekursion<=2)
			{
				//printf("collide 1 level %d node bits %x test %d \n",rekursion,nodeid&255,node_index );
				return true; 
			}

			uint n=nodeid>>OCTREE_NODE_SHIFT;
			uint nadd=node_index;
			uint shr=0;
			if (rekursion<=5)//nodeid&256)
			{
				if (local_root==0){local_root=n=n<<6;} else n+=local_root;
				nadd = get_bitcount(nodeid & ((node_test<<1)-1));
				if(rekursion==2){shr=(nadd&3)<<3;nadd>>=2;}
			}
			nodeid=octree::octree_array_compact[n+nadd]>>shr;
			rekursion --;
		}

		return false;
	}

	uint fetchNormal(uint *octree,
					uint a_node,
					uint a_node_before, 
					uint a_node_before2, 
					uint local_root, 
					uint rekursion,
					uint child_test,
					uint node_index_lvl2)
	{
		uint n=a_node>>OCTREE_NODE_SHIFT;
		uint nadd=8;
		if(rekursion==1)  
		{
				n=node_index_lvl2;
				uint ofs=(a_node_before2>>OCTREE_NODE_SHIFT)+(local_root);
				nadd=0+get_bitcount(a_node_before2&255);
				loopi(1,n)
				{
					nadd+=get_bitcount((octree[ofs+(i>>2)]>>((i&3)<<3))&255);
				}
				nadd += get_bitcount(a_node_before & ((child_test<<1)-1));

				uint pack=(octree[ofs+(nadd>>2)]>>((nadd&3)<<3));

				NormalBlock nb;
				//ulong color32=uncompress_dxt1(pack, octree[ofs-3], octree[ofs-2]);
				ulong normal32=nb.uncompress_3dcx(pack, octree[ofs-1]);
				return normal32;//+(color32<<32);
		}
		if(rekursion==2)
		{
			n=node_index_lvl2;
			uint ofs=(a_node_before>>OCTREE_NODE_SHIFT)+(local_root);
			nadd=1+get_bitcount(a_node_before&255);
			loopi(1,n)
			{
				nadd+=get_bitcount((octree[ofs+(i>>2)]>>((i&3)<<3))&255);
			}
			uint pack=octree[ofs+(nadd>>2)]>>((nadd&3)<<3);
			//ulong color32=octree[ofs-3];//uncompress_dxt1(pack, octree[ofs-3], octree[ofs-2]);
			ulong normal32=octree[ofs-1];//uncompress_3dcx(pack, octree[ofs-1]);
			return normal32;//+(color32<<32);
		}
		if(rekursion==3)
		{
			uint ofs=(a_node>>OCTREE_NODE_SHIFT)+(local_root);
			ulong color32=octree[ofs-3];//uncompress_dxt1(pack, octree[ofs-3], octree[ofs-2]);
			ulong normal32=octree[ofs-1];//uncompress_3dcx(pack, octree[ofs-1]);
			return normal32;//+(color32<<32);
		}
		if (rekursion<=6)//if (a_node&256)
		{
			if (rekursion==6)//if (!(a_node_before&256))		//if (rekursion==6)
			{
				(local_root)=n<<6; 
				n=n^n;
			}
			n+=(local_root);	
			ulong color32=octree[n];
			ulong normal32=octree[n+1+get_bitcount(a_node&255)];
			return normal32;//+(color32<<32);
		}	
		ulong color32=octree[n+8];
		ulong normal32=octree[n+9];
		return normal32;//+(color32<<32);
	}


	bool intersect_box_circle(float cx, float cy, float radius, float left, float top, float right, float bottom)
	{
	   float closestX = (cx < left ? left : (cx > right ? right : cx));
	   float closestY = (cy < top ? top : (cy > bottom ? bottom : cy));
	   float dx = closestX - cx;
	   float dy = closestY - cy;

	   return ( dx * dx + dy * dy ) <= radius * radius;
	}

	vec3f aabb_normal;
	vec3f aabb_hit;
	int aabb_hit_x;
	int aabb_hit_y;
	int aabb_hit_z;
	int aabb_hit_count;

	bool aabb(
		int px1,int py1,int pz1,
		int px2,int py2,int pz2,
		int ax=0,int ay=0,int az=0,
		uint nodeid = octree::root_compact,
		uint rekursion = OCTREE_DEPTH-1,
		uint local_root=0)
	{
		if(rekursion == OCTREE_DEPTH-1)
		{
			aabb_normal = vec3f(0,0,0);
			aabb_hit_x=aabb_hit_y=aabb_hit_z=aabb_hit_count=0;			
			px1*=-1;	px2*=-1;
			py1*=-1;	py2*=-1;
			px1+=OCTREE_DIM/2;	px2+=OCTREE_DIM/2;vswap(px1,px2);
			py1+=OCTREE_DIM/2;	py2+=OCTREE_DIM/2;vswap(py1,py2);
			pz1+=OCTREE_DIM/2;	pz2+=OCTREE_DIM/2;
		}

		static int stack[100];
		static int stack_nodetest[100];

		static int sign_x=0,sign_y=0,sign_z=0,swapxz=0;

		stack[rekursion]=nodeid;

		int s=1<<rekursion;

		int rx=(px2-px1)/2;
		int rz=(pz2-pz1)/2;
		int rxz=(rx+rz)/2;//sqrt(float(rx*rx+rz*rz));

		bool hit=false;

		loopi(0,8)
		{
			uint x1=ax+s*(i&1);
			uint y1=ay+s*((i>>1)&1);
			uint z1=az+s*((i>>2)&1);
			uint x2=x1+s,y2=y1+s,z2=z1+s;

			if(px2<x1 || py2<y1 || pz2<z1)continue;
			if(px1>x2 || py1>y2 || pz1>z2)continue;

			if(!intersect_box_circle(
				px1+rx,
				pz1+rz,
				rxz,
				x1,z1,x2,z2)) continue ;

			uint bitx =  i    &1;
			uint bity = (i>>1)&1;
			uint bitz = (i>>2)&1;
			int node_index= (bitx<<0)|(bity<<1)|(bitz<<2);

			if (rekursion<6)
			{
				bitx^=sign_x;bity^=sign_y;bitz^=sign_z;
				node_index= ((bitx<<swapxz)|(bity<<1)|(bitz<<(swapxz^2)));
			}
			uint node_test = nodeid & (1<<node_index);

			stack_nodetest[rekursion]=node_test;

			if( node_test==0 ) continue;

			if( rekursion == 1 )
			{
				uint n_int=fetchNormal(
					&octree::octree_array_compact[0],
					nodeid,
					stack[rekursion+1],
					stack[rekursion+2],
					local_root,
					rekursion,
					node_test,
					stack_nodetest[rekursion+1]);

				vec3f n;n.decode_normal_uint(n_int);
				aabb_normal=aabb_normal+n;

				aabb_hit_x+=(x1+x2)/2;
				aabb_hit_y+=(y1+y2)/2;
				aabb_hit_z+=(z1+z2)/2;
				aabb_hit_count++;
				//aabb_normal.z=-aabb_normal.z;

				hit=true;
			}
			else
			{
				uint n=nodeid>>OCTREE_NODE_SHIFT;
				uint nadd=i;
				uint shr=0;
				uint lr=local_root;

				if( rekursion <= 5 )//if (nodeid&256)
				{
					if (lr==0){lr=n=n<<6;} else n+=lr;
					nadd = get_bitcount(nodeid & (node_test*2-1));
					if(rekursion==2){shr=(nadd&3)<<3;nadd>>=2;}
				}

				n=octree::octree_array_compact[n+nadd]>>shr;

				if (rekursion==6){uint a=(n>>8)&3;swapxz=a&2;a&=1;sign_x=0^a;sign_z=0^a;}
				//if (rekursion==7){uint a=(n>>8)&3;swapxz=a&2;a&=1;sign_x=0^a;sign_z=0^a;}
				//if (rekursion==8){uint a=(n>>8)&3;swapxz=a&2;a&=1;sign_x=0^a;sign_z=0^a;}

			//	printf("r:%d - %d %d %d\n",rekursion,x1,y1,z1);

				if (aabb(	px1,py1,pz1,
							px2,py2,pz2,
							x1,y1,z1,
							n,
							rekursion-1,
							lr) ) hit=true;
			}
		}
		if(hit)if(rekursion == OCTREE_DEPTH-1)
		{
			aabb_normal.norm();
			aabb_hit_x/=aabb_hit_count;
			aabb_hit_y/=aabb_hit_count;
			aabb_hit_z/=aabb_hit_count;

			aabb_hit_x-=OCTREE_DIM/2;
			aabb_hit_y-=OCTREE_DIM/2;
			aabb_hit_z-=OCTREE_DIM/2;
			aabb_hit_x*=-1;
			aabb_hit_y*=-1;

			aabb_hit.x=aabb_hit_x;
			aabb_hit.y=aabb_hit_y;
			aabb_hit.z=aabb_hit_z;

		}
		return hit;
	}


}// namespace collide

} // namespace octree

namespace octreeblock // namespace begin
{

std::vector<uint> tmp_octree_array_compact;
uint tmp_octree_normal_offset;
uint tmp_root_compact;

struct SubOBlock { int x,y,z,root; };

struct OBlock 
{
	uint texture_id;// preview
	Bmp  texture;
	uint sx,sy,sz;	// dimension
	uint start,end;	// compact octree start/end
	uint root;		// root node
	std::vector<SubOBlock> suboblocks; // all root nodes for 64^3 subblocks

	void print()
	{
		printf("octree::block::OBlock-\n");
		//printf("-node count %d\n",nodes.size());
		printf("-dim %d %d %d\n",sx,sy,sz);
		printf("-start end %d \n",start,end);
		printf("-size %d kb\n",((end-start)*4)/1024);
		printf("-root %d\n",root>>OCTREE_NODE_SHIFT);

		/*
		loopi(0,suboblocks.size())
		{
			//SubOBlock sb=suboblocks[i];
			//printf("-block %d root %d xyz %d %d %d\n",i,sb.root,sb.x,sb.y,sb.z);
		}*/
	}
	void save(char* name)
	{
		char txt[1000];
		sprintf(txt,"%s.png",name);
		texture.save(txt);
		sprintf(txt,"%s.block",name);

		std::vector<uint> mem;
		mem.push_back(sx);
		mem.push_back(sy);
		mem.push_back(sz);
		mem.push_back(start);
		mem.push_back(end);
		mem.push_back(root);
		mem.push_back(suboblocks.size());

		loopi(0,suboblocks.size()*sizeof(SubOBlock)/4)
		{
			mem.push_back(((uint*)&suboblocks[0])[i]);
		}

		file_write(txt, (char*)&mem[0],mem.size()*4);
	}

	void load(char* name)
	{
		char txt[1000];
		sprintf(txt,"%s.png",name);
		texture.load(txt);
		texture_id=ogl_tex_bmp(texture);
		sprintf(txt,"%s.block",name);
		uint len;
		uint* mem=(uint*)file_read(txt,&len);
		int ofs=0;
		sx=mem[ofs];ofs++;
		sy=mem[ofs];ofs++;
		sz=mem[ofs];ofs++;
		start=mem[ofs];ofs++;
		end=mem[ofs];ofs++;
		root=mem[ofs];ofs++;
		uint s=mem[ofs];ofs++;
		suboblocks.resize(s);
		memcpy(&suboblocks[0],&mem[ofs],s*sizeof(SubOBlock));
		free(mem);
	}
};

std::vector<OBlock> oblocks;
OBlock tmp_block;

uint get_octree_compact(uint offset,uint child)
{
	offset>>=OCTREE_NODE_SHIFT;
	uint size=octree::octree_array_compact.size();
	if(offset+child>=size) error_stop("get_octree_compact %d out of bounds (%d)",offset+child,size);

	return octree::octree_array_compact[offset+child];
}

void set_octree_compact(uint offset,uint child,uint val)
{
	offset>>=OCTREE_NODE_SHIFT;
	uint size=octree::octree_array_compact.size();
	if(offset+child>=size) error_stop("set_octree_compact %d out of bounds (%d)",offset+child,size);

	octree::octree_array_compact[offset+child]=val;

	if(child<8)if((val>>OCTREE_NODE_SHIFT)>=size)
		error_stop("set_octree_compact node %d out of bounds (%d)",val>>OCTREE_NODE_SHIFT,size);
}

void set_subblock(int x,int y,int z,SubOBlock &sb,bool remove_block=0,int rotate=0)
{
	uint b_offset= ( remove_block ) ? 0 : sb.root;
	uint b_dim=1<<OCTREE_COMPACT_DEPTH;//max(bl.sx,max(bl.sy,bl.sz));
	uint b_addx=0,b_addy=0,b_addz=0;
	uint offset = octree::root_compact;
	uint parent_offset = 0;
	uint parent_childbit = 0;
	uint bit = OCTREE_DEPTH-1;

	uint bitstop = int(double(log(double(b_dim)))/double(log(2.0)));
	uint child_offset =0 ;
	uint childbit = 0;


	//printf("octree::block::set block id %d has %d levels\n",id,bitstop);

	while(bit>=0)
	{
		//b_offset=get_octree_compact(b_offset,0);
		uint bitx = ((x+b_addx) >> bit)&1;
		uint bity = ((y+b_addy) >> bit)&1;
		uint bitz = ((z+b_addz) >> bit)&1;
		parent_childbit=childbit;
		childbit =  (bitx+bity*2+bitz*4);
		child_offset = get_octree_compact(offset,childbit);//octree_array_compact[(offset>>9)+childbit];
				

		if (bit<OCTREE_DEPTH-1) 
			set_octree_compact(parent_offset,parent_childbit,
				get_octree_compact(parent_offset,parent_childbit)|(1<<childbit));
			//octree_array_compact[(parent_offset>>9)+parent_childbit] |= 1<<childbit;
		else
			octree::root_compact |= 1<<childbit;

		if (bit == bitstop )
		{
			set_octree_compact(offset,childbit,b_offset+rotate*256);
			//printf("octree::block::set at level %d = offset %d\n",bitstop,b_offset>>9);
			break;
		}

		parent_offset=offset;

		if (child_offset==0) // insert
		{	
			//set_octree_compact(octree::octree_normal_offset<<OCTREE_NODE_SHIFT,0,0<<28);
			//octree::octree_normal_offset+=1;

			uint insert_ofs = octree::octree_normal_offset<<OCTREE_NODE_SHIFT;

			//printf("octree::block::insert node %d level %d \n",insert_ofs>>9,bit);

			set_octree_compact(offset,childbit,insert_ofs);

			loopi(0,10)set_octree_compact(insert_ofs,i,0);
			set_octree_compact(insert_ofs,8,0xffff0000);
			set_octree_compact(insert_ofs,9,0xffffffff);

			offset = insert_ofs;

			octree::octree_normal_offset+=10;
		}
		else // continue
		{
			offset = child_offset;
			//printf("octree::block::node %d level %d (b node %d)\n",offset>>9,bit,b_offset>>9);
		}
		bit --;
	}

}

void set_block(int id,int x,int y,int z,bool remove_block=0,int rotate=0,OBlock *ob_ptr=0 )
{
	x*=-1;y*=-1;
	x+=OCTREE_DIM/2;
	y+=OCTREE_DIM/2;
	z+=OCTREE_DIM/2;

	//printf("------------------------------------\n");
	//printf("octree::block::set id %d at %d %d %d\n",id,x,y,z);

	if(id>=oblocks.size())return;

	OBlock &bl = ob_ptr ? *ob_ptr : oblocks[id];

	loopi(0,bl.suboblocks.size())
	{
		SubOBlock &sb=bl.suboblocks[i];

		int b_dim=1<<OCTREE_COMPACT_DEPTH;//max(bl.sx,max(bl.sy,bl.sz));
		uint b_offset= ( remove_block ) ? 0 : sb.root;
		int b_addx=sb.x*b_dim;
		int b_addy=sb.y*b_dim;
		int b_addz=sb.z*b_dim;
	
		if(rotate&1) // flip xz ?
		{
			b_addx=bl.sx-64-b_addx;
			b_addz=bl.sz-64-b_addz;
		}

		if(rotate&2) // rot xz ?
		{
			int a=b_addz;b_addz=b_addx;b_addx=a;
		}

		//set_subblock(x,y,z,sb,remove_block,rotate);
		
		uint offset = octree::root_compact;
		uint parent_offset = 0;
		uint parent_childbit = 0;
		uint bit = OCTREE_DEPTH-1;

		uint bitstop = int(double(log(double(b_dim)))/double(log(2.0)));
		uint child_offset =0 ;
		uint childbit = 0;

		//printf("octree::block::set block id %d has %d levels\n",id,bitstop);

		while(bit>=0)
		{
			//b_offset=get_octree_compact(b_offset,0);

			set_octree_compact(offset,8,0xffff0000);
			set_octree_compact(offset,9,0xffffffff);

			uint bitx = ((x+b_addx) >> bit)&1;
			uint bity = ((y+b_addy) >> bit)&1;
			uint bitz = ((z+b_addz) >> bit)&1;
			parent_childbit=childbit;
			childbit =  (bitx+bity*2+bitz*4);
			child_offset = get_octree_compact(offset,childbit);
				
			if (bit<OCTREE_DEPTH-1) 
				set_octree_compact(parent_offset,parent_childbit,
					get_octree_compact(parent_offset,parent_childbit)|(1<<childbit));
			else
				octree::root_compact |= 1<<childbit;

			if (bit == bitstop )
			{
				set_octree_compact(offset,childbit,b_offset|(rotate*256));
				set_octree_compact(offset,8,0xffff0000);
				set_octree_compact(offset,9,0xffffffff);
				break;
			}

			parent_offset=offset;

			if (child_offset==0) // insert
			{	
			//	set_octree_compact(octree::octree_normal_offset<<OCTREE_NODE_SHIFT,0,0<<28);
			//	octree::octree_normal_offset+=1;

				uint insert_ofs = octree::octree_normal_offset<<OCTREE_NODE_SHIFT;

				//printf("octree::block::insert node %d level %d \n",insert_ofs>>9,bit);

				set_octree_compact(offset,childbit,insert_ofs);

				//new node
				loopi(0,10)set_octree_compact(insert_ofs,i,0);//octree_array_compact[(insert_ofs>>9)+i]=0;
				set_octree_compact(insert_ofs,8,0xffff0000);//octree_array_compact[(insert_ofs>>9)+8]=octree_array_compact[(b_offset>>9)+8];
				set_octree_compact(insert_ofs,9,0xffffffff);//octree_array_compact[(insert_ofs>>9)+9]=octree_array_compact[(b_offset>>9)+9];

				offset = insert_ofs;

				octree::octree_normal_offset+=10;
			}
			else // continue
			{
				offset = child_offset;
				//printf("octree::block::node %d level %d (b node %d)\n",offset>>9,bit,b_offset>>9);
			}
			bit --;
		}
	} // subblock loop
	//printf("octree::block::set done\n");
	/*
	loopi(0,2000) 
	{
		set_octree_compact((i*11)<<OCTREE_NODE_SHIFT,1+8,0xffffffff);
		set_octree_compact((i*11)<<OCTREE_NODE_SHIFT,1+9,0xffffffff);
	}
	*/

}



void begin_oblock(int sx,int sy,int sz,Bmp &texture)
{
	/*
	printf("-------------------------------\n");
	printf("octree::block::begin\n");
	printf("-------------------------------\n");
	printf("octree_array_compact.size : %d kb\n",(octree::octree_array_compact.size()*4)/1024);
	printf("root_compact : %d\n",octree::root_compact);
	printf("octree_normal_offset : %d\n",octree::octree_normal_offset);
	printf("octree_block_offset : %d\n",octree::octree_block_offset );
	printf("-------------------------------\n");
	*/

	// clear
	octree::Node node;
	memset (&node,0,sizeof(	octree::Node ));
	std::vector<octree::Node>().swap(octree::octree_array);
	octree::octree_array.push_back(node);

	//backup original
	//tmp_octree_array_compact.clear();
	//loopi(0,octree_normal_offset) 
	//	tmp_octree_array_compact.push_back(octree_array_compact[i]);

	tmp_octree_normal_offset=octree::octree_normal_offset;
	tmp_root_compact=octree::root_compact;

	//clear original
	//root_compact=octree_block_offset<<9;
	octree::root=0;

	//init block
	octree::octree_normal_offset=octree::octree_block_offset;

	//tmp_block.nodes.clear();
	//texture.flip();
	tmp_block.texture_id=ogl_tex_bmp(texture);
	//texture.flip();
	tmp_block.texture=texture;
	tmp_block.sx=sx;
	tmp_block.sy=sy;
	tmp_block.sz=sz;
	tmp_block.start=octree::octree_array_compact.size();
}
/*
std::vector<SubOBlock> groupblocks;

void gather_group_blocks(uint offset,int x,int y,int z)
{
	static uint iteration=0;
	iteration++;

	uint *childs=&octree::octree_array_compact[offset>>OCTREE_NODE_SHIFT];

	loopi(0,8)
	{
		int xx=x*2+((i>>0)&1);
		int yy=y*2+((i>>1)&1);
		int zz=z*2+((i>>2)&1);

		uint node=childs[i];
		if( node     ==0) continue;
		if((node&255)==0) continue;
		//if((node&256)==0) 
		if(iteration<OCTREE_DEPTH-OCTREE_COMPACT_DEPTH)
		{
			gather_sub_blocks(node,xx,yy,zz);
		}
		else
		{
			SubOBlock sb;
			sb.root=node;sb.x=xx;sb.y=yy;sb.z=zz;
			groupblocks.push_back(sb);
		}
	}

	iteration--;
}
*/
std::vector<SubOBlock> groupblocks;

void gather_groupblocks(
	int px1,int py1,int pz1,
	int px2,int py2,int pz2,
	int ax=0,int ay=0,int az=0,
	uint nodeid = octree::root_compact,
	uint rekursion = OCTREE_DEPTH-1)
{
	static int px0,py0,pz0;

	if(rekursion == OCTREE_DEPTH-1)
	{
		px1*=-1;	px2*=-1;
		py1*=-1;	py2*=-1;
		px1+=OCTREE_DIM/2;	px2+=OCTREE_DIM/2;vswap(px1,px2);
		py1+=OCTREE_DIM/2;	py2+=OCTREE_DIM/2;vswap(py1,py2);
		pz1+=OCTREE_DIM/2;	pz2+=OCTREE_DIM/2;
		px0=px1;py0=px1;pz0=px1;
	}
	int s=1<<rekursion;
	bool hit=false;

	loopi(0,8)
	{
		int x1=ax+s*(i&1);
		int y1=ay+s*((i>>1)&1);
		int z1=az+s*((i>>2)&1);
		int x2=x1+s,y2=y1+s,z2=z1+s;

		if(px2<x1 || py2<y1 || pz2<z1)continue;
		if(px1>x2 || py1>y2 || pz1>z2)continue;

		uint bitx =  i    &1;
		uint bity = (i>>1)&1;
		uint bitz = (i>>2)&1;
		uint node_test = nodeid & (1<<i);

		if( node_test==0 ) continue;

		uint n=nodeid>>OCTREE_NODE_SHIFT;
		uint node_new=octree::octree_array_compact[n+i];

		if( rekursion == 6 )
		{
			SubOBlock sb;
			sb.root=node_new;
			sb.x=(x1-px0)/64;sb.y=(y1-py0)/64;sb.z=(z1-pz0)/64;
			groupblocks.push_back(sb);
		}
		else
		{
			gather_groupblocks(	
						px1,py1,pz1,
						px2,py2,pz2,
						x1,y1,z1,
						octree::octree_array_compact[n+i],
						rekursion-1) ;
		}
	}
}


std::vector<SubOBlock> suboblocks;

void gather_sub_blocks(uint offset,int x,int y,int z)
{
	static uint iteration=0;
	iteration++;

	uint *childs=&octree::octree_array_compact[offset>>OCTREE_NODE_SHIFT];

	loopi(0,8)
	{
		int xx=x*2+((i>>0)&1);
		int yy=y*2+((i>>1)&1);
		int zz=z*2+((i>>2)&1);

		uint node=childs[i];
		if( node     ==0) continue;
		if((node&255)==0) continue;
		//if((node&256)==0) 
		if(iteration<OCTREE_DEPTH-OCTREE_COMPACT_DEPTH)
		{
			gather_sub_blocks(node,xx,yy,zz);
		}
		else
		{
			SubOBlock sb;
			sb.root=node;sb.x=xx;sb.y=yy;sb.z=zz;
			suboblocks.push_back(sb);
		}
	}

	iteration--;
}


void end_oblock()
{
	octree::compact();
	
	tmp_block.root=octree::root_compact;
	tmp_block.end=octree::octree_array_compact.size();

	// gather subblocks
	gather_sub_blocks(tmp_block.root,0,0,0);
	tmp_block.suboblocks=suboblocks;
	suboblocks.clear();
	
	//loopi(0,octree_normal_offset) 
	//	tmp_block.nodes.push_back(octree_array_compact[i]);

	tmp_block.print();

	oblocks.push_back(tmp_block);
	
	//restore original
	
	octree::octree_block_offset=octree::octree_normal_offset;

	//loopi(0,tmp_octree_normal_offset) 
	//	octree_array_compact.push_back(tmp_octree_array_compact[i]);

	octree::octree_normal_offset=tmp_octree_normal_offset;
	octree::root_compact=tmp_root_compact;
	/*
	printf("-------------------------------\n");
	printf("octree size : %d MB\n",octree::octree_array.size()*sizeof(octree::Node)/(1024*1024));
	printf("root_compact : %d\n",octree::root_compact);
//	printf("oblocksize : %d kb\n",);
	printf("-------------------------------\n");
	printf("octree_array_compact.size : %d kb\n",(octree::octree_array_compact.size()*4)/1024);
	printf("octree_normal_offset : %d\n",octree::octree_normal_offset);
	printf("octree_block_offset : %d\n",octree::octree_block_offset );
	printf("-------------------------------\n");
	printf("octree::block::end\n");
	*/
}

} // namespace block
