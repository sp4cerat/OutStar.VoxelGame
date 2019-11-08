namespace world// namespace begin
{

namespace camera// namespace begin
{
	vec3f pos(0.0001,-140.0001,0.0001);
	vec3f rot(0.0001,0.0001,0.0001);
	float fovscale,fovxy,fovx,fovy;
	uint res_x,res_y;
}
namespace cfg// namespace begin
{
	bool remove_outlier=0;//true;
	bool fillhole=0;//true;
	bool fillhole_screen=0;//true;
	//bool coarse_to_fine=true;
	int  hole_pixel_threshold=3;
	int  tiles_horizontal=1;
	int  tiles_vertical  =1;
	bool fxaa=0;
	int  block_index=0;
	int  block_rotation=0;
	int  block_rotation_val=0;
	int  block_repeat=1;
	bool godmode=false;
}


struct Entity
{
	vec3f pos;
	vec3f rot;
	int type;
};

std::vector<Mesh>	objects;
std::vector<Entity> players;
std::vector<Entity> enemies;
std::vector<Entity> animals;
std::vector<Entity> shots;

void init_entities()
{

}

void menu()
{
	// tiles
	if(cfg::godmode)
	{		
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);       // Blending Function For Translucency Based On Source Alpha 
		
		float sizex=0.15,sizey=sizex*float(WINDOW_WIDTH)/float(WINDOW_HEIGHT);
		loopi(0,octreeblock::oblocks.size())
		{
			glColor4f(1,1,1,1);
			if(i==cfg::block_index)glColor4f(1,1,1,1);
			float x=sizex*1.2*float(i-cfg::block_index)-sizex*0.5,y=-1+sizex*0.1;
			ogl_bind(0,octreeblock::oblocks[i].texture_id);
			ogl_drawquad(x,y,x+sizex,y+sizey,0,1,1,0);
			ogl_bind(0,0);			
			if(i==cfg::block_index)	ogl_drawlinequad(
				x-sizex*0.05,
				y-sizex*0.05,
				x+sizex*1.05,
				y+sizey*1.05);
		}
	}
	glColor3f(1,1,1);
	glDisable(GL_BLEND);

	//fps
	{
		static nv::Rect none=nv::Rect(0,0,0,0);
		ui.begin();
		ui.beginGroup(nv::GroupFlags_GrowDownFromRight);
		ui.doLabel(none , str("Fps %2.2f @ %2.0fx%2.0f",fps,raycast_width,raycast_height));
		ui.endGroup();
		ui.end();
	}


	return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	glColor4f(0.1,0.1,0.1,0.8);
	ogl_drawquad(-1,0,-1+400.0f/float(WINDOW_WIDTH),1);
	glDisable(GL_BLEND);

	static nv::Rect none=nv::Rect(0,0,0,0);
    ui.begin();
	ui.beginGroup(nv::GroupFlags_GrowDownFromLeft);
	
	ui.doCheckButton(none, "FXAA On/Off", &cfg::fxaa);
	//ui.doCheckButton(none, "remove_outlier", &cfg::remove_outlier);
	//ui.doCheckButton(none, "fillhole", &cfg::fillhole);
	//ui.doCheckButton(none, "fillhole_screen", &cfg::fillhole_screen);
	//ui.doCheckButton(none, "coarse_to_fine", &cfg::fillhole_screen);
	
		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		{
			char  text[100];static float a=raycast_width;
			ui.doHorizontalSlider(none, 1920/4,1920, &a);
			raycast_width=(uint(a)>>6)<<6;
			sprintf(text,"%3.0f pix (w)",raycast_width);ui.doLabel( none, text);
		}
		ui.endGroup();

		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		{
			char  text[100];static float a=raycast_height;
			ui.doHorizontalSlider(none, 1080/4,1080, &a);
			raycast_height=(uint(a)>>6)<<6;
			sprintf(text,"%3.0f pix (h)",raycast_height);ui.doLabel( none, text);
		}
		ui.endGroup();	

		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		{
			char  text[100];static float a=cfg::hole_pixel_threshold;
			ui.doHorizontalSlider(none, 1,4, &a);
			cfg::hole_pixel_threshold=uint(a);
			sprintf(text,"%d err pix",cfg::hole_pixel_threshold);ui.doLabel( none, text);
		}
		ui.endGroup();		
		
		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		{
			char  text[100];static float a=cfg::tiles_horizontal;
			ui.doHorizontalSlider(none, 1,8, &a);
			cfg::tiles_horizontal=uint(a);
			sprintf(text,"%d tiles x",cfg::tiles_horizontal);ui.doLabel( none, text);
		}
		ui.endGroup();		
		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		{
			char  text[100];static float a=cfg::tiles_vertical;
			ui.doHorizontalSlider(none, 1,8, &a);
			cfg::tiles_vertical=uint(a);
			sprintf(text,"%d tiles y",cfg::tiles_vertical);ui.doLabel( none, text);
		}
		ui.endGroup();
	
	char text[100];
	sprintf(text,"Fps %2.2f",fps);
	ui.doLabel(none , text);

	ui.endGroup();
    ui.end();
}

