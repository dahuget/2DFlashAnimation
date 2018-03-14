// hello make a change to test commit

#include "OpenGP/GL/Eigen.h"
#include "OpenGP/GL/glfw_helpers.h"
#include "Mesh/Mesh.h"
#include <iostream>
#include <math.h>

using namespace std;
using namespace OpenGP;

typedef Eigen::Transform<float,3,Eigen::Affine> Transform;

const int SUN_ROT_PERIOD = 30;        
const int EARTH_ROT_PERIOD = 4;       
const int MOON_ROT_PERIOD = 8;       
const int EARTH_ORBITAL_PERIOD = 10; 
const int MOON_ORBITAL_PERIOD = 5;   
const int SPEED_FACTOR = 1;
    
Mesh sun;
Mesh earth;
Mesh moon;
Mesh triangle;

void init();
std::vector<Vec2> controlPoints;

// de Casteljau's method
int getPt( float n1 , float n2 , float perc )
{
    float diff = n2 - n1;

    return n1 + ( diff * perc );
}
// de Casteljau's method
/*for( float i = 0 ; i < 1 ; i += 0.01 )
{
    // The Green Line
    float xa = getPt(controlPoints.at(0)(0),controlPoints.at(1)(0), i); //getPt( x1 , x2 , i );
    float ya = getPt(controlPoints.at(0)(1),controlPoints.at(1)(1), i); // ( y1 , y2 , i );
    float xb = getPt(controlPoints.at(1)(0),controlPoints.at(2)(0), i); //getPt( x2 , x3 , i );
    float yb = getPt(controlPoints.at(1)(1),controlPoints.at(2)(1), i); //getPt( y2 , y3 , i );

    // The Black Dot
    float x = getPt( xa , xb , i );
    float y = getPt( ya , yb , i );

    cout << x << " " << y << endl;
    //drawPixel( x , y , COLOR_RED );
}*/


void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    // the timer measures time elapsed since GLFW was initialized.
    float time_s = glfwGetTime();
    float freq = M_PI*time_s*SPEED_FACTOR;
    // print out time
    //cout << freq << endl;
    // display a triangle
    Transform tri_M = Transform::Identity();
    tri_M *= Eigen::AlignedScaling3f(0.15, 0.15, 1.0);
    tri_M *= Eigen::Translation3f(-4.5, 0.0, 0.0);

    // how much do i want to move the triangle based on timestamp -> spatial movement
    float x_axis = -4.5+time_s;
    //tri_M *= Eigen::Translation3f(x_axis, 0.0, 0.0);
    //cout << scale_t << endl;

    // move on a straight line (change x-coordinate of triangle)
    tri_M *= Eigen::Translation3f(-4.5+time_s, 0.0, 0.0);

    // create a polyline on a bezier curve (statically, 4 control points)
    // print out bezier points, then move triangle along the curve
    controlPoints = std::vector<Vec2>();
    controlPoints.push_back(Vec2(-8.0f,-7.0f));
    controlPoints.push_back(Vec2(-7.0f, -3.0f));
    controlPoints.push_back(Vec2( -1.0f, 7.0f));
    controlPoints.push_back(Vec2( 5.0f, 4.0f));
    //Vec2 vv = Vec2(0.0, 1.0);
    //cout << controlPoints.at(0)(0) << endl;

    // bezier's p = (1-t)^2 *P0 + 2*(1-t)*t*P1 + t*t*P2
    // p = (1-t)^3 *P0 + 3*t*(1-t)^2*P1 + 3*t^2*(1-t)*P2 + t^3*P3
    // t is usually on 0-1. P0, P1, etc are the control points.
    Vec2 P0 = controlPoints.at(0);
    Vec2 P1 = controlPoints.at(1);
    Vec2 P2 = controlPoints.at(2);
    Vec2 P3 = controlPoints.at(3);
    Transform moon_M = Transform::Identity();
    //make the picture of moon smaller
    moon_M *= Eigen::AlignedScaling3f(0.08, 0.08, 1.0);
    moon_M *= Eigen::Translation3f(P3(0), P3(1), 0.0);
    float scale_t = time_s*0.1; //0.01*std::sin(freq); // normalized percentage [0,1)
    //cout << time_s << endl;
    float x = pow((1-scale_t), 3) *P0(0) + 3*scale_t*pow((1-scale_t),2)*P1(0) + 3*pow(scale_t,2)*(1-scale_t)*P2(0) + pow(scale_t,3)*P3(0);
    float y = pow((1-scale_t), 3) *P0(1) + 3*scale_t*pow((1-scale_t),2)*P1(1) + 3*pow(scale_t,2)*(1-scale_t)*P2(1) + pow(scale_t,3)*P3(1);
    //cout << x << " " << y << endl;
    moon_M *= Eigen::Translation3f(x, y, 0.0);

    //TODO: Set up the transform hierarchies for the three objects!  

    // **** Sun transform
    Transform sun_M = Transform::Identity();
    sun_M *= Eigen::Translation3f(0.2, 0.0, 0.0);
    sun_M *= Eigen::AngleAxisf(-freq/SUN_ROT_PERIOD, Eigen::Vector3f::UnitZ());
    //scale_t: make the sun become bigger and smaller over the time!
    scale_t = 0.01*std::sin(freq); // normalized percentage [0,1)
    sun_M *= Eigen::AlignedScaling3f(0.2 +scale_t, 0.2 +scale_t, 1.0);

    // **** Earth transform
    Transform earth_M = Transform::Identity();
    //calculate the earth's orbit as an ellipse around the sun
    float x_earth_orbit = 0.7*std::cos(-freq/EARTH_ORBITAL_PERIOD);
    float y_earth_orbit = 0.5*std::sin(-freq/EARTH_ORBITAL_PERIOD);
    earth_M *= Eigen::Translation3f(x_earth_orbit, y_earth_orbit, 0.0);
    //save the earth's transform before spinning, so we don't spin the moon
    //with the earth!
    Transform earth_M_prespin = earth_M;
    earth_M *= Eigen::AngleAxisf(-freq/EARTH_ROT_PERIOD, Eigen::Vector3f::UnitZ());
    //make the picture of earth smaller
    earth_M *= Eigen::AlignedScaling3f(0.08, 0.08, 1.0);

    // **** Moon transform
    /*Transform moon_M = earth_M_prespin;
    // Make the moon orbit around the earth with 0.2 units of distance
    moon_M *= Eigen::AngleAxisf(freq/MOON_ORBITAL_PERIOD, Eigen::Vector3f::UnitZ());
    moon_M *= Eigen::Translation3f(0.2, 0.0, 0.0);
    // Make the moon spining according to MOON_ROT_PERIOD
    moon_M *= Eigen::AngleAxisf(-freq/MOON_ROT_PERIOD, -Eigen::Vector3f::UnitZ());
    // Make the picture of moon smaller!
    moon_M *= Eigen::AlignedScaling3f(0.04, 0.04, 1.0);*/

    // draw the sun, the earth and the moon
    sun.draw(sun_M.matrix());
    earth.draw(earth_M.matrix());
    moon.draw(moon_M.matrix());
    triangle.draw(tri_M.matrix());
}

