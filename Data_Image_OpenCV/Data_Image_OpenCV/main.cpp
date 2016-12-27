#include <iostream>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>

int main() {

	cv::Mat imgDepth = cv::imread("imageDepth.png", CV_LOAD_IMAGE_ANYDEPTH);
	cv::Mat imgColor = cv::imread("wound.bmp", CV_LOAD_IMAGE_UNCHANGED);

	uint16_t valor;

	valor = imgDepth.at<uint16_t>(300, 300);

	double Z = (double)((valor * 100) / 8000);
	std::cout << Z << std::endl;




	//Create a black image with the size as the camera output
	cv::Mat imgLines = cv::Mat::zeros(imgColor.size(), CV_8UC3);;

	cv::Mat imgHSV;

	cv::cvtColor(imgColor, imgHSV, cv::COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

	cv::Mat imgThresholded;



	// Threshold the HSV image, keep only the red pixels
	cv::Mat lower_red_hue_range;
	cv::Mat upper_red_hue_range;
	cv::inRange(imgHSV, cv::Scalar(0, 100, 100), cv::Scalar(10, 255, 255), lower_red_hue_range);
	cv::inRange(imgHSV, cv::Scalar(150, 100, 100), cv::Scalar(200, 255, 255), upper_red_hue_range);

	double dataValue = 0;

	
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

	std::cout << "valor es : ";
	printf("%g\n", dataValue);

	std::cout << "El valor del area del cuadro es: " << area << std::endl;;

	cv::imshow("wound", imgDepth);
	cv::imshow("color", imgColor);
	cv::imshow("Thresholded Image", upper_red_hue_range); //show the thresholded image

	

	cv::waitKey(0);





	return 0;
}