namespace voxeledit // namespace begin
{
vec3f view_rot=vec3f(0,0,0);
vec3f view_pos=vec3f(0,0,-1);

int lvl0=16;
int lvl1x=8;
int lvl1y=8;
int lvl1z=8;

int trianglecount=0;

// Menu in

int	  select_wheel_function=0;

bool  cursor_rgb_cross=1;

bool  brush_loop_x=0;
bool  brush_loop_y=0;
bool  brush_loop_z=0;
bool  brush_mirror_x=0;
bool  brush_mirror_y=0;
bool  brush_mirror_z=0;

int   brush_tool=0; 
float brush_size=5;
float brush_intensity=250;
float brush_stroke_length=100;
int   brush_type=0;
//int	  brush_color=15;
float brush_color_angle=0;
int	  brush_color_rgb=255;
float brush_color_specular=0;
bool  brush_color_audio=0;
float brush_color_random=0;
float brush_intensity_random=0;
float brush_rate=100;
float brush_color_glow=0;
int   brush_color_effect=0;
int	  brush_color_texture=0;
float brush_color_reflect=0;

bool  brush_z_const_disabled=0;
float brush_z=0.989038;
vec3f brush_pos;

bool  brush_const_x=0;
bool  brush_const_y=0;
bool  brush_const_z=0;
vec3f brush_const_pos(0,0,brush_z);

int	  selected_tool=0;

bool  helper_shape_enabled=0;
bool  helper_shape_rasterize=0;
int   helper_shape_type=0;
vec3f helper_shape_pos(0,0,70);
vec3f helper_shape_size(255,255,255);
bool  helper_shape_fixed=1;

std::vector<vec3f> poly;
bool  poly_edit=0;
float poly_edit_handle=20;

bool  render_ssao=0;	
bool  render_cubes=0;	
bool  scene_reset=true;

int   menu_border=220;

bool help_on=1;
char help_mb1[1000]={"Left Mouse Button : Draw / Add Polygon Points Clockwise"};
char help_mb2[1000]={"Middle Mouse Button : Pan"};
char help_mb3[1000]={"Right Mouse Button : Rotate / Cancel Polygon"};
char help_mw [1000]={"Mouse Wheel : Brush Z / Brush Size / Zoom"};

void rasterize_poly(std::vector<vec3f> &poly);
void triangulate();
float get_shade(int x,int y,int z);

#include "voxeledit_mc.h"
#include "voxeledit_terrain.h"


void reset()
{	
	if(blocks.size()!=lvl1x*lvl1y*lvl1z)
	{
		blocks.resize(lvl1x*lvl1y*lvl1z);
	}

	loopi(0,blocks_vbo.size())
	{
		if(blocks_vbo[i].vbo_tri<0)continue;
		glDeleteBuffers (1,&blocks_vbo[i].vbo_tri);
	}
	blocks_vbo.clear();

	//view_rot=vec3f(0,0,0);
	//view_pos=vec3f(0,0,-1);
	scene_reset=0;
	brush_mirror_x=
	brush_mirror_y=
	brush_mirror_z=0;
	brush_z=0.989038;

	loopi(0,lvl1x*lvl1y*lvl1z) 
		blocks[i].clear();

	triangulate();

	blocks_undo=blocks;
}

void draw_voxel_cube(std::vector<Block> &blocks,bool draw_grid_etc=1);

void add_cube_to_octree()
{
	// create thumbnail
	//int texture_id=0;		
	Bmp bmp;
	{
		glColor4f(1,1,1,1);
		glDisable(GL_BLEND);
		//glDisable(GL_DEPTH_TEST);
		glDisable(GL_TEXTURE_2D);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_CULL_FACE);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(90.0, (GLfloat)WINDOW_WIDTH/(GLfloat)WINDOW_HEIGHT, 0.01, 400.0);
		//float fov=(GLfloat)WINDOW_WIDTH/(GLfloat)WINDOW_HEIGHT;
		//glOrtho(-fov,fov, -1,1, 0.01, 400.0);//default = -1.0,+1.0,-1.0,+1.0,-1.0,+1.0
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		//glClearColor(1,1,1,1);
	    glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
		draw_voxel_cube(blocks,0);
		int tmp[4],width,height;
		glGetIntegerv(GL_VIEWPORT, tmp);
		width=tmp[2];height=tmp[3];
		printf("wh %d %d\n",width,height);
		bmp.set(width,height,32,0);
		glReadBuffer(GL_BACK);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,&bmp.data[0]);
		glFlush();
		bmp.crop(height,height,(width-height)/2,0);
		bmp.scale(128,128);
		loopi(0,128)
		loopj(0,128)
		if(i<5||i>128-5||j<5||j>128-5)
		{
			bmp.data[(i+j*128)*4+0]=
			bmp.data[(i+j*128)*4+1]=
			bmp.data[(i+j*128)*4+2]=128;
			bmp.data[(i+j*128)*4+3]=0;
		}
		
