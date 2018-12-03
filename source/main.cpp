// TODO: Actually put proper header here

#include <cstdio>

#include <GL/glew.h>
#include <GL/glu.h>

#include "SDL.h"
#include "SDL_opengl.h"

#include "IL/il.h"

#include "shader_fragment.h"
#include "shader_vertex.h"

// Screen dimension constants
constexpr int SCREEN_WIDTH  = 640;
constexpr int SCREEN_HEIGHT = 480;

// The window we'll be rendering to
static SDL_Window *gWindow = nullptr;

// OpenGL context
static SDL_GLContext gContext;

// Render flag
static bool gRenderQuad = true;

// Graphics program
static GLuint gProgramID = 0;
static GLint  gVertexPos2DLocation = -1;
static GLuint gVBO = 0;
static GLuint gIBO = 0;

static ILint    linearheight;
static ILint    linearwidth;
static uint8_t *linearimage;

// Shader loading utility programs

//
//
//
static void printProgramLog(GLuint program)
{
   // Make sure name is shader
   if(glIsProgram(program))
   {
      // Program log length
      int infoLogLength = 0;
      int maxLength = infoLogLength;

      // Get info string length
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

      // Allocate string
      char *infoLog = new char[maxLength];

      // Get info log
      glGetProgramInfoLog(program, maxLength, &infoLogLength, infoLog);
      if(infoLogLength > 0)
         printf("%s\n", infoLog);

      // Deallocate string
      delete[] infoLog;
   }
   else
      printf("Name %d is not a program\n", program);
}

//
//
//
static void printShaderLog(GLuint shader)
{
   // Make sure name is shader
   if(glIsShader(shader))
   {
      // Shader log length
      int infoLogLength = 0;
      int maxLength = infoLogLength;

      // Get info string length
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

      // Allocate string
      char *infoLog = new char[maxLength];

      // Get info log
      glGetShaderInfoLog(shader, maxLength, &infoLogLength, infoLog);
      if(infoLogLength > 0)
         printf("%s\n", infoLog);

      // Deallocate string
      delete[] infoLog;
   }
   else
      printf("Name %d is not a shader\n", shader);
}

//
// Initializes rendering program and clear color
//
static bool initGL()
{
   // Success flag
   bool success = true;

   // Generate program
   gProgramID = glCreateProgram();

   // Create vertex shader
   GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

   // Set vertex source
   glShaderSource(vertexShader, 1, &shader_source_vertex, nullptr);

   // Compile vertex source
   glCompileShader(vertexShader);

   // Check vertex shader for errors
   GLint vShaderCompiled = GL_FALSE;
   glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
   if(vShaderCompiled != GL_TRUE)
   {
      printf("Unable to compile vertex shader %d!\n", vertexShader);
      printShaderLog(vertexShader);
      success = false;
   }
   else
   {
      // Attach vertex shader to program
      glAttachShader(gProgramID, vertexShader);

      // Create fragment shader
      GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

      // Set fragment source
      glShaderSource(fragmentShader, 1, &shader_source_fragment, nullptr);

      // Compile fragment source
      glCompileShader(fragmentShader);

      // Check fragment shader for errors
      GLint fShaderCompiled = GL_FALSE;
      glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
      if(fShaderCompiled != GL_TRUE)
      {
         printf("Unable to compile fragment shader %d!\n", fragmentShader);
         printShaderLog(fragmentShader);
         success = false;
      }
      else
      {
         // Attach fragment shader to program
         glAttachShader(gProgramID, fragmentShader);


         // Link program
         glLinkProgram(gProgramID);

         // Check for errors
         GLint programSuccess = GL_TRUE;
         glGetProgramiv(gProgramID, GL_LINK_STATUS, &programSuccess);
         if(programSuccess != GL_TRUE)
         {
            printf("Error linking program %d!\n", gProgramID);
            printProgramLog(gProgramID);
            success = false;
         }
         else
         {
            // Get vertex attribute location
            gVertexPos2DLocation = glGetAttribLocation(gProgramID, "LVertexPos2D");
            if(gVertexPos2DLocation == -1)
            {
               printf("LVertexPos2D is not a valid glsl program variable!\n");
               success = false;
            }
            else
            {
               // Initialize clear color
               glClearColor(0.f, 0.f, 0.f, 1.f);

               // VBO data
               GLfloat vertexData[] =
               {
                  -1.0f, -1.0f,
                   1.0f, -1.0f,
                   1.0f,  1.0f,
                  -1.0f,  1.0f
               };

               // IBO data
               GLuint indexData[] = { 0, 1, 2, 3 };

               // Create VBO
               glGenBuffers(1, &gVBO);
               glBindBuffer(GL_ARRAY_BUFFER, gVBO);
               glBufferData(GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);

               // Create IBO
               glGenBuffers(1, &gIBO);
               glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
               glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indexData, GL_STATIC_DRAW);
            }
         }
      }
   }

   return success;
}