int main(int, char**){
    glfwInitWindowSize(512, 512);
    glfwMakeWindow("Planets");
    glfwDisplayFunc(display);
    // sets up scene objects
    init();
    // loops through, creates new frames
    glfwMainLoop();
    return EXIT_SUCCESS;
}

// sets up scene
void init(){
    glClearColor(1.0f,1.0f,1.0f, 1.0 );

    // Enable alpha blending so texture backgroudns remain transparent
    glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    sun.init();
    earth.init();
    moon.init();
    triangle.init();

    std::vector<OpenGP::Vec3> quadVert;
    quadVert.push_back(OpenGP::Vec3(-1.0f, -1.0f, 0.0f));
    quadVert.push_back(OpenGP::Vec3(1.0f, -1.0f, 0.0f));
    quadVert.push_back(OpenGP::Vec3(1.0f, 1.0f, 0.0f));
    quadVert.push_back(OpenGP::Vec3(-1.0f, 1.0f, 0.0f));
    std::vector<unsigned> quadInd;
    quadInd.push_back(0);
    quadInd.push_back(1);
    quadInd.push_back(2);
    quadInd.push_back(0);
    quadInd.push_back(2);
    quadInd.push_back(3);
    sun.loadVertices(quadVert, quadInd);
    earth.loadVertices(quadVert, quadInd);
    moon.loadVertices(quadVert, quadInd);
    triangle.loadVertices(quadVert, quadInd);

    std::vector<OpenGP::Vec2> quadTCoord;
    quadTCoord.push_back(OpenGP::Vec2(0.0f, 0.0f));
    quadTCoord.push_back(OpenGP::Vec2(1.0f, 0.0f));
    quadTCoord.push_back(OpenGP::Vec2(1.0f, 1.0f));
    quadTCoord.push_back(OpenGP::Vec2(0.0f, 1.0f));
    sun.loadTexCoords(quadTCoord);
    earth.loadTexCoords(quadTCoord);
    moon.loadTexCoords(quadTCoord);
    triangle.loadTexCoords(quadTCoord);

    sun.loadTextures("sun.png");
    moon.loadTextures("moon.png");
    earth.loadTextures("earth.png");
    triangle.loadTextures("triangle2.png");
}
