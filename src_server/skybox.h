namespace skybox
{

void Draw(vec3f rot,int type=2)
{
	if(type==0)
	{
		glClearColor(0.5,0.5,1.0,1.0);
		glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
	}
	if(type==1)
	{
		static int rasterbartex=ogl_tex_bmp("../data/rasterbars/1.png");
		ogl_bind(0,rasterbartex);
		glMatrixMode( GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		float ofs=0.5+1.2*rot.x/M_PI;
		ogl_drawquad(-1,-1,1,1,1,ofs+0.2,0,ofs-0.2);
		ogl_bind(0,0);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		return;
	}		
	if(type==2)
	{
		glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glRotatef(rot.x*180/M_PI,1,0,0);
		glRotatef(rot.y*180/M_PI,0,1,0);

		uint   time_zero=timeGetTime();
		double time_elapsed=0;

		//time
		time_elapsed=float(timeGetTime()-time_zero)/1000.0;

		glDisable(GL_DEPTH_TEST);
		glCullFace(GL_FRONT);
		glScalef(10,10,10);

		static Mesh sky_box(
			"../data/skybox/ogre.material",
			"../data/skybox/ogre.mesh.xml"	);

		sky_box.Draw(vec3f(0,0,0),vec3f(M_PI,0,0));

		glCullFace(GL_BACK);

		// moon

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
	
		static Mesh ogremoon(
			"../data/moon/ogre.material",
			"../data/moon/ogre.mesh.xml");

		//ogremoon.Draw(vec3f(1.5,1.0, 1.0),vec3f(0,time_elapsed*0.01,0));
		//ogremoon.Draw(vec3f(0,  3.0,-4.0),vec3f(0,time_elapsed*0.02,0));

		glScalef(1.0/10,1.0/10,1.0/10);
	
		glDisable(GL_BLEND);
		glPopMatrix();
	}
}

} // namespace

