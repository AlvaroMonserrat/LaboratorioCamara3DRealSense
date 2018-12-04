#include <iostream>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>

int main() {

	//Reading depth image and color image 
	cv::Mat imgDepth = cv::imread("imageDepth.png", CV_LOAD_IMAGE_ANYDEPTH);
	cv::Mat imgColor = cv::imread("square5x5cm.bmp", CV_LOAD_IMAGE_UNCHANGED);

	//Save value of point(300,300)
	uint16_t valor = imgDepth.at<uint16_t>(300, 300);

	//Distancia entre la camara y caja
	std::cout << "Distancia: ";
	double Z = (double)((valor * 100) / 8000);
	std::cout << Z << " cm" << std::endl;
	

	//Create a black image with the size as the camera output
	cv::Mat imgLines = cv::Mat::zeros(imgColor.size(), CV_8UC3);;

	cv::Mat imgHSV;

	//Convert the captured frame from BGR to HSV
	cv::cvtColor(imgColor, imgHSV, cv::COLOR_BGR2HSV); 


	// Threshold the HSV image, keep only the red pixels
	cv::Mat upper_red_hue_range;
	cv::inRange(imgHSV, cv::Scalar(150, 100, 100), cv::Scalar(200, 255, 255), upper_red_hue_range);

	double dataValue = 0;
	
	//count just red pixels
	for (int y = 0; y < 480; y++) {
		for (int x = 0; x < 640; x++)
		{
			if (upper_red_hue_range.at<uchar>(y, x) == 255)
			{
				dataValue++;
			}
		}
	}


	double area = (dataValue*(0.026458333)*Z) / 480;

	std::cout << "Numero de pixeles -Rojos- son : ";
	printf("%g px\n", dataValue);

	std::cout << "El area del cuadro rojo es de: " << area << " cm2" << std::endl;;

	cv::imshow("wound", imgDepth);
	cv::imshow("color", imgColor);
	cv::imshow("Thresholded Image", upper_red_hue_range); //show the thresholded image

	cv::waitKey(0);

	return 0;
}