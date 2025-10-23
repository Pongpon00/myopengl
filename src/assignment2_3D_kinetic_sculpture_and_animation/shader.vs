#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aAlpha; // for trail rendering

out float vAlpha; // for trail rendering
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec2 vPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform int uRenderMode;

void main() {
    if (uRenderMode == 0) {
        // background quad in NDC
        vPos = aPos.xy;
        gl_Position = vec4(aPos.xy, 0.0, 1.0);
        return;
    }

    if (uRenderMode == 3) {
        vAlpha = aAlpha;
        gl_Position = uProj * uView * vec4(aPos, 1.0);
        return;
    }

    // normal 3D path
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = uProj * uView * worldPos;
}