		loopi(0,128*128) bmp.data[i*4+3] = (bmp.data[i*4+3]==255) ?  64 : 255;
		bmp.flip();
		//bmp.save("../screen.png");
		//texture_id=ogl_tex_bmp(bmp);
	}


	//octree::clear();
	int sx=lvl1x*lvl0;
	int sy=lvl1y*lvl0;
	int sz=lvl1z*lvl0;

	uint startmem=octree::octree_array_compact.size()*4;

	octreeblock::begin_oblock(sx,sy,sz,bmp);
	
	loopijk(0,0,0,   sx,sy,sz)
	{
		if(get_voxel(i,j,k).intensity<0x8000) continue;

		int c=0;
		looplmn(i-1,j-1,k-1,   i+2,j+2,k+2)
			if(get_voxel(l,m,n).intensity>=0x8000) c++;
		/*
		loopl(-1,2)
		{
			if(get_voxel(i+l,j,k).intensity>=0x8000) c++;
			if(get_voxel(i,j+l,k).intensity>=0x8000) c++;
			if(get_voxel(i,j,k+l).intensity>=0x8000) c++;
		}*/

		if(c==3*3*3) continue;

		//normal
		vec3f nrm(0,0,0);
		{
			int i0=i,j0=j,k0=k;
			loopijk(-1,-1,-1,2,2,2)
			{
				int a=get_voxel(i0+i,j0+j,k0+k).intensity>>4;
				nrm.x+=i*int(a);
				nrm.y+=j*int(a);
				nrm.z+=k*int(a);
			}
			nrm.norm();
		}

		// write
		{
			uint color = get_voxel(i,j,k).color;

			if(0)
			{
				int brightness=255.0f*(-nrm.y*0.5+0.5);
				int r= color&255;
				int g=(color>>8)&255;
				int b=(color>>16)&255;
				r=(r*brightness)>>8;
				g=(g*brightness)>>8;
				b=(b*brightness)>>8;
				color=r+g*256+b*256*256;
			}

			uint normal=nrm.encode_normal_uint();
			octree::set_voxel(i,j,k,color,normal);
		}
	}

	octreeblock::end_oblock();

	printf("block size: %d Bytes\n",(octree::octree_array_compact.size()*4-startmem));

	raycast::update_gpu_octree();

}

void smooth()
{
	int sx=lvl1x*lvl0/1;float fx=sx;
	int sy=lvl1y*lvl0/1;float fy=sy;
	int sz=lvl1z*lvl0/1;float fz=sz;
	VoxelData vd;

	loopijk(0,0,0,sx,sy,sz)
	{
		int c=0;
		looplmn(0,0,0,2,2,2)
		{
			c+=uint(get_voxel(i+l,j+m,k+n).intensity);
		}
		c>>=3;

		vd=get_voxel(i,j,k);
		vd.color=c;
		set_voxel(i,j,k,vd);
	}
	loopijk(0,0,0,sx/1,sy/1,sz/1)
	{
		vd=get_voxel(i,j,k);
		vd.intensity=vd.color;
		vd.color=255;
		set_voxel(i,j,k,vd);
	}
}


#include "voxeledit_file.h"
#include "voxeledit_ui.h"

// Menu out

void triangulate()
{
	if(blocks.size()==0)return;
	int a=0;
	for(int z1=0;z1<lvl1z;z1++)
	for(int y1=0;y1<lvl1y;y1++)
	for(int x1=0;x1<lvl1x;x1++)
	{
		if(a>=blocks.size())error_stop("a %d > %d size",a,blocks.size());
		//if(a>=blocks.size())error_stop("triangulate: a (%d) >= xyz %d",a,lvl1x*lvl1y*lvl1z);
		blocks[a++].triangulate(x1,y1,z1);
	}
	//printf("TRI RAM %f MB\n",float(tri.size()*4+tri_col.size()*4)/1000000);
}

void rasterize_triangle(vec3f p1,vec3f p2,vec3f p3,bool shade=false)
{
	vec3f d1=p2-p1;
	vec3f d2=p3-p1;

	float accuracy=4; 

	int c  = d1.len()*accuracy;
	int c2 = d2.len()*accuracy;
	if (c2>c ) c=c2;

	if(c==0)c=1;
	if(c>1000)c=1000;


	vec3f p;
	VoxelData vd;
	
	for(int i=0;i<=c;i++)
	{
		float a=float(i)/float(c);

		vec3f x1=p1+d1*a;
		vec3f x2=p1+d2*a;
		vec3f dx=x2-x1;

		int d = dx.len()*accuracy;

		if(d==0)d=1;
		if(d>1000)d=1000;
			
		for(int j=0;j<=d;j++)
		{
			float b=float(j)/float(d);
			p=x1+dx*b;

			if(shade)
				set_shade( p.x,p.y,p.z);
			else
			{
				vd.intensity=255*256;
				vd.color=brush_color_rgb;
				set_voxel( p.x,p.y,p.z, vd);
			}
		}
	}
}
void rasterize_poly(std::vector<vec3f> &poly)
{
	blocks_undo=blocks;	

	loopi(1,poly.size()-1)
	{
		rasterize_triangle(poly[0],poly[i],poly[i+1]);
		rasterize_triangle(poly[0],poly[i],poly[i+1],1);
	}
}
bool inside_box(vec3f p)
{
	if(p.x>=0)if(p.x<lvl0*lvl1x)
	if(p.y>=0)if(p.y<lvl0*lvl1y)
	if(p.z>=0)if(p.z<lvl0*lvl1z) return true;
	return false;
}
float get_z(float z_in)
{
	float z=ogl_read_z(MOUSE_X,MOUSE_Y);
	vec3f p=ogl_unproject(MOUSE_X,MOUSE_Y,z);
	if(inside_box(p)) return z; else return z_in;
}
float get_z_xy(float z_in,float mx,float my)
{
	float z=ogl_read_z(mx,my);
	vec3f p=ogl_unproject(mx,my,z);
	if(inside_box(p)) return z; else return z_in;
}

