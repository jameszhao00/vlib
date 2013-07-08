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
struct DirectionalLight {
    vec3 incident;
    vec3 color;
};
GLRenderBufferId make_renderbuffer(int width, int height) {
    GLuint renderbuffer;
    glGenRenderbuffers(1, &renderbuffer);
    glNamedRenderbufferStorageEXT(renderbuffer, GL_DEPTH_COMPONENT, width, height);
    return GLRenderBufferId::make(renderbuffer);
}
GLTextureId make_texture(int width, int height) {
    GLTextureId texture;
    glGenTextures(1, texture);
    glTextureImage2DEXT(texture, GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
    GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTextureParameteriEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
    GL_NEAREST);
    glTextureParameteriEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    GL_NEAREST);
    return texture;
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
        return (name < rhs.name)
            ;//|| (type < rhs.type);
    }
};
struct GenGBufferProgram {
    struct Uniforms {
        static const GfxUniformSpec VIEW_PROJ;
    };
    struct Textures {
    };
};
struct ShadeGBufferProgram {
    struct Uniforms {
        static const GfxUniformSpec LIGHT_INCIDENT;
    };
    struct Textures {
        static const GfxUniformSpec ALBEDO;
        static const GfxUniformSpec NORMAL;
    };
};
const GfxUniformSpec GenGBufferProgram::Uniforms::VIEW_PROJ = { "view_proj", UniformTypes::Matrix4 };
const GfxUniformSpec ShadeGBufferProgram::Textures::ALBEDO = { "in_albedo", UniformTypes::Sampler };
const GfxUniformSpec ShadeGBufferProgram::Textures::NORMAL = { "in_normal", UniformTypes::Sampler };
const GfxUniformSpec ShadeGBufferProgram::Uniforms::LIGHT_INCIDENT = { "in_light_incident", UniformTypes::Vec3 };

