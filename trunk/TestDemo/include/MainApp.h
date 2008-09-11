#ifndef MAINAPP_H
#define MAINAPP_H

#include <Ogre.h>
#include "InputManager.h"
#include "OgreOggSound.h"

class MainApp : public Ogre::FrameListener, public OIS::KeyListener, public OIS::MouseListener, public Ogre::Singleton<MainApp>
{
private:
	Ogre::SceneManager *mSceneMgr;
	Ogre::RenderWindow *mWindow;
	Ogre::Camera *mCamera;
	Ogre::Viewport* mVp;
	InputManager *mInputManager;
	bool mQuit;		
	Ogre::Real mFrameTime;

	Ogre::Vector3 mTranslateVectorCamera;
	Ogre::Real mYawAngleCamera;

	OgreOggSound::OgreOggSoundManager *mSoundManager;
public:
	MainApp();
	~MainApp();
	void createCamera();	
	void createViewport();
	void createScene();
	void Init();
	void requestQuit();
	void resize();		

	void finishedCB(OgreOggSound::OgreOggISound* sound);		

	static MainApp& getSingleton();
	static MainApp* getSingletonPtr();	
	bool frameStarted( const Ogre::FrameEvent& evt ); 
	bool frameEnded( const Ogre::FrameEvent& evt );
	void updateStats();	
	virtual bool keyPressed( const OIS::KeyEvent &arg );
    virtual bool keyReleased( const OIS::KeyEvent &arg );	
	virtual bool mouseMoved(const OIS::MouseEvent &arg);
	virtual bool mousePressed(const OIS::MouseEvent &arg ,OIS::MouseButtonID id);
	virtual bool mouseReleased(const OIS::MouseEvent &arg ,OIS::MouseButtonID id);
};


#endif