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
  this->vector_id = 2; 
}

void ProcessManager::init()
{
}

void ProcessManager::setVectorID(int id)
{
  this->vector_id = id;
}

// Processes dataset information and returns a serialized byte buffer according to vector_id(0, 1, 2)
uint8_t *ProcessManager::processData(DataSet *ds, int *dlen)
{
  uint8_t *ret = (uint8_t *)malloc(BUFLEN);  // Allocate memory for output buffer
  memset(ret, 0, BUFLEN);                    // Initialize buffer to zero
  *dlen = 0;                                 // Initialize data length to zero
  uint8_t *p = ret;                          // Pointer to the current write position in the buffer

  // Retrieve temperature and humidity data from the dataset
  TemperatureData *tdata = ds->getTemperatureData();
  HumidityData *hdata = ds->getHumidityData();

  // Get the number of house entries in the dataset
  int num = ds->getNumHouseData();

  // Declare timestamp and time structure
  time_t ts;
  struct tm *tm;
  float month, year;

  ts = ds->getTimestamp();              // Get the dataset's timestamp
  tm = localtime(&ts);                  // Convert timestamp to local time (struct tm)
  month = tm->tm_mon + 1;               // Extract month (tm_mon is 0-based, so add 1)
  year = tm->tm_year + 1900;            // Extract year (tm_year is years since 1900)

  float max_temp = tdata->getMax();     // Retrieve max temperature from temperature data
  float max_humid = hdata->getMax();    // Retrieve max humidity from humidity data
  float avg_humid = hdata->getValue();  // Retrieve average humidity

  // Initialize power and humidity sums
  float power_sum = 0.0;                
  float humidity_sum = 0.0;

   // Debug output of month and year
  printf("month: %f, year: %f\n", month, year);

  // Sum power consumption across all houses
  for (int i = 0; i < num; ++i) {
    HouseData *house = ds->getHouseData(i);
    PowerData *pdata = house->getPowerData();
    power_sum += pdata->getValue();
  }
  
  // Compute average power consumption
  float avg_power = power_sum / num;

  // === Branch based on vector ID ===
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
    // Invalid vector ID; print warning
    cout << "[!] Unknown vector ID: " << vector_id << endl;
  }
  // Return the serialized byte buffer
  return ret;
}