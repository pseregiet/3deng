@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@include shd_include_lighting.glsl
#define NR_POINT_LIGHTS 4

@block bone_functions
void get_mat4_from_texture(in sampler2D map, in ivec4 boneuv, in int index, out mat4 mat) {
    ivec2 pos = ivec2(boneuv.x + index, boneuv.y);
    if (pos.x >= boneuv.z) {
        pos.y++;
        pos.x = pos.x % boneuv.z;
    }
    vec4 m1 = texelFetch(map, pos, 0); pos.x++;
    vec4 m2 = texelFetch(map, pos, 0); pos.x++;
    vec4 m3 = texelFetch(map, pos, 0); pos.x++;
    vec4 m4 = texelFetch(map, pos, 0);
    mat = mat4(m1, m2, m3, m4);
}

void get_weight_from_texture(in sampler2D map, in ivec4 weightuv, in int index, out float bias, out float joint) {
    ivec2 pos = ivec2(weightuv.x + index, weightuv.y);
    if (weightuv.x >= weightuv.z) {
        pos.y++;
        pos.x = pos.x % weightuv.z;
    }
    vec4 pixel = texelFetch(map, pos, 0);
    bias = pixel.x;
    joint = pixel.y;
}
@end

@vs vs_md5_depth
@include_block bone_functions

uniform vs_md5_depth {
    mat4 umodel;
    ivec4 uboneuv;
    ivec4 uweightuv;
};
uniform sampler2D bonemap;
uniform sampler2D weightmap;
in vec3 apos;
in vec2 aweight;

void main() {
    mat4 bonetransform = mat4(0.0);
    int wstart = int(aweight.x);
    int wcount = int(aweight.y);

    for (int i = 0; i < wcount; ++i) {
        float bias;
        float joint;
        mat4 bonemat;
        get_weight_from_texture(weightmap, uweightuv, wstart+i, bias, joint);
        get_mat4_from_texture(bonemap, uboneuv, int(joint)*4, bonemat);

        bonetransform += (bonemat * bias);
    }

    gl_Position = umodel * bonetransform * vec4(apos, 1.0);
}
@end

@fs fs_md5_depth
void main() {}
@end

@program shdmd5_depth vs_md5_depth fs_md5_depth

@vs vs_md5
uniform vs_md5_fast {
    mat4 umodel;
    ivec4 uboneuv;
    ivec4 uweightuv;
};

uniform vs_md5_slow {
    mat4 uvp;
@include_block lighting_vs_uniforms
};

uniform sampler2D bonemap;
uniform sampler2D weightmap;

in vec3 apos;
in vec3 anorm;
in vec3 atang;
in vec2 auv;
in vec2 aweight;

out INTERFACE {
@include_block lighting_tang_interface
    vec2 uv;
} inter;

@include_block bone_functions

void main() {
    mat4 bonetransform = mat4(0.0);
    int wstart = int(aweight.x);
    int wcount = int(aweight.y);

    for (int i = 0; i < wcount; ++i) {
        float bias;
        float joint;
        mat4 bonemat;
        get_weight_from_texture(weightmap, uweightuv, wstart+i, bias, joint);
        get_mat4_from_texture(bonemap, uboneuv, int(joint)*4, bonemat);

        bonetransform += (bonemat * bias);
    }

    mat4 xmodel = umodel * bonetransform;

    mat3 normalmat = mat3(xmodel);

    vec3 T = normalize(normalmat * atang);
    vec3 N = normalize(normalmat * anorm);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    vec3 ssfragpos = vec3(xmodel * vec4(apos,1.0));
    mat3 TBN = transpose(mat3(T, B, N));

    inter.tang_fragpos = TBN * ssfragpos;
    inter.tang_viewpos = TBN * uviewpos;
    inter.tang_dirlight_dir  = TBN * udirlight_dir;
    inter.tang_sptlight_dir  = TBN * usptlight_dir;
    inter.tang_sptlight_pos  = TBN * usptlight_pos;
    for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
        inter.tang_pntlight_pos[i] = vec4(TBN * vec3(upntlight_pos[i]), 0.0);
    }

    gl_Position = uvp * vec4(ssfragpos, 1.0);
    
    inter.uv = auv;
}
@end

@fs fs_md5
#define NR_POINT_LIGHTS 4
in INTERFACE {
@include_block lighting_tang_interface
    vec2 uv;
} inter;

uniform fs_md5_fast {
    float umatshine;
};

uniform fs_md5_dirlight {
@include_block lighting_fs_dirlight_uniforms
} udir_light;


uniform fs_md5_pointlights {
@include_block lighting_fs_pointlight_uniforms
} upoint_lights;

uniform fs_md5_spotlight {
@include_block lighting_fs_spotlight_uniforms
} uspot_light;

@include_block lighting_structures
@include_block lighting_functions

uniform sampler2D imgdiff;
uniform sampler2D imgspec;
uniform sampler2D imgnorm;

out vec4 frag;

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

@program shdmd5 vs_md5 fs_md5

