#version 330

in vec2 texCoord;

out vec4 fragColor;

in vec3 vertex_color;
in vec3 vertex_normal;
in vec3 vertex_position;

uniform sampler2D samp;

uniform int light_src;
uniform int per_vertex_or_per_pixel;
uniform float shininess;

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

uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;



// [TODO] passing texture from main.cpp
// Hint: sampler2D

// self-defined params
vec3 color, direction, diffuse, V, S, specular;
const float PI = 3.1416;
float distance;

void SetDirectional() {
	direction = normalize(light_info[0].position);
	diffuse = max(dot(direction, vertex_normal),0) * light_info[0].diffuse * Kd;
	V = normalize(vec3(0.0f ,0.0f, 2.0f) - vertex_position);	
	S = normalize(direction + V);  
	specular = pow(max(dot(S, vertex_normal),0), shininess) * Ks;
	color = 0.15 * Ka + diffuse + specular;
}

void SetPoint() {
	V = normalize(vec3(0.0f, 0.0f, 2.0f) - vertex_position);
	vec3 L = normalize(light_info[1].position - vertex_position);
	vec3 H = normalize(L + V);
	vec3 N = normalize(vertex_normal);
	vec3 diffuse = max(dot(L, N), 0.0f) * light_info[1].diffuse * Kd;
	vec3 specular = pow(max(dot(H, N), 0.0f), shininess) * Ks;

	distance = length(light_info[1].position - vertex_position);
	float attenuation = light_info[1].att_con 
						+ light_info[1].att_lin * distance 
						+ light_info[1].att_qua * distance * distance;
	float f_att = min(1.0f / attenuation, 1.0f);
	color = 0.15 * Ka + f_att * (diffuse + specular);
}

void SetSpot() {
	direction = normalize(vertex_position - light_info[2].position);
	if (dot(direction, normalize(light_info[2].direction)) > cos(light_info[2].spot_cutoff * PI / 180.0f)) {
		diffuse = max(dot(-direction, vertex_normal),0) * light_info[2].diffuse * Kd;
		V = normalize(vec3(0.0f, 0.0f, 2.0f) - vertex_position);	
		S = normalize(-direction + V);
		distance = length(vertex_position - light_info[2].position);
		float attenuation = light_info[2].att_con 
							+ light_info[2].att_lin * distance 
							+ light_info[2].att_qua * distance * distance;
		float f_att = min(1.0f / attenuation, 1.0f);
		specular = pow(max(dot(S,vertex_normal),0), shininess) * vec3(1.0f, 1.0f, 1.0f) * Ks;
		float spot_effect =  pow(max(dot(direction, light_info[2].direction), 0), light_info[2].spot_exp);
		color = 0.15 * Ka + f_att * (diffuse + specular) * spot_effect;
	}
	else {
		color = 0.15 * Ka;
	}
}

void main() {
	if (per_vertex_or_per_pixel == 0) {
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
		}
		fragColor = vec4(color, 1.0f) * texture(samp, texCoord);
	}
	else {
		fragColor = vec4(vertex_color, 1.0f) * texture(samp, texCoord);
	}
}
