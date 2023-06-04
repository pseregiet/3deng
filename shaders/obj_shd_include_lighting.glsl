
@block lighting_tang_interface
#define NR_POINT_LIGHTS 4
    vec3 tang_fragpos;
    vec3 tang_viewpos;
    vec3 tang_dirlight_dir;
    vec3 tang_sptlight_dir;
    vec3 tang_sptlight_pos;
    vec4 tang_pntlight_pos[NR_POINT_LIGHTS];
@end

@block lighting_vs_uniforms
#define NR_POINT_LIGHTS 4
    vec4 upntlight_pos[NR_POINT_LIGHTS];
    vec3 uviewpos;
    vec3 udirlight_dir;
    vec3 usptlight_dir;
    vec3 usptlight_pos;
@end

@block lighting_structures
struct dir_light_t {
    vec3 dir;
    vec3 ambi;
    vec3 diff;
    vec3 spec;
};

struct point_light_t {
    vec3 pos;
    float constant;
    float linear;
    float quadratic;  
    vec3 ambi;
    vec3 diff;
    vec3 spec;
};

struct spot_light_t {
    vec3 pos;
    vec3 dir;
    float cutoff;
    float outcutoff;
    float constant;
    float linear;
    float quadratic;
    vec3 ambi;
    vec3 diff;
    vec3 spec; 
};

struct lightdata_t {
    vec3 pixdiff;
    vec3 pixspec;
    vec3 normal;
    vec3 fragpos;
    vec3 viewdir;
    float matshine;
};
@end

@block lighting_fs_dirlight_uniforms
    vec3 light_ambi;
    vec3 light_diff;
    vec3 light_spec;
    vec3 light_dir;
@end

@block lighting_fs_pointlight_uniforms
    vec4 ambi[NR_POINT_LIGHTS];
    vec4 diff[NR_POINT_LIGHTS];
    vec4 spec[NR_POINT_LIGHTS];
    vec4 atte[NR_POINT_LIGHTS];
    vec4 pos[NR_POINT_LIGHTS];
    float enabled;
@end

@block lighting_fs_spotlight_uniforms
    vec3 atte;
    float cutoff;
    vec3 ambi;
    float outcutoff;
    vec3 diff;
    vec3 spec;
    vec3 dir;
    vec3 pos;
@end

@block lighting_functions
dir_light_t get_directional_light() {
    return dir_light_t(
        ufs_slow.light_dir,
        ufs_slow.light_ambi,
        ufs_slow.light_diff,
        ufs_slow.light_spec
    );
}

point_light_t get_point_light(const int i) {
    return point_light_t(
        upoint_lights.pos[i].xyz,
        upoint_lights.atte[i].x,
        upoint_lights.atte[i].y,
        upoint_lights.atte[i].z,
        upoint_lights.ambi[i].xyz,
        upoint_lights.diff[i].xyz,
        upoint_lights.spec[i].xyz
    );
}

spot_light_t get_spot_light() {
    return spot_light_t(
        uspot_light.pos,
        uspot_light.dir,
        uspot_light.cutoff,
        uspot_light.outcutoff,
        uspot_light.atte.x,
        uspot_light.atte.y,
        uspot_light.atte.z,
        uspot_light.ambi,
        uspot_light.diff,
        uspot_light.spec
    );
}

vec3 calc_dir_light(const dir_light_t light, const lightdata_t lightdata)
{
    vec3 lightdir = normalize(-light.dir);
    //diffuse shading
    float diff = max(dot(lightdata.normal, lightdir), 0.0);
    //specular
    vec3 reflectdir = reflect(-lightdir, lightdata.normal);
    vec3 halfwaydir = normalize(lightdir + lightdata.viewdir);
    float spec = pow(max(dot(lightdata.normal, halfwaydir), 0.0), lightdata.matshine);
    //combine
    vec3 ambient = light.ambi * lightdata.pixdiff;
    vec3 diffuse = light.diff * diff * lightdata.pixdiff;
    vec3 specular = light.spec * spec * lightdata.pixspec;
    
    return (ambient + diffuse + specular);
}

vec3 calc_point_light(const point_light_t light, const lightdata_t lightdata)
{
    vec3 lightdir = normalize(light.pos - lightdata.fragpos);
    //diffuse
    float diff = max(dot(lightdata.normal, lightdir), 0.0);
    //specular
    vec3 reflectdir = reflect(-lightdir, lightdata.normal);
    float spec = pow(max(dot(lightdata.viewdir, reflectdir), 0.0), lightdata.matshine);
    //attenuation
    float distance = length(light.pos - lightdata.fragpos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));

    //combine
    vec3 ambient = light.ambi * lightdata.pixdiff;
    vec3 diffuse = light.diff * diff * lightdata.pixdiff;
    vec3 specular = light.spec * spec * lightdata.pixspec;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 calc_spot_light(const spot_light_t light, const lightdata_t lightdata)
{
    vec3 lightdir = normalize(light.pos - lightdata.fragpos);
    //diffuse
    float diff = max(dot(lightdata.normal, lightdir), 0.0);
    //specular
    vec3 reflectdir = reflect(-lightdir, lightdata.normal);
    float spec = pow(max(dot(lightdata.viewdir, reflectdir), 0.0), lightdata.matshine);
    //attenuation
    float distance = length(light.pos - lightdata.fragpos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));

    // spotlight intensity
    float theta = dot(lightdir, normalize(-light.dir));
    float epsilon = light.cutoff - light.outcutoff;
    float intensity = clamp((theta - light.outcutoff) / epsilon, 0.0, 1.0);

    //combine
    vec3 ambient = light.ambi * lightdata.pixdiff;
    vec3 diffuse = light.diff * diff * lightdata.pixdiff;
    vec3 specular = light.spec * spec * lightdata.pixspec;

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    return (ambient + diffuse + specular);
}
@end
