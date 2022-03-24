/*
 *  main.c
 *  Patater Shader Kit
 *
 *  Created by Jaeden Amero on 2022-03-24.
 *  Copyright 2022. SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <SDL.h>
#include <glad/gl.h>
#include <stdio.h>
#include <stdlib.h>

enum
{
    SCREEN_WIDTH = 640,
    SCREEN_HEIGHT = 480
};

static SDL_Window *window;
static SDL_GLContext sdlGLContext;
static int isRunning;

/* Shader variables */
static GLint vPosition = -1;

static GLint fResolution = -1;
static GLint fMouse = -1;
static GLint fTime = -1;
static GLint fFrame = -1;

/* Shader management variables */
static GLuint programID;
static GLuint pointVBO;
static GLuint myVAO;

static GLfloat resolution[] = {
    SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2,
};

static GLfloat mouse[] = {
    0.5f,  0.5f
};

static GLfloat points[] = {
    -1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f, -1.0f,
     1.0f,  1.0f
};

static void printProgramInfoLog(GLuint program)
{
    GLchar *log;
    GLsizei length = 0;
    GLsizei maxLength = 1;

    if (!glIsProgram(program))
    {
        fprintf(stderr, "%d is not a valid program ID\n", program);
        return;
    }

    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

    log = calloc(maxLength, 1);

    glGetProgramInfoLog(program, maxLength, &length, log);
    if (length > 0)
    {
        printf("%s\n", log);
    }

    free(log);
}

static void printShaderInfoLog(GLuint shader)
{
    GLchar *log;
    GLsizei length = 0;
    GLsizei maxLength = 1;

    if (!glIsShader(shader))
    {
        fprintf(stderr, "%d is not a valid shader ID\n", shader);
        return;
    }

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

    log = calloc(maxLength, 1);

    glGetShaderInfoLog(shader, maxLength, &length, log);
    if (length > 0)
    {
        printf("%s\n", log);
    }

    free(log);
}

static const char *shaderTypeName(GLenum shaderType)
{
    switch (shaderType)
    {
    case GL_VERTEX_SHADER:
        return "vertex";
    case GL_GEOMETRY_SHADER:
        return "geometry";
    case GL_FRAGMENT_SHADER:
        return "fragment";
    }

    return "unknown";
}

static int setShader(GLuint program, GLenum shaderType, const GLchar *source)
{
    GLuint shader;
    GLint isCompiled;

    shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);

    glCompileShader(shader);
    isCompiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

    if (isCompiled != GL_TRUE)
    {
        fprintf(stderr,
            "Unable to compile %s shader %d\n", shaderTypeName(shaderType), shader);
        printShaderInfoLog(shader);
        exit(1);
    }

    glAttachShader(program, shader);

    return 0;
}

static size_t fileSize(FILE *f)
{
    size_t size;

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    rewind(f);

    return size;
}

static char *fileContents(FILE *f)
{
    size_t size;
    char *contents;
    size_t num_read;

    size = fileSize(f);
    contents = malloc(size + 1); /* Plus 1 for NUL termination */
    if (!contents)
    {
        fprintf(stderr, "Failed to create buffer for file\n");
        exit(1);
    }

    num_read = fread(contents, 1, size, f);
    if (num_read != size)
    {
        fprintf(stderr, "Failed to read contents\n");
        exit(1);
    }

    /* NUL-terminate the content string. */
    contents[num_read] = '\0';

    return contents;
}

static int loadShader(GLuint program, GLenum shaderType, const char *path)
{
    FILE *f;
    char *source;

    f = fopen(path, "rb");
    if (!f)
    {
        fprintf(stderr, "Failed to load %s\n", path);
        exit(1);
    }

    source = fileContents(f);

    setShader(program, shaderType, source);
    free(source);
    fclose(f);

    return 0;
}

