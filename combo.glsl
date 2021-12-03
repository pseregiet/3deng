@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
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
    //mat4 view;
    //mat4 projection;
    mat4 vp;
};

void main() {
   gl_Position = vp * model * vec4(position, 1.0); //projection * view * model * vec4(position, 1.0);
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
    float matshine;
};

uniform fs_dir_light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} dir_light;

#define NR_POINT_LIGHTS 4

uniform fs_point_lights {
    vec4 position[NR_POINT_LIGHTS];
    vec4 ambient[NR_POINT_LIGHTS];
    vec4 diffuse[NR_POINT_LIGHTS];
    vec4 specular[NR_POINT_LIGHTS];
    vec4 attenuation[NR_POINT_LIGHTS];
    float enabled;
} point_lights;

uniform fs_spot_light {
    vec3 position;
    vec3 direction;
    float cutoff;
    float outcutoff;
    vec3 attenuation;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} spot_light;

struct dir_light_t {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct point_light_t {
    vec3 position;
    float constant;
    float linear;
    float quadratic;  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct spot_light_t {
    vec3 position;
    vec3 direction;
    float cutoff;
    float outcutoff;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular; 
};

dir_light_t get_directional_light();
point_light_t get_point_light(int index);
spot_light_t  get_spot_light();

vec3 calc_dir_light(dir_light_t light, vec3 normal, vec3 viewdir);
vec3 calc_point_light(point_light_t light, vec3 normal, vec3 fragpos, vec3 viewdir);
vec3 calc_spot_light(spot_light_t light, vec3 normal, vec3 fragpos, vec3 viewdir);

uniform sampler2D imgdiff;
uniform sampler2D imgspec;
uniform sampler2D imgbump;
void main() {
    //properties
    vec3 norm = normalize(normalo);
    vec3 viewdir = normalize(viewpos - fragpos);

    //phase 1: directional lighting
    vec3 result = calc_dir_light(get_directional_light(), norm, viewdir);
    
    //phase 2: point lights
    int enb = int(point_lights.enabled);
    for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
        if ((enb & (1 << i)) != 0)
            result += calc_point_light(get_point_light(i), norm, fragpos, viewdir);
    }

    /*
    for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
        result += calc_point_light(get_point_light(i), norm, fragpos, viewdir);
    }
    */

    //phase 3: spot light
    result += calc_spot_light(get_spot_light(), norm, fragpos, viewdir);

    frag_color = vec4(result, 1.0);
}

dir_light_t get_directional_light() {
    return dir_light_t(
        dir_light.direction,
        dir_light.ambient,
        dir_light.diffuse,
        dir_light.specular
    );
}

point_light_t get_point_light(int i) {
    return point_light_t(
        point_lights.position[i].xyz,
        point_lights.attenuation[i].x,
        point_lights.attenuation[i].y,
        point_lights.attenuation[i].z,
        point_lights.ambient[i].xyz,
        point_lights.diffuse[i].xyz,
        point_lights.specular[i].xyz
    );
}

spot_light_t get_spot_light() {
    return spot_light_t(
        spot_light.position,
        spot_light.direction,
        spot_light.cutoff,
        spot_light.outcutoff,
        spot_light.attenuation.x,
        spot_light.attenuation.y,
        spot_light.attenuation.z,
        spot_light.ambient,
        spot_light.diffuse,
        spot_light.specular
    );
}

vec3 calc_dir_light(dir_light_t light, vec3 normal, vec3 viewdir) {
    vec3 lightdir = normalize(-light.direction);
    //diffuse shading
    float diff = max(dot(normal, lightdir), 0.0);
    //specular
    vec3 reflectdir = reflect(-lightdir, normal);
    float spec = pow(max(dot(viewdir, reflectdir), 0.0), matshine);
    //combine
    vec3 texel = vec3(texture(imgdiff, uv));
    vec3 spexel = vec3(texture(imgspec, uv));

    vec3 ambient = light.ambient * texel;
    vec3 diffuse = light.diffuse * diff * texel;
    vec3 specular = light.specular * spec * spexel;
    
    return (ambient + diffuse + specular);
}

vec3 calc_point_light(point_light_t light, vec3 normal, vec3 fragpos, vec3 viewdir) {
    vec3 lightdir = normalize(light.position - fragpos);
    //diffuse
    float diff = max(dot(normal, lightdir), 0.0);
    //specular
    vec3 reflectdir = reflect(-lightdir, normal);
    float spec = pow(max(dot(viewdir, reflectdir), 0.0), matshine);
    //attenuation
    float distance = length(light.position - fragpos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));

    //combine
    vec3 texel = vec3(texture(imgdiff, uv));
    vec3 spexel = vec3(texture(imgspec, uv));

    vec3 ambient = light.ambient * texel;
    vec3 diffuse = light.diffuse * diff * texel;
    vec3 specular = light.specular * spec * spexel;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 calc_spot_light(spot_light_t light, vec3 normal, vec3 fragpos, vec3 viewdir) {
    vec3 lightdir = normalize(light.position - fragpos);
    //diffuse
    float diff = max(dot(normal, lightdir), 0.0);
    //specular
    vec3 reflectdir = reflect(-lightdir, normal);
    float spec = pow(max(dot(viewdir, reflectdir), 0.0), matshine);
    //attenuation
    float distance = length(light.position - fragpos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));

    // spotlight intensity
    float theta = dot(lightdir, normalize(-light.direction));
    float epsilon = light.cutoff - light.outcutoff;
    float intensity = clamp((theta - light.outcutoff) / epsilon, 0.0, 1.0);

    //combine
    vec3 texel = vec3(texture(imgdiff, uv));
    vec3 spexel = vec3(texture(imgspec, uv));
    vec3 ambient = light.ambient * texel;
    vec3 diffuse = light.diffuse * diff * texel;
    vec3 specular = light.specular * spec * spexel;

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    return (ambient + diffuse + specular);
}

@end

@program comboshader vs fs


@vs light_cube_vs
in vec3 pos;

uniform vs_paramsl {
    //mat4 model;
    //mat4 view;
    //mat4 projection;
    mat4 mvp;
};

void main() {
    gl_Position = mvp * vec4(pos, 1.0);//projection * view * model * vec4(pos, 1.0);
}
@end

@fs light_cube_fs
out vec4 frag;

void main() {
    frag = vec4(1.0, 1.0, 1.0, 1.0);
}
@end

@program light_cube light_cube_vs light_cube_fs

@vs terrain_vs
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

out vec3 fragpos;
out vec3 normalo;
out vec2 uv;

uniform vs_params {
    mat4 model;
    //mat4 view;
    //mat4 projection;
    mat4 vp;
};

void main() {
   gl_Position = vp * model * vec4(position, 1.0); //projection * view * model * vec4(position, 1.0);
   fragpos = vec3(model * vec4(position, 1.0));
   normalo = mat3(model) * normal;
   uv = texcoord;
}
@end

@fs terrain_fs
in vec3 fragpos;
in vec3 normalo;
in vec2 uv;

out vec4 frag_color;

uniform sampler2D imgblend;
uniform sampler2DArray imggfx;

void main() {
    vec4 bl = texture(imgblend, uv);
    float f = bl.r * 256;
    int i = int(f);

    vec3 vv = vec3(uv.x*10, uv.y*10, float(i));
    vec4 px = texture(imggfx, vv);
    frag_color = px;
}
@end

@program terrainshd terrain_vs terrain_fs
