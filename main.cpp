#include "Angel.h"
#include <stdio.h>

const int NumTimesToSubdivide = 6;
const int NumTriangles        = 16384;  // (4 faces)^(NumTimesToSubdivide + 1)
const int NumVertices         = 3 * NumTriangles;

const int NumSpheres = 6;

typedef Angel::vec4 point4;
typedef Angel::vec4 color4;

point4 points[6][NumVertices];
vec3   normals[6][NumVertices];

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;
GLuint TransformLoc;

// Light properties uniform locations
GLuint AmbientProductLoc;
GLuint DiffuseProductLoc;
GLuint SpecularProductLoc;
GLuint LightPositionLoc;
GLuint ShininessLoc;

GLuint ShadingTypeLoc;
GLuint vao[6];

// Light property parameters
point4 light_position;
color4 light_ambient;
color4 light_diffuse;
color4 light_specular;
// Material property parameters
color4 material_ambient;
color4 material_diffuse;
color4 material_specular;
float  material_shininess;

// Inital position of camera
float init_angle = 30;
mat4 initial_model_view = RotateX(init_angle) * Translate(0.0, -80.0, -160.0);
mat4 model_view;

// Initial planet positions
const mat4 p2trans = Translate(40.0, 0.0, 0.0);
const mat4 p1 = Translate(20.0, 0.0, 0.0) * Scale(1.5, 1.5, 1.5);
const mat4 p2 = p2trans * Scale(2, 2, 2);
const mat4 p3 = Translate(60.0, 0.0, 0.0) * Scale(3.6, 3.6, 3.6);
const mat4 p4 = Translate(90.0, 0.0, 0.0) * Scale(5.0, 5.0, 5.0);
const mat4 m1 = Translate(35.0, 0.0, 0.0) * Scale(0.6, 0.6, 0.6);

// Transform matrices
mat4 transform;
mat4 center;
mat4 planetOne;
mat4 planetTwo;
mat4 planetThree;
mat4 planetFour;
mat4 planetMoon;

// Planet rotation angles
float thetaOne;
float thetaTwo;
float thetaThree;
float thetaFour;
float thetaMoon;

// Camera rotations
float altitudeAngle = -init_angle;
float azimuthAngle = 0;

inline
mat4 transpose2( const mat4& A ) {
    return mat4( A[0][0], A[0][1], A[0][2], A[0][3],
		 A[1][0], A[1][1], A[1][2], A[1][3],
		 A[2][0], A[2][1], A[2][2], A[2][3],
		 A[3][0], A[3][1], A[3][2], A[3][3] );
}

//----------------------------------------------------------------------------

int Index = 0;

void
triangle( const point4& a, const point4& b, const point4& c, int objIndex, bool flat )
{
    vec3  normal = normalize( cross(b - a, c - b) );

	// Argument flat determines flat shading or Phong shading
    normals[objIndex][Index] = flat ? normal : normalize(vec3(a.x, a.y, a.z));
	points[objIndex][Index] = a;  Index++;
    normals[objIndex][Index] = flat ? normal : normalize(vec3(b.x, b.y, b.z));
	points[objIndex][Index] = b;  Index++;
    normals[objIndex][Index] = flat ? normal : normalize(vec3(c.x, c.y, c.z));
	points[objIndex][Index] = c;  Index++;
}

//----------------------------------------------------------------------------

point4
unit( const point4& p )
{
    float len = p.x*p.x + p.y*p.y + p.z*p.z;
    
    point4 t;
    if ( len > DivideByZeroTolerance ) {
	t = p / sqrt(len);
	t.w = 1.0;
    }

    return t;
}