static int initGL(void)
{
    GLint isLinked;

    /* TODO Verbose Info Log */
    printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
    printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    printf("GL_SHADING_LANGUAGE_VERSION:\n\t%s\n",
        glGetString(GL_SHADING_LANGUAGE_VERSION));

    programID = glCreateProgram();

    loadShader(programID, GL_VERTEX_SHADER, "shaders/vertex.vert");
    loadShader(programID, GL_FRAGMENT_SHADER, "fragment.frag");

    glLinkProgram(programID);

    isLinked = GL_FALSE;
    glGetProgramiv(programID, GL_LINK_STATUS, &isLinked);
    if (isLinked != GL_TRUE)
    {
        fprintf(stderr, "Error linking program %d\n", programID);
        printProgramInfoLog(programID);
        exit(1);
    }

    vPosition = glGetAttribLocation(programID, "position");
    if (vPosition == -1)
    {
        fprintf(stderr, "Couldn't find location of 'position'\n");
        exit(1);
    }

    /* Get optional uniforms. Some shaders don't need these, but if the shader
     * requests them, we should populate them at render time. */
    fResolution = glGetUniformLocation(programID, "resolution");
    fMouse = glGetUniformLocation(programID, "mouse");
    fTime = glGetUniformLocation(programID, "time");
    fFrame = glGetUniformLocation(programID, "frame");

    /* Create VBO */
    glGenBuffers(1, &pointVBO);
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

    /* Create VAO */
    glGenVertexArrays(1, &myVAO);
    glBindVertexArray(myVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    glVertexAttribPointer(vPosition, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(vPosition);

    /* Enable Backface culling */
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    /* Disable depth (we just use two flat triangles) */
    glDisable(GL_DEPTH_TEST);

    /* Disable blending, as we don't need OpenGL to blend with previous values
     * in the frame buffer. */
    glDisable(GL_BLEND);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    return 0;
}

static int createWindow(const char *name)
{
    window = SDL_CreateWindow(name, SDL_WINDOWPOS_UNDEFINED,
                  SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT,
                  SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL |
                  SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == NULL)
    {
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
        exit(1);
    }

    sdlGLContext = SDL_GL_CreateContext(window);

    /* Load GL functions */
    gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);

    /* Use Vsync */
    if (SDL_GL_SetSwapInterval(1) < 0)
    {
        printf("Warning: Unable to use VSync! SDL Error: %s\n",
            SDL_GetError());
    }

    initGL();

    return 0;
}

static int render(void)
{
    static GLuint frame = 0;

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(programID);
    glBindVertexArray(myVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (fResolution != -1)
    {
        glUniform2fv(fResolution, 1, resolution);
    }

    /* Update time if shader wanted it */
    if (fTime != -1)
    {
        GLfloat time;
        time = SDL_GetTicks64() / 1000.0f;
        glUniform1fv(fTime, 1, &time);
    }

    if (fFrame != -1)
    {
        glUniform1uiv(fFrame, 1, &frame);
    }

    if (fMouse != -1)
    {
        glUniform2fv(fMouse, 1, mouse);
    }

    glUseProgram(0);

    SDL_GL_SwapWindow(window);

    ++frame;

    return 0;
}

static void handleKey(SDL_KeyboardEvent k)
{
    switch (k.keysym.sym)
    {
    case SDLK_ESCAPE:
    case SDLK_q:
        exit(1);
    }
}

static void handleEvents(void)
{
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        switch(e.type)
        {
        case SDL_QUIT:
        {
            exit(0);
        }
        case SDL_MOUSEMOTION:
        {
            SDL_MouseMotionEvent mm = e.motion;

            mouse[0] = mm.x / (float)SCREEN_WIDTH;
            mouse[1] = 1 - mm.y / (float)SCREEN_HEIGHT;
            break;
        }
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            handleKey(e.key);
            break;
        case SDL_WINDOWEVENT:
        case SDL_TEXTEDITING:
        case SDL_TEXTINPUT:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL:
            break;
        }
    }
}

int main()
{
    int ret;

    ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret < 0)
    {
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "1", SDL_HINT_OVERRIDE);

    /* Don't disable the compositor. */
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    createWindow("Patater Shader Kit");

    isRunning = 1;

    while (isRunning)
    {
        handleEvents();
        render();
    }

    return 0;
}