vec3f collide(vec3f pos,vec3f pos_before)
{
	float ax=20,ay1=20,ay2=80,az=20;		
	static double t0=timeGetTime();		
	if(cfg::godmode){t0=double(timeGetTime());return pos;}
	double accely=(double(timeGetTime())-t0);
	
	pos.y+=accely/fps;
	bool hit_ground=0,hit_side=0;
	vec3f p = pos_before,pos_tmp=pos; 
	p.y=pos.y;
	
	if( octree::collide::aabb(
		p.x-ax,p.y-ay1,p.z-az,
		p.x+ax,p.y+ay2,p.z+az) )
	{
		hit_ground=1;
		pos.y = pos_before.y; 
	}

	if(USE_TERRAIN)
	if(pos.y>-terrain::get_height(pos.x,pos.z)-ay2)
	{
		pos.y=-terrain::get_height(pos.x,pos.z)-ay2;
		hit_ground=1;
	}

	if( octree::collide::aabb(
		pos.x-ax,pos.y-ay1,pos.z-az,
		pos.x+ax,pos.y+ay2,pos.z+az) )	
	{
		bool step_up=0;

		pos_tmp.y=pos.y;

		//if(hit_ground)
		loopi(0,3)
		if(!octree::collide::aabb(
			pos_tmp.x-ax,pos_tmp.y-i*10-ay1,pos_tmp.z-az,
			pos_tmp.x+ax,pos_tmp.y-i*10+ay2,pos_tmp.z+az) )
		{
			loopj(0,5)
			if( !octree::collide::aabb(
				 pos_tmp.x-ax,pos_tmp.y-i*10+j*2-ay1,pos_tmp.z-az,
				 pos_tmp.x+ax,pos_tmp.y-i*10+j*2+ay2,pos_tmp.z+az) )
			{
				step_up=1;
				pos=pos_tmp;
				pos.y=pos.y-i*6+j*2;
				break;
			}
			break;
		}

		if(!step_up)
		{
			vec3f dd=pos-pos_before;
			vec3f dn=dd;dn.norm();
			vec3f n = pos_before-octree::collide::aabb_hit;
			n.y=0;n.norm();
			vec3f p=pos;
			bool c=true;
			
			loopi(0,5)
			{
				c=octree::collide::aabb(
					p.x-ax,p.y-ay1,p.z-az,
					p.x+ax,p.y+ay2,p.z+az);
				if(!c) break;
				p=p+n*2;
			}
			pos = c ? pos_before : p;
		}
		hit_side=1;
	}
	 
	if (hit_ground)
	{
		t0=timeGetTime();
		if(KEYTAB[SDLK_SPACE]) {t0=timeGetTime()+500;}
	}
	//if(KEYTAB[SDLK_r]) {t0=timeGetTime()+12000;}
		
	//pos=pos_before;
	/*
	printf("c %f %f %f p %f %f %f ",
		octree::collide::aabb_hit.x,
		octree::collide::aabb_hit.y,
		octree::collide::aabb_hit.z,
		pos.x,pos.y,pos.z
		);*/
	return pos;
}

