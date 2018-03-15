#include <OpenGP/GL/Application.h>
#include <OpenGP/external/LodePNG/lodepng.cpp>

using namespace OpenGP;

const int width=720, height=720;
#define POINTSIZE 10.0f
#define PRECISION 0.1
typedef Eigen::Transform<float,3,Eigen::Affine> Transform;

const char* fb_vshader =
#include "fb_vshader.glsl"
;
const char* fb_fshader =
#include "fb_fshader.glsl"
;
const char* quad_vshader =
#include "quad_vshader.glsl"
;
const char* quad_fshader =
#include "quad_fshader.glsl"
;
const char* line_vshader =
#include "line_vshader.glsl"
;
const char* line_fshader =
#include "line_fshader.glsl"
;
const char* selection_vshader =
#include "selection_vshader.glsl"
;
const char* selection_fshader =
#include "selection_fshader.glsl"
;

const float SpeedFactor = 1;
void init();
void deCasteljau(Vec2 &bezierPt, Vec2 P0, Vec2 P1, Vec2 P2, Vec2 P3, float t);
void getPt(Vec2 &point, Vec2 P1, Vec2 P2, float t);
void quadInit(std::unique_ptr<GPUMesh> &quad);
void loadTexture(std::unique_ptr<RGBA8Texture> &texture, const char* filename);
void drawScene(float timeCount);
void drawBackground(Transform TRS);
void drawFigure(Transform TRS, std::unique_ptr<RGBA8Texture> &tex);

std::unique_ptr<GPUMesh> quad;

std::unique_ptr<Shader> quadShader;
std::unique_ptr<Shader> fbShader;

std::unique_ptr<Shader> lineShader;
std::unique_ptr<GPUMesh> line;
std::vector<Vec2> controlPoints;
std::unique_ptr<GPUMesh> bezierLine;
std::vector<Vec2> controlBPoints;

/// Selection with framebuffer pointers
std::unique_ptr<Shader> selectionShader;
std::unique_ptr<Framebuffer> selectionFB;
std::unique_ptr<RGBA8Texture> selectionColor;
std::unique_ptr<D16Texture> selectionDepth;

std::unique_ptr<RGBA8Texture> moto;
std::unique_ptr<RGBA8Texture> wheel1;
std::unique_ptr<RGBA8Texture> wheel2;
std::unique_ptr<RGBA8Texture> tail;
std::unique_ptr<RGBA8Texture> saltflats;

/// TODO: declare Framebuffer and color buffer texture
std::unique_ptr<Framebuffer> fb;
std::unique_ptr<RGBA8Texture> c_buf;


