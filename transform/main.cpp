// hello make a change to test commit

#include "OpenGP/GL/Eigen.h"
#include "OpenGP/GL/glfw_helpers.h"
#include "Mesh/Mesh.h"

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

void init();

void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    // the timer measures time elapsed since GLFW was initialized.
    float time_s = glfwGetTime();
    // print out time

    // display a triangle

    // how much do i want to move the triangle based on timestamp -> spatial movement

    // move on a straight line (change x-coordinate of triangle)

    // create a polyline on a bezier curve (statically, 4 control points)
    // print out bezier points, then move triangle along the curve

    //TODO: Set up the transform hierarchies for the three objects!
    float freq = M_PI*time_s*SPEED_FACTOR;

    // **** Sun transform
    Transform sun_M = Transform::Identity();
    sun_M *= Eigen::Translation3f(0.2, 0.0, 0.0);
    sun_M *= Eigen::AngleAxisf(-freq/SUN_ROT_PERIOD, Eigen::Vector3f::UnitZ());
    //scale_t: make the sun become bigger and smaller over the time!
    float scale_t = 0.01*std::sin(freq); // normalized percentage [0,1)
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
    Transform moon_M = earth_M_prespin;
    // Make the moon orbit around the earth with 0.2 units of distance
    moon_M *= Eigen::AngleAxisf(freq/MOON_ORBITAL_PERIOD, Eigen::Vector3f::UnitZ());
    moon_M *= Eigen::Translation3f(0.2, 0.0, 0.0);
    // Make the moon spining according to MOON_ROT_PERIOD
    moon_M *= Eigen::AngleAxisf(-freq/MOON_ROT_PERIOD, -Eigen::Vector3f::UnitZ());
    // Make the picture of moon smaller!
    moon_M *= Eigen::AlignedScaling3f(0.04, 0.04, 1.0);

    // draw the sun, the earth and the moon
    sun.draw(sun_M.matrix());
    earth.draw(earth_M.matrix());
    moon.draw(moon_M.matrix());
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

    std::vector<OpenGP::Vec2> quadTCoord;
    quadTCoord.push_back(OpenGP::Vec2(0.0f, 0.0f));
    quadTCoord.push_back(OpenGP::Vec2(1.0f, 0.0f));
    quadTCoord.push_back(OpenGP::Vec2(1.0f, 1.0f));
    quadTCoord.push_back(OpenGP::Vec2(0.0f, 1.0f));
    sun.loadTexCoords(quadTCoord);
    earth.loadTexCoords(quadTCoord);
    moon.loadTexCoords(quadTCoord);

    sun.loadTextures("sun.png");
    moon.loadTextures("moon.png");
    earth.loadTextures("earth.png");
}
