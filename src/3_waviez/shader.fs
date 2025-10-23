#version 330 core
out vec4 FragColor;

// in vec2 TexCoord;

// texture samplers
// uniform sampler2D texture1;
// uniform sampler2D texture2;

uniform vec3 objColor;

void main()
{
	// linearly interpolate between both textures (20% container, 80% awesomeface)
	// FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.8);

	FragColor = vec4(objColor, 1.0);
}