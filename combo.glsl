@ctype vec3 hmm_vec3
@ctype mat4 hmm_mat4

@vs vs
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

out vec3 fragpos;
out vec3 normalo;
out vec2 uv;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
   gl_Position = projection * view * model * vec4(position, 1.0);
   fragpos = vec3(model * vec4(position, 1.0));
   normalo = mat3(model) * normal;
   uv = texcoord;
}
@end

@fs fs
in vec3 fragpos;
in vec3 normalo;
in vec2 uv;

out vec4 frag_color;

uniform fs_params {
    vec3 viewpos;
};

uniform fs_material {
    vec3 specular;
    float shine;
} material;

uniform fs_light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} light;

uniform sampler2D diffuse_tex;

void main() {

    vec3 ambient = light.ambient * vec3(texture(diffuse_tex, uv));

    vec3 norm = normalize(normalo);
    vec3 lightdir = normalize(light.position - fragpos);
    float diff = max(dot(norm, lightdir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuse_tex, uv));

    vec3 viewdir = normalize(viewpos - fragpos);
    vec3 reflectdir = reflect(-lightdir, norm);
    float spec = pow(max(dot(viewdir, reflectdir), 0.0), material.shine);
    vec3 specular = light.specular * (spec * material.specular);

    vec3 result = ambient + diffuse + specular;
    frag_color = vec4(result, 1.0);
}
@end

@program comboshader vs fs

