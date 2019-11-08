#pragma once
#include "MeshAnimation.h"

//##################################################################//
// Ogre XML Mesh Class
//##################################################################//

class Mesh { 

public:
	// ---------------------------------------- //
	// Structures
	// ---------------------------------------- //
	struct Triangle
	{
		int index[3];	// vertex    ID
		int material;	// material  ID
	};
	// ---------------------------------------- //
	class Vertex
	{
		public: 

		// general
		vec3f position;
		vec3f texcoord;
		vec3f normal;
		vec4f tangent;

		// skinning information
		float weight[3];
		int boneindex[3],count;

		// initialization
		Vertex()
		{
			count=0;
			weight[0]=weight[1]=weight[2]=0;
			boneindex[0]=boneindex[1]=boneindex[2]=0;
		};
	};
	// ---------------------------------------- //
	class Texture
	{
		public:
		Texture(){envmap=0;gl_handle=-1;}

		std::string filename;
		int gl_handle;
		bool envmap;
	};
	// ---------------------------------------- //
	class Material
	{
		public:

		Material()
		{
			ambient = vec3f ( 0.6 , 0.3 ,0 );
			diffuse = vec3f ( 0.3 , 0.3 ,0.3 );
			specular= vec3f ( 0,0,0 );
			emissive= vec3f ( 0,0,0 );
			diffuse_map.gl_handle=-1;
			emissive_map.gl_handle=-1;
			ambient_map.gl_handle=-1;
			alpha = 1;
			reflect = 0;
			name = "";
		}
		std::string name;
		vec3f diffuse;
		vec3f specular;
		vec3f ambient;
		vec3f emissive;
		float alpha,reflect;
		Texture diffuse_map;
		Texture emissive_map;
		Texture ambient_map;
		Texture bump_map;
	};
	// ---------------------------------------- //
	struct DisplayList // GL displaylist for rendering
	{ 
		int list_id;
		int material;
	};
	// ---------------------------------------- //
	struct LOD // level of detail mesh
	{
		std::vector<Triangle>		triangles;
		std::vector<DisplayList>	displaylists;
	};	
	// ---------------------------------------- //
	// Main OBJ Part
	// ---------------------------------------- //
	bool					skinning;	// skinning on ?
	std::vector<LOD>		lod_meshes;	// triangles & displaylists for all LODs
	std::vector<Vertex>		vertices;	// vertex list
	std::vector<Material>	materials;	// material list
	MeshAnimation			animation;	// animation tracks + skeleton
	// ---------------------------------------- //
	// Constructor / Destructor
	// ---------------------------------------- //
	Mesh(){};
	Mesh(char* ogrematerialfilename, char* ogremeshfilenamexml, char* ogreskeletonfilenamexml=0)
	{
		printf("\n\n");
		printf("################################################\n");
		printf("## \n");
		printf("## Loading Mesh %s \n",ogremeshfilenamexml);
		printf("## \n");
		printf("################################################\n\n",ogremeshfilenamexml);

		if(ogreskeletonfilenamexml) animation.LoadSkeletonXML(ogreskeletonfilenamexml);
		if(ogreskeletonfilenamexml) skinning=1;

		LoadOgreXML(ogrematerialfilename,ogremeshfilenamexml);

		printf("\n\n");
	}
	~Mesh(){}
	// ---------------------------------------- //
	void InitDisplaylists()
	{
		loopk(0,lod_meshes.size())
		{
			std::vector<Triangle>		&triangles=lod_meshes[k].triangles;
			std::vector<DisplayList>	&displaylists=lod_meshes[k].displaylists;

			DisplayList dl;	int mtl=0;

			loopi(0,triangles.size())
			{
				Triangle &t=triangles[i];

				if(mtl!=t.material || i==0)
				{
					if(i>0)
					{
						glEnd();
						glEndList();
						displaylists.push_back(dl);
					}
					dl.material= mtl=t.material;
					dl.list_id = glGenLists(1);
					glNewList(dl.list_id, GL_COMPILE);
					glBegin(GL_TRIANGLES);
				}
				
				// Triangle vertices
				loopj(0,3)
				{
					Vertex v=vertices[t.index[j]];

					// texture uv + packed tangent
					vec3f tc=v.texcoord;
					float tx=int((v.tangent.x*0.5+0.5)*255.0);
					float ty=int((v.tangent.y*0.5+0.5)*255.0);
					float tz=int((v.tangent.z*0.5+0.5)*255.0);
					tc.z=(tx+ty/256.0+tz/65536.0)*v.tangent.w; // w is 1 or -1
					glTexCoord3f(tc.x,1.0-tc.y,tc.z);

					// skinning
					float packedboneids=0.0;
					if(skinning) // skinning on
					{
						glColor3f(v.weight[0],v.weight[1],v.weight[2]);
						packedboneids=	float(v.boneindex[0]) +
										float(v.boneindex[1])/ 128.0 +
										float(v.boneindex[2])/(128.0*128.0);
					}
					// normal
					vec3f n=v.normal;
					n.normalize();
					glNormal3f(n.x,n.y,n.z);

					// position + packed bone ids
					glVertex4f(v.position.x,v.position.y,v.position.z,packedboneids);
				}
			}
			glEnd();
			glEndList();
			displaylists.push_back(dl);

			// save memory	
			std::vector<Triangle>().swap(triangles);
		}
		// save memory					
		std::vector<Vertex>().swap(vertices);
	};
	// -------------- materials ------------ //
	int GetMaterialIndex ( std::string name )
	{
		loopi(0,materials.size() )
			if ( name.compare( materials[i].name ) == 0 ) return i;

		printf("couldnt find material %s\n",name.c_str() );
		return -1;
	}
	void PrintMaterials ()
	{
		printf("\n");
		for(uint i=0;i<materials.size();i++)
		{
			printf("Material %i : %s\n",i,materials[i].name.c_str());
			printf("  Ambient RGB %2.2f %2.2f %2.2f\n",	materials[i].ambient.x,materials[i].ambient.y,materials[i].ambient.z);			
			printf("  Specular RGB %2.2f %2.2f %2.2f\n",materials[i].specular.x,materials[i].specular.y,materials[i].specular.z);
			printf("  Emissive RGB %2.2f %2.2f %2.2f\n",materials[i].emissive.x,materials[i].emissive.y,materials[i].emissive.z);			
			printf("  Diffuse RGB %2.2f %2.2f %2.2f\n",	materials[i].diffuse.x,	materials[i].diffuse.y,	materials[i].diffuse.z);
			printf("  Diff. Tex : %s [Env %d]\n",materials[i].diffuse_map.filename.c_str(),materials[i].diffuse_map.envmap	);
			printf("  Amb . Tex : %s \n",materials[i].ambient_map.filename.c_str()	);
			printf("  Bump  Tex : %s \n",materials[i].bump_map.filename.c_str()	);
			printf("  Alpha %2.2f\n", materials[i].alpha);
		}
		printf("\n");
	}
	// ----------------------------- //
	void Draw(vec3f pos=vec3f(0,0,0),vec3f rot=vec3f(0,0,0),int lod=0,bool drawskeleton=0)
	{
		if(lod>=lod_meshes.size()) return;
		std::vector<DisplayList> &displaylists=lod_meshes[lod].displaylists;


		matrix44 mat;
		glGetFloatv(GL_MODELVIEW_MATRIX,  &mat.m[0][0]);
		vec4f lightvec_objspc=mat*vec4f(sunlightvec.x,sunlightvec.y,sunlightvec.z,0);
		lightvec_objspc.norm();

		glPushMatrix();		
		glTranslatef(pos.x,pos.y,pos.z);
		glRotatef(rot.x*360/(2*M_PI),1,0,0);
		glRotatef(rot.y*360/(2*M_PI),0,1,0);
		glRotatef(rot.z*360/(2*M_PI),0,0,1);

		if(drawskeleton)
		{
			for (int i = 0; i < animation.bones.size(); i++)
			{
				glPushMatrix();
				glColor3f(1,1,1);
				glMultMatrixf((float*)animation.bones[i].matrix.m);
				glutSolidCube(0.3);
				glPopMatrix();
			}
			glPopMatrix();
			return;
		}
		
		if(lod_meshes[0].displaylists.size()==0) InitDisplaylists();

		
		static int tex_default=ogl_tex_bmp("../data/white.png");
		static int tex_default_bump=ogl_tex_bmp("../data/grey.png");	
		static Shader tri_shader ("../shader/mesh");

		tri_shader.begin();
		if(skinning) 
		{
			// skinning enabled
			matrix44 bones[100];
			//if(animation.bones.size()>50)MessageBoxA(0,"bones g 100","",0);
			loopi(0,animation.bones.size())	bones[i]=animation.bones[i].invbindmatrix*animation.bones[i].matrix;
			tri_shader.setUniformMatrix4fv("bones",animation.bones.size(),&bones[0].m[0][0]);
			tri_shader.setUniform1i("use_skinning",1);
		}
		else
		{
			// skinning disabled
			tri_shader.setUniform1i("use_skinning",0);
		}

		tri_shader.setUniform4f("lightvec",lightvec_objspc.x,lightvec_objspc.y,lightvec_objspc.z,sunlightvec.w);

		loopi(0,displaylists.size())
		{
			DisplayList dl=displaylists[i];			
			Material &mtl=materials[dl.material];
			vec3f di=mtl.diffuse;
			vec3f am=mtl.ambient+mtl.emissive;
			vec3f sp=mtl.specular;
			int tex_dif = (mtl.diffuse_map.gl_handle<0) ? tex_default:mtl.diffuse_map.gl_handle;
			int tex_amb = (mtl.ambient_map.gl_handle<0) ? tex_default:mtl.ambient_map.gl_handle;
			int tex_bump= (mtl.bump_map.gl_handle<0)	? tex_default_bump:mtl.bump_map.gl_handle;
			tri_shader.setUniform4f("ambient",am.x,am.y,am.z,0);
			tri_shader.setUniform4f("diffuse",di.x,di.y,di.z,0);
			tri_shader.setUniform4f("specular",sp.x,sp.y,sp.z,0);
			tri_shader.setUniform1i("texDiffuse",0);
			tri_shader.setUniform1i("texAmbient",1);
			tri_shader.setUniform1i("texBump"	,2);
			tri_shader.setUniform1f("dif_envmap", mtl.diffuse_map.envmap ? 1 : 0 );
			ogl_bind(2,tex_bump);
			ogl_bind(1,tex_amb);
			ogl_bind(0,tex_dif);				
			glCallList(dl.list_id);
		}
		tri_shader.end();
		loopi(0,3) ogl_bind(2-i,tex_default);
		glPopMatrix();
		ogl_check_error();
	}
	// ---------- Load ogre Obj ------------ //
	void LoadOgreXML(char* name_material, char* name_mesh)
	{
		// materials
		FILE* fn=fopen(name_material, "rb");
		if (!fn) error_stop ( "File %s not found!\n" ,name_material );

		char line[1000],name[1000];
		memset ( line,0,1000 );
		memset ( name,0,1000 );
		Material mat,empty_mat;
	
		while(fgets( line, 1000, fn ) != NULL)
		{	
			if(sscanf(line,"material %s",name )==1){materials.push_back(mat);mat=empty_mat;mat.name=name;}
			sscanf(line," ambient %f %f %f",&mat.ambient.x,&mat.ambient.y,&mat.ambient.z );
			sscanf(line," diffuse %f %f %f",&mat.diffuse.x,&mat.diffuse.y,&mat.diffuse.z );
			sscanf(line," specular %f %f %f",&mat.specular.x,&mat.specular.y,&mat.specular.z );
			sscanf(line," emissive %f %f %f",&mat.emissive.x,&mat.emissive.y,&mat.emissive.z );
			if(sscanf(line," texture %s",name )==1)
			{
				Texture *texture=&mat.diffuse_map; bool bump=0;
				if(strcmp(name,"_unit")==0)continue;
				if(strcmp(name,"_bump")==0)
				{
					bump=1;
					sscanf(line," texture_bump %s",name );
					texture=&mat.bump_map;
				}
				if(strcmp(name,"_ambient")==0)
				{
					sscanf(line," texture_ambient %s",name );
					texture=&mat.ambient_map;
				}			
				texture->filename = get_path( name_material ) + std::string(name);
				Bmp bmp(texture->filename.c_str());
				if(bump) bmp.MakeBump();
				texture->gl_handle = ogl_tex_bmp(bmp,GL_LINEAR_MIPMAP_LINEAR,GL_REPEAT);
			}
			if(sscanf(line," env_map %s",name )==1)
			{
				if(strcmp(name,"spherical")==0)	mat.diffuse_map.envmap=1;
			}
		}
		materials.push_back(mat);
		PrintMaterials();

		// mesh
		fn=fopen(name_mesh, "rb");
		if (!fn){printf ( "File %s not found!\n" ,name_mesh );while(1);}

		memset ( line,0,1000 );
		memset ( name,0,1000 );

		Triangle tri;vec3f v;int a1,a2;float w;
		tri.material=0;

		int offset=0;

		std::vector<Triangle> triangles;
		std::vector<int>  lodmeshindex_offset;	// for LOD
		std::vector<int>  lodmeshindex_material;// for LOD		

		int normal_id=0,texcoord_id=0,tangent_id=0;
	
		while(fgets( line, 1000, fn ) != NULL)
		{		      
			if(sscanf(line," <face v1=\"%d\" v2=\"%d\" v3=\"%d\" />",&tri.index[0],&tri.index[1],&tri.index[2] )==3)
			{
				loopi(0,3) tri.index[i]+=offset;
				triangles.push_back(tri);
			}
			if(sscanf(line," <position x=\"%f\" y=\"%f\" z=\"%f\" />",&v.x,&v.y,&v.z )==3){Vertex a;a.position=v;vertices.push_back(a);}
			if(sscanf(line," <normal x=\"%f\" y=\"%f\" z=\"%f\" />",&v.x,&v.y,&v.z )==3) vertices[normal_id++].normal=v;
			if(sscanf(line," <texcoord u=\"%f\" v=\"%f\" />",&v.x,&v.y )==2)  vertices[texcoord_id++].texcoord=v;
			if(sscanf(line," <tangent x=\"%f\" y=\"%f\" z=\"%f\" w=\"%f\"",&v.x,&v.y,&v.z,&w )==4)vertices[tangent_id++].tangent=vec4f(v.x,v.y,v.z,w);
			if(sscanf(line," <vertexboneassignment vertexindex=\"%d\" boneindex=\"%d\" weight=\"%f\" />",&a1,&a2,&w )==3)
			{
				a1+=offset;
				if(a1>=vertices.size()) error_stop("vertexboneassignment %d (%d) >= %d",a1,a1-offset,vertices.size());

				int i=vertices[a1].count;
				if(i<3)
				{
					vertices[a1].weight[i]=w;
					vertices[a1].boneindex[i]=a2;
					vertices[a1].count++;
				}
			}
			if(sscanf(line,"        <submesh material=\"%s\" usesharedvertices",name )==1)
			{
				offset=vertices.size();
				name[strlen(name)-1]=0;
				tri.material=GetMaterialIndex ( name );
				lodmeshindex_offset.push_back(offset);
				lodmeshindex_material.push_back(tri.material);
			}	
			if(sscanf(line," <lodfacelist submeshindex=\"%d\" numfaces=",&a1)==1)
			{
				if(a1<0 || a1>=lodmeshindex_offset.size()) error_stop("submeshindex %d >= %d",a1,lodmeshindex_offset.size());
				offset=lodmeshindex_offset[a1];
				tri.material=lodmeshindex_material[a1];
			}
			if(sscanf(line," <lodgenerated value=\"%f\">",&w)==1)
			{
				LOD l;l.triangles=triangles;
				lod_meshes.push_back(l);
				triangles.clear();
			} 
		}	

		if(triangles.size()>0)
		{
			LOD l;l.triangles=triangles;
			lod_meshes.push_back(l);
		}

		loopi(0,lod_meshes.size()) printf("LOD %d Triangles %d\n",i,lod_meshes[i].triangles.size());
		printf("\n");
	}
	// ---------------------------------------- //
};
