/* V1.0 I2S */

#include "I2S/i2s.h"
#include <stdio.h>
#include <string.h>

#define WAV_INFO_Printf                 // 打印调试信息

#define I2S_BUFFER_SIZE         44100/2 // 缓冲区数组的大小，缓冲区是uint32_t类型
#define I2S_MAX_AUDIO_FILES     32      // 最大音频文件数量

// 全局变量
volatile bool i2s_trans_complete_flag = false;

static struAudio_t audio_database[I2S_MAX_AUDIO_FILES]; // 音频数据库（最多32首歌）

// 播放列表相关
static struAudio_t *playlist_head = NULL;   // 播放列表头
static struAudio_t *playlist_tail = NULL;   // 播放列表尾
static uint16_t playlist_count = 0;         // 播放列表歌曲数量

// 播放缓冲区,两个缓冲区交替使用
uint32_t data_buffer[2][I2S_BUFFER_SIZE];      

void I2S_Init(){
    fsp_err_t err = FSP_SUCCESS;

    // 打开并启动 GPT 音频时钟
    err = R_GPT_Open(&g_timer0_ctrl, &g_timer0_cfg);
    assert(FSP_SUCCESS == err);

    err = R_GPT_Start(&g_timer0_ctrl);
    assert(FSP_SUCCESS == err);

    // 打开 I2S
    err = R_SSI_Open(&g_i2s0_ctrl, &g_i2s0_cfg);
    assert(FSP_SUCCESS == err);

    printf("[I2S] I2S_Open\r\n");
}

wav_header_t I2S_ReadHeader(FIL *file, UINT *bytesRead)
{
    wav_header_t result;         // WAV文件头
    FRESULT res;

    // 读取RIFF/WAVE头
    res = f_read(file, &result, sizeof(wav_header_t), bytesRead);
    if (res != FR_OK || (*bytesRead) != sizeof(wav_header_t))
    {
        printf("[I2S] Failed to read WAV header\r\n");
        memset(&result, 0, sizeof(wav_header_t));
        return result;
    }

    // 验证RIFF标识
    if (strncmp(result.riff, "RIFF", 4) != 0)
    {
        printf("[I2S] Invalid RIFF format\r\n");
        memset(&result, 0, sizeof(wav_header_t));
        return result;
    }

    // 验证WAVE标识
    if (strncmp(result.wave, "WAVE", 4) != 0)
    {
        printf("[I2S] Invalid WAVE format\r\n");
        memset(&result, 0, sizeof(wav_header_t));
        return result;
    }
    printf("[I2S] WAV Read Header\r\n");
    printf("[I2S] WAV file size: %lu bytes\r\n", result.file_size + 8);
    return result;
}

wav_chunk_t I2S_ReadChunk(FIL *file, UINT *bytesRead)
{
    wav_chunk_t result;
    FRESULT res;

    // 读取块头（块ID + 块大小）
    res = f_read(file, &result, sizeof(wav_chunk_t), bytesRead);
    if (res != FR_OK || (*bytesRead) != sizeof(wav_chunk_t))
    {
        printf("[I2S] Failed to read chunk header\r\n");
        memset(&result, 0, sizeof(wav_chunk_t));
        return result;
    }
    printf("[I2S] WAV Read Chunk\r\n");
    printf("[I2S] WAV ChunkId: %.4s\r\n", result.chunkId);
    return result;
}

// 内部函数：检查文件是否为 WAV 文件
static bool is_wav_file(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (dot == NULL)
    {
        return false;
    }
    return (strncmp(dot, ".wav", 4) == 0);
}

