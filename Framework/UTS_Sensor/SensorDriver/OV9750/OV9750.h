#ifndef _OV9750_H_
#define _OV9750_H_

#include <stdint.h>
#include "../../SensorDriver.h"

#define OTP_OV9750_MAX_SIZE     415
#define OTP_OV9750_START_ADDR   0x7220

class OV9750 : public OVSensor
{
public:
	OV9750();

    int do_prog_otp(int page, int addr, const void *data, int len);
    int do_read_otp(int page, int addr, void *data, int len);

    int do_get_sid(uint8_t *id);
    BOOL GetSensorId(__out CString &strSensorId);
	int wb_writeback(uint8_t *regs, int len);
};

#endif
