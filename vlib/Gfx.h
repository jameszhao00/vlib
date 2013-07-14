#pragma once
#include "OGL.h"
#include "CompiledAsset.h"
#include "TextureFormat.h"
using namespace CompiledAsset;
using namespace texformats;

struct GfxTexture {
	GfxTexture(const GLTextureId& p_id) : id(p_id) { }
	GfxTexture() { } // wtf why do we need this constructor?
	GLTextureId id;
};
struct GfxSampler {
	GfxSampler(GLSamplerId p_id) : id(p_id) { }
	GLSamplerId id;
};
struct GfxRenderBuffer {
	GfxRenderBuffer(GLRenderBufferId p_id) :
id(p_id) {
}
GLRenderBufferId id;
};

struct GfxFramebuffer {
	GfxFramebuffer(GLFramebufferId p_id, const vector<GfxTexture>& p_textures,
		optional<GfxRenderBuffer> p_depth) :
	id(p_id), textures(p_textures), depth(p_depth) {
	}
	GLFramebufferId id;
	ivec2 size;
	vector<GfxTexture> textures;
	optional<GfxRenderBuffer> depth;

	static GfxFramebuffer default_fb() {
		return GfxFramebuffer(GLFramebufferId::default_framebuffer(), vector<GfxTexture>(), nullptr);
	}
private:
};




GfxTexture make_gfx_texture(int width, int height, texformats::GfxTextureFormat format, char* data = nullptr) {
	return GfxTexture(make_texture(width, height, 
		texformats::internal_format(format),
		texformats::type(format),
		texformats::format(format),
		data));
}

GfxTexture make_gfx_texture(const TextureData& tex_data) {
	assert(tex_data.byte_per_pixel == 4 || tex_data.byte_per_pixel == 3);
	texformats::GfxTextureFormat format = tex_data.byte_per_pixel == 4 
		//WARNING: USING SRGB!
		? texformats::GfxTextureFormat::RGBA_UNORM_SRGB : texformats::GfxTextureFormat::RGB_UNORM;
	return make_gfx_texture(tex_data.w, tex_data.h, format, (char*)tex_data.data.data());
}
enum UniformTypes {
	//maybe move to varadic templates later...
	Matrix4 = 1, Vec3, Sampler
};
struct GfxUniformSpec {
	string name;
	UniformTypes type;
	bool operator <(const GfxUniformSpec& rhs) const
	{
		//TODO: Why does this not work? seems like some sort of logic error...        
		return pair<string, UniformTypes>(name, type) < pair<string, UniformTypes>(rhs.name, rhs.type);
	}
};

template<typename TProgram>
struct GfxProgram {
	GfxProgram(GLProgramId p_id, const map<GfxUniformSpec, GLUniformLocationId>& p_uniforms) :
id(p_id), uniforms(p_uniforms) {
}
GLProgramId id;
map<GfxUniformSpec, GLUniformLocationId> uniforms;
};
struct GfxGeometryBuffers {
	GfxGeometryBuffers(
		const GLVertexBufferId& p_position_buffer,
		const GLVertexBufferId& p_normal_buffer,
		const optional<GLVertexBufferId>& p_uv_buffer,
		const GLIndexBufferId& p_index_buffer, 		
		int p_num_indices) :
	position_buffer(p_position_buffer), normal_buffer(p_normal_buffer), index_buffer(p_index_buffer), num_indices(
		p_num_indices), uv_buffer(p_uv_buffer) { }
	GLVertexBufferId position_buffer;
	GLVertexBufferId normal_buffer;
	optional<GLVertexBufferId> uv_buffer;
	GLIndexBufferId index_buffer;
	unsigned int num_indices;
};

size_t size(UniformTypes type) {
	if(type == Matrix4) {
		return 4 * 4 * sizeof(float);
	} else if(type == Vec3) {
		return 3 * sizeof(float);
	} else {
		return -1; //should really throw an error...
	}
}