void
divide_triangle( const point4& a, const point4& b,
		 const point4& c, int count, int objIndex, bool flat )
{
    if ( count > 0 ) {
        point4 v1 = unit( a + b );
        point4 v2 = unit( a + c );
        point4 v3 = unit( b + c );
        divide_triangle(  a, v1, v2, count - 1, objIndex, flat );
        divide_triangle(  c, v2, v3, count - 1, objIndex, flat );
        divide_triangle(  b, v3, v1, count - 1, objIndex, flat );
        divide_triangle( v1, v3, v2, count - 1, objIndex, flat );
    }
    else {
        triangle( a, b, c, objIndex, flat );
    }
}

void
tetrahedron( int count, int objIndex, bool flat )
{
	// Initial tetrahedron points
    point4 v[4] = {
	vec4( 0.0, 0.0, 1.0, 1.0 ),
	vec4( 0.0, 0.942809, -0.333333, 1.0 ),
	vec4( -0.816497, -0.471405, -0.333333, 1.0 ),
	vec4( 0.816497, -0.471405, -0.333333, 1.0 )
    };

    divide_triangle( v[0], v[1], v[2], count, objIndex, flat );
    divide_triangle( v[3], v[2], v[1], count, objIndex, flat );
    divide_triangle( v[0], v[3], v[1], count, objIndex, flat );
    divide_triangle( v[0], v[2], v[3], count, objIndex, flat );
}

//----------------------------------------------------------------------------

// Transform all objects with the same matrix
void transformObjects(mat4 trans, bool post) {
	if (post) {	// Post multiplication
		center = center * trans;
		planetOne = planetOne * trans;
		planetTwo = planetTwo * trans;
		planetThree = planetThree * trans;
		planetFour = planetFour * trans;
		planetMoon = planetMoon * trans;
	} else { // Pre multiplication
		center = trans * center;
		planetOne = trans * planetOne;
		planetTwo = trans * planetTwo;
		planetThree = trans * planetThree;
		planetFour = trans * planetFour;
		planetMoon = trans * planetMoon;
	}
}

float reduceAngle(float angle) {
	if (angle > 360) {
		return angle - 360;
	} else if (angle < -360) {
		return angle + 360;
	}
	return angle;
}

