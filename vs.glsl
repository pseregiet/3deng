#version 330
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

out vec3 fragpos;
out vec3 normalo;
out vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
   gl_Position = projection * view * model * vec4(position, 1.0);
   fragpos = vec3(model * vec4(position, 1.0));
   normalo = mat3(model) * normal;
   uv = texcoord;
}
