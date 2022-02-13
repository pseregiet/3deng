@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@vs vs_obj
#define NR_POINT_LIGHTS 4
in vec3 apos;
in vec3 anorm;
in vec2 auv;
in vec3 atang;

out INTERFACE {
    vec3 tang_fragpos;
    vec3 tang_viewpos;
    vec3 tang_dirlight_dir;
    vec3 tang_sptlight_dir;
    vec3 tang_sptlight_pos;
    vec4 tang_pntlight_pos[4];
    vec2 uv;
} inter;

uniform vs_obj_slow {
    mat4 uvp;
    vec4 upntlight_pos[NR_POINT_LIGHTS];
    vec3 uviewpos;
    vec3 udirlight_dir;
    vec3 usptlight_dir;
    vec3 usptlight_pos;
};

uniform vs_obj_fast {
    mat4 umodel;
};

void main() {

    mat3 normalmat = transpose(inverse(mat3(umodel)));
    
    vec3 T = normalize(normalmat * atang);
    vec3 N = normalize(normalmat * anorm);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    vec3 ssfragpos = vec3(umodel * vec4(apos, 1.0));
    mat3 TBN = transpose(mat3(T,B,N));

    inter.tang_fragpos       = TBN * ssfragpos;
    inter.tang_viewpos       = TBN * uviewpos;
    inter.tang_dirlight_dir  = TBN * udirlight_dir;
    inter.tang_sptlight_dir  = TBN * usptlight_dir;
    inter.tang_sptlight_pos  = TBN * usptlight_pos;
    for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
        inter.tang_pntlight_pos[i] = vec4(TBN * vec3(upntlight_pos[i]), 0.0);
    }

    inter.uv = auv;

    gl_Position = uvp * vec4(ssfragpos, 1.0);
}
@end

@fs fs_obj
#define NR_POINT_LIGHTS 4
in INTERFACE {
    vec3 tang_fragpos;
    vec3 tang_viewpos;
    vec3 tang_dirlight_dir;
    vec3 tang_sptlight_dir;
    vec3 tang_sptlight_pos;
    vec4 tang_pntlight_pos[4];
    vec2 uv;
} inter;

out vec4 frag;

uniform fs_obj_fast {
    float umatshine;
};

uniform fs_obj_dirlight {
    vec3 ambi;
    vec3 diff;
    vec3 spec;
} udir_light;


uniform fs_obj_pointlights {
    vec4 ambi[NR_POINT_LIGHTS];
    vec4 diff[NR_POINT_LIGHTS];
    vec4 spec[NR_POINT_LIGHTS];
    vec4 atte[NR_POINT_LIGHTS];
    float enabled;
} upoint_lights;

uniform fs_obj_spotlight {
    vec3 atte;
    float cutoff;
    vec3 ambi;
    float outcutoff;
    vec3 diff;
    vec3 spec;
} uspot_light;

struct dir_light_t {
    vec3 dir;
    vec3 ambi;
    vec3 diff;
    vec3 spec;
};

struct point_light_t {
    vec3 pos;
    float constant;
    float linear;
    float quadratic;  
    vec3 ambi;
    vec3 diff;
    vec3 spec;
};

struct spot_light_t {
    vec3 pos;
    vec3 dir;
    float cutoff;
    float outcutoff;
    float constant;
    float linear;
    float quadratic;
    vec3 ambi;
    vec3 diff;
    vec3 spec; 
};

struct lightdata_t {
    vec3 pixdiff;
    vec3 pixspec;
    vec3 normal;
    vec3 fragpos;
    vec3 viewdir;
    float matshine;
};

dir_light_t get_directional_light();
point_light_t get_point_light(const int index);
spot_light_t  get_spot_light();

vec3 calc_dir_light(const dir_light_t light, const lightdata_t lightdata);
vec3 calc_point_light(const point_light_t light, const lightdata_t lightdata);
vec3 calc_spot_light(const spot_light_t light, const lightdata_t lightdata);

