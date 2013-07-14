#pragma once
#include "pch.h"
#include "Gfx.h"
struct GBuffer {
	GBuffer(GfxFramebuffer p_framebuffer, GfxTexture p_diffuse, GfxTexture p_normal, GfxRenderBuffer p_depth) :
diffuse(p_diffuse), normal(p_normal), depth(p_depth), framebuffer(p_framebuffer) {
}

GfxTexture diffuse;
GfxTexture normal;
GfxRenderBuffer depth;
GfxFramebuffer framebuffer;
};
struct DirectionalLight {
	vec3 incident;
	vec3 color;
};

struct GenGBufferProgram {
	struct Uniforms {
		static const GfxUniformSpec VIEW_PROJ;
	};
	struct Textures {
		static const GfxUniformSpec ALBEDO;
	};
};
struct ShadeGBufferProgram {
	struct Uniforms {
		static const GfxUniformSpec LIGHT_INCIDENT;
	};
	struct Textures {
		static const GfxUniformSpec DIFFUSE;
		static const GfxUniformSpec NORMAL;
	};
};
const GfxUniformSpec GenGBufferProgram::Uniforms::VIEW_PROJ = { "view_proj", UniformTypes::Matrix4 };
const GfxUniformSpec GenGBufferProgram::Textures::ALBEDO = { "in_albedo", UniformTypes::Sampler };
const GfxUniformSpec ShadeGBufferProgram::Textures::DIFFUSE = { "in_diffuse", UniformTypes::Sampler };
const GfxUniformSpec ShadeGBufferProgram::Textures::NORMAL = { "in_normal", UniformTypes::Sampler };
const GfxUniformSpec ShadeGBufferProgram::Uniforms::LIGHT_INCIDENT = { "in_light_incident", UniformTypes::Vec3 };


GfxProgram<GenGBufferProgram> load_gen_gbuffer_program() {
    vector<GLShaderId> shaders;

	shaders.push_back(create_shader(GL_VERTEX_SHADER, read_all("shaders/gen_gbuffer.vs")));
    shaders.push_back(create_shader(GL_FRAGMENT_SHADER, read_all("shaders/gen_gbuffer.fs")));

    GLProgramId program_id = ogl_create_program(shaders);
    GLUniformLocationId view_proj_loc = {
        glGetUniformLocation(program_id, GenGBufferProgram::Uniforms::VIEW_PROJ.name.c_str())};

    map<GfxUniformSpec, GLUniformLocationId> uniforms;
	uniforms[GenGBufferProgram::Uniforms::VIEW_PROJ] = view_proj_loc;

    return GfxProgram<GenGBufferProgram>(program_id, uniforms);
}
GfxProgram<ShadeGBufferProgram> load_shade_gbuffer_program() {
    vector<GLShaderId> shaders;
	shaders.push_back(create_shader(GL_VERTEX_SHADER, read_all("shaders/shade_gbuffer.vs")));
    shaders.push_back(create_shader(GL_FRAGMENT_SHADER, read_all("shaders/shade_gbuffer.fs")));
    GLProgramId program_id = ogl_create_program(shaders);

    GLUniformLocationId diffuse_loc = { glGetUniformLocation(program_id, ShadeGBufferProgram::Textures::DIFFUSE.name.c_str())};
    GLUniformLocationId normal_loc = { glGetUniformLocation(program_id, ShadeGBufferProgram::Textures::NORMAL.name.c_str())};
    GLUniformLocationId light_incident_loc = {glGetUniformLocation(program_id, ShadeGBufferProgram::Uniforms::LIGHT_INCIDENT.name.c_str())};
    map<GfxUniformSpec, GLUniformLocationId> uniforms;
	uniforms[ShadeGBufferProgram::Textures::DIFFUSE] = diffuse_loc;
    uniforms[ShadeGBufferProgram::Textures::NORMAL] = normal_loc;
	uniforms[ShadeGBufferProgram::Uniforms::LIGHT_INCIDENT] = light_incident_loc;
	/*
    for(auto kv : uniforms) {
        printf("%s, %d\n", kv.first.name.c_str(), kv.first.type);
    } */
	for(auto it = uniforms.begin(); it != uniforms.end(); it++) {
		printf("%s, %d\n", it->first.name.c_str(), it->first.type);
    }
    return GfxProgram<ShadeGBufferProgram>(program_id, uniforms);
}

struct Camera {
	vec3 eye;
	vec3 forward;
	vec3 up;

