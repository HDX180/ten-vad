
//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ten_vad.h"
#include "ten_vad_state_machine.h"

// 状态变化回调函数
void on_state_change(vad_state_t old_state, vad_state_t new_state, void *user_data) {
    printf("🎤 语音状态变化: %s -> %s\n", 
           vad_state_get_name(old_state), 
           vad_state_get_name(new_state));
    
    switch (new_state) {
        case VAD_STATE_SPEECH_START:
            printf("✅ 开始说话\n");
            break;
        case VAD_STATE_SPEECH_PAUSE:
            printf("⏸️  说话停顿\n");
            break;
        case VAD_STATE_SPEECH_END:
            printf("🛑 说话结束\n");
            break;
        case VAD_STATE_SILENCE:
            printf("🔇 进入静音\n");
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("用法: %s <input.wav>\n", argv[0]);
        return 1;
    }
    
    const char *input_file = argv[1];
    const int hop_size = 256;  // 16ms per frame at 16kHz
    const float threshold = 0.5f;
    const int sample_rate = 16000;
    
    // 创建VAD实例
    ten_vad_handle_t vad_handle;
    if (ten_vad_create(&vad_handle, hop_size, threshold) != 0) {
        printf("❌ 创建VAD实例失败\n");
        return 1;
    }
    
    // 配置状态机参数
    vad_state_config_t config = {
        .speech_start_frames = 3,      // 48ms 连续语音才认为开始
        .speech_end_frames = 15,       // 240ms 连续静音才认为结束
        .pause_frames = 8,             // 128ms 连续静音认为是停顿
        .pause_resume_frames = 2,      // 32ms 连续语音恢复说话
        .min_speech_duration_ms = 300.0f,  // 最短语音300ms
        .max_pause_duration_ms = 1500.0f   // 最大停顿1.5秒
    };
    
    // 创建状态机
    vad_state_machine_t *state_machine = vad_state_machine_create(
        &config, hop_size, sample_rate, on_state_change, NULL);
    
    if (!state_machine) {
        printf("❌ 创建状态机失败\n");
        ten_vad_destroy(&vad_handle);
        return 1;
    }
    
    printf("🎯 VAD版本: %s\n", ten_vad_get_version());
    printf("📊 配置信息:\n");
    printf("   - 帧大小: %d samples (%.1fms)\n", hop_size, (float)hop_size * 1000 / sample_rate);
    printf("   - 阈值: %.2f\n", threshold);
    printf("   - 开始检测: %d帧 (%.1fms)\n", config.speech_start_frames, 
           config.speech_start_frames * hop_size * 1000.0f / sample_rate);
    printf("   - 结束检测: %d帧 (%.1fms)\n", config.speech_end_frames,
           config.speech_end_frames * hop_size * 1000.0f / sample_rate);
    
    // 这里应该添加音频文件读取和处理逻辑
    // 为了演示，我们创建一些模拟数据
    printf("\n🎵 开始处理音频流...\n");
    
    // 模拟音频处理循环
    int16_t audio_buffer[256];  // 一帧音频数据
    float probability;
    int vad_flag;
    
    // 模拟不同的音频场景
    int scenarios[] = {
        // 静音 -> 语音 -> 停顿 -> 语音 -> 静音
        0,0,0,0,0,  // 静音
        1,1,1,1,1,1,1,1,1,1,  // 语音开始
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  // 持续语音
        0,0,0,0,0,0,0,0,  // 短暂停顿
        1,1,1,1,1,1,1,1,1,1,  // 恢复语音
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  // 结束静音
    };
    
    int scenario_count = sizeof(scenarios) / sizeof(scenarios[0]);
    
    for (int i = 0; i < scenario_count; i++) {
        // 模拟音频数据（实际应用中从文件或麦克风读取）
        memset(audio_buffer, scenarios[i] ? 1000 : 0, sizeof(audio_buffer));
        
        // VAD处理
        if (ten_vad_process(vad_handle, audio_buffer, hop_size, &probability, &vad_flag) == 0) {
            // 状态机处理
            vad_state_t current_state = vad_state_machine_process(state_machine, vad_flag, probability);
            
            printf("[%03d] VAD: %d (%.3f) | 状态: %s | 持续: %.1fms\n", 
                   i, vad_flag, probability, 
                   vad_state_get_name(current_state),
                   vad_state_machine_get_current_state_duration(state_machine));
        }
        
        // 模拟实时处理延迟
        #ifdef _WIN32
        Sleep(16);  // 16ms
        #else
        usleep(16000);  // 16ms
        #endif
    }
    
    printf("\n✅ 处理完成\n");
    
    // 清理资源
    vad_state_machine_destroy(state_machine);
    ten_vad_destroy(&vad_handle);
    
    return 0;
}