// OpenGL initialization
void
init()
{
    // Subdivide a tetrahedron into a sphere (centered at origin)
	Index = 0;
    tetrahedron( 5, 0, false );
	Index = 0;
	tetrahedron( 3, 1, true );
	Index = 0;
	tetrahedron( 4, 2, false );
	Index = 0;
	tetrahedron( 6, 3, false );
	Index = 0;
	tetrahedron( 5, 4, false );
	Index = 0;
	tetrahedron( 3, 5, false );

    // Generate vertex array objects
    glGenVertexArrays( 6, vao );

	// Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
    glUseProgram( program );

	// 6 objects, 1 sun, 4 planets, 1 moon
	for (int k = 0; k < 6; k++) {
		glBindVertexArray( vao[k] );
		// Create and initialize a buffer object
		GLuint buffer;
		glGenBuffers( 1, &buffer );
		glBindBuffer( GL_ARRAY_BUFFER, buffer );
		glBufferData( GL_ARRAY_BUFFER, sizeof(points[k]) + sizeof(normals[k]),
			  NULL, GL_STATIC_DRAW );
		glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(points[k]), points[k] );
		glBufferSubData( GL_ARRAY_BUFFER, sizeof(points[k]),
				 sizeof(normals[k]), normals[k] );
	
		// set up vertex arrays
		GLuint vPosition = glGetAttribLocation( program, "vPosition" );
		glEnableVertexAttribArray( vPosition );
		glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0,
				   BUFFER_OFFSET(0) );

		GLuint vNormal = glGetAttribLocation( program, "vNormal" ); 
		glEnableVertexAttribArray( vNormal );
		glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
				   BUFFER_OFFSET(sizeof(points[k])) );
	}

    // Initialize shader lighting parameters
    light_position = point4( 0.0, 0.0, 0.0, 0.0 );
    light_ambient = color4( 0.2, 0.2, 0.2, 1.0 );
    light_diffuse = color4( 1.0, 1.0, 1.0, 1.0 );
    light_specular = color4( 1.0, 1.0, 1.0, 1.0 );

    material_ambient = color4( 1.0, 0.0, 1.0, 1.0 );
    material_diffuse = color4( 1.0, 0.8, 0.0, 1.0 );
    material_specular = color4( 1.0, 0.0, 1.0, 1.0 );
    material_shininess = 10.0;

    color4 ambient_product = light_ambient * material_ambient;
    color4 diffuse_product = light_diffuse * material_diffuse;
    color4 specular_product = light_specular * material_specular;

	AmbientProductLoc = glGetUniformLocation(program, "AmbientProduct");
    glUniform4fv( AmbientProductLoc, 1, ambient_product );
	DiffuseProductLoc = glGetUniformLocation(program, "DiffuseProduct");
    glUniform4fv( DiffuseProductLoc, 1, diffuse_product );
	SpecularProductLoc = glGetUniformLocation(program, "SpecularProduct");
    glUniform4fv( SpecularProductLoc, 1, specular_product );
	
	LightPositionLoc = glGetUniformLocation(program, "LightPosition");
    glUniform4fv( LightPositionLoc, 1, light_position );

	ShininessLoc = glGetUniformLocation(program, "Shininess");
    glUniform1f( ShininessLoc, material_shininess );
		 
    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation( program, "ModelView" );
    Projection = glGetUniformLocation( program, "Projection" );

	// Retreive transform matrix uniform locations
	TransformLoc = glGetUniformLocation(program, "Transform");

	// Flat, Gouraund, Phong
	ShadingTypeLoc = glGetUniformLocation(program, "shadingType");

	// Init model-view matrix
	model_view = initial_model_view;
	std::cout << model_view << std::endl;
	std::cout << model_view[1][3] << std::endl;
	std::cout << model_view[2][3] << std::endl;

	// Init angles
	thetaOne = 0;
	thetaTwo = 0;
	thetaThree = 0;
	thetaFour = 0;
	thetaMoon = 0;

	// Init Planet locations, relative to the Sun
	center = Scale(8.0, 8.0, 8.0);
	planetOne = p1;
    planetTwo = p2;
	planetThree = p3;
	planetFour = p4;
	planetMoon = m1;

    glEnable( GL_DEPTH_TEST );
    
    glClearColor( 0.0, 0.0, 0.0, 1.0 ); /* black background */
}

//----------------------------------------------------------------------------

