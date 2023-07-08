#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 1600;
const int WINDOW_HEIGHT = 800;

int cur_window_width = WINDOW_WIDTH;
int cur_window_height = WINDOW_HEIGHT;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;


enum TransMode
{
    GeoTranslation = 0,
    GeoRotation = 1,
    GeoScaling = 2,
    LightEdit = 3,
    ShininessEdit = 4
};

// self-defined params
#define PI 3.1416
bool first_time = true;

GLuint p;
GLint iLocMVP;
GLint iLocM;
GLint temp_info;
float shininess = 60.0f;

struct LightInfo {
    Vector3 position;
    Vector3 direction;
    Vector3 diffuse;
    GLfloat att_con;
    GLfloat att_lin;
    GLfloat att_qua;
    GLfloat spot_cutoff;
    GLfloat spot_exp;
};
LightInfo light_info[3];


enum LightSource {
    DIRECTIONAL = 0,
    POINT = 1,
    SPOT = 2
};
LightSource light_src = DIRECTIONAL;

enum LightingMode {
    PER_VERTEX = 0,
    PER_PIXEL = 1
};
LightingMode per_vertex_or_per_pixel = PER_VERTEX;

///

vector<string> filenames; // .obj filename list

struct PhongMaterial
{
    Vector3 Ka;
    Vector3 Kd;
    Vector3 Ks;
};

typedef struct
{
    GLuint vao;
    GLuint vbo;
    GLuint vboTex;
    GLuint ebo;
    GLuint p_color;
    int vertex_count;
    GLuint p_normal;
    PhongMaterial material;
    int indexCount;
    GLuint m_texture;
} Shape;

struct model
{
    Vector3 position = Vector3(0, 0, 0);
    Vector3 scale = Vector3(1, 1, 1);
    Vector3 rotation = Vector3(0, 0, 0);    // Euler form

    vector<Shape> shapes;
};
vector<model> models;

struct camera
{
    Vector3 position;
    Vector3 center;
    Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
    GLfloat nearClip, farClip;
    GLfloat fovy;
    GLfloat aspect;
    GLfloat left, right, top, bottom;
};
project_setting proj;


TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;

int cur_idx = 0; // represent which model should be rendered now


static GLvoid Normalize(GLfloat v[3])
{
    GLfloat l;

    l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

    n[0] = u[1] * v[2] - u[2] * v[1];
    n[1] = u[2] * v[0] - u[0] * v[2];
    n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
    Matrix4 mat;

    mat = Matrix4(
        1, 0, 0, vec.x,
        0, 1, 0, vec.y,
        0, 0, 1, vec.z,
        0, 0, 0, 1
    );

    return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
    Matrix4 mat;

    mat = Matrix4(
        vec.x, 0, 0, 0,
        0, vec.y, 0, 0,
        0, 0, vec.z, 0,
        0, 0, 0, 1
    );

    return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
    Matrix4 mat;

    GLfloat theta = val * PI / 180.0;
        
    mat = Matrix4(
        1, 0, 0, 0,
        0, cos(theta), -sin(theta), 0,
        0, sin(theta), cos(theta), 0,
        0, 0, 0, 1
    );

    return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
    Matrix4 mat;

    GLfloat theta = val * PI / 180.0;
            
    mat = Matrix4(
        cos(theta), 0, sin(theta), 0,
        0, 1, 0, 0,
        -sin(theta), 0, cos(theta), 0,
        0, 0, 0, 1
    );

    return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
    Matrix4 mat;

    GLfloat theta = val * PI / 180.0;

    mat = Matrix4(
        cos(theta), -sin(theta), 0, 0,
        sin(theta), cos(theta), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );

    return mat;
}

