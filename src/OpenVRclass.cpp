#include "OpenVRclass.h"
#include <iostream>
#include <string>


//*******************************************************************************************************************************************************
bool OpenVRApplication::init_buffers(string resourceDirectory, int msaa_fact)
{
		msaa = msaa_fact;
	//mesh first:
	//init rectangle mesh (2 triangles) for the post processing
	glGenVertexArrays(1, &FBOvao);
	glBindVertexArray(FBOvao);
	glGenBuffers(1, &FBOvbopos);
	glBindBuffer(GL_ARRAY_BUFFER, FBOvbopos);
	GLfloat *rectangle_vertices = new GLfloat[18];
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
	//generate vertex buffer to hand off to OGL
	glGenBuffers(1, &FBOvbotex);
	glBindBuffer(GL_ARRAY_BUFFER, FBOvbotex);
	GLfloat *rectangle_texture_coords = new GLfloat[12];
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

	float msaa_local = msaa;
	for (int i = 0; i < 4; i++)
	{
		if (i > 1)	msaa_local = 1;
		//RGBA8 2D texture, 24 bit depth texture, 256x256
		glGenTextures(1, &FBOtexture[i]);
		glBindTexture(GL_TEXTURE_2D, FBOtexture[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		//NULL means reserve texture memory, but texels are undefined
		//**** Tell OpenGL to reserve level 0
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rtWidth* msaa_local, rtHeight* msaa_local, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		//You must reserve memory for other mipmaps levels as well either by making a series of calls to
		//glTexImage2D or use glGenerateMipmapEXT(GL_TEXTURE_2D).
		//Here, we'll use :

		glGenerateMipmap(GL_TEXTURE_2D);
		glGenFramebuffers(1, &FBO[i]);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOtexture[i], 0);
		if (i < 2)
		{
			glGenRenderbuffers(1, &FBOdepth[i]);
			glBindRenderbuffer(GL_RENDERBUFFER, FBOdepth[i]);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, rtWidth* msaa_local, rtHeight* msaa_local);//1590,1893
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, FBOdepth[i]);
		}

		//Does the GPU support current FBO configuration?
		GLenum status;
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE:
			cout << "status framebuffer nr " << i << " : good";
			break;
		default:
			cout << "status framebuffer nr " << i << " : bad!!!!!!!!!!!!!!!!!!!!!!!!!";
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//program:
	prog.setVerbose(true);
	prog.setShaderNames(resourceDirectory + "/FBOvertex.glsl", resourceDirectory + "/FBOfragment.glsl");
	if (!prog.init())
	{
		std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
		exit(1);
	}
	prog.addUniform("texoff");
	prog.addAttribute("vertPos");
	prog.addAttribute("vertTex");
}
//*******************************************************************************************************************************************************
void OpenVRApplication::render_to_offsetFBO(int selectFBO)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO[selectFBO]);

	float aspect = rtWidth / (float)rtHeight;
	glViewport(0, 0, rtWidth, rtHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	prog.bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, FBOtexture[selectFBO - 2]);
	glBindVertexArray(FBOvao);
	vec2 texoff;
	if (selectFBO == LEFTPOST)
		texoff = vec2(eyedistance, 0);
	else if (selectFBO == RIGHTPOST)
		texoff = vec2(-eyedistance, 0);
	glUniform2fv(prog.getUniform("texoff"), 1, &texoff.x);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	prog.unbind();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, FBOtexture[selectFBO]);
	glGenerateMipmap(GL_TEXTURE_2D);
}
//*******************************************************************************************************************************************************
std::string GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL)
{
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";

	char *pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}
//*******************************************************************************************************************************************************

// Utility functions
//-----------------------------------------------------------------------------
// Purpose: Converts a SteamVR matrix to our local matrix class
//-----------------------------------------------------------------------------
mat4 OpenVRApplication::ConvertSteamVRMatrixToMat4(const vr::HmdMatrix34_t &matPose)
{
	mat4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return matrixObj;
}
//
void OpenVRApplication::PollEvent() {
	// Define a VREvent
	vr::VREvent_t event;
	if (hmd->PollNextEvent(&event, sizeof(event)))
	{
	}
}

//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and its state is true
//---------------------------------------------------------------------------------------------------------------------
bool OpenVRApplication::GetDigitalActionState(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bState;
}


