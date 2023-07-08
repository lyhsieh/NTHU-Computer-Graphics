#version 330 core

out vec4 FragColor;
in vec3 vertex_color;
in vec3 vertex_normal;
in vec3 vertex_position;

vec3 camera_position = vec3(0.0, 0.0, 2.0);
vec3 color = vec3(0.0, 0.0, 0.0);
const float PI = 3.1416;

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


float f_att, d, spotlight_effect;
vec3 light_direction, Diffuse, Specular;
vec3 S, S1, S2 = normalize(camera_position - vertex_position);

void setDirectional() {
    f_att = 1.0;
    Diffuse = max(dot(-light_info[0].direction, vertex_normal), 0) * light_info[0].diffuse * Kd;
    S1 = -light_info[0].direction;
    S = normalize(S1 + S2);
    Specular = pow(max(dot(S, vertex_normal), 0), shininess) * vec3(1.0, 1.0, 1.0) * Ks;
    spotlight_effect = 1.0f;
}

void setPoint() {
    d = length(vertex_position - light_info[1].position);
    f_att = min(1, 1 / (light_info[1].att_con + light_info[1].att_lin * d + light_info[1].att_qua * d * d));
    light_direction = normalize(light_info[1].position - vertex_position);
    vec3 normal = normalize(vertex_normal);
    Diffuse = max(dot(light_direction, normal), 0) * light_info[1].diffuse * Kd;
    S = reflect(-light_direction , normal);
    Specular = pow(max(dot(S, S2), 0), shininess) * vec3(1.0, 1.0, 1.0) * Ks;
    spotlight_effect = 1.0f;
}

void setSpot() {
    light_direction = normalize(vertex_position - light_info[2].position);
    d = length(vertex_position - light_info[2].position);
    f_att = min(1, 1 / (light_info[2].att_con + light_info[2].att_lin * d + light_info[2].att_qua * d * d));
    spotlight_effect = pow(max(dot(light_direction, light_info[2].direction), 0), light_info[2].spot_exp);
    Diffuse = max(dot(-light_direction, vertex_normal), 0) * light_info[2].diffuse * Kd;
    S = normalize(-light_direction + S2);
    Specular = pow(max(dot(S, vertex_normal), 0), shininess) * vec3(1.0, 1.0, 1.0) * Ks;
}

void calc(int light_src) {
    switch(light_src) {
        case 0: setDirectional(); break;
        case 1: setPoint(); break;
        case 2: setSpot(); break;
        default: break;
    }
    color = 0.15 * Ka + f_att * (Diffuse + Specular) * spotlight_effect;

    if (light_src == 2 && dot(light_direction, normalize(light_info[2].direction)) <= cos(light_info[2].spot_cutoff * PI / 180))
        color = 0.15 * Ka;
}

void main() {
    // [TODO]
    if (per_vertex_or_per_pixel == 1) {
        calc(light_src);
        FragColor = vec4(color, 1.0f);
    }
    else
        FragColor = vec4(vertex_color, 1.0f);
}
