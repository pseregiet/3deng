@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@include shd_include_lighting.glsl
#define NR_POINT_LIGHTS 4

@vs vs_obj
in vec3 apos;
in vec3 anorm;
in vec2 auv;
in vec3 atang;

out INTERFACE {
@include_block lighting_tang_interface
    vec2 uv;
} inter;

uniform vs_obj_slow {
    mat4 uvp;
@include_block lighting_vs_uniforms
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
@include_block lighting_tang_interface
    vec2 uv;
} inter;

out vec4 frag;

uniform fs_obj_fast {
    float umatshine;
};

uniform fs_obj_dirlight {
@include_block lighting_fs_dirlight_uniforms
} udir_light;


uniform fs_obj_pointlights {
@include_block lighting_fs_pointlight_uniforms
} upoint_lights;

uniform fs_obj_spotlight {
@include_block lighting_fs_spotlight_uniforms
} uspot_light;

@include_block lighting_structures
@include_block lighting_functions

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

@end

@program shdobj vs_obj fs_obj

@vs vs_obj_depth
uniform vs_obj_depth {
    mat4 umodel;
};
in vec3 apos;

void main() {
    gl_Position = umodel * vec4(apos, 1.0);
}
@end

@fs fs_obj_depth
void main() {}
@end

@program shdobj_depth vs_obj_depth fs_obj_depth

