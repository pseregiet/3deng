@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

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
uniform vs_md5 {
    mat4 uvp;
    mat4 umodel;
    ivec4 uboneuv;
    ivec4 uweightuv;
    vec3 ulightpos;
    vec3 uviewpos;
    float udummy;
};

uniform sampler2D bonemap;
uniform sampler2D weightmap;

in vec3 apos;
in vec3 anorm;
in vec3 atang;
in vec2 auv;
in vec2 aweight;

out INTERFACE {
    vec3 normal;
    vec3 tang_viewpos;
    vec3 tang_lightpos;
    vec3 tang_fragpos;
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

    inter.tang_lightpos = TBN * ulightpos;
    inter.tang_viewpos = TBN * uviewpos;
    inter.tang_fragpos = TBN * ssfragpos;

    gl_Position = uvp * vec4(ssfragpos, 1.0);
    
    inter.uv = auv;
    inter.normal = anorm;
}
@end

@fs fs_md5
in INTERFACE {
    vec3 normal;
    vec3 tang_viewpos;
    vec3 tang_lightpos;
    vec3 tang_fragpos;
    vec2 uv;
} inter;

uniform fs_md5 {
    vec3 uambi;
    vec3 udiff;
    vec3 uspec;
};

uniform sampler2D imgdiff;
uniform sampler2D imgspec;
uniform sampler2D imgnorm;

out vec4 frag;

void main() {
    vec3 pixdiff = texture(imgdiff, inter.uv).rgb;
    vec3 pixspec = texture(imgspec, inter.uv).rgb;
    vec3 pixnorm = texture(imgnorm, inter.uv).rgb;

    pixnorm = normalize(pixnorm * 2.0 - 1.0);
    vec3 ambient = uambi * pixdiff;
    vec3 lightdir = normalize(-inter.tang_lightpos);
    float diff = max(dot(pixnorm, lightdir), 0.0);
    vec3 diffuse = diff * udiff;

    vec3 viewdir = normalize(inter.tang_viewpos - inter.tang_fragpos);
    vec3 reflectdir = reflect(-lightdir, pixnorm);
    float spec = pow(max(dot(pixnorm, reflectdir), 0.0), 32.0);

    vec3 specular = pixspec * spec * uspec;

    vec3 light = (ambient + (diffuse + specular)) * pixdiff;
    frag = vec4(light, 1.0);
}
@end

@program shdmd5 vs_md5 fs_md5

