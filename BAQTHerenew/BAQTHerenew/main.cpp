#include <QtCore/QCoreApplication>
#include "stdafx.h"
#include "TransformationCalculation.h"
#include "MyCloud.h"
#include "camera.h"
#include "CBA/ceresBA.h"
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <pcl/io/pcd_io.h>//PCL��PCD��ʽ�ļ����������ͷ�ļ�  
#include <pcl/point_types.h>//PCL�Ը��ָ�ʽ�ĵ��֧��ͷ�ļ�

using namespace amnl;
//savePoint(QString _t1, QByteArray _t2, QString _t3, int _t4);
QString m_rootfolder = "D:/VSProject/BA/";

int main(int argc, char *argv[])
{
	//QCoreApplication a(argc, argv);
	int a,i;
	std::cout << "started";
	int scantimes = 11; //The number of poses
	//Rvec and Tvec
	std::vector<cv::Mat> rvecsMat; //��ά��ת��������ʽ����ô������Ҫ����ת����ת��Ϊ��ת���� Ӧ���Ǹ�11*3
	std::vector<cv::Mat> tvecsMat; 
	rvecsMat.resize(scantimes);
	tvecsMat.resize(scantimes);
	for (int rti = 0; rti < scantimes; rti++) {
		rvecsMat[rti].create(1, 3, CV_64FC1);
		tvecsMat[rti].create(1, 3, CV_64FC1);
	}
	Camera m_cameraL;
	Camera m_cameraR;
	TransformationCalculation* transformcal = new TransformationCalculation;
	MyCloud mycloud; //mycloud ���沢û�б��湫���㣬ֻ��������ά�㣬������ʵ������Ҫ�������е�����ϵ�ֵ������ƺ���Ҫ�����Լ�����ȥ
	std::vector<cv::Point3f> m_totalmarkerPts;

	string intrinsicsFile = (m_rootfolder + QString("Config/intrinsics.yml")).toStdString();
	string extrinsicsFile = (m_rootfolder + QString("Config/extrinsics.yml")).toStdString();

	//read the config
	string configFile = (m_rootfolder + QString("Config/config.yml")).toStdString();
	Config::setParameterFile(configFile);

	//read the cameras' parameters
	m_cameraL.loadCamMatrix(intrinsicsFile, "M1");
	m_cameraL.loadDistortionCoeffs(intrinsicsFile, "D1");
	cv::Mat matR = cv::Mat::eye(3, 3, CV_64FC1);
	cv::Mat matT = cv::Mat::zeros(3, 1, CV_64FC1);
	m_cameraL.setRotationMatrix(matR);
	m_cameraL.setTranslationVector(matT);
	m_cameraL.loadRMatrix(extrinsicsFile, "R1");
	m_cameraL.loadPMatrix(extrinsicsFile, "P1");
	m_cameraL.loadQMatrix(extrinsicsFile, "Q");

	m_cameraR.loadCamMatrix(intrinsicsFile, "M2");
	m_cameraR.loadDistortionCoeffs(intrinsicsFile, "D2");
	m_cameraR.loadRotationMatrix(extrinsicsFile, "R");
	m_cameraR.loadTranslationVector(extrinsicsFile, "T");
	m_cameraR.loadRMatrix(extrinsicsFile, "R2");
	m_cameraR.loadPMatrix(extrinsicsFile, "P2");
	m_cameraR.loadQMatrix(extrinsicsFile, "Q");

	Reconstruction::Ptr reconstructor;
	//load the images
	int cloud_num = 0;
	std::vector<MyCloud> mycloud_vec_2;


	//��־��ȡ
	std::vector<std::vector<cv::Point2f>> imagePoints;  //float���ͣ�scanf lfʱ��������
	imagePoints.resize(scantimes); //ȷ���˵���11�У�ÿ�д���һ������ʱ��ͼ�е������

	std::vector<std::vector<unsigned short>> order_vector;
	order_vector.resize(scantimes);



	for (int index = 1;index < 12;index++) {
		
		std::vector<cv::Mat> left_image;
		std::vector<cv::Mat> right_image;
		QString left_str, right_str;
		for (size_t sizei = 1; sizei < 13; sizei++)
		{
			left_str = QString("D:/VSProject/BA/PCD/ScanImages/Camera1/%1_Measurement_%2.bmp").arg(index).arg(sizei);
			left_image.push_back(cv::imread(left_str.toStdString(), CV_LOAD_IMAGE_GRAYSCALE));
			right_str = QString("D:/VSProject/BA/PCD/ScanImages/Camera2/%1_Measurement_%2.bmp").arg(index).arg(sizei);
			right_image.push_back(cv::imread(right_str.toStdString(), CV_LOAD_IMAGE_GRAYSCALE));
		}

		cloud_num++;
		char img_buffer[100];
		sprintf_s(img_buffer, "Point%d.pcd", cloud_num);
		string str = string(img_buffer);

		
		mycloud.cloud.reset(new PointCloudT);
		reconstructor->denseReconstruction_cv(left_image, right_image, m_cameraL, m_cameraR, *mycloud.cloud);
		//mycloud_vec_1.push_back(mycloud);
		mycloud.trans_cloud.reset(new PointCloudT);

		left_str = QString("D:/VSProject/BA/PCD/ScanImages/Camera1/%1_Measurement_13.bmp").arg(index);
		right_str = QString("D:/VSProject/BA/PCD/ScanImages/Camera2/%1_Measurement_13.bmp").arg(index);

		cv::Mat img_l = cv::imread(left_str.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);
		cv::Mat img_r = cv::imread(right_str.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);

		double dRegistRrr = 100.0f;
		transformcal->Calculation(&mycloud, img_l, img_r, m_cameraL.getCamMatrix(),
			m_cameraL.getDistortionCoeffs(), m_cameraL.getRotationMatrix(), m_cameraL.getTranslationVector(),
			m_cameraR.getCamMatrix(), m_cameraR.getDistortionCoeffs(), m_cameraR.getRotationMatrix(),
			m_cameraR.getTranslationVector(), dRegistRrr, m_totalmarkerPts);
		//�����RT�Ǵӵ�2/3/4��ͼת�Ƶ���һ��ͼ��RT//������BA������RT�Ǵӵ�һ��ͼ��������ͼ��RT
		std::vector<cv::Point3f> markers;
		markers = mycloud.getMarkers();

		if (dRegistRrr < 1.0f)
		{
			transformcal->Addmarkers(markers, m_totalmarkerPts, mycloud.getRotationMatrix(), mycloud.getTranslationVector());
			FILE* op = fopen("D:\\VSProject\\BA\\REP\\points\\21.txt", "r");
			int npo = markers.size();
			imagePoints[index-1].resize(npo);
			order_vector[index - 1].resize(npo);
			for (i = 0;i < npo;i++) {
				//fscanf(op,"%lf %lf %lf", &imagePoints[index-1][i].x, &imagePoints[index - 1][i].y, &order_vector[index - 1][i]);
				fscanf(op, "%f %f %hu", &imagePoints[index - 1][i].x, &imagePoints[index - 1][i].y, &order_vector[index - 1][i]);

			}
			//�������ǾͰ�observation-x-y�Ͷ�Ӧ����ά�����Ÿ�����ȥ��

			for (size_t i = 0; i < mycloud.trans_cloud->size(); i++)
			{
				mycloud.trans_cloud->points[i].a = 255;
			}

			mycloud.filename = str;
			mycloud_vec_2.push_back(mycloud);

		}
		else
		{
			//QMessageBox::information(this, tr("Data stitching error"),
				//tr("Can't find or match the marker!"));
			std::cout << "Data stitching error";
		}
		// save pcd then
		//char* buffer_temp;
		//buffer_temp = new char[200000000];
		//memset(buffer_temp, 0, 200000000);
		
		std::vector<pcl::PointXYZRGBA, Eigen::aligned_allocator<pcl::PointXYZRGBA>> data;
		//data = mycloud_vec_2[index-1].trans_cloud->points;
		data = mycloud_vec_2[index - 1].trans_cloud->points;
		//memcpy(buffer_temp, &data[0], sizeof(pcl::PointXYZRGBA)*data.size());
		std::string file_name = "D:/VSProject/BA/pcdbeforeBA/" + to_string(index) + ".pcd";
		//QByteArray dataArray = QByteArray::fromRawData(buffer_temp, sizeof(pcl::PointXYZRGBA)*data.size());
		//emit savePoint(QString::fromStdString(file_name), dataArray, "pcd", data.size());
		//���浽PCD�ļ�
		//pcl::io::savePCDFileASCII(file_name, mycloud.cloud);//�����Ʊ��浽PCD�ļ���
		//std::cerr << "Saved" << mycloud.cloud.size() << "data points to test_pcd.pcd" << std::endl;
		//��ʾ��������
		// ����һ������

		
		pcl::PointCloud<pcl::PointXYZ> cloud;

		// ����������
		cloud.width = data.size();
		cloud.height = 1;
		cloud.is_dense = true;
		cloud.points.resize(cloud.width * cloud.height);

		for (size_t i = 0; i < cloud.points.size(); ++i)
		{
			cloud.points[i].x = data[i].x;
			cloud.points[i].y = data[i].y;
			cloud.points[i].z = data[i].z;
		}

		// д��������ݵ� PCD �ļ�
		pcl::io::savePCDFileASCII(file_name, cloud);
		std::cerr << "Saved " << cloud.points.size() << " data points to pcdbeforeba"<< i << ".pcd." << std::endl;
		 
	}
		//// Addingggggggggggg BA 
	//write down point loction
	/*
	FILE* mop = fopen("mtotalpoints.txt", "w");
	for (int mi = 0;mi<m_totalmarkerPts.size();mi++) {
		fprintf(mop, "%f %f %f\n", m_totalmarkerPts[mi].x, m_totalmarkerPts[mi].y, m_totalmarkerPts[mi].z);
	}
	fclose(mop);
	*/

	int num_observations = 0, num_poses = scantimes, num_points = m_totalmarkerPts.size();

		for (const auto &imagePts : imagePoints)
			num_observations += imagePts.size(); //�۲ⷽ�̵ĸ���
		BALProblem BAP; 
		BAP.setParaNums(num_observations, num_poses, num_points); 
		double *observations = BAP.mutable_observations();
		double *params = BAP.mutable_parameters();
		//int *camera_idx = BAP.mutable_camera_index();
		int *pose_idx = BAP.mutable_pose_index();
		int *point_idx = BAP.mutable_point_index();

		for (int i = 0; i < imagePoints.size(); ++i)
		{
			for (int j = 0; j < imagePoints[i].size(); ++j) { //ÿһ�еĹ۲⵽�ĵ���Ŀ����һ��
				//*camera_idx++ = 0;
				*pose_idx++ = i;
				*point_idx++ = order_vector[i][j]; //������θ�points����أ�������Ѱ��ˣ����ǿ��Ը�mtotal_points����ĵ��ţ�Ȼ��ÿ���һ���㣬��ȥ�������������ĸ���õ��ģ������������
				*observations++ = imagePoints[i][j].x;//��������ڼӵ��ʱ�������������Ҫ�����������ô�ӵ�
				*observations++ = imagePoints[i][j].y;
			}
		}

		double partr[9];
		double partt[3];
		double phere[9];
		cv::Mat pr_vec;
		cv::Mat pR_matrix;
		cv::Mat partrre;
		cv::Mat ptt;
		cv::Mat transt;
		for (i = 1;i <= scantimes;i++) {

			pR_matrix = mycloud_vec_2[i - 1].getRotationMatrix();
			ptt = mycloud_vec_2[i - 1].getTranslationVector();
			cv::invert(pR_matrix, partrre);
			//��TҲ�����
			transt = -partrre*ptt;
			cv::Rodrigues(partrre, pr_vec); //�������ת����pr_vec
			

			double parttimecv = 0;
			for (int ri = 0; ri < 3; ri++) {
				parttimecv = pr_vec.at<double>(ri);
				*params++ = parttimecv;
			}
			double parttimecv2 = 0;
			for (int ti = 0; ti < 3; ti++) {
				parttimecv2 = transt.at<double>(ti);
				*params++ = parttimecv2;
			} 

			}

		//3D wolrd control points
		//for (const auto &p : tmp_objectPoints[0])
		for (const auto &p : m_totalmarkerPts)
		{
			*params++ = p.x;
			*params++ = p.y;
			*params++ = p.z;
		}
		BAP.WriteToFile("problem.txt");
		const BundleParams BAparams;
		double final_cost = ceresBA::SolveProblem(BAP, BAparams, false);
		BAP.WriteToFile("solved.txt");
		params = BAP.mutable_parameters();

		for (int i = 0; i < rvecsMat.size(); i++)
		{
			double *r = rvecsMat[i].ptr<double>(0);
			double *t = tvecsMat[i].ptr<double>(0);
			for (int ri = 0; ri < 3; ri++)  r[ri] = *params++;
			for (int ti = 0; ti < 3; ti++)  t[ti] = *params++;
			//�������ǿ������¼������ת�����ƽ������
		}
		std::cout << "RMS After BA: " << std::sqrt(final_cost / num_observations) << endl;

		for (i = 0; i < scantimes; i++) {
			cv::Mat partrr = rvecsMat[i];
			cv::Mat rnine;
			cv::Rodrigues(partrr,rnine);
			cv::Mat ptt2 = tvecsMat[i];
			cv::transpose(ptt2, ptt2);
			cv::Mat partrre2;
			cv::invert(rnine,partrre2);
			cv::Mat transt2;
			transt2 = -partrre2*ptt2;
			//double* pdR = rnine.data;
			std::vector<pcl::PointXYZRGBA, Eigen::aligned_allocator<pcl::PointXYZRGBA>> data2;
			data2 = mycloud_vec_2[i].cloud->points;
			std::string file_name = "D:/VSProject/BA/pcdafterBA/" + to_string(i+1) + ".pcd";
			pcl::PointCloud<pcl::PointXYZ> cloud2;

			// ����������
			cloud2.width = data2.size();
			cloud2.height = 1;
			cloud2.is_dense = true;
			cloud2.points.resize(cloud2.width * cloud2.height);

			for (size_t k = 0; k < cloud2.points.size(); ++k)
			{
				cloud2.points[k].x = partrre2.at<double>(0, 0)* data2[k].x + partrre2.at<double>(0, 1)* data2[k].y + partrre2.at<double>(0, 2)* data2[k].z + transt2.at<double>(0);
				cloud2.points[k].y = partrre2.at<double>(1, 0)* data2[k].x + partrre2.at<double>(1, 1)* data2[k].y + partrre2.at<double>(1, 2)* data2[k].z + transt2.at<double>(1);
				cloud2.points[k].z = partrre2.at<double>(2, 0)* data2[k].x + partrre2.at<double>(2, 1)* data2[k].y + partrre2.at<double>(2, 2)* data2[k].z + transt2.at<double>(2);

			}

			// д��������ݵ� PCD �ļ�
			pcl::io::savePCDFileASCII(file_name, cloud2);
			std::cerr << "Saved " << cloud2.points.size() << " data points to pcdafterBA" << i << ".pcd." << std::endl;

		}

	std::cout << "finished";
	return 0;
}