int main(int, char**){

    Application app;
    init();

    /// TODO: initialize framebuffer
    fb = std::unique_ptr<Framebuffer>(new Framebuffer());
    /// TODO: initialize color buffer texture, and allocate memory
    c_buf = std::unique_ptr<RGBA8Texture>(new RGBA8Texture());
    c_buf->allocate(2*width, 2*height);
    /// TODO: attach color texture to framebuffer
    fb->attach_color_texture(*c_buf); //de-reference pointer to texture

    /// Selection shader
    selectionShader = std::unique_ptr<Shader>(new Shader());
    selectionShader->verbose = true;
    selectionShader->add_vshader_from_source(selection_vshader);
    selectionShader->add_fshader_from_source(selection_fshader);
    selectionShader->link();
    /// Framebuffer for selection shader
    selectionFB = std::unique_ptr<Framebuffer>(new Framebuffer());
    selectionColor = std::unique_ptr<RGBA8Texture>(new RGBA8Texture());
    selectionColor->allocate(width,height);
    selectionDepth = std::unique_ptr<D16Texture>(new D16Texture());
    selectionDepth->allocate(width,height);
    selectionFB->attach_color_texture(*selectionColor);
    selectionFB->attach_depth_texture(*selectionDepth);

    // Mouse position and selected point
    Vec2 pixelPosition = Vec2(0,0);
    Vec2 position = Vec2(0,0);
    Vec2 *selection = nullptr;
    int offsetID = 0;

    Window& window = app.create_window([&](Window&){
        glViewport(0,0,2*width,2*height);
        /// TODO: First draw the scene onto framebuffer
        /// bind and then unbind framebuffer
        fb->bind();
            glClear(GL_COLOR_BUFFER_BIT);
            drawScene(glfwGetTime());
        fb->unbind(); // sets to display ie main screen
        /// Render to Window, uncomment the lines and do TODOs
        glViewport(0, 0, 2*width, 2*height);
        glClear(GL_COLOR_BUFFER_BIT);
        fbShader->bind();
        /// TODO: Bind texture and set uniforms
        glActiveTexture(GL_TEXTURE0);
        c_buf->bind();
        fbShader->set_uniform("tex", 0);
        fbShader->set_uniform("tex_width", float(width));
        fbShader->set_uniform("tex_height", float(height));
        quad->set_attributes(*fbShader);
        quad->draw();
        /// Also unbind the texture
        c_buf->unbind();
        fbShader->unbind();

        glViewport(0,0,2*width,2*height);
        glClear(GL_COLOR_BUFFER_BIT);
        glPointSize(POINTSIZE);

        lineShader->bind();

        // Draw line red
        lineShader->set_uniform("selection", -1);
        line->set_attributes(*lineShader);
        line->set_mode(GL_LINE_STRIP); //polyline
        line->draw();

        lineShader->set_uniform("selection", -1);
        bezierLine->set_attributes(*lineShader);
        bezierLine->set_mode(GL_LINE_STRIP); //polyline
        bezierLine->draw();

        // Draw points red and selected point blue
        if(selection!=nullptr) lineShader->set_uniform("selection", int(selection-&controlPoints[0]));
        line->set_mode(GL_POINTS);
        line->draw();

        lineShader->unbind();
    });
    window.set_title("Assignment 3");
    window.set_size(width, height);

    // Mouse movement callback
    window.add_listener<MouseMoveEvent>([&](const MouseMoveEvent &m){
        // Mouse position in clip coordinates
        pixelPosition = m.position;
        Vec2 p = 2.0f*(Vec2(m.position.x()/width,-m.position.y()/height) - Vec2(0.5f,-0.5f));
        if( selection && (p-position).norm() > 0.0f) {
            selection->x() = position.x();
            selection->y() = position.y();
            line->set_vbo<Vec2>("vposition", controlPoints);
            //bezierLine->set_vbo<Vec2>("vposition", controlBPoints);
        }
        position = p;
    });

    // Mouse click callback
    window.add_listener<MouseButtonEvent>([&](const MouseButtonEvent &e){
        // Mouse selection case
        if( e.button == GLFW_MOUSE_BUTTON_LEFT && !e.released) {

            /// Draw element id's to framebuffer
            selectionFB->bind();
            glViewport(0,0,1*width, 1*height);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear color must be 1,1,1,1
            glPointSize(POINTSIZE);
            selectionShader->bind();
            selectionShader->set_uniform("offsetID", offsetID);
            line->set_attributes(*selectionShader);
            line->set_mode(GL_POINTS);
            line->draw();
            //bezierLine->set_attributes(*selectionShader);
            //bezierLine->set_mode(GL_POINTS);
            //bezierLine->draw();
            selectionShader->unbind();
            glFlush();
            glFinish();

            selection = nullptr;
            unsigned char a[4];
            glReadPixels(int(pixelPosition[0]), height - int(pixelPosition[1]), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &a);
            selection = &controlPoints[0] + (int)a[0];
            selectionFB->unbind();
        }
        // Mouse release case
        if( e.button == GLFW_MOUSE_BUTTON_LEFT && e.released) {
            if(selection) {
                selection->x() = position.x();
                selection->y() = position.y();
                selection = nullptr;
                line->set_vbo<Vec2>("vposition", controlPoints);
                //bezierLine->set_vbo<Vec2>("vposition", controlPoints);
            }
        }
    });

    return app.run();
}