void player_input()
{
	vec3f &pos=camera::pos;
	vec3f &rot=camera::rot;

	rot.y=(MOUSE_X-WINDOW_WIDTH/2)*0.01+.001;
	rot.x=(MOUSE_Y-WINDOW_HEIGHT/2)*0.01+.001;

	//printf("pos %.2f %.2f %.2f rot  %.2f %.2f %.2f\r",pos.x,pos.y,pos.z,rot.x,rot.y,rot.z);

	

	matrix44 m;
	m.rotate_z( -rot.z );
	if(cfg::godmode)
	m.rotate_x( -rot.x );
	m.rotate_y( -rot.y );
	
	vec3f front	= m * vec3f( 0, 0, 1);
	vec3f side	= m * vec3f( 1, 0, 0);
	vec3f up	= m * vec3f( 0, 1, 0);

	vec3f pos_before=pos;

	bool camera_moved=false;
	float speed=40*8;
	if(KEYTAB[SDLK_LSHIFT]) {speed*=20;}
	if(KEYTAB[SDLK_w]) {pos+=front*speed/fps;camera_moved=true;}
	if(KEYTAB[SDLK_s]) {pos-=front*speed/fps;camera_moved=true;}
	if(KEYTAB[SDLK_d]) {pos-=side*speed/fps;camera_moved=true;}
	if(KEYTAB[SDLK_a]) {pos+=side*speed/fps;camera_moved=true;}
	if(KEYTAB[SDLK_e]) {pos-=up*speed/fps;camera_moved=true;}
	if(KEYTAB[SDLK_q]) {pos+=up*speed/fps;camera_moved=true;}
	if(KEYTAB_DOWN[SDLK_F7]) { raycast_width=1024;raycast_height=512; }
	if(KEYTAB_DOWN[SDLK_F8]) { raycast_width=1280;raycast_height=768; }
	if(KEYTAB_DOWN[SDLK_F9]) { raycast_width=1536;raycast_height=768; }
	if(KEYTAB_DOWN[SDLK_F10]){ raycast_width=1920;raycast_height=1080; }
	if(KEYTAB_DOWN[SDLK_F11]){ cfg::fxaa^=1; }
	if(KEYTAB_DOWN[SDLK_TAB]) { cfg::godmode^=1; }

	loopi(0,9) if(KEYTAB_DOWN[SDLK_1+i]) { cfg::block_repeat=1<<i; }

	{
		char lvl[]={"../level/default"};

		static int init=true;

		if(KEYTAB_DOWN[SDLK_F5])
		{
			octree::save_compact(str("%s.oct",lvl));

			loopi(0,octreeblock::oblocks.size())
			{
				octreeblock::oblocks[i].save(str("%s%d",lvl,i));
			}
		}
		if(KEYTAB_DOWN[SDLK_F6] || init )
		{
			init=false;
			if(file_exists(str("%s.oct",lvl)))
			{
				octree::load_compact(str("%s.oct",lvl));

				int c=0;
				while(file_exists(str("%s%d.block",lvl,c)))c++;
				octreeblock::oblocks.resize(c);

				loopi(0,c) octreeblock::oblocks[i].load(str("%s%d",lvl,i));

				raycast::update_gpu_octree();
			}
		}
	}
	// collision
	pos=collide(pos,pos_before);	

	// set / erase block
	if(cfg::godmode)
	{
		cfg::block_index=clamp(cfg::block_index+MOUSE_DWHEEL , 0, octreeblock::oblocks.size()-1);MOUSE_DWHEEL =0;
		matrix44 m;
		m.ident();
		m.rotate_z( -rot.z );
		m.rotate_x( -rot.x );
		m.rotate_y( -rot.y );	
		vec3f front = m * vec3f( 0, 0, 1);


		// Block editor
		{	
			int x=sin(camera::rot.y+M_PI/4)>0 ? 0 : 1;
			int z=cos(camera::rot.y+M_PI/4)>0 ? 1 : 0;
			cfg::block_rotation_val=((x+z*2)+3)%4;//^z;
			cfg::block_rotation    =(((x+z*2)^z)+3)%4;//^z;
		}

		// block copy paste
		{
			static octreeblock::OBlock ob;
			static int x1=-1,y1=-1,z1=-1;
			static int x2=-1,y2=-1,z2=-1;

			if(KEYTAB_DOWN[SDLK_1]){x1=pos.x;y1=pos.y;z1=pos.z;}
			if(KEYTAB_DOWN[SDLK_2]){x2=pos.x;y2=pos.y;z2=pos.z;}
			if(KEYTAB_DOWN[SDLK_3])
			{
				octreeblock::groupblocks.clear();
				octreeblock::gather_groupblocks(
					min(x1,x2),min(y1,y2),min(z1,z2),
					max(x1,x2),max(y1,y2),max(z1,z2));

				ob.suboblocks=octreeblock::groupblocks;//oblocks[0].suboblocks;//groupblocks;
				if(ob.suboblocks.size()>0)
				{
					int xmin=ob.suboblocks[0].x;
					int ymin=ob.suboblocks[0].y;
					int zmin=ob.suboblocks[0].z;
					int xmax=xmin,ymax=ymin,zmax=zmin;

					loopi(0,ob.suboblocks.size())
					{
						xmin=min(ob.suboblocks[i].x,xmin);
						ymin=min(ob.suboblocks[i].y,ymin);
						zmin=min(ob.suboblocks[i].z,zmin);
						xmax=max(ob.suboblocks[i].x,xmax);
						ymax=max(ob.suboblocks[i].y,ymax);
						zmax=max(ob.suboblocks[i].z,zmax);
					}
					loopi(0,ob.suboblocks.size())
					{
						ob.suboblocks[i].x-=xmin;
						ob.suboblocks[i].y-=ymin;
						ob.suboblocks[i].z-=zmin;
					}
					ob.sx=64*(1+xmax-xmin);
					ob.sy=64*(1+ymax-ymin);
					ob.sz=64*(1+zmax-zmin);

					printf("\nblocks %d\n",ob.suboblocks.size());			
					loopi(0,ob.suboblocks.size())
					{
						printf("blocks %d %d %d %d r %d\n",i,
						ob.suboblocks[i].x,
						ob.suboblocks[i].y,
						ob.suboblocks[i].z,
						ob.suboblocks[i].root
						);
					}
					Bmp texture(128,128,32,0);
					loopijk(0,0,0,128,128,4)
						texture.data[(i+j*128)*4+k]=i+j;

					ob.texture_id=ogl_tex_bmp(texture);				
					ob.texture=texture;
					octreeblock::oblocks.push_back(ob);
				}
			}
			if(KEYTAB_DOWN[SDLK_4])
			{
				octreeblock::set_block(0,pos.x,pos.y,pos.z,0,0,&ob);
				//octreeblock::set_block(cfg::block_index,x,y,z,0,0);//xzmirror[xz]); 
				raycast::update_gpu_octree();
				
				//octreeblock::set_subblock(int x,int y,int z,SubOBlock &sb,bool remove_block=0,int rotate=0)
			}
		}

		static float dx1=0,dy1=0,dz1=0;
		// add block
		if( MOUSE_BUTTON_DOWN[0] || MOUSE_BUTTON_DOWN[2]|| (MOUSE_BUTTON[0]&&MOUSE_BUTTON[2])  )
		if( octreeblock::oblocks.size()>0)
		{ 
			dx1=pos.x+front.x*4*64;
			dy1=pos.y+front.y*4*64;
			dz1=pos.z+front.z*4*64;
		}
		// add block
		if( MOUSE_BUTTON_UP[0] || MOUSE_BUTTON_UP[2] || (MOUSE_BUTTON[0]&&MOUSE_BUTTON[2]) )
		if( octreeblock::oblocks.size()>0)
		{ 
			float dx2=pos.x+front.x*4*64;
			float dy2=pos.y+front.y*4*64;
			float dz2=pos.z+front.z*4*64;

			int addremove= MOUSE_BUTTON_UP[0] ? 0 : 1;
			if((MOUSE_BUTTON[0]&&MOUSE_BUTTON[2]))addremove=0;

			float dx=64;//octreeblock::oblocks[block_index].sx/2;
			float dy=64;//octreeblock::oblocks[block_index].sy/2;
			float dz=64;//octreeblock::oblocks[block_index].sz/2;
			float dm=max(dx,max(dy,dz));
			dx=min(dx1,dx2);//pos.x+front.x*4*64;
			dy=min(dy1,dy2);//pos.y+front.y*4*64;
			dz=min(dz1,dz2);//pos.z+front.z*4*64;

			int repx=(max(dx1,dx2)-dx)/octreeblock::oblocks[cfg::block_index].sx;
			int repy=(max(dy1,dy2)-dy)/octreeblock::oblocks[cfg::block_index].sy;
			int repz=(max(dz1,dz2)-dz)/octreeblock::oblocks[cfg::block_index].sz;

			loopijk(0,0,0,repx+1,repy+1,repz+1)
				/*cfg::block_repeat)
			loopj(0,cfg::block_repeat)*/
			{
				//MessageBoxA(0,"","",0);
				/*
				int x= -octreeblock::oblocks[cfg::block_index].sx*i;
				int y= -octreeblock::oblocks[cfg::block_index].sy*j;
				int z= 0;
				if(cfg::block_rotation_val&2)
				{
					int a=x;x=z;z=a;
					x=-x;
					z=-z;
				}
				x+=dx;y+=dy;z+=dz;*/
				int x= dx+octreeblock::oblocks[cfg::block_index].sx*i;
				int y= dy+octreeblock::oblocks[cfg::block_index].sy*j;
				int z= dz+octreeblock::oblocks[cfg::block_index].sz*k;
				octreeblock::set_block(cfg::block_index,x,y,z,addremove,cfg::block_rotation_val);//xzmirror[xz]); 
			}
			raycast::update_gpu_octree();
			//Sleep(100);
		}
		/*
		// remove
		if( KEYTAB_DOWN[SDLK_RETURN] || MOUSE_BUTTON[2] )
		if( octreeblock::oblocks.size()>0)
		{ 
			//int i=0,j=0,k=0;//loopijk(0,0,0,10,10,10)
			float dx=64;//octreeblock::oblocks[block_index].sx/2;
			float dy=64;//octreeblock::oblocks[block_index].sy/2;
			float dz=64;//octreeblock::oblocks[block_index].sz/2;
			float dm=max(dx,max(dy,dz));
			dx=pos.x-dx*0+front.x*4*64;
			dy=pos.y-dy*0+front.y*4*64;
			dz=pos.z-dz*0+front.z*4*64;
			octreeblock::set_block(cfg::block_index,dx,dy,dz,1,cfg::block_rotation_val); 
			raycast::update_gpu_octree();
			//Sleep(100);
		}
		*/
	}
}

