#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

enum PhonemeType
{
  pt_,

  _PhonemeType_Count
};

struct Phoneme 
{
  bool loaded = false;
  size_t sampleCount = 0;
  uint16_t *pSamples = nullptr;
};

struct WaveHeader
{
  char chunkIDRiff[4];
  uint32_t chunkSizeRiff;
  char waveID[4];

  uint32_t chunkIDfmt;
  uint32_t chunkkSizefmt;
  uint16_t formatTag; // coc sagt: Audio format 1=PCM.
  uint16_t channelCount;
  uint32_t sampleRate;
  uint32_t bytesPerSec;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
  
  char chunkIDdata[4];
  uint32_t chunkSizeData;
};

#define PANIC_IF(x) do { if (!(x)) __debugbreak(); return; /* Leaking objects as a service */ } while (false)

#define STRINGIFY(x) #x

#define ASSERT(a) \
  do \
  { if (!(a)) \
    {  puts("Assertion Failed ('" STRINGIFY(a) "') in File '" __FILE__ "' (Line " STRINGIFY(__LINE__) ")"); \
      __debugbreak(); \
  } } while (0)

void ReadFile(const char *filename, uint8_t **ppData, size_t *pSize)
{
  FILE *pFile = nullptr;

  pFile = fopen(filename, "rb");
  PANIC_IF(pFile == nullptr);

  PANIC_IF(0 != fseek(pFile, 0, SEEK_END), lsR_IOFailure);

  const size_t length = ftell(pFile);

  PANIC_IF(0 != fseek(pFile, 0, SEEK_SET), lsR_IOFailure);

  *ppData = reinterpret_cast<uint8_t *>(malloc(length));

  PANIC_IF(*ppData == nullptr);

  const size_t readLength = fread(*ppData, 1, length, pFile);

  *pSize = readLength;

  if (pFile != nullptr)
    fclose(pFile);
}

static void LoadPhoneme(Phoneme *pPhoneme, const PhonemeType type)
{
  //char filename[256];
  // TODO: Get Filename.
  const char *filename = "audio/test.wav";

  uint8_t *pFileContent = nullptr;
  size_t size = 0;

  // Load File
  ReadFile(filename, &pFileContent, &size);

  // Parse WAV.
  ASSERT(pFileContent != nullptr);
  
  WaveHeader header;
  memcpy(&header, pFileContent, sizeof(WaveHeader));

  ASSERT(header.waveID[0] == 'W' && header.waveID[1] == 'A' && header.waveID[2] == 'V' && header.waveID[3] == 'E');
  ASSERT(header.formatTag == 1); // do we have PCM format?
  ASSERT(header.channelCount == 1);
  ASSERT(header.bitsPerSample == 16);
  ASSERT(header.sampleRate == 48000);

  pPhoneme->sampleCount = header.chunkSizeData / sizeof(uint16_t);
  pPhoneme->pSamples = reinterpret_cast<uint16_t *>(malloc(header.chunkSizeData));
  ASSERT(pPhoneme->pSamples);

  memcpy(pPhoneme->pSamples, pFileContent + sizeof(WaveHeader), header.chunkSizeData);

  free(pFileContent);
  pFileContent = nullptr;
}

static Phoneme GetPhoneme(const PhonemeType type)
{
  static Phoneme _Phonemes[_PhonemeType_Count];
  
  Phoneme *pCurrent = &_Phonemes[type];
    
  if (pCurrent->loaded) // Already loaded? Return phoneme.
    return *pCurrent;

  LoadPhoneme(pCurrent, type);

  return *pCurrent;
}

// Returns count of appended phoneme samples.
static size_t AppendPhoneme(uint16_t *pBufferPosition, const PhonemeType type)
{
  Phoneme current = GetPhoneme(type);

  memcpy(pBufferPosition, current.pSamples, current.sampleCount * sizeof(uint16_t));

  return current.sampleCount;
}

static void WriteToWav(const uint16_t *pSamples, const size_t sampleCount, const char *filename)
{
  // TODO: Write Stuff.
}

int32_t main(const int32_t argc, char **pArgv)
{
  // TODO: Load File with Symbols.
  
  PhonemeType *pParsedPhonemes = nullptr;
  size_t parsedPhonemeCount = 0;

  // TODO: Parse input to phoneme types.

  size_t requiredOutSampleCount = 0;

  for (size_t i = 0; i < parsedPhonemeCount; i++)
    requiredOutSampleCount += GetPhoneme(pParsedPhonemes[i]).sampleCount;

  uint16_t *pOutSamples = reinterpret_cast<uint16_t *>(malloc(sizeof(uint16_t) * requiredOutSampleCount));
  size_t outSampleIndex = 0;

  for (size_t i = 0; i < parsedPhonemeCount; i++)
    outSampleIndex += AppendPhoneme(pOutSamples + outSampleIndex, pParsedPhonemes[i]);

  // Write WAV File.
  WriteToWav(pOutSamples, outSampleIndex, "out.wav");

  // Cleanup.
  free(pParsedPhonemes);
  pParsedPhonemes = nullptr;

  free(pOutSamples);
  pOutSamples = nullptr;

  // welp, we're leaking the phoneme samples by design apparently.

  return 0;
}
