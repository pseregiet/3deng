@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@vs vs_md5
uniform vs_md5 {
    mat4 mvp;
    ivec4 boneuv;
    ivec4 weightuv;
};

uniform sampler2D bonemap;
uniform sampler2D weightmap;

in vec3 apos;
in vec3 anorm;
in vec2 auv;
in vec2 aweight;

out INTERFACE {
    vec3 normal;
    vec2 uv;
} inter;

void get_mat4_from_texture(in sampler2D map, in ivec4 pos, in int index, out mat4 mat) {
    pos.x = 0;
    pos.y = 0;
    vec4 m1 = texelFetch(map, ivec2(pos.x + index + 0, pos.y), 0);
    vec4 m2 = texelFetch(map, ivec2(pos.x + index + 1, pos.y), 0);
    vec4 m3 = texelFetch(map, ivec2(pos.x + index + 2, pos.y), 0);
    vec4 m4 = texelFetch(map, ivec2(pos.x + index + 3, pos.y), 0);
    //vec4 m4 = vec4(0.0, 0.0, 0.0, 1.0);

    mat = mat4(m1, m2, m3, m4);
}

void get_weight_from_texture(in sampler2D map, in int index, out float bias, out float joint) {
    ivec2 pos = ivec2(weightuv.x + index + weightuv.w, weightuv.y);
    if (weightuv.x >= weightuv.z) {
        pos.y++;
        pos.x % weightuv.z;
    }

    vec4 pixel = texelFetch(map, pos, 0);
    bias = pixel.x;
    joint = pixel.y;
}

void main() {
    mat4 bonetransform = mat4(0.0);
    int wstart = int(aweight.x);
    int wcount = int(aweight.y);

    for (int i = 0; i < wcount; ++i) {
        float bias;
        float joint;
        mat4 bonemat;
        get_weight_from_texture(weightmap, wstart+i, bias, joint);
        get_mat4_from_texture(bonemap, boneuv, int(joint)*4, bonemat);

        bonetransform += (bonemat * bias);
    }

    vec4 objpos = bonetransform * vec4(apos, 1.0);
    vec4 objnor = bonetransform * vec4(anorm, 1.0);
    inter.uv = auv;

    inter.normal = normalize((mvp * objnor).xyz);
    inter.normal = (inter.normal + 1) * 0.5;
    gl_Position = mvp * objpos;
}
@end

@fs fs_md5
in INTERFACE {
    vec3 normal;
    vec2 uv;
} inter;

out vec4 frag;

void main() {
    frag = vec4(inter.normal, 1.0);
}
@end

@program shdmd5 vs_md5 fs_md5