struct GfxUniformValue {
	size_t stride;
	vector<char> buffer;
	GfxUniformValue() { } //why do we need this default cons?		
	GfxUniformValue(size_t p_stride, const vector<char>& p_buffer)
		: stride(p_stride), buffer(p_buffer) { }
	template<typename T>
	static GfxUniformValue make(const T& v) {
		char* start = (char*)value_ptr(v);
		char* end = start + byte_size(v);		
		return GfxUniformValue(::stride(v), vector<char>(start, end));
	}
};

template<typename TProgram>
struct GfxShaderState {

	GfxShaderState(
		const GfxProgram<TProgram>& p_program,
		const map<GfxUniformSpec, GfxUniformValue>& p_uniforms,
		const map<GfxUniformSpec, GfxTexture>& p_textures,
		const GfxSampler& p_samplers) :
	program(p_program), uniforms(
		p_uniforms), textures(p_textures), sampler(p_samplers) { }

	GfxProgram<TProgram> program;
	map<GfxUniformSpec, GfxUniformValue> uniforms;
	//TODO: make texture's GfxUniformSpec GfxTextureSpec?
	map<GfxUniformSpec, GfxTexture> textures;
	GfxSampler sampler; //default sampler is required for now... just a workaround
};
template<typename TProgram>
struct GfxGraphicsState {
	GfxGraphicsState(
		optional<const GfxFramebuffer&> p_framebuffer,
		optional<const GfxShaderState<TProgram>&> p_shader_state)
		: framebuffer(p_framebuffer), shader_state(p_shader_state) { }

	optional<GfxFramebuffer> framebuffer;
	optional<GfxShaderState<TProgram>> shader_state;
};
template<typename TProgram>
struct GfxDrawOperation {
	GfxDrawOperation(
		const GfxGraphicsState<TProgram>& p_state,
		const GfxGeometryBuffers& p_draw_buffers) :
	state(p_state), draw_buffers(p_draw_buffers) { }
	GfxGraphicsState<TProgram> state;
	GfxGeometryBuffers draw_buffers;
};


GfxFramebuffer make_frame_buffer(const vector<GfxTexture>& textures, const GfxRenderBuffer& depth_buffer) {
	GLFramebufferId frame_buffer;
	glGenFramebuffers(1, frame_buffer);
	for (size_t i = 0; i < textures.size(); i++) {
		glNamedFramebufferTextureEXT(frame_buffer, attachments[i], textures[i].id, 0);
	}
	glNamedFramebufferRenderbufferEXT(frame_buffer, GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER, depth_buffer.id);

	glFramebufferDrawBuffersEXT(frame_buffer, (GLsizei) textures.size(), attachments);
	assert(GL_FRAMEBUFFER_COMPLETE == glCheckNamedFramebufferStatusEXT(frame_buffer, GL_FRAMEBUFFER));

	return GfxFramebuffer(frame_buffer, textures, depth_buffer);
}


void activate_frame_buffer(GfxFramebuffer frame_buffer) {
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);
}
void activate_and_clear_framebuffer(GfxFramebuffer frame_buffer, float z_reset) {
	activate_frame_buffer(frame_buffer);
	for(size_t i = 0; i < frame_buffer.textures.size(); i++){
		//ASSUMPTION: framebuffer textures use 0-N slots when count = N
		glClearBufferfv(GL_COLOR, (GLsizei)i, value_ptr(vec4(0, 0, 0, 0)));
	}
	glClearBufferfv(GL_DEPTH, 0, &z_reset);
}

template<typename TProgram>
void apply_uniform(GfxProgram<TProgram> program, GLUniformLocationId uniform_loc,
	GfxUniformSpec spec, GfxUniformValue val) {
		auto val_buffer_size = val.buffer.size();
		assert(size(spec.type) == val_buffer_size);
		//ext = we dont need to bind program
		glUseProgram(program.id);
		if(spec.type == UniformTypes::Matrix4) {
			//glProgramUniformMatrix4fvEXT(program.id, uniform_loc.id, 1, GL_FALSE, (GLfloat*)val.buffer.data());
			glUniformMatrix4fv(uniform_loc.id, 1, GL_FALSE, (GLfloat*)val.buffer.data());
		} else if(spec.type == UniformTypes::Vec3) {
			//glProgramUniform3fvEXT(program.id, uniform_loc.id, 1, (GLfloat*)val.buffer.data());
			glUniform3fv(uniform_loc.id, 1, (GLfloat*)val.buffer.data());
		} else {
			assert(false);
		}
}


