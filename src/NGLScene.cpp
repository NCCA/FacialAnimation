#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <ngl/Transformation.h>


#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

// create a typecast to tokenizer as it's quicker to write than the whole
// line
typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

NGLScene::NGLScene()
{
  setTitle("Qt5 Simple NGL Demo");
  m_activeWeight=0;

}


NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";

}


void NGLScene::resizeGL( int _w, int _h )
{
  m_project=ngl::perspective( 45.0f, static_cast<float>( _w ) / _h, 0.05f, 350.0f );
  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}

void NGLScene::initializeGL()
{
  ngl::NGLInit::initialize();

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0,1.5,15);
  ngl::Vec3 to(0,1.5,0);
  ngl::Vec3 up(0,1,0);
  m_view=ngl::lookAt(from,to,up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_project=ngl::perspective(45,(float)720.0/576.0,0.05,350);
  // we are creating a shader called PerFragADS
  ngl::ShaderLib::createShaderProgram("PerFragADS");
  // now we are going to create empty shaders for Frag and Vert
  ngl::ShaderLib::attachShader("PerFragADSVertex",ngl::ShaderType::VERTEX);
  ngl::ShaderLib::attachShader("PerFragADSFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  ngl::ShaderLib::loadShaderSource("PerFragADSVertex","shaders/PerFragASDVert.glsl");
  ngl::ShaderLib::loadShaderSource("PerFragADSFragment","shaders/PerFragASDFrag.glsl");
  // compile the shaders
  ngl::ShaderLib::compileShader("PerFragADSVertex");
  ngl::ShaderLib::compileShader("PerFragADSFragment");
  // add them to the program
  ngl::ShaderLib::attachShaderToProgram("PerFragADS","PerFragADSVertex");
  ngl::ShaderLib::attachShaderToProgram("PerFragADS","PerFragADSFragment");

  // now we have associated this data we can link the shader
  ngl::ShaderLib::linkProgramObject("PerFragADS");
  // and make it active ready to load values
  ngl::ShaderLib::use("PerFragADS");
  // now we need to set the material and light values
  /*
   *struct MaterialInfo
   {
        // Ambient reflectivity
        vec3 Ka;
        // Diffuse reflectivity
        vec3 Kd;
        // Specular reflectivity
        vec3 Ks;
        // Specular shininess factor
        float shininess;
  };*/
  ngl::ShaderLib::setUniform("material.Ka",0.1f,0.1f,0.1f);
  // red diffuse
  ngl::ShaderLib::setUniform("material.Kd",0.8f,0.8f,0.8f);
  // white spec
  ngl::ShaderLib::setUniform("material.Ks",1.0f,1.0f,1.0f);
  ngl::ShaderLib::setUniform("material.shininess",800.0f);
  ngl::ShaderLib::setUniform("TBO",0);
  // now for  the lights values (all set to white)
  /*struct LightInfo
  {
  // Light position in eye coords.
  vec4 position;
  // Ambient light intensity
  vec3 La;
  // Diffuse light intensity
  vec3 Ld;
  // Specular light intensity
  vec3 Ls;
  };*/
  ngl::ShaderLib::setUniform("light.position",from);
  ngl::ShaderLib::setUniform("light.La",0.1f,0.1f,0.1f);
  ngl::ShaderLib::setUniform("light.Ld",1.0f,1.0f,1.0f);
  ngl::ShaderLib::setUniform("light.Ls",0.9f,0.9f,0.9f);

  glEnable(GL_DEPTH_TEST); // for removal of hidden surfaces

  ngl::ShaderLib::use("nglDiffuseShader");
  ngl::ShaderLib::setUniform("Colour",1.0f,1.0f,1.0f,1.0f);

  ngl::ShaderLib::setUniform("lightPos",from);
  ngl::ShaderLib::setUniform("lightDiffuse",1.0f,1.0f,1.0f,1.0f);


  // first we create a mesh from an obj passing in the obj file and texture
  m_eyeMesh.reset(new ngl::Obj("models/Eyeball.obj"));
  m_eyeMesh->createVAO();
  parseModelFile();

  m_text=std::make_unique<ngl::Text>("fonts/Arial.ttf",16);
  createMorphMesh();
  glViewport(0,0,1024,720);
  m_text->setScreenSize(width(),height());

}

// a simple structure to hold our vertex data
struct vertData
{
  ngl::Vec3 p1;
  ngl::Vec3 n1;
};
void NGLScene::createMorphMesh()
{
  // texture buffers have to be vec4 unless using GL 4.x so mac is out for now
  // just use Vec4 and waste data see http://www.opengl.org/wiki/Buffer_Texture
  std::vector <ngl::Vec4> targets;
  // base pose is mesh 1 stored in m_meshes[0]
  // get the obj data so we can process it locally
  std::vector <ngl::Vec3> baseVert=m_baseMesh->getVertexList();
  std::vector <ngl::Vec3> baseNormal=m_baseMesh->getNormalList();

  // should really check to see if the poses match if we were doing this properly
  auto numMeshes = m_meshes.size();
  std::cout<<"num meshes "<<numMeshes;
  std::vector <std::vector <ngl::Vec3> > verts;
  std::vector <std::vector <ngl::Vec3> > normals;
  // faces will be the same for each mesh so only need one
  // really we should check that this conforms, however it will
  // just break if it's not right so should know!
  std::vector <ngl::Face> faces=m_baseMesh->getFaceList();

  for(size_t i=0; i<numMeshes; ++i)
  {
    verts.push_back(m_meshes[i]->getVertexList());
    normals.push_back(m_meshes[i]->getNormalList());
  }

  // now we are going to process and pack the mesh into an ngl::VertexArrayObject
  std::vector <vertData> vboMesh;
  vertData d;
  auto nFaces=faces.size();
  // loop for each of the faces
  ngl::Vec3 current;

  for(size_t i=0;i<nFaces;++i)
  {
    // now for each triangle in the face (remember we ensured tri above)
    for(size_t j=0;j<3;++j)
    {
      // pack in the vertex data first

      d.p1=baseVert[faces[i].m_vert[j]];
      // the blend meshes are just the differences so we subtract the base mesh
      // from the current one (could do this on GPU but this saves processing time)
      for(size_t vi=0; vi<numMeshes; ++vi)
      {
        current=verts[vi][faces[i].m_vert[j]]-d.p1;
        targets.push_back(ngl::Vec4(current.m_x,current.m_y,current.m_z,1.0));
      }

      // now do the normals
      d.n1=baseNormal[faces[i].m_norm[j]];
      for(size_t ni=0; ni<numMeshes; ++ni)
      {
        current=normals[ni][faces[i].m_norm[j]]-d.n1;
        targets.push_back(ngl::Vec4(current.m_x,current.m_y,current.m_z,1.0));
      }

    // finally add it to our mesh VAO structure
    vboMesh.push_back(d);
    }
  }

  // generate and bind our matrix buffer this is going to be fed to the feedback shader to
  // generate our model position data for later, if we update how many instances we use
  // this will need to be re-generated (done in the draw routine)
  GLuint morphTarget;
  glGenBuffers(1,&morphTarget);

  glBindBuffer(GL_TEXTURE_BUFFER, morphTarget);
  glBufferData(GL_TEXTURE_BUFFER, targets.size()*sizeof(ngl::Vec4), NULL, GL_STATIC_DRAW);
  glBufferSubData( GL_TEXTURE_BUFFER, 0, targets.size()*sizeof(ngl::Vec4),&targets[0].m_x );              // Fill

  glGenTextures(1, &m_tboID);
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture(GL_TEXTURE_BUFFER,m_tboID);

  glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, morphTarget);

  // first we grab an instance of our VOA class as a TRIANGLE_STRIP
  m_vaoMesh= ngl::VAOFactory::createVAO("simpleVAO",GL_TRIANGLES);
  // next we bind it so it's active for setting data
  m_vaoMesh->bind();
  auto meshSize=vboMesh.size();
  // now we have our data add it to the VAO, we need to tell the VAO the following
  // how much (in bytes) data we are copying
  // a pointer to the first element of data (in this case the address of the first element of the
  // std::vector
  m_vaoMesh->setData(ngl::AbstractVAO::VertexData( meshSize*sizeof(vertData),vboMesh[0].p1.m_x));

  // so data is Vert / Normal for each mesh
  m_vaoMesh->setVertexAttributePointer(0,3,GL_FLOAT,sizeof(vertData),0);
  m_vaoMesh->setVertexAttributePointer(1,3,GL_FLOAT,sizeof(vertData),3);


  // now we have set the vertex attributes we tell the VAO class how many indices to draw when
  // glDrawArrays is called, in this case we use buffSize (but if we wished less of the sphere to be drawn we could
  // specify less (in steps of 3))
  m_vaoMesh->setNumIndices(meshSize);
  // finally we have finished for now so time to unbind the VAO
  m_vaoMesh->unbind();

  // now set all the weights
  for(unsigned int i=0; i<m_meshes.size(); ++i)
  {
    m_weights.push_back(0.0f);
  }
}