Matrix4 rotate(Vector3 vec)
{
    return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
    Vector3 P1P2, P1P3;
    GLfloat Rx[3], Ry[3], Rz[3];
    Matrix4 R, T;
    P1P2 = main_camera.center - main_camera.position;
    P1P3 = main_camera.up_vector - main_camera.position;
    GLfloat p12[3] = {P1P2.x, P1P2.y, P1P2.z}, p13[3] = {P1P3.x, P1P3.y, P1P3.z};
    
    for (int i = 0; i < 3; ++i)
        Rz[i] = -p12[i];
    Cross(p12, p13, Rx);
    Cross(Rz, Rx, Ry);
    Normalize(Rx);
    Normalize(Ry);
    Normalize(Rz);
    
    R = Matrix4(
        Rx[0], Rx[1], Rx[2], 0,
        Ry[0], Ry[1], Ry[2], 0,
        Rz[0], Rz[1], Rz[2], 0,
        0, 0, 0, 1
    );
    T = translate(-main_camera.position);
    view_matrix = R * T;
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
    // project_matrix [...] = ...
    GLfloat f = 1 / tan(proj.fovy * 3.14159 / 180 / 2);

    project_matrix = Matrix4(
        f / proj.aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip), 2 * proj.farClip * proj.nearClip / (proj.nearClip - proj.farClip),
        0, 0, -1, 0
    );
}

void setGLMatrix(GLfloat* glm, Matrix4& m) {
    glm[0] = m[0];  glm[4] = m[1];   glm[8] = m[2];    glm[12] = m[3];
    glm[1] = m[4];  glm[5] = m[5];   glm[9] = m[6];    glm[13] = m[7];
    glm[2] = m[8];  glm[6] = m[9];   glm[10] = m[10];   glm[14] = m[11];
    glm[3] = m[12];  glm[7] = m[13];  glm[11] = m[14];   glm[15] = m[15];
}

// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    // [TODO] change your aspect ratio
    cur_window_width = width;
    cur_window_height = height;
    proj.aspect = (float)width / (float)height / 2.0f;
    setViewingMatrix();
    setPerspective();
}

// set the light of two shaders
void setLight() {
    temp_info = glGetUniformLocation(p, "light_src");
    glUniform1i(temp_info, light_src);
    temp_info = glGetUniformLocation(p, "per_vertex_or_per_pixel");
    glUniform1i(temp_info, per_vertex_or_per_pixel);

    temp_info = glGetUniformLocation(p, "light_info[0].position");
    glUniform3f(temp_info, light_info[0].position.x, light_info[0].position.y, light_info[0].position.z);
    temp_info = glGetUniformLocation(p, "light_info[0].direction");
    glUniform3f(temp_info, light_info[0].direction.x, light_info[0].direction.y, light_info[0].direction.z);

    temp_info = glGetUniformLocation(p, "light_info[1].position");
    glUniform3f(temp_info, light_info[1].position.x, light_info[1].position.y, light_info[1].position.z);

    temp_info = glGetUniformLocation(p, "light_info[2].position");
    glUniform3f(temp_info, light_info[2].position.x, light_info[2].position.y, light_info[2].position.z);
    temp_info = glGetUniformLocation(p, "light_info[2].direction");
    glUniform3f(temp_info, light_info[2].direction.x, light_info[2].direction.y, light_info[2].direction.z);
    temp_info = glGetUniformLocation(p, "light_info[2].spot_cutoff");
    glUniform1f(temp_info, light_info[2].spot_cutoff);
    temp_info = glGetUniformLocation(p, "light_info[2].spot_exp");
    glUniform1f(temp_info, light_info[2].spot_exp);

    temp_info = glGetUniformLocation(p, "light_info[0].diffuse");
    glUniform3f(temp_info, light_info[0].diffuse.x, light_info[0].diffuse.y, light_info[0].diffuse.z);
    temp_info = glGetUniformLocation(p, "light_info[1].diffuse");
    glUniform3f(temp_info, light_info[1].diffuse.x, light_info[1].diffuse.y, light_info[1].diffuse.z);
    temp_info = glGetUniformLocation(p, "light_info[2].diffuse");
    glUniform3f(temp_info, light_info[2].diffuse.x, light_info[2].diffuse.y, light_info[2].diffuse.z);

    temp_info = glGetUniformLocation(p, "light_info[1].att_con");
    glUniform1f(temp_info, light_info[1].att_con);
    temp_info = glGetUniformLocation(p, "light_info[1].att_lin");
    glUniform1f(temp_info, light_info[1].att_lin);
    temp_info = glGetUniformLocation(p, "light_info[1].att_qua");
    glUniform1f(temp_info, light_info[1].att_qua);

    temp_info = glGetUniformLocation(p, "light_info[2].att_con");
    glUniform1f(temp_info, light_info[2].att_con);
    temp_info = glGetUniformLocation(p, "light_info[2].att_lin");
    glUniform1f(temp_info, light_info[2].att_lin);
    temp_info = glGetUniformLocation(p, "light_info[2].att_qua");
    glUniform1f(temp_info, light_info[2].att_qua);

    temp_info = glGetUniformLocation(p, "shininess");
    glUniform1f(temp_info, shininess);


    for (int i = 0; i < models[cur_idx].shapes.size(); ++i) {
        temp_info = glGetUniformLocation(p, "Ka");
        glUniform3fv(temp_info, 1, &(models[cur_idx].shapes[i].material.Ka[0]));
        temp_info = glGetUniformLocation(p, "Kd");
        glUniform3fv(temp_info, 1, &(models[cur_idx].shapes[i].material.Kd[0]));
        temp_info = glGetUniformLocation(p, "Ks");
        glUniform3fv(temp_info, 1, &(models[cur_idx].shapes[i].material.Ks[0]));

        glBindVertexArray(models[cur_idx].shapes[i].vao);
        glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
    }
}

