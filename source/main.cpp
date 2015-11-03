#include <3ds.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

int main(int argc, char* argv[])
{
	gfxInitDefault();
	
	consoleInit(GFX_TOP, nullptr);
	
	// The dsp channel number
	constexpr int channel = 1;
	
	u32 sampleRate;
	u32 dataSize;
	u16 channels;
	u16 bitsPerSample;
	
	// Initialize ndsp
	ndspInit();
	
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspSetOutputCount(1); // Num of buffers
	
	// Reading wav file
	FILE* fp = fopen("./example.wav", "rb");
	
	if(!fp)
	{
		printf("Could not open the example.wav file.\n");
		return 1;
	}
	
	char signature[4];
	
	fread(signature, 1, 4, fp);
	
	if( signature[0] != 'R' &&
		signature[1] != 'I' &&
		signature[2] != 'F' &&
		signature[3] != 'F')
	{
		printf("Wrong file format.\n");
		fclose(fp);
		return 1;
	}
	
	fseek(fp, 40, SEEK_SET);
	fread(&dataSize, 4, 1, fp);
	fseek(fp, 22, SEEK_SET);
	fread(&channels, 2, 1, fp);
	fseek(fp, 24, SEEK_SET);
	fread(&sampleRate, 4, 1, fp);
	fseek(fp, 34, SEEK_SET);
	fread(&bitsPerSample, 2, 1, fp);
	
	if(dataSize == 0 || (channels != 1 && channels != 2) ||
		(bitsPerSample != 8 && bitsPerSample != 16))
	{
		printf("Corrupted wav file.\n");
		fclose(fp);
		return 1;
	}
	
	// Allocating and reading samples
	u8* data = static_cast<u8*>(linearAlloc(dataSize));
	
	if(!data)
	{
		fclose(fp);
		return 1;
	}
	
	fseek(fp, 44, SEEK_SET);
	fread(data, 1, dataSize, fp);
	fclose(fp);
	
	// Find the right format
	u16 ndspFormat;
	
	if(bitsPerSample == 8)
	{
		ndspFormat = (channels == 1) ?
		NDSP_FORMAT_MONO_PCM8 : NDSP_FORMAT_STEREO_PCM8;
	}
	else
	{
		ndspFormat = (channels == 1) ?
		NDSP_FORMAT_MONO_PCM16 : NDSP_FORMAT_STEREO_PCM16;
	}
	
	ndspChnReset(channel);
	ndspChnSetInterp(channel, NDSP_INTERP_NONE);
	ndspChnSetRate(channel, float(sampleRate));
	ndspChnSetFormat(channel, ndspFormat);
	
	ndspChnWaveBufClear(channel);
	
	// Create and play a wav buffer
	ndspWaveBuf waveBuf;
	std::memset(&waveBuf, 0, sizeof(ndspWaveBuf));
	
	if(bitsPerSample == 8)
	{
		waveBuf.data_pcm8 = reinterpret_cast<s8*>(data);
	}
	else
	{
		waveBuf.data_pcm16 = reinterpret_cast<s16*>(data);
	}
	
	waveBuf.nsamples = dataSize / (bitsPerSample >> 3);
	waveBuf.looping = true; // Loop enabled
	waveBuf.offset = 0;
	
	DSP_FlushDataCache(data, dataSize);
	
	ndspChnWaveBufAdd(channel, &waveBuf);
	
	while(aptMainLoop())
	{
		hidScanInput();
		
		u32 keys = hidKeysDown();
		
		if(keys & KEY_START)
			break;
		
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
	
	ndspChnWaveBufClear(channel);
	
	linearFree(data);
	
	gfxExit();
	ndspExit();
	
	return 0;
}
