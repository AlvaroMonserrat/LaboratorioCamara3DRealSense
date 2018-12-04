/*
Prototipos de funciones utilizadas para el an�lisis de Imagen con c�mara Realsense
y Librer�a OpenCV 3.1.0
*/

#pragma once
/*LIBRER�AS OPENCV 3.1.0*/
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\core\core.hpp>


/*LIBRER�A INTEL REALSENSE API*/
#include <rs.hpp>


/*Prototipos RSwound*/

/*	
Se debe ingresar la imagen de profundidad y la imagen de color aliniedada
Elimina Objectos mayores a 36 centimetros de distancia
*/
void SegmentarDepthImage(cv::Mat &imageDepth, cv::Mat &imageColor);

/*
An�lisis de piel
*/
int skinAnalisis();


void anisotropicDiffusion(cv::Mat &output, int width, int height);