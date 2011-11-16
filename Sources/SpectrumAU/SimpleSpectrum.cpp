//
//  Spectrum.cpp
//  SimpleSprectrumAnalyzer
//
//  Created by Fred G on 05/11/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "SimpleSpectrum.h"

#define kMaxBlockSize 16384

#pragma mark ____SimpleSpectrumKernel
SimpleSpectrumKernel::SimpleSpectrumKernel(AUEffectBase *inAudioUnit ) : AUKernelBase(inAudioUnit) 
{
	Reset();
}

SimpleSpectrumKernel::~SimpleSpectrumKernel() 
{
    
}

void SimpleSpectrumKernel::Reset() 
{
	// TODO : Reset stuffs here
}

void SimpleSpectrumKernel::Process(const Float32 	*inSourceP,
                             Float32 		*inDestP,
                             UInt32 	    inFramesToProcess,
                             UInt32			inNumChannels,
                             bool &			ioSilence) 
{
	// input buses passthrough are done in SimpleSpectrum::Render
	//memcpy((void*)inSourceP, (void*)inDestP, inFramesToProcess*sizeof(Float32));
}

#pragma mark ____SimpleSpectrum
SimpleSpectrum::SimpleSpectrum(AudioUnit component) : AUEffectBase(component)
{
	// all the parameters must be set to their initial values here
	//
	// these calls have the effect both of defining the parameters for the first time
	// and assigning their initial values
	//
    SetParameter(kSpectrumParam_BlockSize, kBlockSize_Default);
    SetParameter(kSpectrumParam_SelectChannel, kSelectChannel_Default);
    SetParameter(kSpectrumParam_Window, kWindow_Hann);
    
	SetParamHasSampleRateDependency(false);
}

OSStatus SimpleSpectrum::Initialize() 
{
	OSStatus result = AUEffectBase::Initialize();
	
	if(result == noErr ) {
        mProcessor.Allocate(GetNumberOfChannels(), kMaxBlockSize);
        
		// update UI by sending audio channel layout
        //PropertyChanged(kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Global, 0);
	}
	
	return result;
}

SimpleSpectrum::~SimpleSpectrum()
{
	Cleanup();
}

void SimpleSpectrum::Cleanup() 
{
    
}

OSStatus SimpleSpectrum::Render(AudioUnitRenderActionFlags & ioActionFlags,
                          AudioTimeStamp const&	inTimeStamp,
                        UInt32 inFramesToProcess ) 
{	
    UInt32 actionFlags = 0;
	OSStatus err = PullInput(0, actionFlags, inTimeStamp, inFramesToProcess);
	if (err) return err;
	
	AUInputElement* inputBus = GetInput(0);
	AUOutputElement* outputBus = GetOutput(0);
    
	outputBus->PrepareBuffer(inFramesToProcess); // prepare the output buffer list	
	AudioBufferList& inputBufList = inputBus->GetBufferList();
    
    mProcessor.CopyInputToRingBuffer(inFramesToProcess, &inputBufList);
   
    UInt32 currentBlockSize;
    switch ((UInt32) GetParameter(kSpectrumParam_BlockSize)) {
        case kBlockSize_Option5:
            currentBlockSize = 16384;
            break;
        case kBlockSize_Option4:
            currentBlockSize = 8192;
            break;
        case kBlockSize_Option3:
            currentBlockSize = 4096;
            break;
        case kBlockSize_Option2:
            currentBlockSize = 2048;
            break;
        case kBlockSize_Option1:
        default:
            currentBlockSize = 1024;
            break;
    }
    
    SimpleSpectrumProcessor::Window currentWindow;
    switch ((UInt32) GetParameter(kSpectrumParam_Window)) {
        case kWindow_Rectangular:
            currentWindow = SimpleSpectrumProcessor::Window::Rectangular;
            break;
        case kWindow_Hamming:
            currentWindow = SimpleSpectrumProcessor::Window::Hamming;
            break;
        case kWindow_Blackman:
            currentWindow = SimpleSpectrumProcessor::Window::Blackman;
            break;
        case kWindow_Hann:
        default:
            currentWindow = SimpleSpectrumProcessor::Window::Hann;
            break;
    }
    
    if(mProcessor.TryFFT(currentBlockSize, currentWindow)) {
        UInt32 mBins = currentBlockSize>>1;
        Float32 leftChannelMagnitudes[mBins], min, max;
        for(UInt32 i = 0; i < mBins; ++i)
            leftChannelMagnitudes[i] = 0.;
                    
    }

	return AUEffectBase::Render(ioActionFlags, inTimeStamp, inFramesToProcess);
}

