#pragma once

#include <glm/glm.hpp>
#include <vector>

// 环的数据结构
struct Ring {
    glm::mat4 world;        // 世界变换矩阵
    glm::vec3 center;       // 环心位置
    glm::vec3 axis;         // 法向量（穿越判定用）
    float R, r;             // 主半径、副半径
    bool passed;            // 是否已通过（防重复）
    bool scored;            // 是否已计分
};

// 飞船数据结构
struct Ship {
    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 velocity;
    float speed_current;
    float speed_min, speed_max;
    float radius;           // 碰撞半径
    float acceleration;     // 加速度
    float roll_angle;       // 翻滚角度
    float pitch_angle;      // 俯仰角度
    float yaw_angle;        // 偏航角度
    float roll_speed;       // 翻滚速度
    float pitch_speed;      // 俯仰速度
    float yaw_speed;        // 偏航速度
    
    // 初始化默认值
    Ship() : position(0.0f, 0.0f, 10.0f), forward(0.0f, 0.0f, -1.0f), velocity(0.0f),
             speed_current(5.0f), speed_min(3.0f), speed_max(8.0f), radius(0.5f),
             acceleration(2.0f), roll_angle(0.0f), pitch_angle(0.0f), yaw_angle(0.0f),
             roll_speed(2.0f), pitch_speed(2.0f), yaw_speed(2.0f) {}
};

// 游戏状态
struct GameState {
    enum State { Ready, Playing, GameOver };
    State state;
    int score;
    int misses;
    int combo;
    float play_time;
    size_t next_ring_idx;   // 下一个待判定的环索引
    
    // 初始化默认值
    GameState() : state(Ready), score(0), misses(0), combo(0), 
                  play_time(0.0f), next_ring_idx(0) {}
};

// 游戏参数
struct GameParams {
    float ring_spacing;     // 环间距
    float ring_R, ring_r;   // 环尺寸
    float pass_band;        // 穿越容错带宽
    float assist_strength;  // 辅助转向强度
    
    // 初始化默认值
    GameParams() : ring_spacing(10.0f), ring_R(2.0f), ring_r(0.3f),  // 洞半径 = ring_R - ring_r = 1.7
                   pass_band(1.0f), assist_strength(0.3f) {}
};