	Degree fov;
	float z_near;
	float z_far;
	float aspect_ratio;
	Camera(vec3 p_eye, vec3 p_forward, vec3 p_up, Degree p_fov, float p_z_near, float p_z_far,
		float p_aspect_ratio) :
	eye(p_eye), forward(p_forward), up(p_up), fov(p_fov), z_near(p_z_near), z_far(p_z_far), aspect_ratio(
		p_aspect_ratio) {
	}
	mat4 view_matrix() const {
		return lookAt(eye, eye + forward, up);
	}
	mat4 projection_matrix() const {
		return perspective(fov.value, aspect_ratio, z_near, z_far);
	}
	Camera move(const vec3& dp) const {
		Camera new_camera(*this);
		new_camera.eye += dp;
		return new_camera;
	}
};

GBuffer make_gbuffer(int width, int height) {
	GfxTexture diffuse = make_gfx_texture(width, height, texformats::GfxTextureFormat::RGBA16F);
	GfxTexture normal = make_gfx_texture(width, height, texformats::GfxTextureFormat::RGBA16F);
	GfxRenderBuffer depth = make_renderbuffer(width, height);
	vector<GfxTexture> textures; 
	textures.push_back(diffuse);
	textures.push_back(normal);
	GfxFramebuffer framebuffer = make_frame_buffer(textures, depth);
	return GBuffer(framebuffer, diffuse, normal, depth);
}

struct ShadeGBufferResources {
	ShadeGBufferResources(const GfxSampler& p_sampler,
		const GfxGeometryBuffers& p_fsquad,
		const GfxProgram<ShadeGBufferProgram>& p_shade_gbuffer_program)
		: sampler(p_sampler), fsquad(p_fsquad), shade_gbuffer_program(p_shade_gbuffer_program) { }
	GfxSampler sampler;
	GfxGeometryBuffers fsquad;
	GfxProgram<ShadeGBufferProgram> shade_gbuffer_program;
};


GfxDrawOperation<GenGBufferProgram> make_gen_gbuffer_drawop(
	const Camera& camera, 
	const GfxProgram<GenGBufferProgram>& gen_gbuffer_program,
	const ShadeGBufferResources& shade_gbuffer_resources, 
	const GBuffer& gbuffer, 
	const GfxCachedMesh& cached_mesh)
{
	auto& draw_buffer = cached_mesh.geom_buf;

	map<GfxUniformSpec, GfxUniformValue> uniform_values;
	uniform_values[GenGBufferProgram::Uniforms::VIEW_PROJ] = GfxUniformValue::make(camera.projection_matrix() * camera.view_matrix());

	map<GfxUniformSpec, GfxTexture> textures;
	if(cached_mesh.albedo_tex) {
		textures[GenGBufferProgram::Textures::ALBEDO] = *(cached_mesh.albedo_tex);
	}

	GfxShaderState<GenGBufferProgram> shader_state(gen_gbuffer_program,
		uniform_values, textures, shade_gbuffer_resources.sampler);
	GfxGraphicsState<GenGBufferProgram> state(gbuffer.framebuffer, shader_state);
	return GfxDrawOperation<GenGBufferProgram>(state, draw_buffer);	
}

ShadeGBufferResources make_shade_gbuffer_resources() {
	return ShadeGBufferResources (make_sampler(), make_fsquad(), load_shade_gbuffer_program());
}

GfxDrawOperation<ShadeGBufferProgram> make_shade_gbuffer_state(
	const GBuffer& gbuffer,
	const ShadeGBufferResources& shade_gbuffer_resources,
	const vector<DirectionalLight>& lights) {
		assert(lights.size() == 1); //not supported otherwise
		auto light_incident_uniform_value = GfxUniformValue::make(lights[0].incident);

		map<GfxUniformSpec, GfxUniformValue> uniform_vals;
		uniform_vals[ShadeGBufferProgram::Uniforms::LIGHT_INCIDENT] = light_incident_uniform_value;

		map<GfxUniformSpec, GfxTexture> texture_vals;
		texture_vals[ShadeGBufferProgram::Textures::DIFFUSE] = gbuffer.diffuse;
		texture_vals[ShadeGBufferProgram::Textures::NORMAL] = gbuffer.normal;

		GfxShaderState<ShadeGBufferProgram> shader_state(
			shade_gbuffer_resources.shade_gbuffer_program,
			uniform_vals,
			texture_vals, 
			shade_gbuffer_resources.sampler);
		GfxGraphicsState<ShadeGBufferProgram> gfx_state(GfxFramebuffer::default_fb(), shader_state);
		return GfxDrawOperation<ShadeGBufferProgram>(gfx_state, shade_gbuffer_resources.fsquad);
}
