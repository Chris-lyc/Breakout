#version 330 core

in vec2 TexCoords;
out vec4 color;

uniform sampler2D scene;
uniform vec2 offset[9];
uniform int edge_kernel[9];
uniform float blur_kernel[9];

uniform bool chaos;
uniform bool confuse;
uniform bool shake;

void main()
{
    color = vec4(0.0);
    vec3 sample[9];
    // 如果使用卷积矩阵，则对纹理的偏移像素进行采样
    if(chaos || shake)
    {
        for(int i = 0; i < 9; ++i)
            sample[i] = texture2D(scene, TexCoords + offset[i]).rgb;
    }
    // 处理特效
    if(chaos)
    {
        for(int i = 0; i < 9; ++i)
            color += vec4(sample[i] * edge_kernel[i], 0.0);
        color.a = 1.0;
    }
    else if(confuse)
    {
        color = vec4(1.0 - texture2D(scene, TexCoords).rgb, 1.0);
    }
    else if(shake)
    {
        for(int i = 0; i < 9; ++i)
            color += vec4(sample[i] * blur_kernel[i], 0.0);
        color.a = 1.0;
    }
    else
    {
        color = texture2D(scene, TexCoords);
    }
}
