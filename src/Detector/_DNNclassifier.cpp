/*
 *  Created on: Jan 13, 2019
 *      Author: yankai
 */
#include "_DNNclassifier.h"

namespace kai
{

_DNNclassifier::_DNNclassifier()
{
	m_nW = 224;
	m_nH = 224;
	m_bSwapRB = true;
	m_vMean.init();
	m_scale = 1.0;

	m_iBackend = cv::dnn::DNN_BACKEND_OPENCV;
	m_iTarget = cv::dnn::DNN_TARGET_CPU;
}

_DNNclassifier::~_DNNclassifier()
{
}

bool _DNNclassifier::init(void* pKiss)
{
	IF_F(!this->_ObjectBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;

	KISSm(pK, nW);
	KISSm(pK, nH);
	KISSm(pK, bSwapRB);
	KISSm(pK, scale);
	KISSm(pK, iBackend);
	KISSm(pK, iTarget);
	pK->v("meanB",&m_vMean.x);
	pK->v("meanG",&m_vMean.y);
	pK->v("meanR",&m_vMean.z);

	m_net = readNet(m_trainedFile, m_modelFile);
	IF_Fl(m_net.empty(), "read Net failed");

	m_net.setPreferableBackend(m_iBackend);
	m_net.setPreferableTarget(m_iTarget);

	return true;
}

bool _DNNclassifier::start(void)
{
	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		LOG_E(retCode);
		m_bThreadON = false;
		return false;
	}

	return true;
}

void _DNNclassifier::update(void)
{
	while (m_bThreadON)
	{
		this->autoFPSfrom();

		IF_CONT(!classify());

		m_obj.update();

		if (m_bGoSleep)
		{
			m_obj.m_pPrev->reset();
		}

		this->autoFPSto();
	}
}

int _DNNclassifier::check(void)
{
	NULL__(m_pVision, -1);
	Frame* pBGR = m_pVision->BGR();
	NULL__(pBGR, -1);
	IF__(pBGR->bEmpty(), -1);
	IF__(pBGR->tStamp() <= m_BGR.tStamp(), -1);

	return 0;
}

bool _DNNclassifier::classify(void)
{
	IF_F(check() < 0);
	Frame* pBGR = m_pVision->BGR();
	m_BGR.copy(*pBGR);
	Mat mIn = *m_BGR.m();

	vInt4 iRoi;
	iRoi.x = mIn.cols * m_roi.x;
	iRoi.y = mIn.rows * m_roi.y;
	iRoi.z = mIn.cols * m_roi.z;
	iRoi.w = mIn.rows * m_roi.w;
	Rect rRoi;
	vInt42rect(iRoi, rRoi);
	Mat mBGR = mIn(rRoi);

	m_blob = blobFromImage(mBGR,
							m_scale,
							Size(m_nW, m_nH),
							Scalar(m_vMean.z, m_vMean.y, m_vMean.x),
							m_bSwapRB,
							false);
	m_net.setInput(m_blob);

	Mat mProb = m_net.forward();

    Point pClassID;
    double conf;
    cv::minMaxLoc(mProb.reshape(1, 1), 0, &conf, 0, &pClassID);
    IF_T(conf < m_minConfidence);

	OBJECT obj;
	obj.init();
	obj.m_tStamp = m_tStamp;
	obj.setTopClass(pClassID.x, conf);
	vInt2 cSize;
	cSize.x = mIn.cols;
	cSize.y = mIn.rows;
	obj.setBB(iRoi, cSize);

	this->add(&obj);
	LOG_I("Class: " + i2str(obj.m_topClass));

	return true;
}

bool _DNNclassifier::draw(void)
{
	IF_F(!this->_ObjectBase::draw());
	Window* pWin = (Window*) this->m_pWindow;
	Frame* pFrame = pWin->getFrame();
	Mat* pMat = pFrame->m();
	IF_F(pMat->empty());
	OBJECT* pO = m_obj.at(0);
	NULL_F(pO);

	int iClass = pO->m_topClass;
	IF_F(iClass >= m_nClass);
	IF_F(iClass < 0);

	string oName = m_vClass[iClass].m_name;
	if (oName.length()>0)
	{
		putText(*pMat, oName,
				Point(25, 100),
				FONT_HERSHEY_SIMPLEX, 2.0, Scalar(0,0,255), 5);
	}

	return true;
}

}
