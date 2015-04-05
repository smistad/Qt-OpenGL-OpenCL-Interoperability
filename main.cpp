#include <QApplication>
#include <QHBoxLayout>
#include "main.hpp"


#include <boost/thread.hpp>

#include <iostream>
#include <stdlib.h>

#define PRINT(x) std::cout << x << std::endl;

GLXContext GLWidget::glContext = NULL;


MyWindow::MyWindow(int id, oul::Context context, cl::Image3D image) {
    glWidget = new GLWidget(id,context,image);
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(glWidget);
    setLayout(mainLayout);
    setWindowTitle(tr("Hello GL"));
}


GLWidget::GLWidget(int id, oul::Context context, cl::Image3D image,QWidget *parent)
        : id(id),context(context),clImage(image) {
    sliceNr = 120;
    valid = false;

}

void GLWidget::initializeGL() {

}

void GLWidget::resizeGL(int w, int h) {

}

void GLWidget::paintGL() {
    PRINT("drawing")
    bool success = glXMakeCurrent(XOpenDisplay(0),glXGetCurrentDrawable(),glContext);
    if(!success)
        PRINT("failed to switch to window");
    PRINT("current glX drawable is " << glXGetCurrentDrawable());
    PRINT("current glX context is " << glXGetCurrentContext());


    if (!valid) {
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
        valid = true;
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

    //this->swap_buffers();
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

    GLWidget::glContext = glXCreateContext(display, vi, 0, GL_TRUE);
    PRINT("created GL context with ID " << GLWidget::glContext)

    // Create OpenCL context
    oul::DeviceCriteria criteria;
    criteria.setTypeCriteria(oul::DEVICE_TYPE_GPU);
    criteria.setDeviceCountCriteria(1);
    criteria.setCapabilityCriteria(oul::DEVICE_CAPABILITY_OPENGL_INTEROP);
    oul::OpenCLManager * manager = oul::OpenCLManager::getInstance();
    oul::Context context = manager->createContext(criteria, (unsigned long *)(GLWidget::glContext));
    context.createProgramFromSource("../renderSliceToTexture.cl");
    //oul::Context context2 = manager->createContext(criteria, (unsigned long *)(MyWindow::glContext));

    // Create OpenCL Image
    cl::Image3D clImage = cl::Image3D(
            context.getContext(),
            CL_MEM_READ_WRITE,
            cl::ImageFormat(CL_R, CL_UNSIGNED_INT8),
            400,400,400
    );

    QApplication app(argc, argv);
    MyWindow *window1 = new MyWindow(1, context,clImage);
    window1->resize(400,400);
    window1->show();

    MyWindow *window2 = new MyWindow(2, context,clImage);
    window2->resize(400,400);
    window2->move(400,0);
    window2->show();

    // Test if the main loop can be run in a separate thread
    //thread = new boost::thread(QApplication::exec);
    QApplication::exec();

    return 0;
}
