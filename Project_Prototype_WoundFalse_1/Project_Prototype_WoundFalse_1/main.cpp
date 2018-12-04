/*
Prototypo 3: 
*/
#include <iostream>
#include <fstream>
#include <Windows.h>

#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>

#include <rs.hpp>
#include <vector>
#include <math.h>

#include "tinyxml.h"

#define PI 3.14159265

using namespace std;

/*------------------------------------------
Prototype Functions
--------------------------------------------*/
int initializeCamaraSR300(int index);//void alignImageDepth(cv::Mat &imageDepth); //Alinar Imagen de profundidad con imagen a colo
void SegmentarDepthImage(cv::Mat &imageDepth, cv::Mat &imageColor); //Eliminar fondo imagen de profundidad
int analisisImage(); //Analisis de la imagen capturada

/*Prototype Analisis*/
void foreGroundImage(cv::Mat &depth_image, cv::Mat &bgr_image); //Segmentar frente
double boxSegmentation(cv::Mat &bgr_image, cv::Mat &depth_image); //Segmentar caja
double skinDetection(cv::Mat &imageThreshold, cv::Mat &imageColor, cv::Mat &depth_image, int R, int G, int B); //Deteccion de piel
double getDistance(cv::Point centerPoint, cv::Mat &imageAlignDepth, double scale);	//obtener distancia
double redSegmentation(cv::Mat &bgr_image, cv::Mat &image_color, cv::Mat &depth_image);
double yelloSegmentation(cv::Mat &bgr_image, cv::Mat &image_color, cv::Mat &depth_image);
double blackSegmentation(cv::Mat &bgr_image, cv::Mat &image_color, cv::Mat &depth_image);
double drawCountorToImageColor(cv::Mat &imageThreshold, cv::Mat &imageColor, cv::Mat &depth_image, int R, int G, int B); //Dibujar todos los contornos
bool write_simple_doc(double areaTotal, double areaRed, double areaYellow, double areaBlack);
void CreateFolder(const char * path);

 //Objetos Camara
rs::context ctx;
rs::device *sr300Device = NULL;

//Dimensión de la imagen
cv::Size imageSize(640, 480);

//cv::Size imageSizeColor(1920, 1080);
cv::Size imageSizeColorHD(1280, 720);
double scale = 0;

int main() {

	//Inicializar Camaraa SR300
	if(initializeCamaraSR300(0))
		return -1;

	//Inicar Transmisión
	sr300Device->start();
	
	

	Sleep(1000);
		//Esperar Frame
		sr300Device->wait_for_frames();

		//Declarar objecto para almacenar imagen de profundidad
		cv::Mat depthImageNative(imageSize, CV_16UC1, (uint16_t*)sr300Device->get_frame_data(rs::stream::depth));
		cv::Mat imageColor(imageSize, CV_8UC3, (uint8_t*)sr300Device->get_frame_data(rs::stream::color_aligned_to_depth));

		SegmentarDepthImage(depthImageNative, imageColor);

		

			//Guardar imagen
			cv::imwrite("imageColor.png", imageColor);
			cv::imwrite("ImageDepth.png", depthImageNative);

	
	//Detener transmisión
	sr300Device->stop();

	
	//Analisis de la imagen
	analisisImage();

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
				imageColor.at<cv::Vec3b>(y, x) = cv::Vec3b(0,0,0);
			}

		}
	}
	return;
}

