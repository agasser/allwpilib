// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "WPIUtilJNI.h"

#include <jni.h>

#include <fmt/format.h>

#include "edu_wpi_first_util_WPIUtilJNI.h"
#include "wpi/Synchronization.h"
#include "wpi/jni_util.h"
#include "wpi/timestamp.h"

using namespace wpi::java;

static bool mockTimeEnabled = false;
static uint64_t mockNow = 0;

static JException illegalArgEx;
static JException indexOobEx;
static JException interruptedEx;
static JException nullPointerEx;

static const JExceptionInit exceptions[] = {
    {"java/lang/IllegalArgumentException", &illegalArgEx},
    {"java/lang/IndexOutOfBoundsException", &indexOobEx},
    {"java/lang/InterruptedException", &interruptedEx},
    {"java/lang/NullPointerException", &nullPointerEx}};

void wpi::ThrowIllegalArgumentException(JNIEnv* env, std::string_view msg) {
  illegalArgEx.Throw(env, msg);
}

void wpi::ThrowIndexOobException(JNIEnv* env, std::string_view msg) {
  indexOobEx.Throw(env, msg);
}

void wpi::ThrowNullPointerException(JNIEnv* env, std::string_view msg) {
  nullPointerEx.Throw(env, msg);
}

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
  JNIEnv* env;
  if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
    return JNI_ERR;
  }

  for (auto& c : exceptions) {
    *c.cls = JException(env, c.name);
    if (!*c.cls) {
      return JNI_ERR;
    }
  }

  return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
  JNIEnv* env;
  if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
    return;
  }

  for (auto& c : exceptions) {
    c.cls->free(env);
  }
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    writeStderr
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_writeStderr
  (JNIEnv* env, jclass, jstring str)
{
  fmt::print(stderr, "{}", JStringRef{env, str}.str());
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    enableMockTime
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_enableMockTime
  (JNIEnv*, jclass)
{
#ifdef __FRC_ROBORIO__
  fmt::print(stderr, "WPIUtil: Mocking time is not available on the Rio\n");
#else
  mockTimeEnabled = true;
  wpi::SetNowImpl([] { return mockNow; });
#endif
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    disableMockTime
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_disableMockTime
  (JNIEnv*, jclass)
{
  mockTimeEnabled = false;
  wpi::SetNowImpl(nullptr);
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    setMockTime
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_setMockTime
  (JNIEnv*, jclass, jlong time)
{
  mockNow = time;
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    now
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_now
  (JNIEnv*, jclass)
{
  if (mockTimeEnabled) {
    return mockNow;
  } else {
    return wpi::Now();
  }
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    getSystemTime
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_getSystemTime
  (JNIEnv*, jclass)
{
  return wpi::GetSystemTime();
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    createEvent
 * Signature: (ZZ)I
 */
JNIEXPORT jint JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_createEvent
  (JNIEnv*, jclass, jboolean manualReset, jboolean initialState)
{
  return wpi::CreateEvent(manualReset, initialState);
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    destroyEvent
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_destroyEvent
  (JNIEnv*, jclass, jint eventHandle)
{
  wpi::DestroyEvent(eventHandle);
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    setEvent
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_setEvent
  (JNIEnv*, jclass, jint eventHandle)
{
  wpi::SetEvent(eventHandle);
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    resetEvent
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_resetEvent
  (JNIEnv*, jclass, jint eventHandle)
{
  wpi::ResetEvent(eventHandle);
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    createSemaphore
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_createSemaphore
  (JNIEnv*, jclass, jint initialCount, jint maximumCount)
{
  return wpi::CreateSemaphore(initialCount, maximumCount);
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    destroySemaphore
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_destroySemaphore
  (JNIEnv*, jclass, jint semHandle)
{
  wpi::DestroySemaphore(semHandle);
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    releaseSemaphore
 * Signature: (II)Z
 */
JNIEXPORT jboolean JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_releaseSemaphore
  (JNIEnv*, jclass, jint semHandle, jint releaseCount)
{
  return wpi::ReleaseSemaphore(semHandle, releaseCount);
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    waitForObject
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_waitForObject
  (JNIEnv* env, jclass, jint handle)
{
  if (!wpi::WaitForObject(handle)) {
    interruptedEx.Throw(env, "WaitForObject interrupted");
  }
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    waitForObjectTimeout
 * Signature: (ID)Z
 */
JNIEXPORT jboolean JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_waitForObjectTimeout
  (JNIEnv* env, jclass, jint handle, jdouble timeout)
{
  bool timedOut;
  if (!wpi::WaitForObject(handle, timeout, &timedOut) && !timedOut) {
    interruptedEx.Throw(env, "WaitForObject interrupted");
    return false;
  }
  return timedOut;
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    waitForObjects
 * Signature: ([I)[I
 */
JNIEXPORT jintArray JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_waitForObjects
  (JNIEnv* env, jclass, jintArray handles)
{
  JSpan<const jint> handlesArr{env, handles};
  wpi::SmallVector<WPI_Handle, 8> signaledBuf;
  signaledBuf.resize(handlesArr.size());
  std::span<const WPI_Handle> handlesArr2{
      reinterpret_cast<const WPI_Handle*>(handlesArr.data()),
      handlesArr.size()};

  auto signaled = wpi::WaitForObjects(handlesArr2, signaledBuf);
  if (signaled.empty()) {
    interruptedEx.Throw(env, "WaitForObjects interrupted");
    return nullptr;
  }
  return MakeJIntArray(env, signaled);
}

/*
 * Class:     edu_wpi_first_util_WPIUtilJNI
 * Method:    waitForObjectsTimeout
 * Signature: ([ID)[I
 */
JNIEXPORT jintArray JNICALL
Java_edu_wpi_first_util_WPIUtilJNI_waitForObjectsTimeout
  (JNIEnv* env, jclass, jintArray handles, jdouble timeout)
{
  JSpan<const jint> handlesArr{env, handles};
  wpi::SmallVector<WPI_Handle, 8> signaledBuf;
  signaledBuf.resize(handlesArr.size());
  std::span<const WPI_Handle> handlesArr2{
      reinterpret_cast<const WPI_Handle*>(handlesArr.data()),
      handlesArr.size()};

  bool timedOut;
  auto signaled =
      wpi::WaitForObjects(handlesArr2, signaledBuf, timeout, &timedOut);
  if (signaled.empty() && !timedOut) {
    interruptedEx.Throw(env, "WaitForObjects interrupted");
    return nullptr;
  }
  return MakeJIntArray(env, signaled);
}

}  // extern "C"
