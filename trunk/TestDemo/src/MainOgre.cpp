#include <ogre.h>
#include <windows.h>
#include "MainOgre.h"

HWND MainOgre::mHwnd = 0;
MainOgre* MainOgre::mOgre = 0;
bool MouseCursor::mShowCursor = true;

template<> MainOgre* Ogre::Singleton<MainOgre>::ms_Singleton = 0;

//-----------------------------------------------------------------------
MainOgre* MainOgre::getSingletonPtr()
{
    return ms_Singleton;
}
//-----------------------------------------------------------------------
MainOgre& MainOgre::getSingleton()
{  
    assert( ms_Singleton );
	return ( *ms_Singleton );  
}
//-----------------------------------------------------------------------
bool MainOgre::Render(){
 	
	if (!mRoot->_fireFrameStarted())
		return false;
	
    mRoot->_updateAllRenderTargets();
	
    if (!mRoot->_fireFrameEnded())
		return false;	
		
	return true;	
}
//-----------------------------------------------------------------------
MainOgre::MainOgre(HWND hwnd)
{	
	mHwnd = hwnd;
}
//-----------------------------------------------------------------------
MainOgre::~MainOgre()
{

#ifdef _STEREO
	delete StereoCamera::getSingletonPtr();
#endif			

	delete mInputMgr;	
	delete mRoot;

}
//-----------------------------------------------------------------------
int MainOgre::Init(){
	
	mRoot = new Ogre::Root();	

	// Load resource paths from config file 
	Ogre::ConfigFile cf; 
	cf.load("resources.cfg"); 

	// Go through all sections & settings in the file 
	Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator(); 

	Ogre::String secName, typeName, archName; 
	while (seci.hasMoreElements()) 
	{ 
		secName = seci.peekNextKey(); 
		Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext(); 
		Ogre::ConfigFile::SettingsMultiMap::iterator i; 
		for (i = settings->begin(); i != settings->end(); ++i) 
		{ 
			typeName = i->first; 
			archName = i->second; 
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation( 
				archName, typeName, secName); 
		} 
	}

	// Setup RenderSystem
	Ogre::RenderSystem *currentRenderSystem;
	Ogre::RenderSystemList *rl = Ogre::Root::getSingleton().getAvailableRenderers();
	std::string str;
	for (Ogre::RenderSystemList::iterator it = rl->begin(); it != rl->end(); ++it) {
		currentRenderSystem = (*it);
		str = currentRenderSystem->getName().c_str();
		//if (str.find("D3D9") != std::string::npos) break;	//Change GL to D3D9 for DX9

	}	
	
	Ogre::Root::getSingleton().setRenderSystem(currentRenderSystem); 		

	//Setup Ogre in the existing mWindow
	Ogre::NameValuePairList miscParams;
	mRoot->initialise(false);

	mRoot->setFrameSmoothingPeriod(0.5f);
	miscParams["externalWindowHandle"] = Ogre::StringConverter::toString((UINT)mHwnd); 
	miscParams["vsync"] = "true";	
	
#ifdef _STEREO
	try
	{
		mWindow = Ogre::Root::getSingleton().createRenderWindow("MainWindow", 2048, 768, true, &miscParams);
	}
	catch(...)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage("Please check your stereo display settings - 'Horizontal span' mode is probably not enabled !!");
		MessageBox(mHwnd,"Please check your stereo display settings - 'Horizontal span' mode is probably not enabled !!","Stereo Display Settings Error",MB_ICONERROR);
		exit(1);
	}
#else

	//mWindow = Ogre::Root::getSingleton().createRenderWindow("MainWindow", 1024, 768, true, &miscParams);	
	mWindow = Ogre::Root::getSingleton().createRenderWindow("MainWindow", 800, 600, false, &miscParams);	
		
#endif	

	RECT rc;
	GetClientRect(MainOgre::mHwnd,&rc);
	int width	= rc.right - rc.left;
	int height	= rc.bottom - rc.top;
	float aspectRatio = (float)width / (float)height;

	mWindow->setActive(true);	
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	
	mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC);

	//Init InputManager
	mInputMgr = InputManager::getSingletonPtr();
    mInputMgr->initialise(mWindow);// , MainOgre::mHwnd);	
	ShowCursor(true);	

	return 0;
}