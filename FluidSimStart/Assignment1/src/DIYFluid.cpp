#include "DIYFluid.h"

#include "gl_core_4_4.h"
#include "Utilities.h"

void DIYFluid::SwapColors()
{
	glm::vec3 *tmp = this->front_cells.dye_colour;
	this->front_cells.dye_colour = this->back_cells.dye_colour;
	this->back_cells.dye_colour = tmp;
}

void DIYFluid::SwapVelocities()
{
	glm::vec2 *tmp = this->front_cells.velocity;
	this->front_cells.velocity = this->back_cells.velocity;
	this->back_cells.velocity = tmp;
}

void DIYFluid::SwapPressures()
{
	float *tmp = this->front_cells.pressure;
	this->front_cells.pressure = this->back_cells.pressure;
	this->back_cells.pressure = tmp;
}

DIYFluid::DIYFluid(int _width, int _height, float _viscosity, float _cell_dist)
{
	this->width = _width;
	this->height = _height;
	this->viscosity = _viscosity;
	this ->cell_dist = _cell_dist;

	int cell_count = _width * _height;

	this->front_cells.velocity = new glm::vec2[cell_count];
	this->front_cells.dye_colour = new glm::vec3[cell_count];
	this->front_cells.pressure = new float[cell_count];

	this->back_cells.velocity = new glm::vec2[cell_count];
	this->back_cells.dye_colour = new glm::vec3[cell_count];
	this->back_cells.pressure = new float[cell_count];

	this->divergence = new float[cell_count];

	memset(this->front_cells.velocity, 0, sizeof(glm::vec2) * cell_count);
	memset(this->front_cells.dye_colour, 0, sizeof(glm::vec3) * cell_count);
	memset(this->front_cells.pressure, 0, sizeof(float) * cell_count);

	memset(this->back_cells.velocity, 0, sizeof(glm::vec2) * cell_count);
	memset(this->back_cells.dye_colour, 0, sizeof(glm::vec3) * cell_count);
	memset(this->back_cells.pressure, 0, sizeof(float)* cell_count);

	memset(this->divergence, 0, sizeof(float) * cell_count);

	for (int i = 0; i < this->width * this->height; i++)
	{
		float x = (float)(i % width);
		float y = (float)(i / width);
		front_cells.dye_colour[i] = glm::vec3(x, y, 0);
		front_cells.pressure[i] = 1;
	}



	LoadShader("./shaders/simple_vertex.vs", 0, "./shaders/simple_texture.fs", &this->program);
}

DIYFluid::~DIYFluid()
{
	delete[] this->divergence;

	//front cells
	delete[] this->front_cells.dye_colour;
	delete[] this->front_cells.velocity;
	delete[] this->front_cells.pressure;

	//back cells
	delete[] this->back_cells.dye_colour;
	delete[] this->back_cells.velocity;
	delete[] this->back_cells.pressure;
}

void DIYFluid::UpdateFluid(float dt)
{
	Advect(dt);
	SwapVelocities();
	SwapColors();

	for (int diffuse_step = 0; diffuse_step < 50; ++diffuse_step)
	{
		Diffuse(dt);
		SwapVelocities();
	}

	Divergence(dt);

	for (int diffuse_step = 0; diffuse_step < 60; ++diffuse_step)
	{
		UpdatePressure(dt);
		SwapPressures();
	}

	ApplyPressure(dt);
	SwapVelocities();

	UpdateBoundary();

	int box_size = 10;
	int half_box_size = box_size / 2;

	for (int x = width / 2 - half_box_size; x < width / 2 + half_box_size; ++x)
	{
		for (int y = 5; y < 5 + box_size; y++)
		{
			int cell_index = x + y * this->width;
			this->front_cells.velocity[cell_index].y += 10 * dt;
		}
	}
}