template<typename TProgram>
struct GfxProgram {
    GfxProgram(GLProgramId p_id, const map<GfxUniformSpec, GLUniformLocationId>& p_uniforms) :
        id(p_id), uniforms(p_uniforms) {
    }
    GLProgramId id;
    map<GfxUniformSpec, GLUniformLocationId> uniforms;
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
        glGetUniformLocation(program_id, GenGBufferProgram::Uniforms::VIEW_PROJ.name.c_str())};
    map<GfxUniformSpec, GLUniformLocationId> uniforms = {
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

    GLUniformLocationId albedo_loc = { glGetUniformLocation(program_id, ShadeGBufferProgram::Textures::ALBEDO.name.c_str())};
    GLUniformLocationId normal_loc = { glGetUniformLocation(program_id, ShadeGBufferProgram::Textures::NORMAL.name.c_str())};
    GLUniformLocationId light_incident_loc = {glGetUniformLocation(program_id, ShadeGBufferProgram::Uniforms::LIGHT_INCIDENT.name.c_str())};
    map<GfxUniformSpec, GLUniformLocationId> uniforms = {
        {ShadeGBufferProgram::Textures::ALBEDO, albedo_loc},
        {ShadeGBufferProgram::Textures::NORMAL, normal_loc},
        {ShadeGBufferProgram::Uniforms::LIGHT_INCIDENT, light_incident_loc}
    };
    for(auto kv : uniforms) {
        printf("%s, %d\n", kv.first.name.c_str(), kv.first.type);
    }
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
size_t size(UniformTypes type) {
    if(type == Matrix4) {
        return 4 * 4 * sizeof(float);
    } else if(type == Vec3) {
        return 3 * sizeof(float);
    } else {
        return -1; //should really throw an error...
    }
}

size_t stride(vec3 _) {
    return sizeof(float);
}
size_t stride(mat4 _) {
    return sizeof(float);
}
size_t byte_size(vec3 _) {
    return sizeof(float) * 3;
}
size_t byte_size(mat4 _) {
    return sizeof(float) * 4 * 4;
}
struct GfxUniformValue {
    size_t stride;
    vector<char> buffer;
    template<typename T>
    static GfxUniformValue make(const T& v) {
        char* start = (char*)value_ptr(v);
        char* end = start + byte_size(v);
        return { ::stride(v), vector<char>(start, end) };
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
        const GfxDrawBuffers& p_draw_buffers) :
            state(p_state), draw_buffers(p_draw_buffers) { }
    GfxGraphicsState<TProgram> state;
    GfxDrawBuffers draw_buffers;
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

GfxFramebuffer make_frame_buffer(const vector<GfxTexture>& textures, const GfxRenderBuffer& depth_buffer) {
    GLFramebufferId frame_buffer;
    glGenFramebuffers(1, frame_buffer);
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
    if (glfwGetKey(window, GLFW_KEY_SPACE)) {
        camera = camera.move(vec3(0, move_speed, 0));
    } else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) {
        camera = camera.move(vec3(0, -move_speed, 0));
    }
    return camera;
}
template<typename TProgram>
void apply_uniform(GfxProgram<TProgram> program, GLUniformLocationId uniform_loc,
    GfxUniformSpec spec, GfxUniformValue val) {
    auto val_buffer_size = val.buffer.size();
    assert(size(spec.type) == val_buffer_size);
    //ext = we dont need to bind program
    if(spec.type == UniformTypes::Matrix4) {
        glProgramUniformMatrix4fvEXT(program.id, uniform_loc.id, 1, GL_FALSE, (GLfloat*)val.buffer.data());
    } else if(spec.type == UniformTypes::Vec3) {
        glProgramUniform3fvEXT(program.id, uniform_loc.id, 1, (GLfloat*)val.buffer.data());
    } else {
        assert(false);
    }
}
template<typename TKey, typename TValue>
const TValue& get(const map<TKey, TValue>& src, const TKey& key) {

    auto r = src.find(key);
    if(r == src.end()) {
        for(auto& kv : src) {
            printf("%s, %d\n", kv.first.name.c_str(), kv.first.type);
        }
        auto r2 = src.find(key);
        printf("%s", r2);
        assert(false);
    }
    return r->second;
}

template<typename TProgram>
void apply_state(const GfxGraphicsState<TProgram>& state) {
    if(state.framebuffer) {
        activate_frame_buffer(*state.framebuffer);
    }
    if(state.shader_state) {
        auto& shader_state = *state.shader_state;
        glUseProgram(shader_state.program.id);
        for (auto uniform : shader_state.uniforms) {
            auto uniform_spec = uniform.first;
            auto uniform_value = uniform.second;
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

    glDrawElements(GL_TRIANGLES, draw_buffers.num_indices, GL_UNSIGNED_INT, 0);
}
GfxSampler make_sampler() {
    GLSamplerId sampler_id;
    glGenSamplers(1, sampler_id);
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
struct ShadeGBufferResources {
    ShadeGBufferResources(const GfxSampler& p_sampler,
        const GfxDrawBuffers& p_fsquad,
        const GfxProgram<ShadeGBufferProgram>& p_shade_gbuffer_program)
        : sampler(p_sampler), fsquad(p_fsquad), shade_gbuffer_program(p_shade_gbuffer_program) { }
    GfxSampler sampler;
    GfxDrawBuffers fsquad;
    GfxProgram<ShadeGBufferProgram> shade_gbuffer_program;
};
ShadeGBufferResources make_shade_gbuffer_resources() {
    return ShadeGBufferResources (make_sampler(), make_fsquad(), load_shade_gbuffer_program());
}
GfxDrawOperation<ShadeGBufferProgram> make_shade_gbuffer_state(
    const GBuffer& gbuffer,
    const ShadeGBufferResources& shade_gbuffer_resources,
    const vector<DirectionalLight>& lights) {
    assert(lights.size() == 1); //not supported otherwise
    auto light_incident_uniform_value = GfxUniformValue::make(lights[0].incident);
    GfxShaderState<ShadeGBufferProgram> shader_state(
        shade_gbuffer_resources.shade_gbuffer_program,
        {
            { ShadeGBufferProgram::Uniforms::LIGHT_INCIDENT, light_incident_uniform_value}
        },
        {
            { ShadeGBufferProgram::Textures::ALBEDO, gbuffer.albedo },
            { ShadeGBufferProgram::Textures::NORMAL, gbuffer.normal }
        }, shade_gbuffer_resources.sampler);
    GfxGraphicsState<ShadeGBufferProgram> gfx_state(GfxFramebuffer::default_fb(), shader_state);
    return GfxDrawOperation<ShadeGBufferProgram>(gfx_state, shade_gbuffer_resources.fsquad);
}

template<typename TProgram>
void validate_program(const GfxProgram<TProgram>& prog) {
    for (auto& kv : prog.uniforms) {
        assert(kv.second.id != -1);
    }
}

void main_loop() {

    const float clear_z = 1.0f;
    //auto geometries = load_mesh_asset("nff/sphere.nff", aiProcess_GenSmoothNormals); const double move_speed = 0.02;
    auto geometries = load_mesh_asset("asset_obj/sponza.obj", (aiPostProcessSteps) 0); const double move_speed = 2;
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

        for (auto& draw_buffer : draw_buffers) {
            map<GfxUniformSpec, GfxUniformValue> uniform_values = { { GenGBufferProgram::Uniforms::VIEW_PROJ,
                GfxUniformValue::make(camera.projection_matrix() * camera.view_matrix()) } };
            GfxShaderState<GenGBufferProgram> shader_state(gen_gbuffer_program,
                uniform_values, { }, shade_gbuffer_resources.sampler);
            GfxGraphicsState<GenGBufferProgram> state(gbuffer.framebuffer, shader_state);
            GfxDrawOperation<GenGBufferProgram> drawop(state, draw_buffer);
            draw(drawop);
        }

        auto shade_gbuffer_state = make_shade_gbuffer_state(gbuffer, shade_gbuffer_resources,
            { light0 });
        draw(shade_gbuffer_state);

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
