#include "xml/tinyxml.h"
#include <stdio.h>

//##################################################################//
// Ogre XML-Animation File Reader
//##################################################################//

class MeshAnimation
{
	public:

	enum {  NAME_LEN = 30 };
	
	typedef struct
	{	
		float	time;
		float	rot[4]; // angle,x,y,z
		float	pos[3];
	} TKey;

	typedef struct
	{	
		std::vector<TKey> keys;
	} TTrack;

	typedef struct
	{
		char				nameLength;
		char				name[NAME_LEN];
		float				timeLength;
		std::vector<TTrack>	tracks;
		int					frameCount;
	} TAnimation;
	
	typedef struct
	{	
		char			nameLength;
		char			name[NAME_LEN];
		float			rot[4]; // angle,x,y,z
		float			pos[3];
		int				parent;
		matrix44		matrix; // animated result
		matrix44		invbindmatrix;  // inverse bindmatrix
		std::vector<int> childs;
	} TBone;

	std::vector<TBone>		bones;		
	std::vector<TAnimation>	animations;
	
	MeshAnimation(char* skeletonfilename)
	{
		LoadSkeletonXML (skeletonfilename);
	}
	MeshAnimation(){};

	void  LoadSkeletonXML ( char* ogreXMLfileName );
	int   GetBoneIndexOf ( char* name );
	int   GetAnimationIndexOf  (char* name);
	void  SetPose(int animation,double time);
	void  SetBindPose();
	void  EvalSubtree(int boneid,TAnimation &ani,int frame,float weight);
	TKey& GetInterpolatedKey(TTrack &t,int frame,float weight,bool normalize=false);
	void  ResampleAnimationTracks(double frames_per_second);
};

MeshAnimation::TKey& MeshAnimation::GetInterpolatedKey(TTrack &t,int frame,float weight,bool normalize)
{
	TKey &k0=t.keys[(frame  ) % t.keys.size() ];
	TKey &k1=t.keys[(frame+1) % t.keys.size() ];

	static TKey k;float weight1=1.0-weight;
	loopi(0,3) k.pos[i]=k0.pos[i]*weight1+k1.pos[i]*weight;
	loopi(0,4) k.rot[i]=k0.rot[i]*weight1+k1.rot[i]*weight;

	if(normalize)
	{
		vec3f axis(k.rot[1],k.rot[2],k.rot[3]);
		axis.norm();
		k.rot[1]=axis.x;k.rot[2]=axis.y;k.rot[3]=axis.z;
	}
	return k;
}

void MeshAnimation::EvalSubtree(int id,TAnimation &ani,int frame, float weight=0)
{
	TBone &b=bones[id];	matrix44 a,m,minv;
	
	// bind pose : default
	vec3f pos(b.pos[0],b.pos[1],b.pos[2]);
	m.set(b.rot[0],b.rot[1],b.rot[2],b.rot[3]);		
	
	if(ani.tracks[id].keys.size()>frame) // add animated pose if track available
	if(frame>=0)
	{	
		TKey &k=GetInterpolatedKey(ani.tracks[id],frame,weight);
		a.set(k.rot[0],k.rot[1],k.rot[2],k.rot[3]);		
		pos=pos+vec3f(k.pos[0],k.pos[1],k.pos[2]);
		m=a*m;
	}
	m.set_translation(pos);
		
	// store bone matrix
	if(b.parent>=0) b.matrix=m*bones[b.parent].matrix; else b.matrix=m; 

	// progress through tree
	loopi(0,b.childs.size()) EvalSubtree(b.childs[i],ani,frame,weight);
}

void MeshAnimation::SetPose(int animation_index,double time)
{
	if(animation_index>=animations.size()) error_stop("animation index %d out of range",animation_index);

	TAnimation &ani=animations[animation_index];
	double time01=time/double(ani.timeLength);
	time01=time01-floor(time01);
	float frame=(ani.frameCount-2)*time01+1;
	
	for (int i = 0; i < bones.size(); i++) bones[i].matrix.ident();
	for (int i = 0; i < bones.size(); i++) if (bones[i].parent==-1) EvalSubtree(i,ani,int(frame),frac(frame));
}

void MeshAnimation::SetBindPose()
{
	TAnimation &ani=animations[0];	
	loopi(0,bones.size()) bones[i].matrix.ident();
	loopi(0,bones.size()) if (bones[i].parent==-1) EvalSubtree(i,ani,-1,0);
	loopi(0,bones.size()) bones[i].invbindmatrix=bones[i].matrix;
	loopi(0,bones.size()) bones[i].invbindmatrix.invert_simpler();
}
	
int MeshAnimation::GetAnimationIndexOf ( char* name )
{
	loopi(0,animations.size()) if (strcmp(name, animations[i].name)==0) return i;
	error_stop("animation %s not found!",name);
}

