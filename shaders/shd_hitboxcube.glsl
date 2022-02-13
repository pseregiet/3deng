@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@vs hitboxcube_vs
in vec3 pos;

uniform vs_params {
    mat4 mvp;
};

void main() {
    gl_Position = mvp * vec4(pos, 1.0);
}
@end

@fs hitboxcube_fs
out vec4 frag;

void main() {
    frag = vec4(1.0, 1.0, 1.0, 1.0);
}
@end

@program hitboxcube hitboxcube_vs hitboxcube_fs