void init(){
    glClearColor(1,1,1, /*solid*/1.0 );

    // Enable alpha blending so texture backgroudns remain transparent
    glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    fbShader = std::unique_ptr<Shader>(new Shader());
    fbShader->verbose = true;
    fbShader->add_vshader_from_source(fb_vshader);
    fbShader->add_fshader_from_source(fb_fshader);
    fbShader->link();

    quadShader = std::unique_ptr<Shader>(new Shader());
    quadShader->verbose = true;
    quadShader->add_vshader_from_source(quad_vshader);
    quadShader->add_fshader_from_source(quad_fshader);
    quadShader->link();

    quadInit(quad);

    loadTexture(moto, "moto.png");
    loadTexture(wheel1, "wheel.png");
    loadTexture(wheel2, "wheel.png");
    loadTexture(tail, "sparkle.png");
    loadTexture(saltflats, "saltflats.png");

    lineShader = std::unique_ptr<Shader>(new Shader());
    lineShader->verbose = true;
    lineShader->add_vshader_from_source(line_vshader);
    lineShader->add_fshader_from_source(line_fshader);
    lineShader->link();

    controlPoints = std::vector<Vec2>();
    controlPoints.push_back(Vec2(-0.7f,-0.2f)); //P0
    controlPoints.push_back(Vec2(-0.3f, 0.2f)); //P1
    controlPoints.push_back(Vec2( 0.3f, 0.5f)); //P2
    controlPoints.push_back(Vec2( 0.7f, 0.0f)); //P3

    controlBPoints = std::vector<Vec2>();
    controlBPoints.push_back(Vec2(-0.7f,-0.2f)); //P0
    /*controlBPoints.push_back(Vec2(-0.3f, 0.2f)); //P1
    controlBPoints.push_back(Vec2( 0.3f, 0.5f)); //P2
    controlBPoints.push_back(Vec2( 0.7f, 0.0f)); //P3*/

    for(float i = 0; i < 1; i += PRECISION){
        // calculate points on bezier curve using deCasteljau's method
        Vec2 bezierPt;
        deCasteljau(bezierPt, controlPoints.at(0), controlPoints.at(1), controlPoints.at(2), controlPoints.at(3), i);
        std::cout << "\nx " << bezierPt(0) << " y " << bezierPt(1) << std::endl;
        controlBPoints.push_back(bezierPt);
    }
    controlBPoints.push_back(Vec2( 0.7f, 0.0f)); //P3

    std::cout << "bezierPoint Vec2 list contains: \n";
    for (int i = 0; i < controlBPoints.size(); i++) {
        std::cout << "x " << controlBPoints.at(i)(0) << "y " << controlBPoints.at(i)(1) << std::endl;
    }

    std::cout << "bezier points #: " << controlBPoints.size() << std::endl;

    line = std::unique_ptr<GPUMesh>(new GPUMesh());
    line->set_vbo<Vec2>("vposition", controlPoints);
    std::vector<unsigned int> indices = {0,1,2,3};
    line->set_triangles(indices);

    bezierLine = std::unique_ptr<GPUMesh>(new GPUMesh());
    bezierLine->set_vbo<Vec2>("vposition", controlBPoints);
    std::vector<unsigned int> indicesB;
    for(unsigned int i = 0; i < controlBPoints.size(); i++){
        indicesB.push_back(i);
    }
    bezierLine->set_triangles(indicesB);
}

void deCasteljau(Vec2 &bezierPt, const Vec2 P0, const Vec2 P1, const Vec2 P2, const Vec2 P3, const float t)
{

    Vec2 P11, P21, P31, P12, P22;
    getPt(P11, P0, P1, t); // point between P0 and P1
    getPt(P21, P1, P2, t);
    getPt(P31, P2, P3, t);
    getPt(P12, P11, P21, t);
    getPt(P22, P21, P31, t);
    getPt(bezierPt, P12, P22, t);

}

// interpolation between two points
void getPt(Vec2 &point, const Vec2 P1, const Vec2 P2, const float t)
{

    point(0) = P1(0) + t*(P2(0)-P1(0)); // x coordinate
    point(1) = P1(1) + t*(P2(1)-P1(1)); // y coordinate

}

