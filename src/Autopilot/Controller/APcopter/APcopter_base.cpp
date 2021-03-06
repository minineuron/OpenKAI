#include "../../Controller/APcopter/APcopter_base.h"

namespace kai
{

APcopter_base::APcopter_base()
{
	m_pMavlink = NULL;
	m_lastHeartbeat = 0;
	m_iHeartbeat = 0;
	m_apMode = 0;
	m_lastApMode = 0xffffffff;
	m_bApModeChanged = false;

	m_freqSendHeartbeat = 1;
	m_freqRawSensors = 0;
	m_freqExtStat = 1;
	m_freqRC = 0;
	m_freqPos = 1;
	m_freqExtra1 = 1;

	m_bHomeSet = false;
	m_vHomePos.init();
	m_vPos.init();
	m_apHdg = 0.0;
}

APcopter_base::~APcopter_base()
{
}

bool APcopter_base::init(void* pKiss)
{
	IF_F(!this->ActionBase::init(pKiss));
	Kiss* pK = (Kiss*)pKiss;

	KISSm(pK,freqSendHeartbeat);
	KISSm(pK,freqRawSensors);
	KISSm(pK,freqExtStat);
	KISSm(pK,freqRC);
	KISSm(pK,freqPos);
	KISSm(pK,freqExtra1);

	if(m_freqSendHeartbeat > 0)
		m_freqSendHeartbeat = USEC_1SEC / m_freqSendHeartbeat;
	else
		m_freqSendHeartbeat = 0;

	m_lastHeartbeat = 0;
	m_iHeartbeat = 0;

	string iName;
	iName = "";
	pK->v("_Mavlink", &iName);
	m_pMavlink = (_Mavlink*) (pK->root()->getChildInst(iName));
	NULL_F(m_pMavlink);

	return true;
}

int APcopter_base::check(void)
{
	NULL__(m_pMavlink,-1);

	return 0;
}

void APcopter_base::update(void)
{
	this->ActionBase::update();
	IF_(check()<0);

	//update Ardupilot
	m_apMode = m_pMavlink->m_msg.heartbeat.custom_mode;
	if(m_apMode != m_lastApMode)
	{
		m_bApModeChanged = true;
		m_lastApMode = m_apMode;
	}
	else
	{
		m_bApModeChanged = false;
	}

	//get home position
	if(m_tStamp - m_pMavlink->m_msg.time_stamps.home_position > USEC_1SEC)
	{
		m_pMavlink->clGetHomePosition();
	}
	else
	{
		m_vHomePos.x = ((double)(m_pMavlink->m_msg.home_position.latitude)) * 1e-7;
		m_vHomePos.y = ((double)(m_pMavlink->m_msg.home_position.longitude)) * 1e-7;
		m_vHomePos.z = ((double)(m_pMavlink->m_msg.home_position.altitude)) * 1e-3;
		m_bHomeSet = true;
	}

	//get position
	m_vPos.x = ((double)(m_pMavlink->m_msg.global_position_int.lat)) * 1e-7;
	m_vPos.y = ((double)(m_pMavlink->m_msg.global_position_int.lon)) * 1e-7;
	m_vPos.z = ((double)(m_pMavlink->m_msg.global_position_int.relative_alt)) * 1e-3;
	m_apHdg = ((double)(m_pMavlink->m_msg.global_position_int.hdg)) * 1e-2;

	//Send Heartbeat
	if(m_freqSendHeartbeat > 0 && m_tStamp - m_lastHeartbeat >= m_freqSendHeartbeat)
	{
		m_pMavlink->heartbeat();
		m_lastHeartbeat = m_tStamp;
	}

	//request updates from Mavlink
	if(m_freqRawSensors > 0 && m_tStamp - m_pMavlink->m_msg.time_stamps.raw_imu > USEC_1SEC)
		m_pMavlink->requestDataStream(MAV_DATA_STREAM_RAW_SENSORS, m_freqRawSensors);

	if(m_freqExtStat > 0 && m_tStamp - m_pMavlink->m_msg.time_stamps.mission_current > USEC_1SEC)
		m_pMavlink->requestDataStream(MAV_DATA_STREAM_EXTENDED_STATUS, m_freqExtStat);

	if(m_freqRC > 0 && m_tStamp - m_pMavlink->m_msg.time_stamps.rc_channels_raw > USEC_1SEC)
		m_pMavlink->requestDataStream(MAV_DATA_STREAM_RC_CHANNELS, m_freqRC);

	if(m_freqPos > 0 && m_tStamp - m_pMavlink->m_msg.time_stamps.global_position_int > USEC_1SEC)
		m_pMavlink->requestDataStream(MAV_DATA_STREAM_POSITION, m_freqPos);

	if(m_freqExtra1 > 0 && m_tStamp - m_pMavlink->m_msg.time_stamps.attitude > USEC_1SEC)
		m_pMavlink->requestDataStream(MAV_DATA_STREAM_EXTRA1, m_freqExtra1);

}

void APcopter_base::setApMode(uint32_t iMode)
{
	IF_(check()<0);

	mavlink_set_mode_t D;
	D.custom_mode = iMode;
	m_pMavlink->setMode(D);
}

uint32_t APcopter_base::getApMode(void)
{
	return m_apMode;
}

string APcopter_base::getApModeName(void)
{
	if(m_apMode >= AP_N_CUSTOM_MODE)return "UNDEFINED";

	return custom_mode_name[m_apMode];
}

int APcopter_base::getApModeByName(const string& name)
{
	for(int i=0; i<AP_N_CUSTOM_MODE; i++)
	{
		IF_CONT(custom_mode_name[i] != name);
		return i;
	}

	return -1;
}

bool APcopter_base::bApModeChanged(void)
{
	return m_bApModeChanged;
}

void APcopter_base::setApArm(bool bArm)
{
	IF_(check()<0);

	m_pMavlink->clComponentArmDisarm(bArm);
}

bool APcopter_base::getApArm(void)
{
	return false; //TODO
}

bool APcopter_base::getHomePos(vDouble3* pHome)
{
	NULL_F(pHome);
	IF_F(!m_bHomeSet);

	*pHome = m_vHomePos;
	return true;
}

vDouble3 APcopter_base::getPos(void)
{
	return m_vPos;
}

double APcopter_base::getHdg(void)
{
	return getApHdg();
}

double APcopter_base::getApHdg(void)
{
	return m_apHdg;
}

void APcopter_base::setMount(AP_MOUNT& m)
{
	IF_(check()<0);

	m_pMavlink->mountControl(m.m_control);
	m_pMavlink->mountConfigure(m.m_config);

	mavlink_param_set_t D;
	D.param_type = MAV_PARAM_TYPE_INT8;
	string id;

	D.param_value = m.m_config.stab_pitch;
	id = "MNT_STAB_TILT";
	strcpy(D.param_id, id.c_str());
	m_pMavlink->param_set(D);

	D.param_value = m.m_config.stab_roll;
	id = "MNT_STAB_ROLL";
	strcpy(D.param_id,id.c_str());
	m_pMavlink->param_set(D);
}

bool APcopter_base::draw(void)
{
	IF_F(!this->ActionBase::draw());
	Window* pWin = (Window*)this->m_pWindow;
	Mat* pMat = pWin->getFrame()->m();

	pWin->addMsg(*this->getName());

	pWin->tabNext();

	pWin->addMsg("apMode = " + i2str(m_apMode) + ": " + getApModeName());

	pWin->addMsg("y=" + f2str(m_pMavlink->m_msg.attitude.yaw) +
			", p=" + f2str(m_pMavlink->m_msg.attitude.pitch) +
			", r=" + f2str(m_pMavlink->m_msg.attitude.roll));

	pWin->addMsg("hdg=" + f2str(((float)m_pMavlink->m_msg.global_position_int.hdg)*1e-2));

	pWin->addMsg("alt=" + f2str(((float)m_pMavlink->m_msg.global_position_int.alt)*1e-3)
		  + ", relAlt=" + f2str(((float)m_pMavlink->m_msg.global_position_int.relative_alt)*1e-3));

	pWin->addMsg("lat=" + f2str(((float)m_pMavlink->m_msg.global_position_int.lat)*1e-7, 7)
		  + ", lon=" + f2str(((float)m_pMavlink->m_msg.global_position_int.lon)*1e-7, 7));

	pWin->addMsg("Home: lat=" + f2str(m_vHomePos.x, 7)
				 + ", lon=" + f2str(m_vHomePos.y, 7)
				 + ", alt=" + f2str(m_vHomePos.z, 7)
				 );

	if(m_freqRawSensors > 0)
	{
		pWin->addMsg("xAcc=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.xacc)
					 + ", yAcc=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.yacc)
					 + ", zAcc=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.zacc));

		pWin->addMsg("xGyro=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.xgyro)
					 + ", yGyro=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.ygyro)
					 + ", zGyro=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.zgyro));

		pWin->addMsg("xMag=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.xmag)
					 + ", yMag=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.ymag)
					 + ", zMag=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.zmag));
	}

	pWin->tabPrev();

	return true;
}

bool APcopter_base::console(int& iY)
{
	IF_F(!this->ActionBase::console(iY));
	IF_F(check()<0);

	string msg;

	C_MSG("apMode = " + i2str(m_apMode) + ": " + getApModeName());

	C_MSG("y=" + f2str(m_pMavlink->m_msg.attitude.yaw) +
			", p=" + f2str(m_pMavlink->m_msg.attitude.pitch) +
			", r=" + f2str(m_pMavlink->m_msg.attitude.roll));

	C_MSG("hdg=" + f2str(((float)m_pMavlink->m_msg.global_position_int.hdg)*1e-2));

	C_MSG("alt=" + f2str(((float)m_pMavlink->m_msg.global_position_int.alt)*1e-3)
		  + ", relAlt=" + f2str(((float)m_pMavlink->m_msg.global_position_int.relative_alt)*1e-3));

	C_MSG("lat=" + f2str(((float)m_pMavlink->m_msg.global_position_int.lat)*1e-7, 7)
		  + ", lon=" + f2str(((float)m_pMavlink->m_msg.global_position_int.lon)*1e-7, 7));

	C_MSG("Home: lat=" + f2str(m_vHomePos.x, 7)
				 + ", lon=" + f2str(m_vHomePos.y, 7)
				 + ", alt=" + f2str(m_vHomePos.z, 7)
				 );

	if(m_freqRawSensors > 0)
	{
		C_MSG("xAcc=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.xacc)
					 + ", yAcc=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.yacc)
					 + ", zAcc=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.zacc));

		C_MSG("xGyro=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.xgyro)
					 + ", yGyro=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.ygyro)
					 + ", zGyro=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.zgyro));

		C_MSG("xMag=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.xmag)
					 + ", yMag=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.ymag)
					 + ", zMag=" + i2str((int32_t)m_pMavlink->m_msg.raw_imu.zmag));
	}

	return true;
}

}
