#ifndef _S5K3P9SP_H_
#define _S5K3P9SP_H_

#include <stdint.h>
#include "../../SensorDriver.h"


class S5K3P9SP : public SamsungSensor
{
public:
	S5K3P9SP();

    int do_prog_otp(int page, int addr, const void *data, int len);
    int do_read_otp(int page, int addr, void *data, int len);

	int do_get_sid(uint8_t *id);
    BOOL GetSensorId(__out CString &strSensorId);
	int wb_writeback(uint8_t *regs, int len);
};

#endif