//update parts
void DIYFluid::Advect(float dt)
{
	//loop over every cell
	for (int y = 0; y < this->height; ++y)
	{
		for (int x = 0; x < this->width; ++x)
		{

			//find the point to sample for this cell
			int cell_index = x + y * this->width;

			glm::vec2 vel = this->front_cells.velocity[cell_index] * dt;
			glm::vec2 sample_point = glm::vec2((float)x - vel.x / this->cell_dist, 
											   (float)y - vel.y / this->cell_dist);

			sample_point.x = glm::clamp(sample_point.x, 0.0f, (float)this->width - 1);
			sample_point.y = glm::clamp(sample_point.y, 0.0f, (float)this->height - 1);

			//read each value from front_cells and store in back_cells
				//computing bilerp for the points
			glm::vec2 bl = glm::vec2(floorf(sample_point.x), floorf(sample_point.y));
			glm::vec2 br = bl + glm::vec2(1, 0);
			glm::vec2 tl = bl + glm::vec2(0, 1);
			glm::vec2 tr = bl + glm::vec2(1, 1);

			//get indices
			int bli = (int)bl.x + this->width * (int)bl.y;
			int bri = (int)br.x + this->width * (int)br.y;
			int tli = (int)tl.x + this->width * (int)tl.y;
			int tri = (int)tr.x + this->width * (int)tr.y;

			//get fract
			glm::vec2 sample_fract = sample_point - bl;


			glm::vec3 dye_b = glm::mix(this->front_cells.dye_colour[bli], this->front_cells.dye_colour[bri], sample_fract.x);
			glm::vec3 dye_t = glm::mix(this->front_cells.dye_colour[tli], this->front_cells.dye_colour[tri], sample_fract.x);
					
			glm::vec3 new_dye = glm::mix(dye_b, dye_t, sample_fract.y);

			this->back_cells.dye_colour[cell_index] = new_dye;

			//vel

			glm::vec2 vel_b = glm::mix(this->front_cells.velocity[bli], this->front_cells.velocity[bri], sample_fract.x);
			glm::vec2 vel_t = glm::mix(this->front_cells.velocity[tli], this->front_cells.velocity[tri], sample_fract.x);

			glm::vec2 new_vel = glm::mix(vel_b, vel_t, sample_fract.y);

			this->back_cells.velocity[cell_index] = new_vel;

		}
	}
}

void DIYFluid::Diffuse(float dt)
{
	float inv_vdt = 1.0f / (this->viscosity * dt);

	for (int y = 0; y < this->height; ++y)
	{
		for (int x = 0; x < this->width; ++x)
		{
			int cell_index = x + y * this->width;

			int xp1 = glm::clamp(x + 1, 0, this->width - 1);
			int xm1 = glm::clamp(x - 1, 0, this->width - 1);
			int yp1 = glm::clamp(y + 1, 0, this->height - 1);
			int ym1 = glm::clamp(y + 1, 0, this->height - 1);

			//gather the 4 velocities around us
			int up = x + yp1 * this->width;
			int down = x + ym1 * this->width;
			int right = xp1 + y * this->width;
			int left = xm1 + y * this->width;

			glm::vec2 vel_up = this->front_cells.velocity[up];
			glm::vec2 vel_down = this->front_cells.velocity[down];
			glm::vec2 vel_left = this->front_cells.velocity[left];
			glm::vec2 vel_right = this->front_cells.velocity[right];
			glm::vec2 vel_centre = this->front_cells.velocity[cell_index];

			//out in equation
			float denom = 1.0f /(4 + inv_vdt);

			glm::vec2 diffused_velocity = (vel_up + vel_right + vel_down + vel_left + vel_centre * inv_vdt) * denom;

			this->back_cells.velocity[cell_index] = diffused_velocity;
		}
	}
}

void DIYFluid::Divergence(float dt)
{
	float inv_cell_dist = 1.0f / (2.0f * cell_dist);

	for (int y = 0; y < this->height; ++y)
	{
		for (int x = 0; x < this->width; ++x)
		{
			int cell_index = x + y * this->width;

			int xp1 = glm::clamp(x + 1, 0, this->width - 1);
			int xm1 = glm::clamp(x - 1, 0, this->width - 1);
			int yp1 = glm::clamp(y + 1, 0, this->height - 1);
			int ym1 = glm::clamp(y + 1, 0, this->height - 1);

			//gather the 4 velocities around us
			int up = x + yp1 * this->width;
			int down = x + ym1 * this->width;
			int right = xp1 + y * this->width;
			int left = xm1 + y * this->width;

			float vel_up = this->front_cells.velocity[up].y;
			float vel_down = this->front_cells.velocity[down].y;
			float vel_left = this->front_cells.velocity[left].x;
			float vel_right = this->front_cells.velocity[right].x;

			float divergence = (vel_right - vel_left) + (vel_up - vel_down) * inv_cell_dist;

			this->divergence[cell_index] = divergence;

		}
	}
}

