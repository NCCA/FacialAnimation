#ifndef OPENGLWINDOW_H__
#define OPENGLWINDOW_H__
#include <QtGui/QWindow>

//----------------------------------------------------------------------------------------------------------------------
/// @class OpenGLWindow
/// @file OpenGLWindow.h
/// @brief this is the base class for all our OpenGL widgets, inherit from this class and overide the methods for
/// OpenGL drawing modified from the Qt demo here  http://qt-project.org/doc/qt-5.0/qtgui/openglwindow.html
/// @author Jonathan Macey
/// @version 1.0
/// @date 10/9/13
/// Revision History :
/// This is an initial version used for the new NGL6 / Qt 5 demos
//----------------------------------------------------------------------------------------------------------------------

// pre declare some classes we need
class QPainter;
class QOpenGLContext;
class QOpenGLPaintDevice;

class OpenGLWindow : public QWindow
{
  // need to tell Qt to run the MOC
  Q_OBJECT
  public:
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief ctor for OpenGL window must set the surface type to OpenGL
    /// @param [in] parent the parent window to the class
    //----------------------------------------------------------------------------------------------------------------------
    explicit OpenGLWindow(QWindow *_parent = 0);
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief dtor, remember to remove the device once finished
    //----------------------------------------------------------------------------------------------------------------------

    ~OpenGLWindow();
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief pure virtual render method we override in our base class to do our drawing, called every update
    //----------------------------------------------------------------------------------------------------------------------
    virtual void render()=0;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief pure virtual initialize method we override in our base class to do our drawing
    /// this is only called one time, just after we have a valid GL context use this to init any global GL elements
    //----------------------------------------------------------------------------------------------------------------------
    virtual void initialize()=0;

  public slots:
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief Qt slot to allow use to queue up a render for the next avaliable event process
    //----------------------------------------------------------------------------------------------------------------------
    void renderLater();
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief slot to tell us to render immediatly
    //----------------------------------------------------------------------------------------------------------------------
    void renderNow();

  protected:
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief qt event processing if we post renderLater event this will be called and execute a render
    //----------------------------------------------------------------------------------------------------------------------
    bool event(QEvent *event);
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief this even is called when the window is made visible and will trigger a render
    //----------------------------------------------------------------------------------------------------------------------
    void exposeEvent(QExposeEvent *event);

  private:
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief flag to indicate if we need to update and render
    //----------------------------------------------------------------------------------------------------------------------
    bool m_updatePending;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief the current openGL context, created once when the scene if firt rendered
    //----------------------------------------------------------------------------------------------------------------------
    QOpenGLContext *m_context;
};

#endif
