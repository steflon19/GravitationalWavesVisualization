
#include "Program.h"
#include <iostream>
#include <cassert>
#include <fstream>

#include "GLSL.h"

std::string readFileAsString(const std::string &fileName)
{
	std::string result;
	std::ifstream fileHandle(fileName);

	if (fileHandle.is_open())
	{
		fileHandle.seekg(0, std::ios::end);
		result.reserve((size_t)fileHandle.tellg());
		fileHandle.seekg(0, std::ios::beg);

		result.assign((std::istreambuf_iterator<char>(fileHandle)), std::istreambuf_iterator<char>());
	}
	else
	{
		std::cerr << "Could not open file: '" << fileName << "'" << std::endl;
	}

	return result;
}

void Program::setShaderNames(const std::string &v, const std::string &f, const std::string &g)
{
	vShaderName = v;
	fShaderName = f;
	gShaderName = g;
}

void Program::setShaderNames(const std::string &v, const std::string &f)
{
	vShaderName = v;
	fShaderName = f;
}
void Program::setShaderNames(const std::string &c) {
	cShaderName = c;
	std::cout << "SETTING C SHADER PROPERLY ??!" << c << std::endl;
	//cs_ssbo_data;
}
bool Program::init()
{
	GLint rc;

	// Create shader handles
	GLint VS = -1;// glCreateShader(GL_VERTEX_SHADER);
	GLint FS = -1;// glCreateShader(GL_FRAGMENT_SHADER);
	GLint GS = -1;
	GLint CS = -1;

	// Read shader sources
	std::string vShaderString = readFileAsString(vShaderName);
	std::string fShaderString = readFileAsString(fShaderName);
	std::string gShaderString = readFileAsString(gShaderName);
	std::string cShaderString = readFileAsString(cShaderName);
	const char *vshader = vShaderString.c_str();
	const char *fshader = fShaderString.c_str();
	const char *gshader = gShaderString.c_str();
	const char *cshader = cShaderString.c_str();

	if (vShaderString.size() > 0) {
		VS = glCreateShader(GL_VERTEX_SHADER);
		// TODO: here was once an error, maybe occurs again?
		std::cout << "setting vshader? " << vShaderName << " and VS " << VS << std::endl;
		CHECKED_GL_CALL(glShaderSource(VS, 1, &vshader, NULL));
	}
	if (fShaderString.size() > 0) {
		FS = glCreateShader(GL_FRAGMENT_SHADER);
		CHECKED_GL_CALL(glShaderSource(FS, 1, &fshader, NULL));
	}
	if (gShaderString.size() > 0)
	{
		GS = glCreateShader(GL_GEOMETRY_SHADER);
		CHECKED_GL_CALL(glShaderSource(GS, 1, &gshader, NULL));
	}
	if (cShaderString.size() > 0)
	{
		CS = glCreateShader(GL_COMPUTE_SHADER);
		CHECKED_GL_CALL(glShaderSource(CS, 1, &cshader, NULL));
	}
	if (VS >= 0) {
		// Compile vertex shader
		CHECKED_GL_CALL(glCompileShader(VS));
		CHECKED_GL_CALL(glGetShaderiv(VS, GL_COMPILE_STATUS, &rc));
		if (!rc)
		{
			if (isVerbose())
			{
				GLSL::printShaderInfoLog(VS);
				std::cout << "Error compiling vertex shader " << vShaderName << std::endl;
			}
			return false;
		}
	}
	// Compile geometry shader
	if (GS >= 0)
	{
		CHECKED_GL_CALL(glCompileShader(GS));
		CHECKED_GL_CALL(glGetShaderiv(GS, GL_COMPILE_STATUS, &rc));
		if (!rc)
		{
			if (isVerbose())
			{
				GLSL::printShaderInfoLog(GS);
				std::cout << "Error compiling geometry shader " << gShaderName << std::endl;
			}
			return false;
		}
	}
	if (FS >= 0) {
		// Compile fragment shader
		CHECKED_GL_CALL(glCompileShader(FS));
		CHECKED_GL_CALL(glGetShaderiv(FS, GL_COMPILE_STATUS, &rc));
		if (!rc)
		{
			if (isVerbose())
			{
				GLSL::printShaderInfoLog(FS);
				std::cout << "Error compiling fragment shader " << fShaderName << std::endl;
			}
			return false;
		}
	}
	if (CS >= 0) {
		// Compile compute shader
		CHECKED_GL_CALL(glCompileShader(CS));
		CHECKED_GL_CALL(glGetShaderiv(CS, GL_COMPILE_STATUS, &rc));
		if (!rc)
		{
			if (isVerbose())
			{
				GLSL::printShaderInfoLog(CS);
				std::cout << "Error compiling compute shader " << cShaderName << std::endl;
			}
			return false;
		}
	}


	// Create the program and link
	pid = glCreateProgram();
	if (VS >= 0)
		CHECKED_GL_CALL(glAttachShader(pid, VS));
	if (FS >= 0)
		CHECKED_GL_CALL(glAttachShader(pid, FS));
	if (GS >= 0)
		CHECKED_GL_CALL(glAttachShader(pid, GS));
	if (CS >= 0)
		CHECKED_GL_CALL(glAttachShader(pid, CS));
	CHECKED_GL_CALL(glLinkProgram(pid));
	CHECKED_GL_CALL(glGetProgramiv(pid, GL_LINK_STATUS, &rc));
	if (!rc)
	{
		if (isVerbose())
		{
			GLSL::printProgramInfoLog(pid);
			std::cout << "Error linking shaders " << vShaderName << " and " << fShaderName << std::endl;
		}
		return false;
	}

	if (CS >= 0) {
		glUseProgram(pid);

		GLuint block_index = 0;
		block_index = glGetProgramResourceIndex(pid, GL_SHADER_STORAGE_BLOCK, "shader_data");
		GLuint ssbo_binding_point_index = 2;
		glShaderStorageBlockBinding(pid, block_index, ssbo_binding_point_index);

		for (int ii = 0; ii < SSBO_SIZE; ii++)
		{
			cs_ssbo_data.io[ii] = glm::vec4(ii, 0.0, 0.0, 0.0);
		}
		glGenBuffers(1, &gpuId);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, gpuId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_data), &cs_ssbo_data, GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gpuId);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
	}

	return true;
}

void Program::bind()
{
	CHECKED_GL_CALL(glUseProgram(pid));
}

void Program::unbind()
{
	CHECKED_GL_CALL(glUseProgram(0));
}

void Program::addAttribute(const std::string &name)
{
	attributes[name] = GLSL::getAttribLocation(pid, name.c_str(), isVerbose());
}

void Program::addUniform(const std::string &name)
{
	uniforms[name] = GLSL::getUniformLocation(pid, name.c_str(), isVerbose());
}

GLint Program::getAttribute(const std::string &name) const
{
	std::map<std::string, GLint>::const_iterator attribute = attributes.find(name.c_str());
	if (attribute == attributes.end())
	{
		if (isVerbose())
		{
			std::cout << name << " is not an attribute variable" << std::endl;
		}
		return -1;
	}
	return attribute->second;
}

GLint Program::getUniform(const std::string &name) const
{
	std::map<std::string, GLint>::const_iterator uniform = uniforms.find(name.c_str());
	if (uniform == uniforms.end())
	{
		if (isVerbose())
		{
			std::cout << name << " is not a uniform variable" << std::endl;
		}
		return -1;
	}
	return uniform->second;
}

ssbo_data Program::getSsboData() {
	return cs_ssbo_data;
}

void Program::setSsboData(ssbo_data data) {
	cs_ssbo_data = data;
}