#define BVH_DEPTH 50

/*
morton
x = (x | (x << 16)) & 0x030000FF;
x = (x | (x <<  8)) & 0x0300F00F;
x = (x | (x <<  4)) & 0x030C30C3;
x = (x | (x <<  2)) & 0x09249249;
Then, with x,y and z the three manipulated 10-bit integers we get the result by taking:

x | (y << 1) | (z << 2)
*/

struct AABB 
{ 
	vec3f min,max; 
	void extend(AABB aabb)
	{
		min.minimum(aabb.min);
		max.maximum(aabb.max);
	}
	inline float dx(){return max.x-min.x;}
	inline float dy(){return max.y-min.y;}
	inline float dz(){return max.z-min.z;}
	inline float midx(){return (max.x+min.x)*0.5;}
	inline float midy(){return (max.y+min.y)*0.5;}
	inline float midz(){return (max.z+min.z)*0.5;}
}; // min max

struct BVHNode  // if members of this structure are changed, most cuda fetch-functions (and normal shaders) must be adjusted, too!
{
	int child[2];AABB ab[2];
	//AABB aabb;
	//uint child[2];        // offset of each child
};
struct BVHChild  // if members of this structure are changed, most cuda fetch-functions (and normal shaders) must be adjusted, too!
{
	AABB aabb;
	matrix44 m;
	uint octreeroot;
};

uint bvh_root = 0;
std::vector<BVHNode>	bvh_nodes;
std::vector<BVHChild>	bvh_childs;
std::vector<int>		bvh_ids;

//void bvh_add_child(matrix44 m, uint octreeroot)
void bvh_add_child(vec3f p,vec3f s)
{
	BVHChild c;
	c.aabb.min=p;
	c.aabb.max=p+s;
	c.octreeroot=octree::root;
	bvh_childs.push_back(c);
}

int add_subtree(int a,int b)
{
	if(b==a+1)
	{
		BVHNode n;
		n.ab[0]=bvh_childs[bvh_ids[a]].aabb;
		n.child[0]=-2-bvh_ids[a];
		n.ab[1]=bvh_childs[bvh_ids[a]].aabb;//.min=n.ab[1].max=vec3f(0,0,0);
		n.child[1]=-1;
		bvh_nodes.push_back(n);
		
		return bvh_nodes.size()-1;
	}
	if(a>=b)return -1;

	static int iteration=0;

	AABB ab;
	ab=bvh_childs[bvh_ids[a]].aabb;
	vec3f avg=(ab.min+ab.max);

	loopi(a+1,b) 
	{
		AABB &bb=bvh_childs[bvh_ids[i]].aabb;
		avg=avg+bb.min+bb.max;
		ab.extend(bb);
	}
	avg=avg*(0.5/(b-a));


	int dir=0;
	if (ab.dy()>=ab.dx() && ab.dy()>=ab.dz()) dir=1;
	if (ab.dz()>=ab.dy() && ab.dz()>=ab.dx()) dir=2;

	float mid=avg[dir];//(ab.min[dir]+ab.max[dir])*0.5;
	
	
	static std::vector<int> tmp[2];
	tmp[0].clear();
	tmp[1].clear();

	BVHNode n;

	float eq_test;
	int   eq_test_cnt=1;


	loopi(a,b)
	{
		int id=bvh_ids[i];
		AABB ab=bvh_childs[id].aabb;
		float xyz=(ab.min[dir]+ab.max[dir])*0.5;

		if(i==a)eq_test=xyz; else if(xyz==eq_test) eq_test_cnt++;

		int j=0;if(xyz>mid)j=1;

		if(tmp[j].size()==0)
			n.ab[j]=ab;
		else 
			n.ab[j].extend(ab);

		tmp[j].push_back(id);
	}

	int ofs=a;
	loopj(0,2)loopi(0,tmp[j].size()) bvh_ids[ofs++]=tmp[j][i];

	int current=bvh_nodes.size();
	bvh_nodes.resize(current+1);

	iteration++;
	if(iteration>=BVH_DEPTH)
	{
//		printf("BB:%f %f %f -> dir=%d\n",ab.dx(),ab.dy(),ab.dz(),dir);
		loopi(a,b)
		{
			int id=bvh_ids[i];
			AABB ab=bvh_childs[id].aabb;
			float xyz=(ab.min[dir]+ab.max[dir])*0.5;
//			printf ("%d [%2.2f] ",i,xyz);
		}

		error_stop("Iteration Limit Exceeded");
	}

	int i=a+tmp[0].size();

	if(eq_test_cnt==b-a)i=(a+b)/2;

//	printf("%3d",iteration);
//	loopi(0,iteration)printf(" ");
//	printf("AABB[ %d->%d->%d ] %3.3f > %3.3f > %3.3f \n",
//		a,i,b,
//		ab.min[dir],mid,ab.max[dir]);

	n.child[0]= add_subtree(a,i);
	n.child[1]= add_subtree(i,b);
	iteration--;

	bvh_nodes[current]=n;

	return current;
}

void bvh_build_tree()
{
	bvh_ids.resize(bvh_childs.size());
	loopi(0,bvh_ids.size()) bvh_ids[i]=i;
	bvh_nodes.clear();

	bvh_root=add_subtree(0,bvh_ids.size());

	printf("%d bvh nodes\n%d bvh childs\nbvh_root=%d\n\n",bvh_nodes.size(),bvh_childs.size(),bvh_root);
}
