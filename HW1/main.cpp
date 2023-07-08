#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
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

#define PI 3.1416

using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;


// self-defined constant
#define SOLID_MODE 1
#define WIREFRAME_MODE -1

// self-defined parameter
int display_mode = SOLID_MODE;     // deciding between solid and wireframe mode
bool first_time = true;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	ViewCenter = 3,
	ViewEye = 4,
	ViewUp = 5,
};

GLint iLocMVP;

vector<string> filenames; // .obj filename list

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form
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

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;


typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	int materialId;
	int indexCount;
	GLuint m_texture;
} Shape;
Shape quad;
Shape m_shpae;
vector<Shape> m_shape_list;
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
    // handout p.39

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
    // handout p.40

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
    // handout p.41

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
    // handout p.42

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

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
	cur_proj_mode = Orthogonal;
    GLfloat xmax = proj.right, xmin = proj.left;
    GLfloat ymax = proj.top, ymin = proj.bottom;
    GLfloat zfar = proj.farClip, znear = proj.nearClip;
    
    project_matrix = Matrix4(
        2.0 / (xmax - xmin), 0, 0, -((xmax + xmin) / (xmax - xmin)),
        0, 2.0 / (ymax - ymin), 0, -((ymax + ymin) / (ymax - ymin)),
        0, 0, -2.0 / (zfar - znear), -((zfar + znear) / (zfar - znear)),
        0, 0, 0, 1.0
    );
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	cur_proj_mode = Perspective;
    GLfloat f = 1.0 / tan((proj.fovy * PI / 180) / 2);
    GLfloat far = proj.farClip, near = proj.nearClip;
    project_matrix = Matrix4(
        f / proj.aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (far + near) / (near - far), (2.0 * far * near) / (near - far),
        0, 0, -1, 0
    );
    // p.132
}


// Vertex buffers
GLuint VAO, VBO;
// self-define
GLuint VBO_COLOR;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	// [TODO] change your aspect ratio
    proj.aspect = (float)width / (float)height;
    setViewingMatrix();
    if (cur_proj_mode == Orthogonal)
        setOrthogonal();
    else
        setPerspective();
}

