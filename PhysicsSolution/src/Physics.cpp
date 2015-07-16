#include "Physics.h"

#include "gl_core_4_4.h"
#include "GLFW/glfw3.h"
#include "Gizmos.h"

#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "Ragdoll.h"

#define Assert(val) if (val){}else{ *((char*)0) = 0;}
#define ArrayCount(val) (sizeof(val)/sizeof(val[0]))

using physx::PxPi;

class MyAllocator : public physx::PxAllocatorCallback
{
    virtual void* allocate(size_t size, const char* typeName, const char* filename, int line)
    {
        void* new_mem = malloc(size + 32);
        void* result = (char*)new_mem + (16 - ((size_t)new_mem % 16));
        Assert(((size_t)result % 16) == 0);
        *(void**)result = new_mem;
        return (char*)result + 16;
    }

    virtual void deallocate(void* ptr)
    {
        if (ptr)
        {
            void* real_ptr = *(void**)((char*)ptr - 16);
            free(real_ptr);
        }
    }
};

bool Physics::startup()
{
    if (Application::startup() == false)
    {
        return false;
    }

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    Gizmos::create();


    int screen_width, screen_height;
    glfwGetWindowSize(m_window, &screen_width, &screen_height);

    screen_size = glm::vec2(screen_width, screen_height);


    m_camera = FlyCamera(screen_size.x / screen_size.y, 10.0f);
    m_camera.setLookAt(vec3(10, 10, 10), vec3(0), vec3(0, 1, 0));
    m_camera.sensitivity = 3;

    setupPhysx();
    setupTutorial1();

	//complex humanoid ragdoll example 
	RagdollNode* ragdollData[] = 
	{ 
		new RagdollNode(PxQuat(PxPi / 2.0f, Z_AXIS), NO_PARENT,1,3,1,1,"lower spine"),
		new RagdollNode(PxQuat(PxPi, Z_AXIS), LOWER_SPINE, 1,1,-1,1,"left pelvis"),
		new RagdollNode(PxQuat(0, Z_AXIS), LOWER_SPINE, 1,1,-1, 1,"right pelvis"),
		new RagdollNode(PxQuat(PxPi / 2.0f + 0.2f, Z_AXIS),LEFT_PELVIS,5,2,-1,1,"L upper leg"),
		new RagdollNode(PxQuat(PxPi / 2.0f - 0.2f, Z_AXIS),RIGHT_PELVIS,5,2,-1,1,"R upper leg"),
		new RagdollNode(PxQuat(PxPi / 2.0f + 0.2f, Z_AXIS),LEFT_UPPER_LEG,5,1.75,-1,1,"L lower leg"),
		new RagdollNode(PxQuat(PxPi / 2.0f - 0.2f, Z_AXIS),RIGHT_UPPER_LEG,5,1.75,-1,1,"R lowerleg"),
		new RagdollNode(PxQuat(PxPi / 2.0f, Z_AXIS), LOWER_SPINE, 1, 3, 1, -1, "upper spine"),
		new RagdollNode(PxQuat(PxPi, Z_AXIS), UPPER_SPINE, 1, 1.5, 1, 1, "left clavicle"),
		new RagdollNode(PxQuat(0, Z_AXIS), UPPER_SPINE, 1, 1.5, 1, 1, "right clavicle"),
		new RagdollNode(PxQuat(PxPi / 2.0f, Z_AXIS), UPPER_SPINE, 1, 1, 1, -1, "neck"),
		new RagdollNode(PxQuat(PxPi / 2.0f, Z_AXIS), NECK, 1, 3, 1, -1, "HEAD"),
		new RagdollNode(PxQuat(PxPi - .3, Z_AXIS), LEFT_CLAVICLE, 3, 1.5, -1, 1, "left upper arm"),
		new RagdollNode(PxQuat(0.3, Z_AXIS), RIGHT_CLAVICLE, 3, 1.5, -1, 1, "right upper arm"),
		new RagdollNode(PxQuat(PxPi - .3, Z_AXIS), LEFT_UPPER_ARM, 3, 1, -1, 1, "left lower arm"),
		new RagdollNode(PxQuat(0.3, physx::PxVec3(0.01f,0,0.99f)), RIGHT_UPPER_ARM, 3, 1, -1, 1, "right lower arm"),
		NULL 
	};

	//create ragdoll

	physx::PxArticulation* ragDollArticulation;
	ragDollArticulation = Ragdoll::makeRagdoll(g_Physics, ragdollData, physx::PxTransform(physx::PxVec3(0, 10, 0)), 0.1f, g_PhysicsMaterial);
	g_PhysicsScene->addArticulation(*ragDollArticulation);

	g_PhysXActorsRagDolls[0] = ragDollArticulation;


    return true;
}

