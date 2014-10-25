#include "UserProtocol.h"

#define TYPE_POWER              0
#define TYPE_DIRECTION          1
#define TYPE_BREAK              2
#define TYPE_LEFT_BREAK         3
#define TYPE_RIGHT_BREAK        4

const uint8_t m_left_in1 = 4;
const uint8_t m_left_in2 = 5;
const uint8_t m_left_pwm = 10;
const uint8_t m_left_deadband = 50;
const uint8_t m_right_in1 = 6;
const uint8_t m_right_in2 = 7;
const uint8_t m_right_pwm = 11;
const uint8_t m_right_deadband = 50;

int16_t m_power = 0;
int16_t m_direction = 0;

UserProtocol conn(50);

void MLeftSpeed(int8_t speed)
{
  if (speed > 100) {
    speed = 100;
  }
  if (speed < -100) {
    speed = -100;
  }
  
  uint8_t drived_pwm = 0;
  
  if (speed > 0) {
    digitalWrite(m_left_in1, LOW);
    digitalWrite(m_left_in2, HIGH);
    drived_pwm = m_left_deadband + map(speed, 0, 100, 0, (255 - m_left_deadband));
    analogWrite(m_left_pwm, drived_pwm);
  } else {
    digitalWrite(m_left_in1, HIGH);
    digitalWrite(m_left_in2, LOW);
    drived_pwm = m_left_deadband + map(-speed, 0, 100, 0, (255 - m_left_deadband));
    analogWrite(m_left_pwm, drived_pwm);
  }
}

void MLeftBreak()
{
  digitalWrite(m_left_in1, LOW);
  digitalWrite(m_left_in2, LOW);
  analogWrite(m_left_pwm, 0);
}

void MRightSpeed(int8_t speed)
{
  if (speed > 100) {
    speed = 100;
  }
  if (speed < -100) {
    speed = -100;
  }
  
  uint8_t drived_pwm = 0;
  
  if (speed > 0) {
    digitalWrite(m_right_in1, LOW);
    digitalWrite(m_right_in2, HIGH);
    drived_pwm = m_right_deadband + map(speed, 0, 100, 0, (255 - m_right_deadband));
    analogWrite(m_right_pwm, drived_pwm);
  } else {
    digitalWrite(m_right_in1, HIGH);
    digitalWrite(m_right_in2, LOW);
    drived_pwm = m_right_deadband + map(-speed, 0, 100, 0, (255 - m_right_deadband));
    analogWrite(m_right_pwm, drived_pwm);
  }
}

void MRightBreak()
{
  digitalWrite(m_right_in1, LOW);
  digitalWrite(m_right_in2, LOW);
  analogWrite(m_right_pwm, 0);
}

void MUpdate()
{
  int8_t m_left_speed = 0;
  int8_t m_right_speed = 0;
  
  
  if (m_power + m_direction > 100) {
    m_left_speed = 100;
  } else if (m_power + m_direction < -100) {
    m_left_speed = -100;
  } else {
    m_left_speed = m_power + m_direction;
  }
  
  if (m_power - m_direction > 100) {
    m_right_speed = 100;
  } else if (m_power - m_direction < -100){
    m_right_speed = -100;
  } else {
    m_right_speed = m_power - m_direction; 
  }
  
  MLeftSpeed(m_left_speed);
  MRightSpeed(m_right_speed);
}

void MPower(int8_t speed)
{
  if (speed > 100) {
    speed = 100;
  }
  
  if (speed < -100) {
    speed = -100;
  }
  
  m_power = speed;
  
  MUpdate();
}

void MDirection(int8_t direction)
{
  if (direction > 100) {
    direction = 100;
  }
  
  if (direction < -100) {
    direction = -100;
  }
  
  m_direction = direction;
  
  MUpdate();
}

void setup()
{
  Serial.begin(115200);
  
  pinMode(m_left_in1, OUTPUT);
  pinMode(m_left_in2, OUTPUT);
  pinMode(m_left_pwm, OUTPUT);
  pinMode(m_right_in1, OUTPUT);
  pinMode(m_right_in2, OUTPUT);
  pinMode(m_right_pwm, OUTPUT);
  
  MLeftBreak();
  MRightBreak();
}

void loop()
{ 
  while (Serial.available()) {
    if (conn.processIncome(Serial.read()) > 0) {
      uint8_t *type;
      int8_t *value;
      type = conn.incomePtr();
      value = (int8_t*)(conn.incomePtr() + 1);
      switch((*type)) {
      case TYPE_POWER :
        MPower(*value);
        break;
      case TYPE_DIRECTION :
        MDirection(*value);
        break;
      case TYPE_BREAK : 
        MLeftBreak();
        MRightBreak();
        break;
      case TYPE_LEFT_BREAK :
        MLeftBreak();
        break;
      case TYPE_RIGHT_BREAK :
        MRightBreak();
        break;
      default :
        break;
      }
    }
  }
}
