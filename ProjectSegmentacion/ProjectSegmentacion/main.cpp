#include <iostream>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <rs.hpp>
#include <vector>
#include <math.h>

#define PI 3.14159265

#define WIDTH 640
#define HEIGHT 480

using namespace std;

//Guardar pixeles de profundidad en un vector
vector<cv::Point> *SegmentarDepthImage(cv::Mat &imageDepth, cv::Size &imageSize, vector<cv::Point> *datapoint);

//Guardar pixeles de imagen umbralizada en un vector
vector<cv::Point> *pointSegmentImage(cv::Mat &imageDepth, cv::Size &imageSize, vector<cv::Point> *datapoint);

//Regresa la imagen segmentada
cv::Mat SegmentarColorImage(cv::Mat &imageColor, cv::Size &imageSize, vector<cv::Point> *datapoint);

//Alinar Imagen de profundidad con imagen a color
cv::Mat alignImageDepth(cv::Mat &imageDepth);

//Umbralizar imagen a color
cv::Mat UmbralImageColor(cv::Mat &imageColor);

//obtener distancia
double getDistance(cv::Point centerPoint, cv::Mat imageAlignDepth, float scale);

int main() {

	//Dimensiones de la imagen de profundidad
	cv::Size imageSize(WIDTH, HEIGHT);

	//Iniciar contexto
	rs::context ctx;
	if (ctx.get_device_count() == 0){	return -1;}

	//Iniciar dispositvo
	rs::device *sr300Camara = ctx.get_device(0);
	if (sr300Camara == NULL){	return -1;}

	//Habilitar transmision
	sr300Camara->enable_stream(rs::stream::depth, imageSize.width, imageSize.height, rs::format::z16, 30);
	sr300Camara->enable_stream(rs::stream::color, imageSize.width, imageSize.height, rs::format::bgr8, 30);

	//Comenzar transmision
	sr300Camara->start();

	int userSelect = 0;
	cout << "Presione [1] para capturar la imagen:\n";
	cout << "Presione [2] para salir del programa:\n";
	cin >> userSelect;

	//Factor scalar
	double scalar = sr300Camara->get_depth_scale();
	printf("Factor scala %0.9f\n", sr300Camara->get_depth_scale());
	
	

	if (userSelect == 1)
	{
		//esperar frame
		sr300Camara->wait_for_frames();

		//Imagen
		cv::Mat imageDepth(imageSize, CV_16UC1, (uint16_t*)sr300Camara->get_frame_data(rs::stream::depth));
		cv::Mat imageColor(imageSize, CV_8UC3, (uint8_t*)sr300Camara->get_frame_data(rs::stream::color_aligned_to_depth));
		
	
		//Vector de Point de Depth Image
		vector<cv::Point> *p = new vector<cv::Point>;
		p = SegmentarDepthImage(imageDepth, imageSize, p);

		//Imagen Segmentada
		cv::Mat imageSeg(imageSize, CV_8UC3, cv::Scalar(0, 0, 0));
		imageSeg = SegmentarColorImage(imageColor, imageSize, p);

		//Umbralización
		cv::Mat imgThreshold;
		imgThreshold = UmbralImageColor(imageSeg);

		//Vector de Point de Segment Image
		vector<cv::Point> *v = new vector<cv::Point>;
		v = pointSegmentImage(imgThreshold, imageSize, v);

		//encontrar contornos
		vector<vector<cv::Point> > contours;
		vector<cv::Vec4i> hierarchy;

		cv::Mat imageResult(imageSize, CV_8UC3, cv::Scalar(0, 0, 0));
		findContours(imgThreshold, contours, hierarchy,
			CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
		// iterate through all the top-level contours,
		// draw each connected component with its own random color
		int idx = 0;
		for (; idx >= 0; idx = hierarchy[idx][0])
		{
			cv::Scalar color(0, 0, 255);
			drawContours(imageResult, contours, idx, color, 1, 8, hierarchy);
		}
		// fin contornos

		// Get the moments
		vector<cv::Moments> mu(contours.size());
		for (int i = 0; i < contours.size(); i++)
		{
			mu[i] = moments(contours[i], false);
		}

		//  Get the mass centers:
		vector<cv::Point2f> mc(contours.size());
		for (int i = 0; i < contours.size(); i++)
		{
			mc[i] = cv::Point2f(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
		}

		int centerx = mc[0].x;
		int centery = mc[0].y;

		double distanceZ = getDistance(mc[0], imageDepth, scalar);

		double a = distanceZ * tan((71.5 / 2) * PI / 180.0);
		double l = distanceZ * tan((55.0 / 2.0)*PI / 180.0);

		cv::circle(imageResult, mc[0], 2, cv::Scalar(0, 255, 0));

		double area0 =  cv::contourArea(contours[0]);

		double areatotal = 0;

		if (mc.size() != 1)
		{
			int t = mc.size();
			double *area = new double[t];

			for (int i = 1; i < t; i++)
			{
				cv::circle(imageResult, mc[i], 2, cv::Scalar(0, 255, 0));
				
				area[i] = cv::contourArea(contours[i]);

				areatotal = areatotal + area[i];
				printf("El area de la figura es: %g cm2\n", (2.0*a / 640.0)*(2.0*l / 480.0)*area[i]);
			}
			delete[] area;
		}

		printf("numero de pixeles de profundidad es: %d \n", p->size());
		printf("El valor centro es p(%d, %d)\n", centerx, centery);
		printf("Distancia es %g\n", distanceZ);
		printf("Area en pixeles %g\n", area0);
		printf("El area de la figura es: %g cm2\n", (2.0*a / 640.0)*(2.0*l / 480.0)*area0);
		printf("El area TOTAL de la figura es: %g cm2\n", (2.0*a / 640.0)*(2.0*l / 480.0)*(area0+ areatotal));
		//printf("El tamño es: %d", p->size());
		//Mostrar imagen
		cv::imshow("Imagen Segmentada", imageSeg);
		cv::imshow("Imagen a Color", imageColor);
		cv::imshow("Alineacion Depth", imageDepth);
		//cv::imshow("Umbralizacion", imgThreshold);
		cv::imshow("Result", imageResult);

		sr300Camara->stop();
		cv::waitKey(0);	
	}
	else if (userSelect == 2)
	{
		cout << "Saliendo..." << endl;
	}


	return 0;
}


vector<cv::Point> *SegmentarDepthImage(cv::Mat &imageDepth, cv::Size &imageSize, vector<cv::Point> *datapoint) {

	for (int y = 0; y < imageSize.height; y++)
	{
		for (int x = 0; x < imageSize.width; x++)
		{
			if (imageDepth.at<uint16_t>(y, x) > 3500 || imageDepth.at<uint16_t>(y, x) == 0)
			{
				imageDepth.at<uint16_t>(y, x) = 0;
			}
			else{
				datapoint->push_back(cv::Point(x, y));
			}	
		}
	}
	return datapoint;
}


cv::Mat SegmentarColorImage(cv::Mat &imageColor, cv::Size &imageSize, vector<cv::Point> *datapoint) {
	
	cv::Mat imageSegment(imageSize, CV_8UC3, cv::Scalar(0, 0, 0));

	for (int i = 0; i < datapoint->size(); i++)
	{
		imageSegment.at<cv::Vec3b>(datapoint->at(i)) = imageColor.at<cv::Vec3b>(datapoint->at(i));
		//imageColor.at<cv::Vec3b>(datapoint->at(i)) = 0;
	}



	return imageSegment;
}

cv::Mat alignImageDepth(cv::Mat &imageDepth) {

	cv::Mat croppedImage;
	cv::Size sz(810, 600);
	//Zoom Image Depth
	//cv::pyrUp(imageDepth, imageDepth, imageSize * 5/4);
	//cv::resize(imageDepth, imageDepth, imageDepth.size() * 5 / 4, cv::INTER_LINEAR);
	cv::resize(imageDepth, imageDepth, sz, cv::INTER_LINEAR);
	//Trasladar imagen
	cv::Mat imgTranslated(imageDepth.size(), imageDepth.type(), cv::Scalar::all(0));
	imageDepth(cv::Rect(0, 75, imageDepth.cols - 0, imageDepth.rows - 75)).copyTo(imgTranslated(cv::Rect(0, 0, imageDepth.cols - 0, imageDepth.rows - 75)));

	/*
	//Limpiar imagen
	//Crear elemento para erosion y dilatacion
	int erosion_size = 15;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
	//cv::erode(imgTranslated, imgTranslated, element, cv::Point(-1, -1), 1);
	cv::dilate(imgTranslated, imgTranslated, element, cv::Point(-1, -1), 1);
	cv::erode(imgTranslated, imgTranslated, element, cv::Point(-1, -1), 1);
	*/
	cv::Rect myRoi(0, 0, 640, 480);

	croppedImage = imgTranslated(myRoi);

	
	return croppedImage;
}

cv::Mat UmbralImageColor(cv::Mat &imageColor) {

	//Imagen imgHSV para umbralizar
	cv::Mat imgHSV;
	cv::cvtColor(imageColor, imgHSV, cv::COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

	//Imagen umbralizada
	cv::Mat imgThresholded;
	cv::inRange(imgHSV, cv::Scalar(160, 100, 100), cv::Scalar(200, 255, 255), imgThresholded);

	//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
	int erosion_size = 4;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
	
	erode(imgThresholded, imgThresholded, element);
	dilate(imgThresholded, imgThresholded, element);

	
	return imgThresholded;
}

//Guardar pixeles de imagen umbralizada en un vector
vector<cv::Point> *pointSegmentImage(cv::Mat &imageDepth, cv::Size &imageSize, vector<cv::Point> *datapoint) {

	for (int y = 0; y < imageSize.height; y++)
	{
		for (int x = 0; x < imageSize.width; x++)
		{
			if (imageDepth.at<uchar>(y, x) == 255)
			{
				datapoint->push_back(cv::Point(x, y));
			}
		}
	}

	return datapoint;
}

double getDistance(cv::Point centerPoint, cv::Mat imageAlignDepth, float scale) {

	double distance;

	distance = imageAlignDepth.at<uint16_t>(centerPoint) * scale * 100;

	return distance;

}