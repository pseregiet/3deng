@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@vs vs_particle
in vec2 apos;
in vec2 auv;

in vec4 amodel1;
in vec4 amodel2;
in vec4 amodel3;
in vec4 amodel4;

out INTERFACE {
    vec2 uv;
    float trans;
} inter;

uniform vs_particle_slow {
    mat4 uvp;
};

void main() {
    mat4 model = mat4(amodel1, amodel2, amodel3, amodel4);

    inter.uv = auv;
    inter.trans = model[3][3];
    model[3][3] = 1.0;
    gl_Position = uvp * model * vec4(apos, 0.0, 1.0);
}
@end

@fs fs_particle
in INTERFACE {
    vec2 uv;
    float trans;
} inter;

out vec4 frag;

/*
uniform fs_particle_slow {
    
};
*/

uniform sampler2D imgdiff;
void main() {
    vec4 p = texture(imgdiff, inter.uv);
    p.a *= inter.trans;
    frag = p;
}
@end

@program shdparticle vs_particle fs_particle