// Called when the window needs to be redrawn.
// TODO: Don't forget to add glutPostRedisplay().
void callbackDisplay()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view );

	glBindVertexArray( vao[0] );

	// Draw the Sun
	light_ambient = color4( 0.8, 0.1, 0.1, 1.0 );
    light_diffuse = color4( 0.8, 0.2, 0.2, 1.0 );
    light_specular = color4( 0.4, 0.1, 0.1, 1.0 );
    material_ambient = color4( 0.8, 0.5, 0.5, 1.0 );
    material_diffuse = color4( 0.8, 0.2, 0.2, 1.0 );
    material_specular = color4( 1.0, 1.0, 1.0, 1.0 );
    material_shininess = 1.0;
	glUniform4fv( AmbientProductLoc, 1, light_ambient * material_ambient );
	glUniform4fv( DiffuseProductLoc, 1, light_diffuse * material_diffuse );
	glUniform4fv( SpecularProductLoc, 1, light_specular * material_specular );
	glUniform1f( ShininessLoc, material_shininess );
	glUniform4fv( LightPositionLoc, 1, light_position );
	float shading_type = 2;
	glUniform1f(ShadingTypeLoc, shading_type);
	glUniformMatrix4fv(TransformLoc, 1, GL_TRUE, center);
    glDrawArrays( GL_TRIANGLES, 0, NumVertices );

	light_position = point4( 0.0, 0.0, 0.0, 0.0 );
	glUniform4fv( LightPositionLoc, 1, light_position );

	glBindVertexArray( vao[1] );
	// Draw planet One
	light_ambient = color4( 0.2, 0.2, 0.2, 1.0 );
    light_diffuse = color4( 1.0, 1.0, 1.0, 1.0 );
    light_specular = color4( 1.0, 1.0, 1.0, 1.0 );
    material_ambient = color4( 1.0, 1.0, 1.0, 1.0 );
    material_diffuse = color4( 0.8, 0.8, 0.8, 1.0 );
    material_specular = color4( 1.0, 1.0, 1.0, 1.0 );
    material_shininess = 20.0;
	shading_type = 1;
	glUniform4fv( AmbientProductLoc, 1, light_ambient * material_ambient );
	glUniform4fv( DiffuseProductLoc, 1, light_diffuse * material_diffuse );
	glUniform4fv( SpecularProductLoc, 1, light_specular * material_specular );
	glUniform1f( ShininessLoc, material_shininess );
	glUniformMatrix4fv(TransformLoc, 1, GL_TRUE, planetOne);
	glDrawArrays( GL_TRIANGLES, 0, NumVertices);

	glBindVertexArray( vao[2] );
	// Draw planet Two
	light_ambient = color4( 0.2, 0.2, 0.2, 1.0 );
    light_diffuse = color4( 1.0, 1.0, 1.0, 1.0 );
    light_specular = color4( 1.0, 1.0, 1.0, 1.0 );
    material_ambient = color4( 0.5, 0.8, 1.0, 1.0 );
    material_diffuse = color4( 0.5, 0.8, 0.0, 1.0 );
    material_specular = color4( 0.5, 0.8, 0.0, 1.0 );
    material_shininess = 10.0;
	shading_type = 1;
	glUniform1f(ShadingTypeLoc, shading_type);
	glUniform4fv( AmbientProductLoc, 1, light_ambient * material_ambient );
	glUniform4fv( DiffuseProductLoc, 1, light_diffuse * material_diffuse );
	glUniform4fv( SpecularProductLoc, 1, light_specular * material_specular );
	glUniform1f( ShininessLoc, material_shininess );
	glUniformMatrix4fv(TransformLoc, 1, GL_TRUE, planetTwo);
	glDrawArrays( GL_TRIANGLES, 0, NumVertices);

	glBindVertexArray( vao[3] );
	// Draw planet Three
	light_ambient = color4( 0.2, 0.2, 0.2, 1.0 );
    light_diffuse = color4( 1.0, 1.0, 1.0, 1.0 );
    light_specular = color4( 1.0, 1.0, 1.0, 1.0 );
    material_ambient = color4( 0.5, 0.5, 1.0, 1.0 );
    material_diffuse = color4( 0.5, 0.5, 1.0, 1.0 );
    material_specular = color4( 0.5, 0.5, 1.0, 1.0 );
    material_shininess = 10.0;
	shading_type = 2;
	glUniform1f(ShadingTypeLoc, shading_type);
	glUniform4fv( AmbientProductLoc, 1, light_ambient * material_ambient );
	glUniform4fv( DiffuseProductLoc, 1, light_diffuse * material_diffuse );
	glUniform4fv( SpecularProductLoc, 1, light_specular * material_specular );
	glUniform1f( ShininessLoc, material_shininess );
	glUniformMatrix4fv(TransformLoc, 1, GL_TRUE, planetThree);
	glDrawArrays( GL_TRIANGLES, 0, NumVertices);

	glBindVertexArray( vao[4] );
	// Draw planet Four
	light_ambient = color4( 0.2, 0.2, 0.2, 1.0 );
    light_diffuse = color4( 1.0, 1.0, 1.0, 1.0 );
    light_specular = color4( 0.0, 0.0, 0.0, 1.0 );
    material_ambient = color4( 0.55, 0.27, 0.07, 1.0 );
    material_diffuse = color4( 0.55, 0.27, 0.07, 1.0 );
    material_specular = color4( 0.0, 0.0, 0.0, 1.0 );
    material_shininess = 5.0;
	shading_type = 1;
	glUniform1f(ShadingTypeLoc, shading_type);
	glUniform4fv( AmbientProductLoc, 1, light_ambient * material_ambient );
	glUniform4fv( DiffuseProductLoc, 1, light_diffuse * material_diffuse );
	glUniform4fv( SpecularProductLoc, 1, light_specular * material_specular );
	glUniform1f( ShininessLoc, material_shininess );
	glUniformMatrix4fv(TransformLoc, 1, GL_TRUE, planetFour);
	glDrawArrays( GL_TRIANGLES, 0, NumVertices);

	glBindVertexArray( vao[5] );
	// Draw planet Two's Moon
	light_ambient = color4( 0.2, 0.2, 0.2, 1.0 );
    light_diffuse = color4( 1.0, 1.0, 1.0, 1.0 );
    light_specular = color4( 0.0, 0.0, 0.0, 1.0 );
    material_ambient = color4( 0.33, 0.15, 0.07, 1.0 );
    material_diffuse = color4( 0.55, 0.74, 0.07, 1.0 );
    material_specular = color4( 0.0, 0.0, 0.0, 1.0 );
    material_shininess = 10.0;
	shading_type = 2;
	glUniform1f(ShadingTypeLoc, shading_type);
	glUniform4fv( AmbientProductLoc, 1, light_ambient * material_ambient );
	glUniform4fv( DiffuseProductLoc, 1, light_diffuse * material_diffuse );
	glUniform4fv( SpecularProductLoc, 1, light_specular * material_specular );
	glUniform1f( ShininessLoc, material_shininess );
	glUniformMatrix4fv(TransformLoc, 1, GL_TRUE, planetMoon);
	glDrawArrays( GL_TRIANGLES, 0, NumVertices);

    glutSwapBuffers();
}

