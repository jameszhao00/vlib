// vlib.cpp : Defines the entry point for the console application.
#include "vlib.h"

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
struct GLIndexBufferId: GlId<GLIndexBufferId> {
};
struct GLVertexBufferId: GlId<GLVertexBufferId> {
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
        return GLFramebufferId::make(0);
    }
};
struct GLTextureId: GlId<GLTextureId> {
};
struct GLRenderBufferId: GlId<GLRenderBufferId> {
};
struct GLSamplerId: GlId<GLSamplerId> {
};
struct GfxTexture {
    GfxTexture(GLTextureId p_id) :
        id(p_id) {
    }
    GLTextureId id;
};
struct GfxSampler {
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
        return GfxFramebuffer(GLFramebufferId::default_framebuffer(), {}, nullptr);
    }
private:
};

struct GBuffer {
    GBuffer(GfxFramebuffer p_framebuffer, GfxTexture p_albedo, GfxTexture p_normal, GfxRenderBuffer p_depth) :
        albedo(p_albedo), normal(p_normal), depth(p_depth), framebuffer(p_framebuffer) {
    }

    GfxTexture albedo;
    GfxTexture normal;
    GfxRenderBuffer depth;
    GfxFramebuffer framebuffer;
};

GLRenderBufferId make_renderbuffer(int width, int height) {
    GLuint renderbuffer;
    glGenRenderbuffers(1, &renderbuffer);
    glNamedRenderbufferStorageEXT(renderbuffer, GL_DEPTH_COMPONENT, width, height);
    return GLRenderBufferId::make(renderbuffer);
}
GLTextureId make_texture(int width, int height) {
    GLTextureId texture;
    glGenTextures(1, &texture.value);
    glTextureImage2DEXT(texture, GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
    GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTextureParameteriEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
    GL_NEAREST);
    glTextureParameteriEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    GL_NEAREST);
    return texture;
}
struct GenGBufferProgram {
    struct Uniforms {
        static const char* VIEW_PROJ;
    };
    struct Textures {
    };
};
struct ShadeGBufferProgram {
    struct Uniforms {
    };
    struct Textures {
        static const char* ALBEDO;
        static const char* NORMAL;
    };
};
const char* GenGBufferProgram::Uniforms::VIEW_PROJ = "view_proj";
const char* ShadeGBufferProgram::Textures::ALBEDO = "albedo";
const char* ShadeGBufferProgram::Textures::NORMAL = "normal";

template<typename TProgram>
struct GfxProgram {
    GfxProgram(GLProgramId p_program_id, const map<string, GLUniformLocationId>& p_uniforms) :
        program_id(p_program_id), uniforms(p_uniforms) {
    }
    GLProgramId program_id;
    map<string, GLUniformLocationId> uniforms;
};
struct Degree {
    float value;
};
string read_all(string path) {
    std::ifstream ifs(path);
    return string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}
