#include<iostream>
#include<opencv2\core\core.hpp>
#include<opencv2\highgui\highgui.hpp>
#include<opencv2\imgproc\imgproc.hpp>
#include <vector>
#include <math.h>

#define PI 3.14159265

using namespace std;


double drawCountorToImageColor(cv::Mat &imageThreshold, cv::Mat &imageColor, cv::Mat &depth_image, int R, int G, int B); //Dibujar todos los contornos
void foreGroundImage(cv::Mat &depth_image, cv::Mat &bgr_image); //Segmentar frente
double boxSegmentation(cv::Mat &bgr_image, cv::Mat &depth_image);
double redSegmentation(cv::Mat &bgr_image, cv::Mat &image_color, cv::Mat &depth_image);
double yelloSegmentation(cv::Mat &bgr_image, cv::Mat &image_color, cv::Mat &depth_image);
double blackSegmentation(cv::Mat &bgr_image, cv::Mat &image_color, cv::Mat &depth_image);
double getDistance(cv::Point centerPoint, cv::Mat &imageAlignDepth, double scale);	//obtener distancia
double skinDetection(cv::Mat &imageThreshold, cv::Mat &imageColor, cv::Mat &depth_image, int R, int G, int B); //Deteccion de piel

double scalar = 0.000125;


void conv2(cv::Mat src, int kernel_size)
{
	cv::Mat dst, kernel;
	kernel = cv::Mat::ones(kernel_size, kernel_size, CV_32F) / (float)(kernel_size*kernel_size);

	/// Apply filter
	cv::filter2D(src, dst, -1, kernel, cv::Point(-1, -1), 0, cv::BORDER_REPLICATE);
	cv::namedWindow("filter2D Demo", CV_WINDOW_AUTOSIZE);imshow("filter2D Demo", dst);
}

int main() {

	//Leer imagenes BGR y DEPTH
	cv::Mat imageColorBGR = cv::imread("imageColor.png", cv::ImreadModes::IMREAD_COLOR);
	cv::Mat imageDepthU16 = cv::imread("ImageDepth.png", cv::ImreadModes::IMREAD_UNCHANGED);
	
	if (imageColorBGR.empty())
		return -1;

	if (imageDepthU16.empty())
		return -1;

	//Segmentacion usando Depth Image. Imagen recortada de resultado
	foreGroundImage(imageDepthU16, imageColorBGR);

	cv::Mat imageColorFinal = imageColorBGR.clone(); //Imagen original a mostrar
	cv::GaussianBlur(imageColorBGR, imageColorBGR, cv::Size(5, 5), 0, 0);
	
	cv::Mat imageColor = imageColorBGR.clone();
	cv::Mat imagePro = imageColorBGR.clone();

	double areaWound = boxSegmentation(imageColorBGR, imageDepthU16);

	imagePro = imageColorBGR - imageColor;

	//cv::Laplacian(imageColor, imageColor, CV_8UC3, 1);
	
	//Segmentación del Rojo
	double areaRed = redSegmentation(imagePro, imageColorFinal, imageDepthU16);
	double areaYellow = yelloSegmentation(imagePro, imageColorFinal, imageDepthU16);
	double areaBlack = blackSegmentation(imagePro, imageColorFinal, imageDepthU16);

	printf("El area del Rojo es: %g\n", areaRed);
	printf("El area del Amarillo es: %g\n", areaYellow);
	printf("El area del Negro es: %g\n", areaBlack);

	//cv::imshow("ImagenBGR", imageColorBGR);
	//cv::imshow("ImagenDepth", imageDepthU16);
	cv::imshow("Color", imageColorFinal);
	//cv::imshow("Color pro", imagePro);
	cv::waitKey(0);

	return 0;
}


/*Return Area Total type Double*/
double drawCountorToImageColor(cv::Mat &imageThreshold, cv::Mat &imageColor, cv::Mat &depth_image, int R, int G, int B) {

	//Dibujar contornos
	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;

	cv::findContours(imageThreshold, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_NONE, cv::Point(0, 0));

	
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


	double AreaTotal = 0;
	
	for (size_t i = 0; i < contours.size(); i++)
	{
		cv::Scalar color = cv::Scalar(B, G, R);
		drawContours(imageColor, contours, (int)i, color, 1, 8, hierarchy, 0, cv::Point());
		

		double distanceZ = getDistance(mc[i], depth_image, scalar);
		double a = distanceZ * tan((71.5 / 2.0)* PI / 180.0);
		double l = distanceZ * tan((55.0 / 2.0)* PI / 180.0);

		AreaTotal += (2.0*a / 640.0)*(2.0*l / 480.0) * cv::contourArea(contours[i]);
	}

	return AreaTotal;
}

