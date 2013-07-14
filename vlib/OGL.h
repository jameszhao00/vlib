#pragma once
#include "Data.h"
#include "TextureFormat.h"
using namespace texformats;
template<typename TDerived>
struct GlId {
public:
	GlId() :
	  value(-1) {
	  }
	  GlId(GLuint p_value) :
	  value(p_value) {
	  }
	  operator GLuint() const {
		  return value;
	  }
	  operator GLuint*() {
		  return &value;
	  }
	  GLuint value;
	  static TDerived make() {
		  return TDerived();
	  }
	  static TDerived make(GLuint p_value) {
		  TDerived x;
		  x.value = p_value;
		  return x;
	  }
};
namespace OpenGLFormatConversion {
	GLint internal_format(GfxTextureFormat tex_format) {
		if(tex_format == RGBA16F) {
			return GL_RGBA16F;
		} else if(tex_format == RGBA_UNORM) {
			return GL_RGBA;
		} else if(tex_format == RGB_UNORM) {
			return GL_RGB;
		} else if(tex_format == RGBA_UNORM_SRGB) {
			return GL_SRGB;
		} else {
			assert(false);
		}
	}
	GLint format(GfxTextureFormat tex_format) {
		if(tex_format == RGBA16F) {
			return GL_RGBA;
		} else if(tex_format == RGBA_UNORM) {
			return GL_RGBA;
		} else if(tex_format == RGB_UNORM) {
			return GL_RGB;
		} else if(tex_format == RGBA_UNORM_SRGB) {
			return GL_RGBA;
		} else {
			assert(false);
		}
	}
	GLint type(GfxTextureFormat tex_format) {
		if(tex_format == RGBA16F) {
			return GL_FLOAT;
		} else if(tex_format == RGBA_UNORM || tex_format == RGB_UNORM || tex_format == RGBA_UNORM_SRGB) {
			return GL_UNSIGNED_BYTE;
		} else {
			assert(false);
		}
	}
}
struct GLVertexBufferId: GlId<GLVertexBufferId> {
};
struct GLIndexBufferId: GlId<GLIndexBufferId> {
};
struct GLShaderId: GlId<GLShaderId> {
};
struct GLProgramId: GlId<GLProgramId> {
};
struct GLUniformLocationId {
	GLint id;
};
struct GLFramebufferId: GlId<GLFramebufferId> {
	static GLFramebufferId default_framebuffer() {
		return make(0);
	}
};
struct GLTextureId: GlId<GLTextureId> {
};
struct GLRenderBufferId: GlId<GLRenderBufferId> {
};
struct GLSamplerId: GlId<GLSamplerId> {
};

GLTextureId make_texture(int width, int height, 
	GLint internal_format,
	GLint type,
	GLint format,
	char* data = 0) {
	GLTextureId texture;
	glGenTextures(1, texture);
	glTextureImage2DEXT(texture, GL_TEXTURE_2D, 0, internal_format, width, height, 0,
		format, type, data);
	glTextureParameteriEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		GL_NEAREST);
	glTextureParameteriEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_NEAREST);
	return texture;
}

GLRenderBufferId make_renderbuffer(int width, int height) {
	GLuint renderbuffer;
	glGenRenderbuffers(1, &renderbuffer);
	glNamedRenderbufferStorageEXT(renderbuffer, GL_DEPTH_COMPONENT, width, height);
	return GLRenderBufferId::make(renderbuffer);
}

GLProgramId ogl_create_program(const vector<GLShaderId>& shaders) {
    GLProgramId program = GLProgramId::make(glCreateProgram());

	/*
    for (auto shader : shaders) {
        glAttachShader(program, shader);
    } */
	for(size_t idx = 0; idx < shaders.size(); idx++) {
		glAttachShader(program, shaders[idx]);
	}

    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
        fprintf(stderr, "Linker failure: %s\n", strInfoLog);
        delete[] strInfoLog;
    }

    return program;
}
GLShaderId create_shader(GLenum eShaderType, const string &strShaderFile) {
    GLuint shader = glCreateShader(eShaderType);
    const char *strFileData = strShaderFile.c_str();
    glShaderSource(shader, 1, &strFileData, NULL);

    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

        const char *strShaderType = NULL;
        switch (eShaderType) {
        case GL_VERTEX_SHADER:
            strShaderType = "vertex";
            break;
        case GL_GEOMETRY_SHADER:
            strShaderType = "geometry";
            break;
        case GL_FRAGMENT_SHADER:
            strShaderType = "fragment";
            break;
        }

        fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
        delete[] strInfoLog;
    }

    return GLShaderId::make(shader);
}

