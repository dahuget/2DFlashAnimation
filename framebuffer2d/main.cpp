#include <OpenGP/GL/Application.h>
#include <OpenGP/external/LodePNG/lodepng.cpp>

using namespace OpenGP;

const int width=1000, height=1000;
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

const float SpeedFactor = 1;
void init();
void quadInit(std::unique_ptr<GPUMesh> &quad);
void loadTexture(std::unique_ptr<RGBA8Texture> &texture, const char* filename);
void drawScene(float timeCount);

std::unique_ptr<GPUMesh> quad;

std::unique_ptr<Shader> quadShader;
std::unique_ptr<Shader> fbShader;

std::unique_ptr<Shader> lineShader;
std::unique_ptr<GPUMesh> line;
std::vector<Vec2> controlPoints;

std::unique_ptr<RGBA8Texture> cat;
std::unique_ptr<RGBA8Texture> tail;
std::unique_ptr<RGBA8Texture> stars;

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
    c_buf->allocate(width, height);
    /// TODO: attach color texture to framebuffer
    fb->attach_color_texture(*c_buf); //de-reference pointer to texture


    Window& window = app.create_window([](Window&){
        glViewport(0,0,width,height);
        /// TODO: First draw the scene onto framebuffer
        /// bind and then unbind framebuffer
        fb->bind();
            glClear(GL_COLOR_BUFFER_BIT);
            drawScene(glfwGetTime());
        fb->unbind(); // sets to display ie main screen
        /// Render to Window, uncomment the lines and do TODOs
        glViewport(0, 0, width, height);
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
    });
    window.set_title("FrameBuffer");
    window.set_size(width, height);

    return app.run();
}

void init(){
    glClearColor(1,1,1, /*solid*/1.0 );

    // Enable alpha blending so texture backgroudns remain transparent
    //glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    loadTexture(cat, "nyancat.png");
    loadTexture(tail, "sparkle.png");
    loadTexture(stars, "background.png");

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

/*void drawLine(Vec2 p1, Vec2 p2) {
    glBegin(GL_LINES);
      glVertex2f(p1(0), p1(1));
      glVertex2f(p2(0), p2(1));
    glEnd();
    glFlush();
}*/

void drawScene(float timeCount)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float t = timeCount * SpeedFactor;
    Transform TRS = Transform::Identity();
    //background.draw(TRS.matrix());
    quadShader->bind();
    quadShader->set_uniform("M", TRS.matrix());
    // Make texture unit 0 active
    glActiveTexture(GL_TEXTURE0);
    // Bind the texture to the active unit for drawing
    stars->bind();
    // Set the shader's texture uniform to the index of the texture unit we have
    // bound the texture to
    quadShader->set_uniform("tex", 0);
    quad->set_attributes(*quadShader);
    quad->draw();
    stars->unbind();

    float xcord = 0.7*std::cos(t);
    float ycord = 0.7*std::sin(t);
    //TRS *= Eigen::Translation3f(xcord, ycord, 0);
    //TRS *= Eigen::AngleAxisf(t + M_PI / 2, Eigen::Vector3f::UnitZ());
    TRS *= Eigen::AlignedScaling3f(0.2f, 0.2f, 1);

    // transform along bezier curve (bottom left to top right curve)
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
    Vec2 P3 = Vec2(6.0f, 7.0f);*/
    //ease out
    Vec2 P0 = Vec2(-3.0f,-5.0f);
    Vec2 P1 = Vec2(-2.5f, -4.0f);
    Vec2 P2 = Vec2(0.0f, 4.0f);
    Vec2 P3 = Vec2(4.5f, 4.5f);


    float scale_t = timeCount*0.1;
    float x = pow((1-scale_t), 3) *P0(0) + 3*scale_t*pow((1-scale_t),2)*P1(0) + 3*pow(scale_t,2)*(1-scale_t)*P2(0) + pow(scale_t,3)*P3(0);
    float y = pow((1-scale_t), 3) *P0(1) + 3*scale_t*pow((1-scale_t),2)*P1(1) + 3*pow(scale_t,2)*(1-scale_t)*P2(1) + pow(scale_t,3)*P3(1);
    TRS *= Eigen::Translation3f(x, y, 0.0);

    // add heirarchy of transformation
    // **** tail next level transformation heirarchy
    Transform tail_TRS = TRS;
    tail_TRS *= Eigen::Translation3f(-1.83, 0.3, 0.0);
    //tail_TRS *= Eigen::AlignedScaling3f(0.9, 0.9, 1.0);
    //scale_t: make the sun become bigger and smaller over the time!
    scale_t = 0.1*std::sin(t); // normalized percentage [0,1)
    //std::cout << scale_t << std::endl;
    //tail_TRS *= Eigen::Translation3f(-1.85+scale_t, 0.0, 0.0);
    tail_TRS *= Eigen::AlignedScaling3f(0.85+scale_t, 0.85+scale_t, 1.0);

    quadShader->bind();
    quadShader->set_uniform("M", tail_TRS.matrix());
    // Make texture unit 0 active
    glActiveTexture(GL_TEXTURE0);
    // Bind the texture to the active unit for drawing
    tail->bind();
    // Set the shader's texture uniform to the index of the texture unit we have
    // bound the texture to
    quadShader->set_uniform("tex", 0);
    quad->set_attributes(*quadShader);
    quad->draw();
    tail->unbind();
    quadShader->unbind();
    // transform along bezier curve

    quadShader->bind();
    quadShader->set_uniform("M", TRS.matrix());
    // Make texture unit 0 active
    glActiveTexture(GL_TEXTURE0);
    // Bind the texture to the active unit for drawing
    cat->bind();
    // Set the shader's texture uniform to the index of the texture unit we have
    // bound the texture to
    quadShader->set_uniform("tex", 0);
    quad->set_attributes(*quadShader);
    quad->draw();
    cat->unbind();
    quadShader->unbind();

    lineShader->bind();
    // Draw line red
    lineShader->set_uniform("selection", -1); // highlight the selected vertex blue
    line->set_attributes(*lineShader);
    line->set_mode(GL_LINE_STRIP); // feed in points, and then it makes a line with the points
    line->draw();
    lineShader->unbind();

    glDisable(GL_BLEND);
}
