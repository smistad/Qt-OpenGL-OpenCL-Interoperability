#include <QApplication>
#include <QHBoxLayout>
#include "main.hpp"


#include <boost/thread.hpp>

#include <iostream>
#include <stdlib.h>

#define PRINT(x) std::cout << x << std::endl;

QGLContext* MyWindow::GLContext = NULL;

MyWindow::MyWindow(int id, oul::Context context, cl::Image3D image) {

    glWidget = new GLWidget(id,context,image);

    // Create new GL context for the new GL widget
    QGLContext* glContext = new QGLContext(QGLFormat::defaultFormat(), glWidget); // by including widget here the context becomes valid
    glContext->create(MyWindow::GLContext);
    if(!glContext->isValid()) {
        std::cout << "QGL context 2 is invalid!" << std::endl;
        exit(-1);
    }
    if(glContext->isSharing()) {
        std::cout << "context 2 is sharing" << std::endl;
    }

    glWidget->setContext(glContext); // Assign the context to the widget


    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(glWidget);
    setLayout(mainLayout);
    setWindowTitle(tr("Hello GL"));
}


GLWidget::GLWidget(int id, oul::Context context, cl::Image3D image) : id(id),context(context),clImage(image) {
    sliceNr = 120;
    valid = false;

}

void GLWidget::initializeGL() {

}

void GLWidget::resizeGL(int w, int h) {

}

void GLWidget::paintGL() {
    PRINT("drawing in widget " << id)
    PRINT("GL context is set to " << glXGetCurrentContext());

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

int main(int argc, char ** argv) {

    QApplication *app = new QApplication(argc, argv);

    // Dummy widget
    QGLWidget* widget = new QGLWidget;

    // Create GL context
    QGLContext* qtGLContext = new QGLContext(QGLFormat::defaultFormat(), widget); // by including widget here the context becomes valid
    qtGLContext->create();
    if(!qtGLContext->isValid()) {
        std::cout << "QGL context is invalid!" << std::endl;
        return 0;
    }

    qtGLContext->makeCurrent();
    std::cout << "Initial GL clContext: " << glXGetCurrentContext() << std::endl;

    MyWindow::GLContext = qtGLContext;

    // Create OpenCL clContext
    oul::DeviceCriteria criteria;
    criteria.setTypeCriteria(oul::DEVICE_TYPE_GPU);
    criteria.setDeviceCountCriteria(1);
    criteria.setCapabilityCriteria(oul::DEVICE_CAPABILITY_OPENGL_INTEROP);
    oul::OpenCLManager * manager = oul::OpenCLManager::getInstance();
    oul::Context clContext = manager->createContext(criteria, (unsigned long *)(glXGetCurrentContext()));
    clContext.createProgramFromSource("../renderSliceToTexture.cl");
    //oul::Context context2 = manager->createContext(criteria, (unsigned long *)(MyWindow::glContext));

    // Create OpenCL Image
    cl::Image3D clImage = cl::Image3D(
            clContext.getContext(),
            CL_MEM_READ_WRITE,
            cl::ImageFormat(CL_R, CL_UNSIGNED_INT8),
            400,400,400
    );

    MyWindow *window1 = new MyWindow(1, clContext,clImage);
    window1->resize(400,400);
    window1->show();

    MyWindow *window2 = new MyWindow(2, clContext,clImage);
    window2->resize(400,400);
    window2->move(400,0);
    window2->show();

    QApplication::exec();

    return 0;
}