void Physics::setupTutorial1()
{
    //add a plane
    physx::PxTransform pose = 
        physx::PxTransform(physx::PxVec3(0.0f, 0, 0.0f), 
                            physx::PxQuat(physx::PxHalfPi * 1.0f, 
                            physx::PxVec3(0.0f, 0.0f, 1.0f)));
    
    
    physx::PxRigidStatic* plane = 
        physx::PxCreateStatic(*g_Physics, 
                                pose, 
                                physx::PxPlaneGeometry(), 
                                *g_PhysicsMaterial);
    
    //add it to the physX scene
    g_PhysicsScene->addActor(*plane);
   
    //add a box
    float density = 10;

    for (int i = 0; i < ArrayCount(dynamicActors); ++i)
    {
        physx::PxBoxGeometry box(2, 2, 2);
        physx::PxTransform transform(physx::PxVec3(rand()%10, 50 + (rand()%3), rand()%10));
        dynamicActors[i] = PxCreateDynamic(*g_Physics,
            transform,
            box,
            *g_PhysicsMaterial,
            density);
        //add it to the physX scene
        g_PhysicsScene->addActor(*dynamicActors[i]);
    }
    
}


void Physics::setupPhysx()
{
    physx::PxAllocatorCallback *myCallback = new MyAllocator();

    g_PhysicsFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, *myCallback, gDefaultErrorCallback);
    g_Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *g_PhysicsFoundation, physx::PxTolerancesScale());
    PxInitExtensions(*g_Physics);
    //create physics material  
    g_PhysicsMaterial = g_Physics->createMaterial(0.9f, 0.9f,.1f);
    physx::PxSceneDesc sceneDesc(g_Physics->getTolerancesScale());
    sceneDesc.gravity = physx::PxVec3(0, -10.0f, 0);
    sceneDesc.filterShader = &physx::PxDefaultSimulationFilterShader;
    sceneDesc.cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(1);
    g_PhysicsScene = g_Physics->createScene(sceneDesc);


}

void Physics::shutdown()
{

    g_PhysicsScene->release();
    g_Physics->release();
    g_PhysicsFoundation->release();


    Gizmos::destroy();
    Application::shutdown();
}

