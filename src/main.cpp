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

#include "WindowManager.h"
#include "Shape.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;
#ifndef BYTE
typedef bitset<8> BYTE;
#endif
#define MSAAFACT 1

double get_last_elapsed_time()
{
    static double lasttime = glfwGetTime();
    double actualtime = glfwGetTime();
    double difference = actualtime- lasttime;
    lasttime = actualtime;
    return difference;
}
class Camera
{
public:
    glm::vec3 pos, rot;
    int w, a, s, d, space, c, shift_active;
    Camera()
    {
        w = a = s = d = 0;
        rot = glm::vec3(0, 0, 0);
        pos = glm::vec3(1.5, -0., -0.8);
    }
    glm::mat4 process(double ftime)
    {
        float speed = 0;
        float speedY = 0;
        if (shift_active == 0 && w == 1)
        {
            speed = 1*ftime;
        }
        else if (shift_active == 0 && s == 1)
        {
            speed = -1*ftime;
        }
        if (space == 1)
        {
            speedY = -1*ftime;
        }
        else if (c == 1)
        {
            speedY = 1*ftime;
        }
        float angle_y=0;
        if (a == 1) {
            angle_y = -1*ftime;
        }
        else if(d==1) {
            angle_y = 1*ftime;
        }
        float angle_z = 0;
        if (shift_active == 1 && w == 1) {
            angle_z = 1*ftime;
        }
        else if(shift_active == 1 && s == 1) {
            angle_z = -1*ftime;
        }
        rot.y += angle_y;
        rot.x += angle_z;
        glm::mat4 R_Y = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
        glm::mat4 R_X = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
        glm::vec4 dir = glm::vec4(0, speedY, speed,1);
        dir = dir * R_Y * R_X;
        pos += glm::vec3(dir.x, dir.y, dir.z);
        glm::mat4 T = glm::translate(glm::mat4(1), pos);
        return R_X * R_Y * T;
    }
};

class Application : public EventCallbacks
{
private:
    shared_ptr<Shape> shape_earth_, shape_moon_, shape_gw_source_;

    // sphere controls
    GLuint left_, right_, forward_, backward_, up_, down_;
    float  earth_dir_x_, earth_dir_y_, earth_dir_z_;
    
    Camera cam_;

public:
    
    const std::string resourceDir = "../../resources";
    const std::string shaderDir = "../../resources/shaders";
    WindowManager * windowManager = nullptr;

    // Our shader program
    std::shared_ptr<Program> prog_grid_x;
    std::shared_ptr<Program> prog_grid_y;
    std::shared_ptr<Program> prog_grid_z;
    std::shared_ptr<Program> prog_earth;
    std::shared_ptr<Program> prog_moon;
    std::shared_ptr<Program> prog_skybox;
    std::shared_ptr<Program> prog_gw_source, postprocprog;
    
    std::shared_ptr<Program> prog_box_DEBUG;

    // Contains vertex information for OpenGL
    GLuint VertexArrayID;

    // Data necessary to give our box to OpenGL
    GLuint VertexBufferID, VertexColorIDBox, IndexBufferIDBox;

    //texture data
    GLuint Texture_grid, Texture_earth, Texture_sun, Texture_spiral, Texture_moon;
    GLuint cubemapTexture, FBO_MSAA, FBO_MSAA_depth, FBO_MSAA_color, VertexArrayIDBox, VertexBufferIDBox;


    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        
        // utility
        if(key == GLFW_KEY_R && action == GLFW_RELEASE) {
            earth_dir_x_ = -1.5;
            earth_dir_z_ = 0.2;
            earth_dir_y_ = 0.;
        }
        
