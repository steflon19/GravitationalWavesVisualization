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

void OpenVRApplication::SetupControllers() {

	int numControllersInitialized = 0;
	for (int td = vr::k_unTrackedDeviceIndex_Hmd; td < vr::k_unMaxTrackedDeviceCount; td++) {
		if (hmd != NULL && hmd->IsTrackedDeviceConnected(td)) {
			vr::ETrackedDeviceClass td_class = hmd->GetTrackedDeviceClass(td);
			if (td_class == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller) {

				ControllerData* pC = &(controllers[numControllersInitialized]);

				int sHand = -1;

				vr::ETrackedControllerRole role = hmd->GetControllerRoleForTrackedDeviceIndex(td);
				if (role == vr::TrackedControllerRole_Invalid) //Invalid hand is actually very common, always need to test for invalid hand (lighthouses have lost tracking)
					sHand = 0;
				else if (role == vr::TrackedControllerRole_LeftHand)
					sHand = 1;
				else if (role == vr::TrackedControllerRole_RightHand)
					sHand = 2;
				pC->hand = sHand;
				pC->deviceId = td;
				numControllersInitialized++;
			}
		}
	}
}

void OpenVRApplication::PrintTrackedDevices() {
	for (int td = vr::k_unTrackedDeviceIndex_Hmd; td < vr::k_unMaxTrackedDeviceCount; td++) {
		if (hmd != NULL && hmd->IsTrackedDeviceConnected(td)) {
			vr::ETrackedDeviceClass td_class = hmd->GetTrackedDeviceClass(td);

			char* td_name = new char();
			hmd->GetStringTrackedDeviceProperty(td, vr::Prop_TrackingSystemName_String, td_name, vr::k_unMaxPropertyStringSize);
			// TODO: idk, maybe do something with this info..
			string sss = td_class == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller ? " IS CONTROLLER" : " ";
			cout << "some info, type " << td_class << " name: " << td_name << sss << endl;
		}
	}
}

void OpenVRApplication::PollEvent() {

	// Define a VREvent
	vr::VREvent_t event;
	if (hmd->PollNextEvent(&event, sizeof(event)))
	{
		HandleVRInput(event);
	}
}

void OpenVRApplication::HandleVRInput(const vr::VREvent_t& event) {
	// Touch B is k_EButton_ApplicationMenu ...

	// Process SteamVR action state
	// UpdateActionState is called each frame to update the state of the actions themselves. The application
	// controls which action sets are active with the provided array of VRActiveActionSet_t structs.
	/*vr::VRActiveActionSet_t actionSet = { 0 };
	actionSet.ulActionSet = m_actionsetDemo;
	vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);

	vr::VRInputValueHandle_t ulHapticDevice;
	cout << "did blah" << endl;
	if (GetDigitalActionRisingEdge(m_actionTriggerHaptic, &ulHapticDevice))
	{
		cout << "did something??? id: " << controller_index << endl;
		if (ulHapticDevice == m_rHand[Left].m_source)
		{
			vr::VRInput()->TriggerHapticVibrationAction(m_rHand[Left].m_actionHaptic, 0, 1, 4.f, 1.0f, vr::k_ulInvalidInputValueHandle);
		}
		if (ulHapticDevice == m_rHand[Right].m_source)
		{
			vr::VRInput()->TriggerHapticVibrationAction(m_rHand[Right].m_actionHaptic, 0, 1, 4.f, 1.0f, vr::k_ulInvalidInputValueHandle);
		}

		hmd->TriggerHapticPulse(controller_index, 0, 1.0f);
	} */

	switch (event.eventType)
	{
		// TODO: maybe some other stuff here
	default:
		HandleVRButtonEvent(event);
	}
}

void OpenVRApplication::HandleVRButtonEvent(vr::VREvent_t event) {
	char* buf = new char[100];
	if (event.eventType >= 200 && event.eventType <= 203)
		if (event.data.controller.button == vr::k_EButton_A && event.eventType == vr::VREvent_ButtonPress)
			cout << "handling A/X button press" << endl; // TODO: display stuff here probably
	else
		sprintf(buf, "\nEVENT--(OpenVR) Event: %d", event.eventType);
}

