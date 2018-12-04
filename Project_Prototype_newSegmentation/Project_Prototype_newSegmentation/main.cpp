/*
Nuevo método de segmentación de objetos
4 de enero de 2017
*/
#include <iostream>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <rs.hpp>
#include <vector>
#include <math.h>

#define PI 3.14159265

using namespace std;

/*------------------------------------------
		Prototype Functions
--------------------------------------------*/
int initializeCamaraSR300(int index);//void alignImageDepth(cv::Mat &imageDepth); //Alinar Imagen de profundidad con imagen a colo
void SegmentarDepthImage(cv::Mat &imageDepth, cv::Mat &imageColor); //Eliminar fondo imagen de profundidad
void drawCountorToImageColor(cv::Mat &imageGray, cv::Mat &imageColor, cv::Mat &imageDepth, int R, int G, int B); //Dibujar todos los contornos
								
double getDistance(cv::Point centerPoint, cv::Mat &imageAlignDepth, float scale);	//obtener distancia
/*------------------------------------------
			Variables Globales
--------------------------------------------*/
//Dimensiones de la imagen de profundidad
cv::Size imageSize(640, 480);
//cv::Size imageSizeColor(1920, 1080);
cv::Size imageSizeColorHD(1280, 720);

//Objetos Camara
rs::context ctx;
rs::device *sr300Device = NULL;

double scalar = 0;



int main() {

	//Inicializar Camaraa SR300
	if (initializeCamaraSR300(0)) {
		return -1;
	}

	//Inicar Transmisión
	sr300Device->start();

	//Crear nombre de ventana
	//cv::namedWindow("Depth Image", CV_WINDOW_FULLSCREEN);
	cv::namedWindow("Color Image", CV_WINDOW_FULLSCREEN);

	

	while (true)
	{
		//Esperar por una frame
		sr300Device->wait_for_frames();
		//Declarar objecto para almacenar imagen de profundidad
		cv::Mat depthImageNative(imageSize, CV_16UC1, (uint16_t*)sr300Device->get_frame_data(rs::stream::depth));
		cv::Mat imageColor(imageSize, CV_8UC3, (uint8_t*)sr300Device->get_frame_data(rs::stream::color_aligned_to_depth));


		SegmentarDepthImage(depthImageNative, imageColor);

		//Convertir al Espacio HSV
		cv::cvtColor(imageColor, imageColor, CV_BGR2HSV);
		//cv::GaussianBlur(imageColor, imageColor, cv::Size(7, 7), 1, 1);

		//Convetir a escala de grises
		cv::Mat imageGray;
		cv::cvtColor(imageColor, imageGray, CV_BGR2GRAY);
		cv::GaussianBlur(imageGray, imageGray, cv::Size(7, 7), 1, 1);
		cv::threshold(imageGray, imageGray, 165, 170, CV_THRESH_BINARY);

		//Convertir imagen HSV a BGR
		cv::cvtColor(imageColor, imageColor, CV_HSV2BGR);

		//Dibujar contornos
		drawCountorToImageColor(imageGray, imageColor, depthImageNative, 0, 255, 0);

	
		//Visualizar Imagen
		//cv::imshow("Depth Image", depthImageNative);
		cv::imshow("Color Image", imageColor);
		//cv::imshow("Gray Image", imageGray);

		//Esperar ENTER
		if(cv::waitKey(1) == 27) break;
	}
	

	return 0;
}


int initializeCamaraSR300(int index) {

	//Verificar dispositivo(s) conectados
	if (!ctx.get_device_count()){return -1;}

	//Crear dispositivo
	sr300Device = ctx.get_device(index);

	//Verificar obtención de la cámara
	if (sr300Device == NULL) { return -1;};

	//Factor scalar
	scalar = sr300Device->get_depth_scale();

	//Habilitar transmision
	sr300Device->enable_stream(rs::stream::depth, imageSize.width, imageSize.height, rs::format::z16, 30);
	sr300Device->enable_stream(rs::stream::color, imageSizeColorHD.width, imageSizeColorHD.height, rs::format::bgr8, 30);

	return 0;

}

void SegmentarDepthImage(cv::Mat &imageDepth, cv::Mat &imageColor) {

	for (int y = 0; y < imageDepth.size().height; y++)
	{
		for (int x = 0; x < imageDepth.size().width; x++)
		{
			if (imageDepth.at<uint16_t>(y, x) > 3000 || imageDepth.at<uint16_t>(y, x) == 0)
			{
				imageDepth.at<uint16_t>(y, x) = 0;
				imageColor.at<cv::Vec3b>(y, x) = 0;
			}
			
		}
	}
	return;
}

void drawCountorToImageColor(cv::Mat &imageGray, cv::Mat &imageColor, cv::Mat &imageDepth, int R, int G, int B) {

	//Dibujar contornos
	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;

	cv::findContours(imageGray, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

	for (size_t i = 0; i < contours.size(); i++)
	{
		cv::Scalar color = cv::Scalar(B, G, R);
		drawContours(imageColor, contours, (int)i, color, 2, 8, hierarchy, 0, cv::Point());
	}

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
	
	int t = mc.size();
	double *area = new double[t];

	for (int i = 0; i < t; i++)
	{
		if (t < 6)
		{
			if (mc[i].x > 0)
			{
				float distanceZ = getDistance(mc[i], imageDepth, scalar);
				float a = distanceZ * tan((72.5 / 2) * PI / 180.0);
				float l = distanceZ * tan((57.0 / 2.0)*PI / 180.0);
				area[i] = cv::contourArea(contours[i]);
				area[i] = (2.0*a / 640.0)*(2.0*l / 480.0)*area[i];

				cv::circle(imageColor, mc[i], 2, cv::Scalar(0, 255, 0));
				cv::rectangle(imageColor, cv::Point(510, i * 30 + 5), cv::Point(630, i * 30 + 30), cv::Scalar(255, 255, 0), -1, 8);
				cv::putText(imageColor, to_string(i + 1), mc[i], CV_FONT_NORMAL, 1, cv::Scalar(255, 0, 0), 2, 8);

				cv::putText(imageColor, to_string(i + 1) + ": " + to_string((int)area[i]) + " cm2", cv::Point(i + 510, i * 30 + 28), CV_FONT_NORMAL, 0.6, cv::Scalar(255, 0, 0), 1, 8);
				//cv::Point(i + 510, i * 30 + 28)
			}
		
	
		}
		
	}

	delete[] area;


}

double getDistance(cv::Point centerPoint, cv::Mat &imageAlignDepth, float scale) {

	double distance;

	distance = imageAlignDepth.at<uint16_t>(centerPoint) * scale * 100;

	return distance;

}