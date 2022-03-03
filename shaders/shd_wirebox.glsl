@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@vs wirebox_vs
in vec4 apos;

uniform vs_wirebox_slow {
    mat4 uvp;
};

uniform vs_wirebox_fast {
    mat4 umodel;
};

void main() {
    gl_Position = uvp * umodel * vec4(apos);
}
@end

@fs wirebox_fs
out vec4 frag;

void main() {
    frag = vec4(1.0, 1.0, 1.0, 1.0);
}
@end

@program shdwirebox wirebox_vs wirebox_fs
