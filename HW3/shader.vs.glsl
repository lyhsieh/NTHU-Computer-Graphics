#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

out vec2 texCoord;

out vec3 vertex_position;
out vec3 vertex_color;
out vec3 vertex_normal;

uniform mat4 um4p;	
uniform mat4 um4v;
uniform mat4 um4m;

uniform int light_src;
uniform int per_vertex_or_per_pixel;
uniform float shininess;
uniform vec2 eye_offset;

uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;

struct LightInfo {
	vec3 position;
	vec3 direction;
	vec3 diffuse;
	float spot_cutoff;
    float spot_exp;
	float att_con;
    float att_lin;
    float att_qua;
};
uniform LightInfo light_info[3];

vec3 normal, position;
vec3 direction, diffuse, V, S, specular;
const float PI = 3.1416;
float distance;


void SetDirectional() {
	direction = normalize(light_info[0].position);
	diffuse = max(dot(direction, normal), 0) * light_info[0].diffuse * Kd;
	V = normalize(vec3(0.0f, 0.0f, 2.0f) - position);
	S = normalize(direction + V);  
	specular = pow(max(dot(S, normal), 0), shininess) * Ks;
	vertex_color = 0.15 * Ka + diffuse + specular;
}

void SetPoint() {
	direction = normalize(position - light_info[1].position);
	diffuse = max(dot(-direction, normal),0) * light_info[1].diffuse*Kd;
	V = normalize(vec3(0.0f, 0.0f, 2.0f) - position);	
	S = normalize(-direction + V);
	distance = length(position - light_info[1].position);
	float attenuation = light_info[1].att_con 
						+ light_info[1].att_lin * distance 
						+ light_info[1].att_qua * distance * distance;
	float f_att = min(1.0f / attenuation, 1.0f);
	specular = pow(max(dot(S, normal),0), shininess) * Ks;
	vertex_color = 0.15 * Ka + f_att * (diffuse + specular);
}

void SetSpot() {
	direction = normalize(position - light_info[2].position);
	if (dot(direction, normalize(light_info[2].direction)) > cos(light_info[2].spot_cutoff * PI / 180.0f)) {				
		diffuse = max(dot(-direction, normal), 0) * light_info[2].diffuse * Kd;
		V = normalize(vec3(0.0f, 0.0f, 2.0f) - position);	
		S = normalize(-direction + V);
		distance = length(position - light_info[2].position);
		float attenuation = light_info[2].att_con 
							+ light_info[2].att_lin * distance 
							+ light_info[2].att_qua * distance * distance;
		float f_att = min(1.0f / attenuation, 1.0f);
		specular = pow(max(dot(S, normal), 0), shininess) * vec3(1.0f, 1.0f, 1.0f) * Ks;
		float spot_effect = pow(max(dot(direction, light_info[2].direction), 0), light_info[2].spot_exp);
		vertex_color = 0.15 * Ka + f_att * (diffuse + specular) * spot_effect;
	}
	else {
		vertex_color = 0.15 * Ka;
	}
}

void main()
{
	// [TODO]
	gl_Position = um4p * um4v * um4m * vec4(aPos, 1.0f);
	normal = normalize(mat3(transpose(inverse(um4m))) * aNormal);
	position = vec3(um4m * vec4(aPos.x, aPos.y, aPos.z, 1.0));
	
	texCoord = aTexCoord + eye_offset;

	if (per_vertex_or_per_pixel == 1) {		
		switch(light_src) {
			case 0:
				SetDirectional();
				break;
			case 1:
				SetPoint();
				break;
			case 2:
				SetSpot();
				break;
			default: break;
		}
	}
	else {					
		vertex_position = vec3(um4m * vec4(aPos.x, aPos.y, aPos.z, 1.0));
		vertex_color = aColor;
		vertex_normal = normal;
	}
}