void DIYFluid::UpdatePressure(float dt)
{
	for (int y = 0; y < this->height; ++y)
	{
		for (int x = 0; x < this->width; ++x)
		{
			int cell_index = x + y * this->width;

			int xp1 = glm::clamp(x + 1, 0, this->width - 1);
			int xm1 = glm::clamp(x - 1, 0, this->width - 1);
			int yp1 = glm::clamp(y + 1, 0, this->height - 1);
			int ym1 = glm::clamp(y + 1, 0, this->height - 1);

			//gather the 4 velocities around us
			int up = x + yp1 * this->width;
			int down = x + ym1 * this->width;
			int right = xp1 + y * this->width;
			int left = xm1 + y * this->width;

			float p_up = this->front_cells.pressure[up];
			float p_down = this->front_cells.pressure[down];
			float p_left = this->front_cells.pressure[left];
			float p_right = this->front_cells.pressure[right];

			float d = this->divergence[cell_index];

			float new_pressure = (p_up + p_down + p_left + p_right - d * this->cell_dist * this->cell_dist) * 0.25f;

			this->back_cells.pressure[cell_index] = new_pressure;


		}
	}
}
void DIYFluid::ApplyPressure(float dt)
{
	float inv_cell_dist = 1.0f / (2.0f * cell_dist);

	for (int y = 0; y < this->height; ++y)
	{
		for (int x = 0; x < this->width; ++x)
		{
			int cell_index = x + y * this->width;

			int xp1 = glm::clamp(x + 1, 0, this->width - 1);
			int xm1 = glm::clamp(x - 1, 0, this->width - 1);
			int yp1 = glm::clamp(y + 1, 0, this->height - 1);
			int ym1 = glm::clamp(y + 1, 0, this->height - 1);

			//gather the 4 velocities around us
			int up = x + yp1 * this->width;
			int down = x + ym1 * this->width;
			int right = xp1 + y * this->width;
			int left = xm1 + y * this->width;

			float p_up = this->front_cells.pressure[up];
			float p_down = this->front_cells.pressure[down];
			float p_left = this->front_cells.pressure[left];
			float p_right = this->front_cells.pressure[right];

			glm::vec2 delta_v = -glm::vec2(p_right - p_left, p_up - p_down) * inv_cell_dist;

			this->back_cells.velocity[cell_index] = this->front_cells.velocity[cell_index] + delta_v;

		}
	}
}

void DIYFluid::UpdateBoundary()
{

	float* p = this->front_cells.pressure;
	glm::vec2 *v = this->front_cells.velocity;

	for (int x = 0; x < this->width; x++)
	{

		//first rows
		int first_row_index = x;
		int second_row_index = x + this->width;

		p[first_row_index] = p[second_row_index];
		v[first_row_index].x = v[second_row_index].x;
		v[first_row_index].y = -v[second_row_index].y;

		//last rows
		int last_row_index = x + (this->height - 1) * this->width;
		int second_last_row_index = x + (this->height - 2) * this->width;

		p[last_row_index] =    p[second_last_row_index];
		v[last_row_index].x =  v[second_last_row_index].x;
		v[last_row_index].y = -v[second_last_row_index].y;
	}

	for (int y = 0; y < this->height; y++)
	{
		int first_col_index = 0 + y * this->width;
		int second_col_index = 1 + y * this->width;

		p[first_col_index] = p[second_col_index];
		v[first_col_index].x = -v[second_col_index].x;
		v[first_col_index].y = v[second_col_index].y;

		int last_col_index = (this->width - 1) + y * this->width;
		int second_last_col_index = (this->width - 2) + y * this->width;

		p[last_col_index] = p[second_last_col_index];
		v[last_col_index].x = -v[second_last_col_index].x;
		v[last_col_index].y = v[second_last_col_index].y;
	}
}


void DIYFluid::RenderFluid(glm::mat4 viewProj)
{
	//Allocate space for texture data
	unsigned char* tex_data = new unsigned char[this->width * this->height * 3];

	for (int i = 0; i < this->width * this->height; i++)
	{
		tex_data[i * 3 + 0] = (unsigned char)this->front_cells.dye_colour[i].r;
		tex_data[i * 3 + 1] = (unsigned char)this->front_cells.dye_colour[i].g;
		tex_data[i * 3 + 2] = (unsigned char)this->front_cells.dye_colour[i].b;
	}

	unsigned int texture_handle = CreateGLTextureBasic(tex_data, this->width, this->height, 3);

	unsigned int quad_vao = BuildQuadGLVAO(5.0f);

	RenderQuad(quad_vao, this->program, texture_handle, viewProj);

	glDeleteVertexArrays(1, &quad_vao);
	glDeleteTextures(1, &texture_handle);

	delete[] tex_data;

}