// Called when the window is resized.
void callbackReshape(int width, int height)
{
	//glViewport( 0, 0, width, height );

    GLfloat left = -2.0, right = 2.0;
    GLfloat top = 2.0, bottom = -2.0;

    GLfloat aspect = GLfloat(width)/height;

    if ( aspect > 1.0 ) {
	left *= aspect;
	right *= aspect;
    }
    else {
	top /= aspect;
	bottom /= aspect;
    }

    //mat4 projection = Ortho( left, right, bottom, top, zNear, zFar );
	mat4 projection = Perspective(45.0, aspect, 0.5, 500);
    glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
}

// Called for special keyboard inputs
void callbackSpecial(int key, int x, int y) {
	
	// Look up (Rotate camera up)
	if (key == GLUT_KEY_UP) {
		static float u = 1;
		model_view = RotateX(-u) * model_view;
		altitudeAngle = reduceAngle(altitudeAngle + u);
		//altitudeAngle += u;
	}
	
	// Decrease altitude
	if (key == GLUT_KEY_DOWN) {
		static float d = 1;
		model_view = RotateX(d) * model_view;
		altitudeAngle = reduceAngle(altitudeAngle - d);
		//altitudeAngle -= d;
	}
	
	// Head left
	if (key == GLUT_KEY_LEFT) {
		static float l = 1;
		model_view = RotateY(-l) * model_view;
		azimuthAngle = reduceAngle(azimuthAngle + l);
		//azimuthAngle += l;
	}
	// Head right
	if (key == GLUT_KEY_RIGHT) {
		static float r = 1;
		model_view = RotateY(r) * model_view;
		azimuthAngle = reduceAngle(azimuthAngle - r);
		//azimuthAngle -= r;
	}
	
	glutPostRedisplay();
}

