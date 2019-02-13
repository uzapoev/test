#ifndef __GLEXTTINY_H__
#define __GLEXTTINY_H__

#ifdef __cplusplus
extern "C"
{
#endif
    static PFNGLGENVERTEXARRAYSPROC                 glGenVertexArrays = NULL;
    static PFNGLBINDVERTEXARRAYPROC                 glBindVertexArray = NULL;
    static PFNGLBINDATTRIBLOCATIONPROC              glBindAttribLocation = NULL;
    static PFNGLVERTEXATTRIBPOINTERPROC             glVertexAttribPointer = NULL;
    static PFNGLENABLEVERTEXATTRIBARRAYPROC         glEnableVertexAttribArray = NULL;
    static PFNGLDISABLEVERTEXATTRIBARRAYPROC        glDisableVertexAttribArray = NULL;

    static PFNGLGENFRAMEBUFFERSPROC                 glGenFramebuffers = NULL;
    static PFNGLBINDFRAMEBUFFERPROC                 glBindFramebuffer = NULL;
    static PFNGLGENRENDERBUFFERSPROC                glGenRenderbuffers = NULL;
    static PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC  glRenderbufferStorageMultisample = NULL;
    static PFNGLBINDRENDERBUFFERPROC                glBindRenderbuffer = NULL;

    static PFNGLFRAMEBUFFERRENDERBUFFERPROC         glFramebufferRenderbuffer = NULL;
    static PFNGLFRAMEBUFFERTEXTURE2DPROC            glFramebufferTexture2D = NULL;
    static PFNGLBLITFRAMEBUFFERPROC                 glBlitFramebuffer = NULL;
    static PFNGLDRAWBUFFERSPROC                     glDrawBuffers = NULL;

    static PFNGLCHECKFRAMEBUFFERSTATUSPROC              glCheckFramebufferStatus = NULL;
    static PFNGLGETRENDERBUFFERPARAMETERIVPROC          glGetRenderbufferParameteriv = NULL;
    static PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv = NULL;

    //PFNGLCREATEBUFFERSPROC  glCreateBuffers;
    static PFNGLGENBUFFERSPROC              glGenBuffers = NULL;
    static PFNGLDELETEBUFFERSPROC           glDeleteBuffers = NULL;
    static PFNGLBINDBUFFERPROC              glBindBuffer = NULL;
    static PFNGLBUFFERDATAPROC              glBufferData = NULL;
    static PFNGLBUFFERSUBDATAPROC           glBufferSubData = NULL;
    static PFNGLUNMAPBUFFERPROC             glUnmapBuffer = NULL;
    static PFNGLMAPBUFFERPROC               glMapBuffer = NULL;

    static PFNGLACTIVETEXTUREPROC           glActiveTexture = NULL;
    static PFNGLTEXIMAGE3DPROC              glTexImage3D = NULL;
    static PFNGLCOMPRESSEDTEXIMAGE2DPROC    glCompressedTexImage2D = NULL;
    static PFNGLCOMPRESSEDTEXIMAGE3DPROC    glCompressedTexImage3D = NULL;
    static PFNGLGETCOMPRESSEDTEXIMAGEPROC   glGetCompressedTexImage = NULL;
    static PFNGLGENERATEMIPMAPPROC          glGenerateMipmap = NULL;

    static PFNGLCREATEPROGRAMPROC           glCreateProgram = NULL;
    static PFNGLDELETEPROGRAMPROC           glDeleteProgram = NULL;
    static PFNGLLINKPROGRAMPROC             glLinkProgram = NULL;
    static PFNGLUSEPROGRAMPROC              glUseProgram = NULL;
    static PFNGLGETPROGRAMIVPROC            glGetProgramiv = NULL;
    static PFNGLGETPROGRAMINFOLOGPROC       glGetProgramInfoLog = NULL;
    static PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation = NULL;

    static PFNGLGETACTIVEUNIFORMARBPROC     glGetActiveUniform = NULL;


    static PFNGLCREATESHADERPROC            glCreateShader = NULL;
    static PFNGLDELETESHADERPROC            glDeleteShader = NULL;
    static PFNGLSHADERSOURCEPROC            glShaderSource = NULL;
    static PFNGLCOMPILESHADERPROC           glCompileShader = NULL;
    static PFNGLATTACHSHADERPROC            glAttachShader = NULL;
    static PFNGLGETSHADERIVPROC             glGetShaderiv = NULL;
    static PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog = NULL;

    static PFNGLUNIFORM1IPROC               glUniform1i  = NULL;
    static PFNGLUNIFORM1FVPROC              glUniform1fv = NULL;
    static PFNGLUNIFORM2FVPROC              glUniform2fv = NULL;
    static PFNGLUNIFORM3FVPROC              glUniform3fv = NULL;
    static PFNGLUNIFORM4FVPROC              glUniform4fv = NULL;

    static PFNGLUNIFORM1IVPROC              glUniform1iv = NULL;
    static PFNGLUNIFORM2IVPROC              glUniform2iv = NULL;
    static PFNGLUNIFORM3IVPROC              glUniform3iv = NULL;
    static PFNGLUNIFORM4IVPROC              glUniform4iv = NULL;

    static PFNGLUNIFORMMATRIX2FVPROC        glUniformMatrix2fv = NULL;
    static PFNGLUNIFORMMATRIX3FVPROC        glUniformMatrix3fv = NULL;
    static PFNGLUNIFORMMATRIX4FVPROC        glUniformMatrix4fv = NULL;

    static PFNGLBLENDFUNCSEPARATEPROC       glBlendFuncSeparate = NULL;
    static PFNGLBLENDEQUATIONSEPARATEPROC   glBlendEquationSeparate = NULL;
}