static bool IL_init(const char *filename)
{
   ILuint ImgId = 0;
   ilGenImages(1, &ImgId);
   ilBindImage(ImgId);

   ilLoadImage(filename);

   linearwidth    = ilGetInteger(IL_IMAGE_WIDTH);
   linearheight   = ilGetInteger(IL_IMAGE_HEIGHT);
   uint8_t *image = new uint8_t[linearwidth * linearheight * 3];
   ilCopyPixels(0, 0, 0, linearwidth, linearheight, 1, IL_RGB, IL_UNSIGNED_BYTE, linearimage);

   ilBindImage(0);
   ilDeleteImage(ImgId);
}

//
// Starts up SDL, creates window, and initializes OpenGL
//
static bool init()
{
   // Initialization flag
   bool success = true;

   // Initialize SDL
   if(SDL_Init(SDL_INIT_VIDEO) < 0)
   {
      printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
      success = false;
   }
   else
   {
      // Use OpenGL 3.1 core
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

      // Create window
      gWindow = SDL_CreateWindow("Max Waine Graphics Optics Coursework Final Year Project Word Salad",
                                 SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                 SCREEN_WIDTH, SCREEN_HEIGHT,
                                 SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
      if(gWindow == nullptr)
      {
         printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
         success = false;
      }
      else
      {
         // Create context
         gContext = SDL_GL_CreateContext(gWindow);
         if(gContext == nullptr)
         {
            printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
            success = false;
         }
         else
         {
            // Initialize GLEW
            glewExperimental = GL_TRUE;
            GLenum glewError = glewInit();
            if(glewError != GLEW_OK)
               printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));

            // Use Vsync
            if(SDL_GL_SetSwapInterval(1) < 0)
               printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());

            // Initialize OpenGL
            if(!initGL())
            {
               printf("Unable to initialize OpenGL!\n");
               success = false;
            }
         }
      }
   }

   return success;
}

//
// Input handler
//
static void handleKeys(unsigned char key, int x, int y)
{
   // Toggle quad
   if(key == 'q')
      gRenderQuad = !gRenderQuad;
}

//
// Per frame update
//
static void update()
{
   // No per frame update needed
}

//
// Renders quad to the screen
//
static void render()
{
   // Clear color buffer
   glClear(GL_COLOR_BUFFER_BIT);

   // Render quad
   if(gRenderQuad)
   {
      // Bind program
      glUseProgram(gProgramID);

      // Enable vertex position
      glEnableVertexAttribArray(gVertexPos2DLocation);

      // Set vertex data
      glBindBuffer(GL_ARRAY_BUFFER, gVBO);
      glVertexAttribPointer(gVertexPos2DLocation, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

      // Set index data and render
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
      glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, nullptr);

      // Disable vertex position
      glDisableVertexAttribArray(gVertexPos2DLocation);

      // Unbind program
      glUseProgram(NULL);
   }
}

//
// Frees media and shuts down SDL
//
static void close()
{
   // Deallocate program
   glDeleteProgram(gProgramID);

   // Destroy window
   SDL_DestroyWindow(gWindow);
   gWindow = nullptr;

   // Quit SDL subsystems
   SDL_Quit();
}

// FIXME: This
#ifdef _MSC_VER && !defined(_WIN32_WCE)
#undef main
#endif

int main(int argc, char *args[])
{
   // Load image into linear
   if(argc < 0)
   {
      puts("There must be an argument and it must be the relative path to the loaded image!");
      return -1;
   }

   IL_init(args[1]);

   // Start up SDL and create window
   if(!init())
   {
      puts("Failed to initialize!");
      return -1;
   }
   else
   {
      // Main loop flag
      bool quit = false;

      // Event handler
      SDL_Event e;

      // Enable text input
      SDL_StartTextInput();

      // While application is running
      while(!quit)
      {
         // Handle events on queue
         while(SDL_PollEvent(&e) != 0)
         {
            // User requests quit
            if(e.type == SDL_QUIT)
               quit = true;
            // Handle keypress with current mouse position
            else if(e.type == SDL_TEXTINPUT)
            {
               int x = 0, y = 0;
               SDL_GetMouseState(&x, &y);
               handleKeys(e.text.text[ 0 ], x, y);
            }
         }

         // Render quad
         render();

         // Update screen
         SDL_GL_SwapWindow(gWindow);
      }

      // Disable text input
      SDL_StopTextInput();
   }

   // Free resources and close SDL
   close();

   return 0;
}
