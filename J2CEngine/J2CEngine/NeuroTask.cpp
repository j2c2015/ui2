#define N_FRAMEWORK_NATIVE
#include "NeuroTask.h"
#include "multicorequeuetemplate.h"

#include <NCore.hpp>
#include <NBiometricClient.hpp>
#include <NBiometrics.hpp>
#include <NMedia.hpp>
#include <NLicensing.hpp>
#include <opencv/cv.h>
#include "colorbufferinfo.h"
#include "bufferpoolmanager.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <sys/types.h>
#include <regex>
#include "imagesaver.h"
#include "controller.h"
// c++ 17 filesystem compile error
#if 1
// for older visual studio 2017
#include <experimental/filesystem>
#include <filesystem>
namespace fs = std::experimental::filesystem::v1;
#else
// for latest visual studio 2017
namespace fs = std::filesystem;
#endif

using namespace std;
using namespace Neurotec;
using namespace Neurotec::Licensing;
using namespace Neurotec::Biometrics;
using namespace Neurotec::Biometrics::Client;
//using namespace Neurotec::IO;
//using namespace Neurotec::Devices;
using namespace Neurotec::Images;

NeuroTask gNeuroTask;
NeuroTask::NeuroTask()
{
	bioClient_ = nullptr;
	currentState_ = IDLE;
	worker_ = nullptr;
	stopFlag_ = false;
	enrolled_ = false;
}

void NeuroTask::__enroll(SharedBuffer &buf)
{
	//NBiometricClient client;
	std::cout << "### enroll start ###" << std::endl;
	NBuffer nbuf(buf->getBuffer(), WIDTH_FOR_EYE_VIEW * HEIGHT_FOR_EYE_VIEW, false);
	NSizeType size = WIDTH_FOR_EYE_VIEW * HEIGHT_FOR_EYE_VIEW;
	NPixelFormat format = NPF_GRAYSCALE_8U;

	NImage nimage = NImage::FromData(format, WIDTH_FOR_EYE_VIEW, HEIGHT_FOR_EYE_VIEW, WIDTH_FOR_EYE_VIEW, WIDTH_FOR_EYE_VIEW, nbuf);
	NIris iris;
	iris.SetImage(nimage);
	NSubject subject;
	subject.GetIrises().Add(iris);
	NBiometricStatus status = bioClient_->CreateTemplate(subject);
	if (status == nbsOk)
	{
		std::cout << "Succes make template" << std::endl;
		
	}
	else
	{
		std::cout << "Failed to make template" << std::endl;
		getController().enrollProxyCb(false);
		return;
	}
	//NFile::WriteAllBytes("C:\\work\\template\\2.templ", subject.GetTemplateBuffer());
	std::string filename = workdir_ + "\\" + "enroll" + "\\" + currentEnrollName_ + "_" + std::to_string(enrollCnt_++);
	std::string path = filename + ".tpl";
	NFile::WriteAllBytes(path.c_str(), subject.GetTemplateBuffer());
	getImageSaver().saveGrayImage(buf, filename + ".png", WIDTH_FOR_EYE_VIEW, HEIGHT_FOR_EYE_VIEW);
	getController().enrollProxyCb(true);
	return;
}

bool NeuroTask::__match(SharedBuffer &buf)
{
	std::cout << "------  Neuro match  -----" << std::endl;
	NBuffer idbuf(buf->getBuffer(), WIDTH_FOR_EYE_VIEW*HEIGHT_FOR_EYE_VIEW, false);
	NSizeType idsize = WIDTH_FOR_EYE_VIEW * HEIGHT_FOR_EYE_VIEW;
	NPixelFormat idformat = NPF_GRAYSCALE_8U;
	NImage idnimage = NImage::FromData(idformat, WIDTH_FOR_EYE_VIEW, HEIGHT_FOR_EYE_VIEW, WIDTH_FOR_EYE_VIEW, WIDTH_FOR_EYE_VIEW, idbuf);
	NIris idiris;
	idiris.SetImage(idnimage);
	NSubject idsubject;
	idsubject.GetIrises().Add(idiris);

	NBiometricStatus _status = bioClient_->Identify(idsubject);
	NSubject::MatchingResultCollection matchingResult = idsubject.GetMatchingResults();
	if (_status == nbsOk)
	{
		std::cout << "match found" << std::endl;

		for (int i = 0; i < matchingResult.GetCount(); i++)
		{
			NMatchingResult r(matchingResult.Get(i));
			TCHAR buffer[2048];
			std::string _id = r.GetId();
			if (i == 0)
			{
				// match result forwarding
				//getController().setMatchResult(_id);
			}
#if 0
			NString __id = r.GetId();
			HNString handle = __id.GetHandle();
			NInt size;
			const NAChar *ptr;
			::NStringGetBufferA(handle, &size, &ptr);
#endif
			std::cout << "Match id " << _id.c_str() << std::endl;
			std::cout << "Match score " << r.GetScore() << std::endl;
			//std::cout << "match id" <<  r.GetId();
			std::string extracted_id = _id;//extract(_id);
			NeuroTaskResult result;
			//result.result = _id;
			result.result = extracted_id;
			result.desc = std::to_string(r.GetScore());
			matchResultQueue_.putSharedBuffer(result);
			return true;
		}
	}
	else
	{
		std::cout << "match not found " << std::endl;
	}
	getController().identifyUnBlock();
	return false;
}

