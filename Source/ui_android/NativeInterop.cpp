#include <jni.h>
#include <cassert>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "string_format.h"
#include "PathUtils.h"
#include "../AppConfig.h"
#include "../DiskUtils.h"
#include "../PH_Generic.h"
#include "../PS2VM.h"
#include "../PS2VM_Preferences.h"
#include "../gs/GSH_Null.h"
#include "NativeShared.h"
#include "GSH_OpenGLAndroid.h"
#include "StatsManager.h"

CPS2VM* g_virtualMachine = nullptr;

static boost::filesystem::path GetStateDirectoryPath()
{
	return CAppConfig::GetBasePath() / boost::filesystem::path("states/");
}

static boost::filesystem::path GenerateStatePath(int slot)
{
	auto stateDirPath = GetStateDirectoryPath();
	auto stateFileName = string_format("%s.st%d", g_virtualMachine->m_ee->m_os->GetExecutableName(), slot);
	return stateDirPath / stateFileName;
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_setFilesDirPath(JNIEnv* env, jobject obj, jstring dirPathString)
{
	auto dirPath = env->GetStringUTFChars(dirPathString, 0);
	Framework::PathUtils::SetFilesDirPath(dirPath);
	env->ReleaseStringUTFChars(dirPathString, dirPath);
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_createVirtualMachine(JNIEnv* env, jobject obj)
{
	assert(g_virtualMachine == nullptr);
	g_virtualMachine = new CPS2VM();
	g_virtualMachine->Initialize();
	g_virtualMachine->CreatePadHandler(CPH_Generic::GetFactoryFunction());
#ifdef PROFILE
	g_virtualMachine->ProfileFrameDone.connect(boost::bind(&CStatsManager::OnProfileFrameDone, &CStatsManager::GetInstance(), _1));
#endif
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_virtualapplications_play_NativeInterop_isVirtualMachineCreated(JNIEnv* env, jobject obj)
{
	return (g_virtualMachine != nullptr);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_virtualapplications_play_NativeInterop_isVirtualMachineRunning(JNIEnv* env, jobject obj)
{
	if(g_virtualMachine == nullptr) return false;
	return g_virtualMachine->GetStatus() == CVirtualMachine::RUNNING;
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_resumeVirtualMachine(JNIEnv* env, jobject obj)
{
	assert(g_virtualMachine != nullptr);
	if(g_virtualMachine == nullptr) return;
	g_virtualMachine->Resume();
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_pauseVirtualMachine(JNIEnv* env, jobject obj)
{
	assert(g_virtualMachine != nullptr);
	if(g_virtualMachine == nullptr) return;
	g_virtualMachine->Pause();
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_loadState(JNIEnv* env, jobject obj, jint slot)
{
	assert(g_virtualMachine != nullptr);
	if(g_virtualMachine == nullptr) return;
	auto stateFilePath = GenerateStatePath(slot);
	if(g_virtualMachine->LoadState(stateFilePath.string().c_str()) != 0)
	{
		jclass exceptionClass = env->FindClass("java/lang/Exception");
		env->ThrowNew(exceptionClass, "LoadState failed.");
		return;
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_saveState(JNIEnv* env, jobject obj, jint slot)
{
	assert(g_virtualMachine != nullptr);
	if(g_virtualMachine == nullptr) return;
	Framework::PathUtils::EnsurePathExists(GetStateDirectoryPath());
	auto stateFilePath = GenerateStatePath(slot);
	if(g_virtualMachine->SaveState(stateFilePath.string().c_str()) != 0)
	{
		jclass exceptionClass = env->FindClass("java/lang/Exception");
		env->ThrowNew(exceptionClass, "SaveState failed.");
		return;
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_loadElf(JNIEnv* env, jobject obj, jstring selectedFilePath)
{
	assert(g_virtualMachine != nullptr);
	g_virtualMachine->Pause();
	g_virtualMachine->Reset();
	try
	{
		g_virtualMachine->m_ee->m_os->BootFromFile(GetStringFromJstring(env, selectedFilePath).c_str());
	}
	catch(const std::exception& exception)
	{
		jclass exceptionClass = env->FindClass("java/lang/Exception");
		env->ThrowNew(exceptionClass, exception.what());
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_bootDiskImage(JNIEnv* env, jobject obj, jstring selectedFilePath)
{
	assert(g_virtualMachine != nullptr);
	CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, GetStringFromJstring(env, selectedFilePath).c_str());
	g_virtualMachine->Pause();
	g_virtualMachine->Reset();
	try
	{
		g_virtualMachine->m_ee->m_os->BootFromCDROM(CPS2OS::ArgumentList());
	}
	catch(const std::exception& exception)
	{
		jclass exceptionClass = env->FindClass("java/lang/Exception");
		env->ThrowNew(exceptionClass, exception.what());
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_setupGsHandler(JNIEnv* env, jobject obj, jobject surface)
{
	auto nativeWindow = ANativeWindow_fromSurface(env, surface);
	auto gsHandler = g_virtualMachine->GetGSHandler();
	if(gsHandler == nullptr)
	{
		g_virtualMachine->CreateGSHandler(CGSH_OpenGLAndroid::GetFactoryFunction(nativeWindow));
		g_virtualMachine->m_ee->m_gs->OnNewFrame.connect(
			boost::bind(&CStatsManager::OnNewFrame, &CStatsManager::GetInstance(), _1));
	}
	else
	{
		static_cast<CGSH_OpenGLAndroid*>(gsHandler)->SetWindow(nativeWindow);
	}
}

extern "C" JNIEXPORT jstring JNICALL Java_com_virtualapplications_play_NativeInterop_getDiskId(JNIEnv* env, jobject obj, jstring diskImagePath)
{
	std::string diskId;
	bool succeeded = DiskUtils::TryGetDiskId(GetStringFromJstring(env, diskImagePath).c_str(), &diskId);
	if(!succeeded)
	{
		return NULL;
	}
	jstring result = env->NewStringUTF(diskId.c_str());
	return result;
}
