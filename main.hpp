/*
 * main.hpp
 *
 *  Created on: Mar 11, 2014
 *      Author: smistad
 */

#ifndef MAIN_HPP_
#define MAIN_HPP_

#include <QtOpenGL/QGLWidget>
#include "OpenCLManager.hpp"
#include <GL/glx.h>
class GLWidget : public QGLWidget {
    Q_OBJECT
    protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    int id;
    oul::Context context;
    cl::Image3D clImage;
    GLuint texture;
    bool valid;
#if defined(CL_VERSION_1_2)
    cl::ImageGL imageGL;
#else
    cl::Image2DGL imageGL;
#endif
    int sliceNr;
public:
    GLWidget(int id, oul::Context contex, cl::Image3D image);
};


class MyWindow : public QWidget {
    Q_OBJECT
public:
    MyWindow(int id, oul::Context context, cl::Image3D image);
    static QGLContext* GLContext;
protected:
private:
    GLWidget *glWidget;
};



#endif /* MAIN_HPP_ */
