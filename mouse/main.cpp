#include <OpenGP/GL/Application.h>

using namespace OpenGP;

const int width=720, height=720;
#define POINTSIZE 10.0f

const char* line_vshader =
#include "line_vshader.glsl"
;
const char* line_fshader =
#include "line_fshader.glsl"
;

void init();
std::unique_ptr<Shader> lineShader;
std::unique_ptr<GPUMesh> line;
std::vector<Vec2> controlPoints;


int main(int, char**){

    Application app;
    init();

    // Mouse cursor position and selected point
    Vec2 position = Vec2(0,0);
    Vec2 *selection = nullptr;

    // Display callback
    Window& window = app.create_window([&](Window&){
        glViewport(0,0,2*width,2*height);
        glClear(GL_COLOR_BUFFER_BIT);
        glPointSize(POINTSIZE); // change default point size (make them bigger)

        lineShader->bind();

        // Draw line red
        lineShader->set_uniform("selection", -1); // highlight the selected vertex blue
        line->set_attributes(*lineShader);
        line->set_mode(GL_LINE_STRIP); // feed in points, and then it makes a line with the points
        line->draw();

        // Draw points red and selected point blue
        if(selection!=nullptr) lineShader->set_uniform("selection", int(selection-&controlPoints[0]));
        line->set_mode(GL_POINTS); // will draw points at your vertices
        line->draw();

        lineShader->unbind();
    });
    window.set_title("Mouse");
    window.set_size(width, height);

    // TWO TYPES OF CALLBACKS

    // Mouse movement callback
    window.add_listener<MouseMoveEvent>([&](const MouseMoveEvent &m){
        // find Mouse position in clip coordinates (actual pixel coordinate)
        Vec2 p = 2.0f*(Vec2(m.position.x()/width,-m.position.y()/height) - Vec2(0.5f,-0.5f));
        if( selection && (p-position).norm() > 0.0f) {
            /// TODO: Make selected control points move with cursor
            selection->x() = position.x();
            selection->y() = position.y();
            //selection = nullptr;
            line->set_vbo<Vec2>("vposition", controlPoints); // change the vertex
        }
        position = p;
    });

    // Mouse click callback (whenever right or left button is pressed)
    window.add_listener<MouseButtonEvent>([&](const MouseButtonEvent &e){
        // Mouse selection case
        if( e.button == GLFW_MOUSE_BUTTON_LEFT && !e.released) {
            selection = nullptr;
            for(auto&& v : controlPoints) { // iterate over control points in our list
                if ( (v-position).norm() < POINTSIZE/std::min(width,height) ) { // convert POINTSIZE pixel to coordinate
                    selection = &v; // select a given point
                    break;
                }
            }
        }
        // Mouse release case
        if( e.button == GLFW_MOUSE_BUTTON_LEFT && e.released) {
            if(selection) { // if we have something selected, set location of mouse(x,y)
                selection->x() = position.x();
                selection->y() = position.y();
                selection = nullptr;
                line->set_vbo<Vec2>("vposition", controlPoints); // change the vertex
            }
        }
    });

    return app.run();
}

void init(){
    glClearColor(1,1,1, /*solid*/1.0 );

    lineShader = std::unique_ptr<Shader>(new Shader());
    lineShader->verbose = true;
    lineShader->add_vshader_from_source(line_vshader);
    lineShader->add_fshader_from_source(line_fshader);
    lineShader->link();

    controlPoints = std::vector<Vec2>();
    controlPoints.push_back(Vec2(-0.7f,-0.2f));
    controlPoints.push_back(Vec2(-0.3f, 0.2f));
    controlPoints.push_back(Vec2( 0.3f, 0.5f));
    controlPoints.push_back(Vec2( 0.7f, 0.0f));

    line = std::unique_ptr<GPUMesh>(new GPUMesh());
    line->set_vbo<Vec2>("vposition", controlPoints);
    std::vector<unsigned int> indices = {0,1,2,3};
    line->set_triangles(indices);
}
