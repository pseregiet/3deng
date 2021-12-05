@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@vs vs_terrain
in vec3 apos;
in vec3 anorm;
in vec2 auv;
in vec3 atang;

out INTERFACE {
    vec2 uv;
    vec3 tang_lightpos;
    vec3 tang_viewpos;
    vec3 tang_fragpos;
} inter;

uniform vs_params {
    mat4 vp;
    mat4 model;
    vec3 lightpos;
    vec3 viewpos;
};

/*
mat3 transpose(mat3 mat) {
    vec3 i0 = mat[0];
    vec3 i1 = mat[1];
    vec3 i2 = mat[2];

    return mat3(
        vec3(i0.x, i1.x, i2.x),
        vec3(i0.y, i1.y, i2.y),
        vec3(i0.z, i1.z, i2.z)
    };
}
*/

void main() {
    inter.uv = auv;
    mat3 normalmat = mat3(model);
    vec3 T = normalize(normalmat * atang);
    vec3 N = normalize(normalmat * anorm);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));
    inter.tang_lightpos = TBN * lightpos;
    inter.tang_viewpos = TBN * viewpos;
    inter.tang_fragpos = TBN * vec3(model * vec4(apos, 1.0));

    gl_Position = vp * model * vec4(apos, 1.0);
}
@end

@fs fs_terrain

in INTERFACE {
    vec2 uv;
    vec3 tang_lightpos;
    vec3 tang_viewpos;
    vec3 tang_fragpos;
} inter;

uniform fs_params { 
    vec3 uambi;
    vec3 udiff;
    vec3 uspec;
};

out vec4 fragcolor;

uniform sampler2D imgblend;
uniform sampler2DArray imgdiff;
uniform sampler2DArray imgspec;
uniform sampler2DArray imgnorm;

void main() {
    vec4 bl = texture(imgblend, inter.uv);
    float idx = bl.r * 256;

    vec3 vv = vec3(inter.uv.x*10, inter.uv.y*10, idx);

    vec3 pixdiff = texture(imgdiff, vv).rgb;
    vec3 pixspec = texture(imgspec, vv).rgb;
    vec3 pixnorm = texture(imgnorm, vv).rgb;

    pixnorm = normalize(pixnorm * 2.0 - 1.0);

    vec3 ambient = vec3(0,0,0);//uambi * pixdiff;

    vec3 lightdir = normalize(-inter.tang_lightpos);// - inter.tang_fragpos);
    float diff = max(dot(pixnorm, lightdir), 0.0);
    vec3 diffuse = diff * pixdiff * udiff;

    vec3 viewdir = normalize(inter.tang_viewpos - inter.tang_fragpos);
    vec3 reflectdir = reflect(-lightdir, pixnorm);
    //vec3 halfwaydir = normalize(lightdir + viewdir);
    float spec = pow(max(dot(pixnorm, reflectdir), 0.0), 32.0); //halfwaydir), 0.0), 32.0);

    vec3 specular = pixspec * spec * uspec;

    fragcolor = vec4(ambient + diffuse + specular, 1.0);
}
@end

@program terrainshd vs_terrain fs_terrain