void drawPlane()
{
	GLfloat vertices[18]{ 1.0, -0.9, -1.0,
		1.0, -0.9,  1.0,
		-1.0, -0.9, -1.0,
		1.0, -0.9,  1.0,
		-1.0, -0.9,  1.0,
		-1.0, -0.9, -1.0 };

	GLfloat colors[18]{ 0.0,1.0,0.0,
		0.0,0.5,0.8,
		0.0,1.0,0.0,
		0.0,0.5,0.8,
		0.0,0.5,0.8,
		0.0,1.0,0.0 };


	// [TODO] draw the plane with above vertices and color
    Matrix4 MVP = project_matrix * view_matrix;
    GLfloat mvp[16];
    // row-major ---> column-major
    mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
    mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
    mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
    mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];
    
    // glBindVertexArray(quad.vao);
    glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glGenBuffers(1, &VBO_COLOR);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_COLOR);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, 6);
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
    
	// [TODO] row-major ---> column-major
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];
    
    if (display_mode == SOLID_MODE)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// use uniform to send mvp to vertex shader
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	glBindVertexArray(m_shape_list[cur_idx].vao);
	glDrawArrays(GL_TRIANGLES, 0, m_shape_list[cur_idx].vertex_count);
	drawPlane();

}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_W:
                display_mode = -display_mode;
                cout << "switch display mode to " << display_mode << " (SOLID = 1, WIREFRAME = -1)\n";
                break;
                
            case GLFW_KEY_Z:
                cur_idx--;
                if (cur_idx < 0)
                    cur_idx = 4;
                cout << "switch to model #" << cur_idx << "\n";
                break;
                
            case GLFW_KEY_X:
                cur_idx++;
                cur_idx = cur_idx % 5;
                cout << "switch to model #" << cur_idx << "\n";
                break;
                
            case GLFW_KEY_O:
                cout << "switch to Orthogonal projection\n";
                setOrthogonal();
                break;
            
            case GLFW_KEY_P:
                cout << "switch to NDC Perspective projection\n";
                setPerspective();
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
            
            case GLFW_KEY_E:
                cout << "switch to translate eye position mode\n";
                cur_trans_mode = ViewEye;
                break;
                
            case GLFW_KEY_C:
                cout << "switch to translate viewing center position mode\n";
                cur_trans_mode = ViewCenter;
                break;
                
            case GLFW_KEY_U:
                cout << "switch to translate camera up vector position mode\n";
                cur_trans_mode = ViewUp;
                break;
            
            case GLFW_KEY_I:
                cout << "Information:\n";
                cout << "(1) Translation Matrix:\n" << translate(models[cur_idx].position) << "\n";
                cout << "(2) Rotation Matrix\n" << rotate(models[cur_idx].rotation) << "\n";
                cout << "(3) Scaling Matrix\n" << scaling(models[cur_idx].scale) << "\n";
                cout << "(4) Viewing Matrix\n" << view_matrix << "\n";
                cout << "(5) Projection Matrix\n" << project_matrix << "\n";
                break;
                
            default:
                break;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
    // Apply change on Z axis when scroll the wheel
    // printf("xoff = %lf, yoff = %lf\n", xoffset, yoffset);
    // xoffset is always zero???
    GLfloat scaling_factor = 100.0;
    GLfloat diff = yoffset / scaling_factor;
    switch (cur_trans_mode) {
        case GeoTranslation:
            cout << "(GeoTranslation) position.z: " << models[cur_idx].position.z;
            models[cur_idx].position.z += diff;
            cout << " -> " << models[cur_idx].position.z << "\n";
            break;
            
        case GeoRotation:
            cout << "(GeoTranslation) rotation.z: " << models[cur_idx].rotation.z;
            models[cur_idx].rotation.z += yoffset;
            cout << " -> " << models[cur_idx].rotation.z << "\n";
            break;
            
        case GeoScaling:
            cout << "(GeoTranslation) scale.z: " << models[cur_idx].scale.z;
            models[cur_idx].scale.z += diff;
            cout << " -> " << models[cur_idx].scale.z << "\n";
            break;
        
        case ViewEye:
            cout << "(ViewEye) main_camera.position.z: " << main_camera.position.z;
            main_camera.position.z += diff;
            cout << " -> " << main_camera.position.z << "\n";
            setViewingMatrix();
            break;
        
        case ViewCenter:
            cout << "(ViewCenter) main_camera.center.z: " << main_camera.center.z;
            main_camera.center.z += diff;
            cout << " -> " << main_camera.center.z << "\n";
            setViewingMatrix();
            break;
            
        case ViewUp:
            cout << "(ViewUp) main_camera.up_vector.z: " << main_camera.up_vector.z;
            main_camera.up_vector.z += diff;
            cout << " -> " << main_camera.up_vector.z << "\n";
            setViewingMatrix();
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
            models[cur_idx].rotation.x += dx * 0.5;
            models[cur_idx].rotation.y += dx * 0.5;
            break;
        
        case ViewEye:
            main_camera.position.x += dx / scaling_factor;
            main_camera.position.y += dy / scaling_factor;
            setViewingMatrix();
            break;
            
        case ViewCenter:
            main_camera.center.x += dx / scaling_factor;
            main_camera.center.y += dy / scaling_factor;
            setViewingMatrix();
            break;
        
        case ViewUp:
            main_camera.up_vector.x += dx / scaling_factor;
            main_camera.up_vector.y += dy / scaling_factor;
            setViewingMatrix();
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
	GLuint v, f, p;
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

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, tinyobj::shape_t* shape)
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
		attrib->vertices.at(i) = attrib->vertices.at(i)/ scale;
	}
	size_t index_offset = 0;
	vertices.reserve(shape->mesh.num_face_vertices.size() * 3);
	colors.reserve(shape->mesh.num_face_vertices.size() * 3);
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
		}
		index_offset += fv;
	}
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;

	string err;
	string warn;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Maerial size %d\n", shapes.size(), materials.size());
	
	normalization(&attrib, vertices, colors, &shapes[0]);

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

	m_shape_list.push_back(tmp_shape);
	model tmp_model;
	models.push_back(tmp_model);


	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	shapes.clear();
	materials.clear();
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
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../ColorModels/bunny5KC.obj", "../ColorModels/dragon10KC.obj", "../ColorModels/lucy25KC.obj", "../ColorModels/teapot4KC.obj", "../ColorModels/dolphinC.obj"};
	// [TODO] Load five model at here
    for (int i = 0; i < model_list.size(); ++i) {
        cout << "Loading model #" << i << "\n";
        LoadModels(model_list[i]);
    }
    // LoadModels(model_list[cur_idx]);
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
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "111061553 HW1", NULL, NULL);
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
