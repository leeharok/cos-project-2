#include "process_manager.h"
#include "opcode.h"
#include "byte_op.h"
#include "setting.h"
#include <cstring>
#include <iostream>
#include <ctime>
#include <cmath>
using namespace std;

ProcessManager::ProcessManager()
{
  this->num = 0;
  this->vector_id = 2; // 기본은 5D
}

void ProcessManager::init()
{
}

void ProcessManager::setVectorID(int id)
{
  this->vector_id = id;
}

uint8_t *ProcessManager::processData(DataSet *ds, int *dlen)
{
  uint8_t *ret = (uint8_t *)malloc(BUFLEN);
  memset(ret, 0, BUFLEN);
  *dlen = 0;
  uint8_t *p = ret;

  TemperatureData *tdata = ds->getTemperatureData();
  HumidityData *hdata = ds->getHumidityData();
  int num = ds->getNumHouseData();

  time_t ts;
  struct tm *tm;
  float month, year;
  ts = ds->getTimestamp();
  tm = localtime(&ts);
  month = tm->tm_mon + 1;
  year = tm->tm_year + 1900;

  float max_temp = tdata->getMax();
  float max_humid = hdata->getMax();
  float avg_humid = hdata->getValue();

  float power_sum = 0.0;
  float humidity_sum = 0.0;

  printf("month: %f, year: %f\n", month, year);

  for (int i = 0; i < num; ++i) {
    HouseData *house = ds->getHouseData(i);
    PowerData *pdata = house->getPowerData();
    power_sum += pdata->getValue();
  }
  
  float avg_power = power_sum / num;

  // === 벡터 분기 처리 ===
  if (vector_id == 2) {
    // [max_humid, max_temp, month, year, avg_power]
    VAR_TO_MEM_4BYTES_BIG_ENDIAN(*((uint32_t *)&max_humid), p); *dlen += 4;
    VAR_TO_MEM_4BYTES_BIG_ENDIAN(*((uint32_t *)&max_temp), p);  *dlen += 4;
    VAR_TO_MEM_4BYTES_BIG_ENDIAN(*((uint32_t *)&month), p);     *dlen += 4;
    VAR_TO_MEM_4BYTES_BIG_ENDIAN(*((uint32_t *)&year), p);      *dlen += 4;
    VAR_TO_MEM_4BYTES_BIG_ENDIAN(*((uint32_t *)&avg_power), p); *dlen += 4;

  } else if (vector_id == 1) {
    // [max_temp, avg_humid, avg_power]
    VAR_TO_MEM_4BYTES_BIG_ENDIAN(*((uint32_t *)&max_temp), p);   *dlen += 4;
    VAR_TO_MEM_4BYTES_BIG_ENDIAN(*((uint32_t *)&avg_humid), p);  *dlen += 4;
    VAR_TO_MEM_4BYTES_BIG_ENDIAN(*((uint32_t *)&avg_power), p);  *dlen += 4;

  } else if (vector_id == 0) {
    // [discomfort_index, avg_power]
    float discomfort = 0.81 * max_temp + 0.01 * avg_humid * (0.99 * max_temp - 14.3) + 46.3;
    VAR_TO_MEM_4BYTES_BIG_ENDIAN(*((uint32_t *)&discomfort), p); *dlen += 4;
    VAR_TO_MEM_4BYTES_BIG_ENDIAN(*((uint32_t *)&avg_power), p);  *dlen += 4;

  } else {
    cout << "[!] Unknown vector ID: " << vector_id << endl;
  }

  return ret;
}