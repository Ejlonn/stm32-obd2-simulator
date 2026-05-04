// sensor_model.c - ECU Emulator — Fizik Tabanlı Sensör Simülasyon Modeli

#include "sensor_model.h"
#include "project_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#define Q8(val)  ((int32_t)((val) * 256))

#define RPM_IDLE            Q8(800)
#define RPM_REDLINE         Q8(7000)
#define RPM_SHIFT_UP        Q8(3500)     
#define RPM_SHIFT_DOWN      Q8(1500)     
#define RPM_AFTER_UPSHIFT   Q8(2200)     
#define RPM_AFTER_DOWNSHIFT Q8(3000)     

#define RPM_ACCEL_RATE      Q8(40)       
#define RPM_DECEL_RATE      Q8(25)       
#define RPM_IDLE_PULL       Q8(5)        

static const uint8_t s_gear_speed_factor[7] = {
    0,  
    1,  
    3,  
    4,  
    5,  
    7,  
    8,  
};

#define SHIFT_GAP_TICKS     4            

#define TEMP_AMBIENT        Q8(25)       
#define TEMP_COOLANT_TARGET Q8(90)       
#define TEMP_OIL_TARGET     Q8(95)       
#define TEMP_MAX_COOLANT    Q8(120)      
#define TEMP_MAX_OIL        Q8(145)      

#define THERMAL_HEAT_RATE   2            
#define THERMAL_COOL_RATE   4            

#define SCENARIO_ACCEL_TICKS   270      
#define SCENARIO_CRUISE_TICKS  50        
#define SCENARIO_DECEL_TICKS   60        
#define SCENARIO_IDLE_TICKS    30        
#define SCENARIO_TOTAL_TICKS   (SCENARIO_ACCEL_TICKS + SCENARIO_CRUISE_TICKS + \
                                SCENARIO_DECEL_TICKS + SCENARIO_IDLE_TICKS)

#define FAULT_TARGET_RPM        Q8(600)
#define FAULT_TARGET_COOLANT    Q8(118)
#define FAULT_TARGET_OIL        Q8(142)
#define FAULT_TARGET_SPEED      Q8(0)
#define FAULT_TRANSITION_SHIFT  6        

typedef enum {
    SHIFT_NONE = 0,     
    SHIFT_UP,           
    SHIFT_DOWN          
} ShiftState_t;

typedef struct {
    int32_t  rpm;               
    int32_t  vehicle_speed;     
    int32_t  coolant_temp;      
    int32_t  oil_temp;          

    uint8_t  gear;              
    ShiftState_t shift_state;   
    uint8_t  shift_timer;       
    int32_t  rpm_target;        

    bool     throttle;          
    uint32_t scenario_tick;     
} Drivetrain_t;

static Drivetrain_t s_dt;
static SensorData_t s_data;
static SensorProfile_t s_profile     = SENSOR_PROFILE_NORMAL;
static bool            s_fault_active = false;

static inline int32_t lerp_q8(int32_t current, int32_t target, uint8_t shift)
{
    return current + ((target - current) >> shift);
}

