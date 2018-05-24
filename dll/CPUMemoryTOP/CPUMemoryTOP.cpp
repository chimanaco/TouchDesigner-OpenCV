/* Shared Use License: This file is owned by Derivative Inc. (Derivative) and
 * can only be used, and/or modified for use, in conjunction with 
 * Derivative's TouchDesigner software, and only if you are a licensee who has
 * accepted Derivative's TouchDesigner license or assignment agreement (which
 * also govern the use of this file).  You may share a modified version of this
 * file with another authorized licensee of Derivative's TouchDesigner software.
 * Otherwise, no redistribution or sharing of this file, with or without
 * modification, is permitted.
 */

#include "CPUMemoryTOP.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <cmath>

// These functions are basic C function, which the DLL loader can find
// much easier than finding a C++ Class.
// The DLLEXPORT prefix is needed so the compile exports these functions from the .dll
// you are creating
extern "C"
{

DLLEXPORT
TOP_PluginInfo
GetTOPPluginInfo(void)
{
	TOP_PluginInfo info;
	// This must always be set to this constant
	info.apiVersion = TOPCPlusPlusAPIVersion;

	// Change this to change the executeMode behavior of this plugin.
	info.executeMode = TOP_ExecuteMode::CPUMemWriteOnly;

	return info;
}

DLLEXPORT
TOP_CPlusPlusBase*
CreateTOPInstance(const OP_NodeInfo* info, TOP_Context* context)
{
	// Return a new instance of your class every time this is called.
	// It will be called once per TOP that is using the .dll
	return new CPUMemoryTOP(info);
}

DLLEXPORT
void
DestroyTOPInstance(TOP_CPlusPlusBase* instance, TOP_Context *context)
{
	// Delete the instance here, this will be called when
	// Touch is shutting down, when the TOP using that instance is deleted, or
	// if the TOP loads a different DLL
	delete (CPUMemoryTOP*)instance;
}

};


CPUMemoryTOP::CPUMemoryTOP(const OP_NodeInfo* info) : myNodeInfo(info)
{
	myExecuteCount = 0;
	myStep = 0.0;
}

CPUMemoryTOP::~CPUMemoryTOP()
{

}

void
CPUMemoryTOP::getGeneralInfo(TOP_GeneralInfo* ginfo)
{
	// Uncomment this line if you want the TOP to cook every frame even
	// if none of it's inputs/parameters are changing.
	ginfo->cookEveryFrame = true;
    ginfo->memPixelType = OP_CPUMemPixelType::RGBA32Float;
}

bool
CPUMemoryTOP::getOutputFormat(TOP_OutputFormat* format)
{
	// In this function we could assign variable values to 'format' to specify
	// the pixel format/resolution etc that we want to output to.
	// If we did that, we'd want to return true to tell the TOP to use the settings we've
	// specified.
	// In this example we'll return false and use the TOP's settings
	return false;
}

void
CPUMemoryTOP::execute(const TOP_OutputFormatSpecs* outputFormat,
						OP_Inputs* inputs,
						TOP_Context *context)
{
	myExecuteCount++;


	double speed = inputs->getParDouble("Speed");
	myStep += speed;

	float brightness = (float)inputs->getParDouble("Brightness");

	int xstep = (int)(fmod(myStep, outputFormat->width));
	int ystep = (int)(fmod(myStep, outputFormat->height));

	if (xstep < 0)
		xstep += outputFormat->width;

	if (ystep < 0)
		ystep += outputFormat->height;


    int textureMemoryLocation = 0;

    float* mem = (float*)outputFormat->cpuPixelData[textureMemoryLocation];

    for (int y = 0; y < outputFormat->height; ++y)
    {
        for (int x = 0; x < outputFormat->width; ++x)
        {
            float* pixel = &mem[4*(y*outputFormat->width + x)];

			// RGBA
            pixel[0] = (x > xstep) * brightness;
            pixel[1] = (y > ystep) * brightness;
            pixel[2] = ((float) (xstep % 50) / 50.0f) * brightness;
            pixel[3] = 1;
        }
    }

    outputFormat->newCPUPixelDataLocation = textureMemoryLocation;
    textureMemoryLocation = !textureMemoryLocation;
}