void NGLScene::resetWeights()
{
  for(unsigned int i=0; i<m_weights.size(); ++i)
    m_weights[i]=0.0;
}

void NGLScene::changeActiveWeight(Direction _d)
{
  switch (_d)
  {
    case UP :
      if(++m_activeWeight >= static_cast<int>(m_meshes.size()) )
        m_activeWeight=m_meshes.size()-1;
    break;
    case DOWN :
      if(--m_activeWeight <=0 )
        m_activeWeight=0;
    break;
  }
  std::cout<<m_activeWeight<<"\n";
}

void NGLScene::changeWeight(Direction _d )
{

  if(_d == UP)
    m_weights[m_activeWeight]+=0.05;
  else
    m_weights[m_activeWeight]-=0.05;
  // clamp to 0.0 -> 1.0 range
  m_weights[m_activeWeight]=std::min(1.0f, std::max(0.0f, m_weights[m_activeWeight]));
}


void NGLScene::parseModelFile()
{
  // open the file to parse
  std::fstream fileIn;
  fileIn.open("models.txt");
  if (!fileIn.is_open())
  {
    std::cout <<"File : models.txt Not found Exiting "<<std::endl;
    exit(EXIT_FAILURE);
  }


  // this is the line we wish to parse
  std::string lineBuffer;
  // say which separators should be used in this
  // case Spaces, Tabs and return \ new line
  // I'm also doing , seperated list this time
  boost::char_separator<char> sep(",\r\n");

  // loop through the file
  while(!fileIn.eof())
  {
    // grab a line from the input
    getline(fileIn,lineBuffer,'\n');
    // make sure it's not an empty line
    if(lineBuffer.size() !=0)
    {
      // now tokenize the line
      tokenizer tokens(lineBuffer, sep);
      // and get the first token
      tokenizer::iterator  firstWord = tokens.begin();
      // now see if it's a valid one and call the correct function
      if( *firstWord =="BaseMesh")
      {
          ++firstWord;
          std::cout<<"found base mesh loading "<<*firstWord<<"\n";
          m_baseMesh.reset( new ngl::Obj(*firstWord));
      }
      else if( *firstWord =="BlendShape")
      {
          ++firstWord;
          std::cout<<"Found "<<*firstWord<<"\n";
          m_meshNames.push_back(*firstWord);
          ++firstWord;
          std::unique_ptr<ngl::Obj> mesh( new ngl::Obj(*firstWord));
          m_meshes.push_back(std::move(mesh));      }
    }// end zero loop
   }
}