// 根据文件路径读取 WAV 文件并解析音频信息
void I2S_WavReadAll(const char *path)
{
    DIR dir;
    FILINFO fno;
    FRESULT res;
    char filepath[128];
    UINT bytesRead = 0;
    uint16_t added_count = 0;
    uint8_t current_audio_id = 0;  // 当前音频编号（局部变量）

    printf("[I2S] Scanning directory: %s\r\n", path);

    // 清空播放列表
    playlist_head = NULL;
    playlist_tail = NULL;
    playlist_count = 0;

    // 打开目录
    res = f_opendir(&dir, path);
    if (res != FR_OK)
    {
        printf("[I2S] Failed to open directory: %s, error: %d\r\n", path, res);
        return;
    }

    // 遍历目录
    while (1)
    {
        // 读取下一个文件/目录
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == '\0')
        {
            break;  // 到达目录末尾或出错
        }

        // 跳过目录和隐藏文件
        if (fno.fattrib & AM_DIR)
        {
            continue;
        }

        // 检查是否为 WAV 文件
        if (!is_wav_file(fno.fname))
        {
            continue;
        }

        // 检查是否超过数据库最大容量
        if (playlist_count >= I2S_MAX_AUDIO_FILES)
        {
            printf("[I2S] Warning: Playlist full (max 32), skipping: %s\r\n", fno.fname);
            continue;
        }

        // 构造完整路径
        snprintf(filepath, sizeof(filepath), "%s/%s", path, fno.fname);

        I2S_WavRead(filepath, &bytesRead, &audio_database[playlist_count], current_audio_id);
        if (audio_database[playlist_count].filename[0] != '\0')
        {
            current_audio_id++;  // 成功读取后递增id
            // 设置链表指针
            audio_database[playlist_count].prev = playlist_tail;
            audio_database[playlist_count].next = NULL;

            // 更新链表
            if (playlist_tail == NULL)
            {
                // 第一个元素
                playlist_head = &audio_database[playlist_count];
            }
            else
            {
                playlist_tail->next = &audio_database[playlist_count];
            }
            playlist_tail = &audio_database[playlist_count];

            playlist_count++;
            added_count++;
        }
    }

    // 关闭目录
    f_closedir(&dir);

    printf("[I2S] Scan complete. Found %u WAV file(s), added %u to playlist\r\n",
           added_count, playlist_count);
}

// 读取 WAV 文件并解析音频信息
void I2S_WavRead(const char *filename, UINT *bytesRead, struAudio_t *audio, uint8_t audio_id)
{
    FIL file;       // 文件对象
    FRESULT res;    // 文件操作结果

    wav_chunk_t chunk;
    bool data_chunk_found = false; // 是否找到data块
    wav_header_t header;          // WAV文件头
    wav_chunkFMTdata_t wav_fmtdata; // WAV格式块（临时使用）

    // 打开文件
    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK)
    {
        printf("[I2S] Failed to open file: %s, error code: %d\r\n", filename, res);
        memset(audio, 0, sizeof(struAudio_t));
        return;
    }
    printf("[I2S] File opened: %s\r\n", filename);

    // 清空当前音频信息
    memset(audio, 0, sizeof(struAudio_t));
    strncpy(audio->filename, filename, sizeof(audio->filename) - 1);

    // 将歌曲名字填入（从文件名中提取，去掉路径和扩展名）
    const char *last_slash = strrchr(filename, '/');
    const char *last_backslash = strrchr(filename, '\\');
    const char *last_dot = strrchr(filename, '.');
    const char *name_start = (last_slash > last_backslash) ? last_slash + 1 : (last_backslash ? last_backslash + 1 : filename);
    const char *name_end = last_dot ? last_dot : filename + strlen(filename);

    uint8_t name_len = (uint8_t)(name_end - name_start);
    if (name_len > sizeof(audio->name) - 1)
    {
        name_len = sizeof(audio->name) - 1;
    }
    memcpy(audio->name, name_start, name_len);
    audio->name[name_len] = '\0';

    // 将歌曲编号填入
    audio->id = audio_id;

    // 获取文件大小
    audio->file_size = f_size(&file);

    // 读取RIFF/WAVE头
    header = I2S_ReadHeader(&file, bytesRead);

    // 解析WAV文件块
    while (!data_chunk_found)
    {
        chunk = I2S_ReadChunk(&file, bytesRead);

        // 检查是否读取到文件末尾,或者读取错误
        if (chunk.chunkId[0] == '\0')
        {
            printf("[I2S] Reached end of file without finding data chunk\r\n");
            f_close(&file);
            memset(audio, 0, sizeof(struAudio_t));
            return;
        }

        // 检查fmt块
        if (strncmp(chunk.chunkId, "fmt ", 4) == 0)
        {
            // 只读取标准16字节的fmt数据
            uint16_t fmt_data_size = chunk.chunkSize < sizeof(wav_fmtdata) ? chunk.chunkSize : sizeof(wav_fmtdata);
            res = f_read(&file, &wav_fmtdata, fmt_data_size, bytesRead);
            if (res != FR_OK)
            {
                printf("[I2S] Failed to read fmt chunk\r\n");
                f_close(&file);
                memset(audio, 0, sizeof(struAudio_t));
                return;
            }

            // 跳过fmt chunk剩余的数据（如果有）
            if (chunk.chunkSize > fmt_data_size)
            {
                f_lseek(&file, f_tell(&file) + (chunk.chunkSize - fmt_data_size));
            }

            // 验证音频格式是否支持
            if (wav_fmtdata.formatTag != 1)
            {
                printf("[I2S] Error: Only PCM format is supported\r\n");
                f_close(&file);
                memset(audio, 0, sizeof(struAudio_t));
                return;
            }

            // 存储音频参数到结构体
            memcpy(&audio->fmt, &wav_fmtdata, sizeof(wav_fmtdata));
            audio->bytes_per_sample = wav_fmtdata.channels * wav_fmtdata.bits_per_sample / 8;

#ifdef WAV_INFO_Printf
            // 打印音频参数
            printf("[I2S] Audio format: %s\r\n", (wav_fmtdata.formatTag == 1) ? "PCM" : "Unknown");
            printf("[I2S] Channels: %u\r\n", wav_fmtdata.channels);
            printf("[I2S] Sample rate: %lu Hz\r\n", wav_fmtdata.samplesPerSec);
            printf("[I2S] Bits per sample: %u\r\n", wav_fmtdata.bits_per_sample);
            printf("[I2S] Block align: %u\r\n", wav_fmtdata.blockAlign);
            // printf("[I2S] Bytes per sample: %u\r\n", audio->bytes_per_sample);
#endif

            printf("[I2S] fmt chunk found \r\n");
        }
        // 检查data块
        else if (strncmp(chunk.chunkId, "data", 4) == 0)
        {
            data_chunk_found = true;
            audio->data_offset = f_tell(&file);
            audio->data_size = chunk.chunkSize;
            audio->total_samples = chunk.chunkSize / audio->bytes_per_sample;
            audio->current_sample = 0;
            printf("[I2S] Data chunk found \r\n");
            printf("[I2S] Data offset: %lu, size: %u bytes, samples: %lu\r\n",
                   audio->data_offset, audio->data_size, audio->total_samples);
        }
        // 其他块，跳过
        else
        {
            printf("[I2S] Skipping chunk: %.4s, size: %u bytes\r\n", chunk.chunkId, chunk.chunkSize);
            f_lseek(&file, f_tell(&file) + chunk.chunkSize);
        }
    }

    // 关闭文件
    f_close(&file);
}