void polygon_draw()
{
	vec3f &pos=camera::pos;
	vec3f &rot=camera::rot;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_CULL_FACE);

	//int vp[4];
	glMatrixMode( GL_PROJECTION);
	glLoadIdentity();

	if(1)
	{
		//float n=1.0,f=1*terrain::map_scale*1.5,ff=1/tan(fovxy/2) ;
		float n=1.0,f=terrain::map_scale*1.5;//,ff=4;//1/tan(0.25) ;
		float m16[16]=
		{
			camera::fovscale*4*camera::fovxy, 0 , 0 , 0,
			0,camera::fovscale*4,0,0,
			0,0,(n+f)/(n-f),2*f*n/(n-f),
			0,0,-1,0
		};
		glLoadMatrixf(m16);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if(1)
	{
		// sky
		skybox::Draw(rot);

		glRotatef(rot.x*180/M_PI,1,0,0);
		glRotatef(rot.y*180/M_PI,0,1,0);

		// gl params
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);

		//printf("xyz %f %f %f r %f %f % \r",pos.x,pos.y,pos.z,rot.x,rot.y,rot.z);

		// terrain 
		if(USE_TERRAIN)
		if(1)
		{
			terrain::wireframe=0;
			terrain::draw(pos.x,pos.y,pos.z);
		}

		if(!cfg::godmode)
		{
			glLoadIdentity();
			glTranslated(10,-7,-5);
			if(1)
			{
				sunlightvec.w=-rot.y/(2*M_PI);
				matrix44 mat;
				mat.rotate_y(rot.y);
				mat.rotate_x(rot.x);
				vec4f tmp=sunlightvec;
				sunlightvec=mat*vec4f(sunlightvec.x,sunlightvec.y,sunlightvec.z,0);
				sunlightvec.w=-rot.y/(2*M_PI);
				
				int lod=0;	
				static Mesh weapon(
					"../data/weapon/ogre.material",
					"../data/weapon/ogre.mesh.xml");	
				glScalef(5,5,5);
				double t=double(timeGetTime());
				bool walk=KEYTAB[SDLK_w]|KEYTAB[SDLK_s]|KEYTAB[SDLK_a]|KEYTAB[SDLK_d];
				weapon.Draw(vec3f(
					walk ? sin(t/200)*0.1 : sin(t/2000)*0.05, // walk / idle
					walk ? sin(t/100)*0.1 : sin(t/1000)*0.05, // walk / idle
					0),
					vec3f(0,0,0),lod);

				sunlightvec=tmp;
			}
		}

		// block cube
		if(cfg::godmode)
		{
			glLoadIdentity();
			glRotatef(rot.x*180/M_PI,1,0,0);
			glRotatef(rot.y*180/M_PI,0,1,0);
			{
				glPushMatrix();

				float dx0=64;
				float dy0=64;
				float dz0=64;
				if(octreeblock::oblocks.size()>cfg::block_index)
				{
					dx0=octreeblock::oblocks[cfg::block_index].sx;
					dy0=octreeblock::oblocks[cfg::block_index].sy;
					dz0=octreeblock::oblocks[cfg::block_index].sz;
				}

				dx0=dx0*cfg::block_repeat;
				dy0=dy0*cfg::block_repeat;

				if(cfg::block_rotation_val&2)
				{
					int a=dx0;dx0=dz0;dz0=a;
				}

				matrix44 m;
				m.ident();
				m.rotate_x( -rot.x );
				m.rotate_y( -rot.y );	
				vec3f front = m * vec3f( 0, 0, 1);

				float dx=front.x*4*64;
				float dy=front.y*4*64;
				float dz=front.z*4*64;

				vec3f p=pos+vec3f(OCTREE_DIM/2,OCTREE_DIM/2,OCTREE_DIM/2);
					
				double px=(int(int(p.x+dx)/64)*64-p.x)+64-dx0/2;
				double py=(int(int(p.y+dy)/64)*64-p.y)+64-dy0/2;//-64/2;
				double pz=(int(int(p.z+dz)/64)*64-p.z)+dz0/2;//-64/2+64;

				glLineWidth(2);
				glTranslated(-px,-py,-pz);
				glScalef(dx0,dy0,dz0);
				
				glColor3f(
					sin(  double(timeGetTime())/1000)*0.5+0.5,
					sin(1+double(timeGetTime())/1000)*0.5+0.5,
					sin(2+double(timeGetTime())/1000)*0.5+0.5);
				glutWireCube(1);
				glPopMatrix();
			}

			glTranslated(pos.x,pos.y,pos.z);
	
			if(1)
			{
				int lod=0;
	
				static Mesh turrican(
					"../data/turrican/ogre.material",
					"../data/turrican/ogre.mesh.xml",
					"../data/turrican/ogre.skeleton.xml");
	
				if(GetAsyncKeyState(VK_F1))
					turrican.animation.SetPose(1,double(timeGetTime())/100.0);
				else
					turrican.animation.SetPose(0,double(timeGetTime())/1000.0);
	
				static uint   time_zero=timeGetTime();
				double time_elapsed=float(timeGetTime()-time_zero)/1000.0;

				glScalef(15,15,15);
				loopi(-0,1) loopj(-0,1)
				{
					turrican.Draw(vec3f(i*15,0,-20+j*15),vec3f(0,0*time_elapsed*0.1,0),lod);
				}
			}


		}
	}
	glDisable(GL_DEPTH_TEST);		
}

