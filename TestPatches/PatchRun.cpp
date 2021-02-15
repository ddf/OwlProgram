#include <stdio.h>
#include <malloc.h>

#include "heap.h"
// define these before defining the malloc/free macros
void *pvPortMalloc( size_t xWantedSize ){
  return malloc(xWantedSize);
}
void vPortFree( void *pv ){
  free(pv);
}
#include "basicmaths.h"
#include "message.h"
#include "Patch.h"
#include "ProgramVector.h"
#include "PatchProcessor.h"
#include "MemoryBuffer.hpp"
#include "registerpatch.h"
#include "wav.h"

#define SAMPLE_RATE 48000
#define CHANNELS    2
#define BLOCKSIZE   128

#include "DummyPatch.hpp"

static PatchProcessor processor;
ProgramVector programVector;
static Patch* testpatch = NULL;

int serviceCall(int service, void** params, int len){
  printf("Service call (todo) : %d\n", service);
}

PatchProcessor* getInitialisingPatchProcessor(){
  return &processor;
}

void registerPatch(const char* name, uint8_t inputs, uint8_t outputs, Patch* patch){
  //if(patch == NULL)
  //  error(OUT_OF_MEMORY_ERROR_STATUS, "Out of memory");
  printf("Register patch %s (%d ins, %d outs)\n", name, inputs, outputs);
  testpatch = patch;
}

#define REGISTER_PATCH(T, STR, IN, OUT) registerPatch(STR, IN, OUT, new T)

int main(int argc, char** argv){
  programVector.serviceCall = serviceCall;
#include "registerpatch.cpp"
  ASSERT(testpatch != NULL, "Missing patch");    
  int ret = 0;
  SampleBuffer* samples = new SampleBuffer(BLOCKSIZE);
  if(argc > 1){
    const char* input_filename = argv[1];
    WavHeader *wav_header = new WavHeader();
    int16_t* data = NULL;
    wavread(wav_header, input_filename, &data);
    ASSERT(wav_header->num_channels == CHANNELS, "Incorrect number of channels in input file");
    // ASSERT(wav_header->sample_rate == SAMPLE_RATE, "Incorrect sample rate in input file");
    ASSERT(wav_header->bps == 16, "Incorrect number of bits per sample in input file");
    const int channels = wav_header->num_channels;
    const int len = wav_header->datachunk_size/(wav_header->fmtchunk_size/8);
    int16_t* src = data;
    int16_t* end = data+len;
    while(src+BLOCKSIZE <= end){
      samples->split16(src, BLOCKSIZE);
      testpatch->processAudio(*samples);
      samples->comb16(src);
      src += BLOCKSIZE*channels;
    }
    if(argc > 2){
      const char* output_filename = argv[2];
      wavwrite(wav_header, output_filename, data);
    }
    free(data);
    delete wav_header;
  }else{
    testpatch->processAudio(*samples);
  }
  delete samples;
  delete testpatch;  
  return ret;
}