static void * getGlProcAddress(const char * name)
{
#ifdef _WIN32
    return (void *)::wglGetProcAddress(name);
#elif defined (PLATFORM_APPLE)
    return NULL;//(void *)getProcAddress(name);
#else
    return (void *)::glXGetProcAddressARB((const GLubyte *)name);
#endif
}


static void initGlExtensions()
{
#define WGPA    getGlProcAddress
    glActiveTexture             =   (PFNGLACTIVETEXTUREPROC)        WGPA("glActiveTexture");
    glTexImage3D                =   (PFNGLTEXIMAGE3DPROC)           WGPA("glTexImage3D");
    glCompressedTexImage2D      =   (PFNGLCOMPRESSEDTEXIMAGE2DPROC) WGPA("glCompressedTexImage2D");
    glCompressedTexImage3D      =   (PFNGLCOMPRESSEDTEXIMAGE3DPROC) WGPA("glCompressedTexImage3D");
    glGetCompressedTexImage     =   (PFNGLGETCOMPRESSEDTEXIMAGEPROC)WGPA("glGetCompressedTexImage");
    glGenerateMipmap            =   (PFNGLGENERATEMIPMAPPROC)       WGPA("glGenerateMipmap");

    // VBO
    glGenBuffers                = (PFNGLGENBUFFERSPROC)             WGPA("glGenBuffers");
    glDeleteBuffers             = (PFNGLDELETEBUFFERSPROC)          WGPA("glDeleteBuffers");
    glBindBuffer                = (PFNGLBINDBUFFERPROC)             WGPA("glBindBuffer");
    glBufferData                = (PFNGLBUFFERDATAPROC)             WGPA("glBufferData");
    glBufferSubData             = (PFNGLBUFFERSUBDATAPROC)          WGPA("glBufferSubData");
    glUnmapBuffer               = (PFNGLUNMAPBUFFERPROC)            WGPA("glUnmapBuffer");
    glMapBuffer                 = (PFNGLMAPBUFFERPROC)              WGPA("glMapBuffer");

    // VAO
    glGenVertexArrays           = (PFNGLGENVERTEXARRAYSPROC)        WGPA("glGenVertexArrays");
    glBindVertexArray           = (PFNGLBINDVERTEXARRAYPROC)        WGPA("glBindVertexArray");
    glBindAttribLocation        = (PFNGLBINDATTRIBLOCATIONPROC)     WGPA("glBindAttribLocation");
    glVertexAttribPointer       = (PFNGLVERTEXATTRIBPOINTERPROC)    WGPA("glVertexAttribPointer");
    glEnableVertexAttribArray   = (PFNGLENABLEVERTEXATTRIBARRAYPROC)WGPA("glEnableVertexAttribArray");
    glDisableVertexAttribArray  = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)WGPA("glDisableVertexAttribArray");

    // Shaders
    glCreateProgram             = (PFNGLCREATEPROGRAMPROC)          WGPA("glCreateProgram");
    glDeleteProgram             = (PFNGLDELETEPROGRAMPROC)          WGPA("glDeleteProgram");
    glLinkProgram               = (PFNGLLINKPROGRAMPROC)            WGPA("glLinkProgram");
    glUseProgram                = (PFNGLUSEPROGRAMPROC)             WGPA("glUseProgram");
    glGetProgramiv              = (PFNGLGETPROGRAMIVPROC)           WGPA("glGetProgramiv");
    glGetProgramInfoLog         = (PFNGLGETPROGRAMINFOLOGPROC)      WGPA("glGetProgramInfoLog");
    glGetUniformLocation        = (PFNGLGETUNIFORMLOCATIONPROC)     WGPA("glGetUniformLocation");
    glGetActiveUniform          = (PFNGLGETACTIVEUNIFORMARBPROC)    WGPA("glGetActiveUniform");

    glCreateShader              = (PFNGLCREATESHADERPROC)           WGPA("glCreateShader");
    glDeleteShader              = (PFNGLDELETESHADERPROC)           WGPA("glDeleteShader");
    glShaderSource              = (PFNGLSHADERSOURCEPROC)           WGPA("glShaderSource");
    glCompileShader             = (PFNGLCOMPILESHADERPROC)          WGPA("glCompileShader");
    glAttachShader              = (PFNGLATTACHSHADERPROC)           WGPA("glAttachShader");
    glGetShaderiv               = (PFNGLGETSHADERIVPROC)            WGPA("glGetShaderiv");
    glGetShaderInfoLog          = (PFNGLGETSHADERINFOLOGPROC)       WGPA("glGetShaderInfoLog");

    glUniform1i                 = (PFNGLUNIFORM1IPROC)              WGPA("glUniform1i");
    glUniform1fv                = (PFNGLUNIFORM1FVPROC)             WGPA("glUniform1fv");
    glUniform2fv                = (PFNGLUNIFORM2FVPROC)             WGPA("glUniform2fv");
    glUniform3fv                = (PFNGLUNIFORM3FVPROC)             WGPA("glUniform3fv");
    glUniform4fv                = (PFNGLUNIFORM4FVPROC)             WGPA("glUniform4fv");

    glUniform1iv                = (PFNGLUNIFORM2IVPROC)             WGPA("glUniform1iv");
    glUniform2iv                = (PFNGLUNIFORM2IVPROC)             WGPA("glUniform2iv");
    glUniform3iv                = (PFNGLUNIFORM3IVPROC)             WGPA("glUniform3iv");
    glUniform4iv                = (PFNGLUNIFORM4IVPROC)             WGPA("glUniform4iv");

    glUniformMatrix2fv          = (PFNGLUNIFORMMATRIX2FVPROC)       WGPA("glUniformMatrix2fv");
    glUniformMatrix3fv          = (PFNGLUNIFORMMATRIX3FVPROC)       WGPA("glUniformMatrix3fv");
    glUniformMatrix4fv          = (PFNGLUNIFORMMATRIX4FVPROC)       WGPA("glUniformMatrix4fv");

    // blend
    glBlendFuncSeparate         = (PFNGLBLENDFUNCSEPARATEPROC)      WGPA("glBlendFuncSeparate");
    glBlendEquationSeparate     = (PFNGLBLENDEQUATIONSEPARATEPROC)  WGPA("glBlendEquationSeparate");

    // FBO
    glGenFramebuffers           = (PFNGLGENFRAMEBUFFERSPROC)        WGPA("glGenFramebuffers");
    glBindFramebuffer           = (PFNGLBINDFRAMEBUFFERPROC)        WGPA("glBindFramebuffer");
    glGenRenderbuffers          = (PFNGLGENRENDERBUFFERSPROC)       WGPA("glGenRenderbuffers");
    glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)WGPA("glRenderbufferStorageMultisample");
    glBindRenderbuffer          = (PFNGLBINDRENDERBUFFERPROC)       WGPA("glBindRenderbuffer");

    glFramebufferRenderbuffer   = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)WGPA("glFramebufferRenderbuffer");
    glFramebufferTexture2D      = (PFNGLFRAMEBUFFERTEXTURE2DPROC)   WGPA("glFramebufferTexture2D");
    glBlitFramebuffer           = (PFNGLBLITFRAMEBUFFERPROC)        WGPA("glBlitFramebuffer");
    glDrawBuffers               = (PFNGLDRAWBUFFERSPROC)            WGPA("glDrawBuffers");

    glCheckFramebufferStatus    = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)WGPA("glCheckFramebufferStatus");
    glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)WGPA("glGetRenderbufferParameteriv");
    glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)WGPA("glGetFramebufferAttachmentParameteriv");
}                                                      
                                                       
#endif