bool Physics::update()
{
    if (Application::update() == false)
    {
        return false;
    }

    int screen_width, screen_height;
    glfwGetWindowSize(m_window, &screen_width, &screen_height);
    if (screen_width != screen_size.x || screen_height != screen_size.y)
    {
        glViewport(0, 0, screen_width, screen_height);
        m_camera.proj = 
            glm::perspective(glm::radians(60.0f), screen_size.x / screen_size.y, 0.1f, 1000.0f);
    }

	//ragdoll control

	//get ragdoll joint

	physx::PxArticulationLink* joints[16];
	g_PhysXActorsRagDolls[0]->getLinks(joints, 16);
	

	if (glfwGetKey(m_window,GLFW_KEY_UP) == GLFW_PRESS)
	{
		joints[11]->addForce(PxVec3(100, 0, 0), physx::PxForceMode::eACCELERATION);
	}
	else if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		joints[11]->addForce(PxVec3(-100, 0, 0), physx::PxForceMode::eACCELERATION);
	}
	else if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		joints[11]->addForce(PxVec3(0, 0, 100), physx::PxForceMode::eACCELERATION);
	}
	else if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		joints[11]->addForce(PxVec3(0, 0, -100), physx::PxForceMode::eACCELERATION);
	}

	if (glfwGetKey(m_window, GLFW_KEY_F) == GLFW_PRESS)
	{
		joints[11]->addForce(PxVec3(0, 100, 0), physx::PxForceMode::eACCELERATION);
	}

	////ragdoll dragging
	//if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
	//{
	//	double *m_x, *m_y;
	//	glfwGetCursorPos(m_window, m_x, m_y);
	//
	//	//normalize cursor position
	//	float x = (2.0f * (int)m_x) / 1080.0f - 1.0f;
	//	float y = 1.0f - (2.0f * (int)m_y) / 720.0f;
	//	float z = 1.0f;
	//	vec3 ray_nds = vec3(x,y,z);
	//
	//	vec4 ray_clip = vec4(ray_nds.xy, -1.0f, 1.0f);
	//
	//	vec4 ray_eye = inverse(m_camera.proj) * ray_clip;
	//	ray_eye = vec4(ray_eye.xy, -1.0f, 0.0f);
	//
	//	vec3 ray_wor = (inverse(m_camera.view) * ray_eye).xyz;
	//	//normalise
	//
	//	ray_wor *= vec3(0,0,5);
	//
	//
	//	if (dragging)
	//	{
	//
	//	}
	//	else
	//	{
	//		
	//	}
	//}

	

	




    Gizmos::clear();

    float dt = (float)glfwGetTime();
    glfwSetTime(0.0);

    vec4 white(1);
    vec4 black(0, 0, 0, 1);

    for (int i = 0; i <= 20; ++i)
    {
        Gizmos::addLine(vec3(-10 + i, -0.01, -10), vec3(-10 + i, -0.01, 10),
            i == 10 ? white : black);
        Gizmos::addLine(vec3(-10, -0.01, -10 + i), vec3(10, -0.01, -10 + i),
            i == 10 ? white : black);
    }

    float phys_dt = dt;// *0.01f;
    if (phys_dt > 1.0f / 30.f)
    {
        phys_dt = 1.0f / 30.0f;
    }

    g_PhysicsScene->simulate(phys_dt);
    while (g_PhysicsScene->fetchResults() == false)
    {

    }

    for (int i = 0; i < ArrayCount(dynamicActors); ++i)
    {
        if (dynamicActors[i])
        {
            physx::PxTransform box_transform = dynamicActors[i]->getGlobalPose();
            glm::vec3 pos(box_transform.p.x, box_transform.p.y, box_transform.p.z);
            glm::quat q;

            q.x = box_transform.q.x;
            q.y = box_transform.q.y;
            q.z = box_transform.q.z;
            q.w = box_transform.q.w;

            glm::mat4 rot = glm::mat4(q);

            glm::mat4 model_matrix;
            model_matrix = rot * glm::translate(model_matrix, pos);

            Gizmos::addAABB(pos, vec3(2, 2, 2), vec4(1, 1, 1, 1), &rot);
        }
    }

	//render ragdolls
	for (auto articulation : g_PhysXActorsRagDolls)
	{
		if (articulation != nullptr)
		{

			physx::PxU32 nLinks = articulation->getNbLinks();
			physx::PxArticulationLink** links = new physx::PxArticulationLink*[nLinks];
			articulation->getLinks(links, nLinks);
			
			while (nLinks--)
			{
				physx::PxArticulationLink* link = links[nLinks];
				physx::PxU32 nShapes = link->getNbShapes();
				physx::PxShape** shapes = new physx::PxShape*[nShapes];
				link->getShapes(shapes, nShapes);
				while (nShapes--)
				{
					physx::PxCapsuleGeometry geometry;
					shapes[nShapes]->getCapsuleGeometry(geometry);
					physx::PxTransform linkTransform = link->getGlobalPose();
					

					Gizmos::addCapsule(vec3(linkTransform.p.x, linkTransform.p.y, linkTransform.p.z), geometry.halfHeight * 4, geometry.radius, 6, 6, vec4(1),
						&(glm::mat4)glm::mat4_cast(glm::quat((float)linkTransform.q.w, (float)linkTransform.q.x, (float)linkTransform.q.y, (float)linkTransform.q.z)));
				}
			}
			delete[] links;
		}
	}

    m_camera.update(dt);
    
    return true;
}

void Physics::draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Gizmos::draw(m_camera.proj, m_camera.view);
    Gizmos::draw2D(glm::ortho(-1280/2.0f,1280.0f/2.0f,-720.0f/2.0f,720.0f/2.0f));

    glfwSwapBuffers(m_window);
    glfwPollEvents();
}