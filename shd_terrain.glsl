@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@vs vs_depth
uniform vs_depthparams {
    mat4 model;
};
in vec3 apos;

void main() {
    gl_Position = model * vec4(apos, 1.0);
}
@end

@fs fs_depth
void main() {
}
@end

@program shddepth vs_depth fs_depth

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
    vec4 fragpos_lightspace;
    vec3 normal;
    //vec3 lightdir;
} inter;

uniform vs_params {
    mat4 vp;
    mat4 model;
    mat4 unormalmat;
    mat4 lightspace_matrix;
    vec3 lightpos;
    vec3 lightpos_s;
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
    inter.uv = auv/2;
    mat3 normalmat = mat3(model); //mat3(unormalmat);
    vec3 T = normalize(normalmat * atang);
    vec3 N = normalize(normalmat * anorm);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    vec3 ssfragpos = vec3(model * vec4(apos, 1.0));
    vec4 sslightfragpos = lightspace_matrix * vec4(ssfragpos, 1.0);

    inter.fragpos_lightspace = sslightfragpos;

    mat3 TBN = transpose(mat3(T, B, N));
    inter.tang_lightpos = TBN * lightpos;
    inter.tang_viewpos = TBN * viewpos;
    inter.tang_fragpos = TBN * ssfragpos;

    gl_Position = vp * vec4(ssfragpos, 1.0);
    inter.normal = anorm;
}
@end

@fs fs_terrain

in INTERFACE {
    vec2 uv;
    vec3 tang_lightpos;
    vec3 tang_viewpos;
    vec3 tang_fragpos;
    vec4 fragpos_lightspace;
    vec3 normal;
} inter;

uniform fs_params { 
    vec3 uambi;
    vec3 udiff;
    vec3 uspec;
    vec2 shadowmap_size;
    vec2 blendoffset;
};

out vec4 fragcolor;

uniform sampler2D shadowmap;
uniform sampler2DArray imgblend;
uniform sampler2DArray imgdiff;
uniform sampler2DArray imgspec;
uniform sampler2DArray imgnorm;

/*
float decodedepth(vec4 rgba) {
    return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
}
*/

float shadowcalc(vec4 fragpos_ls) {
    vec3 projcoords = fragpos_ls.xyz / fragpos_ls.w;
    projcoords = projcoords * 0.5 + 0.5;
    float currdepth = projcoords.z;
    vec2 mapuv = vec2(projcoords);
    
    //float bias = max(0.05 * (1.0 - dot(normal, lightdir)), 0.005);
    float bias = 0.00;

    if (currdepth >= 0.99)
        return 0.0;

    float shadow = 0.0;
    vec2 texelsize = 1.0 / shadowmap_size;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfdepth = texture(shadowmap, mapuv + vec2(x,y) * texelsize).r;
            shadow += currdepth - bias > pcfdepth ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;
    return shadow;
}

void main() {
    vec3 vv = vec3(inter.uv.x*30, inter.uv.y*30, 0.0);

    vec3 pixdiff = texture(imgdiff, vv).rgb;
    vec3 pixspec = texture(imgspec, vv).rgb;
    vec3 pixnorm = texture(imgnorm, vv).rgb;

    for (int i = 0; i < 4; ++i) {
        vv.z = float(i+1);
        vec3 buv = vec3(inter.uv + blendoffset, float(i));
        float bfactor = texture(imgblend, buv).a;

        if (bfactor < 0.1)
            continue;

        pixdiff = mix(pixdiff, texture(imgdiff, vv).rgb, bfactor);
        pixspec = mix(pixspec, texture(imgspec, vv).rgb, bfactor);
        pixnorm = mix(pixnorm, texture(imgnorm, vv).rgb, bfactor);

    }

    pixnorm = normalize(pixnorm * 2.0 - 1.0);
    vec3 ambient = uambi * pixdiff;

    vec3 lightdir = normalize(-inter.tang_lightpos);// - inter.tang_fragpos);
    float diff = max(dot(pixnorm, lightdir), 0.0);
    vec3 diffuse = diff * udiff;

    vec3 viewdir = normalize(inter.tang_viewpos - inter.tang_fragpos);
    vec3 reflectdir = reflect(-lightdir, pixnorm);
    //vec3 halfwaydir = normalize(lightdir + viewdir);
    float spec = pow(max(dot(pixnorm, reflectdir), 0.0), 32.0); //halfwaydir), 0.0), 32.0);

    vec3 specular = pixspec * spec * uspec;

    float shadow = shadowcalc(inter.fragpos_lightspace);//, inter.normal, inter.lightdir);
    vec3 light = (ambient + (1.0 - shadow) * (diffuse + specular)) *  pixdiff;
    fragcolor = vec4(light, 1.0);
}
@end

@program terrainshd vs_terrain fs_terrain