void quadInit(std::unique_ptr<GPUMesh> &quad) {
    quad = std::unique_ptr<GPUMesh>(new GPUMesh());
    std::vector<Vec3> quad_vposition = {
        Vec3(-1, -1, 0),
        Vec3(-1,  1, 0),
        Vec3( 1, -1, 0),
        Vec3( 1,  1, 0)
    };
    quad->set_vbo<Vec3>("vposition", quad_vposition);
    std::vector<unsigned int> quad_triangle_indices = {
        0, 2, 1, 1, 2, 3
    };
    quad->set_triangles(quad_triangle_indices);
    std::vector<Vec2> quad_vtexcoord = {
        Vec2(0, 0),
        Vec2(0,  1),
        Vec2( 1, 0),
        Vec2( 1,  1)
    };
    quad->set_vtexcoord(quad_vtexcoord);
}

void loadTexture(std::unique_ptr<RGBA8Texture> &texture, const char *filename) {
    // Used snippet from https://raw.githubusercontent.com/lvandeve/lodepng/master/examples/example_decode.cpp
    std::vector<unsigned char> image; //the raw pixels
    unsigned width, height;
    //decode
    unsigned error = lodepng::decode(image, width, height, filename);
    //if there's an error, display it
    if(error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
    //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...

    // unfortunately they are upside down...lets fix that
    unsigned char* row = new unsigned char[4*width];
    for(int i = 0; i < int(height)/2; ++i) {
        memcpy(row, &image[4*i*width], 4*width*sizeof(unsigned char));
        memcpy(&image[4*i*width], &image[image.size() - 4*(i+1)*width], 4*width*sizeof(unsigned char));
        memcpy(&image[image.size() - 4*(i+1)*width], row, 4*width*sizeof(unsigned char));
    }
    delete row;

    texture = std::unique_ptr<RGBA8Texture>(new RGBA8Texture());
    texture->upload_raw(width, height, &image[0]);
}

void drawScene(float timeCount)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float t = timeCount * SpeedFactor;

    // **** Background transform
    Transform TRS = Transform::Identity();
    // Draw background
    drawBackground(TRS);

    // transform along bezier curve
    // bottom left to top right curve
    /*Vec2 P0 = Vec2(-4.0f,-4.0f);
    Vec2 P1 = Vec2(-4.0f, -3.0f);
    Vec2 P2 = Vec2( 4.0f, 7.0f);
    Vec2 P3 = Vec2( 8.0f, 4.0f);
    // curly q
    Vec2 P0 = Vec2(6.0f,-4.0f);
    Vec2 P1 = Vec2(0.0f, 10.5f);
    Vec2 P2 = Vec2(-7.0f, -8.0f);
    Vec2 P3 = Vec2(6.0f, 2.0f);
    //ease in/out
    Vec2 P0 = Vec2(-3.0f,-5.0f);
    Vec2 P1 = Vec2(6.0f, -3.0f);
    Vec2 P2 = Vec2(1.0f, 6.0f);
    Vec2 P3 = Vec2(4.0f, 4.0f);
    //ease in
    Vec2 P0 = Vec2(-3.0f,-5.0f);
    Vec2 P1 = Vec2(5.0f, -5.0f);
    Vec2 P2 = Vec2(5.0f, 5.5f);
    Vec2 P3 = Vec2(6.0f, 7.0f);
    //ease out
    Vec2 P0 = Vec2(-3.0f,-5.0f);
    Vec2 P1 = Vec2(-2.5f, -4.0f);
    Vec2 P2 = Vec2(0.0f, 4.0f);
    Vec2 P3 = Vec2(4.5f, 4.5f);

    controlPoints.push_back(Vec2(-0.7f,-0.2f)); //P0
    controlPoints.push_back(Vec2(-0.3f, 0.2f)); //P1
    controlPoints.push_back(Vec2( 0.3f, 0.5f)); //P2
    controlPoints.push_back(Vec2( 0.7f, 0.0f)); //P3

*/

    // Bezier curve path control points
    Vec2 P0 = controlPoints.at(0) *3; //Vec2(6.0f,-4.0f);
    Vec2 P1 = controlPoints.at(1) *3; //Vec2(-2.5f, -4.0f);
    Vec2 P2 = controlPoints.at(2) *3; //Vec2(0.0f, 4.0f);
    Vec2 P3 = controlPoints.at(3) *3; //Vec2(4.5f, 4.5f);

    // **** Motorcycle transform
    TRS *= Eigen::AlignedScaling3f(0.38f, 0.2f, 1.0);

    // Calculate motion along Bezier curve
    float scale_t = t*0.3;
    scale_t = fmod(scale_t, 1.0);
    float x_coor = pow((1-scale_t), 3) *P0(0) + 3*scale_t*pow((1-scale_t),2)*P1(0) + 3*pow(scale_t,2)*(1-scale_t)*P2(0) + pow(scale_t,3)*P3(0);
    float y_coor = pow((1-scale_t), 3) *P0(1) + 3*scale_t*pow((1-scale_t),2)*P1(1) + 3*pow(scale_t,2)*(1-scale_t)*P2(1) + pow(scale_t,3)*P3(1);
    TRS *= Eigen::Translation3f(x_coor, y_coor, 0.0);

    // **** Add 2 separate heirarchies of transformation
    // **** 1. Two Wheels transforms
    // spinning separate from motorcycle
    float freq = M_PI*t;
    Transform wheel1_TRS = TRS;
    wheel1_TRS *= Eigen::AlignedScaling3f(0.4f, 0.68f, 1.0);
    wheel1_TRS *= Eigen::Translation3f(-1.8, -0.4, 0.0);
    wheel1_TRS *= Eigen::AngleAxisf(-freq/*spins clockwise*/, Eigen::Vector3f::UnitZ());

    Transform wheel2_TRS = TRS;
    wheel2_TRS *= Eigen::AlignedScaling3f(0.4f, 0.68f, 1.0);
    wheel2_TRS *= Eigen::Translation3f(1.8, -0.4, 0.0);
    wheel2_TRS *= Eigen::AngleAxisf(-freq/*spins clockwise*/, Eigen::Vector3f::UnitZ());

    // **** 2. Rainbow Sparkle Tail transform
    // becomes bigger and smaller over time separate from motorcycle
    Transform tail_TRS = TRS;
    tail_TRS *= Eigen::Translation3f(-1.83, 0.3, 0.0);
    scale_t = 0.25*std::sin(freq); // normalized percentage [0,1)
    tail_TRS *= Eigen::AlignedScaling3f(1.0+scale_t, 1.0+scale_t, 1.0);

    // Draw all scene figures
    drawFigure(tail_TRS, tail);
    drawFigure(TRS, moto);
    drawFigure(wheel1_TRS, wheel1);
    drawFigure(wheel2_TRS, wheel2);

    glDisable(GL_BLEND);
}

void drawBackground(Transform TRS)
{
    // background
    quadShader->bind();
    quadShader->set_uniform("M", TRS.matrix());
    // Make texture unit 0 active
    glActiveTexture(GL_TEXTURE0);
    // Bind the texture to the active unit for drawing
    saltflats->bind();
    // Set the shader's texture uniform to the index of the texture unit we have
    // bound the texture to
    quadShader->set_uniform("tex", 0);
    quad->set_attributes(*quadShader);
    quad->draw();
    saltflats->unbind();
}

void drawFigure(Transform TRS, std::unique_ptr<RGBA8Texture> &tex)
{
    quadShader->bind();
    quadShader->set_uniform("M", TRS.matrix());
    // Make texture unit 0 active
    glActiveTexture(GL_TEXTURE0);
    // Bind the texture to the active unit for drawing
    tex->bind();
    // Set the shader's texture uniform to the index of the texture unit we have
    // bound the texture to
    quadShader->set_uniform("tex", 0);
    quad->set_attributes(*quadShader);
    quad->draw();
    tex->unbind();
    quadShader->unbind();
}