template<typename T>
GLIndexBufferId make_index_buffer_and_upload_data(const DataBuffer<T>& buffer_data) {
	GLIndexBufferId vboID;
	glGenBuffers(1, vboID);	
	glNamedBufferDataEXT(vboID, buffer_data.count() * sizeof(T), buffer_data.data.data(), GL_STATIC_DRAW);
	return vboID;
}

template<typename T>
GLVertexBufferId make_vertex_buffer_and_upload_data(const DataBuffer<T>& buffer_data) {
	GLVertexBufferId vboID;
	glGenBuffers(1, vboID);
	glNamedBufferDataEXT(vboID, buffer_data.count() * sizeof(T), buffer_data.data.data(), GL_STATIC_DRAW);
	return vboID;
}

void error_callback(int error, const char* description) {
	printf("%s", description);
}
void APIENTRY ogl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
	const GLchar* message, void* userParam) {
		printf("ERROR: %s", message);
}

optional<GLFWwindow*> init_glfw() {
	const int width = 1920, height = 1080;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	if (!glfwInit())
		return optional<GLFWwindow*>();
	glfwSetErrorCallback(error_callback);
	const bool is_fullscreen = false;
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	auto window = glfwCreateWindow(width, height, "My Title", is_fullscreen ? monitor : nullptr, nullptr);
	glfwMakeContextCurrent(window);
	return window;
}

const GLenum attachments[] = {
	GL_COLOR_ATTACHMENT0,
	GL_COLOR_ATTACHMENT1,
	GL_COLOR_ATTACHMENT2,
	GL_COLOR_ATTACHMENT3,
	GL_COLOR_ATTACHMENT4,
	GL_COLOR_ATTACHMENT5 };

#include "GfxData.h"
class OglCachedMeshApi {
	GLVertexBufferId vertex_buffer_type;
	GLIndexBufferId index_buffer_type;
};
		 
class OglStateApi {

};
class OglDrawOpApi {
	typedef OglStateApi graphics_state_api_type;
	typedef OglCachedMeshApi geom_api_type;
};
struct GlTexture {
public:
	GlTexture(GLRenderBufferId id) : rb_id_(id), is_tex(false) { }
	GlTexture(GLTextureId id) : tex_id_(id), is_tex(true) { }
	void clear(float val);
	void clear(vec3 val);
	GLTextureId tex_id_;
	GLRenderBufferId rb_id_;
	bool is_tex;
};
class OpenGLApi {
	typedef GlTexture texture_type;
	typedef OglDrawOpApi draw_op_type;
	typedef GLFramebufferId draw_target_type;
	typedef GLVertexBufferId vertex_buffer_type;
	typedef GLIndexBufferId index_buffer_type;
	typedef GLProgramId shader_program_type;	

	texture_type create_depth(ivec2 size) {
		return texture_type(GLRenderBufferId(make_renderbuffer(size.x, size.y)));
	}
	texture_type create_texture(texformats::GfxTextureFormat format, ivec2 size, const TextureData& data) {
		return texture_type(GlTexture(make_texture(size.x, size.y, 
			OpenGLFormatConversion::internal_format(format),
			OpenGLFormatConversion::type(format),
			OpenGLFormatConversion::format(format), (char*)data.data.data())));
	}
	void initialize(ivec2 size) {

	}
	void draw(const draw_op_type& draw_op) {
	}
	cached_geom_type cache_geom() {

	}
	draw_target_type create_draw_target(const vector<texture_type>& textures, optional<texture_type>& depth) {
		GLFramebufferId frame_buffer;
		glGenFramebuffers(1, frame_buffer);
		for (size_t i = 0; i < textures.size(); i++) {
			assert(textures[i].api_.is_tex);
			glNamedFramebufferTextureEXT(frame_buffer, attachments[i], textures[i].api_.tex_id_, 0);
		}
		if(depth) {
			assert(!depth->api_.rb_id_);
			glNamedFramebufferRenderbufferEXT(frame_buffer, GL_DEPTH_ATTACHMENT,
				GL_RENDERBUFFER, depth->api_.rb_id_);
		} else {
			glNamedFramebufferRenderbufferEXT(frame_buffer, GL_DEPTH_ATTACHMENT,
				GL_RENDERBUFFER, 0);
		}

		glFramebufferDrawBuffersEXT(frame_buffer, (GLsizei) textures.size(), attachments);
		assert(GL_FRAMEBUFFER_COMPLETE == glCheckNamedFramebufferStatusEXT(frame_buffer, GL_FRAMEBUFFER));
		return frame_buffer;
	}
	shader_program_type load_shader(string vs_path, string fs_path) {	
		auto vs = create_shader(GL_VERTEX_SHADER, vs_path);
		auto fs = create_shader(GL_FRAGMENT_SHADER, fs_path);
		vector<GLShaderId> src;
		src.push_back(vs);
		src.push_back(fs);
		return ogl_create_program(src);
	}
};