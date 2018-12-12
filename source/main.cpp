// TODO: Actually put proper header here

#include <cstdio>

#include <GL/glew.h>
#include <GL/glu.h>

#include "SDL.h"
#include "SDL_opengl.h"

#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>

#include "shader_fragment.h"
#include "shader_vertex.h"

// Screen dimension constants
constexpr int SCREEN_WIDTH  = 640;
constexpr int SCREEN_HEIGHT = 480;

// The window we'll be rendering to
static SDL_Window *window = nullptr;

// OpenGL context
static SDL_GLContext context;

// Render flag
static bool renderQuad = true;

// Graphics program
static GLuint programID           =  0;
static GLint  vertexPos2DLocation = -1;
static GLuint VBO                 =  0;
static GLuint IBO                 =  0;
static GLuint FBO                 =  0;

static ILint    linearheight;
static ILint    linearwidth;
static uint8_t *linearimage;

static GLuint imageTexID = 0;

static GLuint IL_loadImage(const char *filename)
{
   ILuint imageID;    // Create an image ID as a ULuint
   GLuint textureID;  // Create a texture ID as a GLuint
   ILboolean success; // Create a flag to keep track of success/failure
   ILenum error;      // Create a flag to keep track of the IL error state

   ilGenImages(1, &imageID);           // Generate the image ID
   ilBindImage(imageID);               // Bind the image
   success = ilLoadImage(filename); // Load the image file

   // If we managed to load the image, then we can start to do things with it...
   if(success)
   {
      // If the image is flipped (i.e. upside-down and mirrored, flip it the right way up!)
      ILinfo ImageInfo;
      iluGetImageInfo(&ImageInfo);
      if(ImageInfo.Origin == IL_ORIGIN_UPPER_LEFT)
         iluFlipImage();

      // Convert the image into a suitable format to work with
      // NOTE: If your image contains alpha channel you can replace IL_RGB with IL_RGBA
      success = ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);

      // Quit out if we failed the conversion
      if(!success)
      {
         error = ilGetError();
         printf("Image conversion failed - IL reports error: %d - %s\n", error, iluErrorString(error));
         exit(-1);
      }

      // Generate a new texture
      glGenTextures(1, &textureID);

      // Bind the texture to a name
      glBindTexture(GL_TEXTURE_2D, textureID);

      // Set texture clamping method
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

      // Set texture interpolation method to use linear interpolation (no MIPMAPS)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

      // Specify the texture specification
      glTexImage2D(GL_TEXTURE_2D,                  // Type of texture
                   0,                              // Pyramid level (for mip-mapping) - 0 is the top level
                   ilGetInteger(IL_IMAGE_FORMAT),  // Internal pixel format to use. Can be a generic type like GL_RGB or GL_RGBA, or a sized type
                   ilGetInteger(IL_IMAGE_WIDTH),   // Image width
                   ilGetInteger(IL_IMAGE_HEIGHT),  // Image height
                   0,                              // Border width in pixels (can either be 1 or 0)
                   ilGetInteger(IL_IMAGE_FORMAT),  // Format of image pixel data
                   GL_UNSIGNED_BYTE,               // Image data type
                   ilGetData());                   // The actual image data itself

      // Requires OpenGL 4.2 or later, which is a bit much
      //glTexStorage2D(GL_TEXTURE_2D, 1, ilGetInteger(IL_IMAGE_FORMAT),
      //               ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
   }
   else // If we failed to open the image file in the first place...
   {
      error = ilGetError();
      printf("Image load failed - IL reports error: %d - %s\n", error, iluErrorString(error));
      exit(-1);
   }

   ilDeleteImages(1, &imageID); // Because we have already copied image data into texture data we can release memory used by image.

   puts("Texture creation successful.");

   return textureID; // Return the GLuint to the texture so you can use it!
}

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
   programID = glCreateProgram();

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
      glAttachShader(programID, vertexShader);

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
         glAttachShader(programID, fragmentShader);


         // Link program
         glLinkProgram(programID);

         // Check for errors
         GLint programSuccess = GL_TRUE;
         glGetProgramiv(programID, GL_LINK_STATUS, &programSuccess);
         if(programSuccess != GL_TRUE)
         {
            printf("Error linking program %d!\n", programID);
            printProgramLog(programID);
            success = false;
         }
         else
         {
            // Get vertex attribute location
            vertexPos2DLocation = glGetAttribLocation(programID, "LVertexPos2D");
            if(vertexPos2DLocation == -1)
            {
               printf("LVertexPos2D is not a valid GLSL program variable!\n");
               success = false;
            }
            else
            {
               // Initialize clear color
               glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

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
               glGenBuffers(1, &VBO);
               glBindBuffer(GL_ARRAY_BUFFER, VBO);
               glBufferData(GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);

               // Create IBO
               glGenBuffers(1, &IBO);
               glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
               glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indexData, GL_STATIC_DRAW);

               glGenFramebuffers(1, &FBO);
               glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
               glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, imageTexID, 0);
               glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            }
         }
      }
   }

   return success;
}

static void IL_init()
{
   ilutRenderer(ILUT_OPENGL);
   ilInit();
   iluInit();
   ilutInit();
   ilutRenderer(ILUT_OPENGL);
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
      window = SDL_CreateWindow("Max Waine Graphics Optics Coursework Final Year Project Word Salad",
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                SCREEN_WIDTH, SCREEN_HEIGHT,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
      if(window == nullptr)
      {
         printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
         success = false;
      }
      else
      {
         // Create context
         context = SDL_GL_CreateContext(window);
         if(context == nullptr)
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
      renderQuad = !renderQuad;
}

//
// Per frame update
//
static void update()
{
   // Copy texture to default framebuffer
   glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
   glBlitFramebuffer(0, 0, 480, 640, 0, 0, 480, 640, GL_COLOR_BUFFER_BIT, GL_LINEAR);
   glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

//
// Renders quad to the screen
//
static void render()
{
   // Clear color buffer
   //glClear(GL_COLOR_BUFFER_BIT);

   // Render quad
   if(renderQuad)
   {
      // Bind program
      glUseProgram(programID);

      // Enable vertex position
      glEnableVertexAttribArray(vertexPos2DLocation);

      // Set vertex data
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glVertexAttribPointer(vertexPos2DLocation, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

      // Set index data and render
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
      glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, nullptr);

      // Disable vertex position
      glDisableVertexAttribArray(vertexPos2DLocation);

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
   glDeleteProgram(programID);

   // Destroy window
   SDL_DestroyWindow(window);
   window = nullptr;

   // Quit SDL subsystems
   SDL_Quit();
}

// FIXME: This
#if defined(_MSC_VER) && !defined(_WIN32_WCE)
#undef main
#endif

int main(int argc, char *argv[])
{
   // Load image into linear
   if(argc < 0)
   {
      puts("There must be an argument and it must be the relative path to the loaded image!");
      return -1;
   }

   // Start up SDL and create window
   if(!init())
   {
      puts("Failed to initialize!");
      return -1;
   }
   else
   {
      IL_init();
      imageTexID = IL_loadImage(argv[1]);

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

         update();

         // Render quad
         render();

         // Update screen
         SDL_GL_SwapWindow(window);
      }

      // Disable text input
      SDL_StopTextInput();
   }

   // Free resources and close SDL
   close();

   return 0;
}