// Render function for display rendering
void RenderScene(void) {
    // clear canvas
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    Matrix4 T, R, S;
    // [TODO] update translation, rotation and scaling
    T = translate(models[cur_idx].position);
    R = rotate(models[cur_idx].rotation);
    S = scaling(models[cur_idx].scale);

    Matrix4 MVP;
    GLfloat mvp[16];

    // [TODO] multiply all the matrix
    MVP = project_matrix * view_matrix * T * R * S;
    Matrix4 M = T * R * S;
    GLfloat m[16];
    
    // row-major ---> column-major
    setGLMatrix(mvp, MVP);
    setGLMatrix(m, M);


    // use uniform to send mvp to vertex shader
    glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
    glUniformMatrix4fv(iLocM, 1, GL_FALSE, m);

    // render left plane
    per_vertex_or_per_pixel = PER_VERTEX;
    glViewport(0, 0, cur_window_width / 2, cur_window_height);
    setLight();

    // render right plane
    per_vertex_or_per_pixel = PER_PIXEL;
    glViewport(cur_window_width / 2, 0, cur_window_width / 2, cur_window_height);
    setLight();
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // [TODO] Call back function for keyboard
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_Z:
                cur_idx--;
                if (cur_idx < 0)
                    cur_idx = 4;
                cout << "switch to model #" << cur_idx << "\n";
                break;
                
            case GLFW_KEY_X:
                cur_idx = (++cur_idx) % 5;
                cout << "switch to model #" << cur_idx << "\n";
                break;
                
            case GLFW_KEY_T:
                cout << "switch to translation mode\n";
                cur_trans_mode = GeoTranslation;
                break;
                
            case GLFW_KEY_S:
                cout << "switch to scale mode\n";
                cur_trans_mode = GeoScaling;
                break;
                
            case GLFW_KEY_R:
                cout << "switch to rotation mode\n";
                cur_trans_mode = GeoRotation;
                break;
                
            case GLFW_KEY_L:
                if (light_src == DIRECTIONAL)
                    light_src = POINT;
                else if (light_src == POINT)
                    light_src = SPOT;
                else
                    light_src = DIRECTIONAL;
                cout << "switch to " << light_src << " (0 for direc.; 1 for point; 2 for spot)\n";
                break;
            
            case GLFW_KEY_K:
                cur_trans_mode = LightEdit;
                cout << "switch to light editing mode\n";
                break;
                
            case GLFW_KEY_J:
                cur_trans_mode = ShininessEdit;
                cout << "switch to shininess editing mode\n";
                break;
                
            // case GLFW_KEY_I:
            //     cout << "Information:\n";
            //     cout << "(1) Translation Matrix:\n" << translate(models[cur_idx].position) << "\n";
            //     cout << "(2) Rotation Matrix\n" << rotate(models[cur_idx].rotation) << "\n";
            //     cout << "(3) Scaling Matrix\n" << scaling(models[cur_idx].scale) << "\n";
            //     cout << "(4) Viewing Matrix\n" << view_matrix << "\n";
            //     cout << "(5) Projection Matrix\n" << project_matrix << "\n";
            //     break;

            default:
                break;
        }
    }

}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // [TODO] scroll up positive, otherwise it would be negtive
    GLfloat scaling_factor = 50.0;
    GLfloat diff = yoffset / scaling_factor;
    switch (cur_trans_mode) {
        case GeoTranslation:
            cout << "(GeoTranslation) position.z: " << models[cur_idx].position.z;
            models[cur_idx].position.z += diff;
            cout << " -> " << models[cur_idx].position.z << "\n";
            break;
            
        case GeoRotation:
            cout << "(GeoRotation) rotation.z: " << models[cur_idx].rotation.z;
            models[cur_idx].rotation.z += yoffset;
            cout << " -> " << models[cur_idx].rotation.z << "\n";
            break;
            
        case GeoScaling:
            cout << "(GeoScaling) scale.z: " << models[cur_idx].scale.z;
            models[cur_idx].scale.z += diff;
            cout << " -> " << models[cur_idx].scale.z << "\n";
            break;
            
        case LightEdit:
            if (light_src == DIRECTIONAL) {
                light_info[0].diffuse += Vector3(1, 1, 1) * yoffset * 0.05;
            }
            else if (light_src == POINT) {
                light_info[1].diffuse += Vector3(1, 1, 1) * yoffset * 0.05;
            }
            else if (light_src == SPOT) {
                light_info[2].spot_cutoff += yoffset;
                cout << "cutoff = " << light_info[2].spot_cutoff << "\n";
                if (light_info[2].spot_cutoff >= 90.0)
                    light_info[2].spot_cutoff = 90.0f;
                else if (light_info[2].spot_cutoff <= 0.0)
                    light_info[2].spot_cutoff = 0.0f;
            }
            break;
                
        case ShininessEdit:
            // Apply change on shininess when scroll the wheel.
            // The shininess is applied to all models.
            cout << "(ShininessEdit) shininess: " << shininess;
            shininess += yoffset / 10;
            cout << " -> " << shininess << "\n";
            break;
            
        default:
            break;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // [TODO] mouse press callback function
    if (action == GLFW_PRESS) {
        cout << "mouse pressed\n";
        mouse_pressed = true;
    }
    else {
        cout << "mouse released\n";
        mouse_pressed = false;
        first_time = true;
    }
}


