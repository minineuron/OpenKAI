/*
 * _Lightware_SF40.h
 *
 *  Created on: Nov 4, 2016
 *      Author: yankai
 */

#ifndef AI__Lightware_SF40_H_
#define AI__Lightware_SF40_H_

#include "../Base/common.h"
#include "../Filter/Median.h"
#include "../Sensor/_Lightware_SF40_sender.h"
#include "../IO/SerialPort.h"

#define DEG_AROUND 360.0

namespace kai
{

using std::string;
using namespace std;
using namespace cv;

struct ZONE
{
	double m_x;
	double m_y;
	double m_n;

	void init(void)
	{
		m_x = 0.0;
		m_y = 0.0;
		m_n = 0.0;
	}

	void avr(void)
	{
		if(m_n<=0.0)
		{
			init();
			return;
		}

		double base = 1.0/m_n;
		m_x *= base;
		m_y *= base;
	}
};

class _Lightware_SF40: public _ThreadBase
{
public:
	_Lightware_SF40();
	~_Lightware_SF40();

	bool init(void* pKiss);
	bool link(void);
	bool start(void);
	bool draw(void);

	void setHeading(double hdg);
	vDouble2 getDiffRelativePos(void);
	void reset(void);

private:
	bool connect(void);
	bool updateLidar(void);
	void updateRelativePos(void);
	bool readLine(void);

	void update(void);
	static void* getUpdateThread(void* This)
	{
		((_Lightware_SF40*) This)->update();
		return NULL;
	}

public:
	SerialPort* m_pIn;
	_Lightware_SF40_sender* m_pSF40sender;

	double	m_hdg;
	double	m_offsetAngle;
	int		m_nDiv;
	double	m_dAngle;
	Median* m_pDist;

	double	m_minDist;
	double	m_maxDist;
	uint8_t m_MBS;
	string m_strRecv;
	double m_showScale;
	int64_t m_tStartUp;

	vDouble2 m_dPos;
	vDouble2 m_lastPos;
	double	m_varianceLim;
	double	m_diffMax;
	double	m_diffMin;
};

}

#endif
