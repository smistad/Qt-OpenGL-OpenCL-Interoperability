#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/gl.h>
#include <FL/Fl_Gl_Window.H>
#include <GL/glx.h>

#include "OpenCLManager.hpp"

#include <boost/thread.hpp>

#include <iostream>
#include <stdlib.h>

#define PRINT(x) std::cout << x << std::endl;

class MyWindow : public Fl_Gl_Window {
    void draw();
    int handle(int);
    int id;
    oul::Context context;
    cl::Image3D clImage;
    GLuint texture;
#if defined(CL_VERSION_1_2)
    cl::ImageGL imageGL;
#else
    cl::Image2DGL imageGL;
#endif
    int sliceNr;
public:
    MyWindow(int X, int Y, int W, int H, const char *L, int id, oul::Context contex, cl::Image3D image);
    static GLXContext glContext;
};

GLXContext MyWindow::glContext = NULL;


MyWindow::MyWindow(int X, int Y, int W, int H, const char *L, int id, oul::Context context, cl::Image3D image)
        : Fl_Gl_Window(X, Y, W, H, L),id(id),context(context),clImage(image) {
    sliceNr = 120;

}

void MyWindow::draw() {
    PRINT("drawing")
    bool success = glXMakeCurrent(XOpenDisplay(0),glXGetCurrentDrawable(),glContext);
    if(!success)
        PRINT("failed to switch to window");
    PRINT("current glX drawable is " << glXGetCurrentDrawable());
    PRINT("current glX context is " << glXGetCurrentContext());


    if (!valid()) {
        // Setup OpenGL
        // Create a GL-CL texture/2d image
        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, clImage.getImageInfo<CL_IMAGE_WIDTH>(), clImage.getImageInfo<CL_IMAGE_HEIGHT>(), 0, GL_RGBA, GL_FLOAT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFinish();

#if defined(CL_VERSION_1_2)
            imageGL = cl::ImageGL(
                    context.getContext(),
                    CL_MEM_READ_WRITE,
                    GL_TEXTURE_2D,
                    0,
                    texture
            );
#else
            imageGL = cl::Image2DGL(
                    context.getContext(),
                    CL_MEM_READ_WRITE,
                    GL_TEXTURE_2D,
                    0,
                    texture
            );
#endif
        // Run kernel to fill the texture
        cl::CommandQueue queue = this->context.getQueue(0);
        std::vector<cl::Memory> v;
        v.push_back(imageGL);
        queue.enqueueAcquireGLObjects(&v);

        cl::Kernel kernel(context.getProgram(0), "renderSliceToTexture");
        kernel.setArg(0, clImage);
        kernel.setArg(1, imageGL);
        kernel.setArg(2, sliceNr);
        queue.enqueueNDRangeKernel(
                kernel,
                cl::NullRange,
                cl::NDRange(clImage.getImageInfo<CL_IMAGE_WIDTH>(), clImage.getImageInfo<CL_IMAGE_HEIGHT>()),
                cl::NullRange
        );

        queue.enqueueReleaseGLObjects(&v);
        queue.finish();
        PRINT("Ran OpenCL kernel")
    }

    glClearColor(0.0f,0.0f,0.0f,0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, texture);

    glBegin(GL_QUADS);
    glTexCoord2i(0, 1);
    glVertex3f(-1.0f, 1.0f, 0.0f);
    glTexCoord2i(1, 1);
    glVertex3f( 1.0f, 1.0f, 0.0f);
    glTexCoord2i(1, 0);
    glVertex3f( 1.0f,-1.0f, 0.0f);
    glTexCoord2i(0, 0);
    glVertex3f(-1.0f,-1.0f, 0.0f);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);

    this->swap_buffers();
}

int MyWindow::handle(int event) {
    if(event == FL_KEYBOARD) {
        PRINT("Key pressed in window " << id);
        this->redraw();
    }
}

boost::thread * thread = NULL;

void quit(void) {
    PRINT("quit called")
    if(thread != NULL)
        thread->join();
}

int main(int argc, char ** argv) {

    if(atexit(quit) != 0)
        PRINT("atexit() failed")
    // Create GL context
    int sngBuf[] = { GLX_RGBA,
                     GLX_DOUBLEBUFFER,
                     GLX_RED_SIZE, 1,
                     GLX_GREEN_SIZE, 1,
                     GLX_BLUE_SIZE, 1,
                     GLX_DEPTH_SIZE, 12,
                     None
    };
    Display * display = XOpenDisplay(0);
    PRINT("display is " << display)
    XVisualInfo* vi = glXChooseVisual(display, DefaultScreen(display), sngBuf);

    MyWindow::glContext = glXCreateContext(display, vi, 0, GL_TRUE);
    PRINT("created GL context with ID " << MyWindow::glContext)

    // Create OpenCL context
    oul::DeviceCriteria criteria;
    criteria.setTypeCriteria(oul::DEVICE_TYPE_GPU);
    criteria.setDeviceCountCriteria(1);
    criteria.setCapabilityCriteria(oul::DEVICE_CAPABILITY_OPENGL_INTEROP);
    oul::OpenCLManager * manager = oul::OpenCLManager::getInstance();
    oul::Context context = manager->createContext(criteria, (unsigned long *)(MyWindow::glContext));
    context.createProgramFromSource("../renderSliceToTexture.cl");
    //oul::Context context2 = manager->createContext(criteria, (unsigned long *)(MyWindow::glContext));

    // Create OpenCL Image
    cl::Image3D clImage = cl::Image3D(
            context.getContext(),
            CL_MEM_READ_WRITE,
            cl::ImageFormat(CL_R, CL_UNSIGNED_INT8),
            400,400,400
    );

    MyWindow *window1 = new MyWindow(0,0,400,400,"Window 1", 1, context,clImage);
    window1->show();

    MyWindow *window2 = new MyWindow(400,0,400,400,"Window 2", 2, context,clImage);
    window2->show();

    // Test if the main loop can be run in a separate thread
    thread = new boost::thread(Fl::run);

    return 0;
}