GLProgramId ogl_create_program(const vector<GLShaderId>& shaders) {
    GLProgramId program = GLProgramId::make(glCreateProgram());

    for (auto shader : shaders) {
        glAttachShader(program, shader);
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

GfxProgram<GenGBufferProgram> load_gen_gbuffer_program() {
    vector<GLShaderId> shaders = {
        create_shader(GL_VERTEX_SHADER, read_all("shaders/gen_gbuffer.vs")),
        create_shader(GL_FRAGMENT_SHADER, read_all("shaders/gen_gbuffer.fs"))
    };
    GLProgramId program_id = ogl_create_program(shaders);
    GLUniformLocationId view_proj_loc = {
        glGetUniformLocation(program_id, GenGBufferProgram::Uniforms::VIEW_PROJ)};
    map<string, GLUniformLocationId> uniforms = {
        {GenGBufferProgram::Uniforms::VIEW_PROJ, view_proj_loc}
    };
    return GfxProgram<GenGBufferProgram>(program_id, uniforms);
}
GfxProgram<ShadeGBufferProgram> load_shade_gbuffer_program() {
    vector<GLShaderId> shaders = {
        create_shader(GL_VERTEX_SHADER, read_all("shaders/shade_gbuffer.vs")),
        create_shader(GL_FRAGMENT_SHADER, read_all("shaders/shade_gbuffer.fs"))
    };
    GLProgramId program_id = ogl_create_program(shaders);

    GLUniformLocationId albedo_loc = { glGetUniformLocation(program_id, ShadeGBufferProgram::Textures::ALBEDO)};
    GLUniformLocationId normal_loc = { glGetUniformLocation(program_id, ShadeGBufferProgram::Textures::NORMAL)};

    map<string, GLUniformLocationId> uniforms = {
        {ShadeGBufferProgram::Textures::ALBEDO, albedo_loc},
        {ShadeGBufferProgram::Textures::NORMAL, normal_loc}
    };

    return GfxProgram<ShadeGBufferProgram>(program_id, uniforms);
}
void error_callback(int error, const char* description) {
    printf("%s", description);
}
void APIENTRY ogl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, void* userParam) {
    printf("ERROR: %s", message);
}

struct Vertex {
    Vertex(vec3 p_position, vec3 p_normal) :
        position(p_position), normal(p_normal) {
    }
    vec3 position;
    vec3 normal;
};

template<typename T>
struct BufferData {
    BufferData(const vector<T>& p_data) :
        data(p_data) {
    }
    vector<T> data;
    int count() const {
        return data.size();
    }
};

struct GeometryBufferData {
    GeometryBufferData(const BufferData<float>& p_positions, const BufferData<float>& p_normals,
        const BufferData<unsigned int>& p_indices) :
        positions(p_positions), normals(p_normals), indices(p_indices) {
    }
    BufferData<float> positions;
    BufferData<float> normals;
    BufferData<unsigned int> indices;
    int num_vertices() const {
        return indices.count();
    }
};
struct Geometry {
    Geometry(const vector<Vertex>& p_verticies, const vector<unsigned int>& p_indices) :
        vertices(p_verticies), indices(p_indices) {
    }
    vector<Vertex> vertices;
    vector<unsigned int> indices;
};
vec3 to_vec3(const aiVector3D& v) {
    return vec3(v.x, v.y, v.z);
}
vector<Geometry> load_mesh_asset(string path, aiPostProcessSteps aux_flags) {
    const aiScene* scene = aiImportFile(path.c_str(),
        aiProcess_Triangulate | aiProcess_PreTransformVertices | aux_flags);
    vector<Geometry> geometries;
    for (auto mesh_idx = 0u; mesh_idx < scene->mNumMeshes; mesh_idx++) {
        auto mesh = scene->mMeshes[mesh_idx];
        assert(mesh->HasNormals());
        vector<Vertex> vertices;
        for (auto vert_idx = 0u; vert_idx < mesh->mNumVertices; vert_idx++) {
            auto ai_vertex = mesh->mVertices[vert_idx];
            auto ai_normal = mesh->mNormals[vert_idx];
            vertices.push_back(Vertex(to_vec3(ai_vertex), to_vec3(ai_normal)));
        }
        vector<unsigned int> indices;
        for (auto face_idx = 0u; face_idx < mesh->mNumFaces; face_idx++) {
            const auto& face = mesh->mFaces[face_idx];
            for (auto face_vert_idx = 0u; face_vert_idx < face.mNumIndices; face_vert_idx++) {
                indices.push_back(face.mIndices[face_vert_idx]);
            }
        }
        geometries.push_back(Geometry(vertices, indices));
    }
    return geometries;
}

vector<GeometryBufferData> make_buffer_data(const vector<Geometry>& geometry) {
    vector<GeometryBufferData> result;
    for (const auto& geom : geometry) {
        vector<float> positions;
        vector<float> normals;
        for (auto v : geom.vertices) {
            positions.push_back(v.position.x);
            positions.push_back(v.position.y);
            positions.push_back(v.position.z);
            normals.push_back(v.normal.x);
            normals.push_back(v.normal.y);
            normals.push_back(v.normal.z);
        }
        const vector<unsigned int>& indices = geom.indices;
        result.push_back(GeometryBufferData(positions, normals, indices));
    }
    return result;
}

template<typename T>
GLIndexBufferId make_index_buffer_and_upload_data(const BufferData<T>& buffer_data) {
    GLIndexBufferId vboID;
    glGenBuffers(1, vboID);
    glNamedBufferDataEXT(vboID, buffer_data.count() * sizeof(T), buffer_data.data.data(), GL_STATIC_DRAW);
    return vboID;
}

template<typename T>
GLVertexBufferId make_vertex_buffer_and_upload_data(const BufferData<T>& buffer_data) {
    GLVertexBufferId vboID;
    glGenBuffers(1, vboID);
    glNamedBufferDataEXT(vboID, buffer_data.count() * sizeof(T), buffer_data.data.data(), GL_STATIC_DRAW);
    return vboID;
}
struct GfxDrawBuffers {
    GfxDrawBuffers(const GLVertexBufferId& p_position_buffer, const GLVertexBufferId& p_normal_buffer,
        const GLIndexBufferId& p_index_buffer, int p_num_indices) :
        position_buffer(p_position_buffer), normal_buffer(p_normal_buffer), index_buffer(p_index_buffer), num_indices(
            p_num_indices) {
    }
    GLVertexBufferId position_buffer;
    GLVertexBufferId normal_buffer;
    GLIndexBufferId index_buffer;
    unsigned int num_indices;
};

template<typename TProgram>
struct GfxGraphicsState {
    GfxGraphicsState(const GfxFramebuffer& p_framebuffer, GfxDrawBuffers p_draw_buffers,
        GfxProgram<TProgram> p_program, const map<string, mat4>& p_uniform_mat4,
        const map<string, GfxTexture>& p_textures, GfxSampler p_samplers) :
        framebuffer(p_framebuffer), draw_buffers(p_draw_buffers), program(p_program), uniform_mat4(
            p_uniform_mat4), textures(p_textures), sampler(p_samplers) {
    }
    GfxFramebuffer framebuffer;
    GfxDrawBuffers draw_buffers;
    GfxProgram<TProgram> program;
    map<string, mat4> uniform_mat4;
    map<string, GfxTexture> textures;
    GfxSampler sampler;
};

vector<GfxDrawBuffers> make_draw_buffer_and_upload_data(
    const vector<GeometryBufferData>& geometries) {
    vector<GfxDrawBuffers> result;
    transform(begin(geometries), end(geometries), back_inserter(result),
        [](const GeometryBufferData& geom) {
            auto positions = make_vertex_buffer_and_upload_data(geom.positions);
            auto normals = make_vertex_buffer_and_upload_data(geom.normals);
            auto indices = make_index_buffer_and_upload_data(geom.indices);
            return GfxDrawBuffers(positions, normals, indices, geom.indices.count());
        });
    return result;
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
optional<GLFWwindow*> init_glfw() {
    const int width = 1920, height = 1080;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    if (!glfwInit())
        return optional<GLFWwindow*>();
    glfwSetErrorCallback(error_callback);
    const bool is_fullscreen = true;
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

GfxFramebuffer make_frame_buffer(const vector<GfxTexture>& textures, const GfxRenderBuffer depth_buffer) {
    GLFramebufferId frame_buffer;
    glGenFramebuffers(1, &frame_buffer.value);
    for (size_t i = 0; i < textures.size(); i++) {
        glNamedFramebufferTextureEXT(frame_buffer, attachments[i], textures[i].id, 0);
    }
    glNamedFramebufferRenderbufferEXT(frame_buffer, GL_DEPTH_ATTACHMENT,
    GL_RENDERBUFFER, depth_buffer.id);

    glFramebufferDrawBuffersEXT(frame_buffer, textures.size(), attachments);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckNamedFramebufferStatusEXT(frame_buffer, GL_FRAMEBUFFER));



    return GfxFramebuffer(frame_buffer, textures, depth_buffer);
}

GBuffer make_gbuffer(int width, int height) {
    GfxTexture albedo(make_texture(width, height));
    GfxTexture normal(make_texture(width, height));
    GfxRenderBuffer depth(make_renderbuffer(width, height));
    GfxFramebuffer framebuffer = make_frame_buffer( { albedo, normal }, depth);
    return GBuffer(framebuffer, albedo, normal, depth);
}
void activate_frame_buffer(GfxFramebuffer frame_buffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);
}
void activate_default_frame_buffer() {
    activate_frame_buffer(GfxFramebuffer::default_fb());
}
void activate_and_clear_framebuffer(GfxFramebuffer frame_buffer, float z_reset) {
    activate_frame_buffer(frame_buffer);
    for(size_t i = 0; i < frame_buffer.textures.size(); i++){
        //ASSUMPTION: framebuffer textures use 0-N slots when count = N
        glClearBufferfv(GL_COLOR, i, value_ptr(vec4(0, 0, 0, 0)));
    }
    glClearBufferfv(GL_DEPTH, 0, &z_reset);
}
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
    return camera;
}

