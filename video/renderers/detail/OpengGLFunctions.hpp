#pragma once

#include <GL/glew.h>

#include <fstream>
#include <filesystem>
#include <sstream>
#include <vector>

#include "../../../common/Logger.hpp"

namespace GBA::video::renderer {
	std::string LoadShaderSource(std::filesystem::path const& path) {
		if (!std::filesystem::exists(path))
			return "FILE NOT FOUND";

		std::ostringstream shader_source{ "" };

		std::ifstream shader(path, std::ios::in);

		while (!shader.eof()) {
			std::string line = "";

			std::getline(shader, line);

			if (!line.empty())
				shader_source << line << "\n";
		}

		return shader_source.str();
	}

	std::pair<GLuint, std::string> CreateShader(std::string const& path, GLenum type) {
		GLenum shader_id = glCreateShader(type);

		std::string source = LoadShaderSource(path);

		GLint len = (GLint)source.length();

		const char* source_ptr = source.c_str();

		glShaderSource(shader_id, 1, &source_ptr, &len);
		glCompileShader(shader_id);

		GLint status = 0;

		glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);

		char info_log[512] = {};
		GLsizei info_len = 0;

		if (status == GL_FALSE) {
			glGetShaderInfoLog(shader_id, 512, &info_len, info_log);
		}

		return std::pair{ shader_id, std::string(info_log, info_len) };
	}

	std::pair<GLuint, std::string> CreateProgram(std::vector<std::pair<std::string, GLenum>> const& shaders) {
		GLuint program_id = glCreateProgram();

		for (auto const& shader : shaders) {
			auto shader_result = CreateShader(shader.first, shader.second);

			logging::Logger::Instance().LogInfo("OpenGL Renderer", "Shader log for shader {0} : {1}", shader.first, shader_result.second);

			glAttachShader(program_id, shader_result.first);
		}

		glLinkProgram(program_id);

		GLint status = 0;

		glGetProgramiv(program_id, GL_LINK_STATUS, &status);
		glValidateProgram(program_id);

		char info_log[512] = {};
		GLsizei info_len = 0;

		glGetProgramInfoLog(program_id, 512, &info_len, info_log);

		return std::pair{ program_id, std::string(info_log, info_len) };
	}
}