static inline int32_t clamp_q8(int32_t val, int32_t min_val, int32_t max_val)
{
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

// Hızı vites ve RPM'den hesapla
static int32_t compute_speed(int32_t rpm_q8, uint8_t gear)
{
    if (gear == 0 || gear > 6) return 0;
    int32_t rpm_int = rpm_q8 >> 8;  
    
    return rpm_int * (int32_t)s_gear_speed_factor[gear];
}

// Sürüş senaryosuna göre gaz durumunu belirle
static bool scenario_get_throttle(uint32_t tick)
{
    
    uint32_t phase = tick;
    if (phase >= SCENARIO_TOTAL_TICKS) {
        phase = SCENARIO_TOTAL_TICKS - 1;
    }

    if (phase < SCENARIO_ACCEL_TICKS) {
        return true;   
    }
    if (phase < SCENARIO_ACCEL_TICKS + SCENARIO_CRUISE_TICKS) {
        return true;   
    }
    return false;      
}

// Termal modeli güncelle
static void update_thermals(void)
{
    int32_t rpm_int = s_dt.rpm >> 8;
    int32_t spd_int = s_dt.vehicle_speed >> 8;

    int32_t coolant_heat  = (rpm_int - 2000) >> 5;   
    int32_t coolant_cool  = (spd_int + 20) >> 4;     
    int32_t coolant_delta = coolant_heat - coolant_cool;

    if (s_dt.coolant_temp < TEMP_COOLANT_TARGET) {
        
        coolant_delta += 3;  
    }

    s_dt.coolant_temp += coolant_delta;
    s_dt.coolant_temp = clamp_q8(s_dt.coolant_temp, TEMP_AMBIENT, TEMP_MAX_COOLANT);

    int32_t oil_heat  = (rpm_int - 1500) >> 5;
    int32_t oil_cool  = (spd_int + 10) >> 4;
    int32_t oil_delta = oil_heat - oil_cool;

    if (s_dt.oil_temp < TEMP_OIL_TARGET) {
        oil_delta += 2;
    }

    s_dt.oil_temp += oil_delta;
    s_dt.oil_temp = clamp_q8(s_dt.oil_temp, TEMP_AMBIENT, TEMP_MAX_OIL);
}

// Normal profil: Drivetrain fizik simülasyonu
static void update_drivetrain(void)
{
    s_dt.scenario_tick++;

    s_dt.throttle = scenario_get_throttle(s_dt.scenario_tick);

    if (s_dt.shift_state != SHIFT_NONE)
    {
        s_dt.shift_timer++;

        s_dt.rpm = lerp_q8(s_dt.rpm, s_dt.rpm_target, 2);  

        if (s_dt.shift_timer >= SHIFT_GAP_TICKS)
        {
            s_dt.rpm = s_dt.rpm_target;
            s_dt.shift_state = SHIFT_NONE;
            s_dt.shift_timer = 0;

            s_dt.vehicle_speed = compute_speed(s_dt.rpm, s_dt.gear);
        }
        
        return;
    }

    if (s_dt.throttle)
    {
        
        s_dt.rpm += RPM_ACCEL_RATE;
    }
    else
    {
        
        s_dt.rpm -= RPM_DECEL_RATE;

        if (s_dt.rpm < RPM_IDLE) {
            s_dt.rpm = lerp_q8(s_dt.rpm, RPM_IDLE, 2);
        }
    }

    s_dt.rpm = clamp_q8(s_dt.rpm, RPM_IDLE, RPM_REDLINE);

    if (s_dt.rpm >= RPM_SHIFT_UP && s_dt.gear < 6)
    {
        s_dt.gear++;
        s_dt.shift_state  = SHIFT_UP;
        s_dt.shift_timer  = 0;
        s_dt.rpm_target   = RPM_AFTER_UPSHIFT;
        
        return;
    }

    if (s_dt.rpm <= RPM_SHIFT_DOWN && s_dt.gear > 1 && !s_dt.throttle)
    {
        s_dt.gear--;
        s_dt.shift_state  = SHIFT_DOWN;
        s_dt.shift_timer  = 0;
        s_dt.rpm_target   = RPM_AFTER_DOWNSHIFT;
        return;
    }

    s_dt.vehicle_speed = compute_speed(s_dt.rpm, s_dt.gear);

    s_dt.vehicle_speed = clamp_q8(s_dt.vehicle_speed, 0, Q8(250));
}

// Fault profili: Yavaşça arıza hedeflerine kayma
static void update_fault_profile(void)
{
    s_dt.rpm           = lerp_q8(s_dt.rpm,           FAULT_TARGET_RPM,     FAULT_TRANSITION_SHIFT);
    s_dt.coolant_temp  = lerp_q8(s_dt.coolant_temp,  FAULT_TARGET_COOLANT, FAULT_TRANSITION_SHIFT);
    s_dt.oil_temp      = lerp_q8(s_dt.oil_temp,      FAULT_TARGET_OIL,     FAULT_TRANSITION_SHIFT);
    s_dt.vehicle_speed = lerp_q8(s_dt.vehicle_speed,  FAULT_TARGET_SPEED,  FAULT_TRANSITION_SHIFT);

    if (s_dt.gear > 1 && (s_dt.rpm >> 8) < 1200) {
        s_dt.gear = 1;
    }
}

static void drivetrain_to_data(void)
{
    taskENTER_CRITICAL();

    int32_t rpm_int = s_dt.rpm >> 8;
    s_data.rpm           = (uint16_t)(rpm_int > 0 ? rpm_int : 0);
    s_data.coolant_temp  = (int16_t)(s_dt.coolant_temp >> 8);
    s_data.oil_temp      = (int16_t)(s_dt.oil_temp >> 8);

    int32_t spd = s_dt.vehicle_speed >> 8;
    s_data.vehicle_speed = (uint8_t)(spd > 255 ? 255 : (spd > 0 ? spd : 0));
    s_data.engine_running = (s_data.rpm > 0);

    taskEXIT_CRITICAL();
}

void SensorModel_Init(void)
{
    memset(&s_dt, 0, sizeof(s_dt));

    s_dt.rpm           = RPM_IDLE;
    s_dt.gear          = 1;
    s_dt.vehicle_speed = 0;
    s_dt.coolant_temp  = TEMP_AMBIENT;   
    s_dt.oil_temp      = TEMP_AMBIENT;
    s_dt.shift_state   = SHIFT_NONE;
    s_dt.throttle      = false;
    s_dt.scenario_tick = 0;

    s_profile      = SENSOR_PROFILE_NORMAL;
    s_fault_active = false;

    drivetrain_to_data();

    DEBUG_LOG(DEBUG_LEVEL_INFO, "SensorModel: Init OK (drivetrain physics v3.0)\r\n");
}

void SensorModel_Update(void)
{
    if (s_profile == SENSOR_PROFILE_NORMAL)
    {
        
        update_drivetrain();
        update_thermals();
    }
    else
    {
        
        update_fault_profile();
    }

    drivetrain_to_data();
}

void SensorModel_SetProfile(SensorProfile_t profile)
{
    s_profile = profile;
}

SensorProfile_t SensorModel_GetProfile(void)
{
    return s_profile;
}

void SensorModel_GetData(SensorData_t *data)
{
    if (data == NULL) return;

    taskENTER_CRITICAL();
    *data = s_data;
    taskEXIT_CRITICAL();
}

uint8_t SensorModel_GetGear(void)
{
    return s_dt.gear;
}

void SensorModel_SetFaultInjection(bool active)
{
    s_fault_active = active;
    s_profile = active ? SENSOR_PROFILE_FAULT : SENSOR_PROFILE_NORMAL;

    DEBUG_LOG(DEBUG_LEVEL_INFO, "SensorModel: Fault injection %s\r\n",
              active ? "ACTIVATED" : "DEACTIVATED");
}

bool SensorModel_IsFaultActive(void)
{
    return s_fault_active;
}
