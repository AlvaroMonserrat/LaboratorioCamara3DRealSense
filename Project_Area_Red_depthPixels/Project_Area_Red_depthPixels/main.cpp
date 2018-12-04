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


//Objetos Camara
rs::context ctx;
rs::device *sr300Device = NULL;

//Dimensión de la imagen
cv::Size imageSize(640, 480);

//cv::Size imageSizeColor(1920, 1080);
cv::Size imageSizeColorHD(1280, 720);
double scale = 0;
double ka = (4 * tan((71.5 / 2.0)* PI / 180.0) * tan((55.0 / 2.0)* PI / 180.0)) / (WIDTH * HEIGHT);

/*------------------------------------------
Prototype Functions
--------------------------------------------*/
int initializeCamaraSR300(int index);//void alignImageDepth(cv::Mat &imageDepth); //Alinar Imagen de profundidad con imagen a colo
void SegmentarDepthImage(cv::Mat &imageDepth, cv::Mat &imageColor); //Eliminar fondo imagen de profundidad
int processingImagen();

cv::Mat equalizeIntensity(const cv::Mat& inputImage); //Equializar imagen
void thresholdRed(cv::Mat &imageColor);
double getAreaFigure(cv::Mat &depthImage, cv::Mat &thresholdImage);

int main() {

	//Inicializar Camaraa SR300
	if (initializeCamaraSR300(0))
		return -1;

	//Inicar Transmisión
	sr300Device->start();

	cv::namedWindow("Imagen", CV_WINDOW_FULLSCREEN);

	while (true)
	{
		//Esperar Frame
		sr300Device->wait_for_frames();

		//Declarar objecto para almacenar imagen de profundidad
		cv::Mat depthImageNative(imageSize, CV_16UC1, (uint16_t*)sr300Device->get_frame_data(rs::stream::depth));
		cv::Mat imageColor(imageSize, CV_8UC3, (uint8_t*)sr300Device->get_frame_data(rs::stream::color_aligned_to_depth));

		SegmentarDepthImage(depthImageNative, imageColor);

		cv::imshow("Imagen", imageColor);
		//cv::imshow("Depth", depthImageNative);

		//Esperar ENTER
		if (cv::waitKey(1) == 27) {
			//Guardar imagen
			cv::imwrite("imageColor.png", imageColor);
			cv::imwrite("ImageDepth.png", depthImageNative);
			break;
		}
	}

	//Cerrar vetnana
	cv::destroyWindow("Imagen");
	//Detener transmisión
	sr300Device->stop();

	processingImagen(); //Procesamiento de la imagen

	cv::waitKey(0);

	return 0;
}


int initializeCamaraSR300(int index) {

	//Verificar dispositivo(s) conectados
	if (!ctx.get_device_count()) { return -1; }

	//Crear dispositivo
	sr300Device = ctx.get_device(index);

	//Verificar obtención de la cámara
	if (sr300Device == NULL) { return -1; };

	//Factor scalar
	scale = sr300Device->get_depth_scale();

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
				imageColor.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
			}

		}
	}
	return;
}

int processingImagen() {

	double areaFigure = 0;
	cv::Mat imageColor = cv::imread("imageColor.png", cv::IMREAD_COLOR);
	cv::Mat imageDepthU16 = cv::imread("ImageDepth.png", cv::ImreadModes::IMREAD_UNCHANGED);

	if (imageColor.empty())
		return -1;

	if (imageDepthU16.empty())
		return -1;

	/*Detectar color "Rojo"*/
	cv::Mat imageColorBGR = imageColor.clone(); //Clonar Imagen

	//imageColorBGR = equalizeIntensity(imageColorBGR);
	//cv::GaussianBlur(imageColorBGR, imageColorBGR, cv::Size(5, 5), 0, 0);
	cv::Mat dst;
	cv::bilateralFilter(imageColorBGR, dst, 20, 80, 150);

	//Umbralizar rojo
	thresholdRed(dst);

	areaFigure = getAreaFigure(imageDepthU16, dst);



	cv::imshow("Imagen procesada", dst);
	cv::imshow("Imagen a Color", imageColor);
	cv::imshow("Imagen de Profundidad", imageDepthU16);

	return 0;

}

cv::Mat equalizeIntensity(const cv::Mat& inputImage)
{
	if (inputImage.channels() >= 3)
	{
		cv::Mat ycrcb;

		cvtColor(inputImage, ycrcb, CV_BGR2YCrCb);

		vector<cv::Mat> channels;
		split(ycrcb, channels);

		equalizeHist(channels[0], channels[0]);

		cv::Mat result;
		merge(channels, ycrcb);

		cvtColor(ycrcb, result, CV_YCrCb2BGR);

		return result;
	}
	return cv::Mat();
}

void thresholdRed(cv::Mat &imageColor) {
	
	//cv::cvtColor(imageColor, imageColor, CV_BGR2YUV);
	//threshold Red
	cv::inRange(imageColor, cv::Scalar(0, 0, 120), cv::Scalar(80, 150, 255), imageColor);

	//vector<cv::Mat> channels;
	//cv::split(imageColor, channels);

	//equalizeHist(channels[2], channels[2]);
	//cv::inRange(channels[2], cv::Scalar(75, 75, 75), cv::Scalar(115, 115, 115), channels[2]);
	//cv::threshold(channels[2], channels[2], 75.0, 115.0, CV_THRESH_BINARY_INV);
	//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
	int erosion_size = 3;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
	dilate(imageColor, imageColor, element);
	erode(imageColor, imageColor, element);


}

double getAreaFigure(cv::Mat &depthImage, cv::Mat &thresholdImage) {

	

	//Dibujar contornos
	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;

	cv::findContours(thresholdImage.clone(), contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_NONE, cv::Point(0, 0));
	
	vector<double> areaTotal(contours.size());
	//Puntos interior de la figura
	vector<vector<cv::Point> > points(contours.size());


	//Leer puntos interior de la figura y almacenar su posicion
	for (int n = 0; n < contours.size(); n++)
	{
		for (int i = 0; i < thresholdImage.rows; i++)
		{
			for (int j = 0; j < thresholdImage.cols; j++)
			{
				if (cv::pointPolygonTest(contours[n], cv::Point(i, j), false) > 0)
				{
					points[n].push_back(cv::Point(i, j));
				}
			}
		}
	}


	for (int m = 0; m < contours.size(); m++)
	{
		for (int k = 0; k < points[m].size(); k++)
		{
			areaTotal[m] += pow((double)depthImage.at<uint16_t>(points[m].at(k))* scale * 100.0, 2);

		}
		printf("EL area %d es: %g\n", m , areaTotal[m] * ka);
		//cv::putText(thresholdImage, to_string(m), points[m].at(m), CV_FONT_NORMAL, 1, cv::Scalar(150, 1, 0), 2, 8);
	}

	
	printf("El total de puntos %d\n", points[0].size());

	return (double)areaTotal.size();
}