#pragma once

#include <Windows.h>

class IApplication
{
public:
	virtual ~IApplication() = default;

	virtual void Update(float DeltaTime) = 0;
	virtual void Render() = 0;
	virtual void Shutdown() = 0;
	virtual void OnWindowSize(UINT Width, UINT Height) = 0;
};
