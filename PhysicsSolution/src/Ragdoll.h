#ifndef _RAGDOLL_H_
#define _RAGDOLL_H_

#include <PxPhysicsAPI.h>

using physx::PxVec3;
using physx::PxQuat;

//Parts which make up the ragdoll
enum RagDollParts
{
	NO_PARENT = -1,
	LOWER_SPINE,
	LEFT_PELVIS,
	RIGHT_PELVIS,
	LEFT_UPPER_LEG,
	RIGHT_UPPER_LEG,
	LEFT_LOWER_LEG,
	RIGHT_LOWER_LEG,
	UPPER_SPINE,
	LEFT_CLAVICLE,
	RIGHT_CLAVICLE,
	NECK,
	HEAD,
	LEFT_UPPER_ARM,
	RIGHT_UPPER_ARM,
	LEFT_LOWER_ARM,
	RIGHT_LOWER_ARM,
};

// rotation axis constants
const PxVec3 X_AXIS = PxVec3(1, 0, 0);
const PxVec3 Y_AXIS = PxVec3(0, 1, 0);
const PxVec3 Z_AXIS = PxVec3(0, 0, 1);

//structure to represent each bone in ragdoll
struct RagdollNode
{
	PxQuat globalRotation; //rotation of this link in model space - we could have done this relative to the parent node but it's harder 
							//to visualize when setting up the data by hand

	PxVec3 scaledGlobalPos; //Positon of the link centre in world space which is calculated when we process the node.
							//It's easiest if we store it here so we have it when we transform the child

	int parentNodeIdx;	 // Index of the parent node
	float halfLength;	 // half length of the capsule for this node
	float radius;		 // radius of the capsule for this node
	float parentLinkPos; // relative position of link centre in parent to this node. 0 is the centre of the node, -1 is the left end of the capsule
						 // and 1 is the right end of the capsule relative to the X axis
	float childLinkPos;	 // relative position of link centre in child
	char* name;			 // name of link

	physx::PxArticulationLink* linkPtr;

	//constructor
	RagdollNode(PxQuat _globalRotation, int _parentNodeIdx, float _halfLength, float _radius, float _parentLinkPos, float _childLinkPos, char* _name)
	{
		globalRotation = _globalRotation;
		parentNodeIdx = _parentNodeIdx;
		halfLength = _halfLength;
		radius = _radius;
		parentLinkPos = _parentLinkPos;
		childLinkPos = _childLinkPos; 
		name = _name;
	}

};

class Ragdoll
{
public:
	Ragdoll();
	~Ragdoll();

	static physx::PxArticulation* makeRagdoll(physx::PxPhysics* g_Physics, RagdollNode** nodeArray, physx::PxTransform worldPos,
											float scaleFactor, physx::PxMaterial* ragdollMaterial);

private:

};

#endif // !_RAGDOLL_H_