        if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
        {
            cam_.shift_active = 1;
        }
        if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE)
        {
            cam_.shift_active = 0;
        }
        
        // CAMERA
        // move left/right
        if (key == GLFW_KEY_W && action == GLFW_PRESS)
        {
            cam_.w = 1;
        }
        if (key == GLFW_KEY_W && action == GLFW_RELEASE)
        {
            cam_.w = 0;
        }
        if (key == GLFW_KEY_S && action == GLFW_PRESS)
        {
            cam_.s = 1;
        }
        if (key == GLFW_KEY_S && action == GLFW_RELEASE)
        {
            cam_.s = 0;
        }
        // turn left/right
        if (key == GLFW_KEY_A && action == GLFW_PRESS)
        {
            cam_.a = 1;
        }
        if (key == GLFW_KEY_A && action == GLFW_RELEASE)
        {
            cam_.a = 0;
        }
        if (key == GLFW_KEY_D && action == GLFW_PRESS)
        {
            cam_.d = 1;
        }
        if (key == GLFW_KEY_D && action == GLFW_RELEASE)
        {
            cam_.d = 0;
        }
        // move up/down
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        {
            cam_.space = 1;
        }
        if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
        {
            cam_.space = 0;
        }
        if (key == GLFW_KEY_C && action == GLFW_PRESS)
        {
            cam_.c = 1;
        }
        if (key == GLFW_KEY_C && action == GLFW_RELEASE)
        {
            cam_.c = 0;
        }
        
        // Sphere
        if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
        {
            left_ = 1;
        }
        if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE)
        {
            left_ = 0;
        }
        if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
        {
            right_ = 1;
        }
        if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE)
        {
            right_ = 0;
        }
        if (key == GLFW_KEY_UP && action == GLFW_PRESS)
        {
            forward_ = 1;
        }
        if (key == GLFW_KEY_UP && action == GLFW_RELEASE)
        {
            forward_ = 0;
        }
        if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        {
            backward_ = 1;
        }
        if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE)
        {
            backward_ = 0;
        }
        if (key == GLFW_KEY_O && action == GLFW_PRESS)
        {
            up_ = 1;
        }
        if (key == GLFW_KEY_O && action == GLFW_RELEASE)
        {
            up_ = 0;
        }
        if (key == GLFW_KEY_L && action == GLFW_PRESS)
        {
            down_ = 1;
        }
        if (key == GLFW_KEY_L && action == GLFW_RELEASE)
        {
            down_ = 0;
        }
    }

    // callback for the mouse when clicked move the triangle when helper functions
    // written
    void mouseCallback(GLFWwindow *window, int button, int action, int mods)
    {
        double posX, posY;
        float newPt[2];
        if (action == GLFW_PRESS)
        {
            glfwGetCursorPos(window, &posX, &posY);
            std::cout << "Pos X " << posX <<  " Pos Y " << posY << std::endl;

            //change this to be the points converted to WORLD
            //THIS IS BROKEN< YOU GET TO FIX IT - yay!
            newPt[0] = 0;
            newPt[1] = 0;

            std::cout << "converted:" << newPt[0] << " " << newPt[1] << std::endl;
            glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
            //update the vertex array with the updated points
            glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*6, sizeof(float)*2, newPt);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
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
    
    // TODO: there has to be a way to do this better? oO
    GLuint VAOX, VAOY, VAOZ, VBOX, VBOY, VBOZ;
    GLuint VAODebug, VBODebug;
    GLuint skyboxVAO, skyboxVBO;
    int grid_vertices_size;
    void initGeom()
    {
        // generating the grid
        std::vector<vec3> grid_x, grid_y, grid_z;
        float step = 0.125/2;
        float gridSize = 2.;
        // TODO; increase this later for better curves, for now 2 for better performance
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
        // previous code..
        //                        pos.push_back(vec3(x, y, z));
        //                        pos.push_back(vec3(x, y+step, z));
        //                        pos.push_back(vec3(x, y, z));
        //                        pos.push_back(vec3(x+step, y , z));
        //                        pos.push_back(vec3(x, y, z));
        //                        pos.push_back(vec3(x , y, z+step));
        
        // TODO: add "closing" line (just because) at the end.
        
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



        string resourceDir = "../../resources" ;
        string shadersDirectory = "../../resources/shaders";
        // Initialize mesh.
        shape_earth_ = make_shared<Shape>();
        shape_earth_->loadMesh(resourceDir + "/sphere2.obj");
        shape_earth_->resize();
        shape_earth_->init();
        
        shape_moon_ = make_shared<Shape>();
        shape_moon_->loadMesh(resourceDir + "/sphere2.obj");
        shape_moon_->resize();
        shape_moon_->init();

        // this shoudl be obsolete later, just now to visualize the source of gravitational waves
        shape_gw_source_ = make_shared<Shape>();
        //shape->loadMesh(resourceDir + "/t800.obj");
        shape_gw_source_->loadMesh(resourceDir + "/sphere.obj");
        shape_gw_source_->resize();
        shape_gw_source_->init();
        
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
        GLuint Tex3Location = glGetUniformLocation(prog_gw_source->pid, "tex_sphere");
        glUseProgram(prog_gw_source->pid);
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
        
//        glBindVertexArray(0);
        
//        texture spiral
        str = resourceDir + "/double_spiral_small.png";
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
        GLuint Tex4Location = glGetUniformLocation(prog_box_DEBUG->pid, "tex");
        glUseProgram(prog_box_DEBUG->pid);
        glUniform1i(Tex4Location, 0);
        
        glBindVertexArray(0);
        
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
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1, rectangle_vertices[verccount++] = 0.0;
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
        prog->setShaderNames(shaderDir + shaderv.at(0), shaderDir + shaderv.at(1), shaderv.size() > 2 ? shaderDir + shaderv.at(2) : "");
        
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

    //General OGL initialization - set OGL state here
    void init()
    {
        GLSL::checkVersion();
        left_ =  right_ = forward_ = backward_ = 0;
        earth_dir_x_ = -1.5;
        earth_dir_z_ = 0.2;
        earth_dir_y_ = 0.;
        // Set background color.
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        // Enable z-buffer test.
        glEnable(GL_DEPTH_TEST);

        // Initialize the GLSL program.
        prog_grid_x = make_shared<Program>();
        prog_grid_y = make_shared<Program>();
        prog_grid_z = make_shared<Program>();
        prog_earth = make_shared<Program>();
        prog_moon = make_shared<Program>();
        prog_gw_source = make_shared<Program>();
        postprocprog = make_shared<Program>();
        
        prog_box_DEBUG = make_shared<Program>();
        
        initProgram(prog_grid_x,
                    vector<string>({"/shader_vertex_grid.glsl", "/shader_fragment_grid.glsl", "/shader_geometry_x.glsl"}),
                    vector<string>({"P", "V", "M", "Ry","SpherePos", "MoonPos", "earthScale"}));
        
        initProgram(prog_grid_y,
                    vector<string>({"/shader_vertex_grid.glsl", "/shader_fragment_grid.glsl", "/shader_geometry_y.glsl"}),
                    vector<string>({"P", "V", "M", "Ry","SpherePos", "MoonPos", "earthScale"}));
        
        initProgram(prog_grid_z,
                    vector<string>({"/shader_vertex_grid.glsl", "/shader_fragment_grid.glsl", "/shader_geometry_z.glsl"}),
                    vector<string>({"P", "V", "M", "Ry","SpherePos", "MoonPos", "earthScale"}));
    
    
        initProgram(prog_earth,
                    vector<string>({"/shader_vertex_earth.glsl", "/shader_fragment_sphere.glsl"}),
                    vector<string>({"P", "V", "M", "Ry"}),
                    vector<string>({"vertPos", "vertNor", "vertTex"}));
                
        initProgram(prog_moon,
                    vector<string>({"/shader_vertex_earth.glsl", "/shader_fragment_sphere.glsl"}),
                    vector<string>({"P", "V", "M", "Ry"}),
                    vector<string>({"vertPos", "vertNor", "vertTex"}));
        

                
        initProgram(prog_gw_source,
//                    vector<string>({"vertPos","vertNor", "vertTex"}),
                    vector<string>({"/shader_vertex_sphere.glsl", "/shader_fragment_sphere.glsl"}),
                    vector<string>({"P", "V", "M"}),
                    vector<string>({"vertPos","vertNor", "vertTex"}));
//        prog_gw_source->addUniform("campos");


//        initProgram(prog_box_DEBUG,
////                    vector<string>({"angle"}),
//                    vector<string>({"/shader_vertex_debug.glsl", "/shader_fragment_debug.glsl"}));
//        prog_box_DEBUG->addUniform("angle");

        initProgram(postprocprog,
                    vector<string>({ "/ppvert.glsl", "/ppfrag.glsl" }));

        


        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    
    void printVec(vec3 vec, std::string name = "vec3") {
        std::cout << name << " x: " << vec.x << " y: " << vec.y << " z: " << vec.z << std::endl;
    }
    void printVec(vec4 vec, std::string name = "vec4") {
        std::cout << name << " x: " << vec.x << " y: " << vec.y << " z: " << vec.z << " [3]: " << vec[3] << std::endl;
    }
    
    /****DRAW
    This is the most important function in your program - this is where you
    will actually issue the commands to draw any geometry you have set up to
    draw
    ********/
    void render()
    {

        glBindFramebuffer(GL_FRAMEBUFFER, FBO_MSAA);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBO_MSAA_color, 0);
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, buffers);
        
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);
        double frametime = get_last_elapsed_time();

        // Get current frame buffer size.
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width/(float)height;
        glViewport(0, 0, width*MSAAFACT, height* MSAAFACT);

        // Clear framebuffer.
        glClearColor(0.0, 0.0, 0.0, 1.0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Create the matrix stacks - please leave these alone for now
        
        glm::mat4 V, M, P; //View, Model and Perspective matrix
        V = cam_.process(frametime);
        M = glm::mat4(1);
        // Apply orthographic projection....
        P = glm::perspective((float)(PI / 4.), (float)(aspect), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones

        
        static float ang = 0.;
        ang += sin(frametime);
        mat4 Ry = rotate(mat4(1), ang, vec3(0, 1, 0));
        
        prog_earth->bind();
        glUniformMatrix4fv(prog_earth->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(prog_earth->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        
        float advanceVal = 0.005;
        if (left_ == 1) {
            earth_dir_x_ -= advanceVal;
        } else if (right_ == 1) {
            earth_dir_x_ += advanceVal;
        }
        if (forward_ == 1) {
            earth_dir_z_ -= advanceVal;
        } else if (backward_ == 1) {
            earth_dir_z_ += advanceVal;
        }
        if (up_ == 1) {
            earth_dir_y_ += advanceVal;
        } else if (down_ == 1) {
            earth_dir_y_ -= advanceVal;
        }
        
        
        vec3 moveSphere = vec3(earth_dir_x_, earth_dir_y_, earth_dir_z_-0.1);
        mat4 translateSphere = translate(mat4(1), moveSphere);
        float earthScale = 0.07;
        mat4 scaleM = scale(mat4(1), vec3(earthScale));
        M = mat4(1);
        M = translateSphere * rotate(mat4(1),-3.1415926f,vec3(0,1,0)) * scaleM;
        mat4 earthMatrix = M;
        glUniformMatrix4fv(prog_earth->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(prog_earth->getUniform("Ry"), 1, GL_FALSE, &Ry[0][0]);
//        glUniform3fv(prog_earth->getUniform("campos"), 1, &cam_.pos[0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture_earth);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Texture_spiral);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        shape_earth_->draw(prog_earth,false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        prog_earth->unbind();
        
        // TODO: insert moon draw here
        prog_moon->bind();
        glUniformMatrix4fv(prog_moon->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(prog_moon->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        M = mat4(1);
        mat4 Ryz = rotate(mat4(1), ang, vec3(0, 1, 1));
        M = earthMatrix * Ryz * translate(mat4(1), vec3(2., 0, 0)) * Ry * scale(mat4(1), vec3(0.27));
        vec3 moonPos = vec3(M[3]);
//        M *= ;
        glUniformMatrix4fv(prog_moon->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(prog_moon->getUniform("Ry"), 1, GL_FALSE, &Ry[0][0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture_moon);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Texture_spiral);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        shape_moon_->draw(prog_moon,false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        prog_moon->unbind();

        prog_gw_source->bind();
        glUniformMatrix4fv(prog_gw_source->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(prog_gw_source->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        M = mat4(1);
        mat4 transM = translate(mat4(1), vec3(0.,0.,0.));
        M = transM * scale(mat4(1), vec3(.1,.1,.1));
        glUniformMatrix4fv(prog_gw_source->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//        glUniform3fv(prog_earth->getUniform("campos"), 1, &cam_.pos[0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture_sun);
        
        shape_gw_source_->draw(prog_gw_source, false);
        prog_gw_source->unbind();
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        //M = mat4(1);
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
        glUniform3fv(prog_grid_x->getUniform("SpherePos"), 1, &moveSphere[0]);
        glUniform3fv(prog_grid_x->getUniform("MoonPos"), 1, &moonPos[0]);
        glUniform1f(prog_grid_x->getUniform("earthScale"), earthScale);
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
        glUniform3fv(prog_grid_y->getUniform("SpherePos"), 1, &moveSphere[0]);
        glUniform3fv(prog_grid_y->getUniform("MoonPos"), 1, &moonPos[0]);
        glUniform1f(prog_grid_y->getUniform("earthScale"), earthScale);
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
        glUniform3fv(prog_grid_z->getUniform("SpherePos"), 1, &moveSphere[0]);
        glUniform3fv(prog_grid_z->getUniform("MoonPos"), 1, &moonPos[0]);
        glUniform1f(prog_grid_z->getUniform("earthScale"), earthScale);
        glBindVertexArray(VAOZ);
        glDrawArrays(GL_LINES, 0, grid_vertices_size);
        prog_grid_z->unbind();

//        prog_box_DEBUG->bind();
//        P = V = mat4(1);
//        M = scale(mat4(1),vec3(1.,aspect,1.));
//        glUniformMatrix4fv(prog_box_DEBUG->getUniform("P"), 1, GL_FALSE, &P[0][0]);
//        glUniformMatrix4fv(prog_box_DEBUG->getUniform("V"), 1, GL_FALSE, &V[0][0]);
//        glUniformMatrix4fv(prog_box_DEBUG->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//        glUniform1f(prog_box_DEBUG->getUniform("angle"), ang);
//        glBindVertexArray(VAODebug);
//        glDrawArrays(GL_TRIANGLES, 0, 18);
//        prog_box_DEBUG->unbind();
        
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

        postprocprog->bind();
        glActiveTexture(GL_TEXTURE0);        glBindTexture(GL_TEXTURE_2D, FBO_MSAA_color);
        glBindVertexArray(VertexArrayIDBox);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        postprocprog->unbind();
        }
    //*************************************
};


//******************************************************************************************
int main(int argc, char **argv)
{
//    const std::string resourceDir = "../../resources";
//    if (argc >= 2)
//    {
//        resourceDir = argv[1];
//    }

    Application *application = new Application();

    /* your main will always include a similar set up to establish your window
        and GL context, etc. */
    WindowManager * windowManager = new WindowManager();
    windowManager->init(1920, 1080);
    windowManager->setEventCallbacks(application);
    application->windowManager = windowManager;

    /* This is the code that will likely change program to program as you
        may need to initialize or set up different data and state */
    // Initialize scene.
    application->init();
    application->initGeom();

    // Loop until the user closes the window.
    while(! glfwWindowShouldClose(windowManager->getHandle()))
    {
        // Render scene.
        application->render();
        application->render_postproc();
        // Swap front and back buffers.
        glfwSwapBuffers(windowManager->getHandle());
        // Poll for and process events.
        glfwPollEvents();
    }

    // Quit program.
    windowManager->shutdown();
    return 0;
}
