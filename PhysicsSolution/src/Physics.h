#ifndef SOUND_PROGRAMMING_H_
#define SOUND_PROGRAMMING_H_

#include "Application.h"
#include "Camera.h"

#include <PxPhysicsAPI.h>
#include <PxScene.h>

class Physics : public Application
{
public:
    virtual bool startup();
    virtual void shutdown();
    virtual bool update();
    virtual void draw();

    void setupPhysx();
    void setupTutorial1();

    FlyCamera m_camera;

    glm::vec2 screen_size;

    physx::PxFoundation* g_PhysicsFoundation;
    physx::PxPhysics* g_Physics;
    physx::PxScene* g_PhysicsScene;
    physx::PxDefaultErrorCallback gDefaultErrorCallback;
    physx::PxDefaultAllocator gDefaultAllocatorCallback;
    physx::PxSimulationFilterShader gDefaultFilterShader = physx::PxDefaultSimulationFilterShader;
    physx::PxMaterial* g_PhysicsMaterial;
    physx::PxMaterial* g_boxMaterial;
    physx::PxCooking* g_PhysicsCooker;

    physx::PxRigidDynamic* dynamicActors[32];
	physx::PxArticulation* g_PhysXActorsRagDolls[32];

	//mouse
	bool dragging = false;
};

#endif //CAM_PROJ_H_