#include <iostream>
#include <vector>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>

using namespace cv;
using namespace std;


int main()
{

	Mat imgColor = imread("imageColor.png", IMREAD_COLOR);
	Mat imgHSV;
	Mat imgBinary;

	if (imgColor.empty()) { cout << "Error al Leer la imagen\n";	getchar();
		return -1;
	}

	//Convertir de espacio de colores BGR a HSV;
	cvtColor(imgColor, imgHSV, COLOR_BGR2HSV);

	inRange(imgHSV, Scalar(150, 100, 100), Scalar(200, 255, 255), imgBinary);

	//imshow("ImageColorBGR", imgHSV);
	imshow("ImageBinary", imgBinary);

	
	int erosion_size = 10;
	Mat element = getStructuringElement(MORPH_ELLIPSE, Size(2 * erosion_size + 1, 2 * erosion_size + 1), Point(erosion_size, erosion_size));

	erode(imgBinary, imgBinary, element);
	dilate(imgBinary, imgBinary, element);

	Mat Points;
	findNonZero(imgBinary, Points);

	/*
	for (int i = 0; i < Points.total(); i++) {
		cout << "Zero#" << i << ": " << Points.at<Point>(i).x << ", " << Points.at<Point>(i).y << endl;
	}
	*/


	Rect Min_Rect = boundingRect(Points);

	int areaRect = 0;
	areaRect = Min_Rect.area();
	int width = Min_Rect.width;
	int height = Min_Rect.height;

	rectangle(imgColor, Min_Rect.tl(), Min_Rect.br(), Scalar(0, 255, 0), 1);

	imshow("ImageBinary", imgBinary);
	imshow("Result", imgColor);

	cout << "El area es: " << areaRect << endl;
	cout << "EL ancho es: " << width << " por el largo :" << height << endl;

	waitKey(0);

	return 0;
}