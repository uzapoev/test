#include "renderer_gl.h"

#pragma comment(lib, "opengl32.lib")

#include <glext/glext.h>
#include "glextinit.h"


#if defined (_DEBUG) || !defined(NDEBUG)
#define CHECK_GL(str, ...)     CheckGLError(str,  ##__VA_ARGS__)
#define CHECK_GL_FBO(str, ...) CheckFrameBufferStatus(str, ## __VA_ARGS__)
#else
#define CHECK_GL(str, ...)
#define CHECK_GL_FBO(str, ...)
#endif

static void CheckGLError(const char *str, ...)
{
    GLenum error = glGetError();
    if (error == GL_NO_ERROR)
        return;
    switch (error)
    {
        case GL_INVALID_ENUM:                   printf("\n%s(GL_INVALID_ENUM)", str);       break;
        case GL_INVALID_VALUE:                  printf("\n%s(GL_INVALID_VALUE)", str);      break;
        case GL_INVALID_OPERATION:              printf("\n%s(GL_INVALID_OPERATION)", str);  break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:  printf("\n%s(GL_INVALID_FRAMEBUFFER_OPERATION)", str); break;
        case GL_STACK_OVERFLOW:                 printf("\n%s(GL_STACK_OVERFLOW)", str);     break;
        case GL_STACK_UNDERFLOW:                printf("\n%s(GL_STACK_UNDERFLOW)", str);    break;
        case GL_OUT_OF_MEMORY:                  printf("\n%s(GL_OUT_OF_MEMORY)", str);      break;
        default:                                printf("%s(UNKNOW_ERROR) %d", str, error);  return;
    }
}

static const char * _vattribnames[eVertexAttrib_Count] =
{
    "position", "color", "normal", "tangent",
    "texcoord0", "texcoord1", "texcoord2", "texcoord3",
    "texcoord4", "texcoord5", "texcoord6", "texcoord7"
    "weights", "indices", "INVALID",
};

static const uint16_t _vformatstrides[eVertexFormat_Count] =
{
    0, //eVertexFormat_Invalid
    4, 8, 12, 16,
    4, 8, //half floats
    4, 8, // short
    4, 8, // ushort
    4    // byte4
};

static bool s_enabledVertexAttribFlag[eVertexAttrib_Count] = { 0 };

//static_assert(_countof(_vformat2dxformat) == eVertexFormat_Count, "");
static_assert(_countof(_vattribnames) == eVertexAttrib_Count, "");

bool RendererGl::initialize(long handle)
{
#ifdef _WIN32
    m_hdc = GetDC((HWND)handle);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    {
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cAlphaBits = 8;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;
    }
    int pformat = ChoosePixelFormat(m_hdc, &pfd);
    if (!SetPixelFormat(m_hdc, pformat, &pfd))
        return false;

    m_hrc = wglCreateContext(m_hdc);
    wglMakeCurrent(m_hdc, m_hrc);
#endif

    initGlExtensions();

    printf("\n*************************** Render Info *****************************\n*");
    printf("\n* OpenGL vendor:   %s", glGetString(GL_VENDOR));
    printf("\n* OpenGL renderer: %s", glGetString(GL_RENDERER));
    printf("\n* OpenGL version:  %s", glGetString(GL_VERSION));
    printf("\n* Shader version:  %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("\n*********************************************************************");

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    return true;
}

void RendererGl::release()
{

}

void RendererGl::begin()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
}

void RendererGl::end()
{

}

void RendererGl::present()
{
#ifdef _WIN32
  /*  HDC hdc = m_currentSwapchain != SwapChainHandle::invalidHandle() ?
        ((SwapChainInfo*)m_currentSwapchain.value)->hdc : m_hdc;

    SwapBuffers(hdc);*/
    SwapBuffers(m_hdc);
#elif defined (_APPLE_)
    glSwapAPPLE();
#else 
    assert(0);
#endif
}


uint32_t RendererGl::create_swapchain(long handle)
{
    return 0;
}


void RendererGl::bind_swapchain(uint32_t swapchain)
{
}


