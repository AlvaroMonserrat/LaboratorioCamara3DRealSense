/*
Definiciones de funciones utilizadas para el análisis de Imagen con cámara Realsense
y Librería OpenCV 3.1.0
*/

#include "RSwound.h"

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


int skinAnalisis() {

	/*Leer Imagenes*/
	cv::Mat imageColorBGR = cv::imread("imageColor.png", cv::ImreadModes::IMREAD_COLOR);
	cv::Mat imageDepthU16 = cv::imread("ImageDepth.png", cv::ImreadModes::IMREAD_UNCHANGED);

	/*Verificar que las imagenes no esten vacias*/
	if (imageColorBGR.empty()) return -1;
	if (imageDepthU16.empty()) return -1;

	/*Aplicar Adaptive Bilateral Filter*/
	cv::Mat imageWoundSkin;
	cv::bilateralFilter(imageColorBGR, imageWoundSkin, 15, 100, 10);

	//Visualizar Imagen a Color
	cv::imshow("Imagen NO Analizada", imageColorBGR);
	cv::imshow("Imagen Analizada", imageWoundSkin);

	return 0;
}