vr::HmdVector3_t OpenVRApplication::GetPosition(vr::HmdMatrix34_t matrix)
{
	vr::HmdVector3_t vector;

	vector.v[0] = matrix.m[0][3];
	vector.v[1] = matrix.m[1][3];
	vector.v[2] = matrix.m[2][3];

	return vector;
}

void OpenVRApplication::GetCoords() {
	SetupControllers();

	vr::TrackedDevicePose_t trackedDevicePose;
	vr::VRControllerState_t controllerState;
	vr::HmdQuaternion_t rot;


	for (int i = 0; i < 2; i++)
	{
		ControllerData* pC = &(controllers[i]);

		if (pC->deviceId < 0 ||
			!hmd->IsTrackedDeviceConnected(pC->deviceId) ||
			pC->hand </*=  Allow printing coordinates for invalid hand? Yes.*/ 0)
			continue;

		hmd->GetControllerStateWithPose(vr::TrackingUniverseStanding, pC->deviceId, &controllerState, sizeof(controllerState), &trackedDevicePose);
		pC->pos = GetPosition(trackedDevicePose.mDeviceToAbsoluteTracking);
		// std::cout << "device pos " << pC->pos.v[0] << " and " << pC->pos.v[1] << " and " << pC->pos.v[2] << std::endl;
		//rot = GetRotation(trackedDevicePose.mDeviceToAbsoluteTracking);
	}
}

vr::HmdVector3_t OpenVRApplication::GetControllerPos(int hand_type) {
	GetCoords();
	if (hand_type > 2 || hand_type < 1) cout << "Error, invalid controllers index " << hand_type << ". only 1 or 2 valid" << endl;
	// maybe some error handling if only one or no hand is tracked?
	ControllerData* pC = &(controllers[0]);
	if(!hmd)
		return vr::HmdVector3_t();

	if (pC->hand == hand_type)
		// TODO: improve duplicate code test.
	{
		return pC->pos;
	}
	else {
		pC = &(controllers[1]);

		if (pC->hand == hand_type) {
			return pC->pos;
		}
		else
			return vr::HmdVector3_t();
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
	vr::VRInput()->GetActionSetHandle("/actions/demo", &m_actionsetDemo);
	PrintTrackedDevices();

}
//*******************************************************************************************************************************************************
vr::TrackedDevicePose_t  OpenVRApplication::render_to_VR(void(*renderfunction)(int, int, glm::mat4))
{
	render_to_FBO(LEFTEYE, renderfunction);
	if (hmd)
	{
		render_to_FBO(RIGHTEYE, renderfunction);
		render_to_offsetFBO(LEFTPOST);
		render_to_offsetFBO(RIGHTPOST);
		pose = submitFramesOpenGL(FBOtexture[RIGHTEYE + 2], FBOtexture[LEFTEYE + 2]);
		PollEvent();
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
void OpenVRApplication::render_to_FBO(int selectFBO, void(*renderfunction)(int, int, glm::mat4))
{
	mat4 TR = mat4(1);
	if (hmd)
	{
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
	TR[3][3] = 1;
	mat4 tTR = transpose(TR);

	eyeoffset = tTR * eyeoffset;

	mat4 vT;
	vT = translate(mat4(1), vec3(eyeoffset));

	mat4 viewM = vT * TR;

	glBindFramebuffer(GL_FRAMEBUFFER, FBO[selectFBO]);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Get current frame buffer size.
	float aspect = rtWidth / (float)rtHeight;
	glViewport(0, 0, rtWidth*msaa, rtHeight * msaa);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderfunction(rtWidth * msaa, rtHeight * msaa, TR);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, FBOtexture[selectFBO]);
	glGenerateMipmap(GL_TEXTURE_2D);

}