#pragma once

#include <winnt.h>
#include <profileapi.h>

class PerfTimer {
public:
	PerfTimer() {
		QueryPerformanceFrequency(&_freq_Hz);
	}

	void Start() {
		QueryPerformanceCounter(&_startTicks);
	}

	int64_t Stop() {
		QueryPerformanceCounter(&_stopTicks);
		int64_t micros = (1000000 * (_stopTicks.QuadPart - _startTicks.QuadPart) / _freq_Hz.QuadPart);
		if (_peak < micros) {
			_peak = micros;
		}
		_sum += micros;
		_n++;
		_avg = _sum / _n;
		return micros;
	}

	void ResetStats() {
		_avg = 0.0f;
		_peak = 0.0f;
		_sum = 0.0;
		_n = 0;
	}
private:
	LARGE_INTEGER _freq_Hz;
	LARGE_INTEGER _startTicks;
	LARGE_INTEGER _stopTicks;
	double		_sum = 0.0;
	uint32_t	_n = 0;

public:
	float	_avg = 0.0f;
	float   _peak = 0.0f;


};