void draw_poly()
{
	glColor3f(1,1,1);
	glBegin(GL_LINES);
	loopi(1,poly.size())
	{
		glVertex3f(poly[i].x,poly[i].y,poly[i].z);
		glVertex3f(poly[i-1].x,poly[i-1].y,poly[i-1].z);
		glVertex3f(poly[0].x,poly[0].y,poly[0].z);
		glVertex3f(poly[i].x,poly[i].y,poly[i].z);
	}
	glEnd();
	loopi(0,poly.size())
	{
		glPushMatrix();
		glTranslatef(poly[i].x,poly[i].y,poly[i].z);
		glutSolidSphere(2,6,6);
		glPopMatrix();
	}
	if(poly_edit)
	{
		float val=poly_edit_handle;
		//glLineWidth(3);
		glBegin(GL_LINES);
		loopi(0,poly.size())
		{
			vec3f p=poly[i];
			glColor3f(1,0,0);
			glVertex3f(p.x,p.y,p.z);
			glVertex3f(p.x+val,p.y,p.z);
			glColor3f(0,1,0);
			glVertex3f(p.x,p.y,p.z);
			glVertex3f(p.x,p.y+val,p.z);
			glColor3f(0,0,1);
			glVertex3f(p.x,p.y,p.z);
			glVertex3f(p.x,p.y,p.z+val);
		}
		glEnd();
		loopi(0,poly.size())
		{
			glPushMatrix();
			glTranslatef(poly[i].x,poly[i].y,poly[i].z);
			glColor3f(1,0,0);
			glTranslatef(val,0,0);
			glutSolidSphere(2,8 ,4);
			glColor3f(0,1,0);
			glTranslatef(-val,val,0);
			glutSolidSphere(2,8 ,4);
			glColor3f(0,0,1);
			glTranslatef(0,-val,val);
			glutSolidSphere(2,8 ,4);
			glPopMatrix();
		}
	}
	glEnable(GL_BLEND);
	glDepthMask(0);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);       // Blending Function For Translucency Based On Source Alpha 
	glColor4f(0.5,0.5,0.5,0.3);
	glDisable(GL_CULL_FACE);
	glBegin(GL_TRIANGLE_FAN);
	loopi(0,poly.size()) glVertex3f(poly[i].x,poly[i].y,poly[i].z);
	glEnd();
	glEnable(GL_CULL_FACE);
	glColor4f(1,1,1,1);
	glDisable(GL_BLEND);
	glDepthMask(1);
}
vec3f help_shape_get_vec(matrix44 &m,vec3f u)
{
	vec4f v;
	v.x=u.x;
	v.y=u.y;
	v.z=u.z;
	v.w=1;
	v=m*v;
	v.x+=0.5;
	v.y+=0.5;
	v.z+=0.5;
	v.x*=lvl0*lvl1x;
	v.y*=lvl0*lvl1y;
	v.z*=lvl0*lvl1z;

	return vec3f(v.x,v.y,v.z);
}
void help_shape_rasterize(matrix44 mv)
{
	if(!helper_shape_rasterize)return;
	helper_shape_rasterize=0;
	
	matrix44 m;
	m.scale(vec3f(
			helper_shape_size.x/255.0,
			helper_shape_size.y/255.0,
			helper_shape_size.z/255.0));
	m.translate( helper_shape_pos*(-1.0/100.0f) );

	m=m*mv;

	if(helper_shape_type==0)
	{
		std::vector<vec3f> p;
		loopi(0,4)
		{
			vec3f v;
			v.y=(i>>1);
			v.x=(i&1)^int(v.y);
			v.z=0;
			v.x-=0.5;
			v.y-=0.5;
			p.push_back(help_shape_get_vec(m,v));			//printf("p %f %f %f\n",v.x,v.y,v.z);
		}
		rasterize_poly(p);		//ogl_drawquad(-0.5,-0.5,0.5,0.5);
	}

	if(helper_shape_type==5)
	{
		// Cylinder
		std::vector<vec3f> p;

		for(float a=0;a<=2*M_PI+0.01;a+=2*M_PI/50)
		{
			float b=a+2*M_PI/50;
			vec3f p0(sin(a)*0.5,-0.5,cos(a)*0.5);
			vec3f p1(sin(b)*0.5,-0.5,cos(b)*0.5);
			vec3f p2(sin(b)*0.5, 0.5,cos(b)*0.5);
			vec3f p3(sin(a)*0.5, 0.5,cos(a)*0.5);
			p.push_back(help_shape_get_vec(m,p0));
			p.push_back(help_shape_get_vec(m,p1));
			p.push_back(help_shape_get_vec(m,p2));
			p.push_back(help_shape_get_vec(m,p3));
			rasterize_poly(p);
			p.clear();
		}

		for(float a=0;a<2*M_PI;a+=2*M_PI/50)
			p.push_back(help_shape_get_vec(m,
				vec3f(-sin(a)*0.5,-0.5,cos(a)*0.5)));
		
		rasterize_poly(p);
		p.clear();

		for(float a=0;a<2*M_PI;a+=2*M_PI/50)
			p.push_back(help_shape_get_vec(m,
				vec3f(-sin(a)*0.5, 0.5,cos(a)*0.5)));
		
		rasterize_poly(p);
		p.clear();
		
	}
}