// Called when a key is pressed. x, y is the current mouse position.
void callbackKeyboard(unsigned char key, int x, int y)
{
	// Exit application
	if (key == 'q' || key == 'Q') {
		exit(0);
	}
	
	// Translate forward
	if (key == 'w' || key == 'W') {
		//model_view = model_view * Translate(0.0, 0.0, 1.5);
		model_view = Translate(0.0, 0.0, 1.5) * model_view;
	}
	
	// Translate backwards
	if (key == 's' || key == 'S') {
		//model_view = model_view * Translate(0.0, 0.0, -1.5);
		model_view = Translate(0.0, 0.0, -1.5) * model_view;
	}
	// Translate left
	if (key == 'a' || key == 'A') {
		//model_view = model_view * Translate(1.5, 0.0, 0.0);
		model_view = Translate(1.5, 0.0, 0.0) * model_view;
	}
	// Translate right
	if (key == 'd' || key == 'D') {
		//model_view = model_view * Translate(-1.5, 0.0, 0.0);
		model_view = Translate(-1.5, 0.0, 0.0) * model_view;
	}
	// Translate up
	if (key == 'o' || key == 'O') {
		//model_view = model_view * Translate(0.0, -1.5, 0.0);
		model_view = Translate(0.0, -1.5, 0.0) * model_view;
	}
	// Translate down
	if (key == 'l' || key == 'L') {
		//model_view = model_view * Translate(0.0, 1.5, 0.0);
		model_view = Translate(0.0, 1.5, 0.0) * model_view;
	}

	// Reset the view
	if (key == 'r' || key == 'R') {
		model_view = initial_model_view;
		azimuthAngle = 0;
		altitudeAngle = -init_angle;
	}

	glutPostRedisplay();
}

// Called when a mouse button is pressed or released
void callbackMouse(int button, int state, int x, int y)
{

}

// Called when the mouse is moved with a button pressed
void callbackMotion(int x, int y)
{

}

// Called when the mouse is moved with no buttons pressed
void callbackPassiveMotion(int x, int y)
{

}

// Called when the system is idle. Can be called many times per frame.
void callbackIdle()
{
	
	// Rotate planet One
	planetOne = RotateY(thetaOne) * p1;
	thetaOne += 0.25;
	// Rotate planet Two
	planetTwo = RotateY(thetaTwo) * p2;
	thetaTwo += 0.10;
	// Planet Two's moon
	planetMoon = RotateY(thetaTwo) * p2trans * RotateY(thetaMoon) * Translate(5.0, 0, 0);
	thetaMoon += 0.3;
	// Rotate planet Three
	planetThree = RotateY(thetaThree) * p3;
	thetaThree += 0.08;
	// Rotate planet Four
	planetFour = RotateY(thetaFour) * p4;
	thetaFour += 0.05;
	
	vec4 dd = (RotateY(+azimuthAngle) * RotateX(+altitudeAngle) * model_view) * vec4(1.0, 1.0, 1.0, 1.0);
	light_position = point4( -dd.x, -dd.y, -dd.z, 0.0 );

	glutPostRedisplay();
}

// Called when the timer expires
void callbackTimer(int)
{
	glutTimerFunc(1000/30, callbackTimer, 0);
	glutPostRedisplay();
}

void initGlut(int& argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 50);
	glutInitWindowSize(1200, 800);
	glutCreateWindow("Assignment 3");
}

void initCallbacks()
{
	glutDisplayFunc(callbackDisplay);
	glutReshapeFunc(callbackReshape);
	glutKeyboardFunc(callbackKeyboard);
	glutSpecialFunc(callbackSpecial);
	glutMouseFunc(callbackMouse);
	glutMotionFunc(callbackMotion);
	glutPassiveMotionFunc(callbackPassiveMotion);
	glutIdleFunc(callbackIdle);
	glutTimerFunc(1000/30, callbackTimer, 0);
}

int main(int argc, char** argv)
{
	initGlut(argc, argv);
	glewInit();
	initCallbacks();

	glutInitContextVersion( 3, 2 );
	glutInitContextProfile( GLUT_CORE_PROFILE );

	// Generate spheres (solar system)
	init();

	glutMainLoop();
}