int32_t
CPUMemoryTOP::getNumInfoCHOPChans()
{
	// We return the number of channel we want to output to any Info CHOP
	// connected to the TOP. In this example we are just going to send one channel.
	return 2;
}

void
CPUMemoryTOP::getInfoCHOPChan(int32_t index, OP_InfoCHOPChan* chan)
{
	// This function will be called once for each channel we said we'd want to return
	// In this example it'll only be called once.

	if (index == 0)
	{
		chan->name = "executeCount";
		chan->value = (float)myExecuteCount;
	}

	if (index == 1)
	{
		chan->name = "step";
		chan->value = (float)myStep;
	}
}

bool		
CPUMemoryTOP::getInfoDATSize(OP_InfoDATSize* infoSize)
{
	infoSize->rows = 2;
	infoSize->cols = 2;
	// Setting this to false means we'll be assigning values to the table
	// one row at a time. True means we'll do it one column at a time.
	infoSize->byColumn = false;
	return true;
}

void
CPUMemoryTOP::getInfoDATEntries(int32_t index,
								int32_t nEntries,
								OP_InfoDATEntries* entries)
{
	// It's safe to use static buffers here because Touch will make it's own
	// copies of the strings immediately after this call returns
	// (so the buffers can be reuse for each column/row)
	static char tempBuffer1[4096];
	static char tempBuffer2[4096];

	if (index == 0)
	{
        // Set the value for the first column
#ifdef WIN32
        strcpy_s(tempBuffer1, "executeCount");
#else // macOS
        strlcpy(tempBuffer1, "executeCount", sizeof(tempBuffer1));
#endif
        entries->values[0] = tempBuffer1;

        // Set the value for the second column
#ifdef WIN32
        sprintf_s(tempBuffer2, "%d", myExecuteCount);
#else // macOS
        snprintf(tempBuffer2, sizeof(tempBuffer2), "%d", myExecuteCount);
#endif
        entries->values[1] = tempBuffer2;
	}

	if (index == 1)
	{
#ifdef WIN32
		strcpy_s(tempBuffer1, "step");
#else // macOS
        strlcpy(tempBuffer1, "step", sizeof(tempBuffer1));
#endif
		entries->values[0] = tempBuffer1;

#ifdef WIN32
		sprintf_s(tempBuffer2, "%g", myStep);
#else // macOS
        snprintf(tempBuffer2, sizeof(tempBuffer2), "%g", myStep);
#endif
		entries->values[1] = tempBuffer2;
	}
}

void
CPUMemoryTOP::setupParameters(OP_ParameterManager* manager)
{
	// brightness
	{
		OP_NumericParameter	np;

		np.name = "Brightness";
		np.label = "Brightness";
		np.defaultValues[0] = 1.0;

		np.minSliders[0] =  0.0;
		np.maxSliders[0] =  1.0;

		np.minValues[0] = 0.0;
		np.maxValues[0] = 1.0;

		np.clampMins[0] = true;
		np.clampMaxes[0] = true;
		
		OP_ParAppendResult res = manager->appendFloat(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// speed
	{
		OP_NumericParameter	np;

		np.name = "Speed";
		np.label = "Speed";
		np.defaultValues[0] = 1.0;
		np.minSliders[0] = -10.0;
		np.maxSliders[0] =  10.0;
		
		OP_ParAppendResult res = manager->appendFloat(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// pulse
	{
		OP_NumericParameter	np;

		np.name = "Reset";
		np.label = "Reset";
		
		OP_ParAppendResult res = manager->appendPulse(np);
		assert(res == OP_ParAppendResult::Success);
	}

}

void
CPUMemoryTOP::pulsePressed(const char* name)
{
	if (!strcmp(name, "Reset"))
	{
		myStep = 0.0;
	}


}

