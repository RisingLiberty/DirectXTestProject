#include "GameTimer.h"

#include <Windows.h>

GameTimer::GameTimer():
	m_SecondsPerCount(0.0),
	m_DeltaTime(-1.0),
	m_BaseTime(0),
	m_PausedTime(0),
	m_PrevTime(0),
	m_CurrTime(0),
	m_IsStopped(false)
{
	//The perfromance timer measure time in units called counts. We obtain the current time value,
	//measured in counts, of the performance timer with the QueryPerformanceCounter function.
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	//To get the frequency (counts per second) of the performance timer, we use the QueryPerformanceFrequency function
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);

	m_SecondsPerCount = 1.0 / (double)countsPerSec;

	//So in the future to get the time elapsed:
	//valueInSecs = valueInCounts * m_SecondsPerCount;

	/*
	MSDN has the following remark about QueryPerformanceCounter:
	�On a multiprocessor computer, it should not matter which processor is called.
	However, you can get different results on different processors due to bugs in the
	basic input/output system (BIOS) or the hardware abstraction layer (HAL).� You
	can use the SetThreadAffinityMask function so that the main application
	thread does not get switch to another processor.
	*/
}

void GameTimer::Start()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	//Accumulate the time elapsed betwee stop and start pairs.
	//
	//						|<--------d--------->|
	//------*---------------*--------------------*-------------> time
	//  m_BaseTime      m_StopTime            startTime

	if (m_IsStopped)
	{
		m_PausedTime += (currTime - m_StopTime);

		m_PrevTime = currTime;
		m_StopTime = 0;
		m_IsStopped = false;
	}
	
}

void GameTimer::Stop()
{
	if (!m_IsStopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		m_StopTime = currTime;
		m_IsStopped = true;
	}
}

void GameTimer::Tick()
{
	if (m_IsStopped)
	{
		m_DeltaTime = 0.0;
		return;
	}

	//Get the time this frame.
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_CurrTime = currTime;

	//Time difference between this frame and the previous
	m_DeltaTime = (m_CurrTime - m_PrevTime) * m_SecondsPerCount;

	//Prepare for next frame
	m_PrevTime = m_CurrTime;

	//Force nonnegative. The DXSDK's CDXUTTimer mentions that if the processor goes into a power save mode
	//or we get shuffled to another processor, then m_DeltaTime can be negative.
	if (m_DeltaTime < 0.0)
		m_DeltaTime = 0.0;

}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)(&currTime));

	m_BaseTime = currTime;
	m_PrevTime = currTime;
	m_StopTime = 0;
	m_IsStopped = false;

}

//Returns the total time elapsed since Reset was called,
//NOT including any time when the clock has stopped
float GameTimer::GetGameTime() const
{
	//If we are stopped do not count the time that has passed since we stopped.
	//Moreover, if we previously already had a pause, the distance
	//m_StopTime - m_BaseTime includes paused time, which we do not want to count.
	//To correct this, we can subtract the paused time from m_StopTime:
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  m_BaseTime       m_StopTime        startTime     m_StopTime    m_CurrTime

	if (m_IsStopped)
		return (float)(((m_StopTime - m_PausedTime) - m_BaseTime)*m_SecondsPerCount);

	// The distance mCurrTime - mBaseTime includes paused time,
	// which we do not want to count.  To correct this, we can subtract 
	// the paused time from mCurrTime:  
	//
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime
	return (float)(((m_CurrTime - m_PausedTime) - m_BaseTime)*m_SecondsPerCount);
}

float GameTimer::GetDeltaTime() const
{
	return (float)m_DeltaTime;
}