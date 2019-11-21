precision mediump float;

varying vec3 varying_pos;
varying vec3 varying_normal;
varying vec2 varying_uv;

uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D gloss_map;
uniform samplerCube env_map;

const vec3 light_pos = vec3(0.0, 0.0, 5.0);

void main() {
    // Make sure normal is unit length.
    vec3 n = normalize(varying_normal);

    // Direction from the point to the light source,
    // unit length.
    vec3 l = normalize(light_pos - varying_pos);

    // Lambertian
    float lambert = max(dot(l, n), 0.0);

    // Highlight
    float specular_value = 0.0;
    if (lambert > 0.0) {
        // Direction from point towards viewer.
        vec3 v = normalize(-varying_pos);

        // Halfway between viewer and light vectors,
        // used to approximate reflection (optimization).
        vec3 h = (normalize(l+v));

        // Alpha determines the specular hardness/size of highlight.
        float alpha = 255.0 * (texture2D(gloss_map, varying_uv).r);
        specular_value = pow(max(dot(h, n), 0.0), alpha);
    }

    vec4 refl_color = textureCube(env_map, reflect(varying_pos, n));
    vec4 diffuse_color = texture2D(diffuse_map, varying_uv);
    float spec_intensity = texture2D(specular_map, varying_uv).r;

    // Compute diffuse term.
    vec3 diffuse = lambert * ((1.0 - spec_intensity) * diffuse_color.rgb +
                                     spec_intensity * refl_color.rgb);
    vec3 specular = vec3(specular_value * spec_intensity);
    gl_FragColor = vec4(diffuse + specular, diffuse_color.a);
}
