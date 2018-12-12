#pragma once

#include "SDL_opengl.h"

const GLchar *shader_source_fragment = R"(
#version 330

//uniform sampler2D tex;
//uniform vec2 tex_size;

//layout(location = 0) out vec4 out_color;

void main()
{
    //vec4 in_color = texture(tex, gl_FragCoord.xy / tex_size);
    //out_color = vec4(1.0) - in_color;
    gl_FragColor = vec4(1.0) - gl_FragColor;
}

)";