template<typename TProgram>
void apply_state(const GfxGraphicsState<TProgram>& state) {
	if(state.framebuffer) {
		activate_frame_buffer(*state.framebuffer);
	}
	if(state.shader_state) {
		auto& shader_state = *state.shader_state;
		glUseProgram(shader_state.program.id);
		//for (auto uniform : shader_state.uniforms) {
		for(auto it = shader_state.uniforms.begin(); it != shader_state.uniforms.end(); it++) {
			auto uniform_spec = it->first;
			auto uniform_value = it->second;
			auto uniform_loc = get(shader_state.program.uniforms, uniform_spec);
			apply_uniform(shader_state.program, uniform_loc, uniform_spec, uniform_value);
		}
		{
			int texture_idx = 0;
			for (auto it = begin(shader_state.textures); it != end(shader_state.textures); it++, texture_idx++) {
				//auto tex_name = it->first;
				//auto tex_val = it->second;
				//the following code may not work...
				GLUniformLocationId loc = (shader_state.program.uniforms.find(it->first))->second;
				glUniform1i(loc.id, texture_idx);
				glActiveTexture(GL_TEXTURE0 + texture_idx);
				GfxTexture tex = it->second;
				glBindTexture(GL_TEXTURE_2D, tex.id);
				glBindSampler(0, shader_state.sampler.id);
			}
		}
	}
}

template<typename TProgram>
void draw(const GfxDrawOperation<TProgram>& drawop) {
	apply_state(drawop.state);

	auto& draw_buffers = drawop.draw_buffers;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_buffers.index_buffer);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, draw_buffers.position_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, draw_buffers.normal_buffer);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	if(draw_buffers.uv_buffer) {
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, *(draw_buffers.uv_buffer));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	}

	glDrawElements(GL_TRIANGLES, draw_buffers.num_indices, GL_UNSIGNED_INT, 0);
}
GfxSampler make_sampler() {
	GLSamplerId sampler_id;
	glGenSamplers(1, sampler_id);
	glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	return GfxSampler(sampler_id);
}

GfxGeometryBuffers make_draw_buffer_and_upload_data(const BakedGeometry& geom) {
	auto positions = make_vertex_buffer_and_upload_data(geom.positions);
	auto normals = make_vertex_buffer_and_upload_data(geom.normals);
	auto indices = make_index_buffer_and_upload_data(geom.indices);
	optional<GLVertexBufferId> uv_buffer;
	if(geom.uvs) {
		uv_buffer = make_vertex_buffer_and_upload_data(*(geom.uvs));
	}
	return GfxGeometryBuffers(positions, normals, uv_buffer, indices, (int)geom.indices.count());
}
GfxGeometryBuffers make_fsquad() {
	float pos[] = { -1, 1, 0, //ul
		1, 1, 0, //ur
		1, -1, 0,  //lr
		-1, -1, 0 //ll
	};
	unsigned int indices[] = { 0, 1,2, 2, 3, 0 };

	BakedGeometry geom_buffer_data(
		DataBuffer<float>(vector<float>(pos, pos+12)), 
		DataBuffer<float>( vector<float>(12, 0)),
		optional<DataBuffer<float>>(),
		DataBuffer<unsigned int>(vector<unsigned int>(indices, indices + 6)));
	return make_draw_buffer_and_upload_data(geom_buffer_data);
}


template<typename TProgram>
void validate_program(const GfxProgram<TProgram>& prog) {
	//for (auto& kv : prog.uniforms) {
	for(auto it = prog.uniforms.begin(); it != prog.uniforms.end(); it++) {
		assert(it->second.id != -1);
	}
}

