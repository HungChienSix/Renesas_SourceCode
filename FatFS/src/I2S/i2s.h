/* V1.0 I2S */

#ifndef I2S_I2S_H_
#define I2S_I2S_H_

#include "hal_data.h"
#include "FatFs/ff.h"   

// P112 BCK
// P113 LRCK
// P114 RXD
// P115 TXD

typedef struct {
    char riff[4];               // "RIFF"
    uint32_t file_size;         // 文件大小 - 8
    char wave[4];               // "WAVE"
} wav_header_t;

typedef struct {
    char chunkId[4];            // 块ID: "fmt " 或 "data"
    uint32_t chunkSize;         // 块大小
} wav_chunk_t;

typedef struct {
    uint16_t formatTag;         // 音频格式: 1 = PCM
    uint16_t channels;          // 声道数: 1 = 单声道, 2 = 立体声
    uint32_t samplesPerSec;     // 采样率 (Hz)
    uint32_t avgBytesPerSec;    // 字节率: samplesPerSec * channels * bits_per_sample / 8
    uint16_t blockAlign;        // 块对齐: channels * bits_per_sample / 8
    uint16_t bits_per_sample;   // 位深: 8, 16, 24, 32
} wav_chunkFMTdata_t;

typedef struct {
    // 文件信息
    char filename[64];          // 文件名（包含路径）
    char name[32];              // 歌曲名（显示用）
    uint8_t id;                 // 歌曲编号（用于排序）

    // WAV格式信息
    wav_chunkFMTdata_t fmt;     // 音频格式参数

    // 文件数据位置信息
    uint32_t file_size;         // 文件总大小
    uint32_t data_offset;       // 音频数据在文件中的偏移量
    uint32_t data_size;         // 音频数据大小（字节）

    // 播放信息
    uint32_t total_samples;     // 总样本数
    uint32_t current_sample;    // 当前播放样本位置
    uint32_t bytes_per_sample;  // 每个样本的字节数

    // 播放状态
    uint8_t  buffer_index;      // 0=A缓冲区, 1=B缓冲区
    uint8_t  play_status;       // 0=未播放, 1=播放中, 2=播放完成

    // 链表指针（用于播放列表）
    struct struAudio_t *prev;   // 上一首
    struct struAudio_t *next;   // 下一首
} struAudio_t;

typedef enum {
    WAV_OK = 0,                 // 播放成功
    WAV_FINISH,                 // 播放结束
    WAV_ReadError,              // 读取文件错误
} WAVPlay_t;

// I2S 传输完成标志（外部可访问）
extern volatile bool i2s_trans_complete_flag;

// 文件读取函数
void                I2S_Init();
void                I2S_WavReadAll(const char *path);
void                I2S_WavRead(const char *filename, UINT *bytesRead, struAudio_t *audio, uint8_t audio_id);

// 播放列表相关函数
struAudio_t*        I2S_GetPlaylistHead(void);
struAudio_t*        I2S_GetPlaylistTail(void);
uint16_t            I2S_GetPlaylistCount(void);
struAudio_t*        I2S_GetAudioById(uint8_t id);
struAudio_t*        I2S_GetAudioByName(const char *name);

// 播放控制函数
void                I2S_OpenWavFile(FIL *file, struAudio_t *audio);
WAVPlay_t           I2S_PlayWavFile(FIL *file, struAudio_t *audio);

#endif /* I2S_I2S_H_ */