//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and had a rising edge
//---------------------------------------------------------------------------------------------------------------------
bool OpenVRApplication::GetDigitalActionRisingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bChanged && actionData.bState;
}
OpenVRApplication::OpenVRApplication()
{
	hmd = NULL;

	if (!hmdIsPresent())
	{
		//throw std::runtime_error("Error : HMD not detected on the system");
		std::cout << "Error : HMD not detected on the system" << std::endl;
	}

	if (!vr::VR_IsRuntimeInstalled())
	{
		//throw std::runtime_error("Error : OpenVR Runtime not detected on the system");
		std::cout << "Error : OpenVR Runtime not detected on the system" << std::endl;
	}

	OpenVRApplication::initVR();

	if (!hmd)
	{
		rtWidth = 1280;
		rtHeight = 720;
		std::cout << "NO VR, screen only with suggested render target size : " << rtWidth << "x" << rtHeight << std::endl;
		return;
	}
	if (!vr::VRCompositor())
	{
		throw std::runtime_error("Unable to initialize VR compositor!\n ");
	}
	uint32_t w, h;
	hmd->GetRecommendedRenderTargetSize(&w, &h);
	rtWidth = w;
	rtHeight = h;
	std::cout << "Initialized HMD with suggested render target size : " << rtWidth << "x" << rtHeight << std::endl;
}

//*******************************************************************************************************************************************************

