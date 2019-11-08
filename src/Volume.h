///////////////////////////////////////////
class Volume
{
	public:
	GLuint volume_texture;

	// create a test volume texture, here you could load your own volume
	Volume(
		int VOLUME_TEX_SIZE_X=256,
		int VOLUME_TEX_SIZE_Y=256,
		int VOLUME_TEX_SIZE_Z=256
		)
	{
		int size = 
			VOLUME_TEX_SIZE_X *
			VOLUME_TEX_SIZE_Y * 
			VOLUME_TEX_SIZE_Z ;
		GLubyte *data = new GLubyte[size];

		int ofs=0;
		loopijk(0,0,0,VOLUME_TEX_SIZE_X,VOLUME_TEX_SIZE_Y,VOLUME_TEX_SIZE_Z)
			data[ofs++]=i^j^k;//sample_volume(vec3f(i/2,j/2,k/2)).p/10;//i^j^k;

		cout << "creating terrain volume texture " << (int)(size/1000000) << "MB" << endl;
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
		glGenTextures(1, &volume_texture);
		glBindTexture(GL_TEXTURE_3D, volume_texture);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
		glTexImage3D(GL_TEXTURE_3D, 0,GL_LUMINANCE, 
			VOLUME_TEX_SIZE_X, 
			VOLUME_TEX_SIZE_Y,
			VOLUME_TEX_SIZE_Z,
			0, GL_LUMINANCE, GL_UNSIGNED_BYTE,data);
		ogl_check_error();

		delete []data;
		cout << "terrain volume texture creation finished" << endl;
	}
	// this method is used to draw the front and backside of the volume
	void DrawCube()
	{	
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, volume_texture);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		static Shader shader("../shader/volume");
		matrix44 m;
		glGetFloatv(GL_MODELVIEW_MATRIX,  &m.m[0][0]);
		matrix44 mi = m; mi.invert();	
		shader.begin();	
		shader.setUniform1i("volumetexture",0);	
		shader.setUniform3f("xform0",mi.m[0][0] ,mi.m[1][0],mi.m[2][0]);	
		shader.setUniform3f("xform1",mi.m[0][1] ,mi.m[1][1],mi.m[2][1]);	
		shader.setUniform3f("xform2",mi.m[0][2] ,mi.m[1][2],mi.m[2][2]);
		vector4 rel_pos=mi*vector4 (0,0,0,1.0);
		float x_sign=1;
	
		if(fabs(rel_pos.x)<0.5 &&
		   fabs(rel_pos.y)<0.5 &&
		   fabs(rel_pos.z)<0.5)
		{
			shader.setUniform1f("insideCube",1);	
			x_sign=-1;
		}
		else
		{
			shader.setUniform1f("insideCube",-1);	
			x_sign=1;
		}
	
		glBegin(GL_QUADS);

		for (int sides=0;sides<6;sides++)
		{
			float xa[4]={-1,1,1,-1};
			float ya[4]={-1,-1,1,1};
			float za[4]={ 1, 1, 1, 1};

			for (int i=0;i<4;i++)
			{
				float p[3];
				if (sides==0){ 	p[0]= xa[i];p[1]=ya[i];p[2]= za[i]; }			
				if (sides==1){ 	p[0]=-xa[i];p[1]=ya[i];p[2]=-za[i]; }			
				if (sides==2){ 	p[0]=-xa[i];p[2]=ya[i];p[1]= za[i]; }			
				if (sides==3){ 	p[0]= xa[i];p[2]=ya[i];p[1]=-za[i]; }			
				if (sides==4){ 	p[2]=-xa[i];p[1]=ya[i];p[0]= za[i]; }			
				if (sides==5){ 	p[2]= xa[i];p[1]=ya[i];p[0]=-za[i]; }			

				float x=p[0]*0.5*x_sign;
				float y=p[1]*0.5;
				float z=p[2]*0.5;

				vector4 xf=vector4(x,y,z,0);
				glMultiTexCoord3fARB(GL_TEXTURE0_ARB, x, y, z);
				glVertex3f(xf.x,xf.y,xf.z);
			}	
		}
		glEnd();
		glDisable(GL_TEXTURE_3D);
	
		glDisable(GL_BLEND);
		return;
	}
};
