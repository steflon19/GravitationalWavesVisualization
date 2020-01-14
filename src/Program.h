
#pragma  once

#ifndef LAB471_PROGRAM_H_INCLUDED
#define LAB471_PROGRAM_H_INCLUDED

#include <map>
#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>

std::string readFileAsString(const std::string &fileName);
#define SSBO_SIZE 512

// TODO: make variable size!! why even class and not struct?
class ssbo_data
{
public:
	glm::vec4 io[SSBO_SIZE];//pos input in xyz, output in w
};
class Program
{

public:

	void setVerbose(const bool v) { verbose = v; }
	bool isVerbose() const { return verbose; }

	void setShaderNames(const std::string &v, const std::string &f, const std::string &g);
	void setShaderNames(const std::string &v, const std::string &f);
	void setShaderNames(const std::string &c);
	virtual bool init();
	virtual void bind();
	virtual void unbind();

	void addAttribute(const std::string &name);
	void addUniform(const std::string &name);
	GLint getAttribute(const std::string &name) const;
	GLint getUniform(const std::string &name) const;
	ssbo_data getSsboData();
	void setSsboData(ssbo_data data);
	GLuint pid = 0;
	GLuint gpuId = 0;
	
protected:

	std::string vShaderName;
	std::string fShaderName;
	std::string gShaderName;
	std::string cShaderName;

	GLuint cs_ssbo_gpu_id;
	ssbo_data cs_ssbo_data;

private:


	std::map<std::string, GLint> attributes;
	std::map<std::string, GLint> uniforms;
	bool verbose = true;
};

#endif // LAB471_PROGRAM_H_INCLUDED