/*Separa el fondo del frente utilizando la imagen de profundidad*/
void foreGroundImage(cv::Mat &depth_image, cv::Mat &bgr_image) {

	/*Clonar Depth Imagen 16-bit*/
	cv::Mat depth_Image8 = depth_image.clone();

	/*Convertir Imagen 16-bit depth to 8-bit*/
	depth_Image8.convertTo(depth_Image8, CV_8U);

	/*Smoothing*/
	cv::GaussianBlur(depth_Image8, depth_Image8, cv::Size(7, 7), 2, 0);

	/*Umbralización*/
	cv::Mat OtsuThresholdImage;
	cv::threshold(depth_Image8, OtsuThresholdImage, 0, 255, CV_THRESH_BINARY + CV_THRESH_OTSU);

	//Crear elemento para erosion y dilatacion
	int erosion_size = 8;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));

	//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
	erode(OtsuThresholdImage, OtsuThresholdImage, element);
	dilate(OtsuThresholdImage, OtsuThresholdImage, element);

	//Dibujar minimo rectangulo
	cv::Mat onlyWhitePixels;
	cv::findNonZero(OtsuThresholdImage, onlyWhitePixels);

	//Rectangulo
	cv::Rect min_Rect = cv::boundingRect(onlyWhitePixels);

	/*Imagenes Recortadas*/
	bgr_image = bgr_image(min_Rect);
	depth_image = depth_image(min_Rect);

}

//Regresa el área total del rojo
double redSegmentation(cv::Mat &bgr_image, cv::Mat &image_color, cv::Mat &depth_image) {

	cv::Mat imageRed = bgr_image.clone();

	cv::GaussianBlur(imageRed, imageRed, cv::Size(5, 5), 0, 0);
	cv::inRange(imageRed, cv::Scalar(200, 225, 100), cv::Scalar(245, 255, 160), imageRed);
	int erosion_size = 1;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
	//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
	dilate(imageRed, imageRed, element);
	erode(imageRed, imageRed, element);

	//Aumentar bordes
	dilate(imageRed, imageRed, element);

	return drawCountorToImageColor(imageRed, image_color, depth_image,  0, 255, 0);
}


//Regresa el área total del amarrillo
double yelloSegmentation(cv::Mat &bgr_image, cv::Mat &image_color, cv::Mat &depth_image) {

	cv::Mat imageRed = bgr_image.clone();

	cv::GaussianBlur(imageRed, imageRed, cv::Size(5, 5), 0, 0);
	cv::inRange(imageRed, cv::Scalar(175, 105, 75), cv::Scalar(255, 160, 120), imageRed);
	int erosion_size = 1;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
	//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
	dilate(imageRed, imageRed, element);
	erode(imageRed, imageRed, element);

	//Aumentar bordes
	dilate(imageRed, imageRed, element);

	return drawCountorToImageColor(imageRed, image_color, depth_image, 0, 255, 0);
}

//Regresa el área total del negro
double blackSegmentation(cv::Mat &bgr_image, cv::Mat &image_color, cv::Mat &depth_image) {

	cv::Mat imageRed = bgr_image.clone();

	cv::GaussianBlur(imageRed, imageRed, cv::Size(5, 5), 0, 0);
	cv::inRange(imageRed, cv::Scalar(195, 195, 195), cv::Scalar(255, 255, 255), imageRed);
	int erosion_size = 1;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
	//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
	dilate(imageRed, imageRed, element);
	erode(imageRed, imageRed, element);

	//Aumentar bordes
	dilate(imageRed, imageRed, element);

	return drawCountorToImageColor(imageRed, image_color, depth_image, 0, 255, 0);
}

double boxSegmentation(cv::Mat &bgr_image, cv::Mat &depth_image) {

	cv::Mat imageRed = bgr_image.clone();

	cv::GaussianBlur(imageRed, imageRed, cv::Size(7, 7), 0, 0);
	cv::inRange(imageRed, cv::Scalar(100, 95, 80), cv::Scalar(200, 195, 155), imageRed);
	int erosion_size = 1;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
	//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
	dilate(imageRed, imageRed, element);
	erode(imageRed, imageRed, element);

	//Aumentar bordes
	dilate(imageRed, imageRed, element);

	
	return skinDetection(imageRed, bgr_image, depth_image, 255, 255, 255);
}

/*Return Area Total type Double*/
double skinDetection(cv::Mat &imageThreshold, cv::Mat &imageColor, cv::Mat &depth_image, int R, int G, int B) {

	//Dibujar contornos
	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;

	cv::findContours(imageThreshold, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE, cv::Point(0, 0));


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


	double AreaTotal = 0;

	for (size_t i = 0; i < contours.size(); i++)
	{
		cv::Scalar color = cv::Scalar(B, G, R);
		drawContours(imageColor, contours, (int)i, color, CV_FILLED, 8, hierarchy, 0, cv::Point());


		double distanceZ = getDistance(mc[i], depth_image, scalar);
		double a = distanceZ * tan((71.5 / 2.0)* PI / 180.0);
		double l = distanceZ * tan((55.0 / 2.0)* PI / 180.0);

		AreaTotal += (2.0*a / 640.0)*(2.0*l / 480.0) * cv::contourArea(contours[i]);
	}

	return AreaTotal;
}


double getDistance(cv::Point centerPoint, cv::Mat &imageAlignDepth, double scale) {

	double distance = 0;

	if (centerPoint.x > 0 && centerPoint.y > 0) {
		distance = imageAlignDepth.at<uint16_t>(centerPoint) * scale * 100;
	}


	return distance;

}