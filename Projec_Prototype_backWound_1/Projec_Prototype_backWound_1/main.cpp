/*
Prototipo Análisis de Herida Crónica en Maqueta de Espalda Baja

-	Capturar una imagen de la lesión de interés y obtener el área de la lesión
	y su profundidad máxima. 

--------	23/01/17	------------
*/

/*LIBRERÍAS STANDARD C++*/
#include <iostream>
#include <vector>
#include <math.h>

/*LIBRERÍA REALSENSE WOUND -- INCLUYE INTEL REALSENSE API y OPENCV*/
#include "RSwound.h"


/*Macros*/
#define PI 3.14159265
#define WIDTH 640		//Ancho Imagen BGR calibrada y Depth_uint16
#define HEIGHT 480		//Alto Imagen BGR calibrada y Depth_uint16

/*---------------------------------------
			VARIABLES GLOBALES
----------------------------------------*/
cv::Size imageSize(WIDTH, HEIGHT);		 //Dimensión de la imagen Depth Stream
cv::Size imageSizeColorHD(1280, 720);	 //Dimensión de la imagen Color Stream
double factorScale = 0;					 //Factor de escala 0.00012 Normalmente (Se obtiene de la transmisión)
double kArea = (4 * tan((71.5 / 2.0)* PI / 180.0) * tan((55.0 / 2.0)* PI / 180.0)) / (WIDTH * HEIGHT);		//Constante Área de una forma
/*---------------------------------------
-----------------------------------------*/

/*Variables Globales RealSense*/
rs::context ctx;					//Contexto Intel RealSense
rs::device *sr300Device = NULL;		//Crea un puntero NULL al dispositivo


/*------------------------------------------
			Prototype Functions
--------------------------------------------*/
int initializeCamaraSR300(int index);			//Iniciarlizar Cámara Intel RealSense SR300

int main() {

	//Inicializar Camaraa SR300
	if (initializeCamaraSR300(0))
		return -1;

	sr300Device->start();	//Iniciar Transmisión
	cv::namedWindow("Imagen Wound", CV_WINDOW_FULLSCREEN);

	while (true){

		sr300Device->wait_for_frames();	//Esperar Frames

		/*Leer Frame Imagen de profundidad e Imagen a color alineada*/
		cv::Mat depthImageNative(imageSize, CV_16UC1, (uint16_t*)sr300Device->get_frame_data(rs::stream::depth));
		cv::Mat imageColor(imageSize, CV_8UC3, (uint8_t*)sr300Device->get_frame_data(rs::stream::color_aligned_to_depth));

		/*Segmentar Imagen a Color Aleneada*/
		SegmentarDepthImage(depthImageNative, imageColor);

		/*Visualizar Imagen Cámara a Color Alineada*/
		cv::imshow("Imagen Wound", imageColor);

		//Esperar ENTER
		if (cv::waitKey(1) == 27) {
			//Guardar imagen
			cv::imwrite("imageColor.png", imageColor);
			cv::imwrite("ImageDepth.png", depthImageNative);
			break;
		}
	}

	cv::destroyWindow("Imagen Wound");	//Cerrar vetnana
	sr300Device->stop();			//Detener transmisión

	/*Analizar Imagenesr Almacenadas*/
	if (skinAnalisis()) {
		std::cout << "Error en el análisis de imagen" << std::endl;
	}

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
	factorScale = (double)sr300Device->get_depth_scale();
	if (factorScale == 0)
		return -1;

	//Habilitar transmision
	sr300Device->enable_stream(rs::stream::depth, imageSize.width, imageSize.height, rs::format::z16, 30);
	sr300Device->enable_stream(rs::stream::color, imageSizeColorHD.width, imageSizeColorHD.height, rs::format::bgr8, 30);

	return 0;

}