void draw_help_shape()
{
	if(helper_shape_enabled)
	{
		glPushMatrix();
		glEnable(GL_BLEND);
		glColor4f(0.0f,0.0f,0.5f,0.5f);         // Full Brightness, 50% Alpha ( NEW )
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);       // Blending Function For Translucency Based On Source Alpha 
		
		glLoadIdentity();		
		glTranslatef( view_pos.x,view_pos.y,view_pos.z );
		glRotatef(view_rot.x,1,0,0);
		glRotatef(view_rot.y,0,1,0);
		
		static matrix44 m; static bool init=1;
		
		if(!helper_shape_fixed || scene_reset || init)
		{
			glGetFloatv(GL_MODELVIEW_MATRIX, &m.m[0][0]); 
			m.invert();
			init=0;
		}
		glMultMatrixf(&m.m[0][0]);
		
		glTranslatef( 
			helper_shape_pos.x*(-1.0/100.0f) ,
			helper_shape_pos.y*(-1.0/100.0f) ,
			helper_shape_pos.z*(-1.0/100.0f) 
			);

		glScalef(
			helper_shape_size.x/255.0,
			helper_shape_size.y/255.0,
			helper_shape_size.z/255.0);


		help_shape_rasterize(m);

		if(helper_shape_type==0)
		{
			glDisable(GL_CULL_FACE);	
			ogl_drawquad(-0.5,-0.5,0.5,0.5);
		}
		if(helper_shape_type==1)
			glutSolidSphere(0.5,20,10);

		if(helper_shape_type==2)
			glutSolidCube(0.5);

		if(helper_shape_type==3)
			glutSolidCone(0.5,0.5,20,10);

		if(helper_shape_type==4)
			glutSolidTorus(0.25,0.5,20,40);

		if(helper_shape_type==5)
		{
			// Cylinder
			glBegin(GL_TRIANGLE_STRIP);
			for(float a=0;a<=2*M_PI+0.01;a+=2*M_PI/20)
			{
				glVertex3f(sin(a)*0.5, 0.5,cos(a)*0.5);
				glVertex3f(sin(a)*0.5,-0.5,cos(a)*0.5);
			}
			glEnd();

			glBegin(GL_TRIANGLE_FAN);
			for(float a=0;a<2*M_PI;a+=2*M_PI/20)
				glVertex3f(-sin(a)*0.5,-0.5,cos(a)*0.5);
			glEnd();

			glBegin(GL_TRIANGLE_FAN);
			for(float a=0;a<=2*M_PI;a+=2*M_PI/20)
				glVertex3f( sin(a)*0.5, 0.5,cos(a)*0.5);
			glEnd();
		}

		glDisable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glPopMatrix();
	}
}

void draw_grid(int res)
{
	loopi(0,res+1)
	{
		float a=float(i)/float(res)-0.5;
		ogl_drawline(-0.5,-0.5,a,0.5,-0.5,a);
		ogl_drawline(a,-0.5,-0.5,a,-0.5,0.5);
	}
}

void draw_voxel_cube(std::vector<Block> &blocks,bool draw_grid_etc)
{
	glLoadIdentity();
	glTranslatef( view_pos.x,view_pos.y,view_pos.z );
	glRotatef(view_rot.x,1,0,0);
	glRotatef(view_rot.y,0,1,0);
	//glutWireCube(1);	

	float scale_xyz=1.0/float(lvl0*max(lvl1x,max(lvl1y,lvl1z)));
	
	glPushMatrix();
	//glTranslatef(-0.5,-0.5,-0.5);
	glScalef(scale_xyz,scale_xyz,scale_xyz);
	glTranslatef(-lvl1x*lvl0/2,-lvl1y*lvl0/2,-lvl1z*lvl0/2);

	trianglecount=0;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	int cache=0,drc=0;
	
	loopi(0,lvl1x*lvl1y*lvl1z)
	{
		int ret=blocks[i].draw(i);
		if ( ret==1 ) cache++;
		if ( ret<2 ) drc++;
	}
	//if ( FRAME % 10 == 0 )
	//printf("cache %d drawn %d\n",cache,drc);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glPopMatrix();

	if(draw_grid_etc==false) return;

	glDepthMask(0);
	
	// grid

	glPushMatrix();
	{
		float m=max(lvl1x,max(lvl1y,lvl1z));
		glTranslatef(0,0.5-0.5*float(lvl1y)/m,0);
		glColor3f(0.25,0.25,0.25);	draw_grid(16);
		glColor3f(0.5,0.5,0.5);		draw_grid(8);
		glColor3f(1,1,1);
	}	
	glPopMatrix();

	glPushMatrix();
	{
		float m=max(lvl1x,max(lvl1y,lvl1z));
		glScalef(float(lvl1x)/m,float(lvl1y)/m,float(lvl1z)/m);
		glutWireCube(1);
	}	
	glPopMatrix();

	glDepthMask(1);

	draw_help_shape();
	
	glDisable(GL_BLEND);

	//glTranslatef(-0.5,-0.5,-0.5);
	glScalef(scale_xyz,scale_xyz,scale_xyz);
	glTranslatef(-lvl1x*lvl0/2,-lvl1y*lvl0/2,-lvl1z*lvl0/2);
}

