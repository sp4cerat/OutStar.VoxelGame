namespace raycast
{

cl_mem	mem_octree;
cl_mem	mem_bvh_nodes;
cl_mem	mem_bvh_childs;
cl_mem	mem_backbuffer;
cl_mem	mem_depthbuffer;
int   	pbo_depthbuffer;
cl_mem	mem_colorbuffer;
cl_mem	mem_screen;
int		pbo_screen;
cl_mem	mem_stack;




void octree_init()
{

	octree::clear();

	if(0)
	{
		int max_i=1000;
		int max_j=1000;
		loopi(0,max_i)
		loopj(0,max_j)
		{
			float a=float(i)*2*M_PI/max_i;
			float b=float(j)*  M_PI/max_j;
			vec3f p( sin(a)*sin(b) , cos(a)*sin(b) , cos(b) );

			loopk(0,2)
			loopl(0,2)
			octree::set_voxel( 
				-p.x*50+50+k*100 +100+OCTREE_DIM/2,
				-p.y*50+50+OCTREE_DIM/2,
				-p.z*50+50+l*100+OCTREE_DIM/2,
				0x0000ff00+k*50+l*50*256*256+k*256*256*256,
				p.encode_normal_uint());
		}		
	}	
	//octree::compact();
	//RLE4 rle;rle.load("../voxelstein.rle4",0,OCTREE_DIM/2+256,OCTREE_DIM/2,OCTREE_DIM/2);
	//RLE4 rle;rle.load("../imrodh.rle4",0,OCTREE_DIM/2+256,OCTREE_DIM/2,OCTREE_DIM/2);
	octree::compact();	
	//octree::load_compact("../imrod.oct");
	//rle.load("../../data/sphereN.rle4");
	//Tree tree(2048,4096,2048);
	//tree.loadPLY("../imrod.ply");
	//printf("Compressing Octree\n");
	
	//octree::save_compact("../imrod.oct");
	printf("GPU octree size: %2.3f MB\n",float(octree::octree_array_compact.size()*4)/float(1024*1024));
}

void update_gpu_octree()
{
	ocl_copy_to_device(&octree::octree_array_compact[0], mem_octree, octree::octree_array_compact.size()*4, 0);
	//printf("raycast::update_gpu_octree done\n");
}

void init()
{
	octree_init();

	// octree
	mem_octree=ocl_malloc(OCTREE_MAX_SIZE);//octree::octree_array_compact.size()*4);//,&octree::octree_array_compact[0] );	
	update_gpu_octree();

	int size=WINDOW_WIDTH_MAX*WINDOW_HEIGHT_MAX /*4 buffers*/;//WINDOW_WIDTH*WINDOW_HEIGHT*4;
	int num_buffers=4;
	mem_backbuffer=ocl_malloc(size*16*num_buffers); 
	mem_colorbuffer=ocl_malloc(size*4*num_buffers*2); //64bit
	pbo_depthbuffer=ogl_pbo_new(size*4*num_buffers);
	mem_depthbuffer=ocl_pbo_map(pbo_depthbuffer);
	pbo_screen=ogl_pbo_new(size*4);
	mem_screen=ocl_pbo_map(pbo_screen);
}

void quit()
{
	ocl_pbo_unmap(mem_depthbuffer);
	CL_CHECK(clReleaseMemObject(mem_depthbuffer));
	ocl_exit();
}

void calcViewPlaneGradients(
	float res_x,float res_y,
	vec3f right,vec3f up,vec3f direction,
	vec3f& a_leftTop,vec3f& a_dx,vec3f& a_dy)
{
   a_leftTop=direction*2.0f-right-up;
   a_dx = right *2.0f/float(res_x);
   a_dy = up    *2.0f/float(res_y);
}

void draw(	
	int res_x, int res_y,
	vec3f pos,vec3f rot, 
	FBO& fbo,
	int block_x,int block_y,
	int hole_pixel_threshold,
	float fovx,float fovy,
	bool fxaa)
{
	int size_col=res_x*res_y*4; // color buffer size in bytes
	int size_xyz=size_col*3;	// xyz   buffer size in bytes

	matrix44 m;
	m.rotate_z( rot.z );
	m.rotate_x( rot.x );
	m.rotate_y( rot.y );
	
	vec3f front	= m * vec3f( 0, 0, 1);
	vec3f side	= m * vec3f( 1, 0, 0);
	vec3f up	= m * vec3f( 0, 1, 0);

	vec3f pos_before=pos;

	ocl_begin_all_kernels();

	ocl_pbo_begin(mem_screen);
	ocl_pbo_begin(mem_depthbuffer);

	// init
	if(FRAME<2)
	{
		ocl_memset(mem_depthbuffer, 0, 0xffffff00, WINDOW_WIDTH_MAX*WINDOW_HEIGHT_MAX*4*4);
	}

	/*fill*/
	vec4f v0=pos;
	vec4f vx=m*vec4f(1,0,0,0);
	vec4f vy=m*vec4f(0,1,0,0);
	vec4f vz=m*vec4f(0,0,1,0);

	int t[100];int t_cnt=0;
//	if(frame==0)printf("%d:project\n",t_cnt);
//	t[t_cnt++]=timeGetTime();

	// raycasted part
	int numpixels=WINDOW_WIDTH_MAX*WINDOW_HEIGHT_MAX;
	{
		m.transpose();
	}
	{
		matrix44 mat=m;
		//glGetFloatv(GL_MODELVIEW_MATRIX,  &mat.m[0][0]);
		mat.transpose();
		sunlightvec_objspc=mat*vec4f(sunlightvec.x,sunlightvec.y,sunlightvec.z,0);
		sunlightvec_objspc.norm();
	}

	// common rendering start

	if(1)
	{
		vec4f vx=m*vec4f(1,0,0,0);
		vec4f vy=m*vec4f(0,1,0,0);
		vec4f vz=m*vec4f(0,0,1,0);

		vec3f a_leftTop,a_dx,a_dy;
		calcViewPlaneGradients(	res_x,res_y,  side,up,front,a_leftTop,a_dx,a_dy);	
		
		//int block_x=block_x;
		//int block_y=block_y;
		int block_xy=block_x*block_y;
		int add_x=(res_x/block_x)*(FRAME%block_x);
		int add_y=(res_y/block_y)*((FRAME/block_x)%block_y);

		int block_dx=(res_x/block_x)/4+1;
		int block_dy=(res_y/block_y)/4+1;

		static cl_mem mem_coarse=ocl_malloc(4*((2+WINDOW_WIDTH_MAX/4)*(2+WINDOW_HEIGHT_MAX/4))); 
		static cl_mem mem_coarse_col=ocl_malloc(4*((2+WINDOW_WIDTH_MAX/4)*(2+WINDOW_HEIGHT_MAX/4))); 
		//if(FRAME%2==0)
		if(1)
		{
			static cl_kernel kernel = ocl_get_kernel("raycast_block_coarse");
			ocl_begin(&kernel,block_dx,block_dy,16,16);
			ocl_param(sizeof(cl_mem), &mem_depthbuffer);
			ocl_param(sizeof(cl_mem), &mem_backbuffer);
			ocl_param(sizeof(cl_mem), (void *) &mem_octree);
			ocl_param(sizeof(cl_mem), (void *) &mem_coarse);
			ocl_param(sizeof(cl_mem), (void *) &mem_coarse_col);
			ocl_param(sizeof(cl_int), (void *) &octree::root_compact);
			ocl_param(sizeof(cl_int), (void *) &res_x );
			ocl_param(sizeof(cl_int), (void *) &res_y );
			ocl_param(sizeof(cl_int), (void *) &FRAME );
			ocl_param(sizeof(cl_int), (void *) &add_x );
			ocl_param(sizeof(cl_int), (void *) &add_y );
			ocl_param(sizeof(cl_int), (void *) &block_dx );
			ocl_param(sizeof(cl_int), (void *) &block_dy );
			ocl_param(sizeof(cl_float)*4, (void *) &pos.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_leftTop.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_dx.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_dy.x );
			ocl_param(sizeof(cl_float)*4, (void *) &v0.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vx.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vy.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vz.x );
			ocl_param(sizeof(cl_float)*1, (void *) &fovx );
			ocl_param(sizeof(cl_float)*1, (void *) &fovy );
			ocl_end();
		}
		//if(FRAME%40==0)
		if(1)
		{
			static cl_kernel kernel = ocl_get_kernel("raycast_block");
			ocl_begin(&kernel,res_x/block_x,res_y/block_y,16,16);
			ocl_param(sizeof(cl_mem), &mem_depthbuffer);
			ocl_param(sizeof(cl_mem), &mem_backbuffer);
			ocl_param(sizeof(cl_mem), (void *) &mem_octree);
			ocl_param(sizeof(cl_mem), (void *) &mem_coarse);
			ocl_param(sizeof(cl_mem), (void *) &mem_coarse_col);
			//ocl_param(sizeof(cl_mem), (void *) &mem_x);
			ocl_param(sizeof(cl_mem), (void *) &mem_colorbuffer);			
			ocl_param(sizeof(cl_int), (void *) &octree::root_compact);
			ocl_param(sizeof(cl_int), (void *) &res_x );
			ocl_param(sizeof(cl_int), (void *) &res_y );
			ocl_param(sizeof(cl_int), (void *) &FRAME );
			ocl_param(sizeof(cl_int), (void *) &add_x );
			ocl_param(sizeof(cl_int), (void *) &add_y );
			ocl_param(sizeof(cl_int), (void *) &block_dx );
			ocl_param(sizeof(cl_int), (void *) &block_dy );
			ocl_param(sizeof(cl_float)*4, (void *) &pos.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_leftTop.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_dx.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_dy.x );
			ocl_param(sizeof(cl_float)*4, (void *) &v0.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vx.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vy.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vz.x );
			ocl_param(sizeof(cl_float)*1, (void *) &fovx );
			ocl_param(sizeof(cl_float)*1, (void *) &fovy );
			ocl_end();
		}
	}

	// cached rendering start
	if(block_x*block_y>1)
	{
		ocl_memset(mem_depthbuffer,0,0xffffff00,res_x*res_y*4);
	
		m.transpose();

		/*pro*/
		{
			loopi(0,2)
			// todo more buffers
			{
				int buffer_src_ofs = (i+1)*res_x*res_y;
				static cl_kernel kernel_proj = ocl_get_kernel("raycast_splatting");
				ocl_begin(&kernel_proj,res_x,res_y,16,16);
				ocl_param(sizeof(cl_mem), (void *) &mem_depthbuffer);
				ocl_param(sizeof(cl_mem), (void *) &mem_backbuffer);
				ocl_param(sizeof(cl_mem), (void *) &mem_colorbuffer);
				ocl_param(sizeof(cl_mem), (void *) &mem_screen);		
				ocl_param(sizeof(cl_int), (void *) &res_x );
				ocl_param(sizeof(cl_int), (void *) &res_y );
				ocl_param(sizeof(cl_int), (void *) &FRAME );
				ocl_param(sizeof(cl_int), (void *) &buffer_src_ofs ); // buffer
				ocl_param(sizeof(cl_float)*4, (void *) &v0.x );
				ocl_param(sizeof(cl_float)*4, (void *) &vx.x );
				ocl_param(sizeof(cl_float)*4, (void *) &vy.x );
				ocl_param(sizeof(cl_float)*4, (void *) &vz.x );
				ocl_param(sizeof(cl_float)*1, (void *) &fovx );
				ocl_param(sizeof(cl_float)*1, (void *) &fovy );
				ocl_end();
			}
		}

		m.transpose();//.invert_simpler();

		/*fillhole*/
	//	if(frame==0)printf("%d:fillhole\n",t_cnt);
	//	t[t_cnt++]=timeGetTime();

		// slow !!! fix
		
		//if(cfg::remove_outlier)
		//if(camera_moved)
		if(0)
		if((FRAME&3)==0)
		{		
			static cl_mem mem_tmp=ocl_malloc(4*WINDOW_WIDTH_MAX*WINDOW_HEIGHT_MAX); 

			static cl_kernel kernel = ocl_get_kernel("raycast_remove_outlier");
			ocl_begin(&kernel,res_x,res_y,16,16);
			ocl_param(sizeof(cl_mem), &mem_depthbuffer);
			ocl_param(sizeof(cl_int), &res_x);
			ocl_param(sizeof(cl_int), &res_y);
			ocl_param(sizeof(cl_int), &FRAME);
			ocl_end();		

			//swap
			{
				ocl_memcpy(
					mem_depthbuffer,0,
					mem_tmp,0,
					res_x*res_y*4);
			}
		}
		
	
		// -- count holes -- //
	//	if(frame==0)printf("%d:count holes\n",t_cnt);
	//	t[t_cnt++]=timeGetTime();
	
		int idbuf_size=0;
		static cl_mem mem_idbuffer=ocl_malloc(
			( WINDOW_WIDTH_MAX*WINDOW_HEIGHT_MAX+
			 (WINDOW_WIDTH_MAX/16)*(WINDOW_HEIGHT_MAX/16))*4);

		{
			static cl_kernel kernel_counthole = ocl_get_kernel("raycast_counthole");
			ocl_begin(&kernel_counthole,res_x/16,res_y/16,16,16);
			ocl_param(sizeof(cl_mem), &mem_depthbuffer);
			ocl_param(sizeof(cl_mem), &mem_backbuffer);
			ocl_param(sizeof(cl_mem), &mem_idbuffer);
			ocl_param(sizeof(cl_int), &res_x);
			ocl_param(sizeof(cl_int), &res_y);
			ocl_param(sizeof(cl_int), &FRAME);
			ocl_param(sizeof(cl_int), &hole_pixel_threshold);		
			ocl_end();
		}

		// -- sum ids -- //
	//	if(frame==0)printf("%d:sum ids\n",t_cnt);
	//	t[t_cnt++]=timeGetTime();
		{
			static cl_kernel kernel = ocl_get_kernel("raycast_sumids");
			ocl_begin(&kernel,1,1,1,1);
			ocl_param(sizeof(cl_mem), &mem_depthbuffer);
			ocl_param(sizeof(cl_mem), &mem_backbuffer);
			ocl_param(sizeof(cl_mem), &mem_idbuffer);
			ocl_param(sizeof(cl_int), &res_x);
			ocl_param(sizeof(cl_int), &res_y);
			ocl_param(sizeof(cl_int), &FRAME);
			ocl_end();
			ocl_copy_to_host(&idbuf_size,mem_idbuffer,4);
			//printf("\nnum_pixels:%d\n",idbuf_size);
		}	
		// -- write ids -- //
	//	if(frame==0)printf("%d:write ids\n",t_cnt);
	//	t[t_cnt++]=timeGetTime();
		{
			static cl_kernel kernel = ocl_get_kernel("raycast_writeids");
			ocl_begin(&kernel,res_x/16,res_y/16,16,16);
			ocl_param(sizeof(cl_mem), &mem_depthbuffer);
			ocl_param(sizeof(cl_mem), &mem_backbuffer);
			ocl_param(sizeof(cl_mem), &mem_idbuffer);
			ocl_param(sizeof(cl_int), &res_x);
			ocl_param(sizeof(cl_int), &res_y);
			ocl_param(sizeof(cl_int), &FRAME);
			ocl_param(sizeof(cl_int), &hole_pixel_threshold);		
			ocl_end();		
		}
	
		// -- fill empty pass -- //
	//	if(frame==0)printf("%d:fill empty pass\n",t_cnt);
	//	t[t_cnt++]=timeGetTime();
		//if((frame&7)==0)
		{
			vec4f vx=m*vec4f(1,0,0,0);
			vec4f vy=m*vec4f(0,1,0,0);
			vec4f vz=m*vec4f(0,0,1,0);

			vec3f a_leftTop,a_dx,a_dy;
			calcViewPlaneGradients(	res_x,res_y,  side,up,front,a_leftTop,a_dx,a_dy);	
		
			if(1)
			if(idbuf_size>0)
			{
				static cl_kernel kernel = ocl_get_kernel("raycast_holes");
				ocl_begin(&kernel,idbuf_size,1,256,1);
				ocl_param(sizeof(cl_mem), &mem_depthbuffer);
				ocl_param(sizeof(cl_mem), &mem_backbuffer);
				ocl_param(sizeof(cl_mem), (void *) &mem_octree);
				ocl_param(sizeof(cl_mem), (void *) &mem_bvh_nodes);
				ocl_param(sizeof(cl_mem), (void *) &mem_bvh_childs);
				ocl_param(sizeof(cl_mem), (void *) &mem_stack);
				ocl_param(sizeof(cl_mem), (void *) &mem_idbuffer);
				//ocl_param(sizeof(cl_mem), (void *) &mem_x);
				ocl_param(sizeof(cl_mem), (void *) &mem_colorbuffer);
				ocl_param(sizeof(cl_int), (void *) &octree::root_compact);
				ocl_param(sizeof(cl_int), (void *) &res_x );
				ocl_param(sizeof(cl_int), (void *) &res_y );
				ocl_param(sizeof(cl_int), (void *) &FRAME );
				ocl_param(sizeof(cl_int), (void *) &idbuf_size );
				ocl_param(sizeof(cl_float)*4, (void *) &pos.x );
				ocl_param(sizeof(cl_float)*4, (void *) &a_leftTop.x );
				ocl_param(sizeof(cl_float)*4, (void *) &a_dx.x );
				ocl_param(sizeof(cl_float)*4, (void *) &a_dy.x );
				ocl_param(sizeof(cl_float)*4, (void *) &v0.x );
				ocl_param(sizeof(cl_float)*4, (void *) &vx.x );
				ocl_param(sizeof(cl_float)*4, (void *) &vy.x );
				ocl_param(sizeof(cl_float)*4, (void *) &vz.x );
				ocl_param(sizeof(cl_float)*1, (void *) &fovx );
				ocl_param(sizeof(cl_float)*1, (void *) &fovy );
				ocl_end();
			}
		}
		
		/*copy*/
	//	if(frame==0)printf("%d:copy\n",t_cnt);
	//	t[t_cnt++]=timeGetTime();
	//	if((frame&1)==0)
		{
			int target=((FRAME>>3)%2)+1;
			ocl_memcpy(
				mem_depthbuffer,size_col*target,
				mem_depthbuffer,size_col*0,
				size_col);
		
			ocl_memcpy(
				mem_backbuffer,size_xyz*target,
				mem_backbuffer,size_xyz*0,
				size_xyz);

			ocl_memcpy(
				mem_colorbuffer,2*size_col*target,
				mem_colorbuffer,2*size_col*0,
				size_col*2);
		}

	}
		//if(frame==0)printf("%d:fillhole 2\n",t_cnt);
		//t[t_cnt++]=timeGetTime();

	// -- color -- //

	//if(frame==0)printf("%d:color\n",t_cnt);
	//t[t_cnt++]=timeGetTime();
	//if(0)
	{
		//m.transpose();
		//vec4f vx=m*vec4f(0,0,1,0);

			vec4f vx=m*vec4f(1,0,0,0);
			vec4f vy=m*vec4f(0,1,0,0);
			vec4f vz=m*vec4f(0,0,1,0);

		//m.transpose();


		static cl_kernel kernel = ocl_get_kernel("raycast_colorize");
		ocl_begin(&kernel,res_x,res_y,16,16);		
		ocl_param(sizeof(cl_mem), &mem_colorbuffer);
		ocl_param(sizeof(cl_mem), &mem_depthbuffer);
		ocl_param(sizeof(cl_mem), &mem_backbuffer);
		ocl_param(sizeof(cl_mem), &mem_screen);
		//ocl_param(sizeof(cl_mem), &mem_x);
		//(sizeof(cl_mem), &mem_palette);
		//ocl_param(sizeof(cl_float)*4, (void *) &vx.x );

		ocl_param(sizeof(cl_float)*4, (void *) &sunlightvec_objspc );
		ocl_param(sizeof(cl_float)*4, (void *) &v0.x );
		ocl_param(sizeof(cl_float)*4, (void *) &vx.x );
		ocl_param(sizeof(cl_float)*4, (void *) &vy.x );
		ocl_param(sizeof(cl_float)*4, (void *) &vz.x );
		ocl_param(sizeof(cl_float)*4, (void *) &color_effects );
		ocl_param(sizeof(cl_int), &res_x);
		ocl_param(sizeof(cl_int), &res_y);
		ocl_end();	
	}
	//if(0)
	//if(cfg::fillhole_screen)
	if(block_x*block_y>1)
	loopi(0,2)
	{		
		//uint resx_max=WINDOW_WIDTH_MAX;
		///uint resy_max=WINDOW_HEIGHT_MAX;
		static cl_kernel kernel = ocl_get_kernel("raycast_fillhole_screen");
		ocl_begin(&kernel,res_x,res_y,16,16);
		ocl_param(sizeof(cl_mem), &mem_screen);
		//ocl_param(sizeof(cl_mem), &mem_colorbuffer);
		ocl_param(sizeof(cl_int), &res_x);
		ocl_param(sizeof(cl_int), &res_y);
		ocl_param(sizeof(cl_int), &FRAME);
		ocl_end();		
	}

	ocl_end_all_kernels();	
	ocl_pbo_end(mem_screen);
	ocl_pbo_end(mem_depthbuffer);

	ogl_check_error();
	
	// -- screen pass -- //

	
	

//	if(frame==0)printf("%d:download tex 2\n",t_cnt);
//	t[t_cnt++]=timeGetTime();

    // download texture from PBO
	glEnable(GL_TEXTURE_2D);
	ogl_check_error();

	static int tex_screen=ogl_tex_new(WINDOW_WIDTH_MAX,WINDOW_HEIGHT_MAX,GL_NEAREST);
	ogl_bind(0,tex_screen);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_screen);	
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
                    res_x, res_y,                     //WINDOW_WIDTH_MAX,WINDOW_HEIGHT_MAX,
                    GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	ogl_check_error();

	static int tex_z     =ogl_tex_new(WINDOW_WIDTH_MAX,WINDOW_HEIGHT_MAX,GL_NEAREST);
	ogl_bind(0,tex_z);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_depthbuffer);
	ogl_check_error();	
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
                    res_x, res_y,                     //WINDOW_WIDTH_MAX,WINDOW_HEIGHT_MAX,
                    GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	ogl_check_error();

	static FBO fbo2(WINDOW_WIDTH_MAX,WINDOW_HEIGHT_MAX);
	fbo2.enable(res_x, res_y);

	// Pass - Merge
	ogl_bind(0,tex_screen);
	ogl_bind(1,tex_z);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS );
	//glClearDepth(0.0);
	glDepthMask (GL_TRUE);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	ogl_bind(2,fbo.color_tex);
	ogl_bind(3,fbo.depth_tex);
	static Shader shader_merge("../shader/raycast/merge_background");
	shader_merge.begin();
	shader_merge.setUniform1i("texVoxel",0);
	shader_merge.setUniform1i("texVoxelZ",1);
	shader_merge.setUniform1i("texBg",2);	
	shader_merge.setUniform1i("texBgZ",3);
	glDisable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	float tx=float(res_x)/float(WINDOW_WIDTH_MAX);
	float ty=float(res_y)/float(WINDOW_HEIGHT_MAX);
	glColor3f(1,1,1);
	ogl_drawquad(-1,-1,1,1,0,0,tx,ty);
	shader_merge.end();
	loopi(0,4) 	ogl_bind(3-i,0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	ogl_check_error();
	fbo2.disable();

	glDisable(GL_DEPTH_TEST);

	// Pass - FXAA
	fbo.enable(res_x, res_y);
	glMatrixMode(GL_PROJECTION);glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);	glLoadIdentity();
	//static Shader shader_fxaa("../shader/raycast/fxaa");
	static Shader shader_ssao("../shader/raycast/ssao");
	
	//shader_ssao.begin();
	//shader_ssao.setUniform1i("cTex",0);
	//shader_ssao.setUniform1i("zTex",1);
	
	/*if(fxaa)
	{
		shader_fxaa.begin();
		shader_fxaa.setUniform1i("iTex",0);
		shader_fxaa.setUniform3f("iResolution",WINDOW_WIDTH_MAX,WINDOW_HEIGHT_MAX,0);
	}*/
	glColor4f(1,1,1,1);
	//ogl_bind(1,fbo2.depth_tex);
	ogl_bind(0,fbo2.color_tex);
	ogl_drawquad(-1,-1,1,1,0,0,tx,ty);
	ogl_bind(0,0);
	ogl_bind(1,0);
	//shader_ssao.end();
	//if(fxaa) shader_fxaa.end();
	fbo.disable();

	// Pass - Display
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 	
	glColor4f(1,1,1,clamp(1.0-0.5*fps/(100.0),0.5,1.0));
	ogl_bind(0,fbo.color_tex);
	ogl_drawquad(-1,-1,1,1,0,0,tx,ty);
	glColor4f(1,1,1,1);
	glDisable(GL_BLEND);
	ogl_bind(0,0);

	
	return;
}


} // namespace

