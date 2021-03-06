#ifndef _ZABER_STAGE_H_
#define _ZABER_STAGE_H_

#include <Havana2/Configuration.h>

#ifdef PULLBACK_DEVICE

#include "zb_serial.h"
#include <iostream>
#include <thread>


class ZaberStage
{
public:
	ZaberStage();
	~ZaberStage();

private:
	void SetCommandData(int cmd_data, uint8_t* msg);
	void GetReplyData(uint8_t* msg, int &reply_val);

public:
	bool ConnectStage();
	void DisconnectStage();
	void StopWaitThread();

	void Home();
	void Stop();
	void MoveAbsoulte(double position);
	void SetTargetSpeed(double speed);	

private:
	const char* device_name;
	int max_micro_resolution;
	int micro_resolution;
	double conversion_factor;

private:
	z_port port;
	double microstep_size;
	uint8_t home[6], stop[6], move_absolute[6], target_speed[6], received_msg[6];

private:
	std::thread t_wait;
	bool _running;

};

#endif
#endif