// 获取播放列表头
struAudio_t* I2S_GetPlaylistHead(void)
{
    return playlist_head;
}

// 获取播放列表尾
struAudio_t* I2S_GetPlaylistTail(void)
{
    return playlist_tail;
}

// 获取播放列表歌曲数量
uint16_t I2S_GetPlaylistCount(void)
{
    return playlist_count;
}

// 根据ID获取音频信息
struAudio_t* I2S_GetAudioById(uint8_t id)
{
    struAudio_t *audio = playlist_head;
    while (audio != NULL)
    {
        if (audio->id == id)
        {
            return audio;
        }
        audio = audio->next;
    }
    return NULL;
}

// 根据name获取音频信息
struAudio_t* I2S_GetAudioByName(const char *name)
{
    struAudio_t *audio = playlist_head;
    while (audio != NULL)
    {
        if (strcmp(audio->name, name) == 0)
        {
            return audio;
        }
        audio = audio->next;
    }
    return NULL;
}

void I2S_OpenWavFile(FIL *file, struAudio_t *audio)
{
    FRESULT res;       // 文件操作结果
    fsp_err_t err;

    // 重置 I2S 传输完成标志
    i2s_trans_complete_flag = false;

    // 打开文件
    res = f_open(file, audio->filename, FA_READ);
    if (res != FR_OK) {
        printf("[I2S] Failed to open file: %s, error code: %d\r\n", audio->filename, res);
        return;
    }

    // 定位到wav数据
    res = f_lseek(file, audio->data_offset);
    if (res != FR_OK) {
        printf("[I2S] Failed to seek to data offset: %d\r\n", res);
        f_close(file);
        return;
    }

    // 依据audio->total_samples计算出整个文件的播放时长，并在播放开始前打印出来
    uint32_t total_seconds = audio->total_samples / audio->fmt.samplesPerSec;
    uint32_t minutes = total_seconds / 60;
    uint32_t seconds = total_seconds % 60;
    printf("[I2S] Audio: %s\r\n", audio->name);
    printf("[I2S] Duration: %lu分%lu秒\r\n", minutes, seconds);
}

