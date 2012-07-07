#include "Engine.h"
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include "Logger.h"
#include "ParamReader.h"

static bool initialized = false;
static LoggerPtr logger;

static MatrixStack viewStack;
static MatrixStack projStack;

static int loadResources();

static struct
{
    GLuint colorProgram;
    GLuint rectBuffer;
} resources;

int initEngine(const glm::vec2 &resolution)
{
    if (initialized)
        return 1;

    if (!logger.get())
        logger = Logger::getLogger("Engine");

    if (SDL_Init(SDL_INIT_VIDEO))
    {
        logger->fatal() << "Couldn't initialize SDL: " <<
            SDL_GetError() << '\n';
        return 0;
    }

    int flags = SDL_OPENGL;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Surface *screen = SDL_SetVideoMode(resolution.x, resolution.y, 32, flags);
    if (screen == NULL)
    {
        logger->fatal() << "Couldn't set video mode: " << SDL_GetError() << '\n';
        return 0;
    }
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        logger->fatal() <<  "Error: " << glewGetErrorString(err) << '\n';
        return 0;
    }

    logger->info() << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';

    SDL_WM_SetCaption((strParam("window.name") + "  BUILT  " __DATE__ " " __TIME__).c_str(), "rts");
    SDL_WM_GrabInput(SDL_GRAB_ON);

    initialized = true;

    if (!loadResources())
    {
        logger->fatal() << "Unable to load resources, bailing...\n";
        return 0;
    }

    // success
    return 1;
}

void teardownEngine()
{
    if (!initialized)
        return;

    SDL_Quit();
    initialized = false;
}

void renderRectangleColor(const glm::mat4 &modelMatrix, const glm::vec4 &color)
{
    GLuint program = resources.colorProgram;
    GLuint projectionUniform = glGetUniformLocation(program, "projectionMatrix");
    GLuint modelViewUniform = glGetUniformLocation(program, "modelViewMatrix");
    GLuint colorUniform = glGetUniformLocation(program, "color");

    // Enable program and set up values
    glUseProgram(program);
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projStack.current()));
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewStack.current() * modelMatrix));
    glUniform4fv(colorUniform, 1, glm::value_ptr(color));

    // Bind attributes
    glBindBuffer(GL_ARRAY_BUFFER, resources.rectBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    // RENDER
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Clean up
    glDisableVertexAttribArray(0);
    glUseProgram(0);
}

void renderRectangleProgram(const glm::mat4 &modelMatrix)
{
	GLuint program;
	glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &program);
	if (!program)
	{
		logger->warning() << "No active program on call to renderRectangleProgram\n";
		return;
	}
    GLuint projectionUniform = glGetUniformLocation(program, "projectionMatrix");
    GLuint modelViewUniform = glGetUniformLocation(program, "modelViewMatrix");

    // Enable program and set up values
    glUseProgram(program);
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projStack.current()));
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewStack.current() * modelMatrix));

    // Bind attributes
    glBindBuffer(GL_ARRAY_BUFFER, resources.rectBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    // RENDER
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Clean up
    glDisableVertexAttribArray(0);
    glUseProgram(0);
}

MatrixStack& getViewStack()
{
    return viewStack;
}

MatrixStack& getProjectionStack()
{
    return projStack;
}

GLuint loadProgram(const std::string &vert, const std::string &frag)
{
	GLuint program;
    GLuint vertsh = loadShader(GL_VERTEX_SHADER, vert);
    GLuint fragsh = loadShader(GL_FRAGMENT_SHADER, frag);
    program = linkProgram(vertsh, fragsh);
    glDeleteShader(vertsh);
    glDeleteShader(fragsh);

	return program;
}

GLuint loadShader(GLenum type, const std::string &filename)
{
    // Load file
    std::ifstream file(filename.c_str());
    if (!file)
    {
        logger->error() << "Unable to read shader from " << filename << '\n';
        return 0;
    }
    std::string shaderText;
    std::getline(file, shaderText, (char)EOF);
    const char *text = shaderText.c_str();
    GLint len = shaderText.length();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &text, &len);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

        const char *strShaderType = NULL;
        switch(type)
        {
        case GL_VERTEX_SHADER: strShaderType = "vertex"; break;
        case GL_GEOMETRY_SHADER: strShaderType = "geometry"; break;
        case GL_FRAGMENT_SHADER: strShaderType = "fragment"; break;
        }

        fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
        delete[] strInfoLog;
        return 0;
    }

    return shader;
}

GLuint linkProgram(GLuint vert, GLuint frag)
{
    GLuint program = glCreateProgram();

    glAttachShader(program, vert);
    glAttachShader(program, frag);

    glLinkProgram(program);

    GLint status;
    glGetProgramiv (program, GL_LINK_STATUS, &status);
    if (!status)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
        fprintf(stderr, "Linker failure: %s\n", strInfoLog);
        delete[] strInfoLog;
        return 0;
    }

    glDetachShader(program, vert);
    glDetachShader(program, frag);

    return program;
}

// -------- STATIC FUNCTIONS ---------

static int loadResources()
{
    const float rectPositions[] = {
        0.5, 0.5, 0.0f, 1.0f,
        0.5, -0.5, 0.0f, 1.0f,
        -0.5, 0.5, 0.0f, 1.0f,
        -0.5, -0.5, 0.0f, 1.0f,
    };

    glGenBuffers(1, &resources.rectBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, resources.rectBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectPositions), rectPositions, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create solid color program for renderRectangleColor
	resources.colorProgram = loadProgram("shaders/color.v.glsl", "shaders/color.f.glsl");

    return 1;
}

