// vlib.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <memory>
#include <map>
#include <array>
#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;
using namespace std;
#include <boost/optional.hpp>
using namespace boost;
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assimp/cimport.h>
#include <assimp/scene.h> 
#include <assimp/postprocess.h>

template<typename TDerived>
struct GlId 
{
	GlId() : value(-1) { }
	GlId(GLuint p_value) : value(p_value) { }
	operator GLuint() const { return value; }
	operator GLuint*() { return &value; }
	GLuint value;
	static TDerived make()
	{
		return TDerived();
	}
	static TDerived make(GLuint p_value)
	{
		TDerived x;
		x.value = p_value;
		return x;
	}
};
struct GLIndexBufferId : GlId<GLIndexBufferId> { };
struct GLVertexBufferId : GlId<GLVertexBufferId> { };
struct GLShaderId : GlId<GLShaderId> { };
struct GLProgramIdId : GlId<GLProgramIdId> { };
struct GLUniformLocationId : GlId<GLUniformLocationId> { };
struct GLFramebufferId : GlId<GLFramebufferId> 
{
	static GLFramebufferId default()
	{
		return GLFramebufferId::make(0);
	}
};
struct GLTextureId : GlId<GLTextureId> { };
struct GLRenderBufferId : GlId<GLRenderBufferId> { };
struct GLSamplerId : GlId<GLSamplerId> { };

struct GBuffer
{
	GLFramebufferId framebuffer;
	GLTextureId albedo;
	GLTextureId normal;
	GLTextureId depth;
};

GBuffer make_gbuffer(int width, int height)
{
	return GBuffer();//TODO:
}
struct GfxTexture
{
	GLTextureId id;
};
struct GfxRenderBuffer
{
	GfxRenderBuffer(GLRenderBufferId p_id) : id(p_id) { }
	GLRenderBufferId id;
};
struct GfxFramebuffer
{
	GfxFramebuffer(GLFramebufferId p_id) : id(p_id) { }
	GLFramebufferId id;	
	ivec2 size;
};
struct GenGBufferProgram
{
	struct Uniforms
	{
		static const char* VIEW_PROJ;
	};
};
const char* GenGBufferProgram::Uniforms::VIEW_PROJ = "view_proj";

template<typename TProgram>
struct GfxProgram
{
	GfxProgram(GLProgramIdId p_program_id, const map<string, GLUniformLocationId>& p_uniforms) 
		: program_id(p_program_id), uniforms(p_uniforms) { }
	GLProgramIdId program_id;
	map<string, GLUniformLocationId> uniforms;
};
GfxProgram<GenGBufferProgram> make_gen_gbuffer_program(GLProgramIdId program_id)
{
	GLUniformLocationId view_proj_loc = 
		GLUniformLocationId::make(glGetUniformLocation(program_id, GenGBufferProgram::Uniforms::VIEW_PROJ));
	map<string, GLUniformLocationId> uniforms;
	uniforms[GenGBufferProgram::Uniforms::VIEW_PROJ] = view_proj_loc;
	return GfxProgram<GenGBufferProgram>(program_id, uniforms);
}
struct Degree 
{
	float value;
};
string read_all(string path)
{
	std::ifstream ifs(path);
	return string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}