void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib::use("PerFragADS");
  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  MV=m_view*m_mouseGlobalTX;
  MVP=m_project *MV;
  normalMatrix=MV;
  normalMatrix.inverse().transpose();
  ngl::ShaderLib::setUniform("MVP",MVP);
  ngl::ShaderLib::setUniform("MV",MV);
  ngl::ShaderLib::setUniform("normalMatrix",normalMatrix);
  for(unsigned int i=0; i<m_weights.size(); ++i)
  {
    ngl::ShaderLib::setUniform(fmt::format("weights[{}]",i),m_weights[i]);
  }


}

void NGLScene::paintGL()
{
  // clear the screen and depth buffer
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   // Rotation based on the mouse position for our global transform
   ngl::Transformation trans;
   ngl::Mat4 rotX;
   ngl::Mat4 rotY;
   // create the rotation matrices
   rotX.rotateX(m_win.spinXFace);
   rotY.rotateY(m_win.spinYFace);
   // multiply the rotations
   m_mouseGlobalTX=rotY*rotX;
   // add the translations
   m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
   m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
   m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;

  loadMatricesToShader();
  // draw the mesh
  m_vaoMesh->bind();
  glBindTexture(GL_TEXTURE_BUFFER, m_tboID);
  m_vaoMesh->draw();
  m_vaoMesh->unbind();

  ngl::ShaderLib::use("nglDiffuseShader");
  // left Eye

  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat4 M;
  ngl::Mat3 normalMatrix;
  ngl::Transformation t;
  t.setScale(0.685f,0.583f,0.583f);
  t.setPosition(-1.276f,3.209f,2.271f);
  M=m_mouseGlobalTX*t.getMatrix();
  MV= m_view*M;
  MVP=m_project*MV ;
  normalMatrix=MV;
  normalMatrix.inverse().transpose();

  ngl::ShaderLib::setUniform("MVP",MVP);
  ngl::ShaderLib::setUniform("normalMatrix",normalMatrix);
  m_eyeMesh->draw();

  t.setPosition(1.276f,3.209f,2.271f);
  M=m_mouseGlobalTX*t.getMatrix();
  MV= m_view*M;
  MVP=m_project*MV ;
  normalMatrix=MV;
  normalMatrix.inverse();
  ngl::ShaderLib::setUniform("MVP",MVP);
  ngl::ShaderLib::setUniform("normalMatrix",normalMatrix);
  m_eyeMesh->draw();
  m_text->setColour(1.0f,1.0f,1.0f);
  m_text->renderText(10,700,fmt::format("Current Mesh {} value {}",m_meshNames[m_activeWeight],m_weights[m_activeWeight]));
  m_text->renderText(10,680,"Q-W change Pose Arrows to swap weights");

}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent( QMouseEvent* _event )
{
  // note the method buttons() is the button state when event was called
  // that is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if ( m_win.rotate && _event->buttons() == Qt::LeftButton )
  {
    int diffx = _event->x() - m_win.origX;
    int diffy = _event->y() - m_win.origY;
    m_win.spinXFace += static_cast<int>( 0.5f * diffy );
    m_win.spinYFace += static_cast<int>( 0.5f * diffx );
    m_win.origX = _event->x();
    m_win.origY = _event->y();
    update();
  }
  // right mouse translate code
  else if ( m_win.translate && _event->buttons() == Qt::RightButton )
  {
    int diffX      = static_cast<int>( _event->x() - m_win.origXPos );
    int diffY      = static_cast<int>( _event->y() - m_win.origYPos );
    m_win.origXPos = _event->x();
    m_win.origYPos = _event->y();
    m_modelPos.m_x += INCREMENT * diffX;
    m_modelPos.m_y -= INCREMENT * diffY;
    update();
  }
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent( QMouseEvent* _event )
{
  // that method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
  if ( _event->button() == Qt::LeftButton )
  {
    m_win.origX  = _event->x();
    m_win.origY  = _event->y();
    m_win.rotate = true;
  }
  // right mouse translate mode
  else if ( _event->button() == Qt::RightButton )
  {
    m_win.origXPos  = _event->x();
    m_win.origYPos  = _event->y();
    m_win.translate = true;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent( QMouseEvent* _event )
{
  // that event is called when the mouse button is released
  // we then set Rotate to false
  if ( _event->button() == Qt::LeftButton )
  {
    m_win.rotate = false;
  }
  // right mouse translate mode
  if ( _event->button() == Qt::RightButton )
  {
    m_win.translate = false;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent( QWheelEvent* _event )
{

  // check the diff of the wheel position (0 means no change)
  if ( _event->angleDelta().x() > 0 )
  {
    m_modelPos.m_z += ZOOM;
  }
  else if ( _event->angleDelta().x() < 0 )
  {
    m_modelPos.m_z -= ZOOM;
  }
  update();
}
//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  case Qt::Key_1 : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
  case Qt::Key_2 : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
  case Qt::Key_F : showFullScreen(); break;
  case Qt::Key_N : showNormal(); break;
  case Qt::Key_Q : changeWeight(DOWN); break;
  case Qt::Key_W : changeWeight(UP); break;

  case Qt::Key_Left : changeActiveWeight(DOWN); break;
  case Qt::Key_Right : changeActiveWeight(UP); break;

  case Qt::Key_Space : resetWeights(); break;
  default : break;
  }
  // finally update the GLWindow and re-draw
  //if (isExposed())
    update();
}
