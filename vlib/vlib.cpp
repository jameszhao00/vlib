// vlib.cpp : Defines the entry point for the console application.
#include "pch.h"
#include "rest_temp.h"
#include "GfxData.h"
#include "OGL.h"
#include "Gfx.h"
int main() {
	Gfx::DeviceInterface<OpenGLApi> dev;
	
    main_loop();
    glfwTerminate();
    return 0;
}