GLProgramIdId ogl_create_program(const std::vector<GLShaderId>& shaderList)
{
	GLProgramIdId program = GLProgramIdId::make(glCreateProgram());

	for(size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
	{
		glAttachShader(program, shaderList[iLoop]);
	}

	glLinkProgram(program);

	GLint status;
	glGetProgramiv (program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
		fprintf(stderr, "Linker failure: %s\n", strInfoLog);
		delete[] strInfoLog;
	}

	for(size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
	{
		glDetachShader(program, shaderList[iLoop]);
	}

	return program;
}
GLShaderId create_shader(GLenum eShaderType, const string &strShaderFile)
{
	GLuint shader = glCreateShader(eShaderType);
	const char *strFileData = strShaderFile.c_str();
	glShaderSource(shader, 1, &strFileData, NULL);

	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

		const char *strShaderType = NULL;
		switch(eShaderType)
		{
		case GL_VERTEX_SHADER: strShaderType = "vertex"; break;
		case GL_GEOMETRY_SHADER: strShaderType = "geometry"; break;
		case GL_FRAGMENT_SHADER: strShaderType = "fragment"; break;
		}

		fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete[] strInfoLog;
	}

	return GLShaderId::make(shader);
}

void error_callback(int error, const char* description)
{
	printf(description);
}
void APIENTRY ogl_debug_callback(GLenum source,
						GLenum type,
						GLuint id,
						GLenum severity,
						GLsizei length,
						const GLchar* message,
						void* userParam)
{
	printf("ERROR: %s", message);
}

template<typename T, int N>
vector<T> make_vector(T (&items)[N])
{
	return vector<T>(begin(items), end(items));
}
struct Vertex 
{
	Vertex(vec3 p_position, vec3 p_normal) : position(p_position), normal(p_normal)	{ }
	vec3 position;
	vec3 normal;
};

template<typename T>
struct BufferData
{
	BufferData(const vector<T>& p_data) : data(p_data) { }
	vector<T> data;
	int count() const { return data.size(); }
};

struct GeometryBufferData
{
	GeometryBufferData(
		const BufferData<float>& p_positions, 
		const BufferData<float>& p_normals, 
		const BufferData<unsigned int>& p_indices)
		: positions(p_positions), normals(p_normals), indices(p_indices) { }
	BufferData<float> positions;
	BufferData<float> normals;
	BufferData<unsigned int> indices;
	int num_vertices() const 
	{
		return indices.count();
	}
};
struct Geometry
{
	Geometry(const vector<Vertex>& p_verticies, const vector<unsigned int>& p_indices) 
		: vertices(p_verticies), indices(p_indices) { }
	vector<Vertex> vertices;
	vector<unsigned int> indices;
};
vec3 to_vec3(const aiVector3D& v)
{
	return vec3(v.x, v.y, v.z);
}
vector<unique_ptr<Geometry>> load_mesh_asset(string path, aiPostProcessSteps aux_flags )
{
	const aiScene* scene = aiImportFile(path.c_str(), aiProcess_Triangulate | aiProcess_PreTransformVertices
		| aux_flags);
	vector<unique_ptr<Geometry>> geometries;
	for(auto mesh_idx = 0; mesh_idx < scene->mNumMeshes; mesh_idx++)
	{
		auto mesh = scene->mMeshes[mesh_idx];
		assert(mesh->HasNormals());
		vector<Vertex> vertices;
		for(auto vert_idx = 0; vert_idx < mesh->mNumVertices; vert_idx++)
		{
			auto ai_vertex = mesh->mVertices[vert_idx];
			auto ai_normal = mesh->mNormals[vert_idx];			
			vertices.push_back(Vertex(to_vec3(ai_vertex), to_vec3(ai_normal)));
		}		
		vector<unsigned int> indices;
		for(auto face_idx = 0; face_idx < mesh->mNumFaces; face_idx++)
		{
			const auto& face = mesh->mFaces[face_idx];			
			for(auto face_vert_idx = 0; face_vert_idx < face.mNumIndices; face_vert_idx++)
			{
				indices.push_back(face.mIndices[face_vert_idx]);
			}
		}		
		geometries.push_back(unique_ptr<Geometry>(new Geometry(vertices, indices)));
	}
	return geometries;
}
vector<unique_ptr<GeometryBufferData>> make_buffer_data(const vector<unique_ptr<Geometry>>& geometry)
{
	vector<unique_ptr<GeometryBufferData>> result;
	for(const auto& geom: geometry)
	{
		vector<float> positions;
		vector<float> normals;
		for(auto v : geom->vertices)
		{
			positions.push_back(v.position.x);
			positions.push_back(v.position.y);
			positions.push_back(v.position.z);
			normals.push_back(v.normal.x);
			normals.push_back(v.normal.y);
			normals.push_back(v.normal.z);
		}
		const vector<unsigned int>& indices = geom->indices;
		result.push_back(unique_ptr<GeometryBufferData>(new GeometryBufferData(positions, normals, indices)));
	}
	return result;
}

template<typename T>
GLIndexBufferId make_index_buffer_and_upload_data(const BufferData<T>& buffer_data)
{
	GLIndexBufferId vboID;
	glGenBuffers(1, vboID);
	glNamedBufferDataEXT(vboID, buffer_data.count() * sizeof(T), buffer_data.data.data(), GL_STATIC_DRAW);
	return vboID;
}

template<typename T>
GLVertexBufferId make_vertex_buffer_and_upload_data(const BufferData<T>& buffer_data)
{
	GLVertexBufferId vboID;
	glGenBuffers(1, vboID);
	glNamedBufferDataEXT(vboID, buffer_data.count() * sizeof(T), buffer_data.data.data(), GL_STATIC_DRAW);
	return vboID;
}
struct GfxDrawBuffers
{
	GfxDrawBuffers(const GLVertexBufferId& p_position_buffer, 
		const GLVertexBufferId& p_normal_buffer, 
		const GLIndexBufferId& p_index_buffer,
		int p_num_indices)
		: position_buffer(p_position_buffer), normal_buffer(p_normal_buffer), index_buffer(p_index_buffer),
		num_indices(p_num_indices) 
	{ }
	GLVertexBufferId position_buffer;
	GLVertexBufferId normal_buffer;
	GLIndexBufferId index_buffer;
	unsigned int num_indices;
};

template<typename TProgram>
struct GfxGraphicsState
{
	GfxGraphicsState(const GfxFramebuffer& p_framebuffer, GfxDrawBuffers p_draw_buffers, 
		GfxProgram<TProgram> p_program,
		map<string, mat4> p_uniform_mat4)
		: framebuffer(p_framebuffer), draw_buffers(p_draw_buffers), program(p_program), uniform_mat4(p_uniform_mat4)
	{ }
	GfxProgram<TProgram> program;
	GfxFramebuffer framebuffer;
	GfxDrawBuffers draw_buffers;
	map<string, mat4> uniform_mat4;
};

vector<GfxDrawBuffers> make_draw_buffer_and_upload_data(const vector<unique_ptr<GeometryBufferData>>& geometries)
{
	vector<GfxDrawBuffers> result;
	for(const auto& geom : geometries)
	{
		auto positions = make_vertex_buffer_and_upload_data(geom->positions);
		auto normals = make_vertex_buffer_and_upload_data(geom->normals);
		auto indices = make_index_buffer_and_upload_data(geom->indices);
		result.push_back(GfxDrawBuffers(positions, normals, indices, geom->indices.count()));
	}
	return result;
}

struct Camera 
{
	vec3 eye;
	vec3 forward;
	vec3 up;

	Degree fov;
	float z_near;
	float z_far;
	float aspect_ratio;
	Camera(vec3 p_eye, vec3 p_forward, vec3 p_up, Degree p_fov, float p_z_near, float p_z_far, float p_aspect_ratio)
		: eye(p_eye), forward(p_forward), up(p_up), fov(p_fov), z_near(p_z_near), z_far(p_z_far), aspect_ratio(p_aspect_ratio)
	{ }
	mat4 view_matrix() const
	{
		return lookAt(eye, eye + forward, up);
	}
	mat4 projection_matrix() const
	{
		return perspective(fov.value, aspect_ratio, z_near, z_far);
	}
	Camera move(const vec3& dp) const
	{
		Camera new_camera(*this);
		new_camera.eye += dp;
		return new_camera;
	}
};
optional<GLFWwindow*> init_glfw()
{
	const int width = 1000, height = 1000;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	if (!glfwInit()) return optional<GLFWwindow*>();
	glfwSetErrorCallback(error_callback);
	auto window = glfwCreateWindow(width, height, "My Title", NULL, NULL);
	glfwMakeContextCurrent(window);
	return window;
}
GLRenderBufferId make_renderbuffer(int width, int height)
{
	GLuint renderbuffer;
	glGenRenderbuffers(1, &renderbuffer);
	glNamedRenderbufferStorageEXT(renderbuffer, GL_DEPTH_COMPONENT, width, height);
	return GLRenderBufferId::make(renderbuffer);
}
GLTextureId make_texture(int width, int height)
{
	GLTextureId texture;
	glGenTextures(1, &texture.value);
	glTextureImage2DEXT(texture, GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTextureParameteriEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteriEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	return texture;
}
template<int N>
GLFramebufferId make_frame_buffer(const GLTextureId (&textures)[N], const GLRenderBufferId depth_buffer)
{
	GLFramebufferId frame_buffer;
	glGenFramebuffers(1, &frame_buffer.value);
	const GLenum attachments[] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3,
		GL_COLOR_ATTACHMENT4,
		GL_COLOR_ATTACHMENT5
	};
	for(auto i = 0; i < N; i++)
	{
		glNamedFramebufferTextureEXT(frame_buffer, attachments[i], textures[i], 0);
	}
	glNamedFramebufferRenderbufferEXT(frame_buffer, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
	
	glFramebufferDrawBuffersEXT(frame_buffer, N, attachments);
	assert(GL_FRAMEBUFFER_COMPLETE == glCheckNamedFramebufferStatusEXT(frame_buffer, GL_FRAMEBUFFER));
	return frame_buffer;
}

void activate_frame_buffer(const GfxFramebuffer& frame_buffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);
}
void activate_default_frame_buffer()
{
	activate_frame_buffer(GLFramebufferId::default());
}
void activate_and_clear_framebuffer(GLFramebufferId frame_buffer)
{
	activate_frame_buffer(frame_buffer);
	glClearBufferfv(GL_COLOR, 0, value_ptr(vec4(0,0,0,0)));
	float z_reset = 1;
	glClearBufferfv(GL_DEPTH, 0, &z_reset);
}
Camera move_camera_using_keyboard(GLFWwindow* window, const Camera& p_camera, double move_speed)
{
	auto camera = p_camera;
	if(glfwGetKey(window, 'W') == GLFW_PRESS)
	{
		camera = camera.move(vec3(0, 0, move_speed));
	}
	else if(glfwGetKey(window, 'S') == GLFW_PRESS)
	{
		camera = camera.move(vec3(0, 0, -move_speed));
	}
	if(glfwGetKey(window, 'A') == GLFW_PRESS)
	{
		camera = camera.move(vec3(move_speed, 0, 0));
	}
	else if(glfwGetKey(window, 'D') == GLFW_PRESS)
	{
		camera = camera.move(vec3(-move_speed, 0, 0));
	}
	return camera;
}

template<typename TProgram>
void draw(const GfxGraphicsState<TProgram>& state)
{
	activate_frame_buffer(state.framebuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.draw_buffers.index_buffer);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, state.draw_buffers.position_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, state.draw_buffers.normal_buffer);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glUseProgram(state.program.program_id);
	for(auto mat4_uniform : state.uniform_mat4)
	{
		auto key = state.program.uniforms.find(mat4_uniform.first)->second;
		auto value = mat4_uniform.second;
		glProgramUniformMatrix4fv(state.program.program_id, key, 1, GL_FALSE, value_ptr(value));		
	}
	glDrawElements(GL_TRIANGLES, state.draw_buffers.num_indices, GL_UNSIGNED_INT, 0);
}
void main_loop()
{
	auto geometries = load_mesh_asset("nff/sphere.nff", aiProcess_GenSmoothNormals); const double move_speed = 0.02;
	//auto geometries = load_mesh_asset("asset_obj/sponza.obj", (aiPostProcessSteps)0); const double move_speed = 2;
	auto geometryBufferDatas = make_buffer_data(geometries);

	Degree fov = {60};
	Camera camera(vec3(0,0,-4), vec3(0,0,1), vec3(0, 1, 0), fov, 1, 10000, 1);
	auto window_opt = init_glfw();
	if(!window_opt)
	{
		return;
	}
	auto window = *window_opt;
	if(glewInit() != GLEW_OK) return;
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(ogl_debug_callback, nullptr);
	
	float positions[] = { 0, 0, 1, 0, 1, 1 };
	float positions2[] = { -1, 0, .8, 0, .6, 1 };

	auto draw_buffers = make_draw_buffer_and_upload_data(geometryBufferDatas);

	GLShaderId shaders[] = {
		create_shader(GL_VERTEX_SHADER, read_all("shaders/gen_gbuffer.vs")),
		create_shader(GL_FRAGMENT_SHADER, read_all("shaders/gen_gbuffer.fs"))
	};

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	GfxProgram<GenGBufferProgram> program(make_gen_gbuffer_program(ogl_create_program(make_vector(shaders))));
	int frame_idx = 0;
	GLTextureId color[] = { make_texture(width, height) };
	GLRenderBufferId depth = make_renderbuffer(width, height);
	auto frame_buffer = make_frame_buffer(color, depth);
	while (!glfwWindowShouldClose(window))
	{
		glClearDepth(1);
		glClear(GL_DEPTH_BUFFER_BIT);		
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		camera = move_camera_using_keyboard(window, camera, move_speed);
		activate_and_clear_framebuffer(frame_buffer);
		for(auto& draw_buffer : draw_buffers)
		{
			map<string, mat4> uniform_values;
			uniform_values[GenGBufferProgram::Uniforms::VIEW_PROJ] = camera.projection_matrix() * camera.view_matrix();
			GfxGraphicsState<GenGBufferProgram> state(GLFramebufferId::default(), draw_buffer, program, uniform_values);
			draw(state);
		}
		activate_default_frame_buffer();
		/*
		GLSamplerId albedo_sampler = GLSamplerId::make(glGetUniformLocation(program, "sampler_albedo"));
		GLSamplerId normal_sampler = GLSamplerId::make(glGetUniformLocation(program, "sampler_normal"));
		glProgramUniform1i(program, albedo_sampler, 0);
		glProgramUniform1i(program, normal_sampler, 2);

		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, color[0]);
		GLSamplerId linearFiltering;
		glGenSamplers(1, &linearFiltering.value);
		glBindSampler(0, linearFiltering);
		*/

		glfwSwapBuffers(window);
		glfwPollEvents();
		frame_idx++;
	}
}
int _tmain(int argc, _TCHAR* argv[])
{
	main_loop();
	glfwTerminate();
	return 0;
}

