#include "Ragdoll.h"

Ragdoll::Ragdoll()
{

}

Ragdoll::~Ragdoll()
{

}

physx::PxArticulation* Ragdoll::makeRagdoll(physx::PxPhysics* g_Physics, RagdollNode** nodeArray, physx::PxTransform worldPos,
	float scaleFactor, physx::PxMaterial* ragdollMaterial)
{
	//create the articulation for our ragdoll
	physx::PxArticulation* articulation = g_Physics->createArticulation();
	RagdollNode** currentNode = nodeArray;

	//while there are more nodes to process...
	while (*currentNode != nullptr)
	{
		//get a pointer to the current node							

		RagdollNode* currentNodePtr = *currentNode;

		//create a pointer ready to hold the parent node pointer if there is one
		RagdollNode* parentNode = nullptr;

		//get scaled values for capsule
		float radius = currentNodePtr->radius * scaleFactor;
		float halfLength = currentNodePtr->halfLength * scaleFactor;
		float childHalfLength = radius + halfLength;
		float parentHalfLength = 0;		//will be set later if there is a parent

		//get a pointer to the parent
		physx::PxArticulationLink* parentLinkPtr = nullptr;
		currentNodePtr->scaledGlobalPos = worldPos.p;

		//create link geometry
		if (currentNodePtr->parentNodeIdx != -1)
		{
			// if there is a parent then we need to work out our local position for the link
			// get a pointer to the parent node
			parentNode = *(nodeArray + currentNodePtr->parentNodeIdx);

			//get a pointer to the link for the parent
			parentLinkPtr = parentNode->linkPtr;

			parentHalfLength = (parentNode->radius + parentNode->halfLength) * scaleFactor;

			//work out the local position of the node
			PxVec3 currentRelative = currentNodePtr->childLinkPos * currentNodePtr->globalRotation.rotate(PxVec3(childHalfLength, 0, 0));
			PxVec3 parentRelative = -currentNodePtr->parentLinkPos * parentNode->globalRotation.rotate(PxVec3(parentHalfLength, 0, 0));
			currentNodePtr->scaledGlobalPos = parentNode->scaledGlobalPos - (parentRelative + currentRelative);

		}

		//build the transform for the link
		physx::PxTransform linkTransfrom = physx::PxTransform(currentNodePtr->scaledGlobalPos, currentNodePtr->globalRotation);
		//create the link in the articulation
		physx::PxArticulationLink* link = articulation->createLink(parentLinkPtr, linkTransfrom);

		//add the pointer to this link into the ragdoll data so we have it for later when we want to link it
		currentNodePtr->linkPtr = link;
		float jointSpace = 0.01f;	//gap between joints
		float capsuleHalfLength = (halfLength > jointSpace ? halfLength - jointSpace : 0) + 0.01f;
		physx::PxCapsuleGeometry capsule(radius, capsuleHalfLength);
		link->createShape(capsule, *ragdollMaterial);	//adds a capsule collider to the link
		physx::PxRigidBodyExt::updateMassAndInertia(*link, 300.0f);	//adds some mass, mass should really be part of the data!

		if (currentNodePtr->parentNodeIdx != -1)
		{
			//get the pointer to the joint from the link
			physx::PxArticulationJoint *joint = link->getInboundJoint();

			//get relative rotaion of this link
			PxQuat frameRotation = parentNode->globalRotation.getConjugate() * currentNodePtr->globalRotation;

			//set the parent constraint frame
			physx::PxTransform parentConstraintFrame = physx::PxTransform(PxVec3(currentNodePtr->parentLinkPos * parentHalfLength, 0, 0), frameRotation);

			//set the child constraint frame (this is the contraint frame of the newly added link)
			physx::PxTransform thisConstraintFrame = physx::PxTransform(PxVec3(currentNodePtr->childLinkPos * childHalfLength, 0, 0));

			//st up the poses for the joint so it is in the correct place
			joint->setParentPose(parentConstraintFrame);
			joint->setChildPose(thisConstraintFrame);
			//set up some constraints to stop it flopping around
			joint->setStiffness(20);
			joint->setDamping(20);
			joint->setSwingLimit(0.4f, 0.4f);
			joint->setSwingLimitEnabled(true);
			joint->setTwistLimit(-0.1f, 0.1f);
			joint->setTwistLimitEnabled(true);
		}

		currentNode++;
	}

	return articulation;
}