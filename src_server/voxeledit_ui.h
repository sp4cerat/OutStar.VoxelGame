
void menu()
{
	// Background begin
	glBindTexture(GL_TEXTURE_2D,0);
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0,WINDOW_WIDTH,0,WINDOW_HEIGHT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	glColor4f(0.1,0.1,0.1,0.8);
	ogl_drawquad(0,0,menu_border,WINDOW_HEIGHT);
	ogl_drawquad(WINDOW_WIDTH-menu_border,0,WINDOW_WIDTH,WINDOW_HEIGHT);
	glDisable(GL_BLEND);
	glColor4f(0.4,0.4,0.4,1);
	ogl_drawline(menu_border,0,0,menu_border,WINDOW_HEIGHT,0);
	ogl_drawline(WINDOW_WIDTH-menu_border,0,0,WINDOW_WIDTH-menu_border,WINDOW_HEIGHT,0);
	// Background end

	// palette begin
	{
		int w=192+8,h=192,px=10,py=10;

		//glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
		static int tex_pal=ogl_tex_bmp(palette_name,GL_NEAREST);	
		glActiveTextureARB( GL_TEXTURE0 );
		glBindTexture(GL_TEXTURE_2D,tex_pal);
		glEnable(GL_TEXTURE_2D);
		glColor3f(1,1,1);
		ogl_drawquad(px+w-16,py,px+w,h+py,0,0,1,1);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);

		int x=MOUSE_X;//-dw/32;
		int y=WINDOW_HEIGHT-MOUSE_Y;//-dh/32;
		int cx=(x-px);
		int cy=(y-py);
		
		// color select
		{
			vec3f crgb;
			crgb.decode_color_uint(brush_color_rgb);//palette_bmp.get_pixel3f( , );			
			glColor3f(crgb.x,crgb.y,crgb.z);
			ogl_drawquad(px,py+h+10,32+px,32+py+h+10);

			// set color rgb
			brush_color_rgb=brush_color_rgb&0xffffff;
			brush_color_rgb+=uint(brush_color_specular)<<24;
			brush_color_rgb+=uint(brush_color_glow    )<<26;
			brush_color_rgb+=uint(brush_color_effect  )<<28;
			//brush_color_rgb+=uint(brush_color_reflect )<<30;
		}

		static float hue=0; static vec3f hue_color=vec3f(1,0,0);

		//colorquad
		ogl_draw_colorquad(px,py,px+w-20,py+h,
			vec3f(0,0,0),
			vec3f(0,0,0),
			vec3f(1,1,1),
			hue_color
			);
		glColor3f(0.5,0.5,0.5);
		ogl_drawlinequad(px,py,px+w-20,py+h);

		static float last_x=px,last_y=py+h;

		glColor3f(0,0,0);ogl_drawlinequad(last_x-2,last_y-4,last_x+4,last_y+2);
		glColor3f(1,1,1);ogl_drawlinequad(last_x-3,last_y-3,last_x+3,last_y+3);
		//ogl_drawline(last_x,py,0,last_x,py+h,0);
		//ogl_drawline(px,last_y,0,px+w-20,last_y,0);

		if(cx<w)if(cy<h)if(cy>0)
		if(MOUSE_BUTTON[0])
		{
			if(cx>w-16)
			{
				hue=cy;
				hue_color = palette_bmp.get_pixel3f(1,float(cy*palette_bmp.height)/float(h));
			}
			if(cx<w-20)
			{
				last_x = x;last_y = y;
			}

			vec3f c0=vec3f(0,0,0);
			vec3f c1=vec3f(0,0,0);
			vec3f c2=vec3f(1,1,1);
			vec3f c3=hue_color;

			float a=float(last_x-px)/float(w-20);
			float b=float(last_y-py)/float(h);

			vec3f c01=c0+(c1-c0)*a;
			vec3f c32=c3+(c2-c3)*a;
			vec3f c=c01+(c32-c01)*b;
			brush_color_rgb=c.encode_color_uint();
			
		}

		glColor3f(1,1,1);
		ogl_drawlinequad(	px+w-18,py+hue-1,
							px+w+2 ,py+hue+1);
	}

	// palette end
	
    static nv::Rect none=nv::Rect(0,0,0,0);
	char  text[1000];
    
    ui.begin();

	ui.beginGroup(nv::GroupFlags_GrowDownFromLeft);
	static nv::Rect r=nv::Rect(menu_border,0,0,0);

	//help
	if(help_on)
	{	
		nv_ui_set_text_color(0.4,0.4,0.4);
		ui.doLabel( r , help_mb1);
		ui.doLabel( r , help_mb2);
		ui.doLabel( r , help_mb3);
		ui.doLabel( r , help_mw);
		nv_ui_set_text_color(1,1,1);
		
	}
	ui.endGroup();

	//stats
	{
		ui.beginGroup(nv::GroupFlags_GrowUpFromLeft);
		
		static nv::Rect r=nv::Rect(menu_border,0,0,0);
		sprintf(text,"Triangles: %d Fps: %2.2f",trianglecount,fps);

		nv_ui_set_text_color(0.4,0.4,0.4);
		ui.doLabel( r , text);
		nv_ui_set_text_color(1,1,1);
		ui.endGroup();
	}

    ui.beginGroup(nv::GroupFlags_GrowDownFromRight);
	{
		ui.beginGroup( nv::GroupFlags_GrowRightFromTop);
		{
			if (ui.doButton(none, "Clear All"))
			{
				if(win_messagebox_confirm("Please Confirm","Clear the scene ?"))
				{
					scene_reset=true;
				}
			}

			static int select=0;
			const char * formatLabel[] = { 
				"Load / Save", 
				"Load Voxel", 
				"Save Voxel",
				"Load Obj",
				"Save Obj",
				"Load Brush",
				"Save Brush",
				"Load Image"
			};

			ui.doComboBox(none, 8, formatLabel, &select);

			if(select>0)
			{
				if(select==1)
				{
					char* name=win_getopenfilename("Voxel Files(*.voxel)\0*.voxel\0");
					if(name)load_from_voxel(name);
				}				
				if(select==2)
				{	
					char* name=win_getsavefilename("Voxel Files(*.voxel)\0*.voxel\0");
					if(name)save_voxel(name);
				}			
				if(select==3)
				{
					char* name=win_getopenfilename("Obj Files(*.obj)\0*.obj\0");
				}				
				if(select==4)
				{
					char* name=win_getsavefilename("Obj Files(*.obj)\0*.obj\0");
					save_obj(name);
				}
				if(select==5)
				{
					char* name=win_getopenfilename("Brush Files(*.brush)\0*.brush\0");
				}				
				if(select==6)
				{
					char* name=win_getsavefilename("Brush Files(*.brush)\0*.brush\0");
				}
				if(select==7)
				{
					char* name=win_getsavefilename();
				}
				select=0;
			}

			if (ui.doButton(none, "Undo"))
			{
				if(poly.size()>0)
				{
					poly.resize(poly.size()-1);
				}
				else
					blocks=blocks_undo;
			}

		}
		ui.endGroup();
	}
	
	if(1)
	{
		ui.beginGroup( nv::GroupFlags_GrowRightFromTop);
		ui.doLabel( none, "Voxel Cube Resoution (XYZ)");
		ui.endGroup();
		ui.beginGroup( nv::GroupFlags_GrowRightFromTop);

		
		const char * formatLabel[] = { 
			"16", 
			"32", 
			"64", 
			"128",
			"256",
			"512",
			"1024"
		};


		static int selectx=log(float(lvl1x))/log(2.0f)+0;
		ui.doComboBox(none, 7, formatLabel, &selectx);
		if((1<<(selectx))!=lvl1x)
		{
			if(win_messagebox_confirm("Please Confirm","Clear the scene ?"))
			{
				lvl1x=1<<(selectx);
				blocks.clear();
				blocks_undo.clear();
				scene_reset=true;
			}
			else selectx=log(float(lvl1x))/log(2.0f)+0;
		}


		static int selecty=log(float(lvl1y))/log(2.0f)+0;
		ui.doComboBox(none, 7, formatLabel, &selecty);
		if((1<<(selecty))!=lvl1y)
		{
			if(win_messagebox_confirm("Please Confirm","Clear the scene ?"))
			{
				lvl1y=1<<(selecty);
				blocks.clear();
				blocks_undo.clear();
				scene_reset=true;
			}
			else selecty=log(float(lvl1y))/log(2.0f)+0;
		}


		static int selectz=log(float(lvl1z))/log(2.0f)+0;
		ui.doComboBox(none, 7, formatLabel, &selectz);
		if((1<<(selectz))!=lvl1z)
		{
			if(win_messagebox_confirm("Please Confirm","Clear the scene ?"))
			{
				lvl1z=1<<(selectz);
				blocks.clear();
				blocks_undo.clear();
				scene_reset=true;
			}
			else selectz=log(float(lvl1z))/log(2.0f)+0;
		}

		ui.endGroup();
		
	}

	{
		ui.beginGroup( nv::GroupFlags_GrowRightFromTop);ui.endGroup();
		ui.beginGroup( nv::GroupFlags_GrowRightFromTop);ui.endGroup();

		ui.beginGroup( nv::GroupFlags_GrowRightFromTop);
		if (ui.doButton(none, "Add as Voxel CUBE"))
		{
			add_cube_to_octree();
		}
		/*
		if (ui.doButton(none, "Make Terrain"))
		{
			make_terrain_perlin(FRAME);
		}
		
		if (ui.doButton(none, "Smooth All"))
		{
			smooth();
		}
		*/
		ui.endGroup();

		ui.beginGroup( nv::GroupFlags_GrowRightFromTop);

		const char * formatLabel[] = { "Off", "Helper Shape", "3-Way Intersect", "Mirror Tool", "Functional Brush" };
		ui.doComboBox(none, 5, formatLabel, &selected_tool);
		helper_shape_enabled=( selected_tool==1)?1:0;
		
		ui.doLabel( none, "Active Tool");

		ui.endGroup();

		if(helper_shape_enabled)
		{
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);

			if (ui.doButton(none, "Rasterize"))
			{
				helper_shape_rasterize=1;
			}
			
			{
				const char * formatLabel[] = { "Plane", "Sphere", "Box", "Cone", "Torus", "Cylinder" };
				ui.doComboBox(none, 6, formatLabel, &helper_shape_type);
			}
			
			ui.doCheckButton(none, "Lock", &helper_shape_fixed);

			ui.endGroup();
						
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				char  text[100];
				ui.doHorizontalSlider(none, 0,256, &helper_shape_pos.z);
				sprintf(text,"Distance %3.1f",helper_shape_pos.z);	
				ui.doLabel( none, text);
			}
			ui.endGroup();

			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				char  text[100];
				ui.doHorizontalSlider(none, -128,128, &helper_shape_pos.x);
				sprintf(text,"Offset X %3.1f",helper_shape_pos.x);	
				ui.doLabel( none, text);
			}
			ui.endGroup();
						ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				char  text[100];
				ui.doHorizontalSlider(none, -128,128, &helper_shape_pos.y);
				sprintf(text,"Offset Y %3.1f",helper_shape_pos.y);	
				ui.doLabel( none, text);
			}
			ui.endGroup();

			
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				char  text[100];
				ui.doHorizontalSlider(none, 0,256, &helper_shape_size.x);
				sprintf(text,"Size X %3.0f",helper_shape_size.x);	
				ui.doLabel( none, text);
			}
			ui.endGroup();
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				char  text[100];
				ui.doHorizontalSlider(none, 0,256, &helper_shape_size.y);
				sprintf(text,"Size Y %3.0f",helper_shape_size.y);	
				ui.doLabel( none, text);
			}
			ui.endGroup();
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				char  text[100];
				ui.doHorizontalSlider(none, 0,256, &helper_shape_size.z);
				sprintf(text,"Size Z %3.0f",helper_shape_size.z);	
				ui.doLabel( none, text);
			}
			ui.endGroup();

			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
	
			if (ui.doButton(none, "Reset Tool"))
			{
				helper_shape_pos=vec3f(0,0,100);
				helper_shape_fixed=1;				
				helper_shape_size=vec3f (255,255,255);
			}		
			ui.endGroup();

		}//helper_shape_enabled
	}

	ui.endGroup();

    ui.beginGroup(nv::GroupFlags_GrowUpFromRight);
	{		
		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		if (ui.doButton(none, "Snap"))
		{
			if(view_rot.x>0)view_rot.x=int((view_rot.x+22.5)/45)*45;
			if(view_rot.y>0)view_rot.y=int((view_rot.y+22.5)/45)*45;
			if(view_rot.z>0)view_rot.z=int((view_rot.z+22.5)/45)*45;
			if(view_rot.x<0)view_rot.x=int((view_rot.x-22.5)/45)*45;
			if(view_rot.y<0)view_rot.y=int((view_rot.y-22.5)/45)*45;
			if(view_rot.z<0)view_rot.z=int((view_rot.z-22.5)/45)*45;
		}

		sprintf(text,"Rotation %d %d %d",
			int(view_rot.x),
			int(view_rot.y),
			int(view_rot.z));	
		ui.doLabel( none, text);

		ui.endGroup();

		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		sprintf(text,"3D Cursor %d %d %d",
			int(brush_pos.x),
			int(brush_pos.y),
			int(brush_pos.z));	
		ui.doLabel( none, text);
		ui.endGroup();

		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		ui.doCheckButton(none, "", &render_ssao);
		ui.doLabel( none, "SSAO");
		ui.doCheckButton(none, "", &help_on);
		ui.doLabel( none, "Help Text");		
		ui.endGroup();

		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		bool render_cubes_before=render_cubes;
		ui.doCheckButton(none, "", &render_cubes);
		ui.doLabel( none, "Voxels as Cubes");		
		if(render_cubes_before!=render_cubes)
		{
			loopi(0,lvl1x*lvl1y*lvl1z) blocks[i].dirty=1;
			triangulate();
		}
		ui.endGroup();
		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		ui.doCheckButton(none, "", &cursor_rgb_cross);
		ui.doLabel( none, "XYZ Coordinate Cross");
		ui.endGroup();

		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);		
		{
			const char * formatLabel[] = { "Brush Z", "Brush Size", "Brush Intensity", "Zoom" };
			ui.doComboBox(none, 4, formatLabel, &select_wheel_function);
			ui.doLabel( none, "Mouse Wheel for");

			if(select_wheel_function==0)
			{
				if(MOUSE_DWHEEL>0)brush_z*=0.9995;
				if(MOUSE_DWHEEL<0)brush_z*=1.0005;
			}
			if(select_wheel_function==3) 
				view_pos.z+=MOUSE_DWHEEL/30;

			if(select_wheel_function==1) 
				brush_size+=MOUSE_DWHEEL*1;

			if(select_wheel_function==2) 
				brush_intensity+=MOUSE_DWHEEL*1;
			
			

		}
		ui.endGroup();
		

	}
    ui.endGroup();

    ui.beginGroup(nv::GroupFlags_GrowDownFromLeft);
	{		
		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		{
			const char * formatLabel[] = { 
				"Brush", 
				"Polyon",
				"Procedural",
			};
			ui.doComboBox(none, 3, formatLabel, &brush_tool);
		}
		ui.doCheckButton(none, "Snap Z", &brush_z_const_disabled);
		ui.endGroup();

		//brush_z_dynamic

		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		{
			const char * formatLabel[] = { "Draw", "Erase", "Grow", "Shrink", "Smear", "Stroke", "Colorize" };
			int before=brush_type;
			ui.doComboBox(none, 7, formatLabel, &brush_type);

			if(brush_tool==0 && before!=brush_type )
			{
				if(brush_type==0) brush_z_const_disabled = 0;
				if(brush_type==1) brush_z_const_disabled = 0;
				if(brush_type==2) brush_z_const_disabled = 1;
				if(brush_type==3) brush_z_const_disabled = 1;
				if(brush_type==4) brush_z_const_disabled = 0;
				if(brush_type==5) brush_z_const_disabled = 0;
				if(brush_type==6) brush_z_const_disabled = 1;
			}

			if(brush_tool==1)
			{
				if(poly.size()>0)
				{
					if (ui.doButton(none, "Rasterize"))
					{
						rasterize_poly(poly);
						poly.clear();
						poly_edit=0;
					}
					if(!poly_edit)
					if (ui.doButton(none, "Edit"))
					{
						poly_edit=true;
					}
				}
			}

		}
		ui.endGroup();

		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		{
			char  text[100];
			ui.doHorizontalSlider(none, 1,40, &brush_size);
			sprintf(text,"Size %3.0f",brush_size);	
			ui.doLabel( none, text);
		}
		ui.endGroup();

		if(brush_tool==0)
		{
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				char  text[100];
				ui.doHorizontalSlider(none, 1,255, &brush_intensity);
				sprintf(text,"Intensity %3.0f",brush_intensity);	
				ui.doLabel( none, text);
			}
			ui.endGroup();	

			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				char  text[100];
				ui.doHorizontalSlider(none, 0,3, &brush_color_specular);
				sprintf(text,"Specular %d",uint(brush_color_specular));	
				ui.doLabel( none, text);
			}
			ui.endGroup();	

			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				char  text[100];
				ui.doHorizontalSlider(none, 0,100, &brush_color_random);
				sprintf(text,"Color Random %d",uint(brush_color_random));	
				ui.doLabel( none, text);
			}
			ui.endGroup();	
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				char  text[100];
				ui.doHorizontalSlider(none, 0,100, &brush_intensity_random);
				sprintf(text,"Intensity Rnd %d",uint(brush_intensity_random));	
				ui.doLabel( none, text);
			}
			ui.endGroup();

			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				char  text[100];
				ui.doHorizontalSlider(none, 0,100, &brush_rate);
				sprintf(text,"Rate %d",uint(brush_rate));	
				ui.doLabel( none, text);
			}
			ui.endGroup();	

			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				const char * formatLabel[] = { "No Color Effect", "Audio to Color", "Inverse Audio", "Flashing 1", "Flashing 2", "Pulsating"};
				ui.doComboBox(none, 6, formatLabel, &brush_color_effect);
			}
			ui.endGroup();	

			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			{
				const char * formatLabel[] = { "No Texture", "Texture 1", "Texture 2", "Texture 3" };
				ui.doComboBox(none, 4, formatLabel, &brush_color_texture);
			}
			ui.endGroup();	

			//brush_color_specular
		
			if(brush_type==5) 
			{
				ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
				{
					char  text[100];
					ui.doHorizontalSlider(none, 1,255, &brush_stroke_length);
					sprintf(text,"Length %3.0f",brush_stroke_length);	
					ui.doLabel( none, text);
				}
				ui.endGroup();
			}
		}
		

		
		//
		if(brush_tool==0)
		{
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);			
			ui.doCheckButton(none, "X", &brush_const_x);
			ui.doCheckButton(none, "Y", &brush_const_y);
			//ui.doCheckButton(none, "Z", &brush_const_z);
			ui.doLabel( none, "Fixed (Screen)");
			ui.endGroup();		
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);			
			ui.doCheckButton(none, "X", &brush_loop_x);
			ui.doCheckButton(none, "Y", &brush_loop_y);
			ui.doCheckButton(none, "Z", &brush_loop_z);
			ui.doLabel( none, "Loop");
			ui.endGroup();		
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);			
			ui.doCheckButton(none, "X", &brush_mirror_x);
			ui.doCheckButton(none, "Y", &brush_mirror_y);
			ui.doCheckButton(none, "Z", &brush_mirror_z);
			ui.doLabel( none, "Mirror");
			ui.endGroup();
		}
		if(brush_tool==1)
		{
			char  text[100];

			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			ui.doCheckButton(none, "Outline", &brush_mirror_x);
			ui.endGroup();
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			ui.doHorizontalSlider(none, 1,255, &brush_intensity);
			sprintf(text,"%3.0f Samples",brush_intensity);	
			ui.doLabel( none, text);
			ui.endGroup();

			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			ui.doCheckButton(none, "Fill", &brush_mirror_x);
			ui.endGroup();
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
			ui.doHorizontalSlider(none, 1,255, &brush_intensity);
			sprintf(text,"%3.0f Samples",brush_intensity);	
			ui.doLabel( none, text);
			ui.endGroup();
		}
	}
	ui.endGroup();

		//
		
		//

    // Pass non-ui mouse events to the manipulator
	//updateManipulator(ui, manipulator);
    ui.end();

	brush_size=max(min(brush_size,40),1);
}

// Menu out

