
#include "ten_vad_state_machine.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief 语音状态机内部结构
 */
struct vad_state_machine {
    // 配置参数
    vad_state_config_t config;
    size_t hop_size;
    int sample_rate;
    float frame_duration_ms;  // 每帧时长（毫秒）
    
    // 状态信息
    vad_state_t current_state;
    vad_state_t previous_state;
    
    // 计数器
    int speech_frame_count;     // 连续语音帧计数
    int silence_frame_count;    // 连续静音帧计数
    int total_frame_count;      // 总帧数计数
    
    // 时间跟踪
    int current_state_frames;   // 当前状态持续帧数
    int speech_start_frame;     // 语音开始的帧位置
    int last_speech_frame;      // 最后一次检测到语音的帧位置
    
    // 回调
    vad_state_callback_t callback;
    void *user_data;
};

// 默认配置
static const vad_state_config_t default_config = {
    .speech_start_frames = 3,      // 48ms
    .speech_end_frames = 10,       // 160ms
    .pause_frames = 5,             // 80ms
    .pause_resume_frames = 2,      // 32ms
    .min_speech_duration_ms = 200.0f,
    .max_pause_duration_ms = 1000.0f
};

/**
 * @brief 状态变化处理
 */
static void change_state(vad_state_machine_t *sm, vad_state_t new_state) {
    if (sm->current_state != new_state) {
        vad_state_t old_state = sm->current_state;
        sm->previous_state = sm->current_state;
        sm->current_state = new_state;
        sm->current_state_frames = 0;
        
        // 调用回调函数
        if (sm->callback) {
            sm->callback(old_state, new_state, sm->user_data);
        }
        
        printf("[VAD State] %s -> %s (frame: %d)\n", 
               vad_state_get_name(old_state), 
               vad_state_get_name(new_state),
               sm->total_frame_count);
    }
}

TENVAD_API vad_state_machine_t* vad_state_machine_create(
    const vad_state_config_t *config,
    size_t hop_size,
    int sample_rate,
    vad_state_callback_t callback,
    void *user_data) {
    
    vad_state_machine_t *sm = (vad_state_machine_t*)malloc(sizeof(vad_state_machine_t));
    if (!sm) {
        return NULL;
    }
    
    // 初始化配置
    if (config) {
        sm->config = *config;
    } else {
        sm->config = default_config;
    }
    
    sm->hop_size = hop_size;
    sm->sample_rate = sample_rate;
    sm->frame_duration_ms = (float)hop_size * 1000.0f / sample_rate;
    sm->callback = callback;
    sm->user_data = user_data;
    
    // 初始化状态
    sm->current_state = VAD_STATE_SILENCE;
    sm->previous_state = VAD_STATE_SILENCE;
    
    // 初始化计数器
    sm->speech_frame_count = 0;
    sm->silence_frame_count = 0;
    sm->total_frame_count = 0;
    sm->current_state_frames = 0;
    sm->speech_start_frame = -1;
    sm->last_speech_frame = -1;
    
    return sm;
}

TENVAD_API vad_state_t vad_state_machine_process(
    vad_state_machine_t *sm,
    int vad_flag,
    float probability) {
    
    if (!sm) {
        return VAD_STATE_SILENCE;
    }
    
    sm->total_frame_count++;
    sm->current_state_frames++;
    
    // 更新计数器
    if (vad_flag) {
        sm->speech_frame_count++;
        sm->silence_frame_count = 0;
        sm->last_speech_frame = sm->total_frame_count;
    } else {
        sm->silence_frame_count++;
        sm->speech_frame_count = 0;
    }
    
    // 状态机逻辑
    switch (sm->current_state) {
        case VAD_STATE_SILENCE:
            if (sm->speech_frame_count >= sm->config.speech_start_frames) {
                sm->speech_start_frame = sm->total_frame_count - sm->speech_frame_count + 1;
                change_state(sm, VAD_STATE_SPEECH_START);
            }
            break;
            
        case VAD_STATE_SPEECH_START:
            if (vad_flag) {
                // 继续检测到语音，转为持续说话状态
                change_state(sm, VAD_STATE_SPEECH_CONTINUE);
            } else if (sm->silence_frame_count >= sm->config.speech_end_frames) {
                // 语音太短，可能是误触发，回到静音状态
                float speech_duration = sm->current_state_frames * sm->frame_duration_ms;
                if (speech_duration < sm->config.min_speech_duration_ms) {
                    change_state(sm, VAD_STATE_SILENCE);
                } else {
                    change_state(sm, VAD_STATE_SPEECH_END);
                }
            }
            break;
            
        case VAD_STATE_SPEECH_CONTINUE:
            if (sm->silence_frame_count >= sm->config.pause_frames) {
                // 检测到停顿
                change_state(sm, VAD_STATE_SPEECH_PAUSE);
            }
            break;
            
        case VAD_STATE_SPEECH_PAUSE:
            if (sm->speech_frame_count >= sm->config.pause_resume_frames) {
                // 停顿后恢复说话
                change_state(sm, VAD_STATE_SPEECH_CONTINUE);
            } else if (sm->silence_frame_count * sm->frame_duration_ms >= sm->config.max_pause_duration_ms) {
                // 停顿时间过长，认为说话结束
                change_state(sm, VAD_STATE_SPEECH_END);
            }
            break;
            
        case VAD_STATE_SPEECH_END:
            if (sm->speech_frame_count >= sm->config.speech_start_frames) {
                // 新的语音开始
                sm->speech_start_frame = sm->total_frame_count - sm->speech_frame_count + 1;
                change_state(sm, VAD_STATE_SPEECH_START);
            } else {
                // 回到静音状态
                change_state(sm, VAD_STATE_SILENCE);
            }
            break;
    }
    
    return sm->current_state;
}

TENVAD_API vad_state_t vad_state_machine_get_current_state(
    const vad_state_machine_t *sm) {
    return sm ? sm->current_state : VAD_STATE_SILENCE;
}

TENVAD_API float vad_state_machine_get_current_state_duration(
    const vad_state_machine_t *sm) {
    if (!sm) {
        return 0.0f;
    }
    return sm->current_state_frames * sm->frame_duration_ms;
}

TENVAD_API void vad_state_machine_reset(vad_state_machine_t *sm) {
    if (!sm) {
        return;
    }
    
    sm->current_state = VAD_STATE_SILENCE;
    sm->previous_state = VAD_STATE_SILENCE;
    sm->speech_frame_count = 0;
    sm->silence_frame_count = 0;
    sm->total_frame_count = 0;
    sm->current_state_frames = 0;
    sm->speech_start_frame = -1;
    sm->last_speech_frame = -1;
}

TENVAD_API void vad_state_machine_destroy(vad_state_machine_t *sm) {
    if (sm) {
        free(sm);
    }
}

TENVAD_API const char* vad_state_get_name(vad_state_t state) {
    switch (state) {
        case VAD_STATE_SILENCE:         return "SILENCE";
        case VAD_STATE_SPEECH_START:    return "SPEECH_START";
        case VAD_STATE_SPEECH_CONTINUE: return "SPEECH_CONTINUE";
        case VAD_STATE_SPEECH_PAUSE:    return "SPEECH_PAUSE";
        case VAD_STATE_SPEECH_END:      return "SPEECH_END";
        default:                        return "UNKNOWN";
    }
}