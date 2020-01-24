/*
CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

#include <iostream>
#include <vector>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#define PI 3.14159265359
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "OpenVRclass.h"

#include "fonts.h"
#include "WindowManager.h"
#include "Shape.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <bitset>
using namespace std;
using namespace glm;

#ifndef BYTE
#define BYTE bitset<8>
#endif
#define MSAAFACT 2

#define SSBO_SIZE 4

// Global objects
OpenVRApplication * vrapp = NULL;
bmpfont* font = new bmpfont();

double get_last_elapsed_time()
{
    static double lasttime = glfwGetTime();
    double actualtime = glfwGetTime();
    double difference = actualtime - lasttime;
    lasttime = actualtime;
    return difference;
}

class Camera
{
public:
	glm::vec3 pos_, rot_;
	int w, a, s, d, space, c, shift_active;
	Camera()
	{
		w = a = s = d = space = c = shift_active = 0;
		rot_ = glm::vec3(0, 0, 0);
		pos_ = glm::vec3(0, 0, 0);
	}
	Camera(vec3 pos, vec3 rot)
	{
		w = a = s = d = space = c = shift_active = 0;
		rot_ = rot;
		pos_ = pos;
	}
	glm::mat4 process(double ftime)
	{
		float speed = 0;
		float speedY = 0;
		if (shift_active == 0 && w == 1) { speed = 1 * ftime; }
		else if (shift_active == 0 && s == 1) { speed = -1 * ftime; }

		if (space == 1) { speedY = -1 * ftime; }
		else if (c == 1) { speedY = 1 * ftime; }

		float angle_y = 0;
		if (a == 1) { angle_y = -1 * ftime; }
		else if (d == 1) { angle_y = 1 * ftime; }
		float angle_z = 0;
		if (shift_active == 1 && w == 1) { angle_z = 1 * ftime; }
		else if (shift_active == 1 && s == 1) { angle_z = -1 * ftime; }

		rot_.y += angle_y;
		rot_.x += angle_z;
		glm::mat4 R_Y = glm::rotate(glm::mat4(1), rot_.y, glm::vec3(0, 1, 0));
		glm::mat4 R_X = glm::rotate(glm::mat4(1), rot_.x, glm::vec3(1, 0, 0));
		glm::vec4 dir = glm::vec4(0, speedY, speed, 1);
		dir = dir * R_Y * R_X;
		pos_ += glm::vec3(dir.x, dir.y, dir.z);
		glm::mat4 T = glm::translate(glm::mat4(1), pos_);
		return R_X * R_Y * T;
	}
};

class Application : public EventCallbacks
{
private:
    shared_ptr<Shape> shape_earth_, shape_moon_, shape_gw_source_, shape_hand_left, shape_hand_right;

    // sphere controls
    GLuint left_, right_, forward_, backward_, up_, down_;
	vec3 earth_pos_, cam_start_pos_;
	vec3 manual_hand_pos_left_;
	vec3 manual_hand_pos_right_;
    
    Camera cam_;

	string resourceDir = "";
	string shaderDir = "";

	float sphere_advance_val_ = 0.005f;
	

public:
    WindowManager * windowManager = nullptr;

    // Our shader program
    std::shared_ptr<Program> prog_grid_x, prog_grid_y, prog_grid_z;
	std::shared_ptr<Program> prog_earth, prog_moon, prog_binary_system;
	std::shared_ptr<Program> prog_hand_left, prog_hand_right;
    std::shared_ptr<Program> prog_skybox;
    std::shared_ptr<Program> prog_post_proc;

	std::shared_ptr<Program> prog_gauge;
	std::shared_ptr<Program> prog_compute_shader;
    
    std::shared_ptr<Program> prog_debug;

	vec3 ControllerPosLeft = vec3(0);
	vec3 controller_pos_right_ = vec3(0);

    // Contains vertex information for OpenGL
    GLuint VertexArrayID;

    // Data necessary to give our box to OpenGL
    GLuint VertexBufferID, VertexColorIDBox, IndexBufferIDBox;

	vec2 bi_star_facts = vec2(1, 0.1f);
	float bi_star_angle_off = 0.2;
    //texture data
    GLuint Texture_grid, Texture_earth, Texture_sun, Texture_spiral, Texture_moon, Texture_marble;
	GLuint Texture_gauge, Texture_indicator;
    GLuint cubemapTexture, FBO_MSAA, FBO_MSAA_depth, FBO_MSAA_color, VertexArrayIDBox, VertexBufferIDBox;

	bool paused = false;
	bool manualMode = true;

	ssbo_data ssbo_CPUMEM;
	GLuint ssbo_GPU_id;
	void computeShader()
	{
		if (prog_compute_shader->gpuId <= 0) {
			cout << "invalid Compute shader, abort" << endl;
			exit(1);
		}
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, prog_compute_shader->gpuId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_data), &ssbo_CPUMEM, GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, prog_compute_shader->gpuId);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
		
		GLuint block_index = 0;
		block_index = glGetProgramResourceIndex(prog_compute_shader->pid, GL_SHADER_STORAGE_BLOCK, "shader_data");
		GLuint ssbo_binding_point_index = 0;
		glShaderStorageBlockBinding(prog_compute_shader->pid, block_index, ssbo_binding_point_index);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, prog_compute_shader->gpuId);
		glUseProgram(prog_compute_shader->pid);


		glDispatchCompute((GLuint)SSBO_SIZE, (GLuint)1, 1);				//start compute shader
																		//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

		//copy data back to CPU MEM

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, prog_compute_shader->gpuId);
		GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
		int siz = sizeof(ssbo_data);
		memcpy(&ssbo_CPUMEM, p, siz);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		// cout << "current height: " << ssbo_CPUMEM.io[0].w << endl;
	}

	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
		{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		// utility
		if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
			earth_pos_ = vec3(vec3(-1.5f, -0.2f, 0.f));
			manual_hand_pos_left_ = vec3(-2.f, 0.f, 0.1f);
			manual_hand_pos_right_ = vec3(-1.f, 0.5f, 0.1f);
		}

		if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) { cam_.shift_active = 1; }
		if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE) { cam_.shift_active = 0; }

		// CAMERA
		// move left/right
		if (key == GLFW_KEY_W && action == GLFW_PRESS) { cam_.w = 1; }
		if (key == GLFW_KEY_W && action == GLFW_RELEASE) { cam_.w = 0; }
		if (key == GLFW_KEY_S && action == GLFW_PRESS) { cam_.s = 1; }
		if (key == GLFW_KEY_S && action == GLFW_RELEASE) { cam_.s = 0; }
		// turn left/right
		if (key == GLFW_KEY_A && action == GLFW_PRESS) { cam_.a = 1; }
		if (key == GLFW_KEY_A && action == GLFW_RELEASE) { cam_.a = 0; }
		if (key == GLFW_KEY_D && action == GLFW_PRESS) { cam_.d = 1; }
		if (key == GLFW_KEY_D && action == GLFW_RELEASE) { cam_.d = 0; }
		// move up/down
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) { cam_.space = 1; }
		if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) { cam_.space = 0; }
		if (key == GLFW_KEY_C && action == GLFW_PRESS) { cam_.c = 1; }
		if (key == GLFW_KEY_C && action == GLFW_RELEASE) { cam_.c = 0; }

		// Sphere
		if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) { left_ = 1; }
		if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE) { left_ = 0; }
		if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) { right_ = 1; }
		if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE) { right_ = 0; }
		if (key == GLFW_KEY_UP && action == GLFW_PRESS) { forward_ = 1; }
		if (key == GLFW_KEY_UP && action == GLFW_RELEASE) { forward_ = 0; }
		if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) { backward_ = 1; }
		if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE) { backward_ = 0; }
		if (key == GLFW_KEY_O && action == GLFW_PRESS) { up_ = 1; }
		if (key == GLFW_KEY_O && action == GLFW_RELEASE) { up_ = 0; }
		if (key == GLFW_KEY_L && action == GLFW_PRESS) { down_ = 1; }
		if (key == GLFW_KEY_L && action == GLFW_RELEASE) { down_ = 0; }

		// Binary System attributes
		if (key == GLFW_KEY_Y && action == GLFW_RELEASE) { bi_star_facts.x += 0.1; cout << bi_star_facts.x << endl; }
		if (key == GLFW_KEY_U && action == GLFW_RELEASE) { bi_star_facts.x -= 0.1;	cout << bi_star_facts.x << endl;			}
		if (key == GLFW_KEY_H && action == GLFW_RELEASE) { bi_star_facts.y += 0.1; cout << bi_star_facts.y << endl;			}
		if (key == GLFW_KEY_J && action == GLFW_RELEASE) { bi_star_facts.y -= 0.1; cout << bi_star_facts.y << endl;			}

		if (key == GLFW_KEY_1 && action == GLFW_RELEASE) { vrapp->eyeconvergence += 0.01; cout << vrapp->eyeconvergence << endl; }
		if (key == GLFW_KEY_2 && action == GLFW_RELEASE) { vrapp->eyeconvergence -= 0.01;	cout << vrapp->eyeconvergence << endl; }
		if (key == GLFW_KEY_3 && action == GLFW_RELEASE) { vrapp->eyedistance += 0.01; cout << vrapp->eyedistance << endl; }
		if (key == GLFW_KEY_4 && action == GLFW_RELEASE) { vrapp->eyedistance -= 0.01; cout << vrapp->eyedistance << endl; }

		if (key == GLFW_KEY_P && action == GLFW_RELEASE) { paused = !paused; cout << "Sim " << (paused ? "Paused!" : "Resumed!") << endl; }
		if (key == GLFW_KEY_M && action == GLFW_RELEASE) { manualMode = !manualMode;  cout << "Simulation Mode: " << (manualMode ? "Manual." : "VR.") << endl; }
    }

    // callback for the mouse when clicked move the triangle when helper functions
    // written
    void mouseCallback(GLFWwindow *window, int button, int action, int mods)
    {
        //double posX, posY;
        //float newPt[2];
        //if (action == GLFW_PRESS)
        //{
        //    glfwGetCursorPos(window, &posX, &posY);
        //    std::cout << "Pos X " << posX <<  " Pos Y " << posY << std::endl;

        //    //change this to be the points converted to WORLD
        //    //THIS IS BROKEN< YOU GET TO FIX IT - yay!
        //    newPt[0] = 0;
        //    newPt[1] = 0;

        //    std::cout << "converted:" << newPt[0] << " " << newPt[1] << std::endl;
        //    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
        //    //update the vertex array with the updated points
        //    glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*6, sizeof(float)*2, newPt);
        //    glBindBuffer(GL_ARRAY_BUFFER, 0);
        //}
    }

    //if the window is resized, capture the new size and reset the viewport
    void resizeCallback(GLFWwindow *window, int in_width, int in_height)
    {
        //get the window size - may be different then pixels for retina
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
    }

    /*Note that any gl calls must always happen after a GL state is initialized */
    GLuint generate_texture2D(GLushort colortype, int width, int height, GLushort colororder, GLushort datatype, BYTE* data, GLushort wrap, GLushort minfilter, GLushort magfilter)
    {
        GLuint textureID;
        //RGBA8 2D texture, 24 bit depth texture, 256x256
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter);
        glTexImage2D(GL_TEXTURE_2D, 0, colortype, width, height, 0, colororder, datatype, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        return textureID;
    }
    void bindTexture() {
        
    }
    
    GLuint VAOX, VAOY, VAOZ, VBOX, VBOY, VBOZ;
    GLuint VAODebug, VBODebug;
	GLuint VAOGauge, VBOGauge;
    GLuint skyboxVAO, skyboxVBO;
    int grid_vertices_size;
	void initGeom()
    {
        // generating the grid
        std::vector<vec3> grid_x, grid_y, grid_z;
        float step = 0.125/2;
        float gridSize = 3.;
        int numPoints = 10;
        float innerStep = step / (float)numPoints;
        // the grid is rendered as LINE_STRIP, therefore we always set pairs of points (a->b, b->c, etc..)
        for (float z = -gridSize; z <= 0; z += step)
        {
        for (float x = -gridSize; x <= 0; x += step)
            {
            for (float y = -gridSize / 2.; y <= gridSize / 2.; y += step)
                {
                for (int xi = 0; xi < numPoints; xi++)
                    {
                    grid_x.push_back(vec3(x - (xi * innerStep), y, z));
                    grid_x.push_back(vec3(x - ((xi + 1) * innerStep), y, z));
                    }
                for (int yi = 0; yi < numPoints; yi++)
                    {
                    grid_y.push_back(vec3(x, y + (yi * innerStep), z));
                    grid_y.push_back(vec3(x, y + ((yi + 1) * innerStep), z));
                    }
                for (int zi = 0; zi < numPoints; zi++)
                    {
                    grid_z.push_back(vec3(x, y, z - (zi * innerStep)));
                    grid_z.push_back(vec3(x, y, z - ((zi + 1) * innerStep)));
                    }
                }
            }
        }
        
        // all vertices have to be the same length anyway.
        grid_vertices_size = grid_x.size();
        
        glGenVertexArrays(1, &VAOX);
        glBindVertexArray(VAOX);
        glGenBuffers(1, &VBOX);
        glBindBuffer(GL_ARRAY_BUFFER, VBOX);
        glBufferData(GL_ARRAY_BUFFER, grid_x.size() * sizeof(vec3), grid_x.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

        glBindVertexArray(0);
        
        glGenVertexArrays(1, &VAOY);
        glBindVertexArray(VAOY);
        glGenBuffers(1, &VBOY);
        glBindBuffer(GL_ARRAY_BUFFER, VBOY);
        glBufferData(GL_ARRAY_BUFFER, grid_y.size() * sizeof(vec3), grid_y.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

        glBindVertexArray(0);
        
        glGenVertexArrays(1, &VAOZ);
        glBindVertexArray(VAOZ);
        glGenBuffers(1, &VBOZ);
        glBindBuffer(GL_ARRAY_BUFFER, VBOZ);
        glBufferData(GL_ARRAY_BUFFER, grid_z.size() * sizeof(vec3), grid_z.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

        glBindVertexArray(0);


        // Initialize mesh.
        shape_earth_ = make_shared<Shape>();
        shape_earth_->loadMesh(resourceDir + "/sphere2.obj");
        shape_earth_->resize();
        shape_earth_->init();
        
        shape_moon_ = make_shared<Shape>();
        shape_moon_->loadMesh(resourceDir + "/sphere2.obj");
        shape_moon_->resize();
        shape_moon_->init();

        shape_gw_source_ = make_shared<Shape>();
        shape_gw_source_->loadMesh(resourceDir + "/sphere.obj");
        shape_gw_source_->resize();
        shape_gw_source_->init();

		shape_hand_left = make_shared<Shape>();
		shape_hand_left->loadMesh(resourceDir + "/sphere.obj");
		shape_hand_left->resize();
		shape_hand_left->init();
		shape_hand_right = make_shared<Shape>();
		shape_hand_right->loadMesh(resourceDir + "/sphere.obj");// "/controller_right.obj", &(resourceDir + "/controller_right.mtl"));
		shape_hand_right->resize();
		shape_hand_right->init();
        
        int width, height, channels;
        char filepath[1000];

        //texture 1
        string str = resourceDir + "/lazor_abg_blue.png";
        strcpy(filepath, str.c_str());
        unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
        glGenTextures(1, &Texture_grid);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture_grid);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        //texture 2
        str = resourceDir + "/world_mirrored.jpg";
        strcpy(filepath, str.c_str());
        data = stbi_load(filepath, &width, &height, &channels, 4);
        glGenTextures(1, &Texture_earth);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Texture_earth);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        //[TWOTEXTURES]
        //set the 2 textures to the correct samplers in the fragment shader:
        glUseProgram(prog_grid_x->pid);
        GLuint Tex1Location = glGetUniformLocation(prog_grid_x->pid, "tex");
        glUniform1i(Tex1Location, 0);
        Tex1Location = glGetUniformLocation(prog_grid_x->pid, "tex2");
        glUniform1i(Tex1Location,1);

        glUseProgram(prog_grid_y->pid);
        Tex1Location = glGetUniformLocation(prog_grid_y->pid, "tex");
        glUniform1i(Tex1Location, 0);
        Tex1Location = glGetUniformLocation(prog_grid_y->pid, "tex2");
        glUniform1i(Tex1Location, 1);
        
        glUseProgram(prog_grid_z->pid);
        Tex1Location = glGetUniformLocation(prog_grid_z->pid, "tex");
        glUniform1i(Tex1Location, 0);
        Tex1Location = glGetUniformLocation(prog_grid_z->pid, "tex2");
        glUniform1i(Tex1Location, 1);
        
        glUseProgram(prog_earth->pid);
        GLuint Tex2Location = glGetUniformLocation(prog_earth->pid, "tex_sphere");
        glUniform1i(Tex2Location, 0);
        Tex2Location = glGetUniformLocation(prog_earth->pid, "tex_spiral");
        glUniform1i(Tex2Location, 1);
        
        glUseProgram(prog_moon->pid);
        Tex2Location = glGetUniformLocation(prog_moon->pid, "tex_sphere");
        glUniform1i(Tex2Location, 0);
        Tex2Location = glGetUniformLocation(prog_moon->pid, "tex_spiral");
        glUniform1i(Tex2Location, 1);

		glUseProgram(prog_compute_shader->pid);
		Tex1Location = glGetUniformLocation(prog_compute_shader->pid, "tex");
		glUniform1i(Tex1Location, 0);
		glUseProgram(0);
        
        //texture sun
        str = resourceDir + "/sun.jpg";
        strcpy(filepath, str.c_str());
        data = stbi_load(filepath, &width, &height, &channels, 4);
        glGenTextures(1, &Texture_sun);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, Texture_sun);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        GLuint Tex3Location = glGetUniformLocation(prog_binary_system->pid, "tex_sphere");
        glUseProgram(prog_binary_system->pid);
        glUniform1i(Tex3Location, 0);
        
        //texture moon
        str = resourceDir + "/moon.jpg";
        strcpy(filepath, str.c_str());
        data = stbi_load(filepath, &width, &height, &channels, 4);
        glGenTextures(1, &Texture_moon);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Texture_moon);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        Tex3Location = glGetUniformLocation(prog_moon->pid, "tex_sphere");
        glUseProgram(prog_moon->pid);
        glUniform1i(Tex3Location, 0);
        
        glBindVertexArray(0);