struct GfxCachedMesh {
	GfxCachedMesh(const BakedMesh& p_baked_mesh, optional<GfxTexture>& p_albedo_tex, 
		const GfxGeometryBuffers& p_geom_buf)
		: baked_mesh(p_baked_mesh), albedo_tex(p_albedo_tex), geom_buf(p_geom_buf) { }	
	//WARNING: storing a ref is extremely risky
	BakedMesh baked_mesh;
	optional<GfxTexture> albedo_tex;
	GfxGeometryBuffers geom_buf;
};


vector<GfxCachedMesh> cache_baked_meshes(const vector<BakedMesh>& baked_meshes) {
	vector<GfxCachedMesh> output;
	//TODO: why do I need a const before BakedMesh&? keeps talking about losing qualifiers...
	transform(begin(baked_meshes), end(baked_meshes), back_inserter(output), [](const BakedMesh& mesh) -> GfxCachedMesh {
		auto geom_buffers = make_draw_buffer_and_upload_data(mesh.geometry);
		optional<GfxTexture> albedo_tex;
		if(mesh.albedo_tex) {
			albedo_tex = make_gfx_texture(mesh.albedo_tex->texture_data);
		}
		return GfxCachedMesh(mesh, albedo_tex, geom_buffers);
	});
	return output;
}

namespace Gfx {
	class IDevice {
	public:
		virtual void draw(const IGfxState&) = 0;
		virtual unique_ptr<ITexture> create_texture(ivec2 size, TexturePurpose purpose) = 0;
		virtual unique_ptr<ITexture> cache_texture(TextureData& tex_data) = 0;
		virtual unique_ptr<IRenderTargetState> create_render_target_state(const vector<ITexture&>) = 0;
		virtual unique_ptr<IGeometryState> cache_geometry() = 0;		
		virtual void cache_mesh() = 0;
		virtual void present() = 0;
		virtual void clear(ITexture& tex) = 0;
	protected:
		virtual ~IDevice() = 0;
	};
	template<typename Api>
	class DeviceInterface {
	public:
		typedef Gfx::TextureInterface<Api::texture_type> texture_type;
		typedef Gfx::DrawOperationInterface<Api::draw_op_type> draw_op_type;
		typedef Gfx::DrawTargetInterface<Api::draw_target_type> draw_target_type;
		typedef Gfx::ShaderProgramInterface<Api::shader_program_type> shader_program_type;	
		typedef Gfx::CachedGeometry<Api::vertex_buffer_type, Api::index_buffer_type> cached_geom_type;
		typedef Gfx::CachedMeshInterface<cached_geom_type, texture_type> cached_mesh_type;
	public:
		DeviceInterface () {			
		}
		void initialize(ivec2 size) {
			api_.initialize(size);
		}
		void draw(const draw_op_type& draw_op) {
			api_.draw(draw_op);
		}
		shader_program_type load_shader(string vs_path, string fs_path) {
			return shader_program_type(api_.load_shader(vs_path, fs_path));
		}
		texture_type create_depth(ivec2 size) {
			return texture_type(api_.create_depth(size));
		}
		texture_type create_texture(texformats::GfxTextureFormat format, ivec2 size, const char* data) {
			return texture_type(api_.create_texture(format, size, data));
		}
		draw_target_type create_draw_target(const vector<texture_type>& texture, optional<texture_type>& depth) {		
			return draw_target_type(api_.create_draw_target(texture, depth));
		}
		void present() {
			api_.present();
		}		
		cached_mesh_type cache_mesh(const BakedMesh& baked_meshs) {
			auto cached_geom = api_.cache_geom(mesh.geometry);
			optional<GfxTexture> albedo_tex;
			if(mesh.albedo_tex) {
				albedo_tex = make_gfx_texture(mesh.albedo_tex->texture_data);
			}
			return cached_mesh_type(mesh, albedo_tex, cached_geom);
		}
	private:
		Api api_;
	};
	typedef OpenGLApi GfxApi;
	//typedef Direct3D11Api GfxApi
	typedef DeviceInterface<GfxApi> Device;
	typedef Device::texture_type Texture;
	typedef Device::shader_program_type Shader;
	typedef Device::draw_op_type DrawOp;
	typedef Device::draw_target_type DrawTarget;
}
