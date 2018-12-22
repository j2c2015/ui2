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

		int ScanHeight = getController().GetEyeFindScanHeight();
		int ScanWidth = getController().GetEyeFindScanWidth();
		int rowStart = (HEIGHT_FOR_CAMERA_CROP - ScanHeight) / 2;
		int colStart = (WIDTH_FOR_CAMERA_CROP - ScanWidth) / 2;
		int threshold = getController().GetEyeFindScanThreshold();
		int resultAdded = getController().GetEyeFindScanResultAdded();
		int areaMin = getController().GetEyeFindScanAreaMin();
		int areaMax = getController().GetEyeFindScanAreaMax();
		int lineMin = getController().GetEyeFindScanLineMin();
		int lineMax = getController().GetEyeFindScanLineMax();
		int ratioMin = getController().GetEyeFindScanRatioMin();
		bool bSaveMask = getController().GetEyeFindScanSaveMask();
		bool bSaveSpecular = getController().GetEyeFindScanSaveSpecular();
		bool bTimeLogging = getController().GetEyeFindScanTimeLogging();
		int nRowFindStart = getController().GetEyeFindScanTopIdx();
		int nRowFindEnd = getController().GetEyeFindScanBottomIdx();
		int nColFindStart = getController().GetEyeFindScanLeftIdx();
		int nColFindEnd = getController().GetEyeFindScanRightIdx();
		bool bCheckCond = getController().GetEyeFindScanCheckCond();
		bool bDistTimeLogging = getController().GetEyeDistTimeLogging();
		int nDistExcludeThreshold = getController().GetEyeDistExcludeThreshold();
		bool bSaveDist = getController().GetEyeDistSaveImage();
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
			
			/////////////////////////////////////////////////////////////////////////////
			//
			// cscho (2018-12.20)
			//
			//
			// mask crop image
			//
			int checkInterval = getController().GetEyeFindScanCheckInterval();
			CEllapsedTime timeFind;

			SharedBuffer bufMask = getBufferPoolManager().getBuffer(MASK_CROP_BUFFER_POOL_ID);
			SharedBuffer bufTest = getBufferPoolManager().getBuffer(MASK_TEST_BUFFER_POOL_ID);
			gFindSpecularIdx++;
			if (gFindSpecularIdx == checkInterval)
			{
				gFindSpecularIdx = 0;
				vecRoiSP.clear();
			}
			
			if (bufMask != nullptr && gFindSpecularIdx == 0)
			{
				unsigned char* src = (unsigned char*)buf->getBuffer();
				unsigned char* dest = (unsigned char*)bufMask->getBuffer();
				memset(dest, 0x00, FRAMESIZE_FOR_MASK_CROP);

				int nRowStart = 1;
				int nRowEnd = HEIGHT_FOR_CAMERA_CROP - 1;
				int nColStart = 1;
				int nColEnd = WIDTH_FOR_CAMERA_CROP - 1;				
				int nBaseValue = getController().MaskingChunkFromBuf(src, dest, nRowStart, nRowEnd, nColStart, nColEnd, threshold, bTimeLogging);
				if (nBaseValue > 1)
				{
					//
				}
				
				int nRoiCount = getController().FindSpecularCross(dest, nRowFindStart, nRowFindEnd, nColFindStart, nColFindEnd, nBaseValue, &vecRoiSP, resultAdded, bCheckCond, bTimeLogging);
				if (nRoiCount  > 0)
				{
					bool bFindEye = false;
					if (nRoiCount == 1)
						bFindEye = true;

					if (bFindEye)
					{
						RECT rtROI = vecRoiSP.at(0);

						// save mask
						if (bSaveMask)
						{
							unsigned char* test = (unsigned char*)bufTest->getBuffer();
							memset(test, 0x00, FRAMESIZE_FOR_MASK_TEST);
							for (int row = 0; row < HEIGHT_FOR_CAMERA_CROP; row++)
							{
								for (int col = 0; col < WIDTH_FOR_CAMERA_CROP; col++)
								{
									int nIdxPixel = ((row * WIDTH_FOR_CAMERA_CROP) + col);
									unsigned char value = dest[nIdxPixel];
									if (value >= nBaseValue)
										value = 255;
									test[nIdxPixel] = value;
								}
							}
							Mat _srcTest = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, test, cv::Mat::AUTO_STEP);
							char pathTest[256] = { 0, };
							sprintf(pathTest, "c:\\jtwoc\\eyetest\\test_mask_%s.png", buf->getName());
							imwrite(pathTest, _srcTest);
						}
						// save specular display
						if (bSaveSpecular)
						{
							cv::Rect roiSP;
							roiSP.x = rtROI.left;
							roiSP.y = rtROI.top;
							roiSP.width = (rtROI.right - rtROI.left);
							roiSP.height = (rtROI.bottom - rtROI.top);
							Mat _srcSP = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, src, cv::Mat::AUTO_STEP);
							Mat srcSP;
							cvtColor(_srcSP, srcSP, cv::COLOR_GRAY2BGR);
							rectangle(srcSP, roiSP, Scalar(255, 0, 255), 1);
							char pathSP[256] = { 0, };
							sprintf(pathSP, "c:\\jtwoc\\eyetest\\test_specular_%s.png", buf->getName());
							imwrite(pathSP, srcSP);
						}
						// calculate distance
						float fDeviation = getController().CalculateDistance(src, rtROI, nDistExcludeThreshold, bSaveDist, buf->getName(), bDistTimeLogging);
					}
				}

				/*
				//
				// find specular
				//
				std::vector<RECT> vecRoiAll;

				//int* test = (int*)bufTest->getBuffer();
				//memset(test, 0x00, FRAMESIZE_FOR_CAMERA_CROP);
				
				int nCheckValue = 0;
				int nRecurseCnt = 0;
				int nPixelCnt = 0;
				for (int row = (rowStart + 1); row < (HEIGHT_FOR_CAMERA_CROP - rowStart - 1); row++)
				{
					for (int col = (colStart + 1); col < (WIDTH_FOR_CAMERA_CROP - colStart - 1); col++)
					{
						nCheckValue++;
						
						if (vecRoiAll.size() > 0)
						{
							bool bExistPos = false;
							for (auto &&roi : vecRoiAll)
							{
								RECT rt = roi;
								rt.left -= 1;
								rt.top -= 1;
								rt.right += 1;
								rt.bottom += 1;
								POINT pt = { col, row };
								if (::PtInRect(&rt, pt))
								{
									bExistPos = true;
									col = rt.right;
									break;
								}
							}
							if (bExistPos)
							{
								continue;
							}
						}

						//
						// find specular
						//
						nRecurseCnt = 0;
						nPixelCnt = 0;
						RECT rtSP = { 0, };
						getController().FindSpecular(dest, row, col, threshold, rtSP, nPixelCnt);
						//getController().FindSpecularRecurse(dest, row, col, threshold, test, rtSP, nCheckValue, nPixelCnt, nRecurseCnt);
						if (::IsRectEmpty(&rtSP) == FALSE)
						{
							vecRoiAll.push_back(rtSP);

							int width = rtSP.right - rtSP.left;
							int height = rtSP.bottom - rtSP.top;
							int area = (width * height);
							float ratio = 0.0f;
							if (width > height)
							{
								ratio = ((float)height / (float)width);
							}
							else
							{
								ratio = ((float)width / (float)height);
							}
							ratio *= 100;
							
							if ((width > lineMin && height > lineMin) &&
								(width < lineMax && height < lineMax) &&
								(area > areaMin && area < areaMax) &&
								(ratio >= ratioMin))
							{
								bool bIntersect = false;
								if (vecRoiSP.size() > 0)
								{
									std::vector<RECT>::iterator iVec = vecRoiSP.begin();
									for ( ; iVec != vecRoiSP.end(); iVec++)
									{
										RECT roi = *iVec;
										RECT rtIntersect = { 0, };
										if (::IntersectRect(&rtIntersect, &roi, &rtSP))
										{
											::UnionRect(&rtIntersect, &roi, &rtSP);
											vecRoiSP.erase(iVec);
											vecRoiSP.push_back(rtIntersect);
											bIntersect = true;
											break;
										}
									}
								}
								if (!bIntersect)
								{
									vecRoiSP.push_back(rtSP);
								}
							}
						}						
					}
				}
				*/
				
				if (vecRoiSP.size() > 0)
				{
					//FUNCTION_NAME_IS(_T("Scaler::__internalJob"));
					//LOG_PRINTF(0, _T("[EyeFind] Finished finding specular. (Time= %0.5f)"), fTimeSpecular);
					/*
					char path[256] = "";
					sprintf(path, "c:\\jtwoc\\crop\\crop_roi_%s.png", buf->getName());
					char szFName[256] = "";
					sprintf(szFName, "crop_%s.png", buf->getName());
					std::string strPath = path;
					getImageSaver().saveGrayImage(buf, strPath, WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP);
					*/
					/*
					for (auto &&roi : vecRoiSP)
					{
						drawSpecular(forDisplay, roi);
						LOG_PRINTF(0, _T("[EyeFind] Resulting specular. (Name= %d) [Pos: %d,%d,%d,%d]"), atoi(buf->getName()), roi.x, roi.y, roi.x + roi.width, roi.y + roi.height);
					}
					*/
				}
				bufMask = nullptr;
				bufTest = nullptr;
			}			
			/////////////////////////////////////////////////////////////////////////////

			if (vecRoiSP.size() > 0)
			{
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
				// mask crop image
				//
				if (bufMask != nullptr)
				{
					char path[256] = "";
					sprintf(path, "c:\\jtwoc\\crop\\crop_%s_mask.png", buf->getName());
					std::string strPath = path;
					getImageSaver().saveGrayImage(bufMask, strPath, WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP);
				}

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
				std::string path = savePath_ + "\\" + "crop" + buf->getName()+".png";
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