GLfloat curx, cury, prevx, prevy, difference;

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    // [TODO] cursor position callback function
    prevx = curx;
    prevy = cury;
    curx = xpos;
    cury = ypos;
    GLfloat dx = curx - prevx, dy = cury - prevy;
    GLfloat scaling_factor = 60.0;
    // GLfloat dx = xpos - starting_press_x, dy = ypos - starting_press_y;
    // printf("dx = %lf, dy = %lf\n", dx, dy);
    if (starting_press_x < 0 && starting_press_y < 0) {
        starting_press_x = xpos;
        starting_press_y = ypos;
    }
    // printf("xpos = %lf, xstart = %d, ypos = %lf, ystart = %d, first = %d\n", xpos, starting_press_x, ypos, starting_press_y, first_time);
    if (!mouse_pressed)
        return;
    if (first_time) {
        first_time = false;
        starting_press_x = xpos;
        starting_press_y = ypos;
        return;
    }
    
    switch (cur_trans_mode) {
        case GeoTranslation:
            models[cur_idx].position.x += dx / scaling_factor;
            models[cur_idx].position.y += dy / scaling_factor;
            break;
        
        case GeoScaling:
            models[cur_idx].scale.x += dx / scaling_factor;
            models[cur_idx].scale.y += dy / scaling_factor;
            break;
            
        case GeoRotation:
            models[cur_idx].rotation.x += dy * 0.5;
            models[cur_idx].rotation.y += dx * 0.5;
            break;
        
        case LightEdit:
            if (light_src == DIRECTIONAL) {
                light_info[light_src].direction.x -= dx / scaling_factor;
                light_info[light_src].direction.y += dy / scaling_factor;
            }
            else {
                light_info[light_src].position.x += dx / scaling_factor;
                light_info[light_src].position.y -= dy / scaling_factor;
            }
            break;
            
        default:
            break;
    }
    if (mouse_pressed) {
        starting_press_x = xpos;
        starting_press_y = ypos;
    }
}


