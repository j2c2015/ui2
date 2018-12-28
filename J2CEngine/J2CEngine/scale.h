#pragma once

#include "stdafx.h"
#include "multicorequeue.h"
#include <opencv/cv.h>
#include "colorbufferinfo.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include "controller.h"
#include <stdint.h>
#include "imagesaver.h"
//#include "J2CIrisDemoDlg.h"
using namespace cv;

class Scaler
{
public:
	Scaler();

	void startJob()
	{
		stopFlag_ = false;
		jobThread_ = new std::thread(std::bind(&Scaler::__internalJob, this));
	}

	// thread entry function
	void __internalJob()
	{
		FUNCTION_NAME_IS(_T("Scaler::__internalJob"));
		LOG_PUTS(0, _T("Scaler thread starts..."));

		CreateDirectory(_T("c:\\jtwoc\\eyetest"), NULL);

		SharedBuffer buf = nullptr;
		while (stopFlag_ == false)
		{
			buf = getMultiCoreQueueForScaler().getSharedBufferWithTimeout(timeout_);
			if (buf == nullptr)
				continue;

			SharedBuffer forDisplay = getBufferPoolManager().getBuffer(CAMERA_CROP_BUFFER_POOL_ID);
			if (forDisplay == nullptr)
			{
				buf = nullptr;
				continue;
			}
			memcpy(forDisplay->getBuffer(), buf->getBuffer(), FRAMESIZE_FOR_CAMERA_CROP);
			//drawCircle(forDisplay);
			
			//
			// cscho (2018-12.22)
			//
			FILE* fp = NULL;
			if (getController().GetEyeFindScanViewSPAll())
				fp = LOG_PRINTF_FP(0, NULL, _T(""));
			unsigned char* src = (unsigned char*)buf->getBuffer();
			int nCntSPAll = 0, nCntSPUp = 0, nCntSPDn = 0;
			
			{
				ROI_RESULT roiResult;
				float fDeviationMax = getController().CallEyeFindTest(src, buf->getName(), nCntSPAll, nCntSPUp, nCntSPDn, &roiResult, fp);
				if (fDeviationMax >= getController().GetEyeFindDevValueRef())
				{
					int size = vecRoiSPCandidate.size();
					if (size < 5)
					{
						gTestBufId[size] = atoi(buf->getName());
					}
					roiResult.enrollSharedBuffer = buf;
					vecRoiSPCandidate.push_back(roiResult);
				}
			}
			if (vecRoiSPCandidate.size() >= getController().GetEyeFindDevValueGroupCnt())
			{
				std::sort(vecRoiSPCandidate.begin(), vecRoiSPCandidate.end(), sortByRoiDeviation);
			}
			if (fp)
			{
				fclose(fp);
			}

			if (vecRoiSPCandidate.size() >= getController().GetEyeFindDevValueGroupCnt())
			{
				ROI_RESULT& candidate = vecRoiSPCandidate.back();
				{
					if (getController().GetEyeFindUseOpenCV() == false)
					{
						gEnrollIdx++;
						//if (gEnrollIdx % 15 == 0)
						{
							/****************************************************/
							/*
							RECT rt = vecRoiSP.at(0);
							int cx = (rt.left + rt.right) / 2;
							int cy = (rt.top + rt.bottom) / 2;
							EnrollSharedBuffer enroll;
							enroll.id = buf->getName();
							enroll.opencvSharedBuffer = nullptr;
							enroll.cropSharedBuffer = candidate.enrollSharedBuffer;
							enroll.x = cx;
							enroll.y = cy;
							getEnrollMultiCoreQueue().putSharedBuffer(enroll);
							gEnrollIdx = 0;
							
							J2CRenderCb render = getController().getEnrollRenderCb();
							render(buf->getBuffer(), WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP, COLOR_DEPTH_FOR_CAMERA_CROP);
							*/
							/****************************************************/

							char pathEnroll[256] = "";
							sprintf(pathEnroll, "c:\\jtwoc\\eyetest\\test_enroll_%s.png", candidate.enrollSharedBuffer->getName());
							std::string strEnrollPath = pathEnroll;
							getImageSaver().saveGrayImage(candidate.enrollSharedBuffer, strEnrollPath, WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP);
						}
					}
					vecRoiSPCandidate.clear();
				}
				for (auto &&roi : vecRoiSP)
				{
					drawSpecular(forDisplay, roi);
				}
			}

			//g_pUI->Render((unsigned char *)buf->getBuffer());
			J2CRenderCb cb = getController().getRealTimeRenderCb();
			cb(forDisplay->getBuffer(), WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP, COLOR_DEPTH_FOR_CAMERA_CROP);

			int x, y;
			if (getController().isPartialRenderRequested(x, y) == true)
			{
				//
				// snap[3*3]
				//
				/*
				CEllapsedTime eTime;
				eTime.StartEllapsedTime();
				
				char pathDir[256] = "";
				sprintf(pathDir, "c:\\jtwoc\\crop\\snap_%s", buf->getName());
				::CreateDirectoryA(pathDir, NULL);

				char pathSnap[256] = "";
				int nCntSnap = 0;

				unsigned char tmpSnap[3][3];
				unsigned char* tmpBuf = (unsigned char*)buf->getBuffer();
				for (int row = 0; row < HEIGHT_FOR_CAMERA_CROP; row += 3)
				{
					if ((row + 3) > HEIGHT_FOR_CAMERA_CROP)
						break;

					for (int col = 0; col < WIDTH_FOR_CAMERA_CROP; col += 3)
					{
						if ((col + 3) > WIDTH_FOR_CAMERA_CROP)
							break;

						for (int i = 0; i < 3; i++)
						{
							for (int j = 0; j < 3; j++)
							{
								int nIdxPixel = (((row + i) * WIDTH_FOR_CAMERA_CROP) + (col + j));
								unsigned char value = tmpBuf[nIdxPixel];
								tmpSnap[i][j] = value;
							}
						}
						cv::Mat imgSnap(3, 3, CV_8UC1, tmpSnap);
						sprintf(pathSnap, "%s\\crop_%s_Row%dCol%d.png", pathDir, buf->getName(), row, col);
						//getImageSaver().saveGrayImage(tmpSnap, pathSnap, 3, 3);
						nCntSnap++;
					}
				}
				double fEllapsed = eTime.EndEllapsedTime();
				FUNCTION_NAME_IS(_T("Scaler::__internalJob"));
				LOG_PRINTF(0, _T("Save [crop-Snap] image file. [Name= %s, Ellapsed= %0.5f]"), buf->getName(), fEllapsed);
				*/
								
				// save 800*600
				{
					char path[256] = "";
					sprintf(path, "c:\\jtwoc\\crop\\crop_%s.png", buf->getName());
					char szFName[256] = "";
					sprintf(szFName, "crop_%s.png", buf->getName());

					std::string strPath = path;
					getImageSaver().saveGrayImage(buf, strPath, WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP);

					FUNCTION_NAME_IS(_T("Scaler::__internalJob"));
					LOG_PRINTF(0, _T("Save [crop] image file.[%d * %d] (Name= %s)"), WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP, szFName);
				}

				//std::cout << "Partial Data Extracting " << x << " " << y << std::endl;
				FUNCTION_NAME_IS(_T("Scaler::__internalJob"));
				LOG_PRINTF(0, _T("Partial Data Extracting...(X= %d, Y= %d)"), x, y);

				char *start = (char *)buf->getBuffer();
				char *dst = (char *)getController().getPartialWindow();
				start += (WIDTH_FOR_CAMERA_CROP * y);
				start += x;
				char *result = dst;
				
				/////////////////////////////////////////////////////////////////////////////
				//
				// cscho (2018-12.11)
				//
				/*
				for (int i = 0; i < PARTIAL_HEIGHT; i++)
				{
					memcpy(dst, start, PARITAL_WIDTH);			
					dst += PARITAL_WIDTH;
					start += WIDTH_FOR_CAMERA_CROP;
				}
				J2CRenderCb partial = getController().getPartialRender();
				partial(result, PARITAL_WIDTH, PARTIAL_HEIGHT, 1);
				*/
				int nPartialWidth = getController().getPartialWidth();
				int nPartialHeight = getController().getPartialHeight();

				for (int i = 0; i < nPartialHeight; i++)
				{
					memcpy(dst, start, nPartialWidth);
					dst += nPartialWidth;
					start += WIDTH_FOR_CAMERA_CROP;
				}
				J2CRenderCb partial = getController().getPartialRender();
				partial(result, nPartialWidth, nPartialHeight, 1);
				
				// save 64*64
				{
					Mat  _src = cv::Mat(cvSize(nPartialWidth, nPartialHeight), CV_8UC1, result, cv::Mat::AUTO_STEP);
					Mat src;
					cvtColor(_src, src, cv::COLOR_GRAY2BGR);
					char path[256];
					sprintf(path, "c:\\jtwoc\\partial\\partial_%s.png", buf->getName());
					char szFName[256] = "";
					sprintf(szFName, "partial_%s.png", buf->getName());

					imwrite(path, src);

					FUNCTION_NAME_IS(_T("Scaler::__internalJob"));
					LOG_PRINTF(0, _T("Save partial-image file.[%d * %d] (Name= %s)"), nPartialWidth, nPartialHeight, szFName);
				}
				/////////////////////////////////////////////////////////////////////////////
			}

			if (saveFlag_ == true)
			{
				std::string path = savePath_ + "\\" + "crop_" + buf->getName()+".png";
				getImageSaver().saveGrayImage(buf, path, WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP);
			}

			buf = nullptr;
			forDisplay = nullptr;

			index_++;
		}
	}

