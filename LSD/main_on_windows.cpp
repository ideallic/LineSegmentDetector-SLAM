#include <time.h>
#include <stdio.h>
#include <opencv.hpp>
#include <fstream>
#include <myLSD.h>
#include <myRDP.h>
#include <myFA.h>
//#include <FeatureAssociation.h>
#include <baseFunc.h>

using namespace cv;
using namespace std;

myfa::structFAInput trans2FA(myrdp::structFeatureScan FS, mylsd::structLSD LSD, Mat mapCache);

int main() {
	clock_t time_start, time_end;
	time_start = clock();
	//·��
	//string path1 = "../line_data/data0/";
	string path1 = "../data_20190523/data/";
	string path2;
	const char *path;
	//��ȡmapParam ��ͼ��Ϣ
	path2 = path1 + "mapParam.txt";
	path = path2.data();
	FILE *fp = fopen(path, "r");
	structMapParam mapParam;
	fscanf(fp, "%d %d %lf %lf %lf", &mapParam.oriMapCol, &mapParam.oriMapRow, &mapParam.mapResol, &mapParam.mapOriX, &mapParam.mapOriY);
	fclose(fp);
	int oriMapCol = mapParam.oriMapCol, oriMapRow = mapParam.oriMapRow;
	
	//��ȡmapValue ��ͼ��������
	int cnt_row, cnt_col;
	path2 = path1 + "mapValue.txt";
	path = path2.data();
	fp = fopen(path, "r");
	Mat mapValue = Mat::zeros(oriMapRow, oriMapCol, CV_8UC1);
	int max = 0;
	for (cnt_row = 0; cnt_row < oriMapRow; cnt_row++)
		for (cnt_col = 0; cnt_col < oriMapCol; cnt_col++)
			fscanf(fp, "%d", &mapValue.ptr<uint8_t>(cnt_row)[cnt_col]);
	fclose(fp);
	//imshow("1", mapValue);
	//waitKey(1);

	//����mapCache����������ƥ����������
	Mat mapCache = mylsd::createMapCache(mapValue, mapParam.mapResol, z_occ_max_dis);

	//LineSegmentDetector ��ȡ��ͼ�߽�ֱ����Ϣ
	mylsd::structLSD LSD = mylsd::myLineSegmentDetector(mapValue, oriMapCol, oriMapRow, 0.3, 0.6, 22.5, 0.7, 1024);
	Mat Display = LSD.lineIm.clone();

	//printf("%d %d\n", Display.size[0], Display.size[1]);
	imshow("mapCache", mapCache);
	imshow("Display", Display);
	waitKey(0);

	//��ȡ�״���Ϣ
	path2 = path1 + "Lidar.txt";
	path = path2.data();
	fp = fopen(path, "r");
	int i = 0, len_lp = 0, cnt_frame = 1;
	myrdp::structLidarPointPolar lidarPointPolar[360];
	while (!feof(fp)) {
		len_lp = 0;
		bool is_EOF = false;
		printf("��%d֡:\n", cnt_frame++);
		//ÿ֡���360֡���� ѭ����ȡ�����룩
		for (i = 0; i < pointPerLoop; i++) {
			double val1, val2;
			if (feof(fp)) {
				is_EOF = true;
				break;
			}
			fscanf(fp, "%lf%lf", &val1, &val2);

			if (val1 != INFINITY) {
				lidarPointPolar[len_lp].range = val1;
				lidarPointPolar[len_lp].angle = val2;
				lidarPointPolar[len_lp].split = false;
				len_lp++;
			}
		}
		if (is_EOF == false) {
			//ƥ���״���������ͼ���� ���������������ʵ����
			myrdp::structFeatureScan FS = FeatureScan(mapParam, lidarPointPolar, len_lp, 3, 0.08, 0.5);
			
			double estimatePose_realworld[3];
			double estimatePose[3];
			Mat poseAll;
			myfa::structFAInput FAInput = trans2FA(FS, LSD, mapCache);
			myfa::structScore FA = myfa::FeatureAssociation(&FAInput);

			printf("%f %f %f\n\n", FA.pos.x, FA.pos.y, FA.pos.ang);

			//��ͼ����������ͼ��
			circle(Display, Point ((int)FA.pos.x, (int)FA.pos.y), 1, Scalar(255, 255, 255));
			imshow("lineIm", Display);
			waitKey(1);
		}
		else
			destroyWindow("lineIm");
	}
	fclose(fp);

	imshow("MapGray", mapValue);
	time_end = clock();
	printf("time = %lf\n", (double)(time_end - time_start) / CLOCKS_PER_SEC);
	imshow("lineIm", Display);
	waitKey(0);
	destroyAllWindows();
	return 0;
}

myfa::structFAInput trans2FA(myrdp::structFeatureScan FS, mylsd::structLSD LSD, Mat mapCache) {
	//�����ݸ�ʽתΪFeatureAssociation��ʽ
	myfa::structFAInput FA;
	int i;
	//scanLinesInfo
	FA.scanLinesInfo.resize(FS.len_linesInfo);
	for (i = 0; i < FS.len_linesInfo; i++) {
		FA.scanLinesInfo[i].k = FS.linesInfo[i].k;
		FA.scanLinesInfo[i].b = FS.linesInfo[i].b;
		FA.scanLinesInfo[i].dx = FS.linesInfo[i].dx;
		FA.scanLinesInfo[i].dy = FS.linesInfo[i].dy;
		FA.scanLinesInfo[i].x1 = FS.linesInfo[i].x1;
		FA.scanLinesInfo[i].y1 = FS.linesInfo[i].y1;
		FA.scanLinesInfo[i].x2 = FS.linesInfo[i].x2;
		FA.scanLinesInfo[i].y2 = FS.linesInfo[i].y2;
		FA.scanLinesInfo[i].len = FS.linesInfo[i].len;
	}
	//mapLinesInfo
	FA.mapLinesInfo.resize(LSD.len_linesInfo);
	for (i = 0; i < LSD.len_linesInfo; i++) {
		FA.mapLinesInfo[i].k = LSD.linesInfo[i].k;
		FA.mapLinesInfo[i].b = LSD.linesInfo[i].b;
		FA.mapLinesInfo[i].dx = LSD.linesInfo[i].dx;
		FA.mapLinesInfo[i].dy = LSD.linesInfo[i].dy;
		FA.mapLinesInfo[i].x1 = LSD.linesInfo[i].x1;
		FA.mapLinesInfo[i].y1 = LSD.linesInfo[i].y1;
		FA.mapLinesInfo[i].x2 = LSD.linesInfo[i].x2;
		FA.mapLinesInfo[i].y2 = LSD.linesInfo[i].y2;
		FA.mapLinesInfo[i].len = LSD.linesInfo[i].len;
	}
	//lidarPos
	FA.lidarPos[0] = (int)round(FS.lidarPos.x);
	FA.lidarPos[1] = (int)round(FS.lidarPos.y);
	//printf("x:%lf y:%lf\n", FS.lidarPos.x, FS.lidarPos.y);
	FA.mapCache = mapCache;
	FA.scanImPoint = FS.scanImPoint;

	return FA;
}
