﻿
void save_obj( char* name_obj , int timestep=0 )
{
	FILE* fn = fopen("../materials.mtl","wb");
	if(fn==0)
	{
		PP("cannot open materials.mtl\n");return;
	}
	loopi(0,32)
	{
		float a=float(i)/32.0;
		float r=a;
		float g=a*2-1; if (g<0) g=-g; 
		float b=1.0-a;
		
		fPP(fn,"newmtl material_%d\n",i);
		fPP(fn,"Ka 0.200000 0.200000 0.200000\n");
		fPP(fn,"Kd %f %f %f\n",r,g,b);
		fPP(fn,"Ks 0.000000 0.000000 0.000000\n");
		fPP(fn,"Tr 1.000000\n");
		fPP(fn,"illum 2\n");
		fPP(fn,"Ns 0.000000\n\n");	
	}
	fclose(fn);

	fn = fopen(name_obj,"wb");
	if(fn==0)
	{
		error_stop("cannot open %s\n",name_obj);
	}
	
	//fPP(fn,"mtllib ./materials.mtl\n");
	
	loopi(0,lvl1x*lvl1y*lvl1z)
	loopj(0,blocks[i].tri.size()/5)
	{
		fPP(fn,"v %f %f %f \n",
			blocks[i].tri[j*5+0],
			blocks[i].tri[j*5+1],
			blocks[i].tri[j*5+2] );
	}
	int c=1;
	loopi(0,lvl1x*lvl1y*lvl1z)
	loopj(0,blocks[i].tri.size()/15)
	{
		fPP(fn,"f");
		loopk(0,3) {fPP(fn," %d//",c);c++;}
		fPP(fn,"\n");
	}
	fclose(fn);

	PP("%d faces\n",c/12);
}
void save_voxel(char* filename)
{
	std::vector<Block>().swap( blocks_undo ); // clear undo

	int nblocks=0;
	loopi(0, lvl1x*lvl1y*lvl1z)
		if(blocks[i].vxl.size()>0)nblocks++;

	uint blocksize=(lvl0+2)*(lvl0+2)*(lvl0+2);
	int totalsize=nblocks*blocksize+lvl1x*lvl1y*lvl1z+4;
	printf("total size : %d MB", (totalsize >> 18));

	VoxelData *data=(VoxelData*)malloc(totalsize*sizeof(VoxelData));
	if(data==0) error_stop("save_voxel::Out of memory allocating %d MB",(totalsize >> 18));

	data[0].color=lvl0;
	data[1].color=lvl1x;
	data[2].color=lvl1y;
	data[3].color=lvl1z;
	
	loopi(0, lvl1x*lvl1y*lvl1z)
		data[4+i].color=blocks[i].vxl.size();
		
	uint ofs=lvl1x*lvl1y*lvl1z+4;

	loopi(0, lvl1x*lvl1y*lvl1z)
	{
		Block &b=blocks[i];	
		loopj(0, b.vxl.size())
			data[ofs++]=b.vxl[j];
	}

	uint zipped_len=0;
	uchar* zipped=zip::compress((uchar*)&data[0], totalsize*sizeof(VoxelData), &zipped_len);
	free(data);

	file_write(filename, (char*)zipped,zipped_len);

	free(zipped);
	blocks_undo=blocks;
}


void load_from_voxel(char* filename)
{
	reset();

	uint   zipped_len=0;
	uint* zipped = (uint*)file_read(filename,&zipped_len);

	uint unzipped_len=0;
	VoxelData* data=(VoxelData*)zip::uncompress((uchar*)zipped, zipped_len, &unzipped_len);
	free(zipped);
	
	lvl0 =data[0].color; 
	lvl1x=data[1].color; 
	lvl1y=data[2].color; 
	lvl1z=data[3].color;

	blocks.resize	(lvl1x*lvl1y*lvl1z);

	printf("loaded %d x %d x %d blocks\n",lvl1x,lvl1y,lvl1z);
	printf("lvl0 %d -> %d x %d x %d voxels\n",lvl0,lvl0*lvl1x,lvl0*lvl1y,lvl0*lvl1z);

	uint ofs=4+lvl1x*lvl1y*lvl1z;
	uint blocksize=(lvl0+2)*(lvl0+2)*(lvl0+2);

	loopi(0, lvl1x*lvl1y*lvl1z)
	if(data[4+i].color>0)
	{
		blocks[i].dirty=true;
		blocks[i].vxl.resize(blocksize);
		loopj(0, blocksize) 
			blocks[i].vxl[j]=data[ofs++];
	}

	triangulate();
	blocks_undo=blocks;

	free(data);
}
