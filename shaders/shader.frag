#version 450 core

layout(location = 0) in vec4 fPosition;
layout(location = 1) in vec4 fNormal;
layout(location = 2) in vec4 fLightPos;
layout(location = 3) in vec4 fObjectColor;
layout(location = 4) in vec4 fLightColor;

layout(location = 0) out vec4 outColor;

const float shininess = 4;

void main() {
    // ambient
    const vec4 ambient = fLightColor * fObjectColor;
  	
    // diffuse 
    const vec4 norm = normalize(fNormal);
    const vec4 lightDir = normalize(fLightPos - fPosition);
    const float diff = max(dot(norm, lightDir), 0.0);
    const vec4 diffuse = fLightColor * (diff * fObjectColor);
    
    // specular
    const vec4 viewDir = normalize(fPosition);
    const vec4 reflectDir = reflect(-lightDir, norm);  
    const float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    const vec4 specular = fLightColor * (spec * fObjectColor);
        
    outColor = ambient + diffuse + specular;
}