vr::TrackedDevicePose_t OpenVRApplication::submitFramesOpenGL(int leftEyeTex, int rightEyeTex, bool linear)
{
	if (!hmd)
	{
		throw std::runtime_error("Error : presenting frames when VR system handle is NULL");
	}

	vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
	//	vr::VRCompositor()->GetLastPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
	// Change Begin 
	// for somebody asking for the default figure out the time from now to photons. 
	float fSecondsSinceLastVsync;
	hmd->GetTimeSinceLastVsync(&fSecondsSinceLastVsync, NULL);
	float fDisplayFrequency = hmd->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
	float fFrameDuration = 1.f / fDisplayFrequency;
	float fVsyncToPhotons = hmd->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SecondsFromVsyncToPhotons_Float);
	float fPredictedSecondsFromNow = fFrameDuration * 2 - fSecondsSinceLastVsync + fVsyncToPhotons;
	//hmd->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, fPredictedSecondsFromNow, trackedDevicePose, vr::k_unMaxTrackedDeviceCount);
	// Change END

	///\todo the documentation on this is completely unclear.  I have no idea which one is correct...
	/// seems to imply that we always want Gamma in opengl because linear is for framebuffers that have been
	/// processed by DXGI...
	vr::EColorSpace colorSpace = linear ? vr::ColorSpace_Linear : vr::ColorSpace_Gamma;

	vr::Texture_t leftEyeTexture = { (void*)leftEyeTex, vr::TextureType_OpenGL, colorSpace };
	vr::Texture_t rightEyeTexture = { (void*)rightEyeTex, vr::TextureType_OpenGL, colorSpace };

	vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
	vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);

	vr::VRCompositor()->PostPresentHandoff();
	return trackedDevicePose[0];
}
//*******************************************************************************************************************************************************
void OpenVRApplication::handleVRError(vr::EVRInitError err)
{
	//throw std::runtime_error(vr::VR_GetVRInitErrorAsEnglishDescription(err));
	std::wcout << vr::VR_GetVRInitErrorAsEnglishDescription(err) << std::endl;
}
//*******************************************************************************************************************************************************
void OpenVRApplication::initVR()
{
	vr::EVRInitError err = vr::VRInitError_None;
	hmd = vr::VR_Init(&err, vr::VRApplication_Scene);
	
	if (err != vr::VRInitError_None)
	{
		//handleVRError(err);
		for(int i=0;i<3;i++)
			cout << "VR ERROR, HEADSET NOT FOUND!" << endl;
		cout << endl << "VR turned off, will render to screen with 720p!" << endl << endl;
		return;
	}

	std::clog << GetTrackedDeviceString(hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String) << std::endl;
	std::clog << GetTrackedDeviceString(hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String) << std::endl;

	vr::VRInput()->GetActionHandle("/actions/demo/in/TriggerHaptic", &m_actionTriggerHaptic);
	vr::VRInput()->GetActionHandle("/actions/demo/in/AnalogInput", &m_actionAnalongInput);
	vr::VRInput()->GetActionSetHandle("/actions/demo", &m_actionsetDemo);
	vr::VRInput()->GetActionHandle("/actions/demo/out/Haptic_Left", &m_rHand[Left].m_actionHaptic);
	vr::VRInput()->GetInputSourceHandle("/user/hand/left", &m_rHand[Left].m_source);
	vr::VRInput()->GetActionHandle("/actions/demo/in/Hand_Left", &m_rHand[Left].m_actionPose);

	vr::VRInput()->GetActionHandle("/actions/demo/out/Haptic_Right", &m_rHand[Right].m_actionHaptic);
	vr::VRInput()->GetInputSourceHandle("/user/hand/right", &m_rHand[Right].m_source);
	vr::VRInput()->GetActionHandle("/actions/demo/in/Hand_Right", &m_rHand[Right].m_actionPose);

}
//*******************************************************************************************************************************************************
vr::TrackedDevicePose_t  OpenVRApplication::render_to_VR(void(*renderfunction)(int, int, glm::mat4, int, bool))
{
	render_to_FBO(LEFTEYE, renderfunction);
	if (hmd)
	{
		render_to_FBO(RIGHTEYE, renderfunction);
		render_to_offsetFBO(LEFTPOST);
		render_to_offsetFBO(RIGHTPOST);
		pose = submitFramesOpenGL(FBOtexture[RIGHTEYE + 2], FBOtexture[LEFTEYE + 2]);
		// pulling this manually from main? probably should be done differently.. just commented to prevent double code.
		// GetCoords();
	}
	return pose;
}
//*******************************************************************************************************************************************************
void OpenVRApplication::render_to_screen(int texture_num)
{
	if (texture_num < 0 || texture_num>3 | hmd == NULL) texture_num = 0;
	glViewport(0, 0, rtWidth, rtHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	prog.bind();
	vec2 texoff = vec2(0, 0);
	glUniform2fv(prog.getUniform("texoff"), 1, &texoff.x);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, FBOtexture[texture_num]);
	glBindVertexArray(FBOvao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	prog.unbind();
}
//*******************************************************************************************************************************************************
void OpenVRApplication::render_to_FBO(int selectFBO, void(*renderfunction)(int, int, glm::mat4, int, bool))
{
	mat4 TR = mat4(1);
	bool VRon = false;
	if (hmd)
	{
		VRon = true;
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				TR[i][j] = pose.mDeviceToAbsoluteTracking.m[i][j];
	}
	//free VIEW
	mat4 vRx, vRy, vR;
	float w = 1;
	vRy = rotate(mat4(1), w, vec3(0, 1, 0));//viewrot
	vRx = rotate(mat4(1), w, vec3(1, 0, 0));
	vec4 eyeoffset;
	if (selectFBO == LEFTEYE)
		eyeoffset = vec4(-eyeconvergence, 0, 0, 0);
	else if (selectFBO == RIGHTEYE)
		eyeoffset = vec4(eyeconvergence, 0, 0, 0);

	//trackingM.set_transform_matrix
	TR[0][3] = TR[1][3] = TR[2][3] = 0;
	TR[3][0] = TR[3][1] = TR[3][2] = 0;

	TR[3][3] = 1;
	mat4 tTR = transpose(TR);

	eyeoffset = tTR * eyeoffset;

	mat4 vT;
	vT = translate(mat4(1), vec3(eyeoffset));

	mat4 viewM = vT * TR;

	/*
	pre:
zeigen: transversal, longitudinal wellen
VR tasks:
wellen beobachten
bi stern system welche wellen erzeugen beobachten
wellen beobachten bei versch abst und frequen
messen auf verschiedenen punkten in rot ebene und darueber/darunter
post:
1. welche wellenart
2. abstrahlung
3. amplitudenabh von star-abst und frequ?
4. auf einem punkt?
	*/

	glBindFramebuffer(GL_FRAMEBUFFER, FBO[selectFBO]);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Get current frame buffer size.
	float aspect = rtWidth / (float)rtHeight;
	glViewport(0, 0, rtWidth * msaa, rtHeight * msaa);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderfunction(rtWidth * msaa, rtHeight * msaa, viewM, selectFBO, VRon);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, FBOtexture[selectFBO]);
	glGenerateMipmap(GL_TEXTURE_2D);

}