float getRnd ()
{
	static int seed = 453413;
	seed = (seed*751423^234423) - seed*seed*346 + seed*seed*seed*342521 - 93337524 + (seed/2346) - (seed^234621356) + ((seed*seed)>>16);
	return float(seed&65535)/32767.5f-1.0;
}

void voxel_editor()
{
    //Clear color buffer

	if(MOUSE_BUTTON_DOWN[0])
	{
		if(brush_tool==0) // brush 0 draw start
		{
			blocks_undo=blocks;			
		}
		if(brush_tool==1) // brush 1 poly start
		if(!poly_edit)
		{
			//blocks_undo=blocks;			
		}
		if(brush_tool==2) // brush 2 smooth start
		{
			blocks_undo=blocks;			
		}
	}

	std::vector<Block> &blocks_render=blocks_undo;	

	bool render_undo=1;
	if(brush_tool==0 && (brush_type==2 || brush_type==3)) render_undo=0;
	
	if(render_undo)
		draw_voxel_cube(blocks_undo);
	else
		draw_voxel_cube(blocks);

	if(brush_tool==2) // proce
	{
	}// 

	if(brush_tool==0) // brush 0 start basic
	{
		static int brush_x,brush_y,mouse_x,mouse_y;static float dist;

		// set voxel
		if(MOUSE_BUTTON_DOWN[0])
		{
			//printf("%f\n",brush_z);
			brush_z=get_z(brush_z);//ogl_read_z(MOUSE_X,MOUSE_Y);
			//printf("%f\n",brush_z);
			brush_const_pos=ogl_unproject(MOUSE_X,MOUSE_Y,brush_z);
			mouse_x=MOUSE_X;
			mouse_y=MOUSE_Y;
			dist=0;
		}

		if(MOUSE_BUTTON[0])
		if(brush_x!=MOUSE_X || brush_y!=MOUSE_Y)
		{
			vec3f m0(mouse_x,mouse_y,0);
			vec3f m1(MOUSE_X,MOUSE_Y,0);

			if(!brush_const_x)mouse_x=MOUSE_X;else m1.x=mouse_x;
			if(!brush_const_y)mouse_y=MOUSE_Y;else m1.y=mouse_y;

			vec3f md=m1-m0,p,p2;
			
			int steps=md.len();
			
			loopl(0,steps+1)
			{
				if((l+FRAME) % (int(101-brush_rate)) !=0) continue;

				vec3f m01=m0+md*float(l)/float(steps);
				float mx=m01.x,my=m01.y;
				if(brush_z_const_disabled)	brush_z=get_z_xy(brush_z,mx,my);				
				p2=p;p=ogl_unproject(mx,my,brush_z);
				if(l==0)continue;

				int x=p.x,y=p.y,z=p.z;
				int x2=p2.x,y2=p2.y,z2=p2.z;

				//int sx=lvl1x*lvl0,sy=lvl1y*lvl0,sz=lvl1z*lvl0;

				float mul = (brush_type==1) ? -1 : 1; mul*=brush_intensity*brush_intensity;
				int rad = brush_size;

				if(brush_type==5) 
				{
					float bsl=brush_stroke_length;
					float bi =brush_intensity;

					if(dist<bsl/2)
						mul=bi*bsl/(bsl+5*abs(dist-bsl/2));
					else
						mul=bi*bsl/(bsl+abs(dist-bsl/2));

					mul=mul*mul;
				}

				static std::vector<VoxelData> vxl; vxl.clear();

				//read work cube to buffer

				vec3f color_brush_rgb3f;
				color_brush_rgb3f.decode_color_uint(brush_color_rgb);

				float mul2=mul;

				loopi(-rad,rad+1)
				loopj(-rad,rad+1)
				loopk(-rad,rad+1)
				if(i*i+j*j+k*k<rad*rad)
				{
					float r=1.0-sqrt(float(i*i+j*j+k*k))/brush_size;
					VoxelData v=get_voxel(x+i,y+j,z+k);
					
					uint intensity=v.intensity, intensity_in=intensity; 

					if(brush_intensity_random>0)
					{
						float r=brush_intensity_random*getRnd()/20.0;///20.0f;
						mul=max(mul2*r,0);
					}

					float r_brush_intensity=brush_intensity*r;
					//float r_brush_intensity01=brush_intensity*r/255.0;
					vec3f color3f=color_brush_rgb3f;
					//if(brush_color_random>0)
					{
						float r=brush_color_random*getRnd()/100.0f;
						color3f.x=clamp(color3f.x+r,0,1);
						color3f.y=clamp(color3f.y+r,0,1);
						color3f.z=clamp(color3f.z+r,0,1);

						if(brush_color_texture>0)
						{
							static Bmp tex1("../data/textures/stonewall.jpg");
							int px=(4*(x+i+tex1.width))%tex1.width;
							int py=(4*(y+j+tex1.height))%tex1.height;
							float cx=float(tex1.data[(px+py*tex1.width)*3+0])/255.0f;
							float cy=float(tex1.data[(px+py*tex1.width)*3+1])/255.0f;
							float cz=float(tex1.data[(px+py*tex1.width)*3+2])/255.0f;
							color3f=color3f*vec3f(cx,cy,cz);
						}
					}					

					
					switch (brush_type)
					{
						// "Draw", "Erase", "Grow", "Shrink", "Smear", "Stroke", "Colorize"

						// "Draw"
						case 0:
						// "Erase"
						case 1:
							intensity=float(max(0,min(255*256,mul*r+intensity_in)));
							break;
						// "Grow" smooth add
						case 2:
							intensity=0;
							loop(ii,-1,2)
							loop(jj,-1,2)
							loop(kk,-1,2)
							intensity+=get_voxel(x+i+ii,y+j+jj,z+k+kk).intensity;
						
							intensity/=27-3;
							intensity=clamp((intensity*r_brush_intensity+intensity_in*(511-r_brush_intensity))/511,intensity_in,255*256);
							break;							
						// "Shrink" smooth sub
						case 3:
							intensity=0;
							loop(ii,-1,2)
							loop(jj,-1,2)
							loop(kk,-1,2)
							intensity+=get_voxel(x+i+ii,y+j+jj,z+k+kk).intensity;
							
							intensity/=27+3;
							intensity=clamp((intensity*r_brush_intensity+(intensity_in)*(511-r_brush_intensity))/511,0,intensity_in);
							break;	
						// "Smear"  smear
						case 4:
							intensity=get_voxel(x2+i,y2+j,z2+k).intensity;
							r_brush_intensity*=1.2;
							r_brush_intensity=min(r_brush_intensity,255);
							//intensity=(intensity*r_brush_intensity+intensity_in*(255-r_brush_intensity))/255;
							intensity=max(intensity_in,(intensity*r_brush_intensity+intensity_in*(255-r_brush_intensity))/255);
							break;
						// "Stroke" stroke
						case 5:
							intensity=float(max(0,min(255*256,mul*r+intensity_in)));
							break;						
						// "Colorize" 
						case 6:
							r_brush_intensity/=255.0;
							intensity=intensity_in;//float(max(0,min(255*256,mul*r+intensity_in)));
							vec3f col;
							col.decode_color_uint(v.color);
							col=color3f*r_brush_intensity+col*(1-r_brush_intensity);
							v.color=col.encode_color_uint()|(brush_color_rgb&0xff000000);
							break;
					}
					//if (brush_intensity==255) intensity2= brush_type ? 0:255*256;
					//uint iintensity=int(intensity);

					if(brush_type==0 || brush_type==2 || brush_type==4 || brush_type==5)
					if(intensity>=128*256)
					{
						if(intensity_in>=128*256)
						{
							r_brush_intensity/=255.0;
							//intensity=intensity_in;//float(max(0,min(255*256,mul*r+intensity_in)));
							vec3f col;
							col.decode_color_uint(v.color);
							col=color3f*r_brush_intensity+col*(1-r_brush_intensity);
							v.color=col.encode_color_uint()|(brush_color_rgb&0xff000000);
						}else 
							v.color=brush_color_rgb;							
					}
						//v.color=(intensity>=128*256) ? brush_color_rgb : v.color;


					v.intensity=intensity;
					//
					vxl.push_back(v);
					//set_voxel(x+i,y+j,z+k,intensity2*256*256+color);
				}
				int ofs=0;
				loopi(-rad,rad+1)
				loopj(-rad,rad+1)
				loopk(-rad,rad+1)
				if(i*i+j*j+k*k<rad*rad)
				{
					set_voxel(x+i,y+j,z+k,vxl[ofs++]);
				}
				dist+=1;

			}			
		}
		brush_x=MOUSE_X;brush_y=MOUSE_Y;
	}// brush 0,1 end

	if(brush_tool==1) // poly
	{
		// mousebutton2 -> cancel
		static int cancel_x=0;
		static int cancel_y=0;
		if(MOUSE_BUTTON_DOWN[2])
		{
			cancel_x=MOUSE_X;
			cancel_y=MOUSE_Y;
		}
		if(MOUSE_BUTTON_UP[2])
		if(MOUSE_X==cancel_x)
		if(MOUSE_Y==cancel_y)
		{
			poly_edit=0;
			poly.clear();
		}

		if(!poly_edit)
		{
			if(MOUSE_BUTTON_DOWN[0])
			{
				if(poly.size()==0)
					brush_z=get_z(brush_z);//brush_z=ogl_read_z(MOUSE_X,MOUSE_Y);
				else
				if(brush_z_const_disabled)
				{
					brush_z=get_z(brush_z);
				}
				vec3f p=ogl_unproject(MOUSE_X,MOUSE_Y,brush_z);
				poly.push_back(p);
			}
			if(MOUSE_BUTTON[0])
			{
				if(brush_z_const_disabled)
				{
					brush_z=get_z(brush_z);
				}
				vec3f p=ogl_unproject(MOUSE_X,MOUSE_Y,brush_z);
				if(poly.size()>0)poly[poly.size()-1]=p;
			}
			if(MOUSE_BUTTON_UP[0])
			{
				if(poly.size()>2)
				{
					float d=(poly[poly.size()-1]-poly[poly.size()-2]).len();				
					if(d<2)
					{
						poly.resize(poly.size()-1);
						rasterize_poly(poly);
						poly.clear();
					}
				}
			}
		}

		draw_poly();

		if(poly_edit)
		{
			float val=poly_edit_handle;
			//glLineWidth(3);
			static int select=-1;
			static int direction=0;
			static vec3f select_dx;
			static vec3f select_dy;

			if(MOUSE_BUTTON_DOWN[0])
			{
				//brush_z=get_z(brush_z);;//
				brush_z=ogl_read_z(MOUSE_X,MOUSE_Y);
				vec3f p=ogl_unproject(MOUSE_X,MOUSE_Y,brush_z);
				vec3f px=ogl_unproject(MOUSE_X+val,MOUSE_Y,brush_z);
				vec3f py=ogl_unproject(MOUSE_X,MOUSE_Y+val,brush_z);
				select_dx=px-p;
				select_dy=py-p;
				
				//if(inside_box(p))
				{
					select=-1;
					float dmin=10000;
					loopi(0,poly.size())
					{
						vec3f v=poly[i];
						loopj(0,3)
						{
							vec3f add(0,0,0);
							add[j]=val;
							float d=(v+add-p).len();
							if(d<dmin)
							if(d<val*0.5)
							{
								select=i;
								direction=j;
								dmin=d;
							}
						}
					}
				}
				//else select=-1;
			}

			if(MOUSE_BUTTON[0])
			if(select>=0)
			{
				if(
					fabs(select_dx[direction])>
					fabs(select_dy[direction])
					)
					poly[select][direction]+=
						select_dx[direction]*MOUSE_DX/val;// : -MOUSE_DX;
				else
					poly[select][direction]+=
						select_dy[direction]*MOUSE_DY/val;;//(select_dy[direction]>0) ? MOUSE_DY : -MOUSE_DY;
			}
		}
		glColor3f(1,1,1);
	
	}// brush 2,3 end

	brush_pos=ogl_unproject(MOUSE_X,MOUSE_Y,brush_z);
}

