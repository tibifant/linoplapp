#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

enum PhonemeType
{
  pt_ei,
  pt_au,
  pt_aale,
  pt_assel,
  pt_ober,
  pt_bass,
  pt_chemie,
  pt_docht,
  pt_eber,
  pt_egoist,
  pt_aehre,
  pt_etwas,
  pt_schwa,
  pt_viel,
  pt_geld,
  pt_hase,
  pt_ihm,
  pt_imitat,
  pt_innen,
  pt_jeder,
  pt_kiel,
  pt_last,
  pt_made,
  pt_ng,
  pt_name,
  pt_oetztal,
  pt_ober,
  pt_obelisk,
  pt_eule,
  pt_ordnung,
  pt_oel,
  pt_pfote,
  pt_puppe,
  pt_rose,
  pt_skopus,
  pt_schwere,
  pt_tschechisch,
  pt_zwiebel,
  pt_takt,
  pt_uhu,
  pt_ukulele,
  pt_butt,
  pt_weit,
  pt_nacht,
  pt_ueber,
  pt_buero,
  pt_uecker,
  pt_sahne,
  pt_space,
  pt_dot,
  pt_comma,

  _PhonemeType_Count
};

static const char *_PhonemeStrings[_PhonemeType_Count] = { "aɪ", "aʊ", "aː", "a", "ɐ", "b", "ç", "d", "eː", "e", "ɛː", "ɛ", "ə", "f", "ɡ", "h", "iː", "i", "ɪ", "j", "k", "l", "m", "ŋ", "n", "œ", "oː", "o", "ɔʏ", "ɔ", "øː", "pf", "p", "ʁ", "s", "ʃ", "tʃ", "ts", "t", "uː", "u", "ʊ", "v", "x", "yː", "y", "ʏ", "z", " ", ".", "," };

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

  char chunkIDfmt[4];
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

#define PANIC_IF(x) do { if (x) { __debugbreak(); return; /* Leaking objects as a service */ } } while (false)

#define STRINGIFY(x) #x

#define ASSERT(a) \
  do \
  { if (!(a)) \
    {  puts("Assertion Failed ('" STRINGIFY(a) "') in File '" __FILE__ "' (Line " STRINGIFY(__LINE__) ")"); \
      __debugbreak(); \
  } } while (0)

void ReadFile(const char *filename, uint8_t **ppData, size_t *pSize)
{
  FILE *pFile = fopen(filename, "rb");
  PANIC_IF(pFile == nullptr);

  PANIC_IF(0 != fseek(pFile, 0, SEEK_END));

  const size_t length = ftell(pFile);

  PANIC_IF(0 != fseek(pFile, 0, SEEK_SET));

  *ppData = reinterpret_cast<uint8_t *>(malloc(length));

  PANIC_IF(*ppData == nullptr);

  const size_t readLength = fread(*ppData, 1, length, pFile);

  *pSize = readLength;

  if (pFile != nullptr)
    fclose(pFile);
}

static void LoadPhoneme(Phoneme *pPhoneme, const PhonemeType type)
{
  (void)type;

  //char filename[256];
  // TODO: Get Filename.
  const char *filename = "C:\\git\\linoplapp\\builds\\bin\\audio\\test.wav";

  uint8_t *pFileContent = nullptr;
  size_t size = 0;

  // Load File
  ReadFile(filename, &pFileContent, &size);

  // Parse WAV.
  ASSERT(pFileContent != nullptr);
  
  WaveHeader header;
  memcpy(&header, pFileContent, sizeof(WaveHeader));

  ASSERT(header.chunkIDRiff[0] == 'R' && header.chunkIDRiff[1] == 'I' && header.chunkIDRiff[2] == 'F' && header.chunkIDRiff[3] == 'F');
  ASSERT(header.waveID[0] == 'W' && header.waveID[1] == 'A' && header.waveID[2] == 'V' && header.waveID[3] == 'E');
  ASSERT(header.formatTag == 1); // Checking for PCM format
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
  // Open file.
  FILE *pFile = fopen(filename, "wb");

  // Make header. 
  WaveHeader header;

  const char riff[] = "RIFF";
  memcpy(&header.chunkIDRiff, riff, 4);
  
  header.chunkSizeRiff = (uint32_t)(sampleCount * sizeof(uint16_t) + sizeof(WaveHeader) - 8);
  
  const char wave[] = "WAVE";
  memcpy(&header.waveID, wave, 4);

  const char fmt[] = "fmt ";
  memcpy(&header.chunkIDfmt, fmt, 4);

  header.chunkkSizefmt = 16;
  header.formatTag = 1;
  header.channelCount = 1;
  header.sampleRate = 48000;
  
  header.bitsPerSample = 16;
  header.blockAlign = (header.bitsPerSample + 7) / 8; // Wikipedia says so. For more than one channel: multiply by `channelCount`.

  header.bytesPerSec = header.sampleRate * header.blockAlign;

  const char data[] = "data";
  memcpy(&header.chunkIDdata, data, 4);

  header.chunkSizeData = (uint32_t)(sampleCount * sizeof(uint16_t));

  // Write header to file.
  fwrite(&header, 1, sizeof(WaveHeader), pFile);

  // Write data to file.
  fwrite(pSamples, sizeof(uint16_t), sampleCount, pFile);

  fclose(pFile);
}

int32_t main(const int32_t argc, char **pArgv)
{
  (void)argc;
  (void)pArgv;

  // TODO: Load File with Symbols.
  uint8_t *pFileContent = nullptr;
  size_t size = 0;

  ReadFile("in.txt", &pFileContent, &size);
  
  PhonemeType *pParsedPhonemes = nullptr;
  size_t parsedPhonemeCount = 0;

  // TODO: Parse input to phoneme types.
  
  bool hasData = true;

  while (hasData)
  {
    parsedPhonemeCount++;

    for (size_t i = 0; i < _PhonemeType_Count; i++)
    {
      if (*pFileContent == (uint8_t)strlen(_PhonemeStrings[i]))
        pParsedPhonemes[parsedPhonemeCount] = PhonemeType(i);
    }
  }

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
  //free(pParsedPhonemes);
  //pParsedPhonemes = nullptr;

  free(pOutSamples);
  pOutSamples = nullptr;

  // welp, we're leaking the phoneme samples by design apparently.

  return 0;
}