FRESULT I2S_ReadBuffer(FIL *file, struAudio_t *audio, uint32_t *buffer, uint32_t file_read_size, UINT *bytesRead){
    FRESULT res;       // 文件操作结果

    if(audio->total_samples - audio->current_sample < I2S_BUFFER_SIZE * sizeof(uint32_t)/audio->bytes_per_sample){
        file_read_size = (audio->total_samples - audio->current_sample) * audio->bytes_per_sample;
    }
    else{
        file_read_size = I2S_BUFFER_SIZE * sizeof(uint32_t);
    }

    res = f_read(file, buffer, file_read_size, bytesRead);

    return res;
}

WAVPlay_t I2S_PlayWavFile(FIL *file, struAudio_t *audio)
{
    FRESULT res;       // 文件操作结果
    static UINT buffer0_bytes = 0;  // 缓冲区0实际读取的字节数
    static UINT buffer1_bytes = 0;  // 缓冲区1实际读取的字节数
    static UINT current_write_bytes = 0;  // 当前写入I2S的字节数

    // 计算缓冲区大小（根据当前音频参数）
    uint32_t file_read_size = I2S_BUFFER_SIZE * sizeof(uint32_t);

    if(audio->play_status == 0)
    {
        res = I2S_ReadBuffer(file, audio, data_buffer[audio->buffer_index], file_read_size, &buffer0_bytes);
        if(res != FR_OK)
        {
            printf("[I2S] Failed to read audio data\r\n");
            return WAV_ReadError;
        }
        res = I2S_ReadBuffer(file, audio, data_buffer[(audio->buffer_index+1)%2], file_read_size, &buffer1_bytes);
        if(res != FR_OK)
        {
            printf("[I2S] Failed to read audio data\r\n");
            return WAV_ReadError;
        }

        // 播放buffer0，使用buffer0_bytes
        current_write_bytes = buffer0_bytes;
        R_SSI_Write(&g_i2s0_ctrl, data_buffer[audio->buffer_index], current_write_bytes);
        audio->current_sample += current_write_bytes / audio->bytes_per_sample;
        audio->play_status = 1;
    }
    else {
        if(audio->total_samples <= audio->current_sample){
            audio->play_status = 2;
            return WAV_FINISH;
        }
        else if(i2s_trans_complete_flag == true){
            // 播放另一个缓冲区
            current_write_bytes = (audio->buffer_index == 0) ? buffer1_bytes : buffer0_bytes;
            R_SSI_Write(&g_i2s0_ctrl, data_buffer[(audio->buffer_index+1)%2], current_write_bytes);
            audio->current_sample += current_write_bytes / audio->bytes_per_sample;

            // 读取新数据到当前缓冲区
            if (audio->buffer_index == 0) {
                I2S_ReadBuffer(file, audio, data_buffer[0], file_read_size, &buffer0_bytes);
            } else {
                I2S_ReadBuffer(file, audio, data_buffer[1], file_read_size, &buffer1_bytes);
            }

            // 实时输出播放进度
            printf("[I2S] Playing %lu / %lu\r\n", audio->current_sample, audio->total_samples);
            audio->buffer_index = (audio->buffer_index+1)%2;
            i2s_trans_complete_flag = false;
        }
    }

    return WAV_OK;
}

void I2S_Callback(i2s_callback_args_t *p_args)
{
    if (p_args->event == I2S_EVENT_IDLE)
    {
        // I2S 空闲，传输完成 (由于 DTC 未正确配置，回调可能不触发)
        i2s_trans_complete_flag = true;
        // printf("[I2S_Callback] I2S_EVENT_IDLE - transmission complete\r\n");
    }
    if (p_args->event == I2S_EVENT_TX_EMPTY)
    {
        // TX 缓冲区为空，可以在此排队更多数据
        // printf("[I2S_Callback] I2S_EVENT_TX_EMPTY\r\n");
    }

    if (p_args->event == I2S_EVENT_RX_FULL)
    {
        // RX 缓冲区为满，可以在此读取数据
        // printf("[I2S_Callback] I2S_EVENT_RX_FULL\r\n");
    }
}
