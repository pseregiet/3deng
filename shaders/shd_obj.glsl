@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@include obj_shd_include_lighting.glsl
#define NR_POINT_LIGHTS 4

@vs vs_obj
in vec3 apos;
in vec3 anorm;
in vec4 atang;
in vec2 auv;

out INTERFACE {
    vec4 tang;
    vec2 uv;
    vec3 normal;
    vec3 fragpos;
} inter;

uniform vs_obj_slow {
    mat4 vp;
} uvs_slow;

uniform vs_obj_fast {
    mat4 model;
} uvs_fast;

void main() {
    inter.uv = auv;
    inter.normal = anorm;
    inter.tang = atang;
    inter.fragpos = vec3(uvs_fast.model * vec4(apos, 1.0));

    gl_Position = uvs_slow.vp * vec4(inter.fragpos, 1.0);
}
@end

@fs fs_obj
#define NR_POINT_LIGHTS 4
in INTERFACE {
    vec4 tang;
    vec2 uv;
    vec3 normal;
    vec3 fragpos;
} inter;

out vec4 frag;

uniform fs_obj_fast {
    float umatshine;
};

uniform fs_obj_pointlights {
    @include_block lighting_fs_pointlight_uniforms
} upoint_lights;

uniform fs_obj_spotlight {
    @include_block lighting_fs_spotlight_uniforms
} uspot_light;

uniform fs_obj_slow {
    @include_block lighting_fs_dirlight_uniforms
    vec3 viewpos;
} ufs_slow;

@include_block lighting_structures
@include_block lighting_functions

uniform sampler2D imgdiff;
uniform sampler2D imgspec;
uniform sampler2D imgnorm;
void main() {
    vec3 pixdiff = vec3(texture(imgdiff, inter.uv).xyz);
    vec3 pixspec = vec3(texture(imgspec, inter.uv).xyz);
    vec3 pixnorm = vec3(texture(imgnorm, inter.uv).xyz);

    vec3 vN = inter.normal;
    vec3 vT = vec3(inter.tang);
    vec3 vNt = normalize(pixnorm * 2.0 - 1.0);
    vec3 vB = inter.tang.w * cross(vN, vT);
    vec3 vNout = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );

    vec3 viewdir = normalize(ufs_slow.viewpos - inter.fragpos);

    lightdata_t lightdata = lightdata_t(
            pixdiff,
            pixspec,
            vNout,
            inter.fragpos,
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

