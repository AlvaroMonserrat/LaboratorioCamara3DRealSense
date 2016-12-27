#pragma once
#include <cstdio> 
#include <FreeImage.h>
#include <glfw3.h> 

//Guardar una imagen en formato Bmp
bool SaveImageToBmp(unsigned char *image, char* nameFile, int &width, int &height);

bool SaveImageDepth(unsigned char *image_depth, char* nameFile, int &width, int &height);


//Mostrar imagen guardada
int showImageBmp();

//Leer Imagen de un archivo .bmp
unsigned char* readBMP(char* filename);