int analisisImage() {

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
	//cv::GaussianBlur(imageColorBGR, imageColorBGR, cv::Size(5, 5), 0, 0);

	cv::Mat imageColor = imageColorBGR.clone();
	cv::Mat imagePro = imageColorBGR.clone();
	cv::bilateralFilter(imageColorFinal, imageColor, 15, 80, 20);
	double areaWound = boxSegmentation(imageColorBGR, imageDepthU16);

	imagePro = imageColorBGR - imageColor;

	//cv::Laplacian(imageColor, imageColor, CV_8UC3, 1);

	//Segmentación del Rojo
	double areaRed = redSegmentation(imagePro, imageColorFinal, imageDepthU16);
	double areaYellow = yelloSegmentation(imageColor, imageColorFinal, imageDepthU16);
	//double areaBlack = blackSegmentation(imagePro, imageColorFinal, imageDepthU16);

	double areaBlack = 0;
	double areaTotal = areaRed + areaYellow + areaBlack;
	
	printf("El area total de la figura es: %g\n", areaTotal);
	printf("----------------------------\n");
	printf("El area del Rojo es: %g\n", areaRed);
	printf("El area del Amarillo es: %g\n", areaYellow);
	printf("El area del Negro es: %g\n", areaBlack);
	printf("----------------------------\n");
	printf("----------------------------\n");
	cout << "Porcentaje Rojo = " << (areaRed * 100) / areaTotal << "%" << endl;
	cout << "Porcentaje Amarillo = " << (areaYellow * 100) / areaTotal << "%" << endl;
	cout << "Porcentaje Negro = " << (areaBlack * 100) / areaTotal << "%" << endl;

	//cv::imshow("ImagenBGR", imageColorBGR);
	//cv::imshow("ImagenDepth", imageDepthU16);
	cv::Mat dst;
	cv::bilateralFilter(imageColorFinal, dst, 15, 80, 20);
	cv::imshow("ImagenBGR", imagePro);
	cv::imshow("Imagen Analizada", dst);

	if (write_simple_doc(areaTotal, areaRed, areaYellow, areaBlack))
	{
		//cv::imwrite("C:\\Image_Nurseye\\\imageColor.png", imageColorBGR);
		cv::imwrite("C:\\Image_Nurseye\\\ImageFinal.png", dst);
	}

	return 0;
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

double boxSegmentation(cv::Mat &bgr_image, cv::Mat &depth_image) {

	cv::Mat imageRed = bgr_image.clone();

	cv::GaussianBlur(imageRed, imageRed, cv::Size(7, 7), 0, 0);
	cv::inRange(imageRed, cv::Scalar(80, 85, 80), cv::Scalar(210, 205, 210), imageRed);
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


		double distanceZ = getDistance(mc[i], depth_image, scale);
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

//Regresa el área total del rojo
double redSegmentation(cv::Mat &bgr_image, cv::Mat &image_color, cv::Mat &depth_image) {

	cv::Mat imageRed = bgr_image.clone();

	/*
	cv::GaussianBlur(imageRed, imageRed, cv::Size(5, 5), 0, 0);
	cv::inRange(imageRed, cv::Scalar(110, 160, 0), cv::Scalar(255, 255, 130), imageRed);
	int erosion_size = 1;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
	//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
	dilate(imageRed, imageRed, element);
	erode(imageRed, imageRed, element);
	*/
	//Aumentar bordes
	//dilate(imageRed, imageRed, element);

	cv::cvtColor(imageRed, imageRed, CV_BGR2YCrCb);
	//threshold Red
	//cv::inRange(imageColor, cv::Scalar(0, 0, 100), cv::Scalar(70, 80, 200), imageColor);
	vector<cv::Mat> channels;
	split(imageRed, channels);

	cv::threshold(channels[1], channels[1], 0, 255, CV_THRESH_BINARY_INV + CV_THRESH_OTSU);
	//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
	int erosion_size = 3;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
	dilate(channels[1], channels[1], element);
	erode(channels[1], channels[1], element);
	//cv::imshow("Canal 0", channels[0]);
	//cv::imshow("Canal 1", channels[1]);
	//cv::imshow("Canal 2", channels[2]);
	imageRed = channels[1];

	return drawCountorToImageColor(imageRed, image_color, depth_image, 0, 255, 0);
}


//Regresa el área total del amarrillo
double yelloSegmentation(cv::Mat &bgr_image, cv::Mat &image_color, cv::Mat &depth_image) {

	cv::Mat imageRed = bgr_image.clone();
	/*
	cv::GaussianBlur(imageRed, imageRed, cv::Size(5, 5), 0, 0);
	cv::inRange(imageRed, cv::Scalar(125, 60, 40), cv::Scalar(180, 135, 100), imageRed);
	int erosion_size = 1;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
	//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
	dilate(imageRed, imageRed, element);
	erode(imageRed, imageRed, element);
	*/
	//Aumentar bordes
	//dilate(imageRed, imageRed, element);
	cv::cvtColor(imageRed, imageRed, CV_BGR2YUV);
	//threshold Red
	//cv::inRange(imageColor, cv::Scalar(0, 0, 100), cv::Scalar(70, 80, 200), imageColor);
	vector<cv::Mat> channels;
	split(imageRed, channels);

	cv::inRange(channels[2], cv::Scalar(75, 75, 75), cv::Scalar(115, 115, 115), channels[2]);
	//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
	int erosion_size = 3;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
	dilate(channels[2], channels[2], element);
	erode(channels[2], channels[2], element);
	//cv::imshow("Canal 0", channels[0]);
	//cv::imshow("Canal 1", channels[1]);
	//cv::imshow("Canal 2", channels[2]);
	//cv::imshow("iamgen", imageRed);
	imageRed = channels[2];
	return drawCountorToImageColor(imageRed, image_color, depth_image, 255, 255, 255);
}

//Regresa el área total del negro
double blackSegmentation(cv::Mat &bgr_image, cv::Mat &image_color, cv::Mat &depth_image) {

	cv::Mat imageRed = bgr_image.clone();

	cv::GaussianBlur(imageRed, imageRed, cv::Size(5, 5), 0, 0);
	cv::inRange(imageRed, cv::Scalar(180, 180, 180), cv::Scalar(225, 225, 225), imageRed);
	int erosion_size = 1;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
	//Erosionar y dilatar (Filtro para eliminar particulas pequeñas)
	dilate(imageRed, imageRed, element);
	erode(imageRed, imageRed, element);

	//Aumentar bordes
	dilate(imageRed, imageRed, element);

	return drawCountorToImageColor(imageRed, image_color, depth_image, 0, 255, 0);
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


		double distanceZ = getDistance(mc[i], depth_image, scale);
		double a = distanceZ * tan((71.5 / 2.0)* PI / 180.0);
		double l = distanceZ * tan((55.0 / 2.0)* PI / 180.0);

		AreaTotal += (2.0*a / 640.0)*(2.0*l / 480.0) * cv::contourArea(contours[i]);
	}

	return AreaTotal;
}

bool write_simple_doc(double areaTotal, double areaRed, double areaYellow, double areaBlack) {


	
	TiXmlDocument doc;
	TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
	doc.LinkEndChild(decl);

	TiXmlElement *analisis = new TiXmlElement("Analisis");
	doc.LinkEndChild(analisis);

	TiXmlElement *wound = new TiXmlElement("Herida");
	analisis->LinkEndChild(wound);

	/*Area Total*/
	TiXmlElement *at = new TiXmlElement("at"); //Área Total
	wound->LinkEndChild(at);

	//
	string area_total = to_string(areaTotal);
	const char *pTotal;
	pTotal = area_total.c_str();

	TiXmlText *text_at = new TiXmlText(pTotal);  //Área Herida total
	at->LinkEndChild(text_at);

	/*Area rojo*/
	TiXmlElement *ar = new TiXmlElement("ar"); //Área rojo
	wound->LinkEndChild(ar);

	//
	string area_rojo= to_string(areaRed);

	TiXmlText *text_ar = new TiXmlText(area_rojo.c_str());  //Área Herida total rojo
	ar->LinkEndChild(text_ar);


	/*Area amarillo*/
	TiXmlElement *aa = new TiXmlElement("aa"); //Área amarillo
	wound->LinkEndChild(aa);

	//
	string area_amarrillo = to_string(areaYellow);

	TiXmlText *text_aa = new TiXmlText(area_amarrillo.c_str());  //Área Herida total amarillo
	aa->LinkEndChild(text_aa);

	/*Area negro*/
	TiXmlElement *an = new TiXmlElement("an"); //Área negro
	wound->LinkEndChild(an);

	//
	string area_negro = to_string(areaBlack);

	TiXmlText *text_an = new TiXmlText(area_negro.c_str());  //Área Herida total negro
	an->LinkEndChild(text_an);

	/*Porcentaje rojo*/
	TiXmlElement *pr = new TiXmlElement("pr"); //Porcentaje rojo
	wound->LinkEndChild(pr);

	TiXmlText *text_pr = new TiXmlText("40 %");  //Porcentaje rojo
	pr->LinkEndChild(text_pr);

	/*Porcentaje Amarillo*/
	TiXmlElement *pa = new TiXmlElement("pa"); //Porcentaje Amarillo
	wound->LinkEndChild(pa);

	TiXmlText *text_pa = new TiXmlText("30 %");  //Porcentaje Amarillo
	pa->LinkEndChild(text_pa);

	/*Porcentaje Negro*/
	TiXmlElement *pm = new TiXmlElement("pm"); //Porcentaje Negro (pa)
	wound->LinkEndChild(pm);

	TiXmlText *text_pm = new TiXmlText("30 %");  //Porcentaje Negro
	pm->LinkEndChild(text_pm);

	CreateFolder("C:\\Image_Nurseye\\");

	if (doc.SaveFile("C:\\Image_Nurseye\\\wound.xml"))
	{
		return true;
	}
	return false;

}

void CreateFolder(const char * path)
{
	if (!CreateDirectory(path, NULL))
	{
		return;
	}
}