void draw_logo()
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	static int tex_bg=ogl_tex_bmp("../data/logo.bmp");
	glActiveTextureARB( GL_TEXTURE0 );
	glBindTexture(GL_TEXTURE_2D,tex_bg);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1,1,1);
	ogl_drawquad(-1,-1,1,1,0,0,1,1);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
}




void draw()
{
	if(scene_reset)
	{
		reset();
//		MessageBoxA(0,"ko","ok",0);
	}
	if(MOUSE_BUTTON[2])
	{
		view_rot.x+=MOUSE_DY/4;
		view_rot.y+=MOUSE_DX/4;
	}
	if(MOUSE_BUTTON[1])
	{
		view_pos.x+=MOUSE_DX/1000;
		view_pos.y-=MOUSE_DY/1000;
	}	
	/*
	vec3f front	= m * vec3f( 0, 0, 1);
	vec3f side	= m * vec3f( 1, 0, 0);
	vec3f up	= m * vec3f( 0, 1, 0);

	if(KEYTAB[SDLK_w]) pos+=front*20/fps;
	if(KEYTAB[SDLK_s]) pos-=front*20/fps;
	if(KEYTAB[SDLK_a]) pos-=side*20/fps;
	if(KEYTAB[SDLK_d]) pos+=side*20/fps;
	if(KEYTAB[SDLK_e]) pos-=up*20/fps;
	if(KEYTAB[SDLK_q]) pos+=up*20/fps;	
	*/

	// general
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, (GLfloat)WINDOW_WIDTH/(GLfloat)WINDOW_HEIGHT, 0.01, 400.0);
	//float fov=(GLfloat)WINDOW_WIDTH/(GLfloat)WINDOW_HEIGHT;
	//glOrtho(-fov,fov, -1,1, 0.01, 400.0);//default = -1.0,+1.0,-1.0,+1.0,-1.0,+1.0
	glMatrixMode(GL_MODELVIEW);
    glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );

	if(MOUSE_X>menu_border) 
	if(MOUSE_X<WINDOW_WIDTH-menu_border) 
		voxel_editor();
	
	static FBO fbo(WINDOW_WIDTH_MAX,WINDOW_HEIGHT_MAX);
	fbo.enable(WINDOW_WIDTH,WINDOW_HEIGHT);
	glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
	draw_logo();
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	draw_voxel_cube(blocks);

	//Cursor 3D
	{
		float z=brush_z;//min(get_z(brush_z),brush_z);
		vec3f p=ogl_unproject(MOUSE_X,MOUSE_Y,z);//ogl_read_z(MOUSE_X,MOUSE_Y));
		vec3f px=ogl_unproject(MOUSE_X+10,MOUSE_Y,z)-p;
		vec3f py=ogl_unproject(MOUSE_X,MOUSE_Y+10,z)-p;
		px.norm();py.norm();

		// mouse pos cross rgb
		//if(!MOUSE_BUTTON[0])
		if(cursor_rgb_cross)
		{
			glColor3f(1,0,0);if(inside_box(vec3f(0,brush_pos.y,brush_pos.z)))ogl_drawline(0,brush_pos.y,brush_pos.z,float(lvl0*lvl1x),brush_pos.y,brush_pos.z);
			glColor3f(0,1,0);if(inside_box(vec3f(brush_pos.x,0,brush_pos.z)))ogl_drawline(brush_pos.x,0,brush_pos.z,brush_pos.x,float(lvl0*lvl1y),brush_pos.z);
			glColor3f(0,0,1);if(inside_box(vec3f(brush_pos.x,brush_pos.y,0)))ogl_drawline(brush_pos.x,brush_pos.y,0,brush_pos.x,brush_pos.y,float(lvl0*lvl1z));	
			glColor3f(1,1,1);
		}

		if(brush_tool==0)
		if(!MOUSE_BUTTON[0])
		if(inside_box(p))
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
			glPushMatrix();
			glTranslatef(p.x,p.y,p.z);
			glColor4f(1,1,1,0.2);
			glutSolidSphere(brush_size,16,8);
			glPopMatrix();
			glDisable(GL_BLEND);
		}


		if(0)
		{
			int gridsize=16;
			int mx=MOUSE_X;//WINDOW_WIDTH/2;//MOUSE_X;mx=(mx/gridsize)*gridsize;
			int my=MOUSE_Y;//WINDOW_HEIGHT/2;//MOUSE_Y;my=(my/gridsize)*gridsize;
			vec3f p=ogl_unproject(0,0,z);//ogl_read_z(MOUSE_X,MOUSE_Y));
			vec3f px=ogl_unproject(mx,0,z)-p;
			vec3f py=ogl_unproject(0,my,z)-p;

			float dx=(px).len()/float(gridsize);
			float dy=(py).len()/float(gridsize);
			dx=int(dx);
			dy=int(dy);

			vec3f pm=ogl_unproject(mx,my,z);

			px.norm(gridsize);
			py.norm(gridsize);

			p=p+px*dx+py*dy;

			int count=6;
			loopi(-count,count+1)
			loopj(-count,count+1)
			{
				vec3f p1=p+px*i+py*j;
				float a;//=1.0-vec3f(i,j,0).len()/float(count);
				a=1.0-(pm-p1).len()/float(count*gridsize);
				if(a<0.125)continue;
				glColor3f(a,a,a);				
				if(!inside_box(p1))continue;
				ogl_drawline(p1,p+px*(i+1)+py*j);
				ogl_drawline(p1,p+px*i+py*(j+1));
			}
		}
		//draw_grid(16);

	}	

	if(brush_tool==1) draw_poly();

	fbo.disable();

	//draw quad
	{
		glMatrixMode(GL_PROJECTION);glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);glLoadIdentity();
		glEnable(GL_TEXTURE_2D);	
		static Shader shader("../shader/editor/ssao");

		if(render_ssao)
		{
			shader.begin();
			shader.setUniform1i("tex_col", 0);
			shader.setUniform1i("tex_depth", 1);
			glActiveTextureARB( GL_TEXTURE1 );
			glBindTexture(GL_TEXTURE_2D,fbo.depth_tex);
		}
		glActiveTextureARB( GL_TEXTURE0 );
		glBindTexture(GL_TEXTURE_2D,fbo.color_tex);

		glColor3f(1,1,1);
		{
			float x=float(WINDOW_WIDTH)/float(WINDOW_WIDTH_MAX);
			float y=float(WINDOW_HEIGHT)/float(WINDOW_HEIGHT_MAX);
			ogl_drawquad(-1,-1,1,1,0,0,x,y);
		}
		if(render_ssao) shader.end();
	}

	menu();

	//error?
	//error_check_mem(&blocks[0],sizeof(Block)*blocks.size());
	triangulate();

	blocks_garbage_collect();
	//Sleep(5);	
}


}//namespace end

/*
//color picker
				{
				   COLORREF  colors[16];
				   CHOOSECOLOR  cc;  // common dialog box structure 
				   ZeroMemory(&cc, sizeof(cc));
				   cc.lStructSize = sizeof(cc);
				   ZeroMemory(colors, sizeof(colors));
				   cc.lpCustColors = colors;
				   cc.rgbResult = RGB(0,0,0);
				   cc.Flags = CC_FULLOPEN | CC_RGBINIT;
   
				   if(ChooseColor(&cc))
				   {
					  int const  r = GetRValue(cc.rgbResult);
					  int const  g = GetGValue(cc.rgbResult);
					  int const  b = GetBValue(cc.rgbResult);
					  //return(255 << 24 | b << 16 | g << 8 | r);
				   }
				   //return(o);
				}



*/