uint64_t RendererGl::create_vdecl(VertexAttribute * atribs, size_t count)
{
    VDeclaration vdecl = {};
    for (size_t i = 0; i < count; ++i)
    {
        size_t flag = 1 << atribs[i].type;

        if ((flag & vdecl.flag) == flag)
         assert(false);

        vdecl.attributes[i] = atribs[i];
        vdecl.flag |= flag;
        vdecl.stride += _vformatstrides[atribs[i].format];
    }
    vdecl.count = count;
    m_declarations.push_back(vdecl);
  //  return m_declarations.size();
    return RenderResource::makehandle(m_declarations.size(), eResourceType_vdecl);
}

uint64_t RendererGl::create_vb(void * data, size_t size, bool dynamic)
{
    GLuint vb = 0;
    GLenum usage = dynamic ? GL_STREAM_DRAW : GL_STATIC_DRAW;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glResource res = {};
    res.id = vb;
    res.target = GL_ARRAY_BUFFER;

    CHECK_GL(__FUNCTION__, __FILE__, __LINE__);

    m_resources.push_back(res);
    return RenderResource::makehandle(m_resources.size(), eResourceType_vb);
}

uint64_t RendererGl::create_ib(void * data, size_t size, bool dynamic)
{
    return 0;
}

uint64_t RendererGl::create_texture2d(uint16_t width, uint16_t height, int format, int mips, void * data)
{
    return 0;
}

uint64_t RendererGl::create_texture3d(uint16_t width, uint16_t height, uint16_t depth, int format, int mips, void * data)
{
    return 0;
}

uint64_t RendererGl::create_textureCube(uint16_t width, uint16_t height, int format, int mips, void * data)
{
    return 0;
}


uint64_t RendererGl::create_shader(void * _vdata, size_t _size, void * _pdata, size_t _psize)
{
    static const char * gl_ver = "#version 120\n";
    static const char * gl_presc = "#define lowp \n#define mediump \n#define highp\n";
    static const char * gl_common = "vec4 tex2D( sampler2D sampler, vec2 texcoord ) { return texture2D( sampler, texcoord.xy ); }\n"; // TODO: move it into shader generation

    auto compile = [](GLenum type, const GLchar* const * string, GLsizei count)->GLuint {
        char str[4096] = "";
        GLint compiled = 0;
        GLint logLength = 0;

        GLuint shader = glCreateShader(type);
        glShaderSource(shader, count, string, NULL);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        glGetShaderInfoLog(shader, logLength, &logLength, str);

        if (!compiled) {
            int linecount = 0;
            printf("\n shader compilation error: %s \n(%d)\t", str, linecount++);
            for (GLsizei i = 0; i < count; ++i) {
                const GLchar* curr = string[i];
                while (curr && *curr != '\0') {
                    (curr && ((*curr == '\n'))) ? printf("\n(%d)\t", linecount++) : printf("%c", *curr);
                    curr++;
                }
            }
            glDeleteShader(shader);
        }
        return compiled ? shader : 0;
    };

    const char *vdata[] = { /*gl_ver, gl_presc,*/(const char*)_vdata };
    const char *fdata[] = { /*gl_ver, gl_presc, gl_common,*/ (const char*)_pdata };

    GLuint program = glCreateProgram();

    GLuint vertexShader = compile(GL_VERTEX_SHADER, vdata, _countof(vdata));
    GLuint fragmentShader = compile(GL_FRAGMENT_SHADER, fdata, _countof(fdata));
    if (vertexShader == 0 || fragmentShader == 0)
    {
        glDeleteProgram(program);
        return 0;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    for (int i = eVertexAttrib_Position; i < eVertexAttrib_BoneIndices; ++i)
        glBindAttribLocation(program, i, _vattribnames[i]);

    glLinkProgram(program);

    GLint compiled = 0;
    GLint logLen = 0;
    char  logStr[65536] = "";
    glGetProgramiv(program, GL_LINK_STATUS, &compiled);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);

    if (!compiled)
    {
        glGetProgramInfoLog(program, sizeof(logStr), NULL, logStr);
        glDeleteProgram(program);
        printf("GLSL compile log = '%s'\n", logStr);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    //get uniforms info
    GLint unicount = 0;
    #define GL_ACTIVE_UNIFORM_BLOCKS          0x8A36

    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &unicount);
    for (GLint i = 0; i < unicount; ++i)
    {
        GLsizei uni_len = 0;
        GLint uni_size = 0;
        GLenum uni_type = 0;
        char uniformbuff[256] = "";
        glGetActiveUniform(program, i, sizeof(uniformbuff), &uni_len, &uni_size, &uni_type, uniformbuff);

        auto location = glGetUniformLocation(program, "positions[0]");

        uniformbuff[0] = uniformbuff[0];
    }

   
    glResource res = {};
    res.id = program;
    m_resources.push_back(res);

    CHECK_GL(__FUNCTION__, __FILE__, __LINE__);
    return RenderResource::makehandle(m_resources.size(), eResourceType_shader);
}