int MeshAnimation::GetBoneIndexOf ( char* name )
{
	loopi(0,bones.size()) if (strcmp(name,bones[i].name)==0) return i;
	error_stop( "Error! Bone [%s] does not exist!" ,name );
}
void MeshAnimation::ResampleAnimationTracks(double frames_per_second)
{
	loopi(0,animations.size())
	loopj(0,animations[i].tracks.size())
	if(animations[i].tracks[j].keys.size()>0)
	{
		TTrack dst; 
		TTrack &src=animations[i].tracks[j];
		double length=animations[i].timeLength;
		int newframecount=length*frames_per_second;
		int src_frame=0;
		//printf("src[%d]=%d frames\n",j,src.keys.size());

		loopk(0,newframecount)
		{
			double time=k*length/double(newframecount-1);
			while(src_frame<src.keys.size() && time>src.keys[src_frame].time ) src_frame++;
			
			int src_frame_1 = clamp ( src_frame-1 ,0,src.keys.size()-1);
			int src_frame_2 = clamp ( src_frame   ,0,src.keys.size()-1);

			float t1=src.keys[src_frame_1].time;
			float t2=src.keys[src_frame_2].time;
			float w= (time-t1)/(t2-t1);

			TKey key=GetInterpolatedKey(src,src_frame_1,w,true);
			dst.keys.push_back(key);
		}
		animations[i].tracks[j]=dst;
		animations[i].frameCount=newframecount;
		//printf("dst[%d]=%d frames\n\n",j,dst.keys.size());
	}
}
void MeshAnimation::LoadSkeletonXML (char* ogreXMLfileName)
{
	animations.clear();

	TiXmlDocument doc( ogreXMLfileName );
	if ( !doc.LoadFile() ) error_stop( "File %s load error %s\n", ogreXMLfileName, doc.ErrorDesc() );

	TiXmlNode* node = 0;
	TiXmlElement* skeletonNode = 0;
	TiXmlElement* animationsElement = 0;
	TiXmlElement* animationElement = 0;
	TiXmlElement* bonesElement = 0;
	TiXmlElement* boneElement = 0;
	TiXmlElement* boneHierarchyElement = 0;
	TiXmlElement* boneParentElement = 0;

	node = doc.FirstChild( "skeleton" ); if(!node) error_stop("node ptr 0");
	skeletonNode = node->ToElement(); if(!skeletonNode) error_stop("skeletonNode ptr 0");
	
	bonesElement = skeletonNode->FirstChildElement( "bones" );assert( bonesElement );
	
	for(boneElement = bonesElement->FirstChildElement( "bone" );boneElement;
		boneElement = boneElement->NextSiblingElement( "bone" ) )
	{
		int result=0;
		const char* cBoneName;
		double dBoneID = 0,dPosX = 0,dPosY = 0,dPosZ = 0;
		double dAxisAngle = 0,dAxisX = 0,dAxisY = 0,dAxisZ = 0;

		result+= boneElement->QueryDoubleAttribute( "id", &dBoneID );
		cBoneName = boneElement->Attribute( "name" );
		
		TiXmlElement *positionElement = 0,*rotateElement = 0,*axisElement = 0;
		positionElement = boneElement->FirstChildElement( "position" );assert( positionElement );
		rotateElement = boneElement->FirstChildElement( "rotation" );assert( rotateElement );
		axisElement = rotateElement->FirstChildElement( "axis" );assert( axisElement );
		result+= positionElement->QueryDoubleAttribute( "x", &dPosX );
		result+= positionElement->QueryDoubleAttribute( "y", &dPosY );
		result+= positionElement->QueryDoubleAttribute( "z", &dPosZ );
		result+= rotateElement->QueryDoubleAttribute( "angle", &dAxisAngle );
		result+= axisElement->QueryDoubleAttribute( "x", &dAxisX );
		result+= axisElement->QueryDoubleAttribute( "y", &dAxisY );
		result+= axisElement->QueryDoubleAttribute( "z", &dAxisZ );
		assert( result == 0 );
		
		TBone bone;
		sprintf( bone.name ,"%s", cBoneName);
		bone.nameLength = strnlen(bone.name,NAME_LEN);
		bone.rot[0]    = dAxisAngle;
		bone.rot[1]    = dAxisX;
		bone.rot[2]    = dAxisY;
		bone.rot[3]    = dAxisZ;
		bone.pos[0]    = dPosX;
		bone.pos[1]    = dPosY;
		bone.pos[2]    = dPosZ;
		bone.parent			= -1;		
		bones.push_back(bone);		
		//printf ( "Bone %03d %s\n" , (int)dBoneID , cBoneName ) ;
	}	
	//printf ("\nBone Hierarchy\n" );
	
	boneHierarchyElement = skeletonNode->FirstChildElement( "bonehierarchy" );
	assert( boneHierarchyElement );
	
	for(boneParentElement = boneHierarchyElement->FirstChildElement( "boneparent" );boneParentElement;
		boneParentElement = boneParentElement->NextSiblingElement( "boneparent" ) )
	{
		const char* cBoneName;
		const char* cBoneParentName;
		cBoneName = boneParentElement->Attribute( "bone" );
		cBoneParentName = boneParentElement->Attribute( "parent" );
		int cBoneIndex		= GetBoneIndexOf((char*)cBoneName);
		int cBoneParentIndex	= GetBoneIndexOf((char*)cBoneParentName);		//printf ( "Bone[%s,%d] -> Parent[%s,%d]\n" , cBoneName , cBoneIndex, cBoneParentName , cBoneParentIndex ) ;
		bones[ cBoneIndex ].parent = cBoneParentIndex;
	}

	// build hierarchy in
	for (int i = 0; i < bones.size(); i++)
	{
		int p=bones[i].parent;
		if(p>=0) bones[p].childs.push_back(i);	
	}

	// build hierarchy out
	animationsElement = skeletonNode->FirstChildElement( "animations" );assert( animationsElement );
	
	for( animationElement = animationsElement->FirstChildElement( "animation" ); animationElement;
		 animationElement = animationElement->NextSiblingElement( "animation" ) )
	{
		int result;
		double dAnimationLength=0;
		const char* cAnimationName;
		cAnimationName = animationElement->Attribute( "name" );
		result = animationElement->QueryDoubleAttribute( "length", &dAnimationLength );
		printf ( "Animation[%d] Name:[%s] , Length: %3.03f sec \n" ,animations.size(), cAnimationName , (float) dAnimationLength ) ;
		
		// --- Fill Memory Begin ---//
		
		TAnimation animation;
		animation.frameCount=0;
		sprintf( animation.name ,"%s", cAnimationName);
		animation.nameLength = strnlen(animation.name,NAME_LEN);
		animation.timeLength = dAnimationLength;
		std::vector<TTrack> &tracks = animation.tracks;
		tracks.resize(bones.size());
	
		// --- Fill Memory End ---//
		
		TiXmlElement *tracksElement = 0,*trackElement = 0;
		tracksElement = animationElement->FirstChildElement( "tracks" );
		assert( tracksElement );

		for( trackElement = tracksElement->FirstChildElement( "track" ); trackElement;
			 trackElement = trackElement->NextSiblingElement( "track" ) )
		{
			const char* cBoneName;
			cBoneName = trackElement->Attribute( "bone" );
			//printf ( "\n   Bone Name:[%s]\n\n" , cBoneName ) ;
			
			// --- Fill Memory Begin ---//
			int trackIndex = GetBoneIndexOf((char*)cBoneName);
			TTrack &track = tracks[ trackIndex ];
			//sprintf( track.name ,"%s", cBoneName);
			// --- Fill Memory End ---//
			
			TiXmlElement* keyframesElement = 0, *keyframeElement = 0;
			keyframesElement = trackElement->FirstChildElement( "keyframes" );
			assert( keyframesElement );

			for( keyframeElement = keyframesElement->FirstChildElement( "keyframe" ); keyframeElement;
				 keyframeElement = keyframeElement->NextSiblingElement( "keyframe" ) )
			{
				int result;
				double dKeyTime;
				result = keyframeElement->QueryDoubleAttribute( "time", &dKeyTime );
				//printf ( "     Keyframe Time: %3.3f  < Anination: %s , Bone: %s >\n" , 
				//	(float)dKeyTime , cAnimationName, cBoneName ) ;

				double dTranslateX = 0, dTranslateY = 0,dTranslateZ = 0;
				double dAxisAngle = 0,dAxisX = 0,dAxisY = 0,dAxisZ = 0;
				TiXmlElement *translateElement = 0,*rotateElement = 0,*axisElement = 0;
				translateElement = keyframeElement->FirstChildElement( "translate" );assert( translateElement );
				rotateElement = keyframeElement->FirstChildElement( "rotate" );assert( rotateElement );
				axisElement = rotateElement->FirstChildElement( "axis" );assert( axisElement );
				result = translateElement->QueryDoubleAttribute( "x", &dTranslateX );
				result+= translateElement->QueryDoubleAttribute( "y", &dTranslateY );
				result+= translateElement->QueryDoubleAttribute( "z", &dTranslateZ );
				result+= rotateElement->QueryDoubleAttribute( "angle", &dAxisAngle );
				result+= axisElement->QueryDoubleAttribute( "x", &dAxisX );
				result+= axisElement->QueryDoubleAttribute( "y", &dAxisY );
				result+= axisElement->QueryDoubleAttribute( "z", &dAxisZ );
				assert( result == 0 );

				// --- Fill Memory Begin ---//
				TKey key;
				key.time	  = dKeyTime;
				key.rot[0]    = dAxisAngle;
				key.rot[1]    = dAxisX;
				key.rot[2]    = dAxisY;
				key.rot[3]    = dAxisZ;
				key.pos[0] = dTranslateX;
				key.pos[1] = dTranslateY;
				key.pos[2] = dTranslateZ;
				track.keys.push_back(key);
			}
			animation.frameCount=max(animation.frameCount,track.keys.size());
		}
		animations.push_back(animation);
	}
	ResampleAnimationTracks(20);// 20 keyframes per second
	SetBindPose();				// store bind pose

	printf ( "Skeleton: %d bones\n\n" , bones.size() ) ;

	if(bones.size()>=100) error_stop("too many bones in skeleton (%d>100)\n",bones.size());
}
