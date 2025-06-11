#ifndef __PROCESS_MANAGER_H__
#define __PROCESS_MANAGER_H__

#include "data/dataset.h"
#include <cstdint>

class ProcessManager {
private:
    int num;
    int vector_id;

public:
    ProcessManager();
    void init();

    void setVectorID(int id);
    uint8_t *processData(DataSet *ds, int *dlen);
};

#endif /* __PROCESS_MANAGER_H__ */