void setShaders()
{
    GLuint v, f;
    char *vs = NULL;
    char *fs = NULL;

    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);

    vs = textFileRead("shader.vs");
    fs = textFileRead("shader.fs");

    glShaderSource(v, 1, (const GLchar**)&vs, NULL);
    glShaderSource(f, 1, (const GLchar**)&fs, NULL);

    free(vs);
    free(fs);

    GLint success;
    char infoLog[1000];
    // compile vertex shader
    glCompileShader(v);
    // check for shader compile errors
    glGetShaderiv(v, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(v, 1000, NULL, infoLog);
        std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
    }

    // compile fragment shader
    glCompileShader(f);
    // check for shader compile errors
    glGetShaderiv(f, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(f, 1000, NULL, infoLog);
        std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
    }

    // create program object
    p = glCreateProgram();

    // attach shaders to program object
    glAttachShader(p,f);
    glAttachShader(p,v);

    // link program
    glLinkProgram(p);
    // check for linking errors
    glGetProgramiv(p, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(p, 1000, NULL, infoLog);
        std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(v);
    glDeleteShader(f);

    iLocMVP = glGetUniformLocation(p, "mvp");
    iLocM = glGetUniformLocation(p, "m");


    if (success)
        glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
    vector<float> xVector, yVector, zVector;
    float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

    // find out min and max value of X, Y and Z axis
    for (int i = 0; i < attrib->vertices.size(); i++)
    {
        //maxs = max(maxs, attrib->vertices.at(i));
        if (i % 3 == 0)
        {

            xVector.push_back(attrib->vertices.at(i));

            if (attrib->vertices.at(i) < minX)
            {
                minX = attrib->vertices.at(i);
            }

            if (attrib->vertices.at(i) > maxX)
            {
                maxX = attrib->vertices.at(i);
            }
        }
        else if (i % 3 == 1)
        {
            yVector.push_back(attrib->vertices.at(i));

            if (attrib->vertices.at(i) < minY)
            {
                minY = attrib->vertices.at(i);
            }

            if (attrib->vertices.at(i) > maxY)
            {
                maxY = attrib->vertices.at(i);
            }
        }
        else if (i % 3 == 2)
        {
            zVector.push_back(attrib->vertices.at(i));

            if (attrib->vertices.at(i) < minZ)
            {
                minZ = attrib->vertices.at(i);
            }

            if (attrib->vertices.at(i) > maxZ)
            {
                maxZ = attrib->vertices.at(i);
            }
        }
    }

    float offsetX = (maxX + minX) / 2;
    float offsetY = (maxY + minY) / 2;
    float offsetZ = (maxZ + minZ) / 2;

    for (int i = 0; i < attrib->vertices.size(); i++)
    {
        if (offsetX != 0 && i % 3 == 0)
        {
            attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
        }
        else if (offsetY != 0 && i % 3 == 1)
        {
            attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
        }
        else if (offsetZ != 0 && i % 3 == 2)
        {
            attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
        }
    }

    float greatestAxis = maxX - minX;
    float distanceOfYAxis = maxY - minY;
    float distanceOfZAxis = maxZ - minZ;

    if (distanceOfYAxis > greatestAxis)
    {
        greatestAxis = distanceOfYAxis;
    }

    if (distanceOfZAxis > greatestAxis)
    {
        greatestAxis = distanceOfZAxis;
    }

    float scale = greatestAxis / 2;

    for (int i = 0; i < attrib->vertices.size(); i++)
    {
        //std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
        attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
    }
    size_t index_offset = 0;
    for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
        int fv = shape->mesh.num_face_vertices[f];

        // Loop over vertices in the face.
        for (size_t v = 0; v < fv; v++) {
            // access to vertex
            tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
            vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
            vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
            vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
            // Optional: vertex colors
            colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
            colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
            colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
            // Optional: vertex normals
            if (idx.normal_index >= 0) {
                normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
                normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
                normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
            }
        }
        index_offset += fv;
    }
}

