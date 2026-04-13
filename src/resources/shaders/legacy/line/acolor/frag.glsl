uniform vec4 uColor;

in vec4 vColor;

out vec4 vFragmentColor;

void main()
{
    vFragmentColor = vColor * uColor;
}