uniform sampler2D imgdiff;
uniform sampler2D imgspec;
uniform sampler2D imgnorm;
void main() {
    vec3 pixdiff = vec3(texture(imgdiff, inter.uv).xyz);
    vec3 pixspec = vec3(texture(imgspec, inter.uv).xyz);
    vec3 pixnorm = vec3(texture(imgnorm, inter.uv).xyz);

    //properties
    vec3 norm = normalize(pixnorm * 2.0 - 1.0);
    vec3 viewdir = normalize(inter.tang_viewpos - inter.tang_fragpos);

    lightdata_t lightdata = lightdata_t(
            pixdiff,
            pixspec,
            norm,
            inter.tang_fragpos,
            viewdir,
            umatshine
    );

    //phase 1: directional lighting
    vec3 result = calc_dir_light(get_directional_light(), lightdata);
    
    //phase 2: point lights
    int enb = int(upoint_lights.enabled);
    for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
        if ((enb & (1 << i)) != 0)
            result += calc_point_light(get_point_light(i), lightdata);
    }

    //phase 3: spot light
    result += calc_spot_light(get_spot_light(), lightdata);

    frag = vec4(result, 1.0);
}

dir_light_t get_directional_light() {
    return dir_light_t(
        inter.tang_dirlight_dir,
        udir_light.ambi,
        udir_light.diff,
        udir_light.spec
    );
}

point_light_t get_point_light(const int i) {
    return point_light_t(
        inter.tang_pntlight_pos[i].xyz,
        upoint_lights.atte[i].x,
        upoint_lights.atte[i].y,
        upoint_lights.atte[i].z,
        upoint_lights.ambi[i].xyz,
        upoint_lights.diff[i].xyz,
        upoint_lights.spec[i].xyz
    );
}

spot_light_t get_spot_light() {
    return spot_light_t(
        inter.tang_sptlight_pos,
        inter.tang_sptlight_dir,
        uspot_light.cutoff,
        uspot_light.outcutoff,
        uspot_light.atte.x,
        uspot_light.atte.y,
        uspot_light.atte.z,
        uspot_light.ambi,
        uspot_light.diff,
        uspot_light.spec
    );
}

vec3 calc_dir_light(const dir_light_t light, const lightdata_t lightdata)
{
    vec3 lightdir = normalize(-light.dir);
    //diffuse shading
    float diff = max(dot(lightdata.normal, lightdir), 0.0);
    //specular
    vec3 reflectdir = reflect(-lightdir, lightdata.normal);
    float spec = pow(max(dot(lightdata.viewdir, reflectdir), 0.0), lightdata.matshine);
    //combine
    vec3 ambient = light.ambi * lightdata.pixdiff;
    vec3 diffuse = light.diff * diff * lightdata.pixdiff;
    vec3 specular = light.spec * spec * lightdata.pixspec;
    
    return (ambient + diffuse + specular);
}

vec3 calc_point_light(const point_light_t light, const lightdata_t lightdata)
{
    vec3 lightdir = normalize(light.pos - lightdata.fragpos);
    //diffuse
    float diff = max(dot(lightdata.normal, lightdir), 0.0);
    //specular
    vec3 reflectdir = reflect(-lightdir, lightdata.normal);
    float spec = pow(max(dot(lightdata.viewdir, reflectdir), 0.0), lightdata.matshine);
    //attenuation
    float distance = length(light.pos - lightdata.fragpos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));

    //combine
    vec3 ambient = light.ambi * lightdata.pixdiff;
    vec3 diffuse = light.diff * diff * lightdata.pixdiff;
    vec3 specular = light.spec * spec * lightdata.pixspec;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 calc_spot_light(const spot_light_t light, const lightdata_t lightdata)
{
    vec3 lightdir = normalize(light.pos - lightdata.fragpos);
    //diffuse
    float diff = max(dot(lightdata.normal, lightdir), 0.0);
    //specular
    vec3 reflectdir = reflect(-lightdir, lightdata.normal);
    float spec = pow(max(dot(lightdata.viewdir, reflectdir), 0.0), lightdata.matshine);
    //attenuation
    float distance = length(light.pos - lightdata.fragpos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));

    // spotlight intensity
    float theta = dot(lightdir, normalize(-light.dir));
    float epsilon = light.cutoff - light.outcutoff;
    float intensity = clamp((theta - light.outcutoff) / epsilon, 0.0, 1.0);

    //combine
    vec3 ambient = light.ambi * lightdata.pixdiff;
    vec3 diffuse = light.diff * diff * lightdata.pixdiff;
    vec3 specular = light.spec * spec * lightdata.pixspec;

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    return (ambient + diffuse + specular);
}

@end

@program shdobj vs_obj fs_obj


@vs light_cube_vs
in vec3 pos;

uniform vs_paramsl {
    mat4 mvp;
};

void main() {
    gl_Position = mvp * vec4(pos, 1.0);
}
@end

@fs light_cube_fs
out vec4 frag;

void main() {
    frag = vec4(1.0, 1.0, 1.0, 1.0);
}
@end

@program light_cube light_cube_vs light_cube_fs