void draw()
{
	//skybox
	static FBO fbo(WINDOW_WIDTH_MAX,WINDOW_HEIGHT_MAX);

	fbo.enable(camera::res_x,camera::res_y);

		polygon_draw();

	fbo.disable();
	
	raycast::draw(
		camera::res_x,camera::res_y,
		(camera::pos*vec3f(-1,-1,1)+vec3f(OCTREE_DIM/2,OCTREE_DIM/2,OCTREE_DIM/2)),
		camera::rot,
		fbo,
		cfg::tiles_horizontal,
		cfg::tiles_vertical,
		cfg::hole_pixel_threshold,
		camera::fovx,camera::fovy,
		cfg::fxaa );
	
	return;	
}

void update(int res_x, int res_y)
{

	camera::fovscale=0.75;
	camera::fovxy= float(WINDOW_HEIGHT)/float(WINDOW_WIDTH);
	camera::fovx = 1/float(camera::fovscale*float(res_x)*camera::fovxy);
	camera::fovy = 1/float(camera::fovscale*float(res_y));
	camera::res_x=res_x;
	camera::res_y=res_y;

	player_input();
	draw();
	menu();
}


} // namespace


/*

		if(0) // bench test
		{
			static bool init=1;
			if(init)
			{
				init=0;
				loopi(0,40)
				loopj(0,5)
				loopk(0,40)
				{
					octreeblock::set_block(cfg::block_index,i*256,-j*256,k*256,0); 
				}
				raycast::update_gpu_octree();
			}
		}
		if(0) // minecraft test
		{
			static bool init=1;
			if(init)
			{
				loopi(-350,350)
				loopj(-350,350)
				{
					int x=i*64;
					int y=
						(sin(float(i*3+j*3)/20)+
						sin(float(i)/20)+sin(float(i*3+j)/20)*0.5+
						sin(float(j)/20)+sin(float(j*3+i)/20)*0.5
						)*400;
					int z=j*64;
					octreeblock::set_block(cfg::block_index,x,y,z,0); 
					octreeblock::set_block(cfg::block_index,x,y+64,z,0); 
				}
				raycast::update_gpu_octree();
				init=0;
			}
		}
*/