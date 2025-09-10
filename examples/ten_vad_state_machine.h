
#ifndef TEN_VAD_STATE_MACHINE_H
#define TEN_VAD_STATE_MACHINE_H

#include "ten_vad.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// API导出宏定义
#if defined(__APPLE__) || defined(__ANDROID__) || defined(__linux__)
#define TENVAD_STATE_API __attribute__((visibility("default")))
#elif defined(_WIN32) || defined(__CYGWIN__)
#ifdef TENVAD_STATE_MACHINE_EXPORTS
#define TENVAD_STATE_API __declspec(dllexport)
#else
#define TENVAD_STATE_API __declspec(dllimport)
#endif
#else
#define TENVAD_STATE_API
#endif

/**
 * @brief 语音状态枚举
 */
typedef enum {
    VAD_STATE_SILENCE = 0,      // 静音状态
    VAD_STATE_SPEECH_START,     // 开始说话
    VAD_STATE_SPEECH_CONTINUE,  // 持续说话
    VAD_STATE_SPEECH_PAUSE,     // 说话中的停顿
    VAD_STATE_SPEECH_END        // 说话结束
} vad_state_t;

/**
 * @brief 状态机配置参数
 */
typedef struct {
    int speech_start_frames;    // 连续多少帧检测到语音才认为开始说话（默认3帧，约48ms）
    int speech_end_frames;      // 连续多少帧检测到静音才认为说话结束（默认10帧，约160ms）
    int pause_frames;           // 连续多少帧静音认为是停顿（默认5帧，约80ms）
    int pause_resume_frames;    // 停顿后连续多少帧语音认为恢复说话（默认2帧，约32ms）
    float min_speech_duration_ms;  // 最短语音持续时间，避免误触发（默认200ms）
    float max_pause_duration_ms;   // 最大停顿时间，超过则认为说话结束（默认1000ms）
} vad_state_config_t;

/**
 * @brief 语音状态机句柄
 */
typedef struct vad_state_machine vad_state_machine_t;

/**
 * @brief 状态变化回调函数类型
 * @param old_state 旧状态
 * @param new_state 新状态
 * @param user_data 用户数据
 */
typedef void (*vad_state_callback_t)(vad_state_t old_state, vad_state_t new_state, void *user_data);

/**
 * @brief 创建语音状态机
 * @param config 配置参数，可为NULL使用默认配置
 * @param hop_size VAD处理的帧大小
 * @param sample_rate 采样率（用于时间计算）
 * @param callback 状态变化回调函数，可为NULL
 * @param user_data 传递给回调函数的用户数据
 * @return 状态机句柄，失败返回NULL
 */
TENVAD_STATE_API vad_state_machine_t* vad_state_machine_create(
    const vad_state_config_t *config,
    size_t hop_size,
    int sample_rate,
    vad_state_callback_t callback,
    void *user_data
);

/**
 * @brief 处理VAD结果并更新状态
 * @param state_machine 状态机句柄
 * @param vad_flag VAD检测结果（0=静音，1=语音）
 * @param probability VAD概率值
 * @return 当前状态
 */
TENVAD_STATE_API vad_state_t vad_state_machine_process(
    vad_state_machine_t *state_machine,
    int vad_flag,
    float probability
);

/**
 * @brief 获取当前状态
 * @param state_machine 状态机句柄
 * @return 当前状态
 */
TENVAD_STATE_API vad_state_t vad_state_machine_get_current_state(
    const vad_state_machine_t *state_machine
);

/**
 * @brief 获取当前状态持续时间（毫秒）
 * @param state_machine 状态机句柄
 * @return 当前状态持续时间
 */
TENVAD_STATE_API float vad_state_machine_get_current_state_duration(
    const vad_state_machine_t *state_machine
);

/**
 * @brief 重置状态机到初始状态
 * @param state_machine 状态机句柄
 */
TENVAD_STATE_API void vad_state_machine_reset(vad_state_machine_t *state_machine);

/**
 * @brief 销毁状态机
 * @param state_machine 状态机句柄
 */
TENVAD_STATE_API void vad_state_machine_destroy(vad_state_machine_t *state_machine);

/**
 * @brief 获取状态名称字符串（用于调试）
 * @param state 状态值
 * @return 状态名称字符串
 */
TENVAD_STATE_API const char* vad_state_get_name(vad_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* TEN_VAD_STATE_MACHINE_H */