	void drawCircle(SharedBuffer &buf)
	{
		Mat src = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, buf->getBuffer(), cv::Mat::AUTO_STEP);
		circle(src, Point(CROP_FRAME_CENTER_X, CROP_FRAME_CENTER_Y), 150, Scalar(255, 255, 255), 5);
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// cscho (2018-12.20)
	//
	void drawSpecular(SharedBuffer& buf, RECT& rt)
	{
		cv::Rect roi;
		roi.x = rt.left;
		roi.y = rt.top;
		roi.width = (rt.right - rt.left);
		roi.height = (rt.bottom - rt.top);
		int nCX = roi.x + (roi.width / 2);
		int nCY = roi.y + (roi.height / 2);
		Mat _src = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, buf->getBuffer(), cv::Mat::AUTO_STEP);
		rectangle(_src, roi, Scalar(0, 0, 0), 1);
	}
	/////////////////////////////////////////////////////////////////////////////

	void stop()
	{
		stopFlag_ = true;
	}

	void setTimeout(int ms)
	{
		timeout_ = ms;
	}

	int getTimeout()
	{
		return timeout_;
	}

	void save(bool save, char *path)
	{
		saveFlag_ = save;
		savePath_ = path;
	}
	
	bool stopFlag_;
	int timeout_;
	std::thread *jobThread_;
	int index_;
	std::string savePath_ = "";
	bool saveFlag_ = false;
};

extern Scaler gScaler;
static inline Scaler &getScaler()
{
	return gScaler;
}