OSStatus SimpleSpectrum::GetProperty(AudioUnitPropertyID  inID, 
                               AudioUnitScope		inScope,
                               AudioUnitElement		inElement,
                               void *				outData) {
	if (inScope == kAudioUnitScope_Global)
	{
		switch (inID) {
            // This property allows the host application to find the UI associated with this
            // AudioUnit
			case kAudioUnitProperty_CocoaUI:
			{
				// Look for a resource in the main bundle by name and type.
				CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.fred.SimpleSpectrumAnalyzer"));
				
				if (bundle == NULL) return fnfErr;
                
				CFURLRef bundleURL = CFBundleCopyResourceURL( bundle, 
                                                             CFSTR("SpectrumCocoaView"),
                                                             CFSTR("bundle"),
                                                             NULL);
                
                if (bundleURL == NULL) return fnfErr;
                
				CFStringRef className = CFSTR("SimpleSpectrum_ViewFactory"); // name of the main class that implements the AUCocoaUIBase protocol
				AudioUnitCocoaViewInfo cocoaInfo = { bundleURL, className };
				*((AudioUnitCocoaViewInfo *)outData) = cocoaInfo;
				
				return noErr;
			}
		}
	}
	
	// if we've gotten this far, handles the standard properties
	return AUEffectBase::GetProperty (inID, inScope, inElement, outData);
}

OSStatus SimpleSpectrum::GetPropertyInfo(AudioUnitPropertyID	inID,
                                  AudioUnitScope		inScope,
                                  AudioUnitElement		inElement,
                                  UInt32 &				outDataSize,
                                  Boolean &				outWritable) 
{
	if (inScope == kAudioUnitScope_Global) 	{
		switch (inID) {
			case kAudioUnitProperty_CocoaUI:
				outWritable = false;
				outDataSize = sizeof (AudioUnitCocoaViewInfo);
				return noErr;
		}
	}
	
	return AUEffectBase::GetPropertyInfo (inID, inScope, inElement, outDataSize, outWritable);
}

OSStatus SimpleSpectrum::GetParameterValueStrings(AudioUnitScope inScope, 
                                                  AudioUnitParameterID inParameterID, 
                                                  CFArrayRef *outStrings) 
{
    if (inScope == kAudioUnitScope_Global) {
		switch(inParameterID)
		{
            case kSpectrumParam_BlockSize:
            {
                if (outStrings == NULL) return noErr;   
                
                CFStringRef options [] = {
                    kBlockSize_Option1_Name,
                    kBlockSize_Option2_Name,
                    kBlockSize_Option3_Name,
                    kBlockSize_Option4_Name,
                    kBlockSize_Option5_Name
                };
                
                *outStrings = CFArrayCreate (NULL,
                                             (const void **) options,
                                             (sizeof (options) / sizeof (options[0])),
                                             NULL
                                             );
                return noErr;                
                break;
            }
            case kSpectrumParam_SelectChannel:
            {
                if (outStrings == NULL) return noErr;   
                
                CFStringRef options [] = {
                    kSelectChannel_Left_Name,
                    kSelectChannel_Right_Name,
                    kSelectChannel_Both_Name
                };
                
                *outStrings = CFArrayCreate (NULL,
                                             (const void **) options,
                                             (sizeof (options) / sizeof (options[0])),
                                             NULL
                                             );
                return noErr;                
                break;
            }
            case kSpectrumParam_Window:
            {
                if (outStrings == NULL) return noErr;   
                
                CFStringRef options [] = {
                    kWindow_Rectangular_Name,
                    kWindow_Hann_Name,
                    kWindow_Hamming_Name,
                    kWindow_Blackman_Name
                };
                
                *outStrings = CFArrayCreate (NULL,
                                             (const void **) options,
                                             (sizeof (options) / sizeof (options[0])),
                                             NULL
                                             );
                return noErr;                
                break;
            }
        }
	}
    
	return kAudioUnitErr_InvalidParameter;
}

OSStatus SimpleSpectrum::GetParameterInfo(AudioUnitScope inScope,
                                    AudioUnitParameterID inParameterID,
                                    AudioUnitParameterInfo & outParameterInfo ) 
{
	OSStatus result = noErr;
    
	outParameterInfo.flags = 	
        kAudioUnitParameterFlag_IsWritable + 
        kAudioUnitParameterFlag_IsReadable;
    
	if (inScope == kAudioUnitScope_Global) {
		switch(inParameterID)
		{
			case kSpectrumParam_BlockSize:
				AUBase::FillInParameterName (outParameterInfo, kBlockSize_Name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
				outParameterInfo.minValue = kBlockSize_Option1;
				outParameterInfo.maxValue = kBlockSize_Option5;
				outParameterInfo.defaultValue = kBlockSize_Default;
				break;
				
            case kSpectrumParam_SelectChannel:
                AUBase::FillInParameterName (outParameterInfo, kSelectChannel_Name, false);
                outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
                outParameterInfo.minValue = kSelectChannel_Left;
                outParameterInfo.maxValue = kSelectChannel_Both;
                outParameterInfo.defaultValue = kSelectChannel_Default;
                break;
                
			case kSpectrumParam_Window:
				AUBase::FillInParameterName (outParameterInfo, kWindow_Name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
				outParameterInfo.minValue = kWindow_Rectangular;
				outParameterInfo.maxValue = kWindow_Blackman;
				outParameterInfo.defaultValue = kWindow_Default;
				break;
				
			default:
				result = kAudioUnitErr_InvalidParameter;
				break;
		}
	} else {
		result = kAudioUnitErr_InvalidParameter;
	}
	
	return result;
}