uint64_t RendererGl::create_pipeline(uint64_t vdeclid, uint64_t shaderid, RenderStates * rstates /*uint64_t renderpass*/)
{
    if (!RenderResource::match(vdeclid, eResourceType_vdecl) || !RenderResource::match(shaderid, eResourceType_shader))
    {
        printf("vertex declaration or shader declaration invalid");
        return 0;
    }

    Pipeline pipeline;
    pipeline.shader = shaderid;
    pipeline.vdecl = vdeclid;
    m_pipelines.push_back(pipeline);

    //return m_pipelines.size();
    return RenderResource::makehandle(m_pipelines.size(), eResourceType_pipeline);
}

uint64_t RendererGl::create_renderpass(/*colorformats * formats, siz_t count, VkFormat depthFormat*/)
{
    return 0;
}

uint32_t RendererGl::uniform(uint64_t shader, const char * name)
{
    if (!RenderResource::match(shader, eResourceType_shader))
    {
        printf("vertex declaration or shader declaration invalid");
        return 0;
    }

    uint32_t id = RenderResource::getid(shader);
    auto & prog = m_resources[id - 1];
    // get uniforms
    // return uniform id
    GLint location = glGetUniformLocation(prog.id, name);
    return 0;
}

void RendererGl::update_uniform(uint32_t id, eUniformFormat type, const void *data, size_t size)
{
    GLint location = id;
    GLsizei count = 1;
    switch(type)
    {
        case eUniformFormat_float: glUniform1fv(location, count, static_cast<const float*>(data)); break;
        case eUniformFormat_fvec2: glUniform2fv(location, count, static_cast<const float*>(data)); break;
        case eUniformFormat_fvec4: glUniform4fv(location, count, static_cast<const float*>(data)); break;
         
        case eUniformFormat_int:   glUniform1iv(location, count, static_cast<const GLint*>(data)); break;
        case eUniformFormat_ivec2: glUniform2iv(location, count, static_cast<const GLint*>(data)); break;
        case eUniformFormat_ivec4: glUniform4iv(location, count, static_cast<const GLint*>(data)); break;
         
        case eUniformFormat_mat3: glUniformMatrix3fv(location, count, false, static_cast<const GLfloat*>(data)); break;
        case eUniformFormat_mat4: glUniformMatrix4fv(location, count, false, static_cast<const GLfloat*>(data)); break;
        
        case eUniformFormat_sampler:
        break;
    }

}

void RendererGl::update_bufferdata(uint64_t vb, void * data, size_t size, size_t offset)
{
    if (!RenderResource::match(vb, eResourceType_vb))
    {
        printf("mismatch vertex buffer type");
        return;
    }
    uint32_t id = RenderResource::getid(vb);
    auto & buf = m_resources[id - 1];

    glBindBuffer(buf.target, buf.id);
    glBufferSubData(buf.target, offset, size, data);
}


void RendererGl::destroy_resource(uint64_t id)
{

}

void RendererGl::bind_pipeline(uint64_t pipid)
{
    if (pipid == 0 || !RenderResource::match(pipid, eResourceType_pipeline))
    {
        printf("\n invalid pipline");
        m_curr_vdecl = 0;
        return;
    }

    uint32_t id = RenderResource::getid(pipid);

    auto & pipeline = m_pipelines[id - 1];

    auto shaderid = RenderResource::getid(pipeline.shader);

    auto & shader = m_resources[shaderid - 1];
    m_curr_vdecl = pipeline.vdecl;

    glUseProgram(shader.id);
    CHECK_GL(__FUNCTION__, __FILE__, __LINE__);
}