std::string NeuroTask::extract(std::string &id)
{
	std::string delimiter("_");
	std::size_t pos = id.find_last_of(delimiter);
	if (pos == std::string::npos)
		return "";

	std::string  extracted = id.substr(0, pos);
	return extracted;
}

void NeuroTask::__jpeg(SharedBuffer &buf)
{
	std::vector<uchar> res;
	cv::Mat image = cv::Mat(cvSize(WIDTH_FOR_EYE_VIEW, HEIGHT_FOR_EYE_VIEW), CV_8UC1, buf->getBuffer(), cv::Mat::AUTO_STEP);
	cv::imencode(".jp2", image, res);

	cv::imwrite("c:\\work\\result.jp2", res);
}


void NeuroTask::__enrollSaveAndReload()
{

	std::cout << "### enroll save and reload ###" << std::endl;
	if (bioClient_ != nullptr)
	{
		delete bioClient_;
		bioClient_ = new NBiometricClient;
	}
	else
		bioClient_ = new NBiometricClient;

	NBiometricTask enrollTask = bioClient_->CreateTask(nboEnroll, NULL);
	std::string _dir = workdir_ + "\\enroll";
	std::string delimiter("\\/");
	std::cout << "Enroll directory " << _dir << std::endl;
#if 1
	for (auto& p : fs::recursive_directory_iterator(_dir))
	{
		//directory file listing
		fs::path  path = p.path();
		std::string name = path.string();
		std::cout << "File name " << name << std::endl;
		std::size_t pos = name.find_last_of(delimiter);
		if (pos == std::string::npos)
			continue;
		std::string id = name.substr(pos+1);
		std::cout << "Listed template file =" << id << std::endl;
		if (id == "." || id == "..")
			continue;
		if (name.find(".png") != std::string::npos)
			continue;

		std::string _id="";
		std::regex reg("\\.");
		auto iter = std::sregex_token_iterator(id.begin(), id.end(), reg, -1);
		std::string __id = *iter;
		std::cout << "Extracted id name =" << __id << std::endl;

		auto _pos = __id.find_first_of("_");
		if (_pos != std::string::npos)
		{
			_id = __id.substr(0, _pos);
		}

		std::cout << "Final id = " << _id << std::endl;
		if (_id == "")
			continue;
		NSubject enrollSubject = NSubject::FromFile(name.c_str());
		enrollSubject.SetId(_id.c_str());
		enrollTask.GetSubjects().Add(enrollSubject);
	}
#endif

	bioClient_->PerformTask(enrollTask);
	if (enrollTask.GetStatus() != nbsOk)
	{
		std::cout << "Reload Enroll unsuccessful" << std::endl;
	}
	else
	{
		std::cout << "Reload Enroll successful" << std::endl;
	}
#if 0
	std::string _dir = workdir_ + "\\" + "enroll";
	QDir dir(_dir.c_str());
	QFileInfoList list = dir.entryInfoList();
	NBiometricTask enrollTask = bioClient_->CreateTask(nboEnroll, NULL);

	for (int i = 0; i < list.size(); i++)
	{

		QFileInfo fileInfo = list.at(i);
		std::string name = fileInfo.fileName().toStdString();
		if (name == "." || name == "..")
			continue;
		if (name.find(".png") != std::string::npos)
			continue;

		qDebug() << "template " << name.c_str();
		std::string path = _dir + "\\" + name;
		NSubject enrollSubject = NSubject::FromFile(path.c_str());

		std::regex reg("\\.");
		auto iter = std::sregex_token_iterator(name.begin(), name.end(), reg, -1);
		std::string id = *iter;
		std::cout << "File name " << name.c_str() << " id:" << id.c_str() << std::endl;
		enrollSubject.SetId(id.c_str());
		enrollTask.GetSubjects().Add(enrollSubject);
	}

	bioClient_->PerformTask(enrollTask);
	if (enrollTask.GetStatus() != nbsOk)
	{
		std::cout << "Enroll unsuccessful" << std::endl;
	}
	else
	{
		std::cout << "Enroll successful" << std::endl;
	}
#endif
	// match result forwarding
	//getController().setMatchResult("none");

}
