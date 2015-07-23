#include "glm/glm.hpp"


#pragma once

struct FluidCells
{
	float *pressure;
	glm::vec2 *velocity;
	glm::vec3 *dye_colour;
};

class DIYFluid
{
public:
	DIYFluid(int _width, int _height, float _viscosity, float _cell_dist);
	~DIYFluid();

	void UpdateFluid(float dt);
	void RenderFluid(glm::mat4 viewProj);

	//update parts
	void Advect(float dt);
	void Diffuse(float dt);
	void Divergence(float dt);
	void UpdatePressure(float dt);
	void ApplyPressure(float dt);
	void UpdateBoundary();

	void SwapColors();
	void SwapVelocities();
	void SwapPressures();


public:
	float viscosity;
	float cell_dist;

	FluidCells front_cells;
	FluidCells back_cells;

	float* divergence;

	int width, height;

	unsigned int program;
};