template<typename TProgram>
void draw(const GfxGraphicsState<TProgram>& state) {
    activate_frame_buffer(state.framebuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.draw_buffers.index_buffer);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, state.draw_buffers.position_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, state.draw_buffers.normal_buffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glUseProgram(state.program.program_id);
    for (auto mat4_uniform : state.uniform_mat4) {
        auto key = state.program.uniforms.find(mat4_uniform.first)->second;
        auto value = mat4_uniform.second;
        glProgramUniformMatrix4fv(state.program.program_id, key.id, 1, GL_FALSE, value_ptr(value));
    }
    {
        int texture_idx = 0;
        for (auto it = begin(state.textures); it != end(state.textures); it++, texture_idx++) {
            GLUniformLocationId loc = state.program.uniforms.find(it->first)->second;
            glUniform1i(loc.id, texture_idx);
            glActiveTexture(GL_TEXTURE0 + texture_idx);
            GfxTexture tex = it->second;
            glBindTexture(GL_TEXTURE_2D, tex.id);
            glBindSampler(0, state.sampler.id);
        }
    }
    glDrawElements(GL_TRIANGLES, state.draw_buffers.num_indices,
    GL_UNSIGNED_INT, 0);
}
GfxSampler make_sampler() {
    GLSamplerId sampler_id;
    glGenSamplers(1, &sampler_id.value);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return GfxSampler { sampler_id };
}
GfxDrawBuffers make_fsquad() {
    vector<GeometryBufferData> geom_buffer_data = { GeometryBufferData(
        BufferData<float>( { -1, 1, 0, //ul
            1, 1, 0, //ur
            1, -1, 0,  //lr
            -1, -1, 0 //ll
            }), BufferData<float>( { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }),
            BufferData<unsigned int>( { 0, 1,2, 2, 3, 0 }))};

    return make_draw_buffer_and_upload_data(geom_buffer_data)[0];
}
void main_loop() {

    const float clear_z = 1.0f;
    auto geometries = load_mesh_asset("nff/sphere.nff", aiProcess_GenSmoothNormals); const double move_speed = 0.02;
    //auto geometries = load_mesh_asset("asset_obj/sponza.obj", (aiPostProcessSteps) 0); const double move_speed = 2;
    vector<GeometryBufferData> geometry_buffer_datas = make_buffer_data(geometries);

    Degree fov { 60 };
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

    //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    //glDebugMessageCallback(ogl_debug_callback, nullptr);

    auto draw_buffers = make_draw_buffer_and_upload_data(geometry_buffer_datas);

    vector<GLShaderId> shaders = { create_shader(GL_VERTEX_SHADER, read_all("shaders/gen_gbuffer.vs")),
        create_shader(GL_FRAGMENT_SHADER, read_all("shaders/gen_gbuffer.fs")) };
    auto gbuffer = make_gbuffer(width, height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    auto gen_gbuffer_program = load_gen_gbuffer_program();
    auto shade_gbuffer_program = load_shade_gbuffer_program();
    int frame_idx = 0;
    GfxSampler sampler = make_sampler();
    auto fsquad = make_fsquad();
    while (!glfwWindowShouldClose(window)) {
        glClearDepth(1);
        glClear(GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        camera = move_camera_using_keyboard(window, camera, move_speed);
        activate_and_clear_framebuffer(gbuffer.framebuffer, clear_z);

        for (auto& draw_buffer : draw_buffers) {
            map<string, mat4> uniform_values = { { GenGBufferProgram::Uniforms::VIEW_PROJ,
                camera.projection_matrix() * camera.view_matrix() } };

            GfxGraphicsState<GenGBufferProgram> state(gbuffer.framebuffer, draw_buffer, gen_gbuffer_program,
                uniform_values, { }, sampler);
            draw(state);
        }

        GfxGraphicsState<ShadeGBufferProgram> shade_gbuffer_State(
            GfxFramebuffer::default_fb(), fsquad, shade_gbuffer_program,
            { },
            {
                { ShadeGBufferProgram::Textures::ALBEDO, gbuffer.albedo },
                { ShadeGBufferProgram::Textures::NORMAL, gbuffer.normal }
            }, sampler);
        draw(shade_gbuffer_State);

        glfwSwapBuffers(window);
        glfwPollEvents();
        frame_idx++;
    }
}

int main() {
    main_loop();
    glfwTerminate();
    return 0;
}