void RendererGl::bind_vb(uint64_t vb)
{
    if (vb == 0 || !RenderResource::match(vb, eResourceType_vb))
    {
        printf("\n invalid vertex buffer");
        return;
    }

    uint32_t id = RenderResource::getid(vb);

    auto & buf = m_resources[id - 1];
    glBindBuffer(buf.target, buf.id);

    if (m_curr_vdecl){
        apply_vdecl(m_curr_vdecl);
    //    m_curr_vdecl = 0;
    }
    CHECK_GL(__FUNCTION__, __FILE__, __LINE__);
}

void RendererGl::bind_ib(uint64_t ib)
{

}

void RendererGl::bind_texture(uint64_t texture, uint16_t slot)
{

}

void RendererGl::draw_array(uint32_t start_vert, uint32_t vert_count)
{
    glDrawArrays(GL_TRIANGLES, start_vert, vert_count);
}

void RendererGl::draw_indexed(uint32_t idxcount)
{

}

void RendererGl::apply_vdecl(uint64_t vdeclid)
{
    if (vdeclid == 0 || !RenderResource::match(vdeclid, eResourceType_vdecl))
    {
        printf("\n invalid vertex declaration");
        return;
    }

    uint32_t id = RenderResource::getid(vdeclid);
    auto & vdecl = m_declarations[id - 1];

    size_t stride = vdecl.stride;
    size_t count = vdecl.count;
    for (int i = eVertexAttrib_Position; i < eVertexAttrib_Count; ++i)
    {
        eVertexAttrib attrib = (eVertexAttrib)i;
        bool enabled = (vdecl.flag & (1 << attrib)) == (1 << attrib);  //vdecl->has(attrib);
        if (enabled != s_enabledVertexAttribFlag[attrib])
        {
            if (enabled)
                glEnableVertexAttribArray(attrib);
            else
                glDisableVertexAttribArray(attrib);
            //      CHECK_GL(__FUNCTION__, __FILE__, __LINE__);
            s_enabledVertexAttribFlag[attrib] = enabled;
        }
    }

    GLint offset = 0;
    for (size_t i = 0; i < count; ++i)
    {
        const auto & layer = vdecl.attributes[i];
        GLint size = 0;
        GLenum type = 0;
        GLboolean normalized = GL_FALSE;
        switch (layer.format)
        {
            case eVertexFormat_float1: size = 1;  type = GL_FLOAT; normalized = GL_FALSE; break;        // float
            case eVertexFormat_float2: size = 2;  type = GL_FLOAT; normalized = GL_FALSE; break;        // vec2f
            case eVertexFormat_float3: size = 3;  type = GL_FLOAT; normalized = GL_FALSE; break;        // vec3f
            case eVertexFormat_float4: size = 4;  type = GL_FLOAT; normalized = GL_FALSE; break;        // vec4f

            case eVertexFormat_half2:  size = 2;  type = GL_HALF_FLOAT; normalized = GL_FALSE; break;   // Two 16 bit floating value
            case eVertexFormat_half4:  size = 4;  type = GL_HALF_FLOAT; normalized = GL_FALSE; break;   // Four 16 bit floating value

            case eVertexFormat_short2:  size = 2;  type = GL_SHORT; normalized = GL_FALSE; break;       // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
            case eVertexFormat_short4:  size = 4;  type = GL_SHORT; normalized = GL_FALSE; break;       // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)

            case eVertexFormat_ushort2: size = 2;  type = GL_SHORT; normalized = GL_FALSE;  break;      // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
            case eVertexFormat_ushort4: size = 4;  type = GL_SHORT; normalized = GL_FALSE;  break;      // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)

            case eVertexFormat_byte4:  size = 4;  type = GL_UNSIGNED_BYTE; normalized = GL_TRUE; break; // Each of 4 bytes is normalized by dividing to 255.0
        }

        glVertexAttribPointer(layer.type, size, type, normalized, stride, (char*)NULL + offset);
        offset += _vformatstrides[layer.format];
    }
    CHECK_GL(__FUNCTION__, __FILE__, __LINE__);
}