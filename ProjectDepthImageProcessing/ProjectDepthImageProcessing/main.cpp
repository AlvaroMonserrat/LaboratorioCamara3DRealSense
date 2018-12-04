/*Procesamiento Imagen de profundidad*/

#include <iostream>

/*Libreria OpenCV 3.1.0*/
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>

/*Liberia Intel RealSense API*/
#include <rs.hpp>

//Macro 640x48
#define WIDTH 640
#define HEIGHT 480

cv::Mat RsImage2CVMat(const void* rsImage, rs::format formato) {

	int width = WIDTH;
	int height = HEIGHT;
	int type;

	if (formato == rs::format::rgb8)
	{
		type = CV_8UC3;
		uint8_t* imageData = (uint8_t *)rsImage;
		cv::Mat imageConvert = cv::Mat(cv::Size(width, height), type, imageData);
		return imageConvert;
	}
	else if (formato == rs::format::z16) {
		type = CV_16UC1;

		uint16_t* imageData = (uint16_t *)rsImage;
		cv::Mat imageConvert = cv::Mat(cv::Size(width, height), type, imageData);
		return imageConvert;
	}

}


int main() {
	
	const void *ptrImageDepth = NULL;

	//
	rs::context ctx;

	if (ctx.get_device_count() == 0)
		return -1;

	rs::device *sr300Camara = ctx.get_device(0);

	if (sr300Camara == NULL)
		return -1;

	sr300Camara->enable_stream(rs::stream::depth, WIDTH, HEIGHT, rs::format::z16, 30);
	sr300Camara->start();

	bool key = false;

	//Crear Objeto Imagen Depth
	cv::Mat imageDepth = cv::Mat::zeros(480, 640, CV_16UC1);
	
	cv::namedWindow("Depth Stream", CV_WINDOW_AUTOSIZE);

	float scale = sr300Camara->get_depth_scale();
	printf("Factor de escala: %0.8f\n", scale);

	/*
	while (!key)
	{
		sr300Camara->wait_for_frames();

		ptrImageDepth = sr300Camara->get_frame_data(rs::stream::depth);
		
		imageDepth = RsImage2CVMat(ptrImageDepth, rs::format::z16);
		
		cv::imshow("Depth Stream", imageDepth);

		if (cv::waitKey(1) == 27)
		{
			key = true;;
		}
	}
	*/

	sr300Camara->wait_for_frames();

	ptrImageDepth = sr300Camara->get_frame_data(rs::stream::depth);

	imageDepth = RsImage2CVMat(ptrImageDepth, rs::format::z16);

	
	int histSize = 128;
	float range[] = { 0, 65535 };
	const float* histRange = { range };

	cv::Mat image_hist;

	cv::calcHist(&imageDepth, 1, 0, cv::Mat(), image_hist, 1, &histSize, &histRange, true, false);

	int hist_w = 512;
	int hist_h = 400;

	int bin_w = cvRound((double)hist_w / histSize);

	cv::Mat histImage(hist_h, hist_w, CV_8UC1, cv::Scalar(0, 0, 0));
	//Normalize

	//max and min value of the histogram
	double max_value = 0, min_value = 0;

	cv::minMaxLoc(imageDepth, &min_value, &max_value);

	cv::normalize(image_hist, image_hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat());

	for (int i = 1; i < histSize; i++) {

		cv::line(histImage, cv::Point(bin_w * (i - 1), hist_h - cvRound(image_hist.at<float>(i - 1))), cv::Point(bin_w * (i), hist_h - cvRound(image_hist.at<float>(i))), cv::Scalar(255, 0, 0), 2, 8, 0);
	}

	printf("Valor max: %g y valor min: %g", max_value, min_value);
	
	cv::imshow("hist", histImage);
	cv::imshow("Depth Stream", imageDepth);


	cv::waitKey(0);
	
	

	return 0;
}