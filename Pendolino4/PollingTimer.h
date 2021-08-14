#ifndef _POLLING_TIMER_H_
#define _POLLING_TIMER_H_

#include <Arduino.h>
#include <stdint.h>

// Interval Timer
class IntervalTimer{
public:
    void set(uint32_t msec){
        m_time = micros();
        m_interval = msec * 1000;
    }
    void setMicroseconds(uint32_t usec){
        m_time = micros();
        m_interval = usec;
    }
    bool elapsed(){
        uint32_t now = micros();
        uint32_t elapsed_time = now - m_time;
        if(elapsed_time >= m_interval){
            m_time += m_interval;
            return true;
        }else{
            return false;
        }
    }
    void reset(){
        m_time = micros();
    }
private:
    uint32_t m_time;
    uint32_t m_interval;
};

// One Shot Timer
class OneShotTimer{
public:
    OneShotTimer() : m_time(0), m_hasElasped(false) { }
    void set(uint32_t msec){
        m_time = micros();
        m_interval = msec * 1000;
        m_hasElasped = false;
    }
    void setMicroseconds(uint32_t usec){
        m_time = micros();
        m_interval = usec;
        m_hasElasped = false;
    }
    bool elapsed(){
        if(!m_hasElasped){
            if(m_time == 0) return false;
            uint32_t now = micros();
            uint32_t elapsed_time = now - m_time;
            if(elapsed_time >= m_interval){
                m_hasElasped = true;
                m_time = 0;
            }
        }
        return m_hasElasped;
    }
private:
    uint32_t m_time;
    uint32_t m_interval;
    bool m_hasElasped;
};

// Modulo Counter
class ModuloCounter{
public:
    ModuloCounter(int mod) : m_mod(mod), m_cnt(0){ }
    bool count(){
        m_cnt++;
        if(m_cnt >= m_mod){
            m_cnt = 0;
            return true;
        }else{
            return false;
        }
    }
private:
    int m_mod;
    int m_cnt;
};

// Edge Detection Mode
enum EdgeMode{
    RISING_EDGE,
    FALLING_EDGE,
    BOTH_EDGE
};

// Edge Detector
class EdgeDetector{
public:
    EdgeDetector(EdgeMode mode = RISING_EDGE) : m_mode(mode), m_first(true) { }
    void setMode(EdgeMode mode){
        m_mode = mode;
    }
    bool detect(int value){
        if(m_first){
            m_first = false;
            m_prevVal = value;
        }
        if(m_prevVal != value){
            m_prevVal = value;
            switch(m_mode){
                case RISING_EDGE:  return (value != 0);
                case FALLING_EDGE: return (value == 0);
                case BOTH_EDGE:    return true;
            }
        }else{
            m_prevVal = value;
            return false;
        }
    }
    void reset(){
        m_first = true;
    }
private:
    EdgeMode m_mode;
    bool m_first;
    int  m_prevVal;
};

#endif