//        texture spiral
		str = resourceDir + "/spiral_col.png";//"/double_spiral.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture_spiral);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_spiral);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		//GLuint Tex4Location = glGetUniformLocation(prog_debug->pid, "tex");
		//glUseProgram(prog_debug->pid);
		//glUniform1i(Tex4Location, 0);

		//        texture marble
		str = resourceDir + "/marble.jpg";
		memset(filepath, 0, sizeof(filepath));
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture_marble);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_marble);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		GLuint Tex5Location = glGetUniformLocation(prog_hand_left->pid, "tex");
		glUseProgram(prog_hand_left->pid);
		glUniform1i(Tex5Location, 0);
		Tex5Location = glGetUniformLocation(prog_hand_right->pid, "tex");
		glUseProgram(prog_hand_right->pid);
		glUniform1i(Tex5Location, 0);
        
        glBindVertexArray(0);

		// ---------- Textures Gauge
		//texture gauge
		str = resourceDir + "/gauge.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture_gauge);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_gauge);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		//texture 2
		str = resourceDir + "/indicator.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture_indicator);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Texture_indicator);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glUseProgram(prog_gauge->pid);
		Tex1Location = glGetUniformLocation(prog_gauge->pid, "tex_gauge");
		glUniform1i(Tex1Location, 0);
		Tex1Location = glGetUniformLocation(prog_gauge->pid, "tex_indicator");
		glUniform1i(Tex1Location, 1);

		glUseProgram(0);
		glBindVertexArray(0);
		// ------ end Textures gauge
		// ------ texture display gauge?
		glGenVertexArrays(1, &VAOGauge);
		glBindVertexArray(VAOGauge);
		glGenBuffers(1, &VBOGauge);
		glBindBuffer(GL_ARRAY_BUFFER, VBOGauge);
		GLfloat* rectangle_vertices_gauge = new GLfloat[3];
		int verccount_gauge = 0;
		rectangle_vertices_gauge[verccount_gauge++] = 0.0, rectangle_vertices_gauge[verccount_gauge++] = 0.0, rectangle_vertices_gauge[verccount_gauge++] = 0.0;
		/*rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;*/
		glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(float), rectangle_vertices_gauge, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBindVertexArray(0);
		// ------ end texture display gauge
        
        glGenVertexArrays(1, &VAODebug);
        glBindVertexArray(VAODebug);

        // Send the position array to the GPU
        glGenBuffers(1, &VBODebug);
        glBindBuffer(GL_ARRAY_BUFFER, VBODebug);
        
        static const GLfloat box_debug_data[] = {
            -0.75, -0.25, 0.0,
            -0.25, -0.25, 0.0,
            -0.75, 0.25, 0.0,
            -0.75, 0.25, 0.0,
            -0.25, 0.25, 0.0,
            -0.25, -0.25, 0.0,
        };
        glBufferData(GL_ARRAY_BUFFER, (18) * sizeof(GLfloat), &box_debug_data, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);
        
        //////////////////////////
        glGenVertexArrays(1, &VertexArrayIDBox);
        glBindVertexArray(VertexArrayIDBox);
        glGenBuffers(1, &VertexBufferIDBox);
        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDBox);
        GLfloat* rectangle_vertices = new GLfloat[18];
        int verccount = 0;
        rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
        glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), rectangle_vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        GLuint VertexBufferTex;
        glGenBuffers(1, &VertexBufferTex);
        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTex);
        GLfloat* rectangle_texture_coords = new GLfloat[12];
        int texccount = 0;
        rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 0;
        rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
        rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;
        rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
        rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 1;
        rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;
        glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), rectangle_texture_coords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);


        //create frame buffer for MSAA

        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

        glGenFramebuffers(1, &FBO_MSAA);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_MSAA);
        FBO_MSAA_color = generate_texture2D(GL_RGBA8, width* MSAAFACT, height* MSAAFACT, GL_RGBA, GL_UNSIGNED_BYTE, NULL, GL_CLAMP_TO_BORDER, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBO_MSAA_color, 0);
        glGenRenderbuffers(1, &FBO_MSAA_depth);
        glBindRenderbuffer(GL_RENDERBUFFER, FBO_MSAA_depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width* MSAAFACT, height* MSAAFACT);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, FBO_MSAA_depth);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void initProgram(std::shared_ptr<Program> prog, vector<string> shaderv, vector<string> uniformv = vector<string>(), vector<string> attributesv = vector<string>()) {
		prog->setVerbose(true);
		switch (shaderv.size()) {
			case 1:
				prog->setShaderNames(shaderDir + shaderv.at(0));
				break;
			case 2:
				prog->setShaderNames(shaderDir + shaderv.at(0), shaderDir + shaderv.at(1));
				break;
			case 3:
				prog->setShaderNames(shaderDir + shaderv.at(0), shaderDir + shaderv.at(1), shaderDir + shaderv.at(2));
				break;
			default:
				std::cerr << "Invalid call to initProgramm, invalid number of shaders: " << shaderv.size() << "!" << std::endl;
		}
        
        if (!prog->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        
        // any further uniforms
        for(int i = 0; i < (int)uniformv.size(); i++) {
            prog->addUniform(uniformv.at(i));
        }
        
        // any further attributes
        for(int i = 0; i < (int)attributesv.size(); i++) {
            prog->addAttribute(attributesv.at(i));
        }
    }

	void setResourceDirectory(const std::string& resDir) {
		if (resDir.size() > 0) {
			resourceDir = resDir;
		}

#if defined(__APPLE__)
		resourceDir = "../../resources";
#else
		resourceDir = "../resources";
#endif
		shaderDir = resourceDir + "/shaders";
	}

    //General OGL initialization - set OGL state here
	void init(const std::string& resDir)
    {
        GLSL::checkVersion();
		setResourceDirectory(resDir);
        left_ =  right_ = forward_ = backward_ = 0;
		earth_pos_ = vec3(-1.5f, -0.2f, 0.f);
		cam_start_pos_ = vec3(1.5, -0., -0.8);
		cam_ = Camera(cam_start_pos_, vec3(0));
		manual_hand_pos_left_ = vec3(-2.f, 0.f, 0.2f);
		manual_hand_pos_right_ = vec3(-1.f, 0.f, 0.2f);
        // Set background color.
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        // Enable z-buffer test.
        glEnable(GL_DEPTH_TEST);

        // Initialize the GLSL programs.
        prog_grid_x = make_shared<Program>();
        prog_grid_y = make_shared<Program>();
        prog_grid_z = make_shared<Program>();
        prog_earth = make_shared<Program>();
        prog_moon = make_shared<Program>();
		prog_binary_system = make_shared<Program>();
		prog_post_proc = make_shared<Program>();

		prog_hand_left = make_shared<Program>();
		prog_hand_right = make_shared<Program>();

		prog_gauge = make_shared<Program>();
		prog_compute_shader = make_shared<Program>();
        
        prog_debug = make_shared<Program>();
        
		// INIT Grid
        initProgram(prog_grid_x,
                    vector<string>({"/shader_vertex_grid.glsl", "/shader_fragment_grid.glsl", "/shader_geometry_x.glsl"}),
                    vector<string>({"P", "V", "M", "Ry","SpherePos", "MoonPos", "BPosOne", "BPosTwo", "earthScale","bi_star_facts" }));
        initProgram(prog_grid_y,
                    vector<string>({"/shader_vertex_grid.glsl", "/shader_fragment_grid.glsl", "/shader_geometry_y.glsl"}),
                    vector<string>({"P", "V", "M", "Ry","SpherePos", "MoonPos", "BPosOne", "BPosTwo", "earthScale","bi_star_facts" }));
        initProgram(prog_grid_z,
                    vector<string>({"/shader_vertex_grid.glsl", "/shader_fragment_grid.glsl", "/shader_geometry_z.glsl"}),
                    vector<string>({"P", "V", "M", "Ry","SpherePos", "MoonPos", "BPosOne", "BPosTwo", "earthScale","bi_star_facts" }));

		// INIT Planet objects
        initProgram(prog_earth,
                    vector<string>({"/shader_vertex_earth.glsl", "/shader_fragment_sphere.glsl"}),
                    vector<string>({"P", "V", "M", "Ry","lightpos","colordot","bi_star_facts" }),
                    vector<string>({"vertPos", "vertNor", "vertTex"}));
		initProgram(prog_moon,
			vector<string>({ "/shader_vertex_earth.glsl", "/shader_fragment_sphere.glsl" }),
			vector<string>({ "P", "V", "M", "Ry","lightpos" ,"colordot","bi_star_facts" }),
			vector<string>({ "vertPos", "vertNor", "vertTex" }));

		// INIT Controller objects
		initProgram(prog_hand_left,
			vector<string>({ "/shader_vertex_sphere.glsl", "/shader_fragment_sphere.glsl" }),
			vector<string>({ "P", "V", "M","lightpos" ,"colordot" }),
			vector<string>({ "vertPos", "vertNor", "vertTex"}));
		initProgram(prog_hand_right,
			vector<string>({ "/shader_vertex_sphere.glsl", "/shader_fragment_sphere.glsl" }),
			vector<string>({ "P", "V", "M","lightpos" ,"colordot" }),
			vector<string>({ "vertPos", "vertNor", "vertTex" }));

		// INIT binary system
        initProgram(prog_binary_system,
            vector<string>({"/shader_vertex_sphere.glsl", "/shader_fragment_sphere.glsl"}),
            vector<string>({"P", "V", "M","lightpos" ,"colordot"  }),
            vector<string>({"vertPos","vertNor", "vertTex"}));

		// INIT compute shader
		initProgram(prog_compute_shader,
			vector<string>({"/shader_compute.glsl"}),
			vector<string>({"Ry", "HandPos", "bi_star_facts"}));

		// INIT gauge
		initProgram(prog_gauge,
			vector<string>({ "/shader_vertex_gauge.glsl", "/shader_fragment_gauge.glsl", "/shader_geometry_gauge.glsl" }),
			vector<string>({ "P", "V", "M", "RotM", "HandPos", "gVP", "CamPos"}));

		// INIT post processing program
        initProgram(prog_post_proc,
                    vector<string>({ "/ppvert.glsl", "/ppfrag.glsl" }));

		initProgram(prog_debug,
			vector<string>({"/shader_vertex_debug.glsl", "/shader_fragment_debug.glsl"}),
			vector<string>({"P", "V", "M", "Ry", "angle"}));
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    
	// Utility print vector functions
    void printVec(vec3 vec, std::string name = "vec3") {
        std::cout << name << " x: " << vec.x << " y: " << vec.y << " z: " << vec.z << std::endl;
    }
    void printVec(vec4 vec, std::string name = "vec4") {
        std::cout << name << " x: " << vec.x << " y: " << vec.y << " z: " << vec.z << " w: " << vec.w << " [3]: " << vec[3] << std::endl;
    }

	string RoundToString(float number, int digits) {
		float powFact = pow(10.f, digits);
		string numString = to_string(round(number *  powFact) / powFact);
		return numString.erase(numString.find_last_not_of('0') + 1, std::string::npos);
	}
    
    /****DRAW
    This is the most important function in your program - this is where you
    will actually issue the commands to draw any geometry you have set up to
    draw
    ********/
    void render(int width, int height, glm::mat4 VRheadmatrix, int eye, bool VRon)
    {
		for (int i = 0; i < SSBO_SIZE; i++)
			ssbo_CPUMEM.io[i] = vec4(0, 0, 0, 0);

		vec4 lightpos = vec4(0, 0, 0, 1);
		vec3 colordot = vec3(1);
        //glBindFramebuffer(GL_FRAMEBUFFER, FBO_MSAA);
        //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBO_MSAA_color, 0);
        //GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
        //glDrawBuffers(1, buffers);
        
        double frametime = get_last_elapsed_time();

        // Get current frame buffer size.
        //int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width/(float)height;

		//VR stuff
        //glViewport(0, 0, width*MSAAFACT, height* MSAAFACT);

        // Clear framebuffer.
        glClearColor(0.0, 0.0, 0.0, 1.0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Create the matrix stacks - please leave these alone for now
        
        glm::mat4 V, M, P; //View, Model and Perspective matrix
		mat4 camPos = cam_.process(frametime);
		V = VRheadmatrix * camPos;
        M = glm::mat4(1);
        // Apply orthographic projection....
        P = glm::perspective((float)(PI / 4.), (float)(aspect), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones

		vr::Hmd_Eye currenteye;
		if (eye == LEFTEYE)
			currenteye = vr::Eye_Left;
		else
			currenteye = vr::Eye_Right;
		if (!vrapp->get_projection_matrix(currenteye, 0.000001f, 4000.0f, P))
			P = glm::perspective((float)((float)PI / 4.), (float)((float)width / (float)height), 0.000001f, 4000.0f);

        
        static float angle = 0.;
		if (!paused) {
			angle += sin(frametime* bi_star_facts.y);
		}
        mat4 Ry = rotate(mat4(1), angle, vec3(0, 1, 0));
		float angle_offset = 0.0;
		mat4 Ry_bistars = rotate(mat4(1), -angle * 0.5f + bi_star_angle_off, vec3(0, 1, 0));

		float bi_star_distance = 0.28 * bi_star_facts.x;

		// Render binary system
		prog_binary_system->bind();
		//star 1:
		glUniformMatrix4fv(prog_binary_system->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog_binary_system->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		M = mat4(1);
		mat4 transM = translate(mat4(1), vec3(bi_star_distance*0.5,0,0));
		M = Ry_bistars * transM * scale(mat4(1), vec3(0.02));
		vec3 bStarOne = vec3(M[3]);
		glUniformMatrix4fv(prog_binary_system->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		lightpos.w = 1;
		glUniform4fv(prog_binary_system->getUniform("lightpos"), 1, &lightpos.x);
		colordot = vec3(0.8, 0.9, 2.6);
		glUniform3fv(prog_binary_system->getUniform("colordot"), 1, &colordot.x);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_sun);
		shape_gw_source_->draw(prog_binary_system, false);
		//star 2:
		transM = translate(mat4(1), vec3(-bi_star_distance * 0.5, 0, 0));
		M = Ry_bistars *transM * scale(mat4(1), vec3(0.02));
		vec3 bStarTwo = vec3(M[3]);
		glUniformMatrix4fv(prog_binary_system->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		colordot = vec3(1.1, 1.9, 1.0);
		glUniform3fv(prog_binary_system->getUniform("colordot"), 1, &colordot.x);
		shape_gw_source_->draw(prog_binary_system, false);
		prog_binary_system->unbind();

		// Render Earth
        prog_earth->bind();
        glUniformMatrix4fv(prog_earth->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(prog_earth->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        
        float sphere_advance_val_ = 0.005;
		if (!manualMode)
		{
			if (left_ == 1) {
				earth_pos_.x -= sphere_advance_val_;
			}
			else if (right_ == 1) {
				earth_pos_.x += sphere_advance_val_;
			}
			if (forward_ == 1) {
				earth_pos_.z -= sphere_advance_val_;
			}
			else if (backward_ == 1) {
				earth_pos_.z += sphere_advance_val_;
			}
			if (up_ == 1) {
				earth_pos_.y += sphere_advance_val_;
			}
			else if (down_ == 1) {
				earth_pos_.y -= sphere_advance_val_;
			}
		}
        
        mat4 translateSphere = translate(mat4(1), earth_pos_);
        float earthScale = 0.07;
        mat4 earthScaleM = scale(mat4(1), vec3(earthScale));
        M = mat4(1);
		M = translateSphere * rotate(mat4(1), (float) -PI, vec3(0, 1, 0)) * earthScaleM;
        mat4 earthMatrix = M;
        glUniformMatrix4fv(prog_earth->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(prog_earth->getUniform("Ry"), 1, GL_FALSE, &Ry[0][0]);
		glUniform2fv(prog_earth->getUniform("bi_star_facts"), 1, &bi_star_facts.x);
		lightpos.w = 0;
		glUniform4fv(prog_earth->getUniform("lightpos"), 1, &lightpos.x);
		colordot = vec3(1);
		glUniform3fv(prog_earth->getUniform("colordot"), 1, &colordot.x);
		glUniform2fv(prog_earth->getUniform("bi_star_facts"), 1,&bi_star_facts.x);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture_earth);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Texture_spiral);
        shape_earth_->draw(prog_earth,false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        prog_earth->unbind();

		// Render Moon
        prog_moon->bind();
        glUniformMatrix4fv(prog_moon->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(prog_moon->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        M = mat4(1);
		static float ang_moon = 0.;
		ang_moon += sin(frametime );
        mat4 Ryz = rotate(mat4(1), ang_moon, vec3(0, 1, 1));
        M = earthMatrix * Ryz * translate(mat4(1), vec3(2., 0, 0)) * Ry * scale(mat4(1), vec3(0.27));
        vec3 moonPos = vec3(M[3]);
        glUniformMatrix4fv(prog_moon->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(prog_moon->getUniform("Ry"), 1, GL_FALSE, &Ry[0][0]);
		lightpos.w = 0;
		glUniform4fv(prog_moon->getUniform("lightpos"), 1, &lightpos.x);
		colordot = vec3(1);
		glUniform3fv(prog_moon->getUniform("colordot"), 1, &colordot.x);
		//glUniform2fv(prog_moon->getUniform("bi_star_facts"), 1,&bi_star_facts.x);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture_moon);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Texture_spiral);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        shape_moon_->draw(prog_moon,false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        prog_moon->unbind();


        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
		// Render Grid
        M = mat4(1);//scale(mat4(1), vec3(0.5,0.5,0.5));
        prog_grid_x->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture_grid);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Texture_spiral);
        //send the matrices to the shaders
        glUniformMatrix4fv(prog_grid_x->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(prog_grid_x->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(prog_grid_x->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(prog_grid_x->getUniform("Ry"), 1, GL_FALSE, &Ry[0][0]);
        glUniform3fv(prog_grid_x->getUniform("SpherePos"), 1, &earth_pos_[0]);
		glUniform3fv(prog_grid_x->getUniform("MoonPos"), 1, &moonPos[0]);
		glUniform3fv(prog_grid_x->getUniform("BPosOne"), 1, &bStarOne[0]);
		glUniform3fv(prog_grid_x->getUniform("BPosTwo"), 1, &bStarTwo[0]);
        glUniform1f(prog_grid_x->getUniform("earthScale"), earthScale);
		glUniform2fv(prog_grid_x->getUniform("bi_star_facts"), 1,&bi_star_facts.x);
        glBindVertexArray(VAOX);
        glDrawArrays(GL_LINES, 0, grid_vertices_size);
        prog_grid_x->unbind();
        
        prog_grid_y->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture_grid);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Texture_spiral);
        //send the matrices to the shaders
        glUniformMatrix4fv(prog_grid_y->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(prog_grid_y->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(prog_grid_y->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(prog_grid_y->getUniform("Ry"), 1, GL_FALSE, &Ry[0][0]);
        glUniform3fv(prog_grid_y->getUniform("SpherePos"), 1, &earth_pos_[0]);
        glUniform3fv(prog_grid_y->getUniform("MoonPos"), 1, &moonPos[0]);
        glUniform1f(prog_grid_y->getUniform("earthScale"), earthScale);
		glUniform2fv(prog_grid_y->getUniform("bi_star_facts"), 1, &bi_star_facts.x);
        glBindVertexArray(VAOY);
        glDrawArrays(GL_LINES, 0, grid_vertices_size);
        prog_grid_y->unbind();
        
        prog_grid_z->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture_grid);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Texture_spiral);
        //send the matrices to the shaders
        glUniformMatrix4fv(prog_grid_z->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(prog_grid_z->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(prog_grid_z->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(prog_grid_z->getUniform("Ry"), 1, GL_FALSE, &Ry[0][0]);
        glUniform3fv(prog_grid_z->getUniform("SpherePos"), 1, &earth_pos_[0]);
        glUniform3fv(prog_grid_z->getUniform("MoonPos"), 1, &moonPos[0]);
        glUniform1f(prog_grid_z->getUniform("earthScale"), earthScale);
		glUniform2fv(prog_grid_z->getUniform("bi_star_facts"), 1, &bi_star_facts.x);
        glBindVertexArray(VAOZ);
        glDrawArrays(GL_LINES, 0, grid_vertices_size);
        prog_grid_z->unbind();

		// Render Controllers
		float controllerScale = 0.008f;
		prog_hand_left->bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_marble);
		glUniformMatrix4fv(prog_hand_left->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog_hand_left->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		lightpos.w = 0;
		glUniform4fv(prog_hand_left->getUniform("lightpos"), 1, &lightpos.x);
		colordot = vec3(1);
		glUniform3fv(prog_hand_left->getUniform("colordot"), 1, &colordot.x);

		M = mat4(1);
		M = translate(mat4(1), ControllerPosLeft) * scale(mat4(1), vec3(controllerScale));
		glUniformMatrix4fv(prog_hand_left->getUniform("M"), 1, GL_FALSE, &M[0][0]);

		// shape_hand_left->draw(prog_hand_left, false);
		prog_hand_left->unbind();

		prog_hand_right->bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_marble);
		glUniformMatrix4fv(prog_hand_right->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog_hand_right->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		lightpos.w = 1;
		glUniform4fv(prog_hand_right->getUniform("lightpos"), 1, &lightpos.x);
		colordot = vec3(1);
		glUniform3fv(prog_hand_right->getUniform("colordot"), 1, &colordot.x);

		M = mat4(1);
		// TODO: find actual values for proper translation to "ingame hmd space"
		// pos = glm::vec3(1.5, -0., -0.8);
		if (!manualMode) {
			vec3 transVec = vec3(-1.5f, -1.2f, 0.5f);// * 1.f/controllerScale;
			M = translate(mat4(1), controller_pos_right_ * (1.f) + transVec);// *translate(mat4(1), transVec);
		}
		else {
			if (left_ == 1) {
				manual_hand_pos_right_.x -= sphere_advance_val_;
			}
			else if (right_ == 1) {
				manual_hand_pos_right_.x += sphere_advance_val_;
			}
			if (forward_ == 1) {
				manual_hand_pos_right_.z -= sphere_advance_val_;
			}
			else if (backward_ == 1) {
				manual_hand_pos_right_.z += sphere_advance_val_;
			}
			if (up_ == 1) {
				manual_hand_pos_right_.y += sphere_advance_val_;
			}
			else if (down_ == 1) {
				manual_hand_pos_right_.y -= sphere_advance_val_;
			}
			M = translate(mat4(1), manual_hand_pos_right_);
		}
		M *= scale(mat4(1), vec3(controllerScale));
		mat4 handPosRightMat = M;
		vec3 handPosRightVec = vec3(M[3]); //vec3((V * M)[3]); // TODO: check why multiplied with V

		float amplitude = 0.f;

		//M = translate(mat4(1), vec3(-1.5f, 0.f, 1.f));
		//cout << "POS "; printVec(vec3(M[3])); cout << " trans: "; printVec(transVec); cout << " and diff "; printVec(vec3(M[3]) - transVec);
		glUniformMatrix4fv(prog_hand_right->getUniform("M"), 1, GL_FALSE, &M[0][0]);

		shape_hand_right->draw(prog_hand_right, false);
		prog_hand_right->unbind();

		//// proper amplitude calculation with compute shader

		prog_compute_shader->bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_spiral);		
		glUniformMatrix4fv(prog_compute_shader->getUniform("Ry"), 1, GL_FALSE, &Ry[0][0]);
		glUniform3fv(prog_compute_shader->getUniform("HandPos"), 1, &handPosRightVec.x);
		glUniform2fv(prog_compute_shader->getUniform("bi_star_facts"), 1, &bi_star_facts.x);
		// send additional data "down" to gpu with the buffer.
		//ssbo_CPUMEM.io[0] = vec4(-cam_.pos, 0);
		computeShader(); // prog_compute_shader->computeComputeShader();
		amplitude = ssbo_CPUMEM.io[0].w;
		// cout << "UL.r " << ssbo_CPUMEM.io[0].y << " LR.r " << ssbo_CPUMEM.io[0].z << endl;
		//amplitude = ssbo_CPUMEM.io[0].x;
		//printVec(ssbo_CPUMEM.io[0], "cpumem io vector: ");
		prog_compute_shader->unbind();
		////

		// Drawing text on hand position (previous M). extract to function and add for both hands?
		// round(amplitude * 10000.f) / 10000.f
		string amplString = RoundToString(amplitude, 4);
		static float minAmpl = 1.f;
		if (minAmpl > amplitude) minAmpl = amplitude;
		string minAmplString = RoundToString(minAmpl, 4);
		static float maxAmpl = 0.f;
		if (maxAmpl < amplitude) maxAmpl = amplitude;
		string maxAmplString = RoundToString(maxAmpl, 4);
		// cout << amplString << endl;// " ----- ";

		vec3 handTextOffset = vec3(0.015f, 0.015f, 0.f);
		vec4 screenpos = P * V * vec4(handPosRightVec + handTextOffset, 1);
		screenpos.x /= screenpos.w;
		screenpos.y /= screenpos.w;
		font->draw(screenpos.x, screenpos.y + 0.05f, 0.3f, (string)"Current: " + amplString, 1.f, 1.f, 1.f);
		font->draw(screenpos.x, screenpos.y - 0.05f, 0.3f, (string)"Max: " + maxAmplString, 1.f, 1.f, 1.f);
		font->draw(screenpos.x, screenpos.y - 0.15f, 0.3f, (string)"Min: " + minAmplString, 1.f, 1.f, 1.f);
		font->draw();

		//////// Gauge Rendering
		prog_gauge->bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_gauge);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Texture_indicator);
		//send the matrices to the shaders
		glUniformMatrix4fv(prog_gauge->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog_gauge->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		// no M needed here? 
		M = mat4(1);
		glUniformMatrix4fv(prog_gauge->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		// TODO: probably get proper Rotation matrix here
		glUniformMatrix4fv(prog_gauge->getUniform("RotM"), 1, GL_FALSE, &Ry[0][0]);
		glUniformMatrix4fv(prog_gauge->getUniform("gVP"), 1, GL_FALSE, &(P*V)[0][0]);
		vec3 camPosVec = vec3(camPos[3]);
		glUniform3fv(prog_gauge->getUniform("CamPos"), 1, &camPosVec.x);
		glUniform3fv(prog_gauge->getUniform("HandPos"), 1, &handPosRightVec.x);
		glBindVertexArray(VAOGauge);
		glDrawArrays(GL_POINTS, 0, 1);

		prog_gauge->unbind();
		//////// End Gauge Rendering

		if (false) {
			prog_debug->bind();
			P = V = mat4(1);
			M = scale(mat4(1), vec3(1., aspect, 1.));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Texture_spiral);
			glUniformMatrix4fv(prog_debug->getUniform("P"), 1, GL_FALSE, &P[0][0]);
			glUniformMatrix4fv(prog_debug->getUniform("V"), 1, GL_FALSE, &V[0][0]);
			glUniformMatrix4fv(prog_debug->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			glUniformMatrix4fv(prog_debug->getUniform("Ry"), 1, GL_FALSE, &Ry[0][0]);
			glBindVertexArray(VAODebug);
			glDrawArrays(GL_TRIANGLES, 0, 18);
			prog_debug->unbind();
		}
        
        // fix this, its not rendering anything??
//        prog_skybox->bind();
//        M =  glm::mat4(1);
//        // skybox cube
//        glBindVertexArray(skyboxVAO);
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
//        glDrawArrays(GL_TRIANGLES, 0, 36);
//        glBindVertexArray(0);
//        glDepthFunc(GL_LESS);
//        prog_skybox->unbind();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, FBO_MSAA_color);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    void render_postproc() // aka render to framebuffer
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        /*glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textarget, 0);
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, buffers);
        */
        glClearColor(1.0, 0.0, 0.0, 1.0);
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        glViewport(0, 0, width, height);


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        prog_post_proc->bind();
        glActiveTexture(GL_TEXTURE0);        
		glBindTexture(GL_TEXTURE_2D, FBO_MSAA_color);
        glBindVertexArray(VertexArrayIDBox);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        prog_post_proc->unbind();
        }
    //*************************************
};
Application* application = NULL; 


void my_render(int w, int h, glm::mat4 VRheadmatrix, int eye, bool VRon)
{
	application->render(w, h, VRheadmatrix, eye, VRon);
}
//******************************************************************************************
int main(int argc, char **argv)
{
	std::string resourceDir;// = "../../resources"; // Where the resources are loaded from
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}
	
    application = new Application();

    /* your main will always include a similar set up to establish your window
        and GL context, etc. */
    WindowManager * windowManager = new WindowManager();

	//VR Stuff
	vrapp = new OpenVRApplication();
	windowManager->init(vrapp->get_render_width(), vrapp->get_render_height());

	font->init();
	font->set_font_size(.07f);

	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

    /* This is the code that will likely change program to program as you
        may need to initialize or set up different data and state */
    // Initialize scene.
    application->init(resourceDir);
	application->initGeom();

	vrapp->init_buffers("../resources/shaders", MSAAFACT);

    // Loop until the user closes the window.
    while(! glfwWindowShouldClose(windowManager->getHandle()))
    {
        // Render scene.
		// 1 = left and 2 = right
		vr::HmdVector3_t controllerPos = vrapp->GetControllerPos(1);
		application->ControllerPosLeft = vec3(controllerPos.v[0], controllerPos.v[1], controllerPos.v[2]);
		controllerPos = vrapp->GetControllerPos(2);
		application->controller_pos_right_ = vec3(controllerPos.v[0], controllerPos.v[1], controllerPos.v[2]);

		vrapp->render_to_VR(my_render);
		vrapp->render_to_screen(1);//0..left eye, 1..right eye

        // Swap front and back buffers.
        glfwSwapBuffers(windowManager->getHandle());
        // Poll for and process events.
        glfwPollEvents();
    }

    // Quit program.
    windowManager->shutdown();
    return 0;
}
