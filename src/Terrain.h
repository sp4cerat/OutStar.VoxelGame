///////////////////////////////////////////
// namespace begin
///////////////////////////////////////////
namespace terrain{
///////////////////////////////////////////
bool wireframe=0;
///////////////////////////////////////////
int grid=64 ;				// patch resolution
int levels=10  ;			// LOD levels
int width=4096,height=4096; // heightmap dimensions
float snowfall=0.5;//snow//0.5;normal//+10;
float  heightmap_avg=0.5;
double heightmap_0=0;
///////////////////////////////////////////
int    map_res=8192;
double map_scale=map_res*grid;
float *heightmap=0;
Bmp	   bmp_normal(width,height,32);
///////////////////////////////////////////
int tex_heightmap=0;
int tex_terrain=0;
int tex_rock_sand_grass=0;
int vbo=0;
std::vector<float> vert;
///////////////////////////////////////////
double get_height(double viewpos_x,double viewpos_z)
{
	if(!heightmap) return 0;

	viewpos_x*=1;
	viewpos_z*=1;
	//viewpos_x=-viewpos_x;
	//viewpos_z=-viewpos_z;

	double map_pos_x=-double(int(2*viewpos_x) % int(map_scale))/map_scale;
	double map_pos_y=-double(int(2*viewpos_z) % int(map_scale))/map_scale;

	float px=(map_pos_x+2)*width;
	float py=(map_pos_y+2)*height;
	int x=int(px)%width;
	int y=int(py)%height;

	float h44[4][4];
	float w44[4][4];
	loopi(-2,2)
	loopj(-2,2)
	{
		int ox = (x+i+width)%width;
		int oy = (y+j+height)%height;
		h44[i+2][j+2]=heightmap[ox+oy*width];
		w44[i+2][j+2]=float(bmp_normal.data[(ox+oy*width)*4+3])/255.0; // rock intensity
	}
	float terrain_height=bicubicInterpolate ( h44, frac(px), frac(py));
	float bump_weight=bicubicInterpolate ( w44, frac(px), frac(py));
			
	// details
	int x64=int(px*64)%width;
	int y64=int(py*64)%height;
	terrain_height+=(heightmap[x64+y64*width]-heightmap_avg)*0.01*bump_weight;//*w_dist;
			
	return ((terrain_height)*0.05*map_scale-heightmap_0);
}
///////////////////////////////////////////
void init()
{
	static Bmp bmp(width,height,32);
	//static Bmp tmp(width,height,32);
	if(!bmp.load_float("../data/terrain/result.f32")) error_stop("Bmp::load_float"); 

	heightmap=(float*)&bmp.data[0];
	//loopi(0,width*height)heightmap[i]*=0.6;
	//loopi(0,width*height)heightmap[i]*=1.2;

	Bmp bmp_grass("../data/terrain/grass.bmp");
	Bmp bmp_rock ("../data/terrain/rock.bmp");
	Bmp bmp_sand ("../data/terrain/sand.bmp");
	Bmp bmp_road ("../data/terrain/road.png");
	Bmp bmp_path ("../data/terrain/path.png");

	/*+++++++++++++++++++++++++++++++++++++*/
	// terrain texture

	Bmp bmp_merged(1024,1024,32);

	loopi(0,bmp_grass.width*bmp_grass.height)
	{
		bmp_merged.data[i*4+0]=bmp_rock .data[i*3+1];
		bmp_merged.data[i*4+1]=bmp_sand .data[i*3+2];
		bmp_merged.data[i*4+2]=bmp_grass.data[i*3+1];
		bmp_merged.data[i*4+3]=bmp_road .data[i*3+0];//0;//bmp_elev .data[i*3+0];			
	}
	//tex_rock_sand_grass = ogl_tex_new(bmp_merged.width,bmp_merged.height,GL_LINEAR_MIPMAP_LINEAR,GL_REPEAT,GL_RGBA,GL_RGBA,(uchar*)&bmp_merged.data[0], GL_UNSIGNED_BYTE);
	tex_rock_sand_grass = ogl_tex_new(bmp_merged.width,bmp_merged.height,GL_LINEAR_MIPMAP_NEAREST,GL_REPEAT,GL_RGBA,GL_RGBA,(uchar*)&bmp_merged.data[0], GL_UNSIGNED_BYTE);

	loopj(0,height)	loopi(0,width)
	{
		int ii=i;
		int jj=j;
		float h =heightmap[ii+jj*width];
		float hx=heightmap[(ii+1)%width+jj*width];
		float hy=heightmap[ii+((jj+1)%height)*width];
		vec3f d1(1,0,(hx-h)*1000);
		vec3f d2(0,1,(hy-h)*1000);
		bmp_normal.data[(i+j*width)*4+0]=clamp(d1.z*32.0+128.0,0,255);
		bmp_normal.data[(i+j*width)*4+1]=clamp(d2.z*32.0+128.0,0,255);
		//bmp_normal.data[(i+j*width)*4+2]=255.0*h;
		bmp_normal.data[(i+j*width)*4+2]=bmp_path.data[(i+j*width)*3+0];

		vec3f n;n.cross(d1,d2);n.norm();
		float w1=clamp((n.z*0.5-h)*2.0,0.0,1.0);				// grass-sand
		float w2=clamp((n.z*0.5-h)*8.0,0.0,1.0);				// sand-rock
//		float w3=clamp((n.z*0.7-0.14-clamp(1.0-h*1.5,0,1.0))*16.0,0.0,1.0);		// rock-snow
//			float w3=clamp((n.z*1.0-0.40-clamp(1.0-(h+0.05)*snowfall,0,1.0))*16.0,0.0,1.0);			// rock-snow
		float w3=clamp((n.z-0.5-clamp((snowfall-h)*3.0,0,0.5))*32.0,0.0,1.0);		// rock-snow snowy
		float c1=1;		// rock
		float c2=0;		// 
		float c3=0;		// 
		float c4=0;		// 	
		float col=c1;
		c2 =lerp(c2,c3 ,w1);	
		col=lerp(col,c2,w2);	
		col=lerp(col,c4,w3);	
		bmp_normal.data[(i+j*width)*4+3]=col*255.0f;
	}
	tex_terrain = ogl_tex_new(width,height,GL_LINEAR_MIPMAP_LINEAR,GL_REPEAT,GL_RGBA,GL_RGBA,(uchar*)&bmp_normal.data[0], GL_UNSIGNED_BYTE);
	/*+++++++++++++++++++++++++++++++++++++*/
	// terrain heightmap		
	heightmap_avg=0;
	float heightmap_min=1;
	float heightmap_max=0;
	loopi(0,width*height){float h=heightmap[i];heightmap_min=min(heightmap_min,h);heightmap_max=max(heightmap_max,h);}
	heightmap_avg=(heightmap_min+heightmap_max)/2;		
	tex_heightmap = ogl_tex_new(width,height,GL_LINEAR_MIPMAP_LINEAR,GL_REPEAT,GL_LUMINANCE16F_ARB,GL_LUMINANCE,(uchar*)&bmp.data[0], GL_FLOAT);
	/*+++++++++++++++++++++++++++++++++++++*/
	// make vbo quad patch
	loopj(0,grid+2)
	loopi(0,grid+2)
	{
		++j;
		loopk(0, ((i==0) ? 2 : 1) )
		{
			vert.push_back(float(i)/grid);
			vert.push_back(float(j)/grid);
			vert.push_back(0 );
		}			
		--j;
		loopk(0, ((i==grid+1) ? 2 : 1) )
		{
			vert.push_back(float(i)/grid);
			vert.push_back(float(j)/grid);
			vert.push_back(0 );
		}
	}
	/*+++++++++++++++++++++++++++++++++++++*/
	glGenBuffers(1, (GLuint *)(&vbo));
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vert.size(),&vert[0], GL_DYNAMIC_DRAW_ARB );
	/*+++++++++++++++++++++++++++++++++++++*/
	heightmap_0 = get_height(0,0);
}
///////////////////////////////////////////
void draw	(	double viewpos_x ,
				double viewpos_y ,
				double viewpos_z )
{
	viewpos_y-=heightmap_0;
	//viewpos_x=-viewpos_x;

	glPushMatrix();
	glRotatef(90,1,0,0);

	static Shader shader("../shader/terrain");

	//if(heightmap==0) init();

	double map_pos_x=-double(int(viewpos_x*2) % int(map_scale))/map_scale;
	double map_pos_y=-double(int(viewpos_z*2) % int(map_scale))/map_scale;

	// Enable VBO
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);				ogl_check_error();
	glEnableClientState(GL_VERTEX_ARRAY);					ogl_check_error();
	glVertexPointer  ( 3, GL_FLOAT,0, (char *) 0);			ogl_check_error();

	glTranslatef(0,0,-viewpos_y);	// set height
	glScaled(terrain::map_scale,terrain::map_scale,terrain::map_scale);

	ogl_bind(0,tex_heightmap);
	ogl_bind(1,tex_terrain);
	ogl_bind(2,tex_rock_sand_grass);

	// Triangle Mesh
	shader.begin();
	shader.setUniform1i("tex_heightmap",0);
	shader.setUniform1i("tex_normalmap",1);
	shader.setUniform1i("tex_rock_sand_grass",2);

	float sxy=2; // scale x/y
	shader.setUniform4f("map_position", map_pos_x,map_pos_y,0,0);
	shader.setUniform4f("sunlightvec", sunlightvec.x, sunlightvec.y, sunlightvec.z, sunlightvec.w);
	
	shader.setUniform1f("snowfall", snowfall);
	shader.setUniform1f("heightmap_avg", heightmap_avg);

	matrix44 mat,mat_mv,mat_pr;
	glGetFloatv(GL_PROJECTION_MATRIX, &mat_pr.m[0][0]);		ogl_check_error();
	glGetFloatv(GL_MODELVIEW_MATRIX,  &mat_mv.m[0][0]);		ogl_check_error();
	mat=mat_mv*mat_pr;
	
	loopj(0,levels)
	{
		int i=levels-1-j;
		float sxy=2.0/float(1<<i);
		float ox=(int(int(viewpos_x*2)*(1<<i))%map_res)/map_scale;
		float oy=(int(int(viewpos_z*2)*(1<<i))%map_res)/map_scale;

		vec3f scale	(sxy*0.25,sxy*0.25,1);
		shader.setUniform4f("scale" , scale.x,scale.y,1,1);	

		loopk(-2,2) loopj(-2,2) // each level has 4x4 patches
		{
			if(i!=levels-1) if(k==-1||k==0) if(j==-1||j==0) continue;

			vec3f offset(ox+float(j),oy+float(k),0);
			if(k>=0) offset.y-=1.0/float(grid); // adjust offset for proper overlapping
			if(j>=0) offset.x-=1.0/float(grid); // adjust offset for proper overlapping

			//cull
			int xp=0,xm=0,yp=0,ym=0,zp=0;
			looplmn(0,0,0,2,2,2)
			{
				vec3f v = scale*(offset+vec3f(l,m,float(-n)*0.05)); // bbox vector
				vec4f cs = mat * vec4f(v.x,v.y,v.z,1); // clipspace
				if(cs.z< cs.w) zp++;				
				if(cs.x<-cs.w) xm++;	if(cs.x>cs.w) xp++;
				if(cs.y<-cs.w) ym++;	if(cs.y>cs.w) yp++;
			}
			if(zp==0 || xm==8 || xp==8 || ym==8 || yp==8)continue; // skip if invisible
			
			//render
			shader.setUniform4f("offset", offset.x,offset.y,0,0);
			if(wireframe)	glDrawArrays( GL_LINES, 0, vert.size()/3);
			else			glDrawArrays( GL_TRIANGLE_STRIP, 0, vert.size()/3);
		}
	}	
	shader.end();

	loopi(0,3) ogl_bind(2-i,0);

	// Disable VBO
	glDisableClientState(GL_VERTEX_ARRAY);		ogl_check_error();
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);	ogl_check_error();

	glPopMatrix();
}
///////////////////////////////////////////
}; // namescape end
///////////////////////////////////////////
