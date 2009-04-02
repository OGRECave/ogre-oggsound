#ifndef __OGRE_H__
#define __OGRE_H__

#include <Windows.h>
#include <Ogre.h>
#include "InputManager.h"

#ifdef _STEREO
#include "StereoCamera.h"
#endif


class MainOgre : public Ogre::Singleton<MainOgre>
{
private:
	Ogre::Root* mRoot;
	Ogre::SceneManager* mSceneMgr;
	Ogre::RenderWindow *mWindow;	
	InputManager *mInputMgr;
	static MainOgre *mOgre;	

public:
	MainOgre(HWND hwnd);
	~MainOgre();
	bool Render();
	int Init();
	void Shutdown();	
	static HWND mHwnd;		
	static MainOgre& getSingleton();
	static MainOgre* getSingletonPtr();			
	Ogre::RenderWindow *getWindow(){return mWindow;};
	Ogre::SceneManager *getSceneManager(){return mSceneMgr;};	
	#ifdef _STEREO
	StereoCamera *mStereoCamera;
	#endif
};


class MouseCursor
{
private:
	static bool mShowCursor;
    
public:
	static void Show()
	{
		if (!mShowCursor)
		{
			ShowCursor(true);
			mShowCursor = true;
		}
	};
	
	static void Hide()
	{
		if (mShowCursor)
		{
			ShowCursor(false);
			mShowCursor = false;
		}	
	};
};

#endif