#pragma once
#include "Asset.h"
#include "CompiledAsset.h"
#include "Gfx.h"
#include "OGL.h"
#include "Render.h"
using namespace Asset;
using namespace CompiledAsset;

using namespace Asset;
using namespace CompiledAsset;

Camera move_camera_using_keyboard(GLFWwindow* window, const Camera& p_camera, double move_speed) {
	auto camera = p_camera;
	if (glfwGetKey(window, 'W') == GLFW_PRESS) {
		camera = camera.move(vec3(0, 0, move_speed));
	} else if (glfwGetKey(window, 'S') == GLFW_PRESS) {
		camera = camera.move(vec3(0, 0, -move_speed));
	}
	if (glfwGetKey(window, 'A') == GLFW_PRESS) {
		camera = camera.move(vec3(move_speed, 0, 0));
	} else if (glfwGetKey(window, 'D') == GLFW_PRESS) {
		camera = camera.move(vec3(-move_speed, 0, 0));
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE)) {
		camera = camera.move(vec3(0, move_speed, 0));
	} else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) {
		camera = camera.move(vec3(0, -move_speed, 0));
	}
	return camera;
}
void main_loop() {

    const float clear_z = 1.0f;
	//string path = "nff/sphere.nff"; string cache_name = "sphere.cache"; string texture_path = ""; const double move_speed = 0.02; auto flags = (aiPostProcessSteps) (aiProcess_GenSmoothNormals | aiProcess_Triangulate);
	//string path = "asset_obj/sponza.obj"; string cache_name = "sponza.cache"; const double move_speed = 2;  auto flags = aiProcess_Triangulate;
	//string path = "asset_obj/san-miguel.obj"; string cache_name = "sam-miguel.cache"; const double move_speed = 2;  auto flags = (aiPostProcessSteps)0;
	string path = "asset_obj/crytek-sponza/sponza.obj"; string texture_path =  "asset_obj/crytek-sponza/"; string cache_name = "crytek-sponza.cache"; const double move_speed = 2;  auto flags = aiProcess_Triangulate;
	//string path = "asset_obj/dabrovic-sponza/sponza.obj"; string texture_path =  "asset_obj/dabrovic-sponza/"; string cache_name = "dabrovic-sponza.cache"; const double move_speed = 2;  auto flags = (aiPostProcessSteps)(aiProcess_Triangulate | aiProcess_GenNormals);
	//string path = "asset_obj/sibenik/sibenik.obj"; string texture_path =  "asset_obj/sibenik/"; string cache_name = "sibenik.cache"; const double move_speed = .2;  auto flags = (aiPostProcessSteps)(aiProcess_Triangulate | aiProcess_GenNormals);
	string cache_path = "cache/" + cache_name;
	if(!boost::filesystem::exists(cache_path)) {
		auto meshes = asset_load_meshes(path, texture_path, flags); 
		auto baked_meshes = bake_meshes(meshes);
		ofstream ofs(cache_path, fstream::trunc | fstream::binary);		
		assert(ofs.is_open());
		archive::binary_oarchive output(ofs);
		output << baked_meshes;
		ofs.close();
	} 
	ifstream ifs(cache_path, fstream::binary);
	vector<BakedMesh> baked_meshes;
	archive::binary_iarchive input(ifs);
	input >> baked_meshes;


    Degree fov(60);
    auto window_opt = init_glfw();
    if (!window_opt) {
        return;
    }
    auto window = *window_opt;
    if (glewInit() != GLEW_OK)
        return;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    Camera camera(vec3(0, 0, -2), vec3(0, 0, 1), vec3(0, 1, 0), fov, 1, 10000, width / (float)height);// / 1080.f);


    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(ogl_debug_callback, nullptr);
	GLuint unusedIds = 0;
	glDebugMessageControl(GL_DONT_CARE,
		GL_DONT_CARE,
		GL_DONT_CARE,
		0,
		&unusedIds,
		true);

    auto cached_meshes = cache_baked_meshes(baked_meshes);

    auto gbuffer = make_gbuffer(width, height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    auto gen_gbuffer_program = load_gen_gbuffer_program();
    validate_program(gen_gbuffer_program);
    int frame_idx = 0;

    auto shade_gbuffer_resources = make_shade_gbuffer_resources();
    validate_program(shade_gbuffer_resources.shade_gbuffer_program);
    DirectionalLight light0 = { normalize(vec3(-1, -1, 0)), vec3(1,0,0) };

    while (!glfwWindowShouldClose(window)) {
        glClearDepth(1);
        glClear(GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        camera = move_camera_using_keyboard(window, camera, move_speed);
        activate_and_clear_framebuffer(gbuffer.framebuffer, clear_z);

        //for (auto& cached_mesh : cached_meshes) {
		for(size_t idx = 0; idx < cached_meshes.size(); idx++) {
			auto& cached_mesh = cached_meshes[idx];
			auto drawop = make_gen_gbuffer_drawop(camera, gen_gbuffer_program, shade_gbuffer_resources, gbuffer, 
				cached_mesh);
            draw(drawop);
        }

        auto shade_gbuffer_state = make_shade_gbuffer_state(gbuffer, shade_gbuffer_resources,
            vector<DirectionalLight>(&light0, &light0 + 1));
        draw(shade_gbuffer_state);

        glfwSwapBuffers(window);
        glfwPollEvents();
        frame_idx++;
    }
}