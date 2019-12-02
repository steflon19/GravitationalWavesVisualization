//##############################################################################################################################################################
//
//
//						OpenVR class for OpenGL application
//
//						Last modified: 11/26/2018
//
///			By Christian Eckhardt, ceckhard@calpoly.edu
//
//
///			Code uses the program class for "easier" shader implementation, which comes with this package
///			Program class by Ian Dunn and Zoe" Wood (Cal Poly).
//
//
///			incudes:		OpenVRclass.cpp
///							OpenVRclass.h
///							Program.cpp
///							Program.h
///							FBOvertex.glsl
///							FBOfragment.glsl
//
//--------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//			USAGE
//
//			0. Download (in case you do not work on a lab PC with pre-installed libs but on your own laptop/PC):
//				0.a		OpenVR libs here: https://github.com/ValveSoftware/openvr
///						Install and link the correct libs to your project (MacOS? Win86? Win64? ...)
//				0.b		SteamVR here (its the usual Steam client if you have none): https://steamcommunity.com/steamvr
///						It's the button "Install Steam" on top (as of 11/2018)
//
//			1. Make an pointer of the VR class in your application (main file):
///				OpenVRApplication *vrapp = NULL;
//
//			2. After having created the window, make an instance:
///				vrapp = new OpenVRApplication;
//
//			2.5 Initialize your screen-window with the size given from the VR class
///				i.e.: windowManager->init(vrapp->get_render_width(), vrapp->get_render_height());
//
//
//			3. After initialize OpenGL, initialize the buffers for the VR class:
///				vrapp->init_buffers(resourceDirectory);//resourceDirectory .. where the GLSL shader files can be found
//
//			4. write your render function:
///				your_render_fct(int width, int height, glm::mat4 VRheadmatrix);	//the paramteres will be provided by the class itself later
///				{
///				DO NOT SET following stuff in here (because it will be set by the class):
///					- framebuffer
///					- viewport
///				assign your view matrix (or multiply it) with VRheadmatrix
///				render stuff
///				}
//
//			(4.5 In case your render function is a method, call it from a function with the same parameter list, 
//				because we need to pass your render function as function pointer!)
//
//			5. Render to VR (and screen):
///				in your main loop:
///					vrapp->render_to_VR(your_render_fct);
///					vrapp->render_to_screen(1);//0..left eye, 1..right eye
//
//
//##############################################################################################################################################################


#pragma oncex
#include <openvr.h>
#include <string>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include "Program.h"

using namespace std;
using namespace glm;

enum emeye :int { LEFTEYE = 0, RIGHTEYE = 1, LEFTPOST = 2, RIGHTPOST = 3 };


class OpenVRApplication
{
private:
	float eyeconvergence = 0.1;		// convergence point
	float eyedistance = 0.08;		//3D intesity effec
	Program prog;					//shader program for the postprocessing stages
	vr::TrackedDevicePose_t pose;	//matrix from the headset tracking

	void render_to_FBO(int selectFBO, void(*renderfunction)(int, int, glm::mat4));
	unsigned int FBO[4], FBOtexture[4], FBOdepth[2];
	unsigned int  FBOvao, FBOvbopos, FBOvbotex;
	void render_to_offsetFBO(int selectFBO);

	vr::IVRSystem* hmd = NULL;
	int rtWidth = 0;
	int rtHeight = 0;

	inline static bool hmdIsPresent() { return vr::VR_IsHmdPresent(); }
	vr::TrackedDevicePose_t submitFramesOpenGL(int leftEyeTex, int rightEyeTex, bool linear = false);
	void handleVRError(vr::EVRInitError err);
	void initVR();
public:
	int get_render_width() { return rtWidth; }
	int get_render_height() { return rtHeight; }
	void PrintTrackedDevices();
	bool HandleInput();
	OpenVRApplication();
	bool init_buffers(string resourceDirectory);
	virtual OpenVRApplication::~OpenVRApplication()
	{
		if (hmd)
		{
			vr::VR_Shutdown();
			hmd = NULL;
		}
	}
	vr::TrackedDevicePose_t  render_to_VR(void(*renderfunction)(int, int, glm::mat4));
	unsigned int get_FBO_texture(int i) { if (i < 0 || i>3)return 0; return FBOtexture[i]; }
	void render_to_screen(int texture_num);
};

