/*Convertir Camara a color con SDK a formato Mat de OpenCV*/

#include <iostream>
#include <iomanip>
#include <string>

#include <pxcsensemanager.h>

#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>

using namespace std;

cv::Size imageSize(1280, 720);
cv::Size imageSizeDepth(640, 480);

struct pxcRGB24
{
	pxcBYTE pxcRED;
	pxcBYTE pxcGREEN;
	pxcBYTE pxcBLUE;

};

pxcRGB24 *bufferRGB = 0;

void convertPXCImage(PXCImage *image)
{
	PXCImage::ImageInfo info = image->QueryInfo();
	PXCImage::ImageData data;

	image->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_RGB24, &data);

	int bufferSize = info.width * info.height;

	bufferRGB = new pxcRGB24[bufferSize];

	memcpy_s(bufferRGB, bufferSize * sizeof(pxcRGB24), (pxcRGB24 *)data.planes[0], bufferSize * sizeof(pxcRGB24));

	image->ReleaseAccess(&data);

}



int main()
{
	//Create a SenseManager Instance
	PXCSenseManager *sm = PXCSenseManager::CreateInstance();

	//Si no se creo la session retornar un -1
	if (!sm)
		return -1;

	//Configure the components
	sm->EnableStream(PXCCapture::STREAM_TYPE_COLOR, imageSize.width, imageSize.height);
	//Entrega información de la inicialización de la camara
	if(sm->Init() != PXC_STATUS_NO_ERROR)
		return -2;

	while (true)
	{
		sm->AcquireFrame(false);

		//Retorna la muestra de datos disponible, regresa un NULL si no esta disponible
		const PXCCapture::Sample *sample = sm->QuerySample();

		if (!sample)
			return -3;

		//Crea un objeto PXCImage para guardar la direccion que apunta a la imagen a color
		PXCImage *imageColor = sample->color;
	
		//Convertir imagen en un formato conocido
		convertPXCImage(imageColor);
		/*
		PXCImage::ImageData data;
		
		imageColor->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_RGB24, &data);
		
		//Crear Imagen con libreria OpenCV
		cv::Mat imageOpenCV(imageSize, CV_8UC3, (pxcRGB24 *)data.planes[0]);
		*/

		cv::Mat imageOpenCV(imageSize, CV_8UC3, bufferRGB);


		cv::imshow("Color", imageOpenCV);


		delete[] bufferRGB;


		sm->ReleaseFrame();
		
		if(cv::waitKey(1) == 27)
			break;
	}

	//Comienza el stream y espera un que ocurra un evento

	return 0;

}