string GetBaseDir(const string& filepath) {
    if (filepath.find_last_of("/\\") != std::string::npos)
        return filepath.substr(0, filepath.find_last_of("/\\"));
    return "";
}

void LoadModels(string model_path)
{
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;
    tinyobj::attrib_t attrib;
    vector<GLfloat> vertices;
    vector<GLfloat> colors;
    vector<GLfloat> normals;

    string err;
    string warn;

    string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
    base_dir += "\\";
#else
    base_dir += "/";
#endif

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

    if (!warn.empty()) {
        cout << warn << std::endl;
    }

    if (!err.empty()) {
        cerr << err << std::endl;
    }

    if (!ret) {
        exit(1);
    }

    printf("Load Models Success ! Shapes size %d Material size %d\n", shapes.size(), materials.size());
    model tmp_model;

    vector<PhongMaterial> allMaterial;
    for (int i = 0; i < materials.size(); i++)
    {
        PhongMaterial material;
        material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
        material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
        material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
        allMaterial.push_back(material);
    }

    for (int i = 0; i < shapes.size(); i++)
    {

        vertices.clear();
        colors.clear();
        normals.clear();
        normalization(&attrib, vertices, colors, normals, &shapes[i]);
        // printf("Vertices size: %d", vertices.size() / 3);

        Shape tmp_shape;
        glGenVertexArrays(1, &tmp_shape.vao);
        glBindVertexArray(tmp_shape.vao);

        glGenBuffers(1, &tmp_shape.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        tmp_shape.vertex_count = vertices.size() / 3;

        glGenBuffers(1, &tmp_shape.p_color);
        glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glGenBuffers(1, &tmp_shape.p_normal);
        glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        // not support per face material, use material of first face
        if (allMaterial.size() > 0)
            tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
        tmp_model.shapes.push_back(tmp_shape);
    }
    shapes.clear();
    materials.clear();
    models.push_back(tmp_model);
}

void initParameter()
{
    proj.left = -1;
    proj.right = 1;
    proj.top = 1;
    proj.bottom = -1;
    proj.nearClip = 0.001;
    proj.farClip = 100.0;
    proj.fovy = 80;
    proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

    main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
    main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
    main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

    setViewingMatrix();
    setPerspective();

    light_info[0].position = Vector3(1.0f, 1.0f, 1.0f);
    light_info[0].direction = Vector3(-1.0f, -1.0f, -1.0f);

    light_info[1].position = Vector3(0.0f, 2.0f, 1.0f);

    light_info[2].position = Vector3(0.0f, 0.0f, 2.0f);
    light_info[2].direction = Vector3(0.0f, 0.0f, -1.0f);
    light_info[2].spot_cutoff = 30.0;
    light_info[2].spot_exp = 50.0;

    light_info[0].diffuse = Vector3(1.0f, 1.0f, 1.0f);
    light_info[1].diffuse = Vector3(1.0f, 1.0f, 1.0f);
    light_info[2].diffuse = Vector3(1.0f, 1.0f, 1.0f);

    light_info[1].att_con = 0.01f;
    light_info[1].att_lin = 0.8f;
    light_info[1].att_qua = 0.1f;

    light_info[2].att_con = 0.05f;
    light_info[2].att_lin = 0.3f;
    light_info[2].att_qua = 0.6f;
}

void setupRC()
{
    // setup shaders
    setShaders();
    initParameter();

    // OpenGL States and Values
    glClearColor(0.2, 0.2, 0.2, 1.0);
    vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
    // [TODO] Load five model at here
    for (int i = 0; i < model_list.size(); ++i) {
        LoadModels(model_list[i]);
    }
}

void glPrintContextInfo(bool printExtension)
{
    cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
    cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
    cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
    cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
    if (printExtension)
    {
        GLint numExt;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
        cout << "GL_EXTENSIONS =" << endl;
        for (GLint i = 0; i < numExt; i++)
        {
            cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
        }
    }
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "111061553 HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
    glEnable(GL_DEPTH_TEST);
    // Setup render context
    setupRC();

    // main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
    
    // just for compatibiliy purposes
    return 0;
}
