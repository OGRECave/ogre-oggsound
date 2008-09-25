#include "MainApp.h"
#include "MainOgre.h"
#include "OgreOggSound.h"

using namespace OgreOggSound;

template<> MainApp* Ogre::Singleton<MainApp>::ms_Singleton = 0;

const float CAMERA_ROTATE_SPEED = 0.15f;
const float CAMERA_TRANSLATE_SPEED = 30.0f;
//-----------------------------------------------------------------------
MainApp* MainApp::getSingletonPtr()
{
    return ms_Singleton;
}
//-----------------------------------------------------------------------
MainApp& MainApp::getSingleton()
{  
    assert( ms_Singleton );
	return ( *ms_Singleton );  
}
//-----------------------------------------------------------------------
MainApp::MainApp() : 
mQuit(false),mSceneMgr(0),mWindow(0), mInputManager(0),mFrameTime(0),
mTranslateVectorCamera(Ogre::Vector3::ZERO),mYawAngleCamera(0)
{
	Init();
}
//-----------------------------------------------------------------------
MainApp::~MainApp()
{	
	delete mSoundManager;
}
//-----------------------------------------------------------------------
void MainApp::requestQuit()
{
	mQuit = true;
}
//-----------------------------------------------------------------------
void MainApp::Init()
{
	mSceneMgr = MainOgre::getSingletonPtr()->getSceneManager();
	mWindow = MainOgre::getSingletonPtr()->getWindow();	

	mInputManager = InputManager::getSingletonPtr();
	mInputManager->addKeyListener(this,"KeyListener");
	mInputManager->addMouseListener(this,"MouseListener");

	mSoundManager = new OgreOggSoundManager;
	
	createCamera();
	createViewport();
	createScene();	

	Ogre::Root::getSingleton().addFrameListener(this);
}
//-----------------------------------------------------------------------
void MainApp::createCamera()
{
	mCamera = mSceneMgr->createCamera("MainCamera"); 
	
	Ogre::SceneNode* camNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("CamNode");
	Ogre::SceneNode* pitchNode = camNode->createChildSceneNode("PitchNode");
	pitchNode->attachObject(mCamera);	
	camNode->setPosition(0,0,0);
	
	Ogre::Quaternion camPitchQuat = Ogre::Quaternion(Ogre::Degree(0),Ogre::Vector3(0,1,0));
	camNode->getChild(0)->setOrientation(camPitchQuat);	
	
	mCamera->setProjectionType(Ogre::PT_PERSPECTIVE);	
	mCamera->setNearClipDistance(1.0f);
	mCamera->setFarClipDistance(1000.0f);	

	mCamera->setAutoAspectRatio(true);	
}
//-----------------------------------------------------------------------
void MainApp::createViewport()
{
	mVp = mWindow->addViewport(mCamera);	
	mCamera->setAspectRatio( Ogre::Real(mVp->getActualWidth()) / Ogre::Real(mVp->getActualHeight()) );
	mVp->setBackgroundColour(Ogre::ColourValue(0,0,0,0));	
}
//-----------------------------------------------------------------------
void MainApp::createScene()
{
	Ogre::Entity *eOgreHead = mSceneMgr->createEntity("OgreHead","ogrehead.mesh");
	Ogre::SceneNode *nOgreHeadAxis = mSceneMgr->getRootSceneNode()->createChildSceneNode("OgreHeadAxis");
	Ogre::SceneNode *nOgreHead = nOgreHeadAxis->createChildSceneNode("OgreHead",Ogre::Vector3(100,0,0));
	nOgreHead->setScale(0.5f,0.5f,0.5f);
	nOgreHead->attachObject(eOgreHead);

	Ogre::Entity *eOgreMonster = mSceneMgr->createEntity("OgreMonster","ogre bodyShapednew.mesh");
	Ogre::SceneNode *nOgreMonsterAxis = mSceneMgr->getRootSceneNode()->createChildSceneNode("OgreMonsterAxis");
	Ogre::SceneNode *nOgreMonster = nOgreMonsterAxis->createChildSceneNode("OgreMonster");
	nOgreMonster->setScale(0.5f,0.5f,0.5f);
	nOgreMonster->attachObject(eOgreMonster);
	nOgreMonster->setPosition(0,10,-100);

	Ogre::Entity *eOgreMonster2 = mSceneMgr->createEntity("OgreMonster2","ogre bodyShapednew.mesh");
	Ogre::SceneNode *mOgreMonsterAxis = mSceneMgr->getRootSceneNode()->createChildSceneNode("OgreMonsterAxis2");
	Ogre::SceneNode *mOgreMonster = nOgreMonsterAxis->createChildSceneNode("OgreMonster2");
	mOgreMonster->setScale(0.5f,0.5f,0.5f);
	mOgreMonster->attachObject(eOgreMonster2);
	mOgreMonster->setPosition(0,10,-200);

	Ogre::Light *light = mSceneMgr->createLight("MainLight");
	light->setType(Ogre::Light::LT_POINT);
	light->setPosition(0,200,-250);
	light->setSpecularColour(1.0f,1.0f,1.0f);


	mSoundManager->init();
	mSoundManager->setDistanceModel(AL_LINEAR_DISTANCE);

	mCamera->getParentSceneNode()->attachObject(mSoundManager->getListener());

	/** Sound one - non streamed, looping, moving */
	mSoundManager->createSound("One", "one.ogg", true, true);	
	mSoundManager->getSound("One")->setMaxDistance(250);
	mSoundManager->getSound("One")->setReferenceDistance(50);
	nOgreHead->attachObject(mSoundManager->getSound("One"));
	mSoundManager->getSound("One")->play();
	
	/** Sound two - prebuffered, streamed, looping, EFX room effect */
	EAXREVERBPROPERTIES props = REVERB_PRESET_AUDITORIUM;
	mSoundManager->createSound("Two", "two.ogg", true, true, true);	
	mSoundManager->getSound("Two")->setMaxDistance(50);
	mSoundManager->getSound("Two")->setReferenceDistance(5);
	if ( mSoundManager->hasEFXSupport() )
	{
		mSoundManager->createEFXSlot();
		mSoundManager->createEFXEffect("Auditorium", AL_EFFECT_EAXREVERB, &props);
		mSoundManager->attachEffectToSound("Two", 0, "Auditorium");
	}
	mSoundManager->getSound("Two")->play();
	
	/** Sound one - non streamed, looping, moving */
	mSoundManager->createSound("Three", "three.ogg", true, true);	
	mSoundManager->getSound("Three")->setMaxDistance(50);
	mSoundManager->getSound("Three")->setReferenceDistance(5);
	mOgreMonster->attachObject(mSoundManager->getSound("Three"));
	mSoundManager->getSound("Three")->play();
	
	/** Sound one - streamed, looping, EFX Direct filter */
	mSoundManager->createSound("background", "background.ogg", true, true, true);
	mSoundManager->getSound("background")->setRelativeToListener(true);
	mSoundManager->getSound("background")->setVolume(0.2f);
	mSoundManager->getSound("background")->setPriority(1);
	if ( mSoundManager->hasEFXSupport() )
	{
		mSoundManager->createEFXFilter("LowPassTest", AL_FILTER_LOWPASS, 0.1, 0.5);
		mSoundManager->attachFilterToSound("background", "LowPassTest");
	}
//	mSoundManager->getSound("background")->play();
}
//-----------------------------------------------------------------------
void MainApp::finishedCB(OgreOggISound* sound)
{
	Ogre::String msg="*** sound: "+sound->getName()+" has Looped!";
	Ogre::LogManager::getSingleton().logMessage(msg);
}
//-----------------------------------------------------------------------
bool MainApp::frameStarted( const Ogre::FrameEvent& evt )
{
	mFrameTime = evt.timeSinceLastFrame;

	mInputManager->capture();	

	if (mTranslateVectorCamera != Ogre::Vector3::ZERO)
	{
		mCamera->getParentSceneNode()->translate(mCamera->getDerivedOrientation() * mTranslateVectorCamera * mFrameTime);
		mCamera->getParentSceneNode()->yaw(Ogre::Degree(mYawAngleCamera * mFrameTime));
	}	

	Ogre::SceneNode *nHeadAxis = mSceneMgr->getSceneNode("OgreHeadAxis");
	nHeadAxis->rotate(Ogre::Quaternion(Ogre::Degree(20.0f * evt.timeSinceLastFrame),Ogre::Vector3::UNIT_Y));

	Ogre::SceneNode *nMonsterAxis = mSceneMgr->getSceneNode("OgreMonsterAxis");

	mSoundManager->update(evt.timeSinceLastFrame);
	
	if (mQuit) return false;	

	return true;
}
//-----------------------------------------------------------------------
bool MainApp::frameEnded( const Ogre::FrameEvent& evt )
{
	if (mWindow->isClosed())	
		return false;		
	
	updateStats();
	
	return true;
}
//-----------------------------------------------------------------------
void MainApp::updateStats()
{
		static Ogre::String currFps = "Current FPS: ";
		static Ogre::String avgFps = "Average FPS: ";
		static Ogre::String bestFps = "Best FPS: ";
		static Ogre::String worstFps = "Worst FPS: ";
		static Ogre::String tris = "Triangle Count: ";
		static Ogre::String batches = "Batch Count: ";

		// update stats when necessary
		try {
			Ogre::OverlayElement* guiAvg = Ogre::OverlayManager::getSingleton().getOverlayElement("Core/AverageFps");
			Ogre::OverlayElement* guiCurr = Ogre::OverlayManager::getSingleton().getOverlayElement("Core/CurrFps");
			Ogre::OverlayElement* guiBest = Ogre::OverlayManager::getSingleton().getOverlayElement("Core/BestFps");
			Ogre::OverlayElement* guiWorst = Ogre::OverlayManager::getSingleton().getOverlayElement("Core/WorstFps");

			const Ogre::RenderTarget::FrameStats& stats = mWindow->getStatistics();
			guiAvg->setCaption(avgFps + Ogre::StringConverter::toString(stats.avgFPS));
			guiCurr->setCaption(currFps + Ogre::StringConverter::toString(stats.lastFPS));
			guiBest->setCaption(bestFps + Ogre::StringConverter::toString(stats.bestFPS)
				+" "+Ogre::StringConverter::toString(stats.bestFrameTime)+" ms");
			guiWorst->setCaption(worstFps + Ogre::StringConverter::toString(stats.worstFPS)
				+" "+Ogre::StringConverter::toString(stats.worstFrameTime)+" ms");

			Ogre::OverlayElement* guiTris = Ogre::OverlayManager::getSingleton().getOverlayElement("Core/NumTris");
			guiTris->setCaption(tris + Ogre::StringConverter::toString(stats.triangleCount));

			Ogre::OverlayElement* guiBatches = Ogre::OverlayManager::getSingleton().getOverlayElement("Core/NumBatches");
			guiBatches->setCaption(batches + Ogre::StringConverter::toString(stats.batchCount));
		}
		catch(...) { /* ignore */ }
}
//-----------------------------------------------------------------------
bool MainApp::keyPressed( const OIS::KeyEvent &arg )
{	
	// Quit the application
	if (arg.key == OIS::KC_ESCAPE)   
		mQuit = true;

	if (arg.key == OIS::KC_F1)
	{
		if(mSoundManager->getSound("One")->isPlaying())
			mSoundManager->getSound("One")->stop();
		else mSoundManager->getSound("One")->play();
	}

	if (arg.key == OIS::KC_F2)
	{
		if(mSoundManager->getSound("Two")->isPlaying())
			mSoundManager->getSound("Two")->stop();
		else mSoundManager->getSound("Two")->play();
	}

	if (arg.key == OIS::KC_F3)
	{
		if(mSoundManager->getSound("Three")->isPlaying())
			mSoundManager->getSound("Three")->stop();
		else mSoundManager->getSound("Three")->play();
	}

	if (arg.key == OIS::KC_F4)
	{
		if(mSoundManager->getSound("background")->isPlaying())
			mSoundManager->getSound("background")->stop();
		else mSoundManager->getSound("background")->play();
	}

	// Show the statistics
	if( arg.key == OIS::KC_H )
	{
		Ogre::Overlay *mDebugPanelOverlay = Ogre::OverlayManager::getSingleton().getByName("Core/DebugOverlay");
		mDebugPanelOverlay->isVisible() ? mDebugPanelOverlay->hide() :  mDebugPanelOverlay->show();
	}

	return true;
}
//-----------------------------------------------------------------------
bool MainApp::keyReleased( const OIS::KeyEvent &arg )
{	
	
	return true;
}
//-----------------------------------------------------------------------
bool MainApp::mouseMoved(const OIS::MouseEvent &arg)
{	
	Ogre::Real mouseAbsX = arg.state.X.abs;
	Ogre::Real cenX = MainOgre::getSingleton().getWindow()->getWidth() / 2.0f;	
	mYawAngleCamera = (mouseAbsX - cenX) * -CAMERA_ROTATE_SPEED;

	return true;
}
//-----------------------------------------------------------------------
bool MainApp::mousePressed(const OIS::MouseEvent &arg ,OIS::MouseButtonID id)
{			
	if (id == OIS::MB_Left)
		mTranslateVectorCamera = Ogre::Vector3(0.0, 0.0, -1.0) * CAMERA_TRANSLATE_SPEED;
	if (id == OIS::MB_Right)
		mTranslateVectorCamera = Ogre::Vector3(0.0, 0.0, 1.0) * CAMERA_TRANSLATE_SPEED;
	
	return true;
}
//-----------------------------------------------------------------------
bool MainApp::mouseReleased(const OIS::MouseEvent &arg ,OIS::MouseButtonID id)
{		
	mTranslateVectorCamera = Ogre::Vector3::ZERO;
	return true;
}
//-----------------------------------------------------------------------
void MainApp::resize()
{
	if (mWindow)
	{
		mWindow->windowMovedOrResized();												
		InputManager::getSingletonPtr()->setWindowExtents(mVp->getActualWidth(),mVp->getActualHeight());		
	}
}