#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec2 vPos; // used only by background
in float vAlpha; // used only by trail

uniform sampler2D uTexture;
uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform int uRenderMode; // 0=bg, 1=sun, 2=planet
uniform vec3 uColor; // orbit trail color

void main() {
    if (uRenderMode == 0) {
        // --- SPACE BACKGROUND ---
        float t = (vPos.y + 1.0) * 0.5;
        vec3 topColor = vec3(0.05, 0.08, 0.18);
        vec3 bottomColor = vec3(0.0, 0.0, 0.0);
        vec3 color = mix(bottomColor, topColor, t);
        FragColor = vec4(color, 1.0);
        return;
    }

    vec3 color = texture(uTexture, TexCoord).rgb;

    if (uRenderMode == 1) {
        // emissive sun
        vec3 glow = color * vec3(2.5, 2.3, 1.7);
        FragColor = vec4(glow, 1.0);
        return;
    }

    if (uRenderMode == 2) {
        // phong shading for planets
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(uLightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);

        vec3 viewDir = normalize(uViewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

        vec3 ambient = 0.1 * color;
        vec3 diffuse = diff * color;
        vec3 specular = vec3(0.3) * spec;

        FragColor = vec4(ambient + diffuse + specular, 1.0);
        return;
    }

    if (uRenderMode == 3) {
        // orbit trail
        FragColor = vec4(uColor, vAlpha);
        return;
    }

    // fallback
    FragColor = vec4(1.0, 0.0, 1.0, 1.0); // magenta debug
}
