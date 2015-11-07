#include <jni.h>
#include "MemStream.h"
#include "make_unique.h"
#include "xml/Writer.h"
#include "InputManager.h"
#include "../PH_Generic.h"
#include "../PS2VM.h"
#include "../VirtualPad.h"
#include "com_virtualapplications_play_InputManagerConstants.h"

//Checking that ids match on Java and C++ sides
#if 1
static_assert(com_virtualapplications_play_InputManagerConstants_ANALOG_LEFT_X   == PS2::CControllerInfo::ANALOG_LEFT_X,  "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_ANALOG_LEFT_Y   == PS2::CControllerInfo::ANALOG_LEFT_Y,  "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_ANALOG_RIGHT_X  == PS2::CControllerInfo::ANALOG_RIGHT_X, "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_ANALOG_RIGHT_Y  == PS2::CControllerInfo::ANALOG_RIGHT_Y, "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_UP       == PS2::CControllerInfo::DPAD_UP,        "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_DOWN     == PS2::CControllerInfo::DPAD_DOWN,      "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_LEFT     == PS2::CControllerInfo::DPAD_LEFT,      "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_RIGHT    == PS2::CControllerInfo::DPAD_RIGHT,     "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_SELECT   == PS2::CControllerInfo::SELECT,         "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_START    == PS2::CControllerInfo::START,          "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_SQUARE   == PS2::CControllerInfo::SQUARE,         "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_TRIANGLE == PS2::CControllerInfo::TRIANGLE,       "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_CIRCLE   == PS2::CControllerInfo::CIRCLE,         "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_CROSS    == PS2::CControllerInfo::CROSS,          "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_L1       == PS2::CControllerInfo::L1,             "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_L2       == PS2::CControllerInfo::L2,             "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_L3       == PS2::CControllerInfo::L3,             "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_R1       == PS2::CControllerInfo::R1,             "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_R2       == PS2::CControllerInfo::R2,             "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_BUTTON_R3       == PS2::CControllerInfo::R3,             "Input code mismatch");
static_assert(com_virtualapplications_play_InputManagerConstants_MAX             == PS2::CControllerInfo::MAX_BUTTONS,    "Input code mismatch");
#endif

extern CPS2VM* g_virtualMachine;

void CInputManager::SetButtonState(int buttonId, bool pressed)
{
	auto padHandler = g_virtualMachine->GetPadHandler();
	if(padHandler)
	{
		static_cast<CPH_Generic*>(padHandler)->SetButtonState(buttonId, pressed);
	}
}

void CInputManager::SetAxisState(int buttonId, float value)
{
	auto padHandler = g_virtualMachine->GetPadHandler();
	if(padHandler)
	{
		static_cast<CPH_Generic*>(padHandler)->SetAxisState(buttonId, value);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_InputManager_setButtonState(JNIEnv* env, jobject obj, jint buttonId, jboolean pressed)
{
	CInputManager::GetInstance().SetButtonState(buttonId, (pressed == JNI_TRUE));
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_InputManager_setAxisState(JNIEnv* env, jobject obj, jint buttonId, jfloat value)
{
	CInputManager::GetInstance().SetAxisState(buttonId, value);
}

extern "C" JNIEXPORT jstring JNICALL Java_com_virtualapplications_play_InputManager_getVirtualPadItems(JNIEnv* env, jobject obj, jfloat screenWidth, jfloat screenHeight)
{
	auto items = CVirtualPad::GetItems(screenWidth, screenHeight);
	auto documentNode = std::make_unique<Framework::Xml::CNode>("Document", true);
	for(const auto& item : items)
	{
		auto itemNode = new Framework::Xml::CNode("Item", true);
		itemNode->InsertAttribute("isAnalog", item.isAnalog ? "true" : "false");
		itemNode->InsertAttribute("x1", std::to_string(item.x1).c_str());
		itemNode->InsertAttribute("y1", std::to_string(item.y1).c_str());
		itemNode->InsertAttribute("x2", std::to_string(item.x2).c_str());
		itemNode->InsertAttribute("y2", std::to_string(item.y2).c_str());
		itemNode->InsertAttribute("code0", std::to_string(item.code0).c_str());
		itemNode->InsertAttribute("code1", std::to_string(item.code1).c_str());
		itemNode->InsertAttribute("caption", item.caption.c_str());
		itemNode->InsertAttribute("imageName", item.imageName.c_str());
		documentNode->InsertNode(itemNode);
	}
	Framework::CMemStream outputStream;
	Framework::Xml::CWriter::WriteDocument(outputStream, documentNode.get());
	auto resultString = std::string(outputStream.GetBuffer(), outputStream.GetBuffer() + outputStream.GetSize());
	jstring result = env->NewStringUTF(resultString.c_str());
	return result;
}
