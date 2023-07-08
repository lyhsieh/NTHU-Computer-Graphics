#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 vertex_color;
out vec3 vertex_normal;
out vec3 vertex_position;

uniform mat4 mvp;
uniform mat4 m;

uniform int per_vertex_or_per_pixel;
uniform int light_src;
uniform float shininess;

uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;

struct LightInfo {
    vec3 position;
    vec3 direction;
    vec3 diffuse;
    float att_con;
    float att_lin;
    float att_qua;
    float spot_cutoff;
    float spot_exp;
};
uniform LightInfo light_info[3];

vec3 camera_position = vec3(0.0, 0.0, 2.0);
vec3 normal;
vec3 position = vec3(m * vec4(aPos.x, aPos.y, aPos.z, 1.0));
const float PI = 3.1416;

float f_att, d, spotlight_effect;
vec3 light_direction, Diffuse, Specular;
vec3 S, S1, S2 = normalize(camera_position - position);


void setDirectional() {
    f_att = 1.0;
    Diffuse = max(dot(-light_info[0].direction, normal), 0) * light_info[0].diffuse * Kd;
    S1 = -light_info[0].direction;
    S = normalize(S1 + S2);
    spotlight_effect = 1.0f;
}

void setPoint() {
    d = length(position - light_info[1].position);
    f_att = min(1, 1 / (light_info[1].att_con + light_info[1].att_lin * d + light_info[1].att_qua * d * d));
    light_direction = normalize(position - light_info[1].position);
    Diffuse = max(dot(-light_direction, normal), 0) * light_info[1].diffuse * Kd;
    S1 = -light_direction;
    S = normalize(S1 + S2);
    spotlight_effect = 1.0f;
}

void setSpot() {
    light_direction = normalize(position - light_info[2].position);
    d = length(position - light_info[2].position);
    f_att = min(1, 1 / (light_info[1].att_con + light_info[1].att_lin * d + light_info[1].att_qua * d * d));
    spotlight_effect = pow(max(dot(light_direction, light_info[2].direction), 0), light_info[2].spot_exp);
    Diffuse = max(dot(-light_direction, normal), 0) * light_info[2].diffuse * Kd;
    S1 = -light_direction;
    S = normalize(S1 + S2);
}


void calc(int light_src){
    switch(light_src) {
        case 0: setDirectional(); break;
        case 1: setPoint(); break;
        case 2: setSpot(); break;
        default: break;
    }

    Specular = pow(max(dot(S, normal), 0), shininess) * vec3(1.0, 1.0, 1.0) * Ks;
    
    vertex_color = 0.15 * Ka + f_att * (Diffuse + Specular) * spotlight_effect;

    if (light_src == 2 && dot(light_direction, normalize(light_info[2].direction)) <= cos(light_info[2].spot_cutoff * PI / 180)){
        vertex_color = 0.15 *  Ka;
    }
}

void main()
{
    // [TODO]
    gl_Position = mvp * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    normal = normalize(mat3(transpose(inverse(m))) * aNormal);

    if (per_vertex_or_per_pixel == 0) {
        calc(light_src);
    }
    else {
        vertex_position = vec3(m * vec4(aPos.x, aPos.y, aPos.z, 1.0));
        vertex_color = aColor;
        vertex_